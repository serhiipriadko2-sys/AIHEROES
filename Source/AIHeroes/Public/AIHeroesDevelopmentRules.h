#pragma once

#include "CoreMinimal.h"

// One price source for the existing hotkeys and the new base-management UI.
struct FAIHeroesDevelopmentOffer
{
    FString Name;
    int32 Gold = 0;
    int32 Wood = 0;
    int32 Ore = 0;
    int32 Data = 0;
    bool bAvailable = false;
};

class AIHEROES_API FAIHeroesDevelopmentRules
{
public:
    static FAIHeroesDevelopmentOffer Protocol(int32 Tier);
    static FAIHeroesDevelopmentOffer ModelSlot(int32 Tier);
    // Invalid tiers/resources fail BEFORE any reference argument changes.
    static bool TryProtocol(int32& Tier, int32& Gold, int32& Wood,
        int32& Ore, FString& Error);
    static bool TryModelSlot(int32& Tier, int32& Gold, int32& Data, FString& Error);
};
