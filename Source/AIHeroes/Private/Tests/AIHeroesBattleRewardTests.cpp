#include "AIHeroesBattleRewardRules.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AIHeroesBaseRules.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesBattleRewardBoundaryTest,
    "AIHeroes.BattleRewards.NormalAndExactBoundaries",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesBattleRewardBoundaryTest::RunTest(const FString& Parameters)
{
    const int32 Cap = FAIHeroesBaseRules::MaxResource;
    FString Error;
    FAIHeroesBattleRewardPlan Plan;
    for (EAIHeroesNodeType Type : {EAIHeroesNodeType::ForestMine,
        EAIHeroesNodeType::OreMine, EAIHeroesNodeType::Outpost})
    {
        TestTrue(TEXT("Ordinary victory is planned"),
            FAIHeroesBattleRewardRules::Plan(true, Type, Cap, Cap - 2, Plan, Error));
        TestEqual(TEXT("Ordinary site grants no gold"), Plan.GoldReward, 0);
        TestEqual(TEXT("Ordinary site grants two data"), Plan.DataReward, 2);
        TestEqual(TEXT("Gold at cap is preserved"), Plan.GoldAfter, Cap);
        TestEqual(TEXT("Exactly reaching data cap is allowed"), Plan.DataAfter, Cap);
    }
    TestTrue(TEXT("Ruins exact combined boundary is planned"),
        FAIHeroesBattleRewardRules::Plan(true, EAIHeroesNodeType::Ruins,
            Cap - 100, Cap - 4, Plan, Error));
    TestEqual(TEXT("Ruins grants 100 gold"), Plan.GoldReward, 100);
    TestEqual(TEXT("Ruins grants four data"), Plan.DataReward, 4);
    TestEqual(TEXT("Ruins reaches gold cap exactly"), Plan.GoldAfter, Cap);
    TestEqual(TEXT("Ruins reaches data cap exactly"), Plan.DataAfter, Cap);
    TestTrue(TEXT("Success clears earlier error"), Error.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesBattleRewardRejectedTest,
    "AIHeroes.BattleRewards.RejectWithoutPartialOutput",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesBattleRewardRejectedTest::RunTest(const FString& Parameters)
{
    const int32 Cap = FAIHeroesBaseRules::MaxResource;
    struct FInvalidRewardInput
    {
        EAIHeroesNodeType Type;
        int32 Gold;
        int32 Data;
    };
    const FInvalidRewardInput Cases[] = {
        {EAIHeroesNodeType::ForestMine, 0, Cap - 1},
        {EAIHeroesNodeType::Ruins, Cap - 99, Cap - 4},
        {EAIHeroesNodeType::Ruins, Cap - 100, Cap - 3},
        {EAIHeroesNodeType::Ruins, Cap, Cap},
        {EAIHeroesNodeType::ForestMine, -1, 0},
        {EAIHeroesNodeType::ForestMine, 0, -1},
        {EAIHeroesNodeType::ForestMine, Cap + 1, 0},
        {EAIHeroesNodeType::ForestMine, 0, MAX_int32},
        {EAIHeroesNodeType::Headquarters, 0, 0},
        {static_cast<EAIHeroesNodeType>(255), 0, 0}
    };
    for (const FInvalidRewardInput& Input : Cases)
    {
        FAIHeroesBattleRewardPlan Plan{11, 12, 13, 14};
        FString Error;
        TestFalse(TEXT("Invalid/overflowing reward is rejected"),
            FAIHeroesBattleRewardRules::Plan(true, Input.Type,
                Input.Gold, Input.Data, Plan, Error));
        TestFalse(TEXT("Failure has an explanation"), Error.IsEmpty());
        TestEqual(TEXT("Failure preserves prior gold total"), Plan.GoldAfter, 11);
        TestEqual(TEXT("Failure preserves prior data total"), Plan.DataAfter, 12);
        TestEqual(TEXT("Failure preserves prior gold reward"), Plan.GoldReward, 13);
        TestEqual(TEXT("Failure preserves prior data reward"), Plan.DataReward, 14);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesBattleRewardNoVictoryTest,
    "AIHeroes.BattleRewards.DefeatAndStalemateGrantNothing",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesBattleRewardNoVictoryTest::RunTest(const FString& Parameters)
{
    const int32 Cap = FAIHeroesBaseRules::MaxResource;
    FString Error;
    FAIHeroesBattleRewardPlan Plan;
    for (EAIHeroesNodeType Type : {EAIHeroesNodeType::ForestMine, EAIHeroesNodeType::Ruins})
    {
        TestTrue(TEXT("Non-victory is committable even with full resources"),
            FAIHeroesBattleRewardRules::Plan(false, Type, Cap, Cap, Plan, Error));
        TestEqual(TEXT("No victory keeps gold"), Plan.GoldAfter, Cap);
        TestEqual(TEXT("No victory keeps data"), Plan.DataAfter, Cap);
        TestEqual(TEXT("No victory gives no gold reward"), Plan.GoldReward, 0);
        TestEqual(TEXT("No victory gives no data reward"), Plan.DataReward, 0);
    }
    TestFalse(TEXT("Even a non-victory cannot commit invalid resource input"),
        FAIHeroesBattleRewardRules::Plan(false, EAIHeroesNodeType::ForestMine,
            -1, 0, Plan, Error));
    return true;
}
#endif
