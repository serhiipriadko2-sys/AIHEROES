#pragma once

#include "CoreMinimal.h"

// Original, five-site scouting rules. A mask is a bounded piece of campaign
// information, NOT an attack or movement permission. No Actors, IO or RNG.
// bit 0 HQ, bit 1 FOREST, bit 2 ORE, bit 3 OUTPOST, bit 4 RUINS.
class AIHEROES_API FAIHeroesFogRules
{
public:
    static constexpr int32 InitialMask = 0x07; // HQ and two choices.
    static constexpr int32 AllMask = 0x1F;

    static int32 SiteBit(FName Site);
    static bool IsKnown(bool bEnabled, int32 Mask, FName Site);
    static int32 CountKnown(bool bEnabled, int32 Mask);

    // The old v1 SaveGame has no fog fields; false + mask=0 retains full map
    // knowledge and is never silently converted to the new mode.
    static bool Validate(bool bEnabled, int32 Mask,
        FName HeroSite, FName SelectedSite,
        const TSet<FName>& OwnedSites, FString& Error);

    // Visiting a known site reveals adjacent sites, never auto-moves or buys.
    // Invalid input is rejected without changing OutMask.
    static bool Reveal(bool bEnabled, int32 Mask, FName Visited,
        int32& OutMask, FString& Error);
};
