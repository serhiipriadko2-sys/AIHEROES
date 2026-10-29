#pragma once
#include "CoreMinimal.h"
class AActor;

// Presentation-only scene dressing, not a terrain collider or road graph.
class AIHEROES_API FAIHeroesSceneryV3
{
public:
    static TArray<FTransform> RidgeTransforms();
    static TArray<FTransform> WaystoneTransforms();
    static FTransform WaystationTransform();
    static bool Attach(AActor& Environment);
};