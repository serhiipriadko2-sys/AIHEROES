#include "AIHeroesModelRequest.h"

#include "AIHeroesFogRules.h"
#include "AIHeroesTravelRules.h"

// Namespaced helpers also remain isolated in Unreal unity builds.
namespace AIHeroesModelRequestDetail
{
bool Token(const FString& S)
{
    if (S.Len() != 32) return false;
    for (TCHAR C : S)
        if (!((C >= '0' && C <= '9') || (C >= 'a' && C <= 'f'))) return false;
    return true;
}
bool Identifier(const FString& S)
{
    auto Letter = [](TCHAR C) { return (C >= 'A' && C <= 'Z') || (C >= 'a' && C <= 'z'); };
    if (S.IsEmpty() || S.Len() > 48 || !Letter(S[0])) return false;
    for (TCHAR C : S)
        if (!Letter(C) && !(C >= '0' && C <= '9') && C != '_' && C != '-') return false;
    return true;
}
const TCHAR* TypeName(EAIHeroesNodeType T)
{
    switch (T)
    {
        case EAIHeroesNodeType::Headquarters: return TEXT("Headquarters");
        case EAIHeroesNodeType::ForestMine: return TEXT("ForestMine");
        case EAIHeroesNodeType::OreMine: return TEXT("OreMine");
        case EAIHeroesNodeType::Outpost: return TEXT("Outpost");
        case EAIHeroesNodeType::Ruins: return TEXT("Ruins");
        default: return nullptr;
    }
}
const TCHAR* GoalName(EAIHeroesModelGoal Goal)
{
    switch (Goal)
    {
        case EAIHeroesModelGoal::AssessSelected: return TEXT("AssessSelected");
        case EAIHeroesModelGoal::Explore: return TEXT("Explore");
        case EAIHeroesModelGoal::PreserveArmy: return TEXT("PreserveArmy");
        default: return nullptr;
    }
}
bool Living(const TArray<FAIHeroesArmyStack>& Army)
{
    for (const auto& U : Army) if (U.IsAlive()) return true;
    return false;
}
bool ArmyNames(const TArray<FAIHeroesArmyStack>& Army)
{
    for (const auto& U : Army) if (!Identifier(U.UnitId.ToString())) return false;
    return true;
}
void AppendArmy(FString& Out, const TArray<FAIHeroesArmyStack>& Army)
{
    Out += TEXT("[");
    for (int32 I = 0; I < Army.Num(); ++I)
    {
        if (I) Out += TEXT(",");
        const auto& U = Army[I];
        // All interpolated strings have passed the restricted ASCII identifier
        // allowlist; no quotes, slashes, whitespace or free text can enter JSON.
        Out += FString::Printf(TEXT("{\"unit_id\":\"%s\",\"unit_health\":%d,\"total_health\":%d,\"attack\":%d,\"defense\":%d,\"initiative\":%d}"),
            *U.UnitId.ToString(), U.UnitHealth, U.TotalHealth, U.Attack, U.Defense, U.Initiative);
    }
    Out += TEXT("]");
}
}

bool FAIHeroesModelRequest::Build(const FAIHeroesProposalSnapshot& S,
    const FString& RequestId, const FString& SessionId, EAIHeroesModelGoal Goal,
    FString& OutJson, FString& Error, TArray<FAIHeroesAIProposal>* OutMenu)
{
    using namespace AIHeroesModelRequestDetail;
    OutJson.Empty();
    if (OutMenu) OutMenu->Reset();
    Error = TEXT("MODEL REQUEST: invalid bounded engine snapshot or correlation token.");
    if (!Token(RequestId) || !Token(SessionId) || !GoalName(Goal) || S.Day > 1000000) return false;

    // Reuse existing game validation without claiming a loaded model.
    FAIHeroesAIProposal Probe;
    Probe.ObservedDay = S.Day;
    Probe.ObservedRevision = S.StateRevision;
    Probe.Explanation = TEXT("Read-only export preflight.");
    if (!FAIHeroesAIProposalGate::Validate(S, Probe).bAccepted ||
        !ArmyNames(S.Strategic.PlayerArmy)) return false;
    const auto& A = S.Strategic;
    if (A.TravelRuleVersion == FAIHeroesTravelRules::RoadVersion)
    {
        FAIHeroesAdventureGraph Graph;
        FString Reason;
        if (!FAIHeroesTravelRules::BuildGraph(A, Graph, Reason)) return false;
    }

    TArray<const FAIHeroesAdvisorNode*> Visible;
    const FAIHeroesAdvisorNode* Selected = nullptr;
    int32 Origins = 0;
    for (const auto& Node : A.Nodes)
    {
        // Internal topology can contain hidden sites. Never serialize them.
        if (!FAIHeroesFogRules::IsKnown(A.bFogEnabled, A.ExploredSitesMask, Node.Id)) continue;
        if (!Identifier(Node.Id.ToString()) || !TypeName(Node.Type) ||
            !ArmyNames(Node.Guards) || (Node.bOwned && Living(Node.Guards))) return false;
        Visible.Add(&Node);
        if (Node.Id == A.SelectedNodeId) Selected = &Node;
        if (FVector::DistSquared(A.HeroPosition, Node.Location) < 1.0)
        {
            if (!Node.bOwned) return false;
            ++Origins;
        }
    }
    if (Origins != 1 || (!A.SelectedNodeId.IsNone() && !Selected)) return false;
    Visible.Sort([](const FAIHeroesAdvisorNode& L, const FAIHeroesAdvisorNode& R)
    { return L.Id.ToString().ToLower() < R.Id.ToString().ToLower(); });

    FString J = FString::Printf(TEXT("{\"schema\":\"aiheroes.hero-request.v1\",\"request_id\":\"%s\",\"session_id\":\"%s\",\"day\":%d,\"state_revision\":%u,\"protocol_tier\":%d,\"model_slot_tier\":%d,\"moves_left\":%d,\"battle_active\":%s,\"goal\":\"%s\",\"selected_node\":"),
        *RequestId, *SessionId, S.Day, S.StateRevision, A.ProtocolTier, S.ModelSlotTier,
        A.MovesLeft, S.bBattleActive ? TEXT("true") : TEXT("false"), GoalName(Goal));
    J += Selected ? FString::Printf(TEXT("\"%s\""), *Selected->Id.ToString()) : TEXT("null");
    J += FString::Printf(TEXT(",\"resources\":{\"gold\":%d,\"wood\":%d,\"ore\":%d},\"player_army\":"), A.Gold, A.Wood, A.Ore);
    AIHeroesModelRequestDetail::AppendArmy(J, A.PlayerArmy);
    J += TEXT(",\"visible_nodes\":[");
    for (int32 I = 0; I < Visible.Num(); ++I)
    {
        if (I) J += TEXT(",");
        const auto& N = *Visible[I];
        J += FString::Printf(TEXT("{\"id\":\"%s\",\"type\":\"%s\",\"owned\":%s,\"guards\":"),
            *N.Id.ToString(), TypeName(N.Type), N.bOwned ? TEXT("true") : TEXT("false"));
        AIHeroesModelRequestDetail::AppendArmy(J, N.Guards);
        J += TEXT("}");
    }
    J += TEXT("],\"legal_orders\":[");
    // c0/c1 and subsequent orders exactly share the Python bridge menu order.
    TArray<FAIHeroesAIProposal> Candidates;
    for (const auto Kind : {EAIHeroesProposalKind::NoAction, EAIHeroesProposalKind::Advice})
    {
        FAIHeroesAIProposal Base;
        Base.Kind = Kind; Base.ObservedDay = S.Day; Base.ObservedRevision = S.StateRevision;
        Base.Explanation = Kind == EAIHeroesProposalKind::NoAction
            ? TEXT("No strategic action proposed. Player remains in control.")
            : TEXT("Review the visible map and army before deciding. No action executed.");
        Candidates.Add(MoveTemp(Base));
    }
    int32 Orders = 0;
    if (A.ProtocolTier >= 1 && !S.bBattleActive)
    {
        for (const auto* N : Visible)
        {
            // A zero-cost move to the current site is not a useful suggestion.
            if (FVector::DistSquared(A.HeroPosition, N->Location) < 1.0) continue;
            FAIHeroesAIProposal P;
            P.Kind = EAIHeroesProposalKind::ProposeOrder;
            P.Order = !N->bOwned && Living(N->Guards)
                ? EAIHeroesProposalOrder::AttackNode : EAIHeroesProposalOrder::MoveToNode;
            P.ObservedDay = S.Day;
            P.ObservedRevision = S.StateRevision;
            P.TargetNode = N->Id;
            P.Explanation = TEXT("Engine-checked candidate, never execution.");
            if (!FAIHeroesAIProposalGate::Validate(S, P).bAccepted) continue;
            const auto Path = FAIHeroesTravelRules::Find(A, A.HeroPosition, N->Id, A.MovesLeft);
            if (!Path.IsReachableThisTurn()) return false;
            P.Explanation = FString::Printf(
                TEXT("%s %s; engine-quoted cost %d moves. This is not a victory forecast. Player confirmation required."),
                P.Order == EAIHeroesProposalOrder::AttackNode ? TEXT("Consider attacking") : TEXT("Consider moving to"),
                *N->Id.ToString(), Path.TotalCost);
            Candidates.Add(P);
            if (Orders++) J += TEXT(",");
            J += FString::Printf(TEXT("{\"order\":\"%s\",\"target_node\":\"%s\",\"cost\":%d}"),
                P.Order == EAIHeroesProposalOrder::AttackNode ? TEXT("AttackNode") : TEXT("MoveToNode"),
                *N->Id.ToString(), Path.TotalCost);
        }
    }
    J += TEXT("]}");
    // The complete output is ASCII: TCHAR length equals UTF-8 byte length.
    if (J.Len() <= 0 || J.Len() > MaxBytes) return false;
    if (OutMenu) *OutMenu = MoveTemp(Candidates);
    OutJson = MoveTemp(J);
    Error.Empty();
    return true;
}
