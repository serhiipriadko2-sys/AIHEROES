#include "AIHeroesTurnRules.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesTravelBudgetTest,
    "AIHeroes.Core.TravelBudget",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesTravelBudgetTest::RunTest(const FString& Parameters)
{
    const FVector Home(-650.f, 0.f, 155.f);
    const FVector Mine(150.f, 600.f, 120.f);

    TestEqual(TEXT("HQ to mine requires two movement points"),
        FAIHeroesTurnRules::TravelCost(Home, Mine, true), 2);
    TestEqual(TEXT("Home to itself is free without defenders"),
        FAIHeroesTurnRules::TravelCost(Home, Home, false), 0);
    TestEqual(TEXT("Hostile combat consumes at least one move"),
        FAIHeroesTurnRules::TravelCost(Home, Home, true), 1);
    TestTrue(TEXT("Action allowed with sufficient movement"),
        FAIHeroesTurnRules::CanMove(2, 2));
    TestFalse(TEXT("No attack when movement budget insufficient"),
        FAIHeroesTurnRules::CanMove(1, 2));
    TestFalse(TEXT("Negative cost rejected"),
        FAIHeroesTurnRules::CanMove(3, -1));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesAtomicSpendTest,
    "AIHeroes.Core.AtomicResourceSpend",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesAtomicSpendTest::RunTest(const FString& Parameters)
{
    int32 Gold = 240;
    int32 Wood = 35;
    int32 Ore = 30;

    TestTrue(TEXT("Valid recruitment cost paid"),
        FAIHeroesTurnRules::SpendIfAffordable(Gold, Wood, Ore, 50, 5, 0));
    TestEqual(TEXT("Gold debited once"), Gold, 190);
    TestEqual(TEXT("Wood debited once"), Wood, 30);
    TestEqual(TEXT("Ore unchanged"), Ore, 30);
    TestFalse(TEXT("Over-budget transaction rejected"),
        FAIHeroesTurnRules::SpendIfAffordable(Gold, Wood, Ore, 999, 0, 0));
    TestFalse(TEXT("Negative cost rejected"),
        FAIHeroesTurnRules::SpendIfAffordable(Gold, Wood, Ore, -1, 0, 0));
    TestEqual(TEXT("Reject leaves gold unchanged"), Gold, 190);
    TestEqual(TEXT("Reject leaves wood unchanged"), Wood, 30);
    TestEqual(TEXT("Reject leaves ore unchanged"), Ore, 30);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesModelSlotSpendTest,
    "AIHeroes.Core.ModelSlotPurchase",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesModelSlotSpendTest::RunTest(const FString& Parameters)
{
    int32 Gold = 100;
    int32 Data = 2;
    TestTrue(TEXT("Opening a real future adapter SLOT consumes its own currency"),
        FAIHeroesTurnRules::SpendModelSlotTokens(Gold, Data, 60, 2));
    TestEqual(TEXT("First slot gold cost"), Gold, 40);
    TestEqual(TEXT("First slot data cost"), Data, 0);
    TestFalse(TEXT("Unfunded next model slot is rejected"),
        FAIHeroesTurnRules::SpendModelSlotTokens(Gold, Data, 120, 6));
    TestFalse(TEXT("Negative model upgrade cost is rejected"),
        FAIHeroesTurnRules::SpendModelSlotTokens(Gold, Data, -1, 1));
    TestEqual(TEXT("Failed slot upgrade does not modify gold"), Gold, 40);
    TestEqual(TEXT("Failed slot upgrade does not modify data"), Data, 0);
    return true;
}
#endif
