#include "AIHeroesProceduralCampaignSave.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AIHeroesCampaignRules.h"
#include "AIHeroesCampaignSave.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/Guid.h"

namespace
{
UAIHeroesProceduralCampaignSave* CreateFixture(uint32 Seed, int32 Branches)
{
    UAIHeroesProceduralCampaignSave* S =
        NewObject<UAIHeroesProceduralCampaignSave>(GetTransientPackage());
    FString Error;
    if (!FAIHeroesProceduralCampaignRules::CreateNew(
            Seed, Branches, *S, Error))
        return nullptr;
    return S;
}

FAIHeroesProceduralSavedZone* Find(
    UAIHeroesProceduralCampaignSave& S, const TCHAR* Id)
{
    return S.ZoneStates.FindByPredicate(
        [Id](const FAIHeroesProceduralSavedZone& Z)
        { return Z.ZoneId == FName(Id); });
}

UAIHeroesCampaignSave* LegacyFixture()
{
    UAIHeroesCampaignSave* Legacy =
        NewObject<UAIHeroesCampaignSave>(GetTransientPackage());
    Legacy->HeroNodeId = TEXT("HQ");
    Legacy->PlayerArmy.Add(FAIHeroesArmyStack(
        TEXT("Militia"), 8, 12, 4, 2, 3));
    const TCHAR* Ids[] = {
        TEXT("HQ"), TEXT("FOREST"), TEXT("ORE"),
        TEXT("OUTPOST"), TEXT("RUINS")
    };
    const EAIHeroesNodeType Types[] = {
        EAIHeroesNodeType::Headquarters,
        EAIHeroesNodeType::ForestMine,
        EAIHeroesNodeType::OreMine,
        EAIHeroesNodeType::Outpost,
        EAIHeroesNodeType::Ruins
    };
    for (int32 I = 0; I < 5; ++I)
    {
        FAIHeroesSavedNode Zone;
        Zone.NodeId = FName(Ids[I]);
        Zone.Type = Types[I];
        Zone.bOwned = I == 0;
        Legacy->NodeStates.Add(Zone);
    }
    return Legacy;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesProceduralV2InitialFogTest,
    "AIHeroes.ProceduralV2.InitialStateAndFog",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesProceduralV2InitialFogTest::RunTest(const FString&)
{
    UAIHeroesProceduralCampaignSave* S = CreateFixture(20261028u, 3);
    if (!TestTrue(TEXT("Separate procedural v2 class initializes"), IsValid(S)))
        return false;
    FString Error;
    TestTrue(TEXT("Initial v2 metadata fully validates"),
        FAIHeroesProceduralCampaignRules::Validate(*S, Error));
    TestEqual(TEXT("New schema is version two"), S->SchemaVersion, 2);
    TestEqual(TEXT("10 fixed-ID zones in the seeded test fixture"),
        S->ZoneStates.Num(), 10);
    TestTrue(TEXT("HQ remains owned and scouted"),
        S->ZoneStates[0].bOwned && S->ZoneStates[0].bDiscovered);
    TestFalse(TEXT("Late guarded checkpoint starts concealed"),
        S->ZoneStates[4].bDiscovered);

    FAIHeroesAdventureGraph Player;
    TestTrue(TEXT("Player view is derived from verified snapshot"),
        FAIHeroesProceduralCampaignRules::BuildPlayerGraph(
            *S, Player, Error));
    TestEqual(TEXT("Only four starting zones visible"), Player.Nodes.Num(), 4);
    for (const auto& Road : Player.Links)
    {
        TestFalse(TEXT("No undiscovered PASS road endpoint"),
            Road.From == FName(TEXT("PASS")) ||
            Road.To == FName(TEXT("PASS")));
        TestFalse(TEXT("No RUINS road leaked"),
            Road.From == FName(TEXT("RUINS")) ||
            Road.To == FName(TEXT("RUINS")));
    }
    const auto First = FAIHeroesAdventurePath::Find(
        Player, TEXT("HQ"), TEXT("FOREST"), 1000);
    TestTrue(TEXT("FOREST is still an explicitly guarded encounter"),
        First.Status == EAIHeroesAdventurePathStatus::Ready &&
        First.bBattleAtDestination);
    TestTrue(TEXT("Hidden objective has no traversable proposal"),
        FAIHeroesAdventurePath::Find(Player, TEXT("HQ"), TEXT("RUINS"), 1000)
            .Status == EAIHeroesAdventurePathStatus::InvalidInput);

    // Test-state transition mirrors a battle result applied elsewhere.
    // It is not a method authorizing the outcome of that battle.
    FAIHeroesProceduralSavedZone* Forest = Find(*S, TEXT("FOREST"));
    FAIHeroesProceduralSavedZone* Pass = Find(*S, TEXT("PASS"));
    if (!TestTrue(TEXT("Canonical zone IDs are present"), Forest && Pass))
        return false;
    Forest->bOwned = true;
    Forest->bGuardCleared = true;
    Pass->bDiscovered = true;
    S->ActionRevision = 1;
    TestTrue(TEXT("Stage-one saved state validates"),
        FAIHeroesProceduralCampaignRules::Validate(*S, Error));
    FAIHeroesAdventureGraph Later;
    TestTrue(TEXT("Stage-one fog projection passes"),
        FAIHeroesProceduralCampaignRules::BuildPlayerGraph(
            *S, Later, Error));
    TestTrue(TEXT("Checkpoint available only as new guarded encounter"),
        FAIHeroesAdventurePath::Find(Later, TEXT("HQ"), TEXT("PASS"), 1000)
            .bBattleAtDestination);
    TestTrue(TEXT("An action revision invalidates an old preview"),
        Later.Revision != Player.Revision);
    TestFalse(TEXT("Projection did not mutate hidden state"),
        S->ZoneStates[5].bDiscovered);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesProceduralV2TamperTest,
    "AIHeroes.ProceduralV2.RejectTampering",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesProceduralV2TamperTest::RunTest(const FString&)
{
    UAIHeroesProceduralCampaignSave* S = CreateFixture(37u, 4);
    if (!TestTrue(TEXT("Valid fixture created"), IsValid(S))) return false;
    FString Error;
    TestTrue(TEXT("Baseline valid"),
        FAIHeroesProceduralCampaignRules::Validate(*S, Error));

    S->SchemaVersion = 1;
    TestFalse(TEXT("Existing five-site schema not accepted as v2"),
        FAIHeroesProceduralCampaignRules::Validate(*S, Error));
    S->SchemaVersion = 2;

    const uint32 OriginalRevision = S->TopologyRevision;
    ++S->TopologyRevision;
    TestFalse(TEXT("Forged seed topology rejected"),
        FAIHeroesProceduralCampaignRules::Validate(*S, Error));
    S->TopologyRevision = OriginalRevision;

    S->ExtraRoutes = -1;
    TestFalse(TEXT("Negative route count rejected"),
        FAIHeroesProceduralCampaignRules::Validate(*S, Error));
    S->ExtraRoutes = 4;

    S->ZoneStates[2].bOwned = true;
    TestFalse(TEXT("Cannot own living guarded forest"),
        FAIHeroesProceduralCampaignRules::Validate(*S, Error));
    S->ZoneStates[2].bOwned = false;

    S->HeroZoneId = TEXT("RUINS");
    TestFalse(TEXT("No hero teleporting into unrevealed enemy territory"),
        FAIHeroesProceduralCampaignRules::Validate(*S, Error));
    S->HeroZoneId = TEXT("HQ");

    S->SelectedZoneId = TEXT("PASS");
    TestFalse(TEXT("Fogged selection rejected"),
        FAIHeroesProceduralCampaignRules::Validate(*S, Error));
    S->SelectedZoneId = NAME_None;

    const int32 Gold = S->Gold;
    S->Gold = -1;
    FAIHeroesAdventureGraph Leaked;
    Leaked.Nodes.Add({FName(TEXT("STALE")), true, true, false});
    TestFalse(TEXT("Invalid snapshot cannot build a player graph"),
        FAIHeroesProceduralCampaignRules::BuildPlayerGraph(
            *S, Leaked, Error));
    TestTrue(TEXT("Failure clears pre-existing route data"),
        Leaked.Nodes.IsEmpty() && Leaked.Links.IsEmpty());
    S->Gold = Gold;

    const int32 OriginalAttack = S->PlayerArmy[0].Attack;
    S->PlayerArmy[0].Attack = 9999;
    TestFalse(TEXT("Forged army statistics rejected"),
        FAIHeroesProceduralCampaignRules::Validate(*S, Error));
    S->PlayerArmy[0].Attack = OriginalAttack;

    TestTrue(TEXT("Repeated rejected changes have not corrupted baseline"),
        FAIHeroesProceduralCampaignRules::Validate(*S, Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesProceduralV2DiskRoundTripTest,
    "AIHeroes.ProceduralV2.ScratchDiskRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesProceduralV2DiskRoundTripTest::RunTest(const FString&)
{
    // ONLY a unique disposable test slot; v1 player slots are never touched.
    const FString Scratch = FString::Printf(
        TEXT("AIHEROES_ProceduralV2_TEST_%s"),
        *FGuid::NewGuid().ToString(EGuidFormats::Digits));
    UAIHeroesProceduralCampaignSave* Before =
        CreateFixture(20261029u, 4);
    if (!TestTrue(TEXT("Snapshot initialized"), IsValid(Before))) return false;
    Before->Day = 3;
    Before->MovesLeft = 2;
    Before->SaveSequence = 1;
    Before->ActionRevision = 1;
    Before->ProtocolTier = 1;
    Before->ModelSlotTier = 1;
    Before->DataFragments = 3;
    Before->HeroZoneId = TEXT("FOREST");
    Before->SelectedZoneId = TEXT("PASS");
    Find(*Before, TEXT("FOREST"))->bOwned = true;
    Find(*Before, TEXT("FOREST"))->bGuardCleared = true;
    Find(*Before, TEXT("PASS"))->bDiscovered = true;
    FString Error;
    if (!TestTrue(TEXT("Disk fixture valid before any write"),
            FAIHeroesProceduralCampaignRules::Validate(*Before, Error)))
        return false;

    const bool bSaved = UGameplayStatics::SaveGameToSlot(Before, Scratch, 0);
    TestTrue(TEXT("Only GUID-scoped temporary v2 slot written"), bSaved);
    UAIHeroesProceduralCampaignSave* After = bSaved
        ? Cast<UAIHeroesProceduralCampaignSave>(
            UGameplayStatics::LoadGameFromSlot(Scratch, 0))
        : nullptr;
    TestTrue(TEXT("Real Unreal SaveGame deserialization works"), IsValid(After));
    if (After)
    {
        TestTrue(TEXT("Loaded snapshot is semantically valid"),
            FAIHeroesProceduralCampaignRules::Validate(*After, Error));
        TestTrue(TEXT("Every v2 field survives disk roundtrip"),
            FAIHeroesProceduralCampaignRules::Equivalent(*Before, *After));
        FAIHeroesAdventureGraph Player;
        TestTrue(TEXT("Restored state rebuilds fog-limited route graph"),
            FAIHeroesProceduralCampaignRules::BuildPlayerGraph(
                *After, Player, Error));
        TestTrue(TEXT("Restored selection and hero are preserved"),
            After->HeroZoneId == FName(TEXT("FOREST")) &&
            After->SelectedZoneId == FName(TEXT("PASS")));
        After->Gold = -5;
        TestFalse(TEXT("Corrupted loaded record is rejected"),
            FAIHeroesProceduralCampaignRules::Validate(*After, Error));
    }
    if (bSaved)
    {
        TestTrue(TEXT("GUID-scoped temporary slot removed"),
            UGameplayStatics::DeleteGameInSlot(Scratch, 0));
        TestFalse(TEXT("No test save remains"),
            UGameplayStatics::DoesSaveGameExist(Scratch, 0));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesProceduralV2LegacyBoundaryTest,
    "AIHeroes.ProceduralV2.LegacyRequiresNewCampaign",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesProceduralV2LegacyBoundaryTest::RunTest(const FString&)
{
    UAIHeroesCampaignSave* Old = LegacyFixture();
    FString Error;
    if (!TestTrue(TEXT("Actual five-node schema-one fixture is valid"),
            FAIHeroesCampaignRules::Validate(*Old, Error)))
        return false;
    const int32 OldGold = Old->Gold;
    const int32 OldDay = Old->Day;
    const FString OldSlot = UAIHeroesCampaignSave::SlotName();
    TestTrue(TEXT("Old user slot remains a different namespace"),
        !OldSlot.StartsWith(TEXT("AIHEROES_ProceduralV2_TEST_")));

    const auto Decision =
        FAIHeroesProceduralCampaignRules::AssessLegacy(*Old, Error);
    TestTrue(TEXT("Lossy five->ten-site conversion is explicitly refused"),
        Decision == EAIHeroesProceduralLegacyDecision::RequiresNewCampaign);
    TestTrue(TEXT("User-facing refusal identifies new-campaign requirement"),
        Error.Contains(TEXT("NEW CAMPAIGN REQUIRED")));
    TestTrue(TEXT("No old progress or fields modified"),
        Old->SchemaVersion == 1 && Old->NodeStates.Num() == 5 &&
        Old->Gold == OldGold && Old->Day == OldDay);
    Old->Gold = -5;
    TestTrue(TEXT("Damaged old snapshot is distinguished from valid legacy"),
        FAIHeroesProceduralCampaignRules::AssessLegacy(*Old, Error) ==
        EAIHeroesProceduralLegacyDecision::InvalidLegacy);
    TestTrue(TEXT("Corruption produces an explicit reason"),
        Error.Contains(TEXT("V1 VALIDATION FAILED")));
    return true;
}
#endif
