#include "AIHeroesCampaignSave.h"
#include "AIHeroesSaveSystem.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/Guid.h"

namespace
{
UAIHeroesCampaignSave* MakeTestCampaign(int32 Day, int32 Gold)
{
    UAIHeroesCampaignSave* Snapshot =
        NewObject<UAIHeroesCampaignSave>(GetTransientPackage());
    Snapshot->Day = Day;
    Snapshot->Gold = Gold;
    Snapshot->HeroNodeId = FName(TEXT("HQ"));
    Snapshot->PlayerArmy.Add(FAIHeroesArmyStack(TEXT("Militia"), 8, 12, 4, 2, 3));

    FAIHeroesSavedNode HQ;
    HQ.NodeId = FName(TEXT("HQ"));
    HQ.Type = EAIHeroesNodeType::Headquarters;
    HQ.bOwned = true;
    Snapshot->NodeStates.Add(HQ);

    FAIHeroesSavedNode Forest;
    Forest.NodeId = FName(TEXT("FOREST"));
    Forest.Type = EAIHeroesNodeType::ForestMine;
    Forest.Defenders.Add(FAIHeroesArmyStack(TEXT("Wolf"), 4, 9, 3, 1, 8));
    Snapshot->NodeStates.Add(Forest);

    FAIHeroesSavedNode Ore;
    Ore.NodeId = FName(TEXT("ORE"));
    Ore.Type = EAIHeroesNodeType::OreMine;
    Ore.Defenders.Add(FAIHeroesArmyStack(TEXT("Bandit"), 6, 10, 4, 2, 4));
    Snapshot->NodeStates.Add(Ore);

    FAIHeroesSavedNode Outpost;
    Outpost.NodeId = FName(TEXT("OUTPOST"));
    Outpost.Type = EAIHeroesNodeType::Outpost;
    Outpost.Defenders.Add(FAIHeroesArmyStack(TEXT("Raider"), 10, 11, 5, 2, 5));
    Snapshot->NodeStates.Add(Outpost);

    FAIHeroesSavedNode Ruins;
    Ruins.NodeId = FName(TEXT("RUINS"));
    Ruins.Type = EAIHeroesNodeType::Ruins;
    Ruins.Defenders.Add(FAIHeroesArmyStack(TEXT("Guardian"), 7, 24, 7, 4, 2));
    Snapshot->NodeStates.Add(Ruins);
    return Snapshot;
}

struct FTemporaryPlayerSlots
{
    const FString Prefix = FString::Printf(TEXT("AIHEROES_MULTISLOT_QA_%s"),
        *FGuid::NewGuid().ToString(EGuidFormats::Digits));

    FString Name(int32 Slot) const
    {
        return FString::Printf(TEXT("%s_Player%d"), *Prefix, Slot);
    }

    ~FTemporaryPlayerSlots()
    {
        // Only our own GUID names; never delete the campaign slots.
        for (int32 Slot = 1; Slot <= 3; ++Slot)
        {
            UGameplayStatics::DeleteGameInSlot(FAIHeroesSaveSystem::SlotA(Name(Slot)), 0);
            UGameplayStatics::DeleteGameInSlot(FAIHeroesSaveSystem::SlotB(Name(Slot)), 0);
        }
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesMultiSlotIsolationTest,
    "AIHeroes.Campaign.MultiSlotIsolation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesMultiSlotIsolationTest::RunTest(const FString& Parameters)
{
    FTemporaryPlayerSlots Sandbox;
    UAIHeroesCampaignSave* First = MakeTestCampaign(2, 160);
    UAIHeroesCampaignSave* Second = MakeTestCampaign(7, 310);
    UAIHeroesCampaignSave* Third = MakeTestCampaign(13, 620);
    FString Error;

    TestTrue(TEXT("First slot save"), FAIHeroesSaveSystem::Save(
        *First, Sandbox.Name(1), FString(), Error));
    TestTrue(TEXT("Second slot save"), FAIHeroesSaveSystem::Save(
        *Second, Sandbox.Name(2), FString(), Error));
    TestTrue(TEXT("Third slot save"), FAIHeroesSaveSystem::Save(
        *Third, Sandbox.Name(3), FString(), Error));

    for (int32 Slot = 1; Slot <= 3; ++Slot)
    {
        const FAIHeroesLoadedCampaign Loaded =
            FAIHeroesSaveSystem::Load(Sandbox.Name(Slot), FString());
        if (!TestTrue(TEXT("Separate campaign slot loads"), Loaded.Snapshot != nullptr))
        {
            continue;
        }
        TestEqual(TEXT("Each slot retains its own day"),
            Loaded.Snapshot->Day, Slot == 1 ? 2 : (Slot == 2 ? 7 : 13));
    }

    // Overwriting slot two must neither replace nor mutate slot one.
    Second->Day = 9;
    Second->Gold = 400;
    TestTrue(TEXT("Second save creates a rotating second-slot snapshot"),
        FAIHeroesSaveSystem::Save(*Second, Sandbox.Name(2), FString(), Error));
    const FAIHeroesLoadedCampaign After1 =
        FAIHeroesSaveSystem::Load(Sandbox.Name(1), FString());
    const FAIHeroesLoadedCampaign After2 =
        FAIHeroesSaveSystem::Load(Sandbox.Name(2), FString());
    TestTrue(TEXT("Slot one remains independent"), After1.Snapshot &&
        After1.Snapshot->Day == 2 && After1.Snapshot->Gold == 160);
    TestTrue(TEXT("Slot two obtains new progress"), After2.Snapshot &&
        After2.Snapshot->Day == 9 && After2.Snapshot->Gold == 400);
    TestTrue(TEXT("Slot two has an independent recovery copy"),
        UGameplayStatics::DoesSaveGameExist(
            FAIHeroesSaveSystem::SlotA(Sandbox.Name(2)), 0) &&
        UGameplayStatics::DoesSaveGameExist(
            FAIHeroesSaveSystem::SlotB(Sandbox.Name(2)), 0));
    return true;
}
#endif
