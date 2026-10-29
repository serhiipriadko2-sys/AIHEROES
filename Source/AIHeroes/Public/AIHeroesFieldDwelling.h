#pragma once

#include "CoreMinimal.h"
#include "AIHeroesTypes.h"

// Original AIHEROES adventure-map sub-sites. Not map-graph nodes: each site
// is reached through an existing guarded road destination (FOREST or ORE).
enum class EAIHeroesFieldSite : uint8
{
    ForestCamp,
    OreAerie
};

struct FAIHeroesFieldSiteStock
{
    bool bClaimed = false;
    int32 Available = 0;
};

struct FAIHeroesFieldDwellings
{
    FAIHeroesFieldSiteStock Forest{false, 5};
    FAIHeroesFieldSiteStock Ore{false, 2};

    bool operator==(const FAIHeroesFieldDwellings& Other) const
    {
        return Forest.bClaimed == Other.Forest.bClaimed &&
            Forest.Available == Other.Forest.Available &&
            Ore.bClaimed == Other.Ore.bClaimed &&
            Ore.Available == Other.Ore.Available;
    }
};

// No actors, dice, IO, player saves or LLM. All failure cases leave player
// resources, armies and field inventories untouched. Only the human-confirmed
// GameMode call can invoke TryClaim or TryRecruit.
class AIHEROES_API FAIHeroesFieldDwellingRules
{
public:
    static constexpr int32 MaxDay = 1000000;
    static constexpr int32 ForestCap = 50;
    static constexpr int32 OreCap = 24;

    static FAIHeroesFieldDwellings Initial();
    static bool IsValid(const FAIHeroesFieldDwellings& S);
    static FName ParentNodeId(EAIHeroesFieldSite Site);
    static int32 Starter(EAIHeroesFieldSite Site);
    static int32 WeeklyGrowth(EAIHeroesFieldSite Site);
    static int32 Capacity(EAIHeroesFieldSite Site);

    static const FAIHeroesFieldSiteStock& At(
        const FAIHeroesFieldDwellings& State, EAIHeroesFieldSite Site);

    // Reset unclaimed sites once/week; claimed sites accumulate up to cap.
    // A lost parent territory releases its site. Preflight before EndTurn.
    static bool PlanNextDay(int32 CurrentDay,
        bool bForestOwned, bool bOreOwned,
        const FAIHeroesFieldDwellings& Before,
        FAIHeroesFieldDwellings& After, FString& Error);

    // A separate human E-press is required to claim, then a later E-press
    // purchases ONE full standard pack at the selected secured site.
    static bool TryClaim(EAIHeroesFieldSite Site,
        bool bTerritoryOwned, bool bHeroAtSite,
        FAIHeroesFieldDwellings& State, FString& Error);

    static bool TryRecruit(EAIHeroesFieldSite Site,
        bool bTerritoryOwned, bool bHeroAtSite,
        FAIHeroesFieldDwellings& State,
        int32& Gold, int32& Wood, int32& Ore,
        TArray<FAIHeroesArmyStack>& Army, FString& Error);

private:
    friend class FAIHeroesSupplyLines;
    // Shared transactional purchase core; only the field-local authorization
    // or the fully checked convoy policy may call it. Never AI-autonomous.
    static bool BuyAuthorized(EAIHeroesFieldSite Site,
        FAIHeroesFieldDwellings& State,
        int32& Gold, int32& Wood, int32& Ore,
        TArray<FAIHeroesArmyStack>& Army, int32 ExtraGold, FString& Error);
};
