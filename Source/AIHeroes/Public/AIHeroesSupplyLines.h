#pragma once

#include "CoreMinimal.h"
#include "AIHeroesFieldDwelling.h"

// A read-only transport access snapshot built from the authoritative live map.
// Distinct from a player order, model suggestion, map move, or SaveGame field.
struct FAIHeroesSupplyAccess
{
    EAIHeroesFieldSite Site = EAIHeroesFieldSite::ForestCamp;
    bool bSourceDiscovered = false;
    bool bHeadquartersOwned = false;
    bool bSourceOwned = false;
    bool bOutpostOwned = false;
    bool bRoadOpen = false;
    bool bHeroAtHeadquarters = false;
    int32 MovesLeft = 0;
};

struct FAIHeroesSupplyQuote
{
    bool bReady = false;
    int32 Units = 0;
    int32 Gold = 0;
    int32 Wood = 0;
    int32 Ore = 0;
    int32 MovementCost = 0;
    FString Reason;

    // Quotes are advisory-only and transient, never authorization.
    FString Summary() const;
};

// Original AIHEROES supply rule: hostile OUTPOST raiders make the HQ convoy
// corridor unsafe. Direct local E recruiting is still allowed at field sites.
// An ordered convoy delivers one canonical troop pack to the hero AT HQ.
class AIHEROES_API FAIHeroesSupplyLines
{
public:
    static constexpr int32 ShippingGoldFee = 15;
    static constexpr int32 ShipmentMoves = 1;

    // Does not modify any input; hides route/inventory when source is unknown.
    static FAIHeroesSupplyQuote Quote(const FAIHeroesSupplyAccess& Access,
        const FAIHeroesFieldDwellings& Sites,
        int32 Gold, int32 Wood, int32 Ore,
        const TArray<FAIHeroesArmyStack>& Army);

    // This is the *only* remote supply mutation API. Rechecks all fields
    // and commits resources, stock, army and movement together.
    static bool Dispatch(const FAIHeroesSupplyAccess& Access,
        FAIHeroesFieldDwellings& Sites, int32& MovesLeft,
        int32& Gold, int32& Wood, int32& Ore,
        TArray<FAIHeroesArmyStack>& Army, FString& Error);
};
