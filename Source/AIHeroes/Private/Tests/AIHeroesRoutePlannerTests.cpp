#include "AIHeroesRoutePlanner.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AIHeroesBattleEngine.h"
#include "Misc/AutomationTest.h"

namespace
{
FAIHeroesAdvisorSnapshot MakeRouteScenario()
{
    FAIHeroesAdvisorSnapshot S;
    S.ProtocolTier = 3;
    S.MovesLeft = 3;
    S.Gold = 300;
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

    FAIHeroesAdvisorNode Ruins;
    Ruins.Id = FName(TEXT("RUINS"));
    Ruins.Type = EAIHeroesNodeType::Ruins;
    Ruins.Location = FVector(2500.f, 0.f, 155.f);
    Ruins.Guards.Add(FAIHeroesArmyStack(TEXT("Guardian"), 100, 24, 7, 4, 2));
    S.Nodes.Add(Ruins);
    return S;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesTwoStopRouteTest, "AIHeroes.Advisor.TwoStopRoute",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesTwoStopRouteTest::RunTest(const FString& Parameters)
{
    FAIHeroesAdvisorSnapshot S = MakeRouteScenario();

    const FAIHeroesRoutePlan Plan = FAIHeroesRoutePlanner::Plan(S);
    TestTrue(TEXT("Planner found an available route"), Plan.IsValid());
    TestEqual(TEXT("Two stops included"), Plan.Stops.Num(), 2);
    if (Plan.Stops.Num() == 2)
    {
        TestEqual(TEXT("Economical first stop"), Plan.Stops[0], FName(TEXT("FOREST")));
        TestEqual(TEXT("Then another resource mine"), Plan.Stops[1], FName(TEXT("ORE")));
    }
    TestEqual(TEXT("Two local hops cost two movement points"), Plan.TotalMoves, 2);
    TestEqual(TEXT("No combat means zero losses"), Plan.ExpectedHpLoss, 0);
    TestTrue(TEXT("Advice explicitly describes both stops"),
        Plan.Describe().Contains(TEXT("FOREST -> ORE")));
    TestEqual(TEXT("Same snapshot gives identical route"),
        FAIHeroesRoutePlanner::Plan(S).Describe(), Plan.Describe());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesRouteBudgetTest, "AIHeroes.Advisor.RouteBudgetAndAuthority",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesRouteBudgetTest::RunTest(const FString& Parameters)
{
    FAIHeroesAdvisorSnapshot S = MakeRouteScenario();
    S.MovesLeft = 1;
    const FAIHeroesRoutePlan One = FAIHeroesRoutePlanner::Plan(S);
    TestTrue(TEXT("One reachable stop is still permitted"), One.IsValid());
    TestEqual(TEXT("One move cannot buy two separate hops"), One.Stops.Num(), 1);
    if (One.Stops.Num() == 1)
    {
        TestEqual(TEXT("Only reachable mine is forest"), One.Stops[0], FName(TEXT("FOREST")));
    }
    TestTrue(TEXT("Travel cost stays inside the turn budget"), One.TotalMoves <= S.MovesLeft);

    S.ProtocolTier = 1;
    TestFalse(TEXT("Early protocol cannot access route planning"),
        FAIHeroesRoutePlanner::Plan(S).IsValid());

    S.ProtocolTier = 3;
    S.MovesLeft = 0;
    TestFalse(TEXT("No moves cannot trigger a plan"),
        FAIHeroesRoutePlanner::Plan(S).IsValid());

    S.MovesLeft = 3;
    // Two snapshot entries may refer to the same map ID in untrusted input.
    // Even then, a plan must not promise capture of the same node twice.
    S.Nodes[2].Id = FName(TEXT("FOREST"));
    const FAIHeroesRoutePlan Duplicate = FAIHeroesRoutePlanner::Plan(S);
    TestTrue(TEXT("Does not offer two captures of identical node IDs"),
        Duplicate.Stops.Num() < 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesRouteArmyForecastTest, "AIHeroes.Advisor.RoutePostCombatSurvivors",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesRouteArmyForecastTest::RunTest(const FString& Parameters)
{
    FAIHeroesAdvisorSnapshot S = MakeRouteScenario();
    S.Nodes[1].Guards.Add(FAIHeroesArmyStack(TEXT("Wolf"), 2, 9, 3, 1, 8));
    S.Nodes[2].Guards.Add(FAIHeroesArmyStack(TEXT("Guardian"), 100, 24, 7, 4, 2));
    const FAIHeroesRoutePlan Plan = FAIHeroesRoutePlanner::Plan(S);
    TestTrue(TEXT("Planner can still consider first winnable battle"), Plan.IsValid());
    TestEqual(TEXT("Does not promise a later impossible battle"), Plan.Stops.Num(), 1);
    if (!Plan.IsValid()) return false;

    const FAIHeroesBattleOutcome Fight = FAIHeroesBattleEngine::Resolve(
        S.PlayerArmy, S.Nodes[1].Guards, S.ProtocolTier);
    TestTrue(TEXT("Initial battle is actually winnable"), Fight.bVictory);
    TestTrue(TEXT("Route forecasts non-negative casualties"), Plan.ExpectedHpLoss >= 0);
    return true;
}
#endif
