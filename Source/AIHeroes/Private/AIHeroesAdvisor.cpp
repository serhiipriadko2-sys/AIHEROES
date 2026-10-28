#include "AIHeroesAdvisor.h"

#include "AIHeroesBattleEngine.h"
#include "AIHeroesGridBattle.h"
#include "AIHeroesRoutePlanner.h"
#include "AIHeroesTurnRules.h"

namespace
{
int32 LivingUnits(const TArray<FAIHeroesArmyStack>& Army)
{
    int32 Count = 0;
    for (const FAIHeroesArmyStack& Stack : Army)
    {
        Count += Stack.Count();
    }
    return Count;
}

int32 TotalHP(const TArray<FAIHeroesArmyStack>& Army)
{
    int32 HP = 0;
    for (const FAIHeroesArmyStack& Stack : Army)
    {
        HP += Stack.TotalHealth;
    }
    return HP;
}

bool Guarded(const FAIHeroesAdvisorNode& Node)
{
    for (const FAIHeroesArmyStack& Defender : Node.Guards)
    {
        if (Defender.IsAlive())
        {
            return true;
        }
    }
    return false;
}

int32 EconomicValue(const FAIHeroesAdvisorNode& Node, const FAIHeroesAdvisorSnapshot& S)
{
    switch (Node.Type)
    {
        case EAIHeroesNodeType::ForestMine: return S.Wood < 25 ? 140 : 65;
        case EAIHeroesNodeType::OreMine: return S.Ore < 20 ? 140 : 65;
        case EAIHeroesNodeType::Outpost: return S.Gold < 100 ? 120 : 75;
        case EAIHeroesNodeType::Ruins: return 45;
        default: return 0;
    }
}

FString DescribeTarget(const FAIHeroesAdvisorSnapshot& S, const FAIHeroesAdvisorNode& Target)
{
    const int32 Cost = FAIHeroesTurnRules::TravelCost(
        S.HeroPosition, Target.Location, Guarded(Target) && !Target.bOwned);

    if (Cost > S.MovesLeft)
    {
        return FString::Printf(TEXT("%s: need %d moves, have %d. End turn or choose a nearer node."),
            *Target.Id.ToString(), Cost, S.MovesLeft);
    }
    if (Target.bOwned)
    {
        return FString::Printf(TEXT("%s is already yours. Use it for income and repositioning."),
            *Target.Id.ToString());
    }
    if (!Guarded(Target))
    {
        return FString::Printf(
            TEXT("%s: unguarded; capture costs %d moves. No combat necessary. Your choice."),
            *Target.Id.ToString(), Cost);
    }
    if (LivingUnits(S.PlayerArmy) <= 0)
    {
        return TEXT("No living army. Recruit Militia or Scouts before attacking.");
    }
    if (S.ProtocolTier <= 0)
    {
        return TEXT("SEED: enemy is guarded. Threat analysis locked; outcome UNKNOWN.");
    }

    const FAIHeroesBattleOutcome Forecast = FAIHeroesGridBattle::RunToEnd(
        S.PlayerArmy, Target.Guards, S.ProtocolTier);
    const int32 LostHP = FMath::Max(0, TotalHP(S.PlayerArmy) - TotalHP(Forecast.PlayerAfter));
    const int32 Survivors = LivingUnits(Forecast.PlayerAfter);

    FString Advice = FString::Printf(
        TEXT("%s: %s after %d rounds, %d survivors, -%d HP, %d moves. %s"),
        *Target.Id.ToString(),
        Forecast.bVictory ? TEXT("WIN FORECAST") : TEXT("HIGH RISK"),
        Forecast.Rounds, Survivors, LostHP, Cost,
        Forecast.bVictory ? TEXT("Player decides.") : TEXT("Recruit or avoid."));
    if (S.ProtocolTier >= 2 && !S.LastBattle.IsEmpty())
    {
        Advice += FString::Printf(TEXT(" JOURNAL: %s"), *S.LastBattle.Left(45));
    }
    if (S.ProtocolTier >= 3 && Forecast.bVictory)
    {
        Advice += TEXT(" PLAN: protect survivors; prioritize weakened enemies.");
    }
    return Advice;
}
}

FString FAIHeroesAdvisor::Advise(const FAIHeroesAdvisorSnapshot& S)
{
    const FAIHeroesAdvisorNode* Selected = nullptr;
    for (const FAIHeroesAdvisorNode& Node : S.Nodes)
    {
        if (Node.Id == S.SelectedNodeId)
        {
            Selected = &Node;
            break;
        }
    }
    if (Selected)
    {
        return DescribeTarget(S, *Selected);
    }

    if (S.ProtocolTier <= 0)
    {
        return TEXT("SEED: choose a destination. I cannot rank threats until analysis is unlocked.");
    }
    if (LivingUnits(S.PlayerArmy) <= 0)
    {
        return TEXT("No living units: recruit at HQ before exploring guarded nodes.");
    }

    // TACTICAL PLANNER unlock: two consecutive destinations are evaluated
    // with the true movement budget and post-battle surviving army. This is
    // advisory only; each step still requires explicit player confirmation.
    if (S.ProtocolTier >= 3)
    {
        const FAIHeroesRoutePlan Route = FAIHeroesRoutePlanner::Plan(S);
        if (Route.IsValid())
        {
            return FString::Printf(
                TEXT("STRATEGY: consider %s. Select it yourself. ROUTE: %s. ")
                TEXT("Forecast only; player must confirm each move."),
                *Route.Stops[0].ToString(), *Route.Describe());
        }
    }

    const FAIHeroesAdvisorNode* Best = nullptr;
    const FAIHeroesAdvisorNode* Alternative = nullptr;
    int32 BestScore = MIN_int32;
    int32 AltScore = MIN_int32;

    for (const FAIHeroesAdvisorNode& Node : S.Nodes)
    {
        if (Node.bOwned)
        {
            continue;
        }

        const bool bGuarded = Guarded(Node);
        const int32 Cost = FAIHeroesTurnRules::TravelCost(
            S.HeroPosition, Node.Location, bGuarded);
        if (Cost > S.MovesLeft)
        {
            continue;
        }

        int32 LossHP = 0;
        if (bGuarded)
        {
            const FAIHeroesBattleOutcome Forecast = FAIHeroesGridBattle::RunToEnd(
                S.PlayerArmy, Node.Guards, S.ProtocolTier);
            if (!Forecast.bVictory)
            {
                continue; // Never recommend a known losing target as safe.
            }
            LossHP = FMath::Max(0, TotalHP(S.PlayerArmy) - TotalHP(Forecast.PlayerAfter));
        }

        const int32 Score = EconomicValue(Node, S) - 20 * Cost - LossHP;
        if (Score > BestScore)
        {
            Alternative = Best;
            AltScore = BestScore;
            Best = &Node;
            BestScore = Score;
        }
        else if (Score > AltScore)
        {
            Alternative = &Node;
            AltScore = Score;
        }
    }

    if (!Best)
    {
        return TEXT("No safe reachable target found. Recruit, conserve troops or end the turn.");
    }

    FString Advice = FString::Printf(
        TEXT("STRATEGY: consider %s (resource value, moves, forecast). Select it yourself."),
        *Best->Id.ToString());
    if (S.ProtocolTier >= 2)
    {
        Advice += FString::Printf(TEXT(" Gold=%d Wood=%d Ore=%d."), S.Gold, S.Wood, S.Ore);
    }
    if (S.ProtocolTier >= 3 && Alternative)
    {
        Advice += FString::Printf(TEXT(" ALTERNATIVE: %s."), *Alternative->Id.ToString());
    }
    return Advice;
}
