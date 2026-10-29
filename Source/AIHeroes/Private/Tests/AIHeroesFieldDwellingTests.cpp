#include "AIHeroesFieldDwelling.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AIHeroesCampaignSave.h"
#include "AIHeroesCampaignRules.h"
#include "AIHeroesGameMode.h"
#include "AIHeroesHUDLayout.h"
#include "AIHeroesSaveSystem.h"
#include "AIHeroesWorldNode.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/Guid.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "UObject/UnrealType.h"

namespace
{
struct FFieldFixture
{
    UWorld* World = nullptr;
    AAIHeroesGameMode* Game = nullptr;
    AAIHeroesWorldNode* Forest = nullptr;
    AAIHeroesWorldNode* Ore = nullptr;

    FFieldFixture()
    {
        const UWorld::InitializationValues Values =
            UWorld::InitializationValues()
                .AllowAudioPlayback(false).CreatePhysicsScene(false)
                .CreateNavigation(false).CreateAISystem(false)
                .ShouldSimulatePhysics(false);
        World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None,
            nullptr, true, ERHIFeatureLevel::Num, &Values);
        if (!World) return;
        if (GEngine) GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        Game = World->SpawnActor<AAIHeroesGameMode>();
        if (!Game) return;
        const TCHAR* Names[] = {
            TEXT("HQ"), TEXT("FOREST"), TEXT("ORE"), TEXT("OUTPOST"), TEXT("RUINS")};
        const EAIHeroesNodeType Types[] = {
            EAIHeroesNodeType::Headquarters, EAIHeroesNodeType::ForestMine,
            EAIHeroesNodeType::OreMine, EAIHeroesNodeType::Outpost,
            EAIHeroesNodeType::Ruins};
        for (int32 I = 0; I < 5; ++I)
        {
            const FVector Pos = I == 0
                ? Game->GetHeroStrategicLocation()
                : FVector(-450.f + I * 130.f, -300.f + I * 100.f, 110.f);
            auto* N = World->SpawnActor<AAIHeroesWorldNode>(Pos, FRotator::ZeroRotator);
            if (!N) return;
            N->Configure(Types[I], Names[I], {});
            Game->Nodes.Add(N);
            if (I == 1) Forest = N;
            if (I == 2) Ore = N;
        }
        Game->SelectedNode = Game->Nodes[0];
        Game->PlayerArmy = {
            FAIHeroesArmyStack(TEXT("Militia"), 8, 12, 4, 2, 3),
            FAIHeroesArmyStack(TEXT("Scout"), 3, 8, 6, 1, 7)};
        Game->Gold = 1000;
        Game->Wood = 100;
        Game->Ore = 100;
    }

    ~FFieldFixture()
    {
        if (World)
        {
            World->DestroyWorld(false);
            if (GEngine) GEngine->DestroyWorldContext(World);
        }
    }

    bool Valid() const
    {
        return World && Game && Forest && Ore && Game->Nodes.Num() == 5;
    }

    UAIHeroesCampaignSave* Snapshot() const
    {
        UAIHeroesCampaignSave* S =
            NewObject<UAIHeroesCampaignSave>(GetTransientPackage());
        S->HeroNodeId = TEXT("HQ");
        S->SelectedNodeId = TEXT("HQ");
        S->PlayerArmy = Game->PlayerArmy;
        for (const AAIHeroesWorldNode* N : Game->Nodes)
        {
            FAIHeroesSavedNode X;
            X.NodeId = FName(*N->GetDisplayName());
            X.Type = N->NodeType;
            X.bOwned = N->bOwned;
            X.Defenders = N->Defenders;
            S->NodeStates.Add(X);
        }
        return S;
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesFieldCalendarTest,
    "AIHeroes.Field.CalendarNeutralResetVsOwnedAccrual",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesFieldCalendarTest::RunTest(const FString& Parameters)
{
    const auto Fresh = FAIHeroesFieldDwellingRules::Initial();
    TestEqual(TEXT("Free forest camp starts with five Militia"), Fresh.Forest.Available, 5);
    TestEqual(TEXT("Free ore aerie starts with two Scouts"), Fresh.Ore.Available, 2);
    TestFalse(TEXT("Free camp is not silently player-owned"), Fresh.Forest.bClaimed);
    FAIHeroesFieldDwellings After;
    FString Error;
    for (int32 Day : {1, 2, 6, 8, 13})
    {
        TestTrue(TEXT("Non-boundary transition valid"),
            FAIHeroesFieldDwellingRules::PlanNextDay(
                Day, false, false, Fresh, After, Error));
        TestTrue(TEXT("Stock unchanged between weekly boundaries"), After == Fresh);
    }
    FAIHeroesFieldDwellings Used = Fresh;
    Used.Forest.Available = 1;
    Used.Ore.Available = 0;
    TestTrue(TEXT("Unclaimed sites reset at week 2"),
        FAIHeroesFieldDwellingRules::PlanNextDay(
            7, false, false, Used, After, Error));
    TestEqual(TEXT("Neutral camp resets to a single week's stock"), After.Forest.Available, 5);
    TestEqual(TEXT("Neutral aerie resets without accumulation"), After.Ore.Available, 2);
    Used.Forest.bClaimed = true;
    Used.Ore.bClaimed = true;
    TestTrue(TEXT("Both claimed sites produce"),
        FAIHeroesFieldDwellingRules::PlanNextDay(
            7, true, true, Used, After, Error));
    TestEqual(TEXT("Claimed camp accumulates"), After.Forest.Available, 6);
    TestEqual(TEXT("Claimed aerie accumulates"), After.Ore.Available, 2);
    const FAIHeroesFieldDwellings W2 = After;
    TestTrue(TEXT("Another week accrues another packet"),
        FAIHeroesFieldDwellingRules::PlanNextDay(
            14, true, true, W2, After, Error));
    TestEqual(TEXT("Owned camp keeps unspent troops"), After.Forest.Available, 11);
    TestEqual(TEXT("Owned aerie keeps unspent troops"), After.Ore.Available, 4);
    TestEqual(TEXT("Pure planner has not changed its input"), Used.Forest.Available, 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesFieldOwnershipTest,
    "AIHeroes.Field.ClaimAuthorityAndTerritoryLoss",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesFieldOwnershipTest::RunTest(const FString& Parameters)
{
    auto State = FAIHeroesFieldDwellingRules::Initial();
    const auto Initial = State;
    FString Error;
    TestFalse(TEXT("Cannot claim hostile site"),
        FAIHeroesFieldDwellingRules::TryClaim(
            EAIHeroesFieldSite::ForestCamp, false, true, State, Error));
    TestFalse(TEXT("Cannot remotely claim a secured site"),
        FAIHeroesFieldDwellingRules::TryClaim(
            EAIHeroesFieldSite::ForestCamp, true, false, State, Error));
    TestTrue(TEXT("Rejected claims leave both sites unchanged"), State == Initial);
    TestTrue(TEXT("Visiting cleared site explicitly claims it"),
        FAIHeroesFieldDwellingRules::TryClaim(
            EAIHeroesFieldSite::ForestCamp, true, true, State, Error));
    TestFalse(TEXT("Double claim is no-op"),
        FAIHeroesFieldDwellingRules::TryClaim(
            EAIHeroesFieldSite::ForestCamp, true, true, State, Error));
    TestTrue(TEXT("Forest ownership preserved"), State.Forest.bClaimed);
    FAIHeroesFieldDwellings After;
    TestTrue(TEXT("Territory loss can be preflighted"),
        FAIHeroesFieldDwellingRules::PlanNextDay(
            6, false, false, State, After, Error));
    TestFalse(TEXT("Lost territory revokes facility claim"), After.Forest.bClaimed);
    TestEqual(TEXT("Existing stock remains until next weekly reset"),
        After.Forest.Available, State.Forest.Available);
    TestFalse(TEXT("Invalid site ID cannot be claimed"),
        FAIHeroesFieldDwellingRules::TryClaim(
            static_cast<EAIHeroesFieldSite>(99), true, true, State, Error));
    TestTrue(TEXT("Invalid site never alters ownership"), State.Forest.bClaimed);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesFieldRecruitTest,
    "AIHeroes.Field.RecruitAtomicNoFreeArmy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesFieldRecruitTest::RunTest(const FString& Parameters)
{
    FAIHeroesFieldDwellings Stock = FAIHeroesFieldDwellingRules::Initial();
    int32 Gold = 100, Wood = 10, Ore = 10;
    TArray<FAIHeroesArmyStack> Army = {
        FAIHeroesArmyStack(TEXT("Militia"), 8, 12, 4, 2, 3)};
    FString Error;
    TestFalse(TEXT("Cannot buy unclaimed dwelling"),
        FAIHeroesFieldDwellingRules::TryRecruit(
            EAIHeroesFieldSite::ForestCamp, true, true,
            Stock, Gold, Wood, Ore, Army, Error));
    TestEqual(TEXT("Denied purchase keeps gold"), Gold, 100);
    TestEqual(TEXT("Denied purchase keeps troops"), Army[0].Count(), 8);
    TestTrue(TEXT("Player claims camp with explicit action"),
        FAIHeroesFieldDwellingRules::TryClaim(
            EAIHeroesFieldSite::ForestCamp, true, true, Stock, Error));
    TestFalse(TEXT("No remote recruitment"),
        FAIHeroesFieldDwellingRules::TryRecruit(
            EAIHeroesFieldSite::ForestCamp, true, false,
            Stock, Gold, Wood, Ore, Army, Error));
    TestTrue(TEXT("One local pack succeeds"),
        FAIHeroesFieldDwellingRules::TryRecruit(
            EAIHeroesFieldSite::ForestCamp, true, true,
            Stock, Gold, Wood, Ore, Army, Error));
    TestEqual(TEXT("Consumes exactly fifty gold"), Gold, 50);
    TestEqual(TEXT("Consumes exactly five wood"), Wood, 5);
    TestEqual(TEXT("Does not charge ore"), Ore, 10);
    TestEqual(TEXT("Local stock consumed, HQ reserve independent"), Stock.Forest.Available, 0);
    TestEqual(TEXT("Five Militia join existing stack"), Army[0].Count(), 13);
    TestFalse(TEXT("Second same-week local pack blocked"),
        FAIHeroesFieldDwellingRules::TryRecruit(
            EAIHeroesFieldSite::ForestCamp, true, true,
            Stock, Gold, Wood, Ore, Army, Error));
    TestEqual(TEXT("Failed follow-up does not charge resources"), Gold, 50);
    TestEqual(TEXT("Failed follow-up creates no army"), Army[0].Count(), 13);
    TestFalse(TEXT("Ore aerie does not auto-claim after forest claim"), Stock.Ore.bClaimed);
    TestTrue(TEXT("Ore aerie requires independent claim"),
        FAIHeroesFieldDwellingRules::TryClaim(
            EAIHeroesFieldSite::OreAerie, true, true, Stock, Error));
    TestFalse(TEXT("Insufficient gold prevents Ore pack"),
        FAIHeroesFieldDwellingRules::TryRecruit(
            EAIHeroesFieldSite::OreAerie, true, true,
            Stock, Gold, Wood, Ore, Army, Error));
    Gold = 100;
    TestTrue(TEXT("Scout pack bought locally without HQ stock"),
        FAIHeroesFieldDwellingRules::TryRecruit(
            EAIHeroesFieldSite::OreAerie, true, true,
            Stock, Gold, Wood, Ore, Army, Error));
    TestEqual(TEXT("Scout cost"), Gold, 30);
    TestEqual(TEXT("Scout pack creates second stack"), Army.Num(), 2);
    TestEqual(TEXT("Two scouts"), Army[1].Count(), 2);
    TestEqual(TEXT("Ore aerie stock exhausted"), Stock.Ore.Available, 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesFieldSaveTest,
    "AIHeroes.Field.SaveV1OptionalDataRecovery",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesFieldSaveTest::RunTest(const FString& Parameters)
{
    FFieldFixture F;
    if (!TestTrue(TEXT("Disposable campaign exists"), F.Valid())) return false;
    UAIHeroesCampaignSave* S = F.Snapshot();
    FString Error;
    TestFalse(TEXT("Pre-feature snapshot has no marker"), S->bFieldDwellingsInitialized);
    TestTrue(TEXT("Legacy format remains valid"), FAIHeroesCampaignRules::Validate(*S, Error));
    S->bFieldDwellingsInitialized = true;
    S->bForestCampClaimed = true;
    S->ForestCampUnits = 6;
    TestFalse(TEXT("Unowned FOREST cannot contain claimed dwelling"),
        FAIHeroesCampaignRules::Validate(*S, Error));
    S->NodeStates[1].bOwned = true;
    TestTrue(TEXT("Captured FOREST can contain claimed dwelling"),
        FAIHeroesCampaignRules::Validate(*S, Error));
    S->ForestCampUnits = 51;
    TestFalse(TEXT("Overcapacity field stock denied"),
        FAIHeroesCampaignRules::Validate(*S, Error));
    S->ForestCampUnits = -1;
    TestFalse(TEXT("Negative stock denied"),
        FAIHeroesCampaignRules::Validate(*S, Error));
    S->ForestCampUnits = 6;
    S->bFieldDwellingsInitialized = false;
    TestFalse(TEXT("Absent marker cannot hide nonzero stock"),
        FAIHeroesCampaignRules::Validate(*S, Error));
    S->bFieldDwellingsInitialized = true;

    const FString Prefix = TEXT("AIHEROES_FieldDwellingQA_") +
        FGuid::NewGuid().ToString(EGuidFormats::Digits);
    if (!TestTrue(TEXT("GUID-isolated first save with semantic read-back"),
        FAIHeroesSaveSystem::Save(*S, Prefix, FString(), Error))) return false;
    S->ForestCampUnits = 11;
    if (!TestTrue(TEXT("Second write rotates a protected save copy"),
        FAIHeroesSaveSystem::Save(*S, Prefix, FString(), Error))) return false;
    const FAIHeroesLoadedCampaign Loaded =
        FAIHeroesSaveSystem::Load(Prefix, FString());
    if (TestNotNull(TEXT("Newest valid snapshot loads"), Loaded.Snapshot))
    {
        TestTrue(TEXT("Marker survives tagged-property disk serialization"),
            Loaded.Snapshot->bFieldDwellingsInitialized);
        TestTrue(TEXT("Claimed status survives disk serialization"),
            Loaded.Snapshot->bForestCampClaimed);
        TestEqual(TEXT("New field quantity survives disk serialization"),
            Loaded.Snapshot->ForestCampUnits, 11);
        TestEqual(TEXT("Ore site remains independent"),
            Loaded.Snapshot->OreAerieUnits, 0);
    }
    for (const FString& Slot : {
        FAIHeroesSaveSystem::SlotA(Prefix),
        FAIHeroesSaveSystem::SlotB(Prefix)})
        TestTrue(TEXT("Only test-owned GUID files removed"),
            UGameplayStatics::DeleteGameInSlot(Slot, 0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesFieldGuiTest,
    "AIHeroes.Field.ResponsiveControlsAndCombatLock",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesFieldGuiTest::RunTest(const FString& Parameters)
{
    for (const FVector2D Viewport : {
        FVector2D(1280.f, 720.f),
        FVector2D(960.f, 540.f),
        FVector2D(640.f, 720.f),
        FVector2D(480.f, 800.f)})
    {
        const auto Field = FAIHeroesHUDLayout::Rect(204, Viewport);
        const auto Lab = FAIHeroesHUDLayout::Rect(203, Viewport);
        const auto Goals = FAIHeroesHUDLayout::Rect(202, Viewport);
        const FVector2D Center(Field.X + Field.W / 2.f, Field.Y + Field.H / 2.f);
        TestTrue(TEXT("FIELD E is on-screen and touchable"),
            Field.W >= 90.f && Field.X >= 0.f &&
            Field.X + Field.W <= Viewport.X && Field.Y > 0 &&
            Field.Y + Field.H <= Viewport.Y);
        TestEqual(TEXT("Same touch/mouse event resolves field order"),
            FAIHeroesHUDLayout::HitTest(Center, Viewport), 204);
        TestFalse(TEXT("Lab and Goals do not steal field hitboxes"),
            Lab.Contains(Center) || Goals.Contains(Center));
        TestEqual(TEXT("Battle masks FIELD action"),
            FAIHeroesHUDLayout::Rect(204, Viewport, true).W, 0.f);
        TestEqual(TEXT("Old action 0 remains reachable"),
            FAIHeroesHUDLayout::HitTest(
                FVector2D(FAIHeroesHUDLayout::Rect(0, Viewport).X + 24.f,
                    FAIHeroesHUDLayout::Rect(0, Viewport).Y + 20.f), Viewport), 0);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesFieldGameTest,
    "AIHeroes.Field.GameModeHumanClaimPurchaseAndWeek",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesFieldGameTest::RunTest(const FString& Parameters)
{
    FFieldFixture F;
    if (!TestTrue(TEXT("Disposable GameMode and five nodes exist"), F.Valid())) return false;
    auto& G = *F.Game;
    const int32 StartingGold = G.Gold, StartingWood = G.Wood;
    G.SelectedNode = F.Forest;
    G.InteractFieldDwelling();
    TestFalse(TEXT("Enemy territory cannot be claimed"), G.FieldDwellings.Forest.bClaimed);
    TestEqual(TEXT("Invalid field action spends no gold"), G.Gold, StartingGold);

    F.Forest->SetOwned(true); // Simulate a prior human-authorized capture.
    G.InteractFieldDwelling();
    TestFalse(TEXT("Ownership alone does not teleport the hero"),
        G.FieldDwellings.Forest.bClaimed);
    G.InteractWithNode(F.Forest); // Explicit human movement confirmation.
    if (!TestTrue(TEXT("Hero reached cleared FOREST"),
        FVector::DistSquared(G.GetHeroStrategicLocation(),
            F.Forest->GetActorLocation()) < 1.0)) return false;
    G.InteractFieldDwelling();
    TestTrue(TEXT("Separate E action claims the visited site"),
        G.FieldDwellings.Forest.bClaimed);
    TestEqual(TEXT("Claim alone does not charge gold"), G.Gold, StartingGold);
    TestEqual(TEXT("Claim alone does not add troops"), G.PlayerArmy[0].Count(), 8);
    G.InteractFieldDwelling();
    TestEqual(TEXT("Next E buys exactly one Militia pack"), G.PlayerArmy[0].Count(), 13);
    TestEqual(TEXT("Field recruitment debits the normal price"), G.Gold, StartingGold - 50);
    TestEqual(TEXT("Field recruitment spends five wood"), G.Wood, StartingWood - 5);
    TestEqual(TEXT("Field stock now empty"), G.FieldDwellings.Forest.Available, 0);
    G.InteractFieldDwelling();
    TestEqual(TEXT("Empty field cannot give free troops"), G.PlayerArmy[0].Count(), 13);
    TestEqual(TEXT("Empty field cannot charge gold"), G.Gold, StartingGold - 50);

    G.Day = 7; // Disposable fixture only; no player save mutation.
    G.EndTurn();
    TestEqual(TEXT("Week boundary advances once"), G.Day, 8);
    TestEqual(TEXT("Claimed field grows five Militia"), G.FieldDwellings.Forest.Available, 5);
    TestEqual(TEXT("Unclaimed Ore reset"), G.FieldDwellings.Ore.Available, 2);
    TestTrue(TEXT("Week report distinguishes field from HQ and neutral stock"),
        G.LastEvent.Contains(TEXT("FIELD: FOREST +5")));
    G.InteractFieldDwelling();
    TestEqual(TEXT("Next week allows precisely another field pack"),
        G.FieldDwellings.Forest.Available, 0);
    TestEqual(TEXT("New pack joins hero army"), G.PlayerArmy[0].Count(), 18);
    return true;
}
#endif
