#pragma once
#include "CoreMinimal.h"
class AActor;
// Bounded visual dressing, never movement terrain or map authority.
class AIHEROES_API FAIHeroesScenicCoast
{
public:
    static TArray<FTransform> Transforms();
    static bool Attach(AActor& Environment);
};
