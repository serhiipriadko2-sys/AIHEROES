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
    static FAIHeroesRoutePlan Plan(const FAIHeroesAdvisorSnapshot& Snapshot);
};
