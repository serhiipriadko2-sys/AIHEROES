#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AIHeroesTypes.h"
#include "AIHeroesBattleRecord.h"
#include "AIHeroesRoutePlanner.h"
#include "AIHeroesGridBattle.h"
#include "AIHeroesGameMode.generated.h"

class AAIHeroesWorldNode;
class AAIHeroesBattleStage;
class AAIHeroesLiveBattleStage;
class AStaticMeshActor;
class APawn;
struct FAIHeroesBattleOutcome;

enum class EAIHeroesLiveBattlePhase : uint8
{
    Exploration, Running, Paused, Result
};

UCLASS()
class AIHEROES_API AAIHeroesGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AAIHeroesGameMode();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 Day = 1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 Gold = 240;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 Wood = 35;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 Ore = 30;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 ProtocolTier = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 ModelSlotTier = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 DataFragments = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 MovesLeft = 4;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<FAIHeroesArmyStack> PlayerArmy;

    UPROPERTY()
    TArray<TObjectPtr<AAIHeroesWorldNode>> Nodes;

    UPROPERTY()
    TObjectPtr<AAIHeroesWorldNode> SelectedNode;

    UPROPERTY()
    TObjectPtr<AStaticMeshActor> HeroMarker;

    FString LastEvent;
    FString LastHint;
    FString LastCombatStep;
    TArray<FString> BattleJournal;
    FAIHeroesBattleRecord LastBattleRecord;
    int32 ActiveSaveSlot = 1;
    int32 ReplayCursor = INDEX_NONE;

    void RecruitMilitia();
    void RecruitScouts();
    void UpgradeProtocol();
    void UpgradeModelSlot();
    void EndTurn();
    void SelectNode(AAIHeroesWorldNode* Node);
    void ConfirmSelectedNode();
    void InteractWithNode(AAIHeroesWorldNode* Node);
    void AskAdvisor();
    void AskRecruitmentAdvisor();
    void PlanRoute();
    // Read-only HUD preview; no automatic movement or combat authority.
    const FAIHeroesRoutePlan& GetRoutePreview() const { return RoutePreview; }
    FVector GetHeroStrategicLocation() const { return HeroStrategicLocation; }
    bool IsBattleReplayActive() const;
    FString GetBattleReplayStatus() const;
    FString GetBattleReplayArmyStatus(bool bPlayer) const;
    void SkipBattleReplay();
    void SaveCampaign();
    void LoadCampaign();
    void CycleSaveSlot();
    void AdvanceBattleReplay();
    FString GetReplayDisplayText() const;
    void PlayLastBattleIn3D();
    bool IsLiveBattleActive() const { return LiveBattlePhase != EAIHeroesLiveBattlePhase::Exploration; }
    EAIHeroesLiveBattlePhase GetLiveBattlePhase() const { return LiveBattlePhase; }
    int32 GetLiveBattleActionCount() const { return LiveSession.Actions.Num(); }
    FString GetLiveBattleStatus() const;
    FString GetLiveArmyStatus(bool bPlayer) const;
    float GetLiveBattleSpeed() const { return LiveBattleSpeed; }
    void ToggleLiveBattlePause();
    void CycleLiveBattleSpeed();
    void FastFinishLiveBattle();
    void ReturnFromLiveBattle();
    int32 GetSelectedMoveCost() const;
    FString GetSelectionSummary() const;

    FString GetArmySummary() const;
    FString GetCognitionSummary() const;

private:
    UPROPERTY()
    TObjectPtr<AAIHeroesLiveBattleStage> LiveBattleStage;

    UPROPERTY()
    TObjectPtr<AActor> PreLiveViewTarget;

    TWeakObjectPtr<AAIHeroesWorldNode> LiveBattleNode;
    FAIHeroesGridSession LiveSession;
    TArray<FAIHeroesArmyStack> LivePlayerBefore;
    TArray<FAIHeroesArmyStack> LiveEnemyBefore;
    FString PreBattleFingerprint;
    FString LiveResultText;
    EAIHeroesLiveBattlePhase LiveBattlePhase = EAIHeroesLiveBattlePhase::Exploration;
    int32 LiveMoveCost = 0;
    int32 LiveDay = 0;
    int32 LiveProtocolTier = 0;
    float LiveBattleSpeed = 1.f;
    bool bFastFinishingLiveBattle = false;
    bool bLiveBattleCommitted = false;
    bool BlockStrategicAction();
    bool StartLiveBattle(AAIHeroesWorldNode* Node, int32 MoveCost);
    void TickLiveBattle(float DeltaSeconds);
    void CommitLiveBattle();
    void AbortLiveBattle(const FString& Error);
    void ClearLiveStage(bool bReleaseViewTarget = true);
    FString CampaignFingerprint() const;
    void BeginLiveBattleSmoke();

    UPROPERTY()
    TObjectPtr<AAIHeroesBattleStage> ActiveBattleStage;

    UPROPERTY()
    TObjectPtr<APawn> ReplayCameraPawn;

    FTransform PreReplayCameraTransform;
    bool bSavedReplayCamera = false;

    FVector HeroStrategicLocation = FVector(-650.f, 0.f, 155.f);
    FAIHeroesRoutePlan RoutePreview;
    TArray<FAIHeroesRoutePlan> RouteAlternatives;
    int32 ActiveRouteIndex = 0;
    FAIHeroesAdvisorSnapshot BuildAdvisorSnapshot() const;
    void InvalidateRoutePreview();
    void SpawnWorld();
    void SpawnHeroMarker();
    void MoveHeroTo(const FVector& Location);
    AAIHeroesWorldNode* SpawnNode(
        EAIHeroesNodeType Type,
        const FString& Name,
        const FVector& At,
        const TArray<FAIHeroesArmyStack>& Guards);
    void SetEvent(const FString& Event);
    void AddStack(const FAIHeroesArmyStack& NewStack);
    bool HasLivingArmy() const;
};
