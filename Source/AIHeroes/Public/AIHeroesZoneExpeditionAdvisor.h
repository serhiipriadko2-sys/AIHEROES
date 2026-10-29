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
};
