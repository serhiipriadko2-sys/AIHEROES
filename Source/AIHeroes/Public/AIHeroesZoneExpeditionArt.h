#pragma once

#include "CoreMinimal.h"
#include "AIHeroesZoneTemplate.h"

class AGameModeBase;
class AStaticMeshActor;

// Non-authoritative, original Amber March presentation for the optional
// procedural expedition. Never reads player secrets or alters game rules.
struct AIHEROES_API FAIHeroesZoneExpeditionArt
{
    // Pure deterministic cosmetic scatter: depends on seed, NOT on
    // undiscovered locations, road connections, ownership or guard state.
    static TArray<FVector2D> Scatter(uint32 Seed, int32 MaxItems, uint32 Salt);

    // Fallback-safe. A missing art kit preserves the legacy clickable
    // primitives, full visibility gates, paths, and player confirmations.
    static bool Apply(AGameModeBase& Owner,
        const FAIHeroesGeneratedWorld& Generated,
        const TMap<FName, TWeakObjectPtr<AStaticMeshActor>>& Zones,
        TWeakObjectPtr<AStaticMeshActor> HeroMarker);

    // Cosmetic surface only; no change to graph or route costs.
    static void StyleRoad(AStaticMeshActor& Road);
};
