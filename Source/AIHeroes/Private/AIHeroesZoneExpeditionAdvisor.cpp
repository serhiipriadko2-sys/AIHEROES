#include "AIHeroesZoneExpeditionAdvisor.h"

#include "AIHeroesAdventurePath.h"
#include "AIHeroesZoneExpeditionRules.h"

namespace
{
int32 TotalLivingHP(const FAIHeroesZoneExpeditionState& State)
{
    int32 HP = 0;
    for (const FAIHeroesArmyStack& Unit : State.PlayerArmy)
        HP += FMath::Max(0, Unit.TotalHealth);
    return HP;
}

bool HasVisibleNode(const FAIHeroesAdventureGraph& Graph, FName Id)
{
    return Graph.Nodes.ContainsByPredicate(
        [Id](const FAIHeroesAdventureNode& Node)
        {
            return Node.Id == Id && Node.bDiscovered;
        });
}

FString LayerName(int32 Tier)
{
    switch (Tier)
    {
        case 1: return TEXT("ANALYST");
        case 2: return TEXT("LEDGER");
        case 3: return TEXT("PLANNER");
        default: return TEXT("SEED");
    }
}

int32 StrategicValue(const FAIHeroesZoneExpeditionState& State,
                     const FName Target, bool bGuarded)
{
    // Only *known* zone IDs arrive here. No undetected template metadata.
    int32 Score = bGuarded ? 85 : 130;
    if (State.ProtocolTier >= 1)
    {
        if (Target == FName(TEXT("FOREST")))
            Score += State.Wood < 20 ? 75 : 25;
        else if (Target == FName(TEXT("ORE")))
            Score += State.Ore < 20 ? 75 : 25;
        else if (Target == FName(TEXT("SUPPLY")))
            Score += 25;
        else if (Target == FName(TEXT("PASS")))
            Score += 15;
    }
    if (State.ProtocolTier >= 2 && bGuarded &&
        TotalLivingHP(State) < 80)
        Score -= 35; // Caution only; no fake casualty forecast.
    if (State.ProtocolTier >= 3 && bGuarded &&
        Target == FName(TEXT("PASS")))
        Score += 20; // The known guarded frontier opens strategic options.
    return Score;
}

FString GuardCaution(int32 Tier, bool bGuarded)
{
    if (!bGuarded) return TEXT("No guard on the visible destination.");
    if (Tier <= 0)
        return TEXT("SEED: defender outcome UNKNOWN; no battle forecast.");
    if (Tier == 1)
        return TEXT("ANALYST: guarded encounter; casualties UNKNOWN.");
    if (Tier == 2)
        return TEXT("LEDGER: conserve army HP; no outcome prediction.");
    return TEXT("PLANNER: guarded chokepoint; plan reserves, not an auto-attack.");
}
}

FAIHeroesZoneExpeditionAdvice FAIHeroesZoneExpeditionAdvisor::Advise(
    const FAIHeroesZoneExpeditionState& State, FName InspectTarget)
{
    FAIHeroesZoneExpeditionAdvice Advice;
    FString Error;
    FAIHeroesAdventureGraph Visible;
    if (!FAIHeroesZoneExpeditionRules::PlayerView(State, Visible, Error))
    {
        // The reason may reveal details of an untrusted world. Never
        // disclose a partial graph or copy invalid state into the HUD.
        Advice.Message = TEXT("HERO OFFLINE: map validation failed; no advice.");
        return Advice;
    }
    Advice.SourceRevision = Visible.Revision;
    const int32 LivingHP = TotalLivingHP(State);

    if (!InspectTarget.IsNone())
    {
        if (!HasVisibleNode(Visible, InspectTarget))
        {
            Advice.Status = EAIHeroesZoneAdviceStatus::UnknownTarget;
            Advice.Message = TEXT("HERO OFFLINE: destination not in discovered map.");
            return Advice;
        }
        const auto Route = FAIHeroesAdventurePath::Find(
            Visible, State.HeroSite, InspectTarget, State.MovesLeft);
        if (Route.Status == EAIHeroesAdventurePathStatus::InvalidInput ||
            Route.Status == EAIHeroesAdventurePathStatus::Unreachable)
        {
            Advice.Status = EAIHeroesZoneAdviceStatus::NoReachableTarget;
            Advice.Message = TEXT("HERO OFFLINE: no authorized known route.");
            return Advice;
        }
        Advice.TravelCost = Route.TotalCost;
        Advice.MissingMoves = Route.MissingMoves;
        Advice.bGuarded = Route.bBattleAtDestination;
        if (Route.Status == EAIHeroesAdventurePathStatus::NeedsMoreMoves)
        {
            Advice.Status = EAIHeroesZoneAdviceStatus::NeedsMovement;
            Advice.Message = FString::Printf(
                TEXT("HERO OFFLINE: known %s costs %d; need %d more moves. Player may end day."),
                *InspectTarget.ToString(), Route.TotalCost, Route.MissingMoves);
            return Advice;
        }
        if (Route.bBattleAtDestination && LivingHP <= 0)
        {
            Advice.Status = EAIHeroesZoneAdviceStatus::NoArmy;
            Advice.Message = TEXT(
                "HERO OFFLINE: no living troops for this known guarded destination. Recruit first.");
            return Advice;
        }
        Advice.Status = EAIHeroesZoneAdviceStatus::Suggestion;
        Advice.SuggestedSite = InspectTarget;
        Advice.Message = FString::Printf(
            TEXT("HERO %s | %s costs %d moves. %s Human ENTER required."),
            *LayerName(State.ProtocolTier),
            *InspectTarget.ToString(), Route.TotalCost,
            *GuardCaution(State.ProtocolTier, Route.bBattleAtDestination));
        return Advice;
    }

    int32 BestScore = MIN_int32;
    int32 CheapestMissing = MAX_int32;
    bool bArmyBlocking = false;
    for (const FAIHeroesAdventureNode& Node : Visible.Nodes)
    {
        if (Node.Id == State.HeroSite ||
            FAIHeroesZoneExpeditionRules::IsCleared(State, Node.Id))
            continue;

        const auto Route = FAIHeroesAdventurePath::Find(
            Visible, State.HeroSite, Node.Id, State.MovesLeft);
        if (Route.Status == EAIHeroesAdventurePathStatus::NeedsMoreMoves)
        {
            CheapestMissing = FMath::Min(CheapestMissing, Route.MissingMoves);
            continue;
        }
        if (!Route.IsReachableThisTurn()) continue;
        if (Route.bBattleAtDestination && LivingHP <= 0)
        {
            bArmyBlocking = true;
            continue;
        }
        if (Route.bBattleAtDestination && LivingHP < 48 &&
            State.ProtocolTier >= 1)
        {
            bArmyBlocking = true;
            continue; // Rule-based caution, NOT a predicted defeat.
        }
        const int32 Score = StrategicValue(State, Node.Id,
            Route.bBattleAtDestination) - 13 * Route.TotalCost;
        const bool bTieBreak = Advice.SuggestedSite.IsNone() ||
            Node.Id.ToString().Compare(
                Advice.SuggestedSite.ToString(), ESearchCase::CaseSensitive) < 0;
        if (Score > BestScore || (Score == BestScore && bTieBreak))
        {
            BestScore = Score;
            Advice.Status = EAIHeroesZoneAdviceStatus::Suggestion;
            Advice.SuggestedSite = Node.Id;
            Advice.TravelCost = Route.TotalCost;
            Advice.bGuarded = Route.bBattleAtDestination;
        }
    }
    if (Advice.HasSuggestion())
    {
        Advice.Message = FString::Printf(
            TEXT("HERO %s: consider visible %s (%d moves). %s Select it yourself; ENTER authorizes."),
            *LayerName(State.ProtocolTier),
            *Advice.SuggestedSite.ToString(), Advice.TravelCost,
            *GuardCaution(State.ProtocolTier, Advice.bGuarded));
        return Advice;
    }
    if (bArmyBlocking)
    {
        Advice.Status = EAIHeroesZoneAdviceStatus::NoArmy;
        Advice.Message = TEXT(
            "HERO OFFLINE: known guarded routes are risky without troops. Return to HQ and recruit.");
        return Advice;
    }
    if (CheapestMissing != MAX_int32)
    {
        Advice.Status = EAIHeroesZoneAdviceStatus::NeedsMovement;
        Advice.MissingMoves = CheapestMissing;
        Advice.Message = FString::Printf(
            TEXT("HERO OFFLINE: known targets need at least %d more moves. Human may end day."),
            CheapestMissing);
        return Advice;
    }
    Advice.Status = EAIHeroesZoneAdviceStatus::NoReachableTarget;
    Advice.Message = TEXT(
        "HERO OFFLINE: no reachable unclaimed known target. Explore by authorized movement.");
    return Advice;
}
