#include "AIHeroesBattleAftermath.h"
#include "AIHeroesBattleRecord.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AIHeroesGridBattle.h"
#include "Misc/AutomationTest.h"

namespace
{
FAIHeroesBattleRecord MakeRecord(bool bDefeat=false)
{
    TArray<FAIHeroesArmyStack> Player={FAIHeroesArmyStack(TEXT("Militia"),
        bDefeat?1:8,12,4,2,3)};
    if(!bDefeat)Player.Add(FAIHeroesArmyStack(TEXT("Scout"),3,8,6,1,7));
    const TArray<FAIHeroesArmyStack> Enemy={FAIHeroesArmyStack(TEXT("Wolf"),
        bDefeat?40:4,9,3,1,8)};
    return FAIHeroesBattleRecordRules::Capture(TEXT("FOREST"),2,0,Player,Enemy,
        FAIHeroesGridBattle::RunToEnd(Player,Enemy,0),2);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBattleAftermathLosses,
    "AIHeroes.Playability.BattleAftermath.VictoryAndWounded",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBattleAftermathLosses::RunTest(const FString&)
{
    const auto Record = MakeRecord();
    const auto View = FAIHeroesBattleAftermath::Build(Record);
    TestTrue(TEXT("Fixture has genuine partial-HP survivors"),
        Record.bVictory&&Record.PlayerAfter[0].TotalHealth>0&&
        Record.PlayerAfter[0].TotalHealth<Record.PlayerBefore[0].TotalHealth);
    TestTrue(TEXT("Valid committed data has count display"), View.bCountsAvailable);
    TestTrue(TEXT("After/before troop counts reflect partial HP"),
        View.PlayerLine.Contains(TEXT("11 из 11")));
    TestTrue(TEXT("Only actual whole-unit casualties are claimed"),
        View.PlayerLine.Contains(TEXT("потери: 0")));
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
    const auto Record = MakeRecord(true);
    const auto View = FAIHeroesBattleAftermath::Build(Record);
    TestTrue(TEXT("Defeat still reports observed own-army losses"), View.bCountsAvailable);
    TestTrue(TEXT("Actual lone fighter is lost"),
        Record.bDefeat && View.PlayerLine.Contains(TEXT("0 из 1")) &&
        View.PlayerLine.Contains(TEXT("потери: 1")));
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
    TestTrue(TEXT("Positive control has verified casualty facts"),Before.bCountsAvailable);
    TestFalse(TEXT("Unverifiable enemy metadata invalidates the whole record"),After.bCountsAvailable);
    TestFalse(TEXT("No opponent identity in player casualty text"),
        After.PlayerLine.Contains(TEXT("SecretGuard")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBattleAftermathAppliedRewards,
    "AIHeroes.Playability.BattleAftermath.OnlyActualCommittedRewards",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBattleAftermathAppliedRewards::RunTest(const FString&)
{
    const auto Record=MakeRecord();
    const auto Historic=FAIHeroesBattleAftermath::Build(Record);
    TestFalse(TEXT("Historical record has no authoritative reward receipt"),
        Historic.bRewardsVerified);
    TestTrue(TEXT("Legacy dossier never guesses current reward amounts"),
        Historic.RewardLine.Contains(TEXT("нет подтверждённой")));
    FAIHeroesBattleCommitReceipt Receipt;
    Receipt.BoundRecord=Record;
    Receipt.NodeId=Record.NodeId;
    Receipt.Day=Record.Day;
    Receipt.RuleVersion=Record.RuleVersion;
    Receipt.MovesSpent=2;
    Receipt.GoldGranted=0;
    Receipt.DataGranted=2;
    Receipt.bApplied=true;
    const auto Applied=FAIHeroesBattleAftermath::Build(Record,&Receipt);
    TestTrue(TEXT("Matching one-time actual commit allows the reward line"),
        Applied.bRewardsVerified);
    TestTrue(TEXT("Only committed data are displayed"),
        Applied.RewardLine.Contains(TEXT("+2 данных")) &&
        Applied.RewardLine.Contains(TEXT("ходов: 2")));
    Receipt.bApplied=false;
    TestFalse(TEXT("Preflight without commit never grants UI reward evidence"),
        FAIHeroesBattleAftermath::Build(Record,&Receipt).bRewardsVerified);
    Receipt.bApplied=true;
    Receipt.NodeId=FName(TEXT("RUINS"));
    TestFalse(TEXT("Another battle cannot lend its reward receipt"),
        FAIHeroesBattleAftermath::Build(Record,&Receipt).bRewardsVerified);
    Receipt.NodeId=Record.NodeId;
    Receipt.Day=Record.Day+1;
    TestFalse(TEXT("A receipt from another day cannot match"),
        FAIHeroesBattleAftermath::Build(Record,&Receipt).bRewardsVerified);
    Receipt.Day=Record.Day;
    Receipt.RuleVersion=1;
    TestFalse(TEXT("Another rule revision cannot lend rewards"),
        FAIHeroesBattleAftermath::Build(Record,&Receipt).bRewardsVerified);
    Receipt.RuleVersion=Record.RuleVersion;
    Receipt.DataGranted=-2;
    TestFalse(TEXT("Negative or corrupt reward data fails closed"),
        FAIHeroesBattleAftermath::Build(Record,&Receipt).bRewardsVerified);
    Receipt.DataGranted=2;
    Receipt.MovesSpent=5;
    TestFalse(TEXT("Impossible move cost cannot be displayed as paid"),
        FAIHeroesBattleAftermath::Build(Record,&Receipt).bRewardsVerified);
    const auto Defeat=MakeRecord(true);
    Receipt.BoundRecord=Defeat;
    Receipt.MovesSpent=2;
    TestFalse(TEXT("Defeat cannot display a fabricated positive reward"),
        FAIHeroesBattleAftermath::Build(Defeat,&Receipt).bRewardsVerified);
    Receipt.DataGranted=0;
    TestTrue(TEXT("A committed defeat honestly shows zero reward"),
        FAIHeroesBattleAftermath::Build(Defeat,&Receipt).bRewardsVerified);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBattleAftermathDuplicateGroups,
    "AIHeroes.Playability.BattleAftermath.IndexedGroupsPartialHP",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBattleAftermathDuplicateGroups::RunTest(const FString&)
{
    auto Record=MakeRecord();
    TArray<FAIHeroesArmyStack> Player={
        FAIHeroesArmyStack(TEXT("Militia"),8,12,4,2,3),
        FAIHeroesArmyStack(TEXT("Militia"),4,12,4,2,3)};
    Player[1].TotalHealth=37; // Four living units; the last is wounded.
    const TArray<FAIHeroesArmyStack> Enemy={FAIHeroesArmyStack(TEXT("Wolf"),4,9,3,1,8)};
    Record=FAIHeroesBattleRecordRules::Capture(TEXT("FOREST"),2,0,Player,Enemy,
        FAIHeroesGridBattle::RunToEnd(Player,Enemy,0),2);
    const auto View=FAIHeroesBattleAftermath::Build(Record);
    TestTrue(TEXT("Two indexed copies of the same unit have safe counts"),
        View.bCountsAvailable && View.PlayerGroups.Num()==2);
    if(View.PlayerGroups.Num()!=2)return false;
    TestTrue(TEXT("First group retains its own initial count"),
        View.PlayerGroups[0].Contains(TEXT("A1")) &&
        View.PlayerGroups[0].Contains(TEXT("8 из 8")));
    TestTrue(TEXT("Second group has its own four living units and partial HP"),
        View.PlayerGroups[1].Contains(TEXT("A2")) &&
        View.PlayerGroups[1].Contains(TEXT("4 из 4")) &&
        View.PlayerGroups[1].Contains(TEXT("ОЗ 37")));
    TestTrue(TEXT("Pre-existing wounds never become fictitious new whole-unit losses"),
        View.PlayerLine.Contains(TEXT("12 из 12")) &&
        View.PlayerLine.Contains(TEXT("потери: 0")));
    Record.PlayerAfter[1].UnitId=FName(TEXT("Scout"));
    TestFalse(TEXT("Mismatched group identity fails closed"),
        FAIHeroesBattleAftermath::Build(Record).bCountsAvailable);
    return true;
}
#endif
