#pragma once

#include "CoreMinimal.h"

class AActor;

// Purely visual crossing of the existing HQ -> FOREST Road V2 link.
// Does not add an edge, hit target, collider, navigation obstacle or save state.
class AIHEROES_API FAIHeroesScenicCrossing
{
public:
    static bool CalculateTransform(
        const FVector& RoadStart, const FVector& RoadEnd, FTransform& Out);
    static bool Attach(AActor& Environment,
        const FVector& RoadStart, const FVector& RoadEnd);
    static void SetShown(AActor& Environment, bool bShown);
};