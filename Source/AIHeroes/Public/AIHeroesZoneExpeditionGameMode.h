#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AIHeroesZoneExpeditionRules.h"
#include "AIHeroesZoneExpeditionAdvisor.h"
#include "AIHeroesZoneExpeditionGameMode.generated.h"

class ACameraActor;
class AStaticMeshActor;
class ATextRenderActor;

// Strictly player-visible presentation data. The original generator oracle
// and enemy HP remain private; screen labels cannot authorize movement.
struct FAIHeroesVisibleSiteMarker
{
    FName Id = NAME_None;
    FVector Anchor = FVector::ZeroVector;
    bool bSelected = false;
    bool bCleared = false;
    bool bKnownGuard = false;
};

// Explicit alternate mode, NEVER the default Campaign v1.
// URL: /Engine/Maps/Entry?game=/Script/AIHeroes.AIHeroesZoneExpeditionGameMode
// Only a human Enter click confirms combat; autonomous Grid V2 resolves
// the encounter offscreen (not a real-time tactical stage in this mode).
UCLASS()
class AIHEROES_API AAIHeroesZoneExpeditionGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AAIHeroesZoneExpeditionGameMode();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    bool IsReady() const { return bReady; }
    const FAIHeroesZoneExpeditionState& GetState() const { return Campaign; }
    int32 TotalZones() const { return Generated.Zones.Num(); }
    int32 KnownZones() const { return FAIHeroesZoneExpeditionRules::DiscoveredCount(Campaign); }
    FName GetSelection() const { return SelectedSite; }
    FString GetLastMessage() const { return LastMessage; }
    FString GetSelectedPathInfo() const;
    bool IsDossierOpen() const { return bDossierOpen; }
    const FAIHeroesZoneExpeditionDossier& GetDossier() const { return Dossier; }
    void GetVisibleSiteMarkers(TArray<FAIHeroesVisibleSiteMarker>& Out) const;

private:
    struct FRoadVisual
    {
        TWeakObjectPtr<AStaticMeshActor> Actor;
        FName From = NAME_None;
        FName To = NAME_None;
    };

    bool bReady = false;
    bool bVisualQAEnabled = false;
    int32 VisualQASerial = INDEX_NONE;
    float VisualQACountdown = -1.f;
    void ScheduleVisualQAFrame();
    void CaptureVisualQAFrameTick(float DeltaSeconds);
    FAIHeroesZoneExpeditionState Campaign;
    FAIHeroesGeneratedWorld Generated;
    FName SelectedSite = NAME_None;
    FString LastMessage;
    bool bDossierOpen = false;
    FAIHeroesZoneExpeditionDossier Dossier;
    TMap<FName, TWeakObjectPtr<AStaticMeshActor>> ZoneActors;
    TMap<FName, TWeakObjectPtr<ATextRenderActor>> ZoneLabels;
    TArray<FRoadVisual> RoadVisuals;
    TWeakObjectPtr<AStaticMeshActor> HeroMarker;
    UPROPERTY()
    TObjectPtr<ACameraActor> ExpeditionCamera;

    bool CreateScene();
    void RefreshScene();
    void CycleSelection();
    void ClickSelectAtScreen(const FVector2D& Position);
    bool HandleScreenPress(const FVector2D& Position);
    void ConfirmSelected();
    void AdvanceDay();
    void Recruit();
    void Upgrade();
    void AskHero(bool bInspectSelection);
    void PreviewJourney();
    void ToggleDossier();
    void Save();
    void Load();
    void Status(const FString& Message);
};
