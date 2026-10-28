#include "AIHeroesBattleRecord.h"
#include "AIHeroesBattleEngine.h"
#include "AIHeroesCampaignSave.h"
#include "AIHeroesCampaignRules.h"
#include "AIHeroesGridBattle.h"

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

FAIHeroesBattleRecord SampleGridReplay()
{
    const TArray<FAIHeroesArmyStack> Friendly = {
        FAIHeroesArmyStack(TEXT("Militia"), 8, 12, 4, 2, 3),
        FAIHeroesArmyStack(TEXT("Scout"), 3, 8, 6, 1, 7)
    };
    const TArray<FAIHeroesArmyStack> Neutral = {
        FAIHeroesArmyStack(TEXT("Raider"), 6, 11, 5, 2, 5),
        FAIHeroesArmyStack(TEXT("Raider"), 4, 11, 5, 2, 5)
    };
    const FAIHeroesBattleOutcome Outcome =
        FAIHeroesGridBattle::RunToEnd(Friendly, Neutral, 3);
    return FAIHeroesBattleRecordRules::Capture(
        FName(TEXT("OUTPOST")), 2, 3, Friendly, Neutral, Outcome, 2);
}

UAIHeroesCampaignSave* SampleGridCampaign()
{
    UAIHeroesCampaignSave* Save = NewObject<UAIHeroesCampaignSave>(GetTransientPackage());
    Save->Day = 2;
    Save->HeroNodeId = FName(TEXT("HQ"));
    Save->SelectedNodeId = FName(TEXT("OUTPOST"));
    Save->LastBattleRecord = SampleGridReplay();
    Save->PlayerArmy = Save->LastBattleRecord.PlayerBefore;

    const TCHAR* Ids[] = {TEXT("HQ"), TEXT("FOREST"), TEXT("ORE"), TEXT("OUTPOST"), TEXT("RUINS")};
    const EAIHeroesNodeType Types[] = {
        EAIHeroesNodeType::Headquarters, EAIHeroesNodeType::ForestMine,
        EAIHeroesNodeType::OreMine, EAIHeroesNodeType::Outpost, EAIHeroesNodeType::Ruins
    };
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Ids); ++Index)
    {
        FAIHeroesSavedNode Node;
        Node.NodeId = FName(Ids[Index]);
        Node.Type = Types[Index];
        Node.bOwned = Index == 0;
        if (Node.Type == EAIHeroesNodeType::Outpost)
        {
            Node.Defenders = Save->LastBattleRecord.EnemyBefore;
        }
        Save->NodeStates.Add(Node);
    }
    return Save;
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
    TestEqual(TEXT("Default capture keeps the legacy rule version"), Replay.RuleVersion, 1);
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
    FAIHeroesReplayRuleVersionTest,
    "AIHeroes.Replay.VersionedRules",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesReplayRuleVersionTest::RunTest(const FString& Parameters)
{
    FString Error;
    const FAIHeroesBattleRecord Grid = SampleGridReplay();
    TestEqual(TEXT("Grid capture declares its rule version"), Grid.RuleVersion, 2);
    TestEqual(TEXT("Grid record retains both neutral formations"), Grid.EnemyBefore.Num(), 2);
    TestTrue(TEXT("Grid transcript is reproduced with grid rules"),
        FAIHeroesBattleRecordRules::Verify(Grid, Error));
    if (Grid.Trace.IsEmpty() || Grid.PlayerAfter.IsEmpty())
    {
        AddError(TEXT("Grid fixture did not produce a transcript and resulting friendly army."));
        return false;
    }

    FAIHeroesBattleRecord Modified = Grid;
    Modified.RuleVersion = 1;
    TestFalse(TEXT("A grid transcript cannot be relabelled as legacy"),
        FAIHeroesBattleRecordRules::Verify(Modified, Error));
    TestFalse(TEXT("Rule version participates in semantic read-back"),
        FAIHeroesBattleRecordRules::Equivalent(Grid, Modified));

    Modified = Grid;
    Modified.RuleVersion = 999;
    TestFalse(TEXT("Unknown combat rules are rejected"),
        FAIHeroesBattleRecordRules::Verify(Modified, Error));
    FAIHeroesBattleRecord Empty;
    TestEqual(TEXT("Legacy records missing new metadata default to V1"), Empty.RuleVersion, 1);
    Empty.RuleVersion = 999;
    TestFalse(TEXT("Unknown rules cannot bypass validation through an empty record"),
        FAIHeroesBattleRecordRules::Verify(Empty, Error));
    Empty.RuleVersion = 2;
    TestFalse(TEXT("An anonymous grid record is not a legacy empty record"),
        FAIHeroesBattleRecordRules::Verify(Empty, Error));

    Modified = Grid;
    Modified.Trace[0] += TEXT(" FORGED");
    TestFalse(TEXT("Forged grid action transcript rejected"),
        FAIHeroesBattleRecordRules::Verify(Modified, Error));
    Modified = Grid;
    Modified.Summary += TEXT(" FORGED");
    TestFalse(TEXT("Forged grid summary rejected"),
        FAIHeroesBattleRecordRules::Verify(Modified, Error));
    Modified = Grid;
    Modified.bVictory = !Modified.bVictory;
    TestFalse(TEXT("Forged grid terminal flag rejected"),
        FAIHeroesBattleRecordRules::Verify(Modified, Error));
    Modified = Grid;
    Modified.PlayerAfter[0].TotalHealth += 1;
    TestFalse(TEXT("Forged grid survivor HP rejected"),
        FAIHeroesBattleRecordRules::Verify(Modified, Error));

    Modified = Grid;
    Modified.EnemyBefore.Add(Grid.EnemyBefore[0]);
    TestFalse(TEXT("Grid record cannot exceed two neutral formations"),
        FAIHeroesBattleRecordRules::Verify(Modified, Error));
    Modified = Grid;
    Modified.Trace.Init(TEXT("extra action"), 257);
    TestFalse(TEXT("Grid record cannot exceed 256 trace events"),
        FAIHeroesBattleRecordRules::Verify(Modified, Error));
    Modified = SampleReplay();
    Modified.Trace.Init(TEXT("extra attack"), 129);
    TestFalse(TEXT("Legacy record retains its 128-event limit"),
        FAIHeroesBattleRecordRules::Verify(Modified, Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesGridGarrisonCompatibilityTest,
    "AIHeroes.Campaign.GridGarrisonCompatibility",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesGridGarrisonCompatibilityTest::RunTest(const FString& Parameters)
{
    UAIHeroesCampaignSave* Save = SampleGridCampaign();
    FString Error;
    TestTrue(TEXT("Two legal formations and a V2 record validate"),
        FAIHeroesCampaignRules::Validate(*Save, Error));
    FAIHeroesSavedNode& Outpost = Save->NodeStates[3];
    Outpost.Defenders = {FAIHeroesArmyStack(TEXT("Raider"), 10, 11, 5, 2, 5)};
    TestTrue(TEXT("A legacy unsplit garrison remains valid and is not rewritten"),
        FAIHeroesCampaignRules::Validate(*Save, Error));
    TestEqual(TEXT("Validation preserves the unsplit formation"), Outpost.Defenders.Num(), 1);
    TestEqual(TEXT("Validation preserves the old formation health"), Outpost.Defenders[0].TotalHealth, 110);
    Outpost.Defenders = Save->LastBattleRecord.EnemyBefore;
    // Unreal TArray::Add forbids references into its own storage: copy first.
    const FAIHeroesArmyStack ThirdDefender = Outpost.Defenders[0];
    Outpost.Defenders.Add(ThirdDefender);
    TestFalse(TEXT("Third neutral formation rejected"),
        FAIHeroesCampaignRules::Validate(*Save, Error));
    Outpost.Defenders = Save->LastBattleRecord.EnemyBefore;
    Outpost.Defenders[1] = FAIHeroesArmyStack(TEXT("Wolf"), 4, 9, 3, 1, 8);
    TestFalse(TEXT("Second formation must still match the node guard type"),
        FAIHeroesCampaignRules::Validate(*Save, Error));
    Outpost.Defenders = Save->LastBattleRecord.EnemyBefore;
    Save->PlayerArmy[1] = Save->PlayerArmy[0];
    TestFalse(TEXT("Duplicate friendly formations remain forbidden"),
        FAIHeroesCampaignRules::Validate(*Save, Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesReplayDiskRoundTripTest,
    "AIHeroes.Replay.DiskRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesReplayDiskRoundTripTest::RunTest(const FString& Parameters)
{
    for (int32 RuleVersion = 1; RuleVersion <= 2; ++RuleVersion)
    {
        UAIHeroesCampaignSave* Original = NewObject<UAIHeroesCampaignSave>(GetTransientPackage());
        Original->LastBattleRecord = RuleVersion == 1 ? SampleReplay() : SampleGridReplay();
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
            TestEqual(TEXT("Rule version survives SaveGame serialization"),
                Loaded->LastBattleRecord.RuleVersion, RuleVersion);
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
    }
    return true;
}
#endif
