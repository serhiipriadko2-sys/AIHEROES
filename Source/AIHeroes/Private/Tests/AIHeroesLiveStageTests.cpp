#include "AIHeroesLiveBattleStage.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AIHeroesGridBattle.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextRenderActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/AutomationTest.h"

namespace
{
struct FAIHeroesStageTestWorld
{
    UWorld* World = nullptr;

    FAIHeroesStageTestWorld()
    {
        const UWorld::InitializationValues Values = UWorld::InitializationValues()
            .AllowAudioPlayback(false).CreatePhysicsScene(false)
            .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
        World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
            true, ERHIFeatureLevel::Num, &Values);
        if (World && GEngine)
        {
            GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        }
    }

    ~FAIHeroesStageTestWorld()
    {
        if (World)
        {
            World->DestroyWorld(false);
            if (GEngine) GEngine->DestroyWorldContext(World);
        }
    }
};

TArray<FAIHeroesArmyStack> StageHeroes()
{
    return {FAIHeroesArmyStack(TEXT("Militia"), 8, 12, 4, 2, 3)};
}

TArray<FAIHeroesArmyStack> StageEnemies()
{
    return {FAIHeroesArmyStack(TEXT("Wolf"), 4, 9, 3, 1, 8)};
}

FAIHeroesGridAction StageWaitAction()
{
    FAIHeroesGridAction Action;
    Action.Kind = EAIHeroesGridActionKind::Wait;
    Action.Round = 1;
    Action.bPlayerAttacker = true;
    Action.AttackerStackIndex = 0;
    Action.AttackerId = FName(TEXT("Militia"));
    Action.FromCell = FIntPoint(0, 2);
    Action.ToCell = Action.FromCell;
    Action.Reason = TEXT("No legal action");
    return Action;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesLiveStagePauseTest,
    "AIHeroes.LiveBattle.StagePauseAndFinish",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesLiveStagePauseTest::RunTest(const FString& Parameters)
{
    FAIHeroesStageTestWorld Fixture;
    AAIHeroesLiveBattleStage* Stage = Fixture.World
        ? Fixture.World->SpawnActor<AAIHeroesLiveBattleStage>() : nullptr;
    if (!TestNotNull(TEXT("Transient test stage spawns"), Stage)) return false;
    if (!TestTrue(TEXT("Initial snapshot starts without calculating a battle"),
        Stage->Initialize(StageHeroes(), StageEnemies(),
            {FIntPoint(0, 2)}, {FIntPoint(7, 2)}))) return false;
    TestNotNull(TEXT("Separate perspective camera exists"), Stage->GetViewTarget());
    // Presentation v2 keeps floating tags compact to avoid covering figures.
    // Health and initiative are read from the exact roster used by HUD cards.
    bool bCompactGroupTag = false;
    bool bOldOverlappingDebugText = false;
    for (TActorIterator<ATextRenderActor> It(Fixture.World); It; ++It)
    {
        const FString Label = It->GetTextRender()->Text.ToString();
        bCompactGroupTag |= Label.Contains(TEXT("A1")) && Label.Contains(TEXT("x8"));
        bOldOverlappingDebugText |= Label.Contains(TEXT("INIT"));
    }
    TestTrue(TEXT("Floating label identifies group A1 and its real count"), bCompactGroupTag);
    TestFalse(TEXT("Old initiative debug paragraph no longer overlaps figures"), bOldOverlappingDebugText);
    const auto& VisibleRoster = Stage->GetPresentedArmy(true);
    if (TestTrue(TEXT("HUD presentation exposes allied roster"), VisibleRoster.Num() == 1))
    {
        TestEqual(TEXT("HUD reads actual initial initiative"), VisibleRoster[0].Initiative, 3);
        TestEqual(TEXT("HUD reads actual health"), VisibleRoster[0].TotalHealth, 96);
    }
    TestFalse(TEXT("Initialization has no future action to play"), Stage->IsPresenting());
    TestTrue(TEXT("An actual event can be presented"), Stage->PresentAction(StageWaitAction()));
    TestTrue(TEXT("Presentation has begun"), Stage->IsPresenting());
    Stage->SetPresentationPaused(true);
    Stage->Tick(20.f);
    TestTrue(TEXT("Paused clock does not finish its event"), Stage->IsPresenting());
    Stage->OrbitCamera(20.f, 120.f);
    Stage->SetPresentationPaused(false);
    Stage->Tick(2.f);
    TestFalse(TEXT("Unpaused presentation completes"), Stage->IsPresenting());

    TestTrue(TEXT("A later receipt starts normally"), Stage->PresentAction(StageWaitAction()));
    Stage->FinishPresentationImmediately();
    TestFalse(TEXT("Finish only ends current presentation"), Stage->IsPresenting());
    Stage->ShowResult(TEXT("VICTORY"));
    TestTrue(TEXT("Explicit final result is visible"), Stage->GetStatusText().Contains(TEXT("VICTORY")));
    TestFalse(TEXT("Final result refuses additional action receipts"),
        Stage->PresentAction(StageWaitAction()));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesLiveStageVisualLossTest,
    "AIHeroes.LiveBattle.StageMissingVisualDoesNotHang",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesLiveStageVisualLossTest::RunTest(const FString& Parameters)
{
    FAIHeroesStageTestWorld Fixture;
    AAIHeroesLiveBattleStage* Stage = Fixture.World
        ? Fixture.World->SpawnActor<AAIHeroesLiveBattleStage>() : nullptr;
    if (!TestNotNull(TEXT("Transient test stage spawns"), Stage)) return false;
    if (!TestTrue(TEXT("Stage initializes"), Stage->Initialize(StageHeroes(), StageEnemies(),
        {FIntPoint(0, 2)}, {FIntPoint(7, 2)}))) return false;

    // Only actors in this disposable test world are touched, never a player world.
    for (TActorIterator<AStaticMeshActor> It(Fixture.World); It; ++It) It->Destroy();
    TestTrue(TEXT("Missing figurines retain a text-only receipt"),
        Stage->PresentAction(StageWaitAction()));
    Stage->Tick(2.f);
    TestFalse(TEXT("Missing figurines cannot leave presentation waiting forever"),
        Stage->IsPresenting());
    TestTrue(TEXT("Text still identifies actual action"),
        Stage->GetStatusText().Contains(TEXT("Militia")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesLiveStageInvalidCellsTest,
    "AIHeroes.LiveBattle.StageRejectsInvalidSnapshot",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesLiveStageInvalidCellsTest::RunTest(const FString& Parameters)
{
    FAIHeroesStageTestWorld Fixture;
    AAIHeroesLiveBattleStage* Stage = Fixture.World
        ? Fixture.World->SpawnActor<AAIHeroesLiveBattleStage>() : nullptr;
    if (!TestNotNull(TEXT("Transient test stage spawns"), Stage)) return false;
    TestFalse(TEXT("Mismatched cell count is rejected before starting"),
        Stage->Initialize(StageHeroes(), StageEnemies(), {}, {FIntPoint(7, 2)}));
    TestFalse(TEXT("Failed initialization is not presenting"), Stage->IsPresenting());
    TestFalse(TEXT("Outside-grid cells are rejected"),
        Stage->Initialize(StageHeroes(), StageEnemies(),
            {FIntPoint(-1, 2)}, {FIntPoint(7, 2)}));
    TestFalse(TEXT("Overlapping living stacks are rejected"),
        Stage->Initialize(StageHeroes(), StageEnemies(),
            {FIntPoint(0, 2)}, {FIntPoint(0, 2)}));
    TArray<FAIHeroesArmyStack> ThreeStacks = StageHeroes();
    ThreeStacks.Add(FAIHeroesArmyStack(TEXT("Scout"), 3, 8, 6, 1, 7));
    ThreeStacks.Add(FAIHeroesArmyStack(TEXT("Militia"), 4, 12, 4, 2, 3));
    TestFalse(TEXT("Presentation contract rejects more than two stacks per side"),
        Stage->Initialize(ThreeStacks, StageEnemies(),
            {FIntPoint(0, 1), FIntPoint(0, 2), FIntPoint(0, 3)}, {FIntPoint(7, 2)}));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesLiveStageReceiptTest,
    "AIHeroes.LiveBattle.StageReceiptAndSpeed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesLiveStageReceiptTest::RunTest(const FString& Parameters)
{
    FAIHeroesStageTestWorld Fixture;
    AAIHeroesLiveBattleStage* Stage = Fixture.World
        ? Fixture.World->SpawnActor<AAIHeroesLiveBattleStage>() : nullptr;
    if (!TestNotNull(TEXT("Transient test stage spawns"), Stage)) return false;
    if (!TestTrue(TEXT("Stage initializes"), Stage->Initialize(StageHeroes(), StageEnemies(),
        {FIntPoint(0, 2)}, {FIntPoint(7, 2)}))) return false;

    FAIHeroesGridAction Move = StageWaitAction();
    Move.Kind = EAIHeroesGridActionKind::Move;
    Move.Path = {FIntPoint(1, 2), FIntPoint(2, 2)};
    Move.ToCell = Move.Path.Last();
    TestTrue(TEXT("Only an existing one-action receipt starts movement"), Stage->PresentAction(Move));
    TestFalse(TEXT("Second event cannot overwrite an active presentation"), Stage->PresentAction(Move));
    Stage->SetPresentationSpeed(1.f);
    Stage->Tick(0.15f);
    TestTrue(TEXT("Normal speed preserves the intermediate move"), Stage->IsPresenting());
    Stage->SetPresentationSpeed(4.f);
    Stage->Tick(0.1f);
    TestFalse(TEXT("4x finishes the same event sooner"), Stage->IsPresenting());
    TestFalse(TEXT("An old from-cell cannot move the view backward"), Stage->PresentAction(Move));

    FAIHeroesGridAction Wait = StageWaitAction();
    Wait.FromCell = Move.ToCell;
    Wait.ToCell = Move.ToCell;
    TestTrue(TEXT("A new receipt observes the actual final cell"), Stage->PresentAction(Wait));
    Stage->FinishPresentationImmediately();
    AActor* Camera = Stage->GetViewTarget();
    if (TestNotNull(TEXT("Camera remains independently available"), Camera))
    {
        const FVector Before = Camera->GetActorLocation();
        Stage->SetPresentationPaused(true);
        Stage->OrbitCamera(15.f, 0.f);
        TestFalse(TEXT("Camera can orbit independently of pause"),
            Camera->GetActorLocation().Equals(Before));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesLiveStageCleanupTest,
    "AIHeroes.LiveBattle.StageCleanupBeforeBeginPlay",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesLiveStageCleanupTest::RunTest(const FString& Parameters)
{
    FAIHeroesStageTestWorld Fixture;
    AAIHeroesLiveBattleStage* Stage = Fixture.World
        ? Fixture.World->SpawnActor<AAIHeroesLiveBattleStage>() : nullptr;
    if (!TestNotNull(TEXT("Transient test stage spawns"), Stage)) return false;
    TestFalse(TEXT("Fixture deliberately has not begun gameplay"), Stage->HasActorBegunPlay());
    if (!TestTrue(TEXT("Stage initializes"), Stage->Initialize(StageHeroes(), StageEnemies(),
        {FIntPoint(0, 2)}, {FIntPoint(7, 2)}))) return false;
    TArray<TWeakObjectPtr<AActor>> OwnedActors;
    for (TActorIterator<AActor> It(Fixture.World); It; ++It)
    {
        if (It->GetOwner() == Stage) OwnedActors.Add(*It);
    }
    TestTrue(TEXT("Fixture owns camera and figures"), OwnedActors.Num() > 0);
    TestTrue(TEXT("Stage can be destroyed before BeginPlay"), Stage->Destroy());
    for (const TWeakObjectPtr<AActor>& Owned : OwnedActors)
    {
        TestFalse(TEXT("Stage-owned actors cannot outlive pre-BeginPlay destruction"), Owned.IsValid());
    }
    return true;
}
#endif
