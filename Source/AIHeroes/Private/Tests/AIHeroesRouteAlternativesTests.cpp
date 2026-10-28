#include "AIHeroesRoutePlanner.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

namespace
{
FAIHeroesAdvisorSnapshot AIHMakeRankedScenario()
{
    FAIHeroesAdvisorSnapshot S;
    S.ProtocolTier = 3;
    S.MovesLeft = 3;
    S.Gold = 240;
    S.Wood = 0;
    S.Ore = 0;
    S.HeroPosition = FVector(0.f, 0.f, 155.f);
    S.PlayerArmy.Add(FAIHeroesArmyStack(TEXT("Militia"), 8, 12, 4, 2, 3));
    S.PlayerArmy.Add(FAIHeroesArmyStack(TEXT("Scout"), 3, 8, 6, 1, 7));

    FAIHeroesAdvisorNode HQ;
    HQ.Id = FName(TEXT("HQ"));
    HQ.Type = EAIHeroesNodeType::Headquarters;
    HQ.Location = S.HeroPosition;
    HQ.bOwned = true;
    S.Nodes.Add(HQ);

    FAIHeroesAdvisorNode Forest;
    Forest.Id = FName(TEXT("FOREST"));
    Forest.Type = EAIHeroesNodeType::ForestMine;
    Forest.Location = FVector(400.f, 0.f, 155.f);
    S.Nodes.Add(Forest);

    FAIHeroesAdvisorNode Ore;
    Ore.Id = FName(TEXT("ORE"));
    Ore.Type = EAIHeroesNodeType::OreMine;
    Ore.Location = FVector(800.f, 0.f, 155.f);
    S.Nodes.Add(Ore);

    FAIHeroesAdvisorNode Outpost;
    Outpost.Id = FName(TEXT("OUTPOST"));
    Outpost.Type = EAIHeroesNodeType::Outpost;
    Outpost.Location = FVector(1200.f, 0.f, 155.f);
    S.Nodes.Add(Outpost);
    return S;
}

FString AIHTestRouteKey(const FAIHeroesRoutePlan& Plan)
{
    FString Key;
    for (const FName& Stop : Plan.Stops)
    {
        Key += Stop.ToString() + TEXT("/");
    }
    return Key;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesRouteRankingTest,
    "AIHeroes.Advisor.RankedAlternativesDeterministic",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesRouteRankingTest::RunTest(const FString& Parameters)
{
    FAIHeroesAdvisorSnapshot S = AIHMakeRankedScenario();
    const TArray<FAIHeroesRoutePlan> Ranked = FAIHeroesRoutePlanner::Rank(S);

    TestEqual(TEXT("Exactly three bounded alternatives"), Ranked.Num(), 3);
    if (Ranked.Num() != 3) return false;
    TestEqual(TEXT("Top route keeps existing Plan compatibility"),
        Ranked[0].Describe(), FAIHeroesRoutePlanner::Plan(S).Describe());
    TestEqual(TEXT("Top route keeps the same heuristic"),
        Ranked[0].Score, FAIHeroesRoutePlanner::Plan(S).Score);

    TSet<FString> Seen;
    for (int32 I = 0; I < Ranked.Num(); ++I)
    {
        const FAIHeroesRoutePlan& Route = Ranked[I];
        TestTrue(TEXT("Every recommendation is nonempty"), Route.IsValid());
        TestTrue(TEXT("Every route fits the movement budget"),
            Route.TotalMoves > 0 && Route.TotalMoves <= S.MovesLeft);
        TestTrue(TEXT("No fictitious casualties"), Route.ExpectedHpLoss >= 0);
        TestTrue(TEXT("Route has one or two stops"),
            Route.Stops.Num() > 0 && Route.Stops.Num() <= 2);

        const FString Key = AIHTestRouteKey(Route);
        TestFalse(TEXT("No duplicate ordered route"), Seen.Contains(Key));
        Seen.Add(Key);
        if (I > 0)
        {
            TestTrue(TEXT("Scores sorted descending"),
                Ranked[I - 1].Score >= Route.Score);
            if (Ranked[I - 1].Score == Route.Score)
            {
                TestTrue(TEXT("Tie respects cost then alphabetical route key"),
                    Ranked[I - 1].TotalMoves < Route.TotalMoves ||
                    (Ranked[I - 1].TotalMoves == Route.TotalMoves &&
                        AIHTestRouteKey(Ranked[I - 1]) < Key));
            }
        }
    }

    // Shuffle actor snapshot order; outcome must not drift.
    S.Nodes.Swap(1, 2);
    const TArray<FAIHeroesRoutePlan> Shuffled = FAIHeroesRoutePlanner::Rank(S);
    TestEqual(TEXT("Shuffled snapshot keeps three choices"), Shuffled.Num(), 3);
    for (int32 I = 0; I < FMath::Min(Shuffled.Num(), Ranked.Num()); ++I)
    {
        TestEqual(TEXT("Rank and order stable across input permutations"),
            Shuffled[I].Describe(), Ranked[I].Describe());
        TestEqual(TEXT("Heuristic score stable across permutations"),
            Shuffled[I].Score, Ranked[I].Score);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesRouteCapacityTest,
    "AIHeroes.Advisor.RankedAlternativesBounds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesRouteCapacityTest::RunTest(const FString& Parameters)
{
    FAIHeroesAdvisorSnapshot S = AIHMakeRankedScenario();
    TestEqual(TEXT("Request for zero choices is a no-op"),
        FAIHeroesRoutePlanner::Rank(S, 0).Num(), 0);
    TestEqual(TEXT("Negative request cannot enable planning"),
        FAIHeroesRoutePlanner::Rank(S, -10).Num(), 0);
    TestEqual(TEXT("Explicit one choice preserves old API"),
        FAIHeroesRoutePlanner::Rank(S, 1).Num(), 1);
    TestEqual(TEXT("Excessive count still capped at three"),
        FAIHeroesRoutePlanner::Rank(S, 100000).Num(), 3);

    S.ProtocolTier = 2;
    TestTrue(TEXT("Alternatives remain locked below protocol tier three"),
        FAIHeroesRoutePlanner::Rank(S).IsEmpty());
    S.ProtocolTier = 3;
    S.MovesLeft = 0;
    TestTrue(TEXT("No movement makes every option infeasible"),
        FAIHeroesRoutePlanner::Rank(S).IsEmpty());
    S.MovesLeft = 3;

    // Repeated IDs in an untrusted snapshot may not multiply alternatives.
    S.Nodes[3].Id = S.Nodes[2].Id;
    const TArray<FAIHeroesRoutePlan> DuplicateIds = FAIHeroesRoutePlanner::Rank(S);
    TSet<FString> Keys;
    for (const FAIHeroesRoutePlan& Route : DuplicateIds)
    {
        const FString Key = AIHTestRouteKey(Route);
        TestFalse(TEXT("Duplicate node ID does not double-list a route"),
            Keys.Contains(Key));
        Keys.Add(Key);
        if (Route.Stops.Num() == 2)
        {
            TestNotEqual(TEXT("A node cannot be captured twice"), Route.Stops[0], Route.Stops[1]);
        }
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesRouteSafeOptionTest,
    "AIHeroes.Advisor.RankedAlternativesRejectLosingBattle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesRouteSafeOptionTest::RunTest(const FString& Parameters)
{
    FAIHeroesAdvisorSnapshot S = AIHMakeRankedScenario();
    // One hopelessly defended mine must never appear as a safe candidate,
    // even after capturing other nodes.
    S.Nodes[2].Guards.Add(
        FAIHeroesArmyStack(TEXT("Guardian"), 100, 24, 7, 4, 2));
    const TArray<FAIHeroesRoutePlan> Ranked = FAIHeroesRoutePlanner::Rank(S);
    TestTrue(TEXT("Other safe routes remain available"), !Ranked.IsEmpty());
    for (const FAIHeroesRoutePlan& Route : Ranked)
    {
        TestFalse(TEXT("No recommendation can include a known losing mine"),
            Route.Stops.Contains(FName(TEXT("ORE"))));
    }
    return true;
}
#endif
