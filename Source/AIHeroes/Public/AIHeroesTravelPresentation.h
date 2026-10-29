#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AIHeroesAdventurePath.h"
#include "AIHeroesTravelMotion.h"
#include "AIHeroesTravelPresentation.generated.h"

class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class AStaticMeshActor;

// Disposable view. Deliberately has no GameMode reference or gameplay commands.
UCLASS()
class AIHEROES_API AAIHeroesTravelPresentation : public AActor
{
    GENERATED_BODY()
public:
    AAIHeroesTravelPresentation();
    static FVector MarkerOffset() { return FVector(0.0, 0.0, 290.0); }
    void ShowPath(const TArray<FVector>& Points, EAIHeroesAdventurePathStatus Status);
    void ClearPath();
    bool BeginMovement(AStaticMeshActor* Marker, const TArray<FVector>& ApprovedPoints);
    void Advance(double DeltaSeconds, const FVector& LogicalDestination);
    void FinishAt(const FVector& LogicalDestination);
    bool IsMoving() const { return Motion.IsPlaying() && MovingMarker.IsValid(); }
    int32 GetPathInstanceCount() const;
    int32 GetGeometryRevision() const { return GeometryRevision; }
    EAIHeroesAdventurePathStatus GetDisplayedStatus() const { return CachedStatus; }
    UInstancedStaticMeshComponent* GetPathMesh() const { return PathMesh; }

private:
    UPROPERTY()
    TObjectPtr<UInstancedStaticMeshComponent> PathMesh;
    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> ReadyMaterial;
    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> OverBudgetMaterial;
    TWeakObjectPtr<AStaticMeshActor> MovingMarker;
    FAIHeroesTravelMotion Motion;
    TArray<FVector> CachedPoints;
    EAIHeroesAdventurePathStatus CachedStatus = EAIHeroesAdventurePathStatus::InvalidInput;
    int32 GeometryRevision = 0;
    bool bMaterialAttempted = false;
    void EnsureMaterials();
};
