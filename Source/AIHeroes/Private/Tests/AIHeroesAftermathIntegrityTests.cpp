#include "AIHeroesBattleAftermath.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AIHeroesBattleRecord.h"
#include "AIHeroesGridBattle.h"
#include "Misc/AutomationTest.h"

namespace
{
FAIHeroesBattleRecord MakeAftermathIntegrityRecord(int32 Tier)
{
    const TArray<FAIHeroesArmyStack> Player={
        FAIHeroesArmyStack(TEXT("Militia"),8,12,4,2,3),
        FAIHeroesArmyStack(TEXT("Scout"),3,8,6,1,7)};
    const TArray<FAIHeroesArmyStack> Enemy={
        FAIHeroesArmyStack(TEXT("Wolf"),4,9,3,1,8)};
    return FAIHeroesBattleRecordRules::Capture(TEXT("FOREST"),1,Tier,
        Player,Enemy,FAIHeroesGridBattle::RunToEnd(Player,Enemy,Tier),2);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAftermathRejectsCorruptTrace,
    "AIHeroes.Playability.AftermathIntegrity.CorruptTraceHasNoCasualtyFacts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAftermathRejectsCorruptTrace::RunTest(const FString&)
{
    auto Record=MakeAftermathIntegrityRecord(0);
    FString Error;
    if(!TestTrue(TEXT("Control record is genuinely replay-verifiable"),
        FAIHeroesBattleRecordRules::Verify(Record,Error)))return false;
    if(!TestTrue(TEXT("Control record has actual events"),!Record.Trace.IsEmpty()))return false;
    Record.Trace[0]+=TEXT(" corrupted");
    TestFalse(TEXT("The modified transcript fails the existing verifier"),
        FAIHeroesBattleRecordRules::Verify(Record,Error));
    const auto View=FAIHeroesBattleAftermath::Build(Record);
    TestFalse(TEXT("Unverified history cannot claim casualty analysis"),View.bCountsAvailable);
    TestFalse(TEXT("Unverified history cannot claim a reward"),View.bRewardsVerified);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAftermathBindsWholeBattle,
    "AIHeroes.Playability.AftermathIntegrity.SameMetadataCannotBorrowReward",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAftermathBindsWholeBattle::RunTest(const FString&)
{
    const auto Original=MakeAftermathIntegrityRecord(0);
    const auto Different=MakeAftermathIntegrityRecord(1);
    FString Error;
    if(!TestTrue(TEXT("Original record verifies"),FAIHeroesBattleRecordRules::Verify(Original,Error)) ||
       !TestTrue(TEXT("Different-policy record verifies"),FAIHeroesBattleRecordRules::Verify(Different,Error)))return false;
    TestFalse(TEXT("Same site/day/rule version need not mean the same battle"),
        FAIHeroesBattleRecordRules::Equivalent(Original,Different));
    FAIHeroesBattleCommitReceipt Receipt;
    Receipt.NodeId=Original.NodeId;Receipt.Day=Original.Day;
    Receipt.RuleVersion=Original.RuleVersion;Receipt.MovesSpent=2;
    Receipt.GoldGranted=0;Receipt.DataGranted=2;Receipt.bApplied=true;
    TestTrue(TEXT("Positive control accepts its own applied evidence"),
        FAIHeroesBattleAftermath::Build(Original,&Receipt).bRewardsVerified);
    TestFalse(TEXT("Applied evidence must not be borrowed by another battle"),
        FAIHeroesBattleAftermath::Build(Different,&Receipt).bRewardsVerified);
    return true;
}
#endif
