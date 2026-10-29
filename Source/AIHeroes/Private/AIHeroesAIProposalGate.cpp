#include "AIHeroesAIProposalGate.h"

#include "AIHeroesTravelRules.h"
#include "AIHeroesFogRules.h"

namespace
{
FAIHeroesProposalVerdict Rejected(const TCHAR* Why)
{
    FAIHeroesProposalVerdict Result;
    Result.Reason = Why;
    return Result;
}

FAIHeroesProposalVerdict Accepted(bool bNeedsConfirmation, const TCHAR* Why)
{
    FAIHeroesProposalVerdict Result;
    Result.bAccepted = true;
    Result.bPlayerConfirmationRequired = bNeedsConfirmation;
    Result.Reason = Why;
    return Result;
}

bool PositionIsSafe(const FVector& Point)
{
    // TravelCost uses CeilToInt; reject enormous and nonfinite inputs first.
    const float MaxPosition = 1000000.f;
    return FMath::IsFinite(Point.X) && FMath::IsFinite(Point.Y) &&
        FMath::IsFinite(Point.Z) &&
        FMath::Abs(Point.X) <= MaxPosition &&
        FMath::Abs(Point.Y) <= MaxPosition &&
        FMath::Abs(Point.Z) <= MaxPosition;
}

bool HasGuard(const FAIHeroesAdvisorNode& Node)
{
    for (const FAIHeroesArmyStack& Stack : Node.Guards)
    {
        if (Stack.IsAlive()) return true;
    }
    return false;
}

bool HasArmy(const TArray<FAIHeroesArmyStack>& Army)
{
    for (const FAIHeroesArmyStack& Stack : Army)
    {
        if (Stack.IsAlive()) return true;
    }
    return false;
}

bool ValidUnit(const FAIHeroesArmyStack& Unit)
{
    return !Unit.UnitId.IsNone() && Unit.UnitHealth > 0 &&
        Unit.UnitHealth <= 500000 && Unit.TotalHealth >= 0 &&
        Unit.TotalHealth <= 500000 && Unit.Attack >= 0 &&
        Unit.Attack <= 100000 && Unit.Defense >= 0 &&
        Unit.Defense <= 100000 && Unit.Initiative >= 0 &&
        Unit.Initiative <= 100000;
}

bool ValidNodeType(EAIHeroesNodeType Type)
{
    switch (Type)
    {
        case EAIHeroesNodeType::Headquarters:
        case EAIHeroesNodeType::ForestMine:
        case EAIHeroesNodeType::OreMine:
        case EAIHeroesNodeType::Outpost:
        case EAIHeroesNodeType::Ruins:
            return true;
        default:
            return false;
    }
}

bool ValidSnapshot(const FAIHeroesProposalSnapshot& S)
{
    const FAIHeroesAdvisorSnapshot& A = S.Strategic;
    if (!FAIHeroesTravelRules::IsSupported(A.TravelRuleVersion) ||
        S.Day < 1 || S.StateRevision == 0 ||
        A.ProtocolTier < 0 || A.ProtocolTier > 3 ||
        S.ModelSlotTier < 0 || S.ModelSlotTier > 2 ||
        A.MovesLeft < 0 || A.MovesLeft > 4 ||
        A.Gold < 0 || A.Wood < 0 || A.Ore < 0 ||
        A.Nodes.Num() > FAIHeroesAIProposalGate::MaxVisibleNodes ||
        A.PlayerArmy.Num() > FAIHeroesAIProposalGate::MaxArmyStacks ||
        !PositionIsSafe(A.HeroPosition) ||
        (!A.bFogEnabled && A.ExploredSitesMask != 0) ||
        (A.bFogEnabled &&
            ((A.ExploredSitesMask & FAIHeroesFogRules::InitialMask) !=
                FAIHeroesFogRules::InitialMask ||
             (A.ExploredSitesMask & ~FAIHeroesFogRules::AllMask) != 0 ||
             (!A.SelectedNodeId.IsNone() &&
              !FAIHeroesFogRules::IsKnown(true, A.ExploredSitesMask,
                  A.SelectedNodeId)))))
    {
        return false;
    }
    for (const FAIHeroesArmyStack& Stack : A.PlayerArmy)
    {
        if (!ValidUnit(Stack)) return false;
    }
    TSet<FName> Known;
    for (const FAIHeroesAdvisorNode& Node : A.Nodes)
    {
        if (Node.Id.IsNone() || Known.Contains(Node.Id) ||
            !ValidNodeType(Node.Type) || !PositionIsSafe(Node.Location) ||
            Node.Guards.Num() > FAIHeroesAIProposalGate::MaxArmyStacks)
        {
            return false;
        }
        Known.Add(Node.Id);
        for (const FAIHeroesArmyStack& Guard : Node.Guards)
        {
            if (!ValidUnit(Guard)) return false;
        }
    }
    return true;
}
}

FAIHeroesAIProposal FAIHeroesAIProposalGate::SuggestSelected(
    const FAIHeroesProposalSnapshot& Snapshot)
{
    FAIHeroesAIProposal P;
    P.Source = EAIHeroesProposalSource::OfflineRules;
    P.ObservedDay = Snapshot.Day;
    P.ObservedRevision = Snapshot.StateRevision;
    P.Explanation = TEXT("Select a visible strategic node for an offline proposal.");

    if (!ValidSnapshot(Snapshot) ||
        Snapshot.Strategic.SelectedNodeId.IsNone() || Snapshot.bBattleActive)
    {
        return P; // NO_ACTION; no implicit strategy selection.
    }

    const FAIHeroesAdvisorNode* Selected = nullptr;
    for (const FAIHeroesAdvisorNode& Node : Snapshot.Strategic.Nodes)
    {
        if (Node.Id == Snapshot.Strategic.SelectedNodeId)
        {
            Selected = &Node;
            break;
        }
    }
    if (!Selected || !FAIHeroesFogRules::IsKnown(
            Snapshot.Strategic.bFogEnabled,
            Snapshot.Strategic.ExploredSitesMask, Snapshot.Strategic.SelectedNodeId))
    {
        P.Explanation = TEXT("Selected node is not discovered or visible in the current snapshot.");
        return P;
    }
    if (Snapshot.Strategic.ProtocolTier == 0)
    {
        P.Kind = EAIHeroesProposalKind::Advice;
        P.Explanation = TEXT("SEED can give basic observations; typed orders unlock with analysis.");
        return P;
    }

    P.Kind = EAIHeroesProposalKind::ProposeOrder;
    P.TargetNode = Selected->Id;
    const bool bHostile = !Selected->bOwned && HasGuard(*Selected);
    P.Order = bHostile ? EAIHeroesProposalOrder::AttackNode :
        EAIHeroesProposalOrder::MoveToNode;
    P.Explanation = bHostile
        ? TEXT("Suggest considering an attack; the player must explicitly confirm.")
        : TEXT("Suggest considering a move; the player must explicitly confirm.");
    return P;
}

FAIHeroesProposalVerdict FAIHeroesAIProposalGate::Validate(
    const FAIHeroesProposalSnapshot& S,
    const FAIHeroesAIProposal& P)
{
    if (!ValidSnapshot(S))
    {
        return Rejected(TEXT("Invalid or oversized game snapshot."));
    }
    if (P.ObservedDay != S.Day || P.ObservedRevision != S.StateRevision)
    {
        return Rejected(TEXT("Stale day or state revision."));
    }
    if (P.Explanation.IsEmpty() ||
        P.Explanation.Len() > MaxExplanationChars ||
        P.Explanation.TrimStartAndEnd().IsEmpty())
    {
        return Rejected(TEXT("Missing or excessive explanation."));
    }
    for (int32 Index = 0; Index < P.Explanation.Len(); ++Index)
    {
        const TCHAR C = P.Explanation[Index];
        if (C < 0x20 || C == 0x7f || C == 0x2028 || C == 0x2029)
        {
            return Rejected(TEXT("Control characters not allowed in proposal text."));
        }
    }

    switch (P.Source)
    {
        case EAIHeroesProposalSource::OfflineRules:
            if (P.ClaimedModelSlotTier != INDEX_NONE)
            {
                return Rejected(TEXT("Offline policy must not impersonate a loaded model."));
            }
            break;
        case EAIHeroesProposalSource::LocalModel:
            if (!S.bLocalModelReady ||
                P.ClaimedModelSlotTier != S.ModelSlotTier)
            {
                return Rejected(TEXT("Local model checkpoint not verified for this slot."));
            }
            break;
        case EAIHeroesProposalSource::RemoteModel:
            if (!S.bRemoteModelReady || !S.bRemoteInferenceApproved ||
                P.ClaimedModelSlotTier != S.ModelSlotTier)
            {
                return Rejected(TEXT("Remote inference is not connected and explicitly approved."));
            }
            break;
        default:
            return Rejected(TEXT("Unknown proposal source."));
    }

    if (P.Kind == EAIHeroesProposalKind::Advice ||
        P.Kind == EAIHeroesProposalKind::NoAction)
    {
        if (P.Order != EAIHeroesProposalOrder::None ||
            !P.TargetNode.IsNone())
        {
            return Rejected(TEXT("Advice or refusal cannot smuggle an order."));
        }
        return Accepted(false, P.Kind == EAIHeroesProposalKind::Advice
            ? TEXT("Read-only advice; no action executed.")
            : TEXT("No action; no campaign mutation."));
    }
    if (P.Kind != EAIHeroesProposalKind::ProposeOrder)
    {
        return Rejected(TEXT("Unknown proposal kind."));
    }
    if (S.bBattleActive)
    {
        return Rejected(TEXT("Strategic proposals are locked during battle."));
    }
    if (S.Strategic.ProtocolTier < 1)
    {
        return Rejected(TEXT("Formal order proposals require analysis protocol."));
    }
    if (P.TargetNode.IsNone())
    {
        return Rejected(TEXT("Missing target node."));
    }
    if (P.Order != EAIHeroesProposalOrder::MoveToNode &&
        P.Order != EAIHeroesProposalOrder::AttackNode)
    {
        return Rejected(TEXT("Unsupported strategic order type."));
    }

    const FAIHeroesAdvisorNode* Target = nullptr;
    for (const FAIHeroesAdvisorNode& Node : S.Strategic.Nodes)
    {
        if (Node.Id == P.TargetNode)
        {
            Target = &Node;
            break;
        }
    }
    if (!Target || !FAIHeroesFogRules::IsKnown(
            S.Strategic.bFogEnabled, S.Strategic.ExploredSitesMask, P.TargetNode))
    {
        return Rejected(TEXT("Target is unknown or not discovered."));
    }
    const bool bHostile = !Target->bOwned && HasGuard(*Target);
    if ((P.Order == EAIHeroesProposalOrder::AttackNode) != bHostile)
    {
        return Rejected(TEXT("Attack/move order does not match guard ownership."));
    }
    if (P.Order == EAIHeroesProposalOrder::AttackNode &&
        (Target->bOwned || Target->Type == EAIHeroesNodeType::Headquarters ||
         !HasArmy(S.Strategic.PlayerArmy)))
    {
        return Rejected(TEXT("Attacking owned/HQ targets or without an army is forbidden."));
    }

    const auto Path = FAIHeroesTravelRules::Find(S.Strategic,
        S.Strategic.HeroPosition, Target->Id, S.Strategic.MovesLeft);
    if (!Path.IsReachableThisTurn())
    {
        return Rejected(*FAIHeroesTravelRules::Describe(Path));
    }

    // Positive verdict is NOT an authorization. No actor or campaign API
    // is exposed here; only a real player action can trigger GameMode.
    return Accepted(true, TEXT("Legal suggestion; player selection and confirmation required."));
}

FString FAIHeroesAIProposalGate::Describe(
    const FAIHeroesAIProposal& P, const FAIHeroesProposalVerdict& V)
{
    if (!V.bAccepted)
    {
        return FString::Printf(TEXT("PROPOSAL GATE: BLOCKED | %s | No action executed."),
            *V.Reason);
    }
    if (V.bPlayerConfirmationRequired)
    {
        return FString::Printf(
            TEXT("PROPOSAL GATE: PASS | %s %s | PLAYER MUST CONFIRM. No action executed."),
            P.Order == EAIHeroesProposalOrder::AttackNode ? TEXT("ATTACK") : TEXT("MOVE"),
            *P.TargetNode.ToString());
    }
    return FString::Printf(TEXT("PROPOSAL GATE: PASS | %s | Offline read-only, no action."),
        P.Kind == EAIHeroesProposalKind::Advice ? TEXT("ADVICE") : TEXT("NO_ACTION"));
}
