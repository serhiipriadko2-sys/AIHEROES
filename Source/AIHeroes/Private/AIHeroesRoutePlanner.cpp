#include "AIHeroesRoutePlanner.h"

#include "AIHeroesBattleEngine.h"
#include "AIHeroesTurnRules.h"

namespace
{
int32 TotalHP(const TArray<FAIHeroesArmyStack>& Army)
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
    int32& LostHP)
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

    MoveCost = FAIHeroesTurnRules::TravelCost(Position, Node.Location, bGuarded);
    if (!FAIHeroesTurnRules::CanMove(AvailableMoves, MoveCost))
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
        const FAIHeroesBattleOutcome Forecast = FAIHeroesBattleEngine::Resolve(
            Army, Node.Guards, Snapshot.ProtocolTier);
        if (!Forecast.bVictory)
        {
            return false;
        }
        Survivors = Forecast.PlayerAfter;
        LostHP = FMath::Max(0, TotalHP(Army) - TotalHP(Survivors));
    }
    return true;
}

void ConsiderCandidate(const FAIHeroesRoutePlan& Candidate,
    FAIHeroesRoutePlan& Best)
{
    // When scores tie, prefer fewer movement points and stable node IDs.
    auto Key = [](const FAIHeroesRoutePlan& P)
    {
        FString Text;
        for (const FName& Stop : P.Stops)
        {
            Text += Stop.ToString() + TEXT("/");
        }
        return Text;
    };
    if (!Best.IsValid() ||
        Candidate.Score > Best.Score ||
        (Candidate.Score == Best.Score &&
            (Candidate.TotalMoves < Best.TotalMoves ||
                (Candidate.TotalMoves == Best.TotalMoves && Key(Candidate) < Key(Best)))))
    {
        Best = Candidate;
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

FAIHeroesRoutePlan FAIHeroesRoutePlanner::Plan(
    const FAIHeroesAdvisorSnapshot& Snapshot)
{
    FAIHeroesRoutePlan Best;
    if (Snapshot.ProtocolTier != 3 || Snapshot.MovesLeft <= 0 ||
        Snapshot.Nodes.Num() > 64 || Snapshot.PlayerArmy.Num() > 12)
    {
        return Best;
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
        ConsiderCandidate(One, Best);

        const int32 MovesRemaining = Snapshot.MovesLeft - FirstCost;
        if (MovesRemaining <= 0)
        {
            continue;
        }

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
                Survivors, MovesRemaining, SurvivorsAfterSecond, SecondCost, SecondHP))
            {
                continue;
            }

            FAIHeroesRoutePlan Two = One;
            Two.Stops.Add(Second.Id);
            Two.TotalMoves += SecondCost;
            Two.ExpectedHpLoss += SecondHP;
            Two.RemainingUnits = LivingCount(SurvivorsAfterSecond);
            Two.Score += StrategicValue(Second, Snapshot) - SecondCost * 20 - SecondHP;
            ConsiderCandidate(Two, Best);
        }
    }
    return Best;
}
