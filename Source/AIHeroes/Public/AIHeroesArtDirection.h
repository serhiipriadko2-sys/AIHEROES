#pragma once
#include "CoreMinimal.h"
class AAIHeroesWorldNode;
class AAIHeroesGameMode;
// Disposable original visual kit. Does not change coordinates, path costs or ownership.
class AIHEROES_API FAIHeroesArtDirection
{
public:
    static bool ApplyNode(AAIHeroesWorldNode& Node);
    static void ApplyWorld(AAIHeroesGameMode& Game);
    static void RemoveWorld(AAIHeroesGameMode& Game);
};
