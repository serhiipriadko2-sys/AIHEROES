#include "AIHeroesSaveSystem.h"
#include "AIHeroesCampaignRules.h"
#include "AIHeroesCampaignSave.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/Guid.h"

namespace
{
struct FTemporaryCampaignSlots
{
    const FString Prefix = FString::Printf(TEXT("AIHEROES_TEST_%s"),
        *FGuid::NewGuid().ToString(EGuidFormats::Digits));
    const FString Legacy = Prefix + TEXT("_Legacy");

    ~FTemporaryCampaignSlots()
    {
        // Never touch the player's slots, only this test's unique names.
        UGameplayStatics::DeleteGameInSlot(FAIHeroesSaveSystem::SlotA(Prefix), 0);
        UGameplayStatics::DeleteGameInSlot(FAIHeroesSaveSystem::SlotB(Prefix), 0);
        UGameplayStatics::DeleteGameInSlot(Legacy, 0);
    }
};

UAIHeroesCampaignSave* MakeRotatingSample()
{
    UAIHeroesCampaignSave* Save = NewObject<UAIHeroesCampaignSave>(GetTransientPackage());
    Save->Day = 2;
    Save->Gold = 290;
    Save->Wood = 35;
    Save->Ore = 30;
    Save->HeroNodeId = FName(TEXT("HQ"));
    Save->PlayerArmy.Add(FAIHeroesArmyStack(TEXT("Militia"), 8, 12, 4, 2, 3));

    FAIHeroesSavedNode HQ;
    HQ.NodeId = FName(TEXT("HQ"));
    HQ.Type = EAIHeroesNodeType::Headquarters;
    HQ.bOwned = true;
    Save->NodeStates.Add(HQ);

    FAIHeroesSavedNode Forest;
    Forest.NodeId = FName(TEXT("FOREST"));
    Forest.Type = EAIHeroesNodeType::ForestMine;
    Forest.Defenders.Add(FAIHeroesArmyStack(TEXT("Wolf"), 4, 9, 3, 1, 8));
    Save->NodeStates.Add(Forest);

    FAIHeroesSavedNode Ore;
    Ore.NodeId = FName(TEXT("ORE"));
    Ore.Type = EAIHeroesNodeType::OreMine;
    Ore.Defenders.Add(FAIHeroesArmyStack(TEXT("Bandit"), 6, 10, 4, 2, 4));
    Save->NodeStates.Add(Ore);

    FAIHeroesSavedNode Outpost;
    Outpost.NodeId = FName(TEXT("OUTPOST"));
    Outpost.Type = EAIHeroesNodeType::Outpost;
    Outpost.Defenders.Add(FAIHeroesArmyStack(TEXT("Raider"), 10, 11, 5, 2, 5));
    Save->NodeStates.Add(Outpost);

    FAIHeroesSavedNode Ruins;
    Ruins.NodeId = FName(TEXT("RUINS"));
    Ruins.Type = EAIHeroesNodeType::Ruins;
    Ruins.Defenders.Add(FAIHeroesArmyStack(TEXT("Guardian"), 7, 24, 7, 4, 2));
    Save->NodeStates.Add(Ruins);
    return Save;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesRotatingSaveRecoveryTest,
    "AIHeroes.Campaign.RotatingSaveRecovery",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesRotatingSaveRecoveryTest::RunTest(const FString& Parameters)
{
    FTemporaryCampaignSlots Slots;
    UAIHeroesCampaignSave* Save = MakeRotatingSample();
    FString Result;
    FString Error;
    TestTrue(TEXT("Initial snapshot valid"), FAIHeroesCampaignRules::Validate(*Save, Error));

    const bool bFirstSave = FAIHeroesSaveSystem::Save(
        *Save, Slots.Prefix, Slots.Legacy, Result);
    TestTrue(TEXT("First save written to A and verified"), bFirstSave);
    if (!bFirstSave) return false;

    const FAIHeroesLoadedCampaign First = FAIHeroesSaveSystem::Load(
        Slots.Prefix, Slots.Legacy);
    TestTrue(TEXT("First load works"), First.Snapshot != nullptr);
    if (!First.Snapshot) return false;
    TestEqual(TEXT("First save sequence is one"), First.Snapshot->SaveSequence, 1);
    TestEqual(TEXT("First slot is A"), First.SlotName, FAIHeroesSaveSystem::SlotA(Slots.Prefix));
    TestFalse(TEXT("Initial slot is not a recovery"), First.bRecovered);

    Save->Day = 5;
    Save->Gold = 410;
    Save->BattleJournal.Add(TEXT("Day 4 | FOREST | VICTORY"));
    const bool bSecondSave = FAIHeroesSaveSystem::Save(
        *Save, Slots.Prefix, Slots.Legacy, Result);
    TestTrue(TEXT("Second save written to B and verified"), bSecondSave);
    if (!bSecondSave) return false;

    const FAIHeroesLoadedCampaign Second = FAIHeroesSaveSystem::Load(
        Slots.Prefix, Slots.Legacy);
    TestTrue(TEXT("Newer B snapshot selected"), Second.Snapshot != nullptr &&
        Second.SlotName == FAIHeroesSaveSystem::SlotB(Slots.Prefix));
    if (!Second.Snapshot) return false;
    TestEqual(TEXT("New day loaded"), Second.Snapshot->Day, 5);
    TestEqual(TEXT("New sequence loaded"), Second.Snapshot->SaveSequence, 2);
    TestTrue(TEXT("Backup A still exists"), UGameplayStatics::DoesSaveGameExist(
        FAIHeroesSaveSystem::SlotA(Slots.Prefix), 0));

    // Simulate a logically corrupted newest file without manipulating the
    // player's real saves or relying on filesystem-specific disk operations.
    UAIHeroesCampaignSave* Damaged = Cast<UAIHeroesCampaignSave>(
        UGameplayStatics::LoadGameFromSlot(FAIHeroesSaveSystem::SlotB(Slots.Prefix), 0));
    if (!TestTrue(TEXT("Read B for corruption simulation"), IsValid(Damaged)))
        return false;
    Damaged->Gold = -900;
    TestTrue(TEXT("Write intentionally-invalid test-only B"),
        UGameplayStatics::SaveGameToSlot(Damaged, FAIHeroesSaveSystem::SlotB(Slots.Prefix), 0));

    const FAIHeroesLoadedCampaign Recovery = FAIHeroesSaveSystem::Load(
        Slots.Prefix, Slots.Legacy);
    TestTrue(TEXT("Falls back to valid A"), Recovery.Snapshot != nullptr &&
        Recovery.SlotName == FAIHeroesSaveSystem::SlotA(Slots.Prefix));
    TestTrue(TEXT("Recovery is explicitly labelled"), Recovery.bRecovered);
    if (Recovery.Snapshot)
    {
        TestEqual(TEXT("Recovered preceding day"), Recovery.Snapshot->Day, 2);
        TestEqual(TEXT("Recovered previous gold"), Recovery.Snapshot->Gold, 290);
    }

    Save->Day = 6;
    Save->Gold = 500;
    const bool bRepaired = FAIHeroesSaveSystem::Save(
        *Save, Slots.Prefix, Slots.Legacy, Result);
    TestTrue(TEXT("Next save overwrites damaged B, preserving valid A"), bRepaired);
    const FAIHeroesLoadedCampaign Repaired = FAIHeroesSaveSystem::Load(
        Slots.Prefix, Slots.Legacy);
    TestTrue(TEXT("Latest repaired campaign valid"), Repaired.Snapshot != nullptr);
    if (Repaired.Snapshot)
    {
        TestEqual(TEXT("Next generation after recovery"), Repaired.Snapshot->SaveSequence, 2);
        TestEqual(TEXT("Game progress after repair"), Repaired.Snapshot->Day, 6);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesLegacySaveMigrationTest,
    "AIHeroes.Campaign.LegacySaveMigration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesLegacySaveMigrationTest::RunTest(const FString& Parameters)
{
    FTemporaryCampaignSlots Slots;
    UAIHeroesCampaignSave* Legacy = MakeRotatingSample();
    Legacy->Day = 9;
    Legacy->SaveSequence = 0; // Old .sav did not serialize this optional property.
    TestTrue(TEXT("Legacy snapshot written to a unique temporary slot"),
        UGameplayStatics::SaveGameToSlot(Legacy, Slots.Legacy, 0));

    const FAIHeroesLoadedCampaign Before = FAIHeroesSaveSystem::Load(
        Slots.Prefix, Slots.Legacy);
    TestTrue(TEXT("Legacy format is accepted"), Before.Snapshot != nullptr &&
        Before.bLegacy);
    if (!Before.Snapshot) return false;
    TestEqual(TEXT("Old save sequence defaults to zero"), Before.Snapshot->SaveSequence, 0);
    TestEqual(TEXT("Old progress is intact"), Before.Snapshot->Day, 9);

    FString Message;
    Legacy->Day = 10;
    TestTrue(TEXT("Migration creates A without writing legacy"),
        FAIHeroesSaveSystem::Save(*Legacy, Slots.Prefix, Slots.Legacy, Message));
    const FAIHeroesLoadedCampaign New = FAIHeroesSaveSystem::Load(
        Slots.Prefix, Slots.Legacy);
    TestTrue(TEXT("Newer A is preferred"), New.Snapshot != nullptr && !New.bLegacy);
    if (New.Snapshot)
    {
        TestEqual(TEXT("New progress preserved"), New.Snapshot->Day, 10);
        TestEqual(TEXT("First new generation is one"), New.Snapshot->SaveSequence, 1);
    }

    UAIHeroesCampaignSave* OldAgain = Cast<UAIHeroesCampaignSave>(
        UGameplayStatics::LoadGameFromSlot(Slots.Legacy, 0));
    TestTrue(TEXT("Legacy file remains readable"), IsValid(OldAgain));
    if (OldAgain)
    {
        TestEqual(TEXT("Legacy snapshot was never overwritten"), OldAgain->Day, 9);
        TestEqual(TEXT("Legacy sequence stays zero"), OldAgain->SaveSequence, 0);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesRejectSaveWithoutMutationTest,
    "AIHeroes.Campaign.RejectedSaveKeepsBackup",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesRejectSaveWithoutMutationTest::RunTest(const FString& Parameters)
{
    FTemporaryCampaignSlots Slots;
    UAIHeroesCampaignSave* Save = MakeRotatingSample();
    FString Message;
    TestTrue(TEXT("Baseline A saved"),
        FAIHeroesSaveSystem::Save(*Save, Slots.Prefix, Slots.Legacy, Message));

    Save->Gold = -1;
    TestFalse(TEXT("Invalid replacement rejected before disk writes"),
        FAIHeroesSaveSystem::Save(*Save, Slots.Prefix, Slots.Legacy, Message));
    TestFalse(TEXT("No B file created on rejected save"),
        UGameplayStatics::DoesSaveGameExist(FAIHeroesSaveSystem::SlotB(Slots.Prefix), 0));
    const FAIHeroesLoadedCampaign Stable = FAIHeroesSaveSystem::Load(
        Slots.Prefix, Slots.Legacy);
    TestTrue(TEXT("Earlier A remains loadable"), Stable.Snapshot != nullptr);
    if (Stable.Snapshot)
    {
        TestEqual(TEXT("Earlier resources were not modified"), Stable.Snapshot->Gold, 290);
    }

    // A forged 0 sequence is not accepted in resilient slots.
    UAIHeroesCampaignSave* BadSlot = MakeRotatingSample();
    BadSlot->SaveSequence = 0;
    TestTrue(TEXT("Write a test-only zero-generation B"),
        UGameplayStatics::SaveGameToSlot(BadSlot, FAIHeroesSaveSystem::SlotB(Slots.Prefix), 0));
    const FAIHeroesLoadedCampaign Ignored = FAIHeroesSaveSystem::Load(
        Slots.Prefix, Slots.Legacy);
    TestTrue(TEXT("Wrong generation B ignored"), Ignored.Snapshot != nullptr &&
        Ignored.SlotName == FAIHeroesSaveSystem::SlotA(Slots.Prefix));
    TestTrue(TEXT("Corrupted B is reported"), Ignored.bRecovered);
    return true;
}
#endif
