#pragma once

#include "CoreMinimal.h"
#include "AIHeroesAdvisor.h"

// A deterministic proposal, NOT an order to be executed automatically.
// Two-stop forecasts use actual post-combat survivors and the same
// movement rules as GameMode. The player must confirm every movement.
struct FAIHeroesRoutePlan
{
    TArray<FName> Stops;
    int32 TotalMoves = 0;
    int32 ExpectedHpLoss = 0;
    int32 RemainingUnits = 0;
    int32 Score = MIN_int32;

    bool IsValid() const { return !Stops.IsEmpty(); }
    FString Describe() const;
};

class AIHEROES_API FAIHeroesRoutePlanner
{
public:
    // Deterministic, unique and ranked feasible routes. MaxPlans is capped at
    // three to keep planning cheap and the player-facing choice understandable.
    // The highest-ranked entry is always identical to Plan(Snapshot).
    static TArray<FAIHeroesRoutePlan> Rank(
        const FAIHeroesAdvisorSnapshot& Snapshot, int32 MaxPlans = 3);
    static FAIHeroesRoutePlan Plan(const FAIHeroesAdvisorSnapshot& Snapshot);
};
