#include "AIHeroesBaseRules.h"

bool FAIHeroesBaseRules::IsValidTier(int32 Tier)
{
    return Tier >= 0 && Tier <= MaxTier;
}

FAIHeroesBaseUpgradeCost FAIHeroesBaseRules::UpgradeCost(int32 Tier)
{
    switch (Tier)
    {
        case 0: return {100, 10, 10, true};
        case 1: return {180, 20, 15, true};
        case 2: return {300, 35, 25, true};
        default: return {}; // Maxed or invalid is NOT a free upgrade.
    }
}

int32 FAIHeroesBaseRules::DailyGold(int32 Tier)
{
    return IsValidTier(Tier) ? 40 + 20 * Tier : 0;
}

bool FAIHeroesBaseRules::TryUpgrade(int32& Tier, int32& Gold,
    int32& Wood, int32& Ore, bool bAtSelectedOwnedHQ, FString& Error)
{
    Error.Empty();
    if (!IsValidTier(Tier) || Gold < 0 || Gold > MaxResource ||
        Wood < 0 || Wood > MaxResource || Ore < 0 || Ore > MaxResource)
    {
        Error = TEXT("HQ BLOCKED: invalid progress or resource totals. Nothing spent.");
        return false;
    }
    if (!bAtSelectedOwnedHQ)
    {
        Error = TEXT("HQ: select your headquarters and return the hero there before upgrading.");
        return false;
    }
    const FAIHeroesBaseUpgradeCost Cost = UpgradeCost(Tier);
    if (!Cost.bAvailable)
    {
        Error = TEXT("HQ: maximum level 3 reached. Nothing spent.");
        return false;
    }
    if (Gold < Cost.Gold || Wood < Cost.Wood || Ore < Cost.Ore)
    {
        Error = FString::Printf(TEXT("HQ needs %d gold / %d wood / %d ore. Nothing spent."),
            Cost.Gold, Cost.Wood, Cost.Ore);
        return false;
    }
    // No fallible operation after preflight. Do not award income on purchase.
    Gold -= Cost.Gold;
    Wood -= Cost.Wood;
    Ore -= Cost.Ore;
    ++Tier;
    return true;
}
