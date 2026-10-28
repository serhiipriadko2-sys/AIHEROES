#include "AIHSimulation.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHCombatPolicyTest, "AIHeroes.Simulation.CombatPolicy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHCombatPolicyTest::RunTest(const FString& Parameters)
{
    const FAIHArmy StartingArmy;
    const FAIHCombatReport Easy = FAIHCombatEngine::Resolve(StartingArmy, 8, 0);
    TestTrue(TEXT("First enemy can be defeated"), Easy.bVictory);
    TestTrue(TEXT("Losses never exceed army size"),
        Easy.Casualties <= StartingArmy.Count());
    TestTrue(TEXT("No negative survivors"),
        Easy.Survivors.Scouts >= 0 && Easy.Survivors.Guardians >= 0 &&
        Easy.Survivors.Sparks >= 0);

    FAIHArmy EmptyArmy;
    EmptyArmy.Scouts = 0;
    EmptyArmy.Guardians = 0;
    const FAIHCombatReport Empty = FAIHCombatEngine::Resolve(EmptyArmy, 10, 5);
    TestFalse(TEXT("Empty army cannot win"), Empty.bVictory);

    const FAIHCombatReport Low = FAIHCombatEngine::Resolve(StartingArmy, 18, 0);
    const FAIHCombatReport Trained = FAIHCombatEngine::Resolve(StartingArmy, 18, 5);
    TestTrue(TEXT("Cognitive protocols improve the game policy"),
        Trained.OurPower > Low.OurPower);
    return true;
}
#endif
