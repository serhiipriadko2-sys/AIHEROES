#include "AIHeroesBattleEngine.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

namespace
{
TArray<FAIHeroesArmyStack> MakeHeroes()
{
    return {
        FAIHeroesArmyStack(TEXT("Militia"), 8, 12, 4, 2, 3),
        FAIHeroesArmyStack(TEXT("Scout"), 3, 8, 6, 1, 7)
    };
}

TArray<FAIHeroesArmyStack> MakeEnemies()
{
    return {
        FAIHeroesArmyStack(TEXT("Raider"), 10, 11, 5, 2, 5),
        FAIHeroesArmyStack(TEXT("Wolf"), 4, 9, 3, 1, 8)
    };
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesTypedTimelineTest,
    "AIHeroes.Core.TypedBattleTimeline",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesTypedTimelineTest::RunTest(const FString& Parameters)
{
    const TArray<FAIHeroesArmyStack> Heroes = MakeHeroes();
    const TArray<FAIHeroesArmyStack> Enemies = MakeEnemies();

    for (int32 Tier = 0; Tier <= 3; ++Tier)
    {
        const FAIHeroesBattleOutcome Result =
            FAIHeroesBattleEngine::Resolve(Heroes, Enemies, Tier);
        TestTrue(FString::Printf(TEXT("Tier %d trace verifies"), Tier),
            FAIHeroesBattleEngine::VerifyReplay(Heroes, Enemies, Result));
        TestEqual(TEXT("One typed receipt per rendered log line"),
            Result.Actions.Num(), Result.Trace.Num());
        TestTrue(TEXT("Every actual battle has actions"), !Result.Actions.IsEmpty());

        for (int32 Index = 0; Index < Result.Actions.Num(); ++Index)
        {
            const FAIHeroesBattleAction& Action = Result.Actions[Index];
            TestEqual(TEXT("Typed and textual event agree"),
                Action.ToLogLine(), Result.Trace[Index]);
            TestTrue(TEXT("HP never rises during attack"),
                Action.TargetHealthBefore >= Action.TargetHealthAfter);
            TestTrue(TEXT("Every actual attack has positive damage"),
                Action.Damage > 0);
        }

        const FAIHeroesBattleOutcome Again =
            FAIHeroesBattleEngine::Resolve(Heroes, Enemies, Tier);
        TestEqual(TEXT("Replay length deterministic"),
            Result.Actions.Num(), Again.Actions.Num());
        for (int32 Index = 0; Index < Result.Actions.Num(); ++Index)
        {
            TestEqual(TEXT("Replay event deterministic"),
                Result.Actions[Index].ToLogLine(), Again.Actions[Index].ToLogLine());
            TestEqual(TEXT("Replay HP deterministic"),
                Result.Actions[Index].TargetHealthAfter,
                Again.Actions[Index].TargetHealthAfter);
        }
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesTamperReplayTest,
    "AIHeroes.Core.ReplayTamperRejected",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesTamperReplayTest::RunTest(const FString& Parameters)
{
    const TArray<FAIHeroesArmyStack> Heroes = MakeHeroes();
    const TArray<FAIHeroesArmyStack> Enemies = MakeEnemies();
    const FAIHeroesBattleOutcome Valid =
        FAIHeroesBattleEngine::Resolve(Heroes, Enemies, 3);

    if (!TestTrue(TEXT("Fixture contains a combat action"), Valid.Actions.Num() > 0))
    {
        return false;
    }
    TestTrue(TEXT("Fixture valid"), FAIHeroesBattleEngine::VerifyReplay(Heroes, Enemies, Valid));

    FAIHeroesBattleOutcome Tampered = Valid;
    Tampered.Actions[0].TargetHealthAfter += 1;
    TestFalse(TEXT("Forged health rejected"),
        FAIHeroesBattleEngine::VerifyReplay(Heroes, Enemies, Tampered));

    Tampered = Valid;
    Tampered.Actions[0].AttackerStackIndex = 99;
    TestFalse(TEXT("Forged attacker stack index rejected"),
        FAIHeroesBattleEngine::VerifyReplay(Heroes, Enemies, Tampered));

    Tampered = Valid;
    Tampered.Actions[0].Damage = 0;
    TestFalse(TEXT("Forged zero damage rejected"),
        FAIHeroesBattleEngine::VerifyReplay(Heroes, Enemies, Tampered));

    Tampered = Valid;
    Tampered.PlayerAfter[0].TotalHealth += 1;
    TestFalse(TEXT("Forged final army state rejected"),
        FAIHeroesBattleEngine::VerifyReplay(Heroes, Enemies, Tampered));

    Tampered = Valid;
    Tampered.Trace[0] = TEXT("R1 NPC fabricated move");
    TestFalse(TEXT("Forged textual record rejected"),
        FAIHeroesBattleEngine::VerifyReplay(Heroes, Enemies, Tampered));

    Tampered = Valid;
    Tampered.bVictory = !Valid.bVictory;
    TestFalse(TEXT("Forged winner rejected"),
        FAIHeroesBattleEngine::VerifyReplay(Heroes, Enemies, Tampered));

    Tampered = Valid;
    Tampered.Summary += TEXT(" forged");
    TestFalse(TEXT("Forged summary rejected"),
        FAIHeroesBattleEngine::VerifyReplay(Heroes, Enemies, Tampered));

    Tampered = Valid;
    Tampered.Rounds += 1;
    TestFalse(TEXT("Forged round count rejected"),
        FAIHeroesBattleEngine::VerifyReplay(Heroes, Enemies, Tampered));

    TestTrue(TEXT("Valid original unchanged after all rejections"),
        FAIHeroesBattleEngine::VerifyReplay(Heroes, Enemies, Valid));
    return true;
}
#endif
