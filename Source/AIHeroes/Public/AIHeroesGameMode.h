#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AIHeroesTypes.h"
#include "AIHeroesAdventurePath.h"
#include "AIHeroesBasePanel.h"
#include "AIHeroesBattleRecord.h"
#include "AIHeroesRoutePlanner.h"
#include "AIHeroesGridBattle.h"
#include "AIHeroesGridReplay.h"
#include "AIHeroesRecruitmentStock.h"
#include "AIHeroesFieldDwelling.h"
#include "AIHeroesSupplyLines.h"
#include "AIHeroesAdvisorDossier.h"
#include "AIHeroesCampaignObjectives.h"
#include "AIHeroesRivalStrategy.h"
#include "AIHeroesFogRules.h"
#include "AIHeroesModelReply.h"
#include "AIHeroesLocalExchange.h"
#include "AIHeroesGameMode.generated.h"

class AAIHeroesWorldNode;
class AAIHeroesBattleStage;
class AAIHeroesLiveBattleStage;
class AAIHeroesTravelPresentation;
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
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Destroyed() override;
    bool IsStrategicTravelMoving() const;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 Day = 1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 Gold = 240;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 Wood = 35;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 Ore = 30;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 BaseTier = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 ProtocolTier = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 ModelSlotTier = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 DataFragments = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 MovesLeft = 4;

    // Raw/legacy objects default to V1; a new BeginPlay campaign explicitly selects V2.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 TravelRuleVersion = 1;

    // The HQ maintains unpurchased recruits. This does NOT affect
    // neutral guard growth or grant AI authority to spend resources.
    FAIHeroesRecruitmentStock RecruitmentStock;
    int32 GetAvailableMilitia() const { return RecruitmentStock.Militia; }
    int32 GetAvailableScouts() const { return RecruitmentStock.Scouts; }

    // Local field stock is *not* the headquarters recruiting inventory.
    // Its ownership and quantities survive SaveGame rotations.
    FAIHeroesFieldDwellings FieldDwellings;
    void InteractFieldDwelling();
    FString GetFieldDwellingSummary() const;
    FString GetFieldDwellingLabel(EAIHeroesNodeType Type) const;
    // X / CONVOY: first action quotes, second confirms the identical state.
    void RequestSupplyConvoy();
    // Esc cancels a transient quotation without touching campaign resources.
    // Returns false when no quote was pending; never triggers a purchase.
    bool CancelSupplyConvoyQuote();
    FString GetSupplyConvoyStatus() const;
    bool IsSupplyConvoyPending() const;

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
    void UpgradeHeadquarters();
    void ToggleBasePanel();
    void CloseBasePanel();
    bool IsBasePanelVisible() const;
    void PreviewBasePurchase(EAIHeroesBasePurchase Kind);
    void ConfirmBasePurchase();
    bool CanConfirmBasePurchase() const;
    FAIHeroesBasePanelOffer GetBasePanelOffer(EAIHeroesBasePurchase Kind) const;
    EAIHeroesBasePurchase GetSelectedBasePurchase() const { return PendingBaseQuote.Kind; }
    const FString& GetBasePanelMessage() const { return BasePanelMessage; }
    FString GetHeadquartersSummary() const;
    int32 GetDailyGoldIncome() const;
    void UpgradeProtocol();
    void UpgradeModelSlot();
    void EndTurn();
    FString GetRivalForecast() const;
    bool IsNodeDiscovered(FName SiteId) const
    {
        return FAIHeroesFogRules::IsKnown(bFogEnabled, FogKnownMask, SiteId);
    }
    FString GetScoutingStatus() const;
    void SelectNode(AAIHeroesWorldNode* Node);
    void ConfirmSelectedNode();
    void InteractWithNode(AAIHeroesWorldNode* Node);
    void AskAdvisor();
    void AskRecruitmentAdvisor();
    FAIHeroesCampaignProgress GetCampaignProgress() const;
    void ShowCampaignObjectives();
    void OpenProtocolLab();
    void AnnounceCampaignVictoryIfEarned();
    const FAIHeroesAdvisorDossier& GetAdvisorDossier() const { return AdvisorDossier; }
    int32 GetAdvisorDossierPage() const { return AdvisorDossierPage; }
    bool IsAdvisorDossierVisible() const;
    bool IsAdvisorDossierFresh() const;
    void ToggleAdvisorDossier();
    void CloseAdvisorDossier();
    void RefreshAdvisorDossier();
    void ChangeAdvisorDossierPage(int32 Direction, int32 PageCount);
    void PlanRoute();
    // Read-only HUD preview; no automatic movement or combat authority.
    const FAIHeroesRoutePlan& GetRoutePreview() const { return RoutePreview; }
    FVector GetHeroStrategicLocation() const { return HeroStrategicLocation; }
    bool IsBattleReplayActive() const;
    FString GetBattleReplayStatus() const;
    FString GetBattleReplayArmyStatus(bool bPlayer) const;
    void SkipBattleReplay();
    void ToggleGridReplayPause();
    void CycleGridReplaySpeed();
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
    const AAIHeroesLiveBattleStage* GetLiveBattlePresentation() const;
    // Copy of an already produced Step only; never advances or selects an action.
    bool TryGetLatestLiveBattleAction(FAIHeroesGridAction& OutAction) const;
    float GetLiveBattleSpeed() const { return LiveBattleSpeed; }
    void ToggleLiveBattlePause();
    void CycleLiveBattleSpeed();
    void FastFinishLiveBattle();
    void ReturnFromLiveBattle();
    int32 GetSelectedMoveCost() const;
    FAIHeroesAdventurePathResult GetSelectedTravelPath() const;
    // Read-only query for known current sites; does not select, move or reveal.
    FAIHeroesAdventurePathResult GetTravelPathTo(FName Destination) const;
    FString GetSelectionSummary() const;

    FString GetArmySummary() const;
    FString GetCognitionSummary() const;

    // Read-only, game-thread export for the optional local-model adapter.
    // Caller owns session/request correlation; this does not enable inference.
    bool BuildLocalModelRequest(const FString& RequestId, const FString& SessionId,
        FString& OutJson, FString& Error) const;

    // Prepare/consume a suggestion only. No HTTP, model readiness or gameplay writes.
    bool BeginLocalModelAdvice(FString& OutJson, FString& Error);
    FAIHeroesModelReplyResult CompleteLocalModelAdvice(const FString& SelectionJson);
    void CancelLocalModelAdvice();
    bool IsLocalModelAdvicePending() const { return LocalModelReplies.IsPending(); }

    // Optional loopback transport. Disabled unless an explicit host call opts in.
    // A successful exchange is NOT verified weights; offline UI stays authoritative.
    bool StartLocalModelExchange(int32 Port, const FString& Model, bool bExplicitLocalOptIn, FString& Error);
    void CancelLocalModelExchange();
    bool IsLocalModelExchangePending() const { return LocalModelExchange.IsPending(); }
    const FAIHeroesModelReplyResult& GetLocalModelExchangeResult() const { return LocalExchangeResult; }

private:
    FAIHeroesLocalExchange LocalModelExchange;
    FAIHeroesModelReplyResult LocalExchangeResult;
    bool bLocalExchangeHandled = true;
    void TickLocalModelExchange();
    void BeginLocalExchangeSmoke();
    FAIHeroesModelReplySession LocalModelReplies;
    bool BuildLocalModelSnapshot(FAIHeroesProposalSnapshot& Out) const;
    // Transient UI quote, never a campaign save or an AI authorization.
    bool bBasePanelOpen = false;
    // Explicitly false for legacy worlds and isolated GameMode tests.
    // BeginPlay activates new scouting campaigns with 3 known sites.
    bool bFogEnabled = false;
    int32 FogKnownMask = 0;
    void RefreshFogVisibility();
    void RevealAfterArrival(FName ArrivedSite);
    FAIHeroesBasePanelOffer PendingBaseQuote;
    FString PendingBaseFingerprint;
    FString BasePanelMessage;
    // UI evidence is transient. It is neither a campaign save nor AI memory.
    FAIHeroesAdvisorDossier AdvisorDossier;
    int32 AdvisorDossierPage = 0;
    bool bAdvisorDossierOpen = false;
    void RecordAdvisorDossier(
        const FString& Title, const TArray<FString>& Paragraphs,
        bool bRecruitment, bool bObjectives = false, bool bProtocolLab = false);
    // Transient, non-SaveGame authorization. Any campaign change invalidates it.
    FString PendingSupplyFingerprint;
    FAIHeroesSupplyAccess BuildSupplyAccess() const;
    FString BuildAIProposalGateStatus() const;

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
    // Transient presentation time only; never part of a save or Grid session.
    float LiveTextPresentationSeconds = 0.f;
    bool bFastFinishingLiveBattle = false;
    bool bLiveBattleCommitted = false;
    bool BlockStrategicAction();
    bool StartLiveBattle(AAIHeroesWorldNode* Node, int32 MoveCost);
    void TickLiveBattle(float DeltaSeconds);
    void CommitLiveBattle();
    void AbortLiveBattle(const FString& Error);
    void ClearLiveStage(bool bReleaseViewTarget = true);
    FString CampaignFingerprint() const;
    bool BuildRivalSnapshot(int32 NewDay, int32 NewGold, int32 NewWood, int32 NewOre,
        FAIHeroesRivalSnapshot& Out, FString& Error) const;
    void BeginLiveBattleSmoke();
    void BeginTravelSmoke();
    void BeginTravelViewSmoke();
    void BeginSupplySmoke();
    void BeginModelRequestSmoke();

    UPROPERTY()
    TObjectPtr<AAIHeroesBattleStage> ActiveBattleStage;

    // V2 uses the same disposable 3D arena as live combat, but only
    // presents preverified receipts. It never owns campaign authority.
    UPROPERTY()
    TObjectPtr<AAIHeroesLiveBattleStage> GridReplayStage;
    TWeakObjectPtr<AActor> PreGridReplayViewTarget;
    FAIHeroesGridReplay GridReplay;
    int32 GridReplayCursor = 0;
    float GridReplayHoldClock = 0.f;
    float GridReplaySpeed = 1.f;
    bool bGridReplayPaused = false;
    bool bGridReplayResultShown = false;
    FString PreGridReplayFingerprint;
    void TickGridReplay(float DeltaSeconds);
    void FinishGridReplay(const FString& Reason);

    UPROPERTY()
    TObjectPtr<APawn> ReplayCameraPawn;

    FTransform PreReplayCameraTransform;
    bool bSavedReplayCamera = false;

    UPROPERTY()
    TArray<TObjectPtr<AStaticMeshActor>> AdventureRoadActors;
    void RefreshAdventureRoads();
    UPROPERTY()
    TObjectPtr<AAIHeroesTravelPresentation> TravelPresentation;
    AAIHeroesTravelPresentation* EnsureTravelPresentation();
    bool BuildTravelViewPoints(const FAIHeroesAdventurePathResult& Path,
        const FVector& Origin, TArray<FVector>& Out) const;
    void PresentStrategicTravel(const FAIHeroesAdventurePathResult& Path, const FVector& Origin);
    void FinishStrategicTravelPresentation();
    void TickStrategicTravelPresentation(float DeltaSeconds);
    void DestroyTravelPresentation();

    UPROPERTY()
    TArray<TObjectPtr<AStaticMeshActor>> FieldDwellingVisuals;
    void SpawnFieldDwellingVisuals();
    void RefreshFieldDwellingVisuals();

    FVector HeroStrategicLocation = FVector(-650.f, 0.f, 155.f);
    FAIHeroesRoutePlan RoutePreview;
    TArray<FAIHeroesRoutePlan> RouteAlternatives;
    int32 ActiveRouteIndex = 0;
    FAIHeroesAdvisorSnapshot BuildAdvisorSnapshot() const;
    void InvalidateRoutePreview();
    bool IsAtSelectedOwnedHeadquarters() const;
    void RefreshHeadquartersVisual();
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
