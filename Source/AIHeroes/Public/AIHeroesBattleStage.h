#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AIHeroesBattleEngine.h"
#include "AIHeroesBattleStage.generated.h"

class AStaticMeshActor;
class ATextRenderActor;
class UPointLightComponent;
class USceneComponent;
class UStaticMesh;

// Small, disposable 3D representation of an already-authorized battle.
// It NEVER modifies campaign resources, HP or combat decisions.
UCLASS()
class AIHEROES_API AAIHeroesBattleStage : public AActor
{
    GENERATED_BODY()

public:
    AAIHeroesBattleStage();
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    bool Initialize(
        const FVector& InCenter,
        const TArray<FAIHeroesArmyStack>& PlayerInitial,
        const TArray<FAIHeroesArmyStack>& EnemyInitial,
        const FAIHeroesBattleOutcome& Outcome);

    bool IsFinished() const { return bFinished; }
    int32 GetDisplayedSteps() const { return DisplayedSteps; }
    int32 GetTotalSteps() const { return Battle.Actions.Num(); }
    FString GetStatusLine() const;
    FString GetArmyStatus(bool bPlayer) const;
    void Skip();

private:
    UPROPERTY()
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY()
    TObjectPtr<UPointLightComponent> PlayerGlow;

    UPROPERTY()
    TObjectPtr<UPointLightComponent> EnemyGlow;

    UPROPERTY()
    TObjectPtr<AStaticMeshActor> Arena;

    UPROPERTY()
    TArray<TObjectPtr<AStaticMeshActor>> PlayerFigures;

    UPROPERTY()
    TArray<TObjectPtr<AStaticMeshActor>> EnemyFigures;

    UPROPERTY()
    TArray<TObjectPtr<ATextRenderActor>> PlayerLabels;

    UPROPERTY()
    TArray<TObjectPtr<ATextRenderActor>> EnemyLabels;

    UPROPERTY()
    TObjectPtr<UStaticMesh> CubeMesh;

    UPROPERTY()
    TObjectPtr<UStaticMesh> SphereMesh;

    UPROPERTY()
    TObjectPtr<AStaticMeshActor> PulsingFigure;

    FAIHeroesBattleOutcome Battle;
    TArray<FAIHeroesArmyStack> PlayerState;
    TArray<FAIHeroesArmyStack> EnemyState;
    FVector Center = FVector::ZeroVector;
    FVector PreviousPulseScale = FVector::OneVector;
    int32 DisplayedSteps = 0;
    float StepClock = 0.f;
    float HoldClock = 0.f;
    float PulseClock = 0.f;
    bool bStarted = false;
    bool bFinished = false;

    static constexpr int32 MaxVisualSteps = 18;
    void SpawnFigure(bool bPlayer, int32 Index);
    void UpdateFigure(bool bPlayer, int32 Index);
    void ApplyNextAction();
    void ApplyFinalSnapshot();
    void ResetPulse();
};
