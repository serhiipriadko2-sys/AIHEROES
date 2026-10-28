#include "AIHeroesBattleEngine.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesDeterministicBattleTest,
    "AIHeroes.Core.DeterministicBattle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesDeterministicBattleTest::RunTest(const FString& Parameters)
{
    const TArray<FAIHeroesArmyStack> Heroes = {
        FAIHeroesArmyStack(TEXT("Militia"), 8, 12, 4, 2, 3),
        FAIHeroesArmyStack(TEXT("Scout"), 3, 8, 6, 1, 7)
    };
    const TArray<FAIHeroesArmyStack> Enemies = {
        FAIHeroesArmyStack(TEXT("Wolf"), 4, 9, 3, 1, 8)
    };

    const FAIHeroesBattleOutcome A = FAIHeroesBattleEngine::Resolve(Heroes, Enemies, 1);
    const FAIHeroesBattleOutcome B = FAIHeroesBattleEngine::Resolve(Heroes, Enemies, 1);

    TestEqual(TEXT("Identical winner"), A.bVictory, B.bVictory);
    TestEqual(TEXT("Identical number of rounds"), A.Rounds, B.Rounds);
    TestEqual(TEXT("Identical trace length"), A.Trace.Num(), B.Trace.Num());
    TestEqual(TEXT("Identical summary"), A.Summary, B.Summary);
    TestTrue(TEXT("At least one attack was simulated"), A.Trace.Num() > 0);
    for (int32 Index = 0; Index < A.PlayerAfter.Num(); ++Index)
    {
        TestEqual(TEXT("Survivors deterministic"), A.PlayerAfter[Index].TotalHealth, B.PlayerAfter[Index].TotalHealth);
        TestTrue(TEXT("HP not negative"), A.PlayerAfter[Index].TotalHealth >= 0);
        TestTrue(TEXT("HP cannot exceed initial"), A.PlayerAfter[Index].TotalHealth <= Heroes[Index].TotalHealth);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesNoArmyTest,
    "AIHeroes.Core.NoArmyCannotWin",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesNoArmyTest::RunTest(const FString& Parameters)
{
    const FAIHeroesBattleOutcome Result = FAIHeroesBattleEngine::Resolve(
        TArray<FAIHeroesArmyStack>(),
        {FAIHeroesArmyStack(TEXT("Guard"), 2, 10, 3, 1, 3)},
        3);
    TestFalse(TEXT("Empty army cannot win"), Result.bVictory);
    TestTrue(TEXT("Empty army loses"), Result.bDefeat);
    TestEqual(TEXT("No turns should execute"), Result.Rounds, 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesTacticalUpgradeTest,
    "AIHeroes.Core.ProtocolChangesTactics",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesTacticalUpgradeTest::RunTest(const FString& Parameters)
{
    const TArray<FAIHeroesArmyStack> Heroes = {
        FAIHeroesArmyStack(TEXT("Ranger"), 3, 12, 6, 1, 12)
    };
    const TArray<FAIHeroesArmyStack> Enemies = {
        FAIHeroesArmyStack(TEXT("Tank"), 1, 40, 5, 5, 2),
        FAIHeroesArmyStack(TEXT("Weak"), 1, 8, 2, 0, 1)
    };

    const FAIHeroesBattleOutcome Seed = FAIHeroesBattleEngine::Resolve(Heroes, Enemies, 0);
    const FAIHeroesBattleOutcome Planner = FAIHeroesBattleEngine::Resolve(Heroes, Enemies, 3);
    TestTrue(TEXT("Seed AI prioritizes first enemy"),
        Seed.Trace.Num() > 0 && Seed.Trace[0].Contains(TEXT("-> Tank")));
    TestTrue(TEXT("Planner prioritizes lethal strike"),
        Planner.Trace.Num() > 0 && Planner.Trace[0].Contains(TEXT("-> Weak")));
    return true;
}
#endif
