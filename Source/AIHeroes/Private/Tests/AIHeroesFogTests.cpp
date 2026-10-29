#include "AIHeroesFogRules.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AIHeroesAdvisor.h"
#include "AIHeroesAIProposalGate.h"
#include "AIHeroesCampaignRules.h"
#include "AIHeroesCampaignSave.h"
#include "AIHeroesTravelRules.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/Guid.h"

namespace
{
FAIHeroesAdvisorSnapshot MakeFogTravelSnapshot()
{
    FAIHeroesAdvisorSnapshot S;
    S.TravelRuleVersion = FAIHeroesTravelRules::RoadVersion;
    S.bFogEnabled = true;
    S.ExploredSitesMask = FAIHeroesFogRules::InitialMask;
    S.MovesLeft = 4;
    S.ProtocolTier = 3;
    S.HeroPosition = FVector(-650.f, 0.f, 155.f);
    S.PlayerArmy.Add(FAIHeroesArmyStack(TEXT("Militia"), 8, 12, 4, 2, 3));
    S.PlayerArmy.Add(FAIHeroesArmyStack(TEXT("Scout"), 3, 8, 6, 1, 7));

    auto Site = [&S](const TCHAR* Id, EAIHeroesNodeType Type,
        FVector At, bool bOwned, const TArray<FAIHeroesArmyStack>& Defenders)
    {
        FAIHeroesAdvisorNode Node;
        Node.Id = FName(Id);
        Node.Type = Type;
        Node.Location = At;
        Node.bOwned = bOwned;
        Node.Guards = Defenders;
        S.Nodes.Add(MoveTemp(Node));
    };

    Site(TEXT("HQ"), EAIHeroesNodeType::Headquarters,
        S.HeroPosition, true, {});
    Site(TEXT("FOREST"), EAIHeroesNodeType::ForestMine,
        FVector(-100.f, -650.f, 110.f), false,
        {FAIHeroesArmyStack(TEXT("Wolf"), 4, 9, 3, 1, 8)});
    Site(TEXT("ORE"), EAIHeroesNodeType::OreMine,
        FVector(150.f, 600.f, 120.f), false,
        {FAIHeroesArmyStack(TEXT("Bandit"), 6, 10, 4, 2, 4)});
    Site(TEXT("OUTPOST"), EAIHeroesNodeType::Outpost,
        FVector(700.f, -300.f, 140.f), false,
        {FAIHeroesArmyStack(TEXT("Raider"), 6, 11, 5, 2, 5)});
    Site(TEXT("RUINS"), EAIHeroesNodeType::Ruins,
        FVector(750.f, 650.f, 165.f), false,
        {FAIHeroesArmyStack(TEXT("Guardian"), 7, 24, 7, 4, 2)});
    return S;
}

UAIHeroesCampaignSave* MakeFogSave()
{
    UAIHeroesCampaignSave* Save =
        NewObject<UAIHeroesCampaignSave>(GetTransientPackage());
    Save->TravelRuleVersion = FAIHeroesTravelRules::RoadVersion;
    Save->Day = 2;
    Save->Gold = 245;
    Save->HeroNodeId = FName(TEXT("HQ"));
    Save->SelectedNodeId = FName(TEXT("FOREST"));
    Save->PlayerArmy.Add(FAIHeroesArmyStack(TEXT("Militia"), 8, 12, 4, 2, 3));
    Save->bFogInitialized = true;
    Save->ExploredSitesMask = FAIHeroesFogRules::InitialMask;

    auto Site = [Save](const TCHAR* Id, EAIHeroesNodeType Type,
        bool bOwned, const TArray<FAIHeroesArmyStack>& Defenders)
    {
        FAIHeroesSavedNode Node;
        Node.NodeId = FName(Id);
        Node.Type = Type;
        Node.bOwned = bOwned;
        Node.Defenders = Defenders;
        Save->NodeStates.Add(MoveTemp(Node));
    };
    Site(TEXT("HQ"), EAIHeroesNodeType::Headquarters, true, {});
    Site(TEXT("FOREST"), EAIHeroesNodeType::ForestMine, false,
        {FAIHeroesArmyStack(TEXT("Wolf"), 4, 9, 3, 1, 8)});
    Site(TEXT("ORE"), EAIHeroesNodeType::OreMine, false,
        {FAIHeroesArmyStack(TEXT("Bandit"), 6, 10, 4, 2, 4)});
    Site(TEXT("OUTPOST"), EAIHeroesNodeType::Outpost, false,
        {FAIHeroesArmyStack(TEXT("Raider"), 6, 11, 5, 2, 5),
         FAIHeroesArmyStack(TEXT("Raider"), 4, 11, 5, 2, 5)});
    Site(TEXT("RUINS"), EAIHeroesNodeType::Ruins, false,
        {FAIHeroesArmyStack(TEXT("Guardian"), 7, 24, 7, 4, 2)});
    return Save;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesFogDiscoveryTest,
    "AIHeroes.Fog.AdjacentDiscoveryAndLegacy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesFogDiscoveryTest::RunTest(const FString&)
{
    const int32 Starter = FAIHeroesFogRules::InitialMask;
    TestEqual(TEXT("Three starter sites"), FAIHeroesFogRules::CountKnown(true, Starter), 3);
    TestTrue(TEXT("Starting FOREST is known"),
        FAIHeroesFogRules::IsKnown(true, Starter, FName(TEXT("FOREST"))));
    TestFalse(TEXT("OUTPOST initially hidden"),
        FAIHeroesFogRules::IsKnown(true, Starter, FName(TEXT("OUTPOST"))));
    TestFalse(TEXT("RUINS initially hidden"),
        FAIHeroesFogRules::IsKnown(true, Starter, FName(TEXT("RUINS"))));

    int32 Next = INDEX_NONE;
    FString Error;
    TestTrue(TEXT("FOREST visit reveals one branch"),
        FAIHeroesFogRules::Reveal(true, Starter, FName(TEXT("FOREST")), Next, Error));
    TestEqual(TEXT("FOREST reveals OUTPOST only"), Next, 15);
    const int32 AfterForest = Next;
    TestTrue(TEXT("Discovering RUINS requires travel toward ORE"),
        FAIHeroesFogRules::Reveal(true, AfterForest, FName(TEXT("ORE")), Next, Error));
    TestEqual(TEXT("Both branches explored"), Next, FAIHeroesFogRules::AllMask);

    TestTrue(TEXT("Legacy no fog retains all five known"),
        FAIHeroesFogRules::IsKnown(false, 0, FName(TEXT("RUINS"))));
    TestEqual(TEXT("Legacy total sites"), FAIHeroesFogRules::CountKnown(false, 0), 5);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesFogBoundariesTest,
    "AIHeroes.Fog.TamperAndAuthorityBoundaries",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesFogBoundariesTest::RunTest(const FString&)
{
    FString Error;
    TSet<FName> Owned = {FName(TEXT("HQ"))};
    TestTrue(TEXT("Modern initial snapshot is valid"), FAIHeroesFogRules::Validate(
        true, 7, FName(TEXT("HQ")), FName(TEXT("FOREST")), Owned, Error));
    TestTrue(TEXT("Legacy missing optional fields are valid"), FAIHeroesFogRules::Validate(
        false, 0, FName(TEXT("HQ")), NAME_None, Owned, Error));
    TestFalse(TEXT("Legacy must not carry a modern mask"), FAIHeroesFogRules::Validate(
        false, 7, FName(TEXT("HQ")), NAME_None, Owned, Error));
    TestFalse(TEXT("Unknown bit rejected"), FAIHeroesFogRules::Validate(
        true, 7 | 64, FName(TEXT("HQ")), NAME_None, Owned, Error));
    TestFalse(TEXT("Hidden hero rejected"), FAIHeroesFogRules::Validate(
        true, 7, FName(TEXT("RUINS")), NAME_None, Owned, Error));
    TestFalse(TEXT("Hidden selection rejected"), FAIHeroesFogRules::Validate(
        true, 7, FName(TEXT("HQ")), FName(TEXT("OUTPOST")), Owned, Error));
    Owned.Add(FName(TEXT("OUTPOST")));
    TestFalse(TEXT("Cannot hide owned territory"), FAIHeroesFogRules::Validate(
        true, 7, FName(TEXT("HQ")), NAME_None, Owned, Error));

    int32 Mask = INDEX_NONE;
    TestFalse(TEXT("No remote exploration of unknown RUINS"), FAIHeroesFogRules::Reveal(
        true, 7, FName(TEXT("RUINS")), Mask, Error));
    TestEqual(TEXT("Failed scouting preserves the input mask"), Mask, 7);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesFogPathTest,
    "AIHeroes.Fog.TravelGraphAndAdviceDoNotLeak",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesFogPathTest::RunTest(const FString&)
{
    FAIHeroesAdvisorSnapshot S = MakeFogTravelSnapshot();
    FAIHeroesAdventureGraph Graph;
    FString Error;
    if (!TestTrue(TEXT("Map graph remains valid with hidden nodes"),
        FAIHeroesTravelRules::BuildGraph(S, Graph, Error))) return false;
    TestFalse(TEXT("OUTPOST unavailable for pathfinding"), Graph.Nodes[3].bDiscovered);
    TestFalse(TEXT("RUINS unavailable for pathfinding"), Graph.Nodes[4].bDiscovered);
    TestTrue(TEXT("FOREST is explorable"), Graph.Nodes[1].bDiscovered);

    const auto Hidden = FAIHeroesTravelRules::Find(
        S, S.HeroPosition, FName(TEXT("OUTPOST")), S.MovesLeft);
    TestEqual(TEXT("Hidden OUTPOST not reachable"), Hidden.Status,
        EAIHeroesAdventurePathStatus::Unreachable);
    const auto Open = FAIHeroesTravelRules::Find(
        S, S.HeroPosition, FName(TEXT("FOREST")), S.MovesLeft);
    TestEqual(TEXT("FOREST available at start"), Open.TotalCost, 2);
    TestTrue(TEXT("FOREST player-confirmable"), Open.IsReachableThisTurn());

    const FString Advice = FAIHeroesAdvisor::Advise(S);
    TestFalse(TEXT("Hidden OUTPOST omitted from no-selection advice"),
        Advice.Contains(TEXT("OUTPOST")));
    TestFalse(TEXT("Hidden RUINS omitted from no-selection advice"),
        Advice.Contains(TEXT("RUINS")));

    S.ExploredSitesMask = 15;
    S.HeroPosition = FVector(-100.f, -650.f, 110.f);
    S.Nodes[1].bOwned = true;
    S.Nodes[1].Guards.Empty();
    const auto Opened = FAIHeroesTravelRules::Find(
        S, S.HeroPosition, FName(TEXT("OUTPOST")), S.MovesLeft);
    TestTrue(TEXT("After forest visit OUTPOST is one move away"),
        Opened.IsReachableThisTurn());
    TestEqual(TEXT("OUTPOST road cost"), Opened.TotalCost, 1);
    const auto StillHidden = FAIHeroesTravelRules::Find(
        S, S.HeroPosition, FName(TEXT("RUINS")), S.MovesLeft);
    TestEqual(TEXT("RUINS stays unknown until scouted"), StillHidden.Status,
        EAIHeroesAdventurePathStatus::Unreachable);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesFogSaveTest,
    "AIHeroes.Fog.SaveValidationAndDiskRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesFogSaveTest::RunTest(const FString&)
{
    UAIHeroesCampaignSave* Original = MakeFogSave();
    FString Error;
    if (!TestTrue(TEXT("Existing campaign schema accepts optional fog"),
        FAIHeroesCampaignRules::Validate(*Original, Error))) return false;

    Original->ExploredSitesMask = 7 | 32;
    TestFalse(TEXT("Injected visibility bits fail closed"),
        FAIHeroesCampaignRules::Validate(*Original, Error));
    Original->ExploredSitesMask = 7;
    Original->SelectedNodeId = FName(TEXT("RUINS"));
    TestFalse(TEXT("Cannot restore a forbidden hidden selection"),
        FAIHeroesCampaignRules::Validate(*Original, Error));
    Original->SelectedNodeId = FName(TEXT("FOREST"));
    Original->bFogInitialized = false;
    TestFalse(TEXT("Uninitialized legacy fog may not carry modern bits"),
        FAIHeroesCampaignRules::Validate(*Original, Error));
    Original->ExploredSitesMask = 0;
    TestTrue(TEXT("Old save structure accepted with full knowledge"),
        FAIHeroesCampaignRules::Validate(*Original, Error));
    Original->bFogInitialized = true;
    Original->ExploredSitesMask = 7;

    const FString Slot = FString::Printf(TEXT("AIHEROES_FOG_TEST_%s"),
        *FGuid::NewGuid().ToString(EGuidFormats::Digits));
    const bool bWrote = UGameplayStatics::SaveGameToSlot(Original, Slot, 0);
    if (!TestTrue(TEXT("Unique temporary SaveGame written"), bWrote)) return false;
    UAIHeroesCampaignSave* Read = Cast<UAIHeroesCampaignSave>(
        UGameplayStatics::LoadGameFromSlot(Slot, 0));
    if (TestTrue(TEXT("Fog SaveGame readable"), IsValid(Read)))
    {
        TestTrue(TEXT("Fog flag preserved"), Read->bFogInitialized);
        TestEqual(TEXT("Fog bitmask persists exactly"), Read->ExploredSitesMask, 7);
        TestTrue(TEXT("Loaded snapshot remains valid"),
            FAIHeroesCampaignRules::Validate(*Read, Error));
    }
    TestTrue(TEXT("Only test GUID slot deleted"),
        UGameplayStatics::DeleteGameInSlot(Slot, 0));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesFogProposalGateTest,
    "AIHeroes.Fog.ProposalGateRejectsUndiscoveredSite",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesFogProposalGateTest::RunTest(const FString&)
{
    FAIHeroesProposalSnapshot Snapshot;
    Snapshot.Day = 2;
    Snapshot.StateRevision = 0xADA55A1u;
    Snapshot.ModelSlotTier = 0;
    Snapshot.Strategic = MakeFogTravelSnapshot();
    Snapshot.Strategic.ProtocolTier = 2;
    Snapshot.Strategic.SelectedNodeId = FName(TEXT("FOREST"));

    const FAIHeroesAIProposal Legal =
        FAIHeroesAIProposalGate::SuggestSelected(Snapshot);
    TestEqual(TEXT("Visible forest can be suggested"),
        Legal.Kind, EAIHeroesProposalKind::ProposeOrder);
    TestTrue(TEXT("Legal proposal still needs player permission"),
        FAIHeroesAIProposalGate::Validate(Snapshot, Legal).bPlayerConfirmationRequired);

    FAIHeroesAIProposal Forged = Legal;
    Forged.TargetNode = FName(TEXT("OUTPOST"));
    Forged.Order = EAIHeroesProposalOrder::AttackNode;
    TestFalse(TEXT("Forged hidden-site order must be rejected"),
        FAIHeroesAIProposalGate::Validate(Snapshot, Forged).bAccepted);

    Snapshot.Strategic.SelectedNodeId = FName(TEXT("OUTPOST"));
    const FAIHeroesAIProposal Hidden =
        FAIHeroesAIProposalGate::SuggestSelected(Snapshot);
    TestEqual(TEXT("Even a typed hidden selection cannot propose a command"),
        Hidden.Kind, EAIHeroesProposalKind::NoAction);

    Snapshot.Strategic.ExploredSitesMask = FAIHeroesFogRules::InitialMask | 64;
    const FAIHeroesAIProposal Corrupted =
        FAIHeroesAIProposalGate::SuggestSelected(Snapshot);
    TestEqual(TEXT("Invalid extra fog bits fail closed"),
        Corrupted.Kind, EAIHeroesProposalKind::NoAction);

    Snapshot.Strategic.ExploredSitesMask = 15;
    const FAIHeroesAIProposal Revealed =
        FAIHeroesAIProposalGate::SuggestSelected(Snapshot);
    TestEqual(TEXT("Discovered outpost is eligible for advisory proposal"),
        Revealed.Kind, EAIHeroesProposalKind::ProposeOrder);
    return true;
}
#endif
