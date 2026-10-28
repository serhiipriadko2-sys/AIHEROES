#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AIHeroesGridBattle.h"
#include "AIHeroesLiveBattleStage.generated.h"

class ACameraActor;
class AStaticMeshActor;
class ATextRenderActor;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class UPointLightComponent;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;

// A disposable view of receipts already produced ONE AT A TIME by GridRulesV2.
// It never computes future actions or mutates campaign/engine authority.
UCLASS()
class AIHEROES_API AAIHeroesLiveBattleStage : public AActor
{
    GENERATED_BODY()

public:
    AAIHeroesLiveBattleStage();
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Destroyed() override;

    bool Initialize(
        const TArray<FAIHeroesArmyStack>& PlayerArmy,
        const TArray<FAIHeroesArmyStack>& EnemyArmy,
        const TArray<FIntPoint>& PlayerCells,
        const TArray<FIntPoint>& EnemyCells);
    bool Initialize(
        const TArray<FAIHeroesArmyStack>& PlayerArmy,
        const TArray<FAIHeroesArmyStack>& EnemyArmy);
    bool PresentAction(const FAIHeroesGridAction& Action);
    bool IsPresenting() const { return bPresenting; }
    void SetPresentationPaused(bool bPaused);
    void SetPresentationSpeed(float Speed);
    void ShowResult(const FString& Result);
    void FinishPresentationImmediately();
    void OrbitCamera(float YawDelta, float ZoomDelta);
    AActor* GetViewTarget();
    FString GetStatusText() const;
    // Read-only view of the stage's current HP, including historical V2 replay.
    FString GetArmyStatus(bool bPlayer) const;

private:
    UPROPERTY()
    TObjectPtr<USceneComponent> SceneRoot;
    UPROPERTY()
    TObjectPtr<UStaticMeshComponent> ArenaBase;
    UPROPERTY()
    TObjectPtr<UInstancedStaticMeshComponent> LightTiles;
    UPROPERTY()
    TObjectPtr<UInstancedStaticMeshComponent> DarkTiles;
    UPROPERTY()
    TObjectPtr<UInstancedStaticMeshComponent> PathMarkers;
    UPROPERTY()
    TObjectPtr<UPointLightComponent> FillLight;
    UPROPERTY()
    TObjectPtr<ACameraActor> BattleCamera;
    UPROPERTY()
    TArray<TObjectPtr<AStaticMeshActor>> PlayerFigures;
    UPROPERTY()
    TArray<TObjectPtr<AStaticMeshActor>> EnemyFigures;
    UPROPERTY()
    TArray<TObjectPtr<ATextRenderActor>> PlayerLabels;
    UPROPERTY()
    TArray<TObjectPtr<ATextRenderActor>> EnemyLabels;
    UPROPERTY()
    TObjectPtr<ATextRenderActor> DamageLabel;
    UPROPERTY()
    TArray<TObjectPtr<AActor>> SpawnedActors;
    UPROPERTY()
    TObjectPtr<UStaticMesh> CubeMesh;
    UPROPERTY()
    TObjectPtr<UStaticMesh> SphereMesh;
    UPROPERTY()
    TObjectPtr<UStaticMesh> CylinderMesh;
    UPROPERTY()
    TObjectPtr<UStaticMesh> ConeMesh;
    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> PlayerMaterial;
    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> EnemyMaterial;
    UPROPERTY()
    TObjectPtr<UMaterialInstanceDynamic> DamageMaterial;
    UPROPERTY()
    TArray<TObjectPtr<UMaterialInstanceDynamic>> BoardMaterials;

    TArray<FAIHeroesArmyStack> PlayerState;
    TArray<FAIHeroesArmyStack> EnemyState;
    TArray<FIntPoint> PlayerPositions;
    TArray<FIntPoint> EnemyPositions;
    FAIHeroesGridAction CurrentAction;
    FVector Center = FVector::ZeroVector;
    FString Status = TEXT("Live battle view is not initialized.");
    float EventElapsed = 0.f;
    float EventDuration = 0.f;
    float PresentationSpeed = 1.f;
    float CameraYaw = -90.f;
    float CameraRadius = 1550.f;
    bool bInitialized = false;
    bool bPresenting = false;
    bool bPresentationPaused = false;
    bool bEventCommitted = false;
    bool bResultShown = false;

    static constexpr float CellSize = 180.f;
    static bool ValidCell(const FIntPoint& Cell);
    FVector CellLocation(const FIntPoint& Cell, float Height = 70.f) const;
    AStaticMeshActor* Figure(bool bPlayer, int32 Index) const;
    void SpawnFigure(bool bPlayer, int32 Index);
    void RefreshFigure(bool bPlayer, int32 Index);
    void UpdateAnimation(float Alpha);
    void CommitCurrentAction();
    void UpdateCamera();
    void FaceLabelsToCamera();
    void CleanupPresentationActors();
    bool ValidateReceipt(const FAIHeroesGridAction& Action) const;
};
