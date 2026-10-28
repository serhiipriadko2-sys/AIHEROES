#pragma once

#include "CoreMinimal.h"

struct FAIHeroesBaseUpgradeCost
{
    int32 Gold = 0;
    int32 Wood = 0;
    int32 Ore = 0;
    bool bAvailable = false;
};

// Pure, bounded economy rules. No actors, IO, model inference or authority.
class AIHEROES_API FAIHeroesBaseRules
{
public:
    static constexpr int32 MaxTier = 3;
    static constexpr int32 MaxResource = 500000000;
    static constexpr int32 MaxDay = 1000000;
    static bool IsValidTier(int32 Tier);
    static FAIHeroesBaseUpgradeCost UpgradeCost(int32 Tier);
    static int32 DailyGold(int32 Tier);
    // Failure preserves ALL references. Location permission comes from GameMode.
    static bool TryUpgrade(int32& Tier, int32& Gold, int32& Wood, int32& Ore,
        bool bAtSelectedOwnedHQ, FString& Error);
};
