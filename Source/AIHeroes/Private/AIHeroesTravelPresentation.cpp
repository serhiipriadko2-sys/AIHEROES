#include "AIHeroesTravelPresentation.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

AAIHeroesTravelPresentation::AAIHeroesTravelPresentation()
{
    PrimaryActorTick.bCanEverTick = false; // GameMode feeds one explicit view update.
    PathMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("SelectedRoadPath"));
    RootComponent = PathMesh;
    PathMesh->SetMobility(EComponentMobility::Movable);
    PathMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    PathMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
    PathMesh->SetGenerateOverlapEvents(false);
    PathMesh->SetCanEverAffectNavigation(false);
    PathMesh->SetCastShadow(false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> Base(
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (Cube.Succeeded()) PathMesh->SetStaticMesh(Cube.Object);
    if (Base.Succeeded()) PathMesh->SetMaterial(0, Base.Object);
}

void AAIHeroesTravelPresentation::EnsureMaterials()
{
    if (bMaterialAttempted) return;
    bMaterialAttempted = true;
    UMaterialInterface* Base = PathMesh->GetMaterial(0);
    if (!Base) return;
    TArray<FMaterialParameterInfo> Parameters;
    TArray<FGuid> Ids;
    Base->GetAllVectorParameterInfo(Parameters, Ids);
    FName Tint = NAME_None;
    for (const auto& Parameter : Parameters)
    {
        if (Parameter.Name == FName(TEXT("Color")) || Parameter.Name == FName(TEXT("Colour")))
        {
            Tint = Parameter.Name;
            break;
        }
    }
    ReadyMaterial = UMaterialInstanceDynamic::Create(Base, this);
    OverBudgetMaterial = UMaterialInstanceDynamic::Create(Base, this);
    if (!Tint.IsNone() && ReadyMaterial && OverBudgetMaterial)
    {
        ReadyMaterial->SetVectorParameterValue(Tint, FLinearColor(0.025f, 0.65f, 1.f));
        OverBudgetMaterial->SetVectorParameterValue(Tint, FLinearColor(1.f, 0.48f, 0.025f));
    }
    UE_LOG(LogTemp, Display, TEXT("[TRAVEL VIEW] material_tint=%s geometry_status_also_encoded=1"), *Tint.ToString());
}

void AAIHeroesTravelPresentation::ClearPath()
{
    if (!CachedPoints.IsEmpty() || PathMesh->GetInstanceCount() > 0)
    {
        PathMesh->ClearInstances();
        CachedPoints.Reset();
        ++GeometryRevision;
    }
    CachedStatus = EAIHeroesAdventurePathStatus::InvalidInput;
}

void AAIHeroesTravelPresentation::ShowPath(
    const TArray<FVector>& Points, EAIHeroesAdventurePathStatus Status)
{
    if (Status != EAIHeroesAdventurePathStatus::Ready &&
        Status != EAIHeroesAdventurePathStatus::NeedsMoreMoves)
    {
        ClearPath();
        return;
    }
    FAIHeroesTravelMotion Validation;
    if (!Validation.Begin(Points)) { ClearPath(); return; }
    if (CachedStatus == Status && CachedPoints == Points) return;
    ClearPath();
    CachedPoints = Points;
    CachedStatus = Status;
    EnsureMaterials();
    if (UMaterialInstanceDynamic* Material = Status == EAIHeroesAdventurePathStatus::Ready
        ? ReadyMaterial.Get() : OverBudgetMaterial.Get()) PathMesh->SetMaterial(0, Material);
    const bool bAffordable = Status == EAIHeroesAdventurePathStatus::Ready;
    for (int32 I = 1; I < Points.Num(); ++I)
    {
        FVector A(Points[I - 1].X, Points[I - 1].Y, 10.0);
        FVector B(Points[I].X, Points[I].Y, 10.0);
        const FVector Delta = B - A;
        const double Length = Delta.Size();
        if (Length <= 0.001) continue;
        const FRotator Direction = Delta.Rotation();
        // Short segments keep the marks outside large placeholder buildings.
        // Affordable = connected line; over-budget = arrowheads only.
        if (bAffordable)
            PathMesh->AddInstance(FTransform(Direction, (A + B) * 0.5,
                FVector(FMath::Max(12.0, Length - 220.0) / 100.0, 0.18, 0.035)), true);
        // Two larger direction cues survive partial coverage by a building/HUD.
        // Bounded to five instances per edge (four if movement is unaffordable).
        const double ArrowLength = FMath::Min(72.0, Length * 0.2);
        for (double Along : {0.33, 0.72})
        {
            const FVector Tip = FMath::Lerp(A, B, Along);
            for (double Side : {-1.0, 1.0})
            {
                const FRotator Wing(0.0, Direction.Yaw + Side * 35.0, 0.0);
                const FVector Center = Tip - Wing.Vector() * ArrowLength * 0.5;
                PathMesh->AddInstance(FTransform(Wing, Center,
                    FVector(ArrowLength / 100.0, 0.26, 0.055)), true);
            }
        }
    }
    ++GeometryRevision;
}

bool AAIHeroesTravelPresentation::BeginMovement(
    AStaticMeshActor* Marker, const TArray<FVector>& ApprovedPoints)
{
    FAIHeroesTravelMotion Candidate;
    if (!IsValid(Marker) || !Candidate.Begin(ApprovedPoints)) return false;
    Motion = MoveTemp(Candidate);
    MovingMarker = Marker;
    Marker->SetActorLocation(Motion.Position() + MarkerOffset());
    ShowPath(ApprovedPoints, EAIHeroesAdventurePathStatus::Ready);
    UE_LOG(LogTemp, Display, TEXT("[TRAVEL VIEW] BEGIN points=%d duration=%.3f campaign_writes=0"),
        ApprovedPoints.Num(), Motion.GetDuration());
    return true;
}

void AAIHeroesTravelPresentation::FinishAt(const FVector& LogicalDestination)
{
    if (MovingMarker.IsValid() && !LogicalDestination.ContainsNaN())
        MovingMarker->SetActorLocation(LogicalDestination + MarkerOffset());
    Motion.Reset();
    MovingMarker.Reset();
    ClearPath();
}

void AAIHeroesTravelPresentation::Advance(double DeltaSeconds, const FVector& LogicalDestination)
{
    if (!Motion.IsPlaying()) return;
    if (!MovingMarker.IsValid() || !Motion.GetDestination().Equals(LogicalDestination, 0.01))
    {
        FinishAt(LogicalDestination);
        return;
    }
    if (!Motion.Advance(DeltaSeconds)) return;
    MovingMarker->SetActorLocation(Motion.Position() + MarkerOffset());
    if (!Motion.IsPlaying())
    {
        FinishAt(LogicalDestination);
        UE_LOG(LogTemp, Display, TEXT("[TRAVEL VIEW] FINISH campaign_writes=0"));
    }
}

int32 AAIHeroesTravelPresentation::GetPathInstanceCount() const
{
    return PathMesh ? PathMesh->GetInstanceCount() : 0;
}
