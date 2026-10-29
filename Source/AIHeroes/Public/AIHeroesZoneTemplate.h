#pragma once

#include "CoreMinimal.h"
#include "AIHeroesAdventurePath.h"

// Read-only candidate for a future dynamic campaign schema.
// The existing five-node campaign, its SaveGame slots and encounter rules
// are never modified or silently reinterpreted by this preview.
enum class EAIHeroesZoneRole : uint8
{
    Headquarters, Hub, Resource, Checkpoint, Objective, SideRoute
};

struct FAIHeroesGeneratedZone
{
    FName Id = NAME_None;
    EAIHeroesZoneRole Role = EAIHeroesZoneRole::SideRoute;
    FIntPoint Grid;
    int32 DangerTier = 0;
    bool bGuarded = false;
};

struct FAIHeroesGeneratedWorld
{
    uint32 Schema = 1;
    uint32 Seed = 0;
    uint32 Revision = 0;
    TArray<FAIHeroesGeneratedZone> Zones;
    FAIHeroesAdventureGraph Adventure; // Complete oracle for validation only.
};

class AIHEROES_API FAIHeroesZoneTemplate
{
public:
    static constexpr int32 MaxExtraRoutes = 4;

    // All inputs bounded; failure resets OutDraft; no actor or save writes.
    static bool Generate(uint32 Seed, int32 ExtraRoutes,
                         FAIHeroesGeneratedWorld& OutDraft, FString& Error);
    static bool Validate(const FAIHeroesGeneratedWorld& Draft, FString& Error);

    // A future level-generation tool can consume this; the result is NOT
    // a discovered map or a player movement authorization.
    static FString Describe(const FAIHeroesGeneratedWorld& Draft);
};
