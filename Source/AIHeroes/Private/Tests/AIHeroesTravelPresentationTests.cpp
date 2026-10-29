#include "AIHeroesTravelPresentation.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AIHeroesGameMode.h"
#include "AIHeroesWorldNode.h"
#include "AIHeroesCampaignSave.h"
#include "AIHeroesCampaignRules.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include <limits>

namespace TravelViewTest
{
struct WorldFixture
{
    UWorld* World = nullptr;
    AAIHeroesGameMode* Game = nullptr;
    AAIHeroesWorldNode* HQ = nullptr;
    AAIHeroesWorldNode* Forest = nullptr;
    AAIHeroesWorldNode* Outpost = nullptr;
    WorldFixture()
    {
        const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false)
            .CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
        World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
        if (!World) return;
        if (GEngine) GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        Game = World->SpawnActor<AAIHeroesGameMode>();
        if (!Game) return;
        Game->TravelRuleVersion = 2;
        const TCHAR* Names[] = {TEXT("HQ"), TEXT("FOREST"), TEXT("ORE"), TEXT("OUTPOST"), TEXT("RUINS")};
        const EAIHeroesNodeType Types[] = {EAIHeroesNodeType::Headquarters, EAIHeroesNodeType::ForestMine,
            EAIHeroesNodeType::OreMine, EAIHeroesNodeType::Outpost, EAIHeroesNodeType::Ruins};
        const FVector At[] = {FVector(-650,0,155), FVector(-100,-650,110), FVector(150,600,120),
            FVector(700,-300,140), FVector(750,650,165)};
        for (int32 I = 0; I < 5; ++I)
        {
            auto* Node = World->SpawnActor<AAIHeroesWorldNode>(At[I], FRotator::ZeroRotator);
            if (!Node) return;
            Node->Configure(Types[I], Names[I], {});
            Node->SetOwned(I < 4);
            if (I == 4) Node->Defenders.Add(FAIHeroesArmyStack(TEXT("Guardian"), 7, 24, 7, 4, 2));
            Game->Nodes.Add(Node);
            if (I == 0) HQ = Node;
            if (I == 1) Forest = Node;
            if (I == 3) Outpost = Node;
        }
        Game->PlayerArmy = {FAIHeroesArmyStack(TEXT("Militia"), 8, 12, 4, 2, 3),
            FAIHeroesArmyStack(TEXT("Scout"), 3, 8, 6, 1, 7)};
        Game->HeroMarker = World->SpawnActor<AStaticMeshActor>();
        if (Game->HeroMarker)
        {
            Game->HeroMarker->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
            Game->HeroMarker->SetActorLocation(At[0] + AAIHeroesTravelPresentation::MarkerOffset());
        }
        Game->SelectedNode = Outpost;
    }
    ~WorldFixture()
    {
        if (World) { World->DestroyWorld(false); if (GEngine) GEngine->DestroyWorldContext(World); }
    }
    bool Valid() const { return Game && HQ && Forest && Outpost && Game->HeroMarker && Game->Nodes.Num() == 5; }
    AAIHeroesTravelPresentation* View() const
    {
        for (TActorIterator<AAIHeroesTravelPresentation> It(World); It; ++It) return *It;
        return nullptr;
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTravelMotionSegmentsTest, "AIHeroes.TravelView.SegmentsAndFrameParity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTravelMotionSegmentsTest::RunTest(const FString& Parameters)
{
    const TArray<FVector> Points = {FVector(0,0,0), FVector(100,0,0), FVector(100,100,0)};
    FAIHeroesTravelMotion A, B;
    TestTrue(TEXT("Bounded path accepted"), A.Begin(Points));
    B.Begin(Points);
    const double Quarter = A.GetDuration() / 4.0;
    A.Advance(Quarter);
    TestTrue(TEXT("First edge, not a shortcut diagonal"), A.Position().Equals(FVector(50,0,0), 0.001));
    A.Advance(Quarter);
    TestTrue(TEXT("Exact bend is traversed"), A.Position().Equals(FVector(100,0,0), 0.001));
    for (int32 I = 0; I < 120; ++I) B.Advance(B.GetDuration() / 240.0);
    TestTrue(TEXT("Equal elapsed time at different frame counts gives same position"), A.Position().Equals(B.Position(), 0.001));
    A.Advance(Quarter);
    TestTrue(TEXT("Second edge"), A.Position().Equals(FVector(100,50,0), 0.001));
    A.Advance(1000000.0);
    TestFalse(TEXT("Large delta finishes without an unbounded loop"), A.IsPlaying());
    TestTrue(TEXT("Exact final position"), A.Position().Equals(Points.Last(), 0.001));
    TestTrue(TEXT("Zero-length intermediate edge is safe"), B.Begin({Points[0], Points[0], Points[2]}));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTravelMotionBoundsTest, "AIHeroes.TravelView.InvalidInputsAndBounds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTravelMotionBoundsTest::RunTest(const FString& Parameters)
{
    FAIHeroesTravelMotion M;
    const TArray<FVector> Valid = {FVector::ZeroVector, FVector(100,0,0)};
    M.Begin(Valid); M.Advance(0.1);
    const FVector Before = M.Position();
    TestFalse(TEXT("Negative elapsed rejected"), M.Advance(-1.0));
    TestFalse(TEXT("NaN elapsed rejected"), M.Advance(std::numeric_limits<double>::quiet_NaN()));
    TestFalse(TEXT("Infinity elapsed rejected"), M.Advance(std::numeric_limits<double>::infinity()));
    TestFalse(TEXT("Zero speed rejected"), M.Begin(Valid, 0.0));
    TestFalse(TEXT("Empty path rejected"), M.Begin({}));
    TestFalse(TEXT("Repeated points only rejected"), M.Begin({FVector::ZeroVector, FVector::ZeroVector}));
    TestFalse(TEXT("Oversized coordinates rejected"), M.Begin({FVector::ZeroVector, FVector(1.e12,0,0)}));
    TArray<FVector> TooMany; TooMany.Init(FVector::ZeroVector, FAIHeroesTravelMotion::MaxPoints + 1);
    TestFalse(TEXT("Point allocation is bounded"), M.Begin(TooMany));
    TestTrue(TEXT("Rejected input leaves current presentation unchanged"), M.Position().Equals(Before));
    M.Finish();
    TestFalse(TEXT("Skip is terminal"), M.IsPlaying());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTravelViewPreviewTest, "AIHeroes.TravelView.ReadOnlyPreviewCacheAndCollision",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTravelViewPreviewTest::RunTest(const FString& Parameters)
{
    TravelViewTest::WorldFixture F;
    if (!TestTrue(TEXT("Disposable world valid"), F.Valid())) return false;
    const FVector Before = F.Game->GetHeroStrategicLocation();
    F.Game->Tick(0.0f);
    auto* V = F.View();
    if (!TestNotNull(TEXT("Selection creates disposable view"), V)) return false;
    TestTrue(TEXT("A ready path has real scene instances"), V->GetPathInstanceCount() > 0);
    TestTrue(TEXT("Visual geometry cannot block clicks"), V->GetPathMesh()->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
    const int32 Revision = V->GetGeometryRevision(), FullCount = V->GetPathInstanceCount();
    for (int32 I = 0; I < 20; ++I) F.Game->Tick(0.1f);
    TestEqual(TEXT("Stable preview does not rebuild each frame"), V->GetGeometryRevision(), Revision);
    TestEqual(TEXT("Preview never spends moves"), F.Game->MovesLeft, 4);
    TestEqual(TEXT("Preview never spends resources"), F.Game->Gold, 240);
    TestTrue(TEXT("Preview never moves hero"), F.Game->GetHeroStrategicLocation().Equals(Before));
    F.Game->MovesLeft = 1; F.Game->Tick(0.0f);
    TestTrue(TEXT("Over-budget state is explicit"), V->GetDisplayedStatus() == EAIHeroesAdventurePathStatus::NeedsMoreMoves);
    TestTrue(TEXT("Over-budget route differs by geometry as well as color"), V->GetPathInstanceCount() < FullCount);
    F.Outpost->SetActorHiddenInGame(true); F.Game->Tick(0.0f);
    TestEqual(TEXT("Hidden target cannot leak a route"), V->GetPathInstanceCount(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTravelViewCommitTest, "AIHeroes.TravelView.CommitOnceAndNextCommand",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTravelViewCommitTest::RunTest(const FString& Parameters)
{
    TravelViewTest::WorldFixture F;
    if (!TestTrue(TEXT("Disposable world valid"), F.Valid())) return false;
    const FVector Origin = F.Game->GetHeroStrategicLocation();
    const int32 Cost = F.Game->GetSelectedMoveCost();
    F.Game->ConfirmSelectedNode();
    TestEqual(TEXT("Original approved price charged once"), F.Game->MovesLeft, 4 - Cost);
    TestTrue(TEXT("Logical destination committed before presentation"), F.Game->GetHeroStrategicLocation().Equals(F.Outpost->GetActorLocation()));
    TestTrue(TEXT("Presentation starts"), F.Game->IsStrategicTravelMoving());
    TestTrue(TEXT("Marker starts at origin"), F.Game->HeroMarker->GetActorLocation().Equals(Origin + AAIHeroesTravelPresentation::MarkerOffset()));
    F.Game->Tick(0.15f);
    TestFalse(TEXT("Marker advances visibly"), F.Game->HeroMarker->GetActorLocation().Equals(Origin + AAIHeroesTravelPresentation::MarkerOffset()));
    TestEqual(TEXT("Animation does not spend again"), F.Game->MovesLeft, 4 - Cost);
    F.Game->AskAdvisor();
    TestFalse(TEXT("A new command completes old view first"), F.Game->IsStrategicTravelMoving());
    TestTrue(TEXT("Skip lands at exact logical destination"), F.Game->HeroMarker->GetActorLocation().Equals(F.Game->GetHeroStrategicLocation() + AAIHeroesTravelPresentation::MarkerOffset()));
    TestEqual(TEXT("No fabricated battle"), F.Game->BattleJournal.Num(), 0);
    TestEqual(TEXT("No fabricated rewards"), F.Game->DataFragments, 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTravelViewLossTest, "AIHeroes.TravelView.VisualLossAndDrift",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTravelViewLossTest::RunTest(const FString& Parameters)
{
    TravelViewTest::WorldFixture F;
    if (!TestTrue(TEXT("Disposable world valid"), F.Valid())) return false;
    F.Game->ConfirmSelectedNode(); F.Game->Tick(0.15f);
    auto* V = F.View();
    if (!TestNotNull(TEXT("Animation view exists"), V)) return false;
    V->Destroy(); F.Game->Tick(0.15f);
    TestFalse(TEXT("Missing presenter never locks game"), F.Game->IsStrategicTravelMoving());
    TestEqual(TEXT("Missing presenter never charges a second time"), F.Game->MovesLeft, 1);
    TestTrue(TEXT("Missing presenter restores marker"), F.Game->HeroMarker->GetActorLocation().Equals(F.Game->GetHeroStrategicLocation() + AAIHeroesTravelPresentation::MarkerOffset()));
    F.Game->MovesLeft = 4; F.Game->SelectNode(F.HQ); F.Game->ConfirmSelectedNode();
    V = F.View();
    if (!TestNotNull(TEXT("Second presentation recreates view"), V)) return false;
    F.Game->HeroMarker->Destroy(); F.Game->Tick(0.1f);
    TestFalse(TEXT("Destroyed marker finishes safely"), V->IsMoving());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTravelViewModalTest, "AIHeroes.TravelView.ModalAndTeardown",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTravelViewModalTest::RunTest(const FString& Parameters)
{
    TravelViewTest::WorldFixture F;
    if (!TestTrue(TEXT("Disposable world valid"), F.Valid())) return false;
    F.Game->Tick(0.0f);
    auto* V = F.View();
    if (!TestNotNull(TEXT("Preview exists"), V)) return false;
    F.Game->ToggleAdvisorDossier(); F.Game->Tick(0.0f);
    TestEqual(TEXT("Read-only modal hides route visuals"), V->GetPathInstanceCount(), 0);
    F.Game->CloseAdvisorDossier(); F.Game->Tick(0.0f);
    TestTrue(TEXT("Closing modal restores current preview"), V->GetPathInstanceCount() > 0);
    TWeakObjectPtr<AAIHeroesTravelPresentation> WeakView(V);
    F.Game->Destroy();
    TestFalse(TEXT("Owned view cannot outlive pre-BeginPlay GameMode destruction"), WeakView.IsValid());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTravelViewSaveTest, "AIHeroes.TravelView.DurableDestinationNotVisualPosition",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTravelViewSaveTest::RunTest(const FString& Parameters)
{
    TravelViewTest::WorldFixture F;
    if (!TestTrue(TEXT("Disposable world valid"), F.Valid())) return false;
    F.Game->ConfirmSelectedNode(); F.Game->Tick(0.1f);
    UAIHeroesCampaignSave* S = NewObject<UAIHeroesCampaignSave>();
    S->TravelRuleVersion = 2; S->MovesLeft = F.Game->MovesLeft;
    S->PlayerArmy = F.Game->PlayerArmy;
    for (const AAIHeroesWorldNode* N : F.Game->Nodes)
    {
        FAIHeroesSavedNode Entry;
        Entry.NodeId = FName(*N->GetDisplayName()); Entry.Type = N->NodeType;
        Entry.bOwned = N->bOwned; Entry.Defenders = N->Defenders; S->NodeStates.Add(Entry);
        if (N->GetActorLocation().Equals(F.Game->GetHeroStrategicLocation(), 0.01)) S->HeroNodeId = Entry.NodeId;
    }
    FString Error;
    TestTrue(TEXT("Committed campaign is valid while visual is in transit"), FAIHeroesCampaignRules::Validate(*S, Error));
    TArray<uint8> Bytes;
    if (!TestTrue(TEXT("In-memory save succeeds without touching player files"), UGameplayStatics::SaveGameToMemory(S, Bytes))) return false;
    auto* Loaded = Cast<UAIHeroesCampaignSave>(UGameplayStatics::LoadGameFromMemory(Bytes));
    if (!TestNotNull(TEXT("In-memory save restores"), Loaded)) return false;
    TestEqual(TEXT("Save retains real destination, not intermediate marker"), Loaded->HeroNodeId, FName(TEXT("OUTPOST")));
    TestEqual(TEXT("Save retains one movement debit"), Loaded->MovesLeft, 1);
    F.Game->CycleSaveSlot();
    TestFalse(TEXT("Existing save-management authority gate finishes transit"), F.Game->IsStrategicTravelMoving());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTravelViewBlockedTest, "AIHeroes.TravelView.BlockedMoveHasNoPresentation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTravelViewBlockedTest::RunTest(const FString& Parameters)
{
    TravelViewTest::WorldFixture F;
    if (!TestTrue(TEXT("Disposable world valid"), F.Valid())) return false;
    F.Forest->SetOwned(false);
    F.Forest->Defenders = {FAIHeroesArmyStack(TEXT("Wolf"), 4, 9, 3, 1, 8)};
    F.Game->ConfirmSelectedNode(); F.Game->Tick(0.1f);
    TestFalse(TEXT("Blocked path never animates"), F.Game->IsStrategicTravelMoving());
    TestTrue(TEXT("Blocked path retains origin"), F.Game->GetHeroStrategicLocation().Equals(F.HQ->GetActorLocation()));
    TestEqual(TEXT("Blocked path retains moves"), F.Game->MovesLeft, 4);
    if (F.View()) TestEqual(TEXT("No imaginary path through guards"), F.View()->GetPathInstanceCount(), 0);
    F.Game->SelectNode(F.Forest); F.Game->MovesLeft = 0; F.Game->ConfirmSelectedNode();
    TestFalse(TEXT("Insufficient budget cannot start movement or combat"), F.Game->IsStrategicTravelMoving() || F.Game->IsLiveBattleActive());
    return true;
}
#endif
