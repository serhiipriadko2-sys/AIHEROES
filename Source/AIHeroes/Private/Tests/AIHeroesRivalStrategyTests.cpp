#include "AIHeroesRivalStrategy.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AIHeroesCampaignRules.h"
#include "AIHeroesCampaignSave.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/Guid.h"

namespace
{
FAIHeroesRivalSnapshot MakeRival(int32 Day)
{
    FAIHeroesRivalSnapshot Snapshot;
    Snapshot.Day = Day;
    Snapshot.Gold = 280;
    Snapshot.Wood = 65;
    Snapshot.Ore = 33;
    Snapshot.bOutpostPresent = true;
    Snapshot.OutpostDefenders.Add(FAIHeroesArmyStack(
        FName(TEXT("Raider")), 10, 11, 5, 2, 5));
    return Snapshot;
}

UAIHeroesCampaignSave* MakeSaveWithOutpost(
    const FAIHeroesRivalSnapshot& Rival)
{
    UAIHeroesCampaignSave* Save =
        NewObject<UAIHeroesCampaignSave>(GetTransientPackage());
    Save->Day = Rival.Day;
    Save->Gold = Rival.Gold;
    Save->Wood = Rival.Wood;
    Save->Ore = Rival.Ore;
    Save->HeroNodeId = FName(TEXT("HQ"));
    Save->PlayerArmy.Add(FAIHeroesArmyStack(
        FName(TEXT("Militia")), 8, 12, 4, 2, 3));

    FAIHeroesSavedNode Headquarters;
    Headquarters.NodeId = FName(TEXT("HQ"));
    Headquarters.Type = EAIHeroesNodeType::Headquarters;
    Headquarters.bOwned = true;
    Save->NodeStates.Add(Headquarters);

    FAIHeroesSavedNode Forest;
    Forest.NodeId = FName(TEXT("FOREST"));
    Forest.Type = EAIHeroesNodeType::ForestMine;
    Forest.bOwned = Rival.bForestOwned;
    if (!Forest.bOwned)
        Forest.Defenders.Add(FAIHeroesArmyStack(TEXT("Wolf"), 4, 9, 3, 1, 8));
    Save->NodeStates.Add(Forest);

    FAIHeroesSavedNode Ore;
    Ore.NodeId = FName(TEXT("ORE"));
    Ore.Type = EAIHeroesNodeType::OreMine;
    Ore.bOwned = Rival.bOreOwned;
    if (!Ore.bOwned)
        Ore.Defenders.Add(FAIHeroesArmyStack(TEXT("Bandit"), 6, 10, 4, 2, 4));
    Save->NodeStates.Add(Ore);

    FAIHeroesSavedNode Outpost;
    Outpost.NodeId = FName(TEXT("OUTPOST"));
    Outpost.Type = EAIHeroesNodeType::Outpost;
    Outpost.bOwned = Rival.bOutpostOwned;
    Outpost.Defenders = Rival.OutpostDefenders;
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
    FAIHeroesRivalRaidTest,
    "AIHeroes.Rival.DeterministicRaidsAndAuthority",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesRivalRaidTest::RunTest(const FString& Parameters)
{
    FAIHeroesRivalSnapshot Before = MakeRival(3);
    Before.bForestOwned = true;
    Before.bOreOwned = true;
    const int32 OriginalGold = Before.Gold;
    const int32 OriginalOre = Before.Ore;
    FAIHeroesRivalSnapshot After;
    FAIHeroesRivalTurn Turn;
    FString Error;
    TestTrue(TEXT("Day three chooses a bounded raid"),
        FAIHeroesRivalStrategy::Resolve(Before, After, Turn, Error));
    TestEqual(TEXT("First raid targets ore"), Turn.Action, EAIHeroesRivalAction::RaidOre);
    TestEqual(TEXT("Ore loses four, and no more"), After.Ore, OriginalOre - 4);
    TestEqual(TEXT("Rival never steals gold"), After.Gold, OriginalGold);
    TestEqual(TEXT("Planning never mutates the input"), Before.Ore, OriginalOre);
    TestEqual(TEXT("Defender count unchanged by a raid"),
        After.OutpostDefenders[0].TotalHealth, Before.OutpostDefenders[0].TotalHealth);

    Before.Day = 6;
    TestTrue(TEXT("Second raid resolves"), FAIHeroesRivalStrategy::Resolve(Before, After, Turn, Error));
    TestEqual(TEXT("Second raid alternates to forest"), Turn.Action,
        EAIHeroesRivalAction::RaidForest);
    TestEqual(TEXT("Forest raid removes six wood"), After.Wood, Before.Wood - 6);

    Before.Wood = 2;
    TestTrue(TEXT("Resource loss is capped at available stock"),
        FAIHeroesRivalStrategy::Resolve(Before, After, Turn, Error));
    TestEqual(TEXT("Never create negative wood"), After.Wood, 0);
    TestEqual(TEXT("Limited raid takes only two"), Turn.Amount, 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesRivalRecruitTest,
    "AIHeroes.Rival.WeeklyRecruitmentAndCaptureStop",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesRivalRecruitTest::RunTest(const FString& Parameters)
{
    FAIHeroesRivalSnapshot Before = MakeRival(8);
    FAIHeroesRivalSnapshot After;
    FAIHeroesRivalTurn Turn;
    FString Error;
    TestTrue(TEXT("Week two recruits"), FAIHeroesRivalStrategy::Resolve(Before, After, Turn, Error));
    TestEqual(TEXT("Week two recruitment action"),
        Turn.Action, EAIHeroesRivalAction::RecruitAtOutpost);
    TestEqual(TEXT("Two healthy defenders added"),
        After.OutpostDefenders[0].Count(), 12);
    TestEqual(TEXT("NPC cannot change player's resource count"), After.Gold, Before.Gold);
    TestEqual(TEXT("Recruiting does not alter original stack"),
        Before.OutpostDefenders[0].Count(), 10);

    Before.Day = 15;
    Before.OutpostDefenders[0].TotalHealth = 16 * 11;
    TestTrue(TEXT("Full garrison is valid"), FAIHeroesRivalStrategy::Resolve(Before, After, Turn, Error));
    TestFalse(TEXT("Full garrison does not multiply indefinitely"), Turn.IsActive());

    Before.OutpostDefenders.Empty();
    TestTrue(TEXT("Empty hostile garrison can recover"),
        FAIHeroesRivalStrategy::Resolve(Before, After, Turn, Error));
    TestEqual(TEXT("Recover a single typed stack"), After.OutpostDefenders.Num(), 1);
    TestEqual(TEXT("Recruited stack uses the canonical unit"),
        After.OutpostDefenders[0].UnitId, FName(TEXT("Raider")));

    Before.bOutpostOwned = true;
    Before.OutpostDefenders.Empty();
    Before.bForestOwned = true;
    Before.bOreOwned = true;
    Before.Day = 18;
    TestTrue(TEXT("Captured enemy HQ does not raid"),
        FAIHeroesRivalStrategy::Resolve(Before, After, Turn, Error));
    TestFalse(TEXT("Player victory neutralizes the raider economy"), Turn.IsActive());
    TestEqual(TEXT("No resource theft after capture"), After.Wood, Before.Wood);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesRivalFailClosedTest,
    "AIHeroes.Rival.InvalidInputsAndFailClosed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesRivalFailClosedTest::RunTest(const FString& Parameters)
{
    FAIHeroesRivalSnapshot Before = MakeRival(3);
    Before.Ore = -10;
    FAIHeroesRivalSnapshot After;
    FAIHeroesRivalTurn Turn;
    FString Error;
    TestFalse(TEXT("Negative inputs block the entire NPC turn"),
        FAIHeroesRivalStrategy::Resolve(Before, After, Turn, Error));
    TestEqual(TEXT("A rejected calculation cannot change other resources"),
        After.Gold, Before.Gold);
    TestEqual(TEXT("No action escapes a rejected calculation"),
        Turn.Action, EAIHeroesRivalAction::None);
    TestTrue(TEXT("Fail-closed reason is explicit"), !Error.IsEmpty());

    Before = MakeRival(8);
    Before.OutpostDefenders[0].Attack = 900;
    TestFalse(TEXT("Forged NPC unit stats are refused"),
        FAIHeroesRivalStrategy::Resolve(Before, After, Turn, Error));

    Before = MakeRival(3);
    Before.bOutpostPresent = false;
    TestFalse(TEXT("Missing enemy region blocks a world-step"),
        FAIHeroesRivalStrategy::Resolve(Before, After, Turn, Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesRivalSaveCompatibilityTest,
    "AIHeroes.Rival.ExistingSaveGameRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesRivalSaveCompatibilityTest::RunTest(const FString& Parameters)
{
    const FAIHeroesRivalSnapshot Before = MakeRival(8);
    FAIHeroesRivalSnapshot After;
    FAIHeroesRivalTurn Turn;
    FString Error;
    if (!TestTrue(TEXT("Obtain deterministic next-day result"),
        FAIHeroesRivalStrategy::Resolve(Before, After, Turn, Error))) return false;

    UAIHeroesCampaignSave* Snapshot = MakeSaveWithOutpost(After);
    if (!TestTrue(TEXT("NPC changes fit the existing schema v1"),
        FAIHeroesCampaignRules::Validate(*Snapshot, Error))) return false;

    const FString Slot = FString::Printf(TEXT("AIHEROES_RIVAL_TEST_%s"),
        *FGuid::NewGuid().ToString(EGuidFormats::Digits));
    const bool bSaved = UGameplayStatics::SaveGameToSlot(Snapshot, Slot, 0);
    TestTrue(TEXT("Write a uniquely named temporary test save"), bSaved);
    UAIHeroesCampaignSave* Loaded = bSaved
        ? Cast<UAIHeroesCampaignSave>(UGameplayStatics::LoadGameFromSlot(Slot, 0))
        : nullptr;
    if (TestTrue(TEXT("Temporary SaveGame is readable"), IsValid(Loaded)))
    {
        TestTrue(TEXT("NPC garrison survives strict save validation"),
            FAIHeroesCampaignRules::Validate(*Loaded, Error));
        TestEqual(TEXT("NPC count persists across reload"),
            Loaded->NodeStates[3].Defenders[0].Count(), 12);
        TestEqual(TEXT("World day persists"), Loaded->Day, 8);
    }
    if (bSaved)
        TestTrue(TEXT("Only the GUID test save is removed"),
            UGameplayStatics::DeleteGameInSlot(Slot, 0));
    return true;
}
#endif
