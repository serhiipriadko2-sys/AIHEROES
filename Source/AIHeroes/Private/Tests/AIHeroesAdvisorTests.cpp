#include "AIHeroesAdvisor.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

namespace
{
FAIHeroesAdvisorSnapshot MakeAdvisorScenario()
{
    FAIHeroesAdvisorSnapshot S;
    S.ProtocolTier = 1;
    S.MovesLeft = 4;
    S.Gold = 240;
    S.Wood = 35;
    S.Ore = 30;
    S.HeroPosition = FVector(-650.f, 0.f, 155.f);
    S.SelectedNodeId = FName(TEXT("FOREST"));
    S.PlayerArmy.Add(FAIHeroesArmyStack(TEXT("Militia"), 8, 12, 4, 2, 3));
    S.PlayerArmy.Add(FAIHeroesArmyStack(TEXT("Scout"), 3, 8, 6, 1, 7));

    FAIHeroesAdvisorNode HQ;
    HQ.Id = FName(TEXT("HQ"));
    HQ.Type = EAIHeroesNodeType::Headquarters;
    HQ.bOwned = true;
    HQ.Location = S.HeroPosition;
    S.Nodes.Add(HQ);

    FAIHeroesAdvisorNode Forest;
    Forest.Id = FName(TEXT("FOREST"));
    Forest.Type = EAIHeroesNodeType::ForestMine;
    Forest.Location = FVector(-100.f, -650.f, 110.f);
    Forest.Guards.Add(FAIHeroesArmyStack(TEXT("Wolf"), 4, 9, 3, 1, 8));
    S.Nodes.Add(Forest);

    FAIHeroesAdvisorNode Ore;
    Ore.Id = FName(TEXT("ORE"));
    Ore.Type = EAIHeroesNodeType::OreMine;
    Ore.Location = FVector(150.f, 600.f, 120.f);
    Ore.Guards.Add(FAIHeroesArmyStack(TEXT("Bandit"), 6, 10, 4, 2, 4));
    S.Nodes.Add(Ore);
    return S;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesAdvisorProgressionTest,
    "AIHeroes.Advisor.ProgressionAndForecast",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesAdvisorProgressionTest::RunTest(const FString& Parameters)
{
    FAIHeroesAdvisorSnapshot S = MakeAdvisorScenario();
    S.ProtocolTier = 0;
    TestTrue(TEXT("Seed cannot forecast battles"),
        FAIHeroesAdvisor::Advise(S).Contains(TEXT("UNKNOWN")));

    S.ProtocolTier = 1;
    const FString Prediction = FAIHeroesAdvisor::Advise(S);
    TestTrue(TEXT("Analyst estimates outcome"), Prediction.Contains(TEXT("WIN FORECAST")));
    TestTrue(TEXT("Advisor provides cost of survival"), Prediction.Contains(TEXT("HP")));
    TestTrue(TEXT("Advisor never assumes permission to attack"),
        Prediction.Contains(TEXT("Player decides")));

    S.ProtocolTier = 3;
    S.LastBattle = TEXT("Previous target defended strongly");
    TestTrue(TEXT("Archivist exposes last battle evidence"),
        FAIHeroesAdvisor::Advise(S).Contains(TEXT("JOURNAL")));
    TestEqual(TEXT("Same snapshot yields identical advice"),
        FAIHeroesAdvisor::Advise(S), FAIHeroesAdvisor::Advise(S));

    S.MovesLeft = 0;
    TestTrue(TEXT("No movement cannot produce an attack recommendation"),
        FAIHeroesAdvisor::Advise(S).Contains(TEXT("need")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesAdvisorGlobalPlanTest,
    "AIHeroes.Advisor.GlobalStrategicChoice",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesAdvisorGlobalPlanTest::RunTest(const FString& Parameters)
{
    FAIHeroesAdvisorSnapshot S = MakeAdvisorScenario();
    S.SelectedNodeId = NAME_None;
    S.ProtocolTier = 3;
    const FString Recommendation = FAIHeroesAdvisor::Advise(S);
    TestTrue(TEXT("No selection can still generate a strategic suggestion"),
        Recommendation.Contains(TEXT("STRATEGY:")));
    TestTrue(TEXT("Planning leaves the action to the player"),
        Recommendation.Contains(TEXT("Select it yourself")));

    S.PlayerArmy.Empty();
    TestTrue(TEXT("Empty army prevents unsafe recommendations"),
        FAIHeroesAdvisor::Advise(S).Contains(TEXT("recruit")));

    S = MakeAdvisorScenario();
    S.ProtocolTier = 1;
    S.Nodes[1].bOwned = true;
    TestTrue(TEXT("Already owned target is not treated as an enemy"),
        FAIHeroesAdvisor::Advise(S).Contains(TEXT("already yours")));
    return true;
}
#endif
