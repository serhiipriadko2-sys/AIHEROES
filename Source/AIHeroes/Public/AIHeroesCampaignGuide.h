#pragma once

#include "CoreMinimal.h"
#include "AIHeroesCampaignObjectives.h"

// A read-only, contextual explanation of the canonical campaign objectives.
// It never selects a node, spends currency, moves the hero or reveals fog.
struct FAIHeroesCampaignGuideContext
{
    FName SelectedId = NAME_None;
    bool bTargetDiscovered = false;
    bool bHasArmy = true;
    // Unknown targets fail closed; the HUD supplies this only from a known
    // objective. An empty army still permits an unguarded visit.
    bool bTargetGuarded = true;
    bool bTravelMoving = false;
    bool bRouteReady = false;
    bool bNeedsMoreMoves = false;
    int32 MissingMoves = 0;
    // Read-only UI context. The original campaign restores four moves per
    // day; the guide must not tell the player to wait for an impossible path.
    int32 FullDayMoveBudget = 4;
    int32 RouteTotalCost = 0;
    // Populated only from an already-discovered, owned intermediate node
    // in the authoritative path. Never reveals an unexplored waypoint.
    FName IntermediateOwnedSite = NAME_None;
    // A known owned stage exists within a full day's budget, but not today's
    // remaining moves. Rest helps only that leg, never the whole long route.
    bool bOwnedStopAfterRest = false;
};

struct FAIHeroesCampaignGuideCard
{
    FString Heading;
    FString NextStep;
    FString Detail;
    FName FocusId = NAME_None;
    bool bVictory = false;
};

class AIHEROES_API FAIHeroesCampaignGuide
{
public:
    // Display eligibility only. GameMode independently validates every order.
    static bool CanOfferOrder(bool bRouteReady, bool bHasArmy, bool bRequiresBattle);
    static FAIHeroesCampaignGuideCard Build(
        const FAIHeroesCampaignProgress& Progress,
        const FAIHeroesCampaignGuideContext& Context);
};
