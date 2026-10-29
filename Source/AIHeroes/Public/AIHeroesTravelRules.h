#pragma once

#include "CoreMinimal.h"
#include "AIHeroesAdventurePath.h"

struct FAIHeroesAdvisorSnapshot;

// Authoritative versioned strategic travel; no actors or mutation methods.
class AIHEROES_API FAIHeroesTravelRules
{
public:
    static constexpr int32 LegacyVersion = 1;
    static constexpr int32 RoadVersion = 2;
    static bool IsSupported(int32 Version) { return Version == LegacyVersion || Version == RoadVersion; }

    // Strict five-site map provider for V2. Cleared sites are for a private
    // forecast branch only, never a player order or a change to the snapshot.
    static bool BuildGraph(const FAIHeroesAdvisorSnapshot& Snapshot,
        FAIHeroesAdventureGraph& Out, FString& Error,
        const TSet<FName>& Cleared = TSet<FName>());
    static FAIHeroesAdventurePathResult Find(const FAIHeroesAdvisorSnapshot& Snapshot,
        const FVector& From, FName Destination, int32 AvailableMoves,
        const TSet<FName>& Cleared = TSet<FName>());
    static FString Describe(const FAIHeroesAdventurePathResult& Path);
};
