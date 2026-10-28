#include "AIHeroesLiveBattleStage.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextRenderActor.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/UObjectGlobals.h"

namespace
{
FActorSpawnParameters StageSpawnParameters(AActor* Owner)
{
    FActorSpawnParameters Parameters;
    Parameters.Owner = Owner;
    Parameters.ObjectFlags |= RF_Transient;
    Parameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    return Parameters;
}

FVector FigureScale(const FAIHeroesArmyStack& Stack)
{
    return FVector(0.8f, 0.8f, FMath::Clamp(0.75f + Stack.Count() * 0.025f, 0.8f, 1.3f));
}
}

AAIHeroesLiveBattleStage::AAIHeroesLiveBattleStage()
{
    PrimaryActorTick.bCanEverTick = true;
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("LiveBattleRoot"));
    RootComponent = SceneRoot;

    ArenaBase = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArenaBase"));
    ArenaBase->SetupAttachment(SceneRoot);
    ArenaBase->SetRelativeLocation(FVector(0.f, 0.f, -25.f));
    ArenaBase->SetRelativeScale3D(FVector(15.3f, 11.7f, 0.4f));
    ArenaBase->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ArenaBase->SetCastShadow(false);

    LightTiles = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("LightGridCells"));
    DarkTiles = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("DarkGridCells"));
    PathMarkers = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ActualActionPath"));
    for (UInstancedStaticMeshComponent* Tiles : {LightTiles.Get(), DarkTiles.Get(), PathMarkers.Get()})
    {
        Tiles->SetupAttachment(SceneRoot);
        Tiles->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Tiles->SetCastShadow(false);
    }

    FillLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("ArenaFill"));
    FillLight->SetupAttachment(SceneRoot);
    FillLight->SetRelativeLocation(FVector(0.f, 0.f, 950.f));
    FillLight->SetIntensity(30000.f);
    FillLight->SetAttenuationRadius(2700.f);
    FillLight->SetCastShadows(false);
}

bool AAIHeroesLiveBattleStage::ValidCell(const FIntPoint& Cell)
{
    return Cell.X >= 0 && Cell.X < FAIHeroesGridBattle::Width &&
        Cell.Y >= 0 && Cell.Y < FAIHeroesGridBattle::Height;
}

FVector AAIHeroesLiveBattleStage::CellLocation(const FIntPoint& Cell, float Height) const
{
    return Center + FVector((Cell.X - 3.5f) * CellSize,
        (Cell.Y - 2.5f) * CellSize, Height);
}

bool AAIHeroesLiveBattleStage::Initialize(
    const TArray<FAIHeroesArmyStack>& PlayerArmy,
    const TArray<FAIHeroesArmyStack>& EnemyArmy)
{
    FAIHeroesGridSession Initial;
    FString Error;
    // Begin ONLY deploys initial state. It does not calculate any future action.
    if (!FAIHeroesGridBattle::Begin(PlayerArmy, EnemyArmy, 0, Initial, Error)) return false;
    return Initialize(PlayerArmy, EnemyArmy, Initial.PlayerCells, Initial.EnemyCells);
}

bool AAIHeroesLiveBattleStage::Initialize(
    const TArray<FAIHeroesArmyStack>& PlayerArmy,
    const TArray<FAIHeroesArmyStack>& EnemyArmy,
    const TArray<FIntPoint>& PlayerCells,
    const TArray<FIntPoint>& EnemyCells)
{
    if (bInitialized || !GetWorld() || PlayerArmy.Num() != PlayerCells.Num() ||
        EnemyArmy.Num() != EnemyCells.Num() || PlayerArmy.Num() > 2 || EnemyArmy.Num() > 2)
    {
        Status = TEXT("Live battle view: invalid initial snapshot.");
        return false;
    }
    TSet<FIntPoint> Occupied;
    auto ValidArmy = [&Occupied](const TArray<FAIHeroesArmyStack>& Army,
                                const TArray<FIntPoint>& Cells)
    {
        for (int32 Index = 0; Index < Army.Num(); ++Index)
        {
            if (Army[Index].UnitHealth <= 0 || Army[Index].TotalHealth < 0) return false;
            if (!Army[Index].IsAlive()) continue;
            if (!ValidCell(Cells[Index]) || Occupied.Contains(Cells[Index])) return false;
            Occupied.Add(Cells[Index]);
        }
        return true;
    };
    if (!ValidArmy(PlayerArmy, PlayerCells) || !ValidArmy(EnemyArmy, EnemyCells))
    {
        Status = TEXT("Live battle view: invalid or overlapping initial cells.");
        return false;
    }

    CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    SphereMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    CylinderMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    ConeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cone.Cone"));
    if (!CubeMesh)
    {
        Status = TEXT("Live battle view: arena mesh unavailable.");
        return false;
    }

    // Default arena is outside the strategy board; the caller may spawn elsewhere.
    Center = GetActorLocation().IsNearlyZero() ? FVector(6500.f, 0.f, 200.f) : GetActorLocation();
    SetActorLocation(Center);
    BattleCamera = GetWorld()->SpawnActor<ACameraActor>(
        ACameraActor::StaticClass(), Center, FRotator::ZeroRotator, StageSpawnParameters(this));
    if (!IsValid(BattleCamera.Get()))
    {
        Status = TEXT("Live battle view: perspective camera unavailable.");
        return false;
    }
    SpawnedActors.Add(BattleCamera.Get());
    BattleCamera->GetCameraComponent()->SetProjectionMode(ECameraProjectionMode::Perspective);
    BattleCamera->GetCameraComponent()->SetFieldOfView(58.f);
    BattleCamera->GetCameraComponent()->bConstrainAspectRatio = false;
    UpdateCamera();

    UMaterialInterface* BaseMaterial = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    auto MakeMaterial = [this, BaseMaterial](const FLinearColor& Color)
    {
        UMaterialInstanceDynamic* Material = BaseMaterial
            ? UMaterialInstanceDynamic::Create(BaseMaterial, this) : nullptr;
        if (Material)
        {
            Material->SetVectorParameterValue(TEXT("Color"), Color);
            BoardMaterials.Add(Material);
        }
        return Material;
    };
    PlayerMaterial = MakeMaterial(FLinearColor(0.035f, 0.52f, 0.95f));
    EnemyMaterial = MakeMaterial(FLinearColor(0.95f, 0.16f, 0.035f));
    DamageMaterial = MakeMaterial(FLinearColor(1.f, 0.9f, 0.08f));
    ArenaBase->SetStaticMesh(CubeMesh);
    ArenaBase->SetMaterial(0, MakeMaterial(FLinearColor(0.035f, 0.05f, 0.08f)));
    LightTiles->SetStaticMesh(CubeMesh);
    DarkTiles->SetStaticMesh(CubeMesh);
    PathMarkers->SetStaticMesh(CubeMesh);
    LightTiles->SetMaterial(0, MakeMaterial(FLinearColor(0.21f, 0.29f, 0.34f)));
    DarkTiles->SetMaterial(0, MakeMaterial(FLinearColor(0.095f, 0.14f, 0.19f)));
    PathMarkers->SetMaterial(0, MakeMaterial(FLinearColor(0.1f, 0.85f, 0.75f)));
    for (int32 X = 0; X < FAIHeroesGridBattle::Width; ++X)
    {
        for (int32 Y = 0; Y < FAIHeroesGridBattle::Height; ++Y)
        {
            UInstancedStaticMeshComponent* Tiles = (X + Y) % 2 ? LightTiles.Get() : DarkTiles.Get();
            Tiles->AddInstance(FTransform(FQuat::Identity,
                CellLocation(FIntPoint(X, Y), 0.f) - Center, FVector(1.73f, 1.73f, 0.07f)));
        }
    }
    PlayerState = PlayerArmy;
    EnemyState = EnemyArmy;
    PlayerPositions = PlayerCells;
    EnemyPositions = EnemyCells;
    for (int32 Index = 0; Index < PlayerState.Num(); ++Index) SpawnFigure(true, Index);
    for (int32 Index = 0; Index < EnemyState.Num(); ++Index) SpawnFigure(false, Index);
    DamageLabel = GetWorld()->SpawnActor<ATextRenderActor>(
        ATextRenderActor::StaticClass(), Center, FRotator::ZeroRotator, StageSpawnParameters(this));
    if (IsValid(DamageLabel.Get()))
    {
        SpawnedActors.Add(DamageLabel.Get());
        DamageLabel->GetTextRender()->SetWorldSize(40.f);
        DamageLabel->GetTextRender()->SetHorizontalAlignment(EHTA_Center);
        DamageLabel->GetTextRender()->SetTextRenderColor(FColor(255, 230, 80));
        DamageLabel->GetTextRender()->SetCastShadow(false);
        DamageLabel->SetActorHiddenInGame(true);
    }
    bInitialized = true;
    Status = TEXT("LIVE GRID BATTLE | Waiting for the next actual AI action.");
    FaceLabelsToCamera();
    return true;
}

AStaticMeshActor* AAIHeroesLiveBattleStage::Figure(bool bPlayer, int32 Index) const
{
    const TArray<TObjectPtr<AStaticMeshActor>>& Figures = bPlayer ? PlayerFigures : EnemyFigures;
    return Figures.IsValidIndex(Index) && IsValid(Figures[Index].Get()) ? Figures[Index].Get() : nullptr;
}

void AAIHeroesLiveBattleStage::SpawnFigure(bool bPlayer, int32 Index)
{
    const TArray<FAIHeroesArmyStack>& Army = bPlayer ? PlayerState : EnemyState;
    const TArray<FIntPoint>& Cells = bPlayer ? PlayerPositions : EnemyPositions;
    TArray<TObjectPtr<AStaticMeshActor>>& Figures = bPlayer ? PlayerFigures : EnemyFigures;
    TArray<TObjectPtr<ATextRenderActor>>& Labels = bPlayer ? PlayerLabels : EnemyLabels;
    AStaticMeshActor* Body = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(),
        CellLocation(Cells[Index]), FRotator::ZeroRotator, StageSpawnParameters(this));
    Figures.Add(Body);
    if (Body)
    {
        SpawnedActors.Add(Body);
        UStaticMeshComponent* Mesh = Body->GetStaticMeshComponent();
        Mesh->SetMobility(EComponentMobility::Movable);
        UStaticMesh* Shape = CubeMesh;
        if (Army[Index].UnitId == FName(TEXT("Scout")) || Army[Index].UnitId == FName(TEXT("Wolf")))
            Shape = SphereMesh ? SphereMesh.Get() : CubeMesh.Get();
        else if (Army[Index].UnitId == FName(TEXT("Militia")) || Army[Index].UnitId == FName(TEXT("Guardian")))
            Shape = CylinderMesh ? CylinderMesh.Get() : CubeMesh.Get();
        else if (Army[Index].UnitId == FName(TEXT("Raider")))
            Shape = ConeMesh ? ConeMesh.Get() : CubeMesh.Get();
        Mesh->SetStaticMesh(Shape);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetCastShadow(false);
        if (UMaterialInstanceDynamic* Material = bPlayer ? PlayerMaterial.Get() : EnemyMaterial.Get())
            Mesh->SetMaterial(0, Material);
    }
    ATextRenderActor* Label = GetWorld()->SpawnActor<ATextRenderActor>(ATextRenderActor::StaticClass(),
        CellLocation(Cells[Index], 165.f), FRotator::ZeroRotator, StageSpawnParameters(this));
    Labels.Add(Label);
    if (Label)
    {
        SpawnedActors.Add(Label);
        UTextRenderComponent* Text = Label->GetTextRender();
        Text->SetWorldSize(24.f);
        Text->SetHorizontalAlignment(EHTA_Center);
        Text->SetTextRenderColor(bPlayer ? FColor(90, 220, 255) : FColor(255, 160, 100));
        Text->SetCastShadow(false);
    }
    RefreshFigure(bPlayer, Index);
}

void AAIHeroesLiveBattleStage::RefreshFigure(bool bPlayer, int32 Index)
{
    const TArray<FAIHeroesArmyStack>& Army = bPlayer ? PlayerState : EnemyState;
    const TArray<FIntPoint>& Cells = bPlayer ? PlayerPositions : EnemyPositions;
    const TArray<TObjectPtr<ATextRenderActor>>& Labels = bPlayer ? PlayerLabels : EnemyLabels;
    if (!Army.IsValidIndex(Index) || !Cells.IsValidIndex(Index)) return;
    const FAIHeroesArmyStack& Stack = Army[Index];
    if (AStaticMeshActor* Body = Figure(bPlayer, Index))
    {
        Body->SetActorLocation(CellLocation(Cells[Index]));
        Body->SetActorScale3D(FigureScale(Stack));
        Body->SetActorHiddenInGame(!Stack.IsAlive());
        if (UMaterialInstanceDynamic* Material = bPlayer ? PlayerMaterial.Get() : EnemyMaterial.Get())
            Body->GetStaticMeshComponent()->SetMaterial(0, Material);
    }
    if (Labels.IsValidIndex(Index) && IsValid(Labels[Index].Get()))
    {
        Labels[Index]->SetActorLocation(CellLocation(Cells[Index], 165.f));
        Labels[Index]->GetTextRender()->SetText(FText::FromString(FString::Printf(
            TEXT("%s %d\nHP %d | INIT %d"), *Stack.UnitId.ToString(), Stack.Count(),
            Stack.TotalHealth, Stack.Initiative)));
    }
}

bool AAIHeroesLiveBattleStage::ValidateReceipt(const FAIHeroesGridAction& Action) const
{
    const TArray<FAIHeroesArmyStack>& Army = Action.bPlayerAttacker ? PlayerState : EnemyState;
    const TArray<FIntPoint>& Cells = Action.bPlayerAttacker ? PlayerPositions : EnemyPositions;
    if (Action.Round <= 0 || !Army.IsValidIndex(Action.AttackerStackIndex) ||
        !Cells.IsValidIndex(Action.AttackerStackIndex) ||
        !Army[Action.AttackerStackIndex].IsAlive() ||
        Army[Action.AttackerStackIndex].UnitId != Action.AttackerId ||
        Cells[Action.AttackerStackIndex] != Action.FromCell || !ValidCell(Action.ToCell)) return false;

    switch (Action.Kind)
    {
        case EAIHeroesGridActionKind::Move:
        {
            if (Action.Path.IsEmpty() || Action.Path.Num() > 48 || Action.Path.Last() != Action.ToCell)
                return false;
            FIntPoint Previous = Action.FromCell;
            for (const FIntPoint& Cell : Action.Path)
            {
                if (!ValidCell(Cell) || FMath::Abs(Cell.X - Previous.X) + FMath::Abs(Cell.Y - Previous.Y) != 1)
                    return false;
                Previous = Cell;
            }
            return true;
        }
        case EAIHeroesGridActionKind::Attack:
        {
            const TArray<FAIHeroesArmyStack>& Targets = Action.bPlayerAttacker ? EnemyState : PlayerState;
            const TArray<FIntPoint>& TargetCells = Action.bPlayerAttacker ? EnemyPositions : PlayerPositions;
            return Targets.IsValidIndex(Action.TargetStackIndex) &&
                TargetCells.IsValidIndex(Action.TargetStackIndex) &&
                Targets[Action.TargetStackIndex].IsAlive() &&
                Targets[Action.TargetStackIndex].UnitId == Action.TargetId &&
                TargetCells[Action.TargetStackIndex] == Action.ToCell && Action.Damage > 0 &&
                Action.TargetHealthBefore == Targets[Action.TargetStackIndex].TotalHealth &&
                Action.TargetHealthAfter == FMath::Max(0, Action.TargetHealthBefore - Action.Damage);
        }
        case EAIHeroesGridActionKind::Wait:
            return Action.FromCell == Action.ToCell;
        default:
            return false;
    }
}

bool AAIHeroesLiveBattleStage::PresentAction(const FAIHeroesGridAction& Action)
{
    if (!bInitialized || bResultShown || bPresenting || !ValidateReceipt(Action)) return false;
    CurrentAction = Action;
    EventElapsed = 0.f;
    EventDuration = Action.Kind == EAIHeroesGridActionKind::Move
        ? FMath::Clamp(Action.Path.Num() * 0.22f, 0.45f, 1.35f)
        : Action.Kind == EAIHeroesGridActionKind::Attack ? 0.85f : 0.4f;
    bEventCommitted = false;
    bPresenting = true;
    Status = Action.ToLogLine();
    if (!Action.Reason.IsEmpty()) Status += TEXT(" | ") + Action.Reason.Left(120);
    PathMarkers->ClearInstances();
    for (const FIntPoint& Cell : Action.Path)
    {
        PathMarkers->AddInstance(FTransform(FQuat::Identity,
            CellLocation(Cell, 7.f) - Center, FVector(1.45f, 1.45f, 0.025f)));
    }
    if (Action.Kind == EAIHeroesGridActionKind::Attack)
    {
        PathMarkers->AddInstance(FTransform(FQuat::Identity,
            CellLocation(Action.ToCell, 7.f) - Center, FVector(1.45f, 1.45f, 0.025f)));
    }
    return true;
}

void AAIHeroesLiveBattleStage::CommitCurrentAction()
{
    if (bEventCommitted) return;
    bEventCommitted = true;
    if (CurrentAction.Kind == EAIHeroesGridActionKind::Move)
    {
        TArray<FIntPoint>& Cells = CurrentAction.bPlayerAttacker ? PlayerPositions : EnemyPositions;
        Cells[CurrentAction.AttackerStackIndex] = CurrentAction.ToCell;
        RefreshFigure(CurrentAction.bPlayerAttacker, CurrentAction.AttackerStackIndex);
    }
    else if (CurrentAction.Kind == EAIHeroesGridActionKind::Attack)
    {
        TArray<FAIHeroesArmyStack>& Targets = CurrentAction.bPlayerAttacker ? EnemyState : PlayerState;
        Targets[CurrentAction.TargetStackIndex].TotalHealth = CurrentAction.TargetHealthAfter;
        RefreshFigure(!CurrentAction.bPlayerAttacker, CurrentAction.TargetStackIndex);
        if (AStaticMeshActor* Target = Figure(!CurrentAction.bPlayerAttacker, CurrentAction.TargetStackIndex))
        {
            if (DamageMaterial) Target->GetStaticMeshComponent()->SetMaterial(0, DamageMaterial);
        }
    }
}

void AAIHeroesLiveBattleStage::UpdateAnimation(float Alpha)
{
    AStaticMeshActor* Attacker = Figure(CurrentAction.bPlayerAttacker, CurrentAction.AttackerStackIndex);
    if (CurrentAction.Kind == EAIHeroesGridActionKind::Move && Attacker)
    {
        const float AlongPath = FMath::Min(Alpha * CurrentAction.Path.Num(),
            static_cast<float>(CurrentAction.Path.Num()) - UE_KINDA_SMALL_NUMBER);
        const int32 Segment = FMath::Clamp(FMath::FloorToInt(AlongPath), 0, CurrentAction.Path.Num() - 1);
        const FIntPoint From = Segment == 0 ? CurrentAction.FromCell : CurrentAction.Path[Segment - 1];
        Attacker->SetActorLocation(FMath::Lerp(CellLocation(From),
            CellLocation(CurrentAction.Path[Segment]), AlongPath - Segment));
        const TArray<TObjectPtr<ATextRenderActor>>& Labels =
            CurrentAction.bPlayerAttacker ? PlayerLabels : EnemyLabels;
        if (Labels.IsValidIndex(CurrentAction.AttackerStackIndex) &&
            IsValid(Labels[CurrentAction.AttackerStackIndex].Get()))
        {
            Labels[CurrentAction.AttackerStackIndex]->SetActorLocation(
                Attacker->GetActorLocation() + FVector(0.f, 0.f, 95.f));
        }
    }
    else if (CurrentAction.Kind == EAIHeroesGridActionKind::Attack)
    {
        if (Attacker)
        {
            const FVector Direction = (CellLocation(CurrentAction.ToCell) - CellLocation(CurrentAction.FromCell)).GetSafeNormal();
            Attacker->SetActorLocation(CellLocation(CurrentAction.FromCell) +
                Direction * FMath::Sin(Alpha * UE_PI) * 65.f);
        }
        if (Alpha >= 0.45f)
        {
            CommitCurrentAction();
            if (IsValid(DamageLabel.Get()))
            {
                DamageLabel->SetActorHiddenInGame(false);
                DamageLabel->SetActorLocation(CellLocation(CurrentAction.ToCell, 195.f + Alpha * 75.f));
                DamageLabel->GetTextRender()->SetText(FText::FromString(
                    FString::Printf(TEXT("-%d"), CurrentAction.Damage)));
            }
        }
    }
}

void AAIHeroesLiveBattleStage::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bInitialized) return;
    if (bPresenting && !bPresentationPaused && FMath::IsFinite(DeltaSeconds) && DeltaSeconds > 0.f)
    {
        EventElapsed += DeltaSeconds * PresentationSpeed;
        const float Alpha = FMath::Clamp(EventElapsed / EventDuration, 0.f, 1.f);
        UpdateAnimation(Alpha);
        if (Alpha >= 1.f) FinishPresentationImmediately();
    }
    FaceLabelsToCamera();
}

void AAIHeroesLiveBattleStage::FinishPresentationImmediately()
{
    if (!bPresenting) return;
    UpdateAnimation(1.f);
    CommitCurrentAction();
    RefreshFigure(CurrentAction.bPlayerAttacker, CurrentAction.AttackerStackIndex);
    if (CurrentAction.Kind == EAIHeroesGridActionKind::Attack)
        RefreshFigure(!CurrentAction.bPlayerAttacker, CurrentAction.TargetStackIndex);
    if (IsValid(DamageLabel.Get())) DamageLabel->SetActorHiddenInGame(true);
    PathMarkers->ClearInstances();
    bPresenting = false;
}

void AAIHeroesLiveBattleStage::SetPresentationPaused(bool bPaused)
{
    bPresentationPaused = bPaused;
}

void AAIHeroesLiveBattleStage::SetPresentationSpeed(float Speed)
{
    if (FMath::IsFinite(Speed)) PresentationSpeed = Speed < 1.5f ? 1.f : Speed < 3.f ? 2.f : 4.f;
}

void AAIHeroesLiveBattleStage::ShowResult(const FString& Result)
{
    FinishPresentationImmediately();
    bResultShown = true;
    Status = TEXT("BATTLE COMPLETE | ") + Result;
}

void AAIHeroesLiveBattleStage::OrbitCamera(float YawDelta, float ZoomDelta)
{
    if (!FMath::IsFinite(YawDelta) || !FMath::IsFinite(ZoomDelta)) return;
    CameraYaw = FMath::UnwindDegrees(CameraYaw + YawDelta);
    CameraRadius = FMath::Clamp(CameraRadius + ZoomDelta, 1050.f, 2300.f);
    UpdateCamera();
    FaceLabelsToCamera();
}

void AAIHeroesLiveBattleStage::UpdateCamera()
{
    if (!IsValid(BattleCamera.Get())) return;
    const float Radians = FMath::DegreesToRadians(CameraYaw);
    const FVector Focus = Center + FVector(0.f, 0.f, 70.f);
    const FVector Location = Focus + FVector(FMath::Cos(Radians) * CameraRadius,
        FMath::Sin(Radians) * CameraRadius, CameraRadius * 0.82f);
    BattleCamera->SetActorLocationAndRotation(Location, (Focus - Location).Rotation());
}

void AAIHeroesLiveBattleStage::FaceLabelsToCamera()
{
    if (!IsValid(BattleCamera.Get())) return;
    auto FaceLabel = [this](ATextRenderActor* Label)
    {
        if (!IsValid(Label)) return;
        const FVector Direction = BattleCamera->GetActorLocation() - Label->GetActorLocation();
        Label->SetActorRotation(FRotator(0.f, Direction.Rotation().Yaw, 0.f));
    };
    for (ATextRenderActor* Label : PlayerLabels) FaceLabel(Label);
    for (ATextRenderActor* Label : EnemyLabels) FaceLabel(Label);
    FaceLabel(DamageLabel.Get());
}

AActor* AAIHeroesLiveBattleStage::GetViewTarget()
{
    return IsValid(BattleCamera.Get()) ? BattleCamera.Get() : nullptr;
}

FString AAIHeroesLiveBattleStage::GetStatusText() const
{
    return bPresentationPaused && bPresenting ? TEXT("PAUSED | ") + Status : Status;
}

FString AAIHeroesLiveBattleStage::GetArmyStatus(bool bPlayer) const
{
    const TArray<FAIHeroesArmyStack>& Army = bPlayer ? PlayerState : EnemyState;
    FString Summary;
    for (int32 Index = 0; Index < Army.Num(); ++Index)
    {
        const FAIHeroesArmyStack& Stack = Army[Index];
        Summary += FString::Printf(TEXT("%s%s #%d: %d units / %d HP"),
            Summary.IsEmpty() ? TEXT("") : TEXT(" | "),
            *Stack.UnitId.ToString(), Index + 1,
            Stack.Count(), Stack.TotalHealth);
    }
    return Summary.IsEmpty() ? TEXT("NO UNITS") : Summary;
}

void AAIHeroesLiveBattleStage::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    CleanupPresentationActors();
    Super::EndPlay(EndPlayReason);
}

void AAIHeroesLiveBattleStage::Destroyed()
{
    // RouteEndPlay does not invoke EndPlay before the actor has begun play.
    // Initialize may still have spawned a camera/figures in a disposable world.
    CleanupPresentationActors();
    Super::Destroyed();
}

void AAIHeroesLiveBattleStage::CleanupPresentationActors()
{
    bPresenting = false;
    bInitialized = false;
    // Camera, labels and figures are owned by this view, never strategy actors.
    for (AActor* Actor : SpawnedActors)
    {
        if (IsValid(Actor)) Actor->Destroy();
    }
    SpawnedActors.Empty();
    BattleCamera = nullptr;
    DamageLabel = nullptr;
    PlayerFigures.Empty();
    EnemyFigures.Empty();
    PlayerLabels.Empty();
    EnemyLabels.Empty();
}
