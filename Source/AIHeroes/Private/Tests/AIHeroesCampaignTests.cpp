#include "AIHeroesCampaignRules.h"
#include "AIHeroesCampaignSave.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/Guid.h"

namespace
{
UAIHeroesCampaignSave* MakeSampleCampaign()
{
    UAIHeroesCampaignSave* Save = NewObject<UAIHeroesCampaignSave>(GetTransientPackage());
    Save->Day = 7;
    Save->Gold = 510;
    Save->Wood = 57;
    Save->Ore = 23;
    Save->MovesLeft = 2;
    Save->ProtocolTier = 2;
    Save->ModelSlotTier = 1;
    Save->DataFragments = 8;
    Save->HeroNodeId = FName(TEXT("FOREST"));
    Save->SelectedNodeId = FName(TEXT("ORE"));
    Save->PlayerArmy = {
        FAIHeroesArmyStack(TEXT("Militia"), 5, 12, 4, 2, 3),
        FAIHeroesArmyStack(TEXT("Scout"), 2, 8, 6, 1, 7)
    };

    FAIHeroesSavedNode HQ;
    HQ.NodeId = FName(TEXT("HQ"));
    HQ.Type = EAIHeroesNodeType::Headquarters;
    HQ.bOwned = true;
    Save->NodeStates.Add(HQ);

    FAIHeroesSavedNode Forest;
    Forest.NodeId = FName(TEXT("FOREST"));
    Forest.Type = EAIHeroesNodeType::ForestMine;
    Forest.bOwned = true;
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

    Save->BattleJournal.Add(TEXT("Day 6 | FOREST | VICTORY"));
    Save->LastCombatStep = TEXT("R2 HERO Scout -> Wolf");
    return Save;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesCampaignValidationTest,
    "AIHeroes.Campaign.Validation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesCampaignValidationTest::RunTest(const FString& Parameters)
{
    UAIHeroesCampaignSave* Save = MakeSampleCampaign();
    FString Error;
    TestTrue(TEXT("Valid snapshot accepted"), FAIHeroesCampaignRules::Validate(*Save, Error));

    Save->Gold = -1;
    TestFalse(TEXT("Negative resources rejected"), FAIHeroesCampaignRules::Validate(*Save, Error));
    Save->Gold = 510;

    Save->SchemaVersion = 999;
    TestFalse(TEXT("Future versions rejected"), FAIHeroesCampaignRules::Validate(*Save, Error));
    Save->SchemaVersion = UAIHeroesCampaignSave::CurrentSchema;

    Save->HeroNodeId = FName(TEXT("RUINS"));
    TestFalse(TEXT("Hero cannot be at an enemy node"), FAIHeroesCampaignRules::Validate(*Save, Error));
    Save->HeroNodeId = FName(TEXT("FOREST"));

    Save->NodeStates[2].NodeId = FName(TEXT("FOREST"));
    TestFalse(TEXT("Duplicate node IDs rejected"), FAIHeroesCampaignRules::Validate(*Save, Error));
    Save->NodeStates[2].NodeId = FName(TEXT("ORE"));

    Save->PlayerArmy[0].Attack = 9999;
    TestFalse(TEXT("Forged unit stats rejected"), FAIHeroesCampaignRules::Validate(*Save, Error));
    Save->PlayerArmy[0].Attack = 4;

    Save->NodeStates[2].bOwned = true;
    TestFalse(TEXT("Captured node cannot have living guards"), FAIHeroesCampaignRules::Validate(*Save, Error));
    Save->NodeStates[2].bOwned = false;

    TestTrue(TEXT("Valid snapshot remains recoverable after rejected edits"),
        FAIHeroesCampaignRules::Validate(*Save, Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesCampaignDiskRoundTripTest,
    "AIHeroes.Campaign.DiskRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesCampaignDiskRoundTripTest::RunTest(const FString& Parameters)
{
    UAIHeroesCampaignSave* Before = MakeSampleCampaign();
    const FString Slot = FString::Printf(TEXT("AIHEROES_Automation_%s"),
        *FGuid::NewGuid().ToString(EGuidFormats::Digits));

    const bool bSaved = UGameplayStatics::SaveGameToSlot(Before, Slot, 0);
    TestTrue(TEXT("Snapshot writes to a temporary local save slot"), bSaved);

    UAIHeroesCampaignSave* After = bSaved
        ? Cast<UAIHeroesCampaignSave>(UGameplayStatics::LoadGameFromSlot(Slot, 0))
        : nullptr;

    const bool bRestored = IsValid(After);
    TestTrue(TEXT("Snapshot reloads with correct class"), bRestored);
    if (bRestored)
    {
        FString Error;
        TestTrue(TEXT("Restored snapshot passes all constraints"),
            FAIHeroesCampaignRules::Validate(*After, Error));
        TestEqual(TEXT("Day preserved"), After->Day, Before->Day);
        TestEqual(TEXT("Resource preserved"), After->Wood, Before->Wood);
        TestEqual(TEXT("Protocol preserved"), After->ProtocolTier, Before->ProtocolTier);
        TestEqual(TEXT("Model slot preserved"), After->ModelSlotTier, Before->ModelSlotTier);
        TestEqual(TEXT("Data fragments preserved"), After->DataFragments, Before->DataFragments);
        TestEqual(TEXT("Hero location node preserved"), After->HeroNodeId, Before->HeroNodeId);
        TestEqual(TEXT("Node count preserved"), After->NodeStates.Num(), Before->NodeStates.Num());
        TestEqual(TEXT("Army HP preserved"),
            After->PlayerArmy[0].TotalHealth, Before->PlayerArmy[0].TotalHealth);
    }

    // The test cleans up only its own unique slot. Never touch a player save.
    if (bSaved)
    {
        TestTrue(TEXT("Temporary test slot deleted"),
            UGameplayStatics::DeleteGameInSlot(Slot, 0));
    }
    return true;
}
#endif
