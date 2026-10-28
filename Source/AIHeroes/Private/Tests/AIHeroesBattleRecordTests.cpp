#include "AIHeroesBattleRecord.h"
#include "AIHeroesBattleEngine.h"
#include "AIHeroesCampaignSave.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/Guid.h"

namespace
{
FAIHeroesBattleRecord SampleReplay()
{
    const TArray<FAIHeroesArmyStack> Friendly = {
        FAIHeroesArmyStack(TEXT("Militia"), 8, 12, 4, 2, 3),
        FAIHeroesArmyStack(TEXT("Scout"), 3, 8, 6, 1, 7)
    };
    const TArray<FAIHeroesArmyStack> Neutral = {
        FAIHeroesArmyStack(TEXT("Wolf"), 4, 9, 3, 1, 8)
    };
    const FAIHeroesBattleOutcome Outcome =
        FAIHeroesBattleEngine::Resolve(Friendly, Neutral, 1);
    return FAIHeroesBattleRecordRules::Capture(
        FName(TEXT("FOREST")), 2, 1, Friendly, Neutral, Outcome);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesReplayIntegrityTest,
    "AIHeroes.Replay.DeterministicVerification",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesReplayIntegrityTest::RunTest(const FString& Parameters)
{
    FAIHeroesBattleRecord Replay = SampleReplay();
    FString Error;
    TestTrue(TEXT("Recorded combat can be reproduced exactly"),
        FAIHeroesBattleRecordRules::Verify(Replay, Error));
    TestTrue(TEXT("Replay contains player action steps"), Replay.Trace.Num() > 0);
    TestTrue(TEXT("Readable replay includes node and step number"),
        FAIHeroesBattleRecordRules::DescribeStep(Replay, 0).Contains(TEXT("FOREST 1/")));
    TestTrue(TEXT("Self-equivalent record"), FAIHeroesBattleRecordRules::Equivalent(Replay, Replay));

    FAIHeroesBattleRecord Modified = Replay;
    Modified.Trace[0] += TEXT(" FORGED");
    TestFalse(TEXT("Mutated trace rejected"),
        FAIHeroesBattleRecordRules::Verify(Modified, Error));
    TestFalse(TEXT("Mutated replay is not equivalent"),
        FAIHeroesBattleRecordRules::Equivalent(Replay, Modified));

    Modified = Replay;
    Modified.PlayerAfter[0].TotalHealth += 1;
    TestFalse(TEXT("Mutated survivor HP rejected"),
        FAIHeroesBattleRecordRules::Verify(Modified, Error));

    FAIHeroesBattleRecord EmptyRecord;
    TestTrue(TEXT("Legacy save without replay remains valid"),
        FAIHeroesBattleRecordRules::Verify(EmptyRecord, Error));
    EmptyRecord.Trace.Add(TEXT("Anonymous attack"));
    TestFalse(TEXT("Replay without source identity rejected"),
        FAIHeroesBattleRecordRules::Verify(EmptyRecord, Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesReplayDiskRoundTripTest,
    "AIHeroes.Replay.DiskRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesReplayDiskRoundTripTest::RunTest(const FString& Parameters)
{
    UAIHeroesCampaignSave* Original = NewObject<UAIHeroesCampaignSave>(GetTransientPackage());
    Original->LastBattleRecord = SampleReplay();
    const FString TestSlot = FString::Printf(TEXT("AIHEROES_REPLAY_TEST_%s"),
        *FGuid::NewGuid().ToString(EGuidFormats::Digits));

    const bool bWritten = UGameplayStatics::SaveGameToSlot(Original, TestSlot, 0);
    TestTrue(TEXT("Replay record serialized by Unreal SaveGame"), bWritten);
    UAIHeroesCampaignSave* Loaded = bWritten
        ? Cast<UAIHeroesCampaignSave>(UGameplayStatics::LoadGameFromSlot(TestSlot, 0))
        : nullptr;
    TestTrue(TEXT("Replay record loaded"), IsValid(Loaded));
    if (Loaded)
    {
        FString Error;
        TestTrue(TEXT("Reloaded replay still validates"),
            FAIHeroesBattleRecordRules::Verify(Loaded->LastBattleRecord, Error));
        TestTrue(TEXT("Replay bytes round-trip semantically"),
            FAIHeroesBattleRecordRules::Equivalent(
                Original->LastBattleRecord, Loaded->LastBattleRecord));
    }

    if (bWritten)
    {
        TestTrue(TEXT("Only own unique test slot deleted"),
            UGameplayStatics::DeleteGameInSlot(TestSlot, 0));
    }
    return true;
}
#endif
