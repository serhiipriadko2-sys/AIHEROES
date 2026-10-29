#include "AIHeroesLiveBattleStage.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextRenderActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/AutomationTest.h"

namespace BattleReadabilityTestDetail
{
struct FWorld
{
    UWorld* World = nullptr;
    FWorld()
    {
        const UWorld::InitializationValues Values = UWorld::InitializationValues()
            .AllowAudioPlayback(false).CreatePhysicsScene(false)
            .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
        World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
            true, ERHIFeatureLevel::Num, &Values);
        if (World && GEngine) GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    }
    ~FWorld()
    {
        if (World)
        {
            World->DestroyWorld(false);
            if (GEngine) GEngine->DestroyWorldContext(World);
        }
    }
    AAIHeroesLiveBattleStage* SpawnStage() const
    {
        return World ? World->SpawnActor<AAIHeroesLiveBattleStage>() : nullptr;
    }
};

ATextRenderActor* LabelContaining(AAIHeroesLiveBattleStage* Stage, const TCHAR* Text)
{
    for (TActorIterator<ATextRenderActor> It(Stage->GetWorld()); It; ++It)
        if (It->GetOwner() == Stage && It->GetTextRender()->Text.ToString().Contains(Text)) return *It;
    return nullptr;
}

int32 VisibleFigures(AAIHeroesLiveBattleStage* Stage)
{
    int32 Count = 0;
    for (TActorIterator<AStaticMeshActor> It(Stage->GetWorld()); It; ++It)
        if (It->GetOwner() == Stage && !It->IsHidden()) ++Count;
    return Count;
}

bool InitializePair(AAIHeroesLiveBattleStage* Stage, int32 WolfCount = 1)
{
    return Stage->Initialize(
        {FAIHeroesArmyStack(TEXT("Militia"), 1, 12, 9, 2, 3)},
        {FAIHeroesArmyStack(TEXT("Wolf"), WolfCount, 9, 3, 1, 8)},
        {FIntPoint(0, 2)}, {FIntPoint(1, 2)});
}

// Presentation receipts only; the scene does not select attacks or resolve damage.
FAIHeroesGridAction AttackReceipt(int32 Damage, int32 Before, int32 After)
{
    FAIHeroesGridAction Action;
    Action.Kind = EAIHeroesGridActionKind::Attack;
    Action.Round = 1;
    Action.bPlayerAttacker = true;
    Action.AttackerStackIndex = 0;
    Action.AttackerId = FName(TEXT("Militia"));
    Action.TargetStackIndex = 0;
    Action.TargetId = FName(TEXT("Wolf"));
    Action.FromCell = FIntPoint(0, 2);
    Action.ToCell = FIntPoint(1, 2);
    Action.Damage = Damage;
    Action.TargetHealthBefore = Before;
    Action.TargetHealthAfter = After;
    Action.Reason = TEXT("readability reason appears once");
    return Action;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesBattleStableLabelIdentityTest,
    "AIHeroes.LiveBattle.Readability.StableGroupIdentity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesBattleStableLabelIdentityTest::RunTest(const FString& Parameters)
{
    using namespace BattleReadabilityTestDetail;
    FWorld Fixture;
    AAIHeroesLiveBattleStage* Stage = Fixture.SpawnStage();
    if (!TestNotNull(TEXT("Disposable stage exists"), Stage)) return false;
    if (!TestTrue(TEXT("Two groups on each side initialize"), Stage->Initialize(
        {FAIHeroesArmyStack(TEXT("Militia"), 8, 12, 4, 2, 3),
         FAIHeroesArmyStack(TEXT("Scout"), 3, 8, 6, 1, 7)},
        {FAIHeroesArmyStack(TEXT("Raider"), 6, 11, 5, 2, 5),
         FAIHeroesArmyStack(TEXT("Raider"), 4, 11, 5, 2, 5)},
        {FIntPoint(0, 2), FIntPoint(0, 3)}, {FIntPoint(7, 2), FIntPoint(7, 3)}))) return false;
    const TCHAR* Ids[] = {TEXT("HERO[0]"), TEXT("HERO[1]"), TEXT("NPC[0]"), TEXT("NPC[1]")};
    for (const TCHAR* Id : Ids)
    {
        ATextRenderActor* Label = LabelContaining(Stage, Id);
        if (TestNotNull(FString::Printf(TEXT("Receipt identity %s is visible"), Id), Label))
        {
            TestFalse(TEXT("Living group's label is not hidden"), Label->IsHidden());
            TestTrue(TEXT("Initiative remains in the label"), Label->GetTextRender()->Text.ToString().Contains(TEXT("INIT")));
        }
    }
    TestEqual(TEXT("Both equal-name enemy figures are retained"), VisibleFigures(Stage), 4);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesBattleDeadLabelVisibilityTest,
    "AIHeroes.LiveBattle.Readability.DeathHidesLabelAndFigure",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesBattleDeadLabelVisibilityTest::RunTest(const FString& Parameters)
{
    using namespace BattleReadabilityTestDetail;
    // Initial death, ordinary animation completion, and fast completion all agree.
    for (int32 Variant = 0; Variant < 3; ++Variant)
    {
        FWorld Fixture;
        AAIHeroesLiveBattleStage* Stage = Fixture.SpawnStage();
        if (!TestNotNull(TEXT("Disposable stage exists"), Stage)) return false;
        if (!TestTrue(TEXT("Stage accepts explicit initial snapshot"), InitializePair(Stage, Variant == 0 ? 0 : 1))) return false;
        ATextRenderActor* WolfLabel = LabelContaining(Stage, TEXT("Wolf"));
        if (!TestNotNull(TEXT("Target's label actor exists for lifecycle checks"), WolfLabel)) return false;
        if (Variant > 0)
        {
            TestFalse(TEXT("Living target starts with a visible label"), WolfLabel->IsHidden());
            if (!TestTrue(TEXT("Only a supplied receipt starts the hit"), Stage->PresentAction(AttackReceipt(9, 9, 0)))) return false;
            if (Variant == 1) Stage->Tick(2.f);
            else Stage->FinishPresentationImmediately();
        }
        TestTrue(TEXT("Zero-HP label never floats over a free cell"), WolfLabel->IsHidden());
        TestEqual(TEXT("Only the living body remains visible"), VisibleFigures(Stage), 1);
        TestTrue(TEXT("Read-only roster still reports the actual zero HP"), Stage->GetArmyStatus(false).Contains(TEXT("0 units / 0 HP")));
        Stage->ShowResult(TEXT("VICTORY"));
        TestTrue(TEXT("Result cannot reveal a dead label again"), WolfLabel->IsHidden());
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesBattleBillboardTest,
    "AIHeroes.LiveBattle.Readability.FullBillboardDuringPause",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesBattleBillboardTest::RunTest(const FString& Parameters)
{
    using namespace BattleReadabilityTestDetail;
    FWorld Fixture;
    AAIHeroesLiveBattleStage* Stage = Fixture.SpawnStage();
    if (!TestNotNull(TEXT("Disposable stage exists"), Stage)) return false;
    if (!TestTrue(TEXT("Stage initializes"), InitializePair(Stage, 4))) return false;
    if (!TestTrue(TEXT("Supplied nonlethal hit starts"), Stage->PresentAction(AttackReceipt(5, 36, 31)))) return false;
    Stage->Tick(.4f); // Hit commits at 45% of the 0.85-second presentation.
    Stage->SetPresentationPaused(true);
    const FString BeforeOrbit = Stage->GetArmyStatus(false);
    bool bSawDamage = false;
    for (const FVector2D& Orbit : {FVector2D(0.f, 0.f), FVector2D(95.f, -500.f), FVector2D(-190.f, 2000.f)})
    {
        Stage->OrbitCamera(Orbit.X, Orbit.Y);
        Stage->Tick(2.f);
        AActor* Camera = Stage->GetViewTarget();
        if (!TestNotNull(TEXT("Perspective camera remains available during pause"), Camera)) return false;
        for (TActorIterator<ATextRenderActor> It(Fixture.World); It; ++It)
        {
            if (It->GetOwner() != Stage || It->IsHidden()) continue;
            const FVector Direction = (Camera->GetActorLocation() - It->GetActorLocation()).GetSafeNormal();
            TestTrue(TEXT("Text plane normal faces camera in pitch and yaw"),
                FVector::DotProduct(It->GetActorForwardVector(), Direction) > .999f);
            bSawDamage |= It->GetTextRender()->Text.ToString() == TEXT("-5");
        }
        TestEqual(TEXT("Orbit and paused ticks do not alter presented HP"), Stage->GetArmyStatus(false), BeforeOrbit);
        TestTrue(TEXT("Pause keeps the same event active"), Stage->IsPresenting());
    }
    TestTrue(TEXT("Alignment also covered the visible damage popup"), bSawDamage);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesBattleReasonAndCleanupTest,
    "AIHeroes.LiveBattle.Readability.ReasonOnceAndCleanup",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesBattleReasonAndCleanupTest::RunTest(const FString& Parameters)
{
    using namespace BattleReadabilityTestDetail;
    FWorld Fixture;
    AAIHeroesLiveBattleStage* Stage = Fixture.SpawnStage();
    if (!TestNotNull(TEXT("Disposable stage exists"), Stage)) return false;
    if (!TestTrue(TEXT("Stage initializes"), InitializePair(Stage))) return false;
    const FAIHeroesGridAction Action = AttackReceipt(5, 9, 4);
    if (!TestTrue(TEXT("Supplied hit is accepted"), Stage->PresentAction(Action))) return false;
    TestEqual(TEXT("Status uses one authoritative log line without appending its reason twice"),
        Stage->GetStatusText(), Action.ToLogLine());
    TArray<TWeakObjectPtr<AActor>> OwnedActors;
    for (TActorIterator<AActor> It(Fixture.World); It; ++It)
        if (It->GetOwner() == Stage) OwnedActors.Add(*It);
    TestTrue(TEXT("Fixture owns camera, figures and labels"), OwnedActors.Num() >= 6);
    TestTrue(TEXT("Destroying stage during an action succeeds"), Stage->Destroy());
    for (const auto& Actor : OwnedActors)
        TestFalse(TEXT("Owned labels/figures/camera do not leak"), Actor.IsValid());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesTacticalCameraBlurTest,
    "AIHeroes.LiveBattle.Readability.TacticalCameraHasNoMotionBlur",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesTacticalCameraBlurTest::RunTest(const FString& Parameters)
{
    using namespace BattleReadabilityTestDetail;
    FWorld Fixture;
    AAIHeroesLiveBattleStage* Stage = Fixture.SpawnStage();
    if (!TestNotNull(TEXT("Disposable tactical stage exists"), Stage)) return false;
    if (!TestTrue(TEXT("Stage initializes its own camera"), InitializePair(Stage))) return false;
    ACameraActor* Camera = Cast<ACameraActor>(Stage->GetViewTarget());
    if (!TestNotNull(TEXT("Tactical view is a camera actor"), Camera)) return false;
    const UCameraComponent* Component = Camera->GetCameraComponent();
    TestTrue(TEXT("Tactical camera explicitly overrides motion blur"),
        static_cast<bool>(Component->PostProcessSettings.bOverride_MotionBlurAmount));
    TestEqual(TEXT("Tactical motion blur amount is zero"),
        Component->PostProcessSettings.MotionBlurAmount, 0.f);
    TestTrue(TEXT("Camera-local post processing has positive blend weight"),
        Component->PostProcessBlendWeight > 0.f);
    Stage->SetPresentationPaused(true);
    Stage->OrbitCamera(70.f, -300.f);
    TestEqual(TEXT("Paused orbit keeps the readability setting"),
        Component->PostProcessSettings.MotionBlurAmount, 0.f);
    ACameraActor* UnrelatedCamera = Fixture.World->SpawnActor<ACameraActor>();
    if (!TestNotNull(TEXT("Independent camera exists"), UnrelatedCamera)) return false;
    TestFalse(TEXT("No global/default camera blur override was changed"),
        static_cast<bool>(UnrelatedCamera->GetCameraComponent()->PostProcessSettings.bOverride_MotionBlurAmount));
    return true;
}
#endif
