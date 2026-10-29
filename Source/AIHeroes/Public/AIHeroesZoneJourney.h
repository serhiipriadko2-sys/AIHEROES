#pragma once

#include "CoreMinimal.h"
#include "AIHeroesAdventurePath.h"
#include "AIHeroesZoneExpeditionSave.h"

// Bounded, offline observation of player-visible topology. Never an order.
enum class EAIHeroesJourneyStatus : uint8
{
    InvalidCampaign,
    UnknownDestination,
    NoLivingTroops,
    NoLegalItinerary,
    AlreadyThere,
    ReadyNow,
    PlannedWithRestOrStages
};

enum class EAIHeroesJourneyStepKind : uint8
{
    EndDay,
    ConfirmTravel
};

struct FAIHeroesJourneyStep
{
    EAIHeroesJourneyStepKind Kind = EAIHeroesJourneyStepKind::EndDay;
    FName Destination = NAME_None; // None for an explicit human End Day.
    int32 DayOffset = 0; // Zero means current campaign day.
    int32 MoveCost = 0;
    int32 MovesRemainingAfter = 0;
    bool bBattleAtDestination = false; // Outcome is never predicted.
};

struct FAIHeroesJourneyPlan
{
    EAIHeroesJourneyStatus Status = EAIHeroesJourneyStatus::InvalidCampaign;
    uint32 SourceRevision = 0;
    FName Goal = NAME_None;
    int32 ArrivalDay = 0;
    int32 DayAdvances = 0;
    int32 TravelConfirmations = 0;
    int32 TotalMovement = 0;
    bool bGuardianOutcomeUnknown = false;
    TArray<FAIHeroesJourneyStep> Steps; // <= 40; no executed actions.
    FString Explanation;

    bool HasPlan() const
    {
        return Status == EAIHeroesJourneyStatus::AlreadyThere ||
               Status == EAIHeroesJourneyStatus::ReadyNow ||
               Status == EAIHeroesJourneyStatus::PlannedWithRestOrStages;
    }
};

class AIHEROES_API FAIHeroesZoneJourney
{
public:
    // Validates the campaign and obtains PlayerView itself; never accepts
    // the generator's hidden graph or mutates SaveGame/army/world/actors.
    // Plans only legal all-or-nothing site moves of at most 8 points.
    // Every intermediate stop must be unguarded or previously cleared.
    // Each proposed action must be human-confirmed separately; replan
    // after EVERY performed step because discovery can change the map.
    static FAIHeroesJourneyPlan Plan(
        const FAIHeroesZoneExpeditionState& Campaign,
        FName KnownDestination);

#if WITH_DEV_AUTOMATION_TESTS
    // Development-only graph invariant tests. Production entrypoint
    // always calls validated PlayerView, never an oracle.
    static FAIHeroesJourneyPlan PlanVisibleGraphForTests(
        const FAIHeroesAdventureGraph& PlayerVisibleGraph,
        FName From, FName To, int32 CurrentMoves,
        int32 CurrentDay, bool bHasLivingArmy);
#endif
};
