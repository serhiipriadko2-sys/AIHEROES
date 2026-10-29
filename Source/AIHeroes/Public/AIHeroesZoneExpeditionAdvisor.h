#pragma once

#include "CoreMinimal.h"
#include "AIHeroesZoneExpeditionSave.h"

// Read-only reasoning for the independently playable 7..11-zone expedition.
// All decisions remain with the human; proposals carry no action authority.
// Never receives the unfiltered generator oracle, user files or model text.
enum class EAIHeroesZoneAdviceStatus : uint8
{
    InvalidState,
    UnknownTarget,
    NoArmy,
    NeedsMovement,
    NoReachableTarget,
    Suggestion
};

struct FAIHeroesZoneExpeditionAdvice
{
    EAIHeroesZoneAdviceStatus Status = EAIHeroesZoneAdviceStatus::InvalidState;
    FName SuggestedSite = NAME_None;
    uint32 SourceRevision = 0; // Staleness hint, NOT player approval.
    int32 TravelCost = 0;
    int32 MissingMoves = 0;
    bool bGuarded = false;
    FString Message;

    bool HasSuggestion() const
    {
        return Status == EAIHeroesZoneAdviceStatus::Suggestion &&
            !SuggestedSite.IsNone();
    }
};


enum class EAIHeroesDossierReadiness : uint8
{
    ReadyNoGuard,
    GuardOutcomeUnknown,
    ThinReserves,
    NoLivingArmy,
    InsufficientMovement
};

// A reasoned, bounded comparison; NOT a battle forecast or executable order.
// Every listed identity and route is already available in PlayerView.
struct FAIHeroesZoneDossierOption
{
    FName Site = NAME_None;
    int32 TravelCost = 0;
    int32 MissingMoves = 0;
    int32 RankScore = 0; // Explicit heuristic, not win probability.
    bool bGuarded = false;
    bool bReachableNow = false;
    EAIHeroesDossierReadiness Readiness =
        EAIHeroesDossierReadiness::InsufficientMovement;
    FString Assessment;
    FString Rationale;
};

struct FAIHeroesZoneExpeditionDossier
{
    bool bValid = false;
    uint32 SourceRevision = 0; // Observation epoch only; no action consent.
    int32 ConsideredKnownTargets = 0;
    TArray<FAIHeroesZoneDossierOption> Options; // At most three.
    FString Summary;
};

class AIHEROES_API FAIHeroesZoneExpeditionAdvisor
{
public:
    // No selected target: recommend only a visible reachable destination.
    // With an InspectTarget: describe only that known target and its route.
    // Neither operation writes files, moves actors, spends resources, runs
    // combat, changes the passed state or reveals a hidden destination.
    static FAIHeroesZoneExpeditionAdvice Advise(
        const FAIHeroesZoneExpeditionState& State,
        FName InspectTarget = NAME_None);

    // Deterministically rank up to three visible, not-yet-owned targets.
    // Cannot select, move, battle, spend, save or access hidden topology.
    static FAIHeroesZoneExpeditionDossier CompareKnown(
        const FAIHeroesZoneExpeditionState& State);
};
