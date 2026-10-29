#include "AIHeroesRoutePlanner.h"

#include "AIHeroesBattleEngine.h"
#include "AIHeroesGridBattle.h"
#include "AIHeroesTravelRules.h"

namespace
{
int32 RouteTotalHP(const TArray<FAIHeroesArmyStack>& Army)
{
    int32 HP = 0;
    for (const FAIHeroesArmyStack& Stack : Army)
    {
        HP += FMath::Max(0, Stack.TotalHealth);
    }
    return HP;
}

int32 LivingCount(const TArray<FAIHeroesArmyStack>& Army)
{
    int32 Units = 0;
    for (const FAIHeroesArmyStack& Stack : Army)
    {
        Units += Stack.Count();
    }
    return Units;
}

int32 StrategicValue(const FAIHeroesAdvisorNode& Node, const FAIHeroesAdvisorSnapshot& S)
{
    switch (Node.Type)
    {
        case EAIHeroesNodeType::ForestMine: return S.Wood < 25 ? 140 : 65;
        case EAIHeroesNodeType::OreMine: return S.Ore < 20 ? 140 : 65;
        case EAIHeroesNodeType::Outpost: return S.Gold < 100 ? 120 : 75;
        case EAIHeroesNodeType::Ruins: return 115;
        default: return 0;
    }
}

// Never fabricate extra movement or assume the army heals after a fight.
// Both the travel cost and combat outcomes are computed by the same
// authoritative deterministic game rules.
bool ConsiderVisit(const FAIHeroesAdvisorNode& Node,
    const FAIHeroesAdvisorSnapshot& Snapshot,
    const FVector& Position,
    const TArray<FAIHeroesArmyStack>& Army,
    int32 AvailableMoves,
    TArray<FAIHeroesArmyStack>& Survivors,
    int32& MoveCost,
    int32& LostHP, const TSet<FName>& Cleared = TSet<FName>())
{
    if (Node.Id.IsNone() || Node.bOwned || Node.Type == EAIHeroesNodeType::Headquarters)
    {
        return false;
    }

    bool bGuarded = false;
    for (const FAIHeroesArmyStack& Stack : Node.Guards)
    {
        bGuarded |= Stack.IsAlive();
    }

    const auto Path = FAIHeroesTravelRules::Find(Snapshot, Position, Node.Id, AvailableMoves, Cleared);
    MoveCost = Path.TotalCost;
    if (!Path.IsReachableThisTurn())
    {
        return false;
    }

    Survivors = Army;
    LostHP = 0;
    if (bGuarded)
    {
        if (LivingCount(Army) == 0)
        {
            return false;
        }
        const FAIHeroesBattleOutcome Forecast = FAIHeroesGridBattle::RunToEnd(
            Army, Node.Guards, Snapshot.ProtocolTier);
        if (!Forecast.bVictory)
        {
            return false;
        }
        Survivors = Forecast.PlayerAfter;
        LostHP = FMath::Max(0, RouteTotalHP(Army) - RouteTotalHP(Survivors));
    }
    return true;
}

FString AIHRouteKey(const FAIHeroesRoutePlan& Plan)
{
    FString Key;
    for (const FName& Stop : Plan.Stops)
    {
        Key += Stop.ToString() + TEXT("/");
    }
    return Key;
}

bool AIHRouteBetter(const FAIHeroesRoutePlan& A, const FAIHeroesRoutePlan& B)
{
    // Do not depend on TArray iteration order when two routes tie.
    if (A.Score != B.Score) return A.Score > B.Score;
    if (A.TotalMoves != B.TotalMoves) return A.TotalMoves < B.TotalMoves;
    return AIHRouteKey(A) < AIHRouteKey(B);
}

void ConsiderCandidate(const FAIHeroesRoutePlan& Candidate,
    TArray<FAIHeroesRoutePlan>& Top, int32 Capacity)
{
    if (!Candidate.IsValid() || Capacity <= 0)
    {
        return;
    }

    // Two node instances with the same ID are untrusted input, not a
    // justification to recommend the same route twice.
    for (FAIHeroesRoutePlan& Existing : Top)
    {
        if (Existing.Stops == Candidate.Stops)
        {
            if (AIHRouteBetter(Candidate, Existing))
            {
                Existing = Candidate;
                Top.Sort(AIHRouteBetter);
            }
            return;
        }
    }
    Top.Add(Candidate);
    Top.Sort(AIHRouteBetter);
    if (Top.Num() > Capacity)
    {
        Top.RemoveAt(Capacity, Top.Num() - Capacity, EAllowShrinking::No);
    }
}
}

FString FAIHeroesRoutePlan::Describe() const
{
    if (Stops.IsEmpty())
    {
        return TEXT("No feasible route this turn.");
    }

    FString Names;
    for (const FName& Stop : Stops)
    {
        if (!Names.IsEmpty())
        {
            Names += TEXT(" -> ");
        }
        Names += Stop.ToString();
    }
    return FString::Printf(TEXT("%s | %d moves, ~%d HP at risk, %d survivors"),
        *Names, TotalMoves, ExpectedHpLoss, RemainingUnits);
}

TArray<FAIHeroesRoutePlan> FAIHeroesRoutePlanner::Rank(
    const FAIHeroesAdvisorSnapshot& Snapshot, int32 MaxPlans)
{
    TArray<FAIHeroesRoutePlan> Top;
    const int32 Capacity = FMath::Clamp(MaxPlans, 0, 3);
    if (Capacity == 0 || Snapshot.ProtocolTier != 3 || Snapshot.MovesLeft <= 0 ||
        Snapshot.Nodes.Num() > 64 || Snapshot.PlayerArmy.Num() > 12)
    {
        return Top;
    }

    for (int32 FirstIndex = 0; FirstIndex < Snapshot.Nodes.Num(); ++FirstIndex)
    {
        const FAIHeroesAdvisorNode& First = Snapshot.Nodes[FirstIndex];
        TArray<FAIHeroesArmyStack> Survivors;
        int32 FirstCost = 0;
        int32 FirstHP = 0;
        if (!ConsiderVisit(First, Snapshot, Snapshot.HeroPosition,
            Snapshot.PlayerArmy, Snapshot.MovesLeft, Survivors, FirstCost, FirstHP))
        {
            continue;
        }

        FAIHeroesRoutePlan One;
        One.Stops.Add(First.Id);
        One.TotalMoves = FirstCost;
        One.ExpectedHpLoss = FirstHP;
        One.RemainingUnits = LivingCount(Survivors);
        One.Score = StrategicValue(First, Snapshot) - FirstCost * 20 - FirstHP;
        ConsiderCandidate(One, Top, Capacity);

        const int32 MovesRemaining = Snapshot.MovesLeft - FirstCost;
        if (MovesRemaining <= 0)
        {
            continue;
        }

        TSet<FName> FirstCleared;
        FirstCleared.Add(First.Id);
        for (int32 SecondIndex = 0; SecondIndex < Snapshot.Nodes.Num(); ++SecondIndex)
        {
            if (SecondIndex == FirstIndex ||
                Snapshot.Nodes[SecondIndex].Id == First.Id)
            {
                continue;
            }

            const FAIHeroesAdvisorNode& Second = Snapshot.Nodes[SecondIndex];
            TArray<FAIHeroesArmyStack> SurvivorsAfterSecond;
            int32 SecondCost = 0;
            int32 SecondHP = 0;
            if (!ConsiderVisit(Second, Snapshot, First.Location,
                Survivors, MovesRemaining, SurvivorsAfterSecond, SecondCost, SecondHP, FirstCleared))
            {
                continue;
            }

            FAIHeroesRoutePlan Two = One;
            Two.Stops.Add(Second.Id);
            Two.TotalMoves += SecondCost;
            Two.ExpectedHpLoss += SecondHP;
            Two.RemainingUnits = LivingCount(SurvivorsAfterSecond);
            Two.Score += StrategicValue(Second, Snapshot) - SecondCost * 20 - SecondHP;
            ConsiderCandidate(Two, Top, Capacity);
        }
    }
    return Top;
}

FAIHeroesRoutePlan FAIHeroesRoutePlanner::Plan(
    const FAIHeroesAdvisorSnapshot& Snapshot)
{
    const TArray<FAIHeroesRoutePlan> Alternatives = Rank(Snapshot, 1);
    return Alternatives.IsEmpty() ? FAIHeroesRoutePlan() : Alternatives[0];
}
