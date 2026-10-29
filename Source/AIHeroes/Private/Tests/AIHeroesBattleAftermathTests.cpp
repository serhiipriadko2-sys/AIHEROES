#include "AIHeroesBattleAftermath.h"
#include "AIHeroesBattleRecord.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

namespace
{
FAIHeroesBattleRecord MakeRecord()
{
    FAIHeroesBattleRecord Record;
    Record.NodeId = FName(TEXT("FOREST"));
    Record.Day = 2;
    Record.RuleVersion = 2;
    Record.bVictory = true;
    Record.PlayerBefore = {
        FAIHeroesArmyStack(TEXT("Militia"), 8, 10, 4, 2, 4),
        FAIHeroesArmyStack(TEXT("Scout"), 3, 8, 6, 1, 7)
    };
    Record.PlayerAfter = {
        FAIHeroesArmyStack(TEXT("Militia"), 5, 10, 4, 2, 4),
        FAIHeroesArmyStack(TEXT("Scout"), 2, 8, 6, 1, 7)
    };
    // Two living scouts, even with one of them wounded.
    Record.PlayerAfter[1].TotalHealth = 9;
    return Record;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBattleAftermathLosses,
    "AIHeroes.Playability.BattleAftermath.VictoryAndWounded",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBattleAftermathLosses::RunTest(const FString&)
{
    const auto Record = MakeRecord();
    const auto View = FAIHeroesBattleAftermath::Build(Record);
    TestTrue(TEXT("Valid committed data has count display"), View.bCountsAvailable);
    TestTrue(TEXT("After/before troop counts reflect partial HP"),
        View.PlayerLine.Contains(TEXT("7 из 11")));
    TestTrue(TEXT("Only actual whole-unit casualties are claimed"),
        View.PlayerLine.Contains(TEXT("потери: 4")));
    TestTrue(TEXT("Player sees the available next action"),
        View.PlayerLine.Contains(TEXT("На карту")));
    TestFalse(TEXT("No unsupported reward amount is invented"),
        View.PlayerLine.Contains(TEXT("золота")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBattleAftermathDefeat,
    "AIHeroes.Playability.BattleAftermath.DefeatAndNoSurvivors",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBattleAftermathDefeat::RunTest(const FString&)
{
    auto Record = MakeRecord();
    Record.bVictory = false;
    Record.bDefeat = true;
    Record.PlayerAfter.Empty();
    const auto View = FAIHeroesBattleAftermath::Build(Record);
    TestTrue(TEXT("Defeat still reports observed own-army losses"), View.bCountsAvailable);
    TestTrue(TEXT("All eleven fighters are lost"),
        View.PlayerLine.Contains(TEXT("0 из 11")) &&
        View.PlayerLine.Contains(TEXT("потери: 11")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBattleAftermathInvalid,
    "AIHeroes.Playability.BattleAftermath.InvalidFailsClosed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBattleAftermathInvalid::RunTest(const FString&)
{
    auto Record = MakeRecord();
    Record.NodeId = NAME_None;
    auto View = FAIHeroesBattleAftermath::Build(Record);
    TestFalse(TEXT("Missing battle identity cannot claim casualties"),
        View.bCountsAvailable);
    TestTrue(TEXT("Fallback keeps a path back"),
        View.PlayerLine.Contains(TEXT("На карту")));

    Record = MakeRecord();
    Record.bDefeat = true;
    TestFalse(TEXT("Contradictory outcome fails closed"),
        FAIHeroesBattleAftermath::Build(Record).bCountsAvailable);

    Record = MakeRecord();
    Record.PlayerAfter[0].UnitHealth = 0;
    TestFalse(TEXT("Invalid per-unit HP fails closed"),
        FAIHeroesBattleAftermath::Build(Record).bCountsAvailable);

    Record = MakeRecord();
    Record.PlayerAfter[0].TotalHealth = -1;
    TestFalse(TEXT("Negative health fails closed"),
        FAIHeroesBattleAftermath::Build(Record).bCountsAvailable);

    Record = MakeRecord();
    Record.PlayerAfter[0].TotalHealth = 10000;
    TestFalse(TEXT("Unexpected army growth cannot claim negative losses"),
        FAIHeroesBattleAftermath::Build(Record).bCountsAvailable);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBattleAftermathFogSafety,
    "AIHeroes.Playability.BattleAftermath.NoEnemyOrRewardLeak",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBattleAftermathFogSafety::RunTest(const FString&)
{
    auto Record = MakeRecord();
    const auto Before = FAIHeroesBattleAftermath::Build(Record);
    Record.EnemyBefore.Add(FAIHeroesArmyStack(TEXT("SecretGuard"), 999, 100, 1, 1, 1));
    Record.EnemyAfter.Add(FAIHeroesArmyStack(TEXT("SecretGuard"), 1, 100, 1, 1, 1));
    const auto After = FAIHeroesBattleAftermath::Build(Record);
    TestEqual(TEXT("Enemy metadata does not change player casualty text"),
        Before.PlayerLine, After.PlayerLine);
    TestFalse(TEXT("No opponent identity in player casualty text"),
        After.PlayerLine.Contains(TEXT("SecretGuard")));
    return true;
}
#endif
