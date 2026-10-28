#pragma once

#include "CoreMinimal.h"

class UAIHeroesCampaignSave;

// The game never executes arbitrary paths: only engine-managed SaveGame slots.
// Returned UObjects are used immediately by GameMode (no async lifetime).
struct FAIHeroesLoadedCampaign
{
    UAIHeroesCampaignSave* Snapshot = nullptr;
    FString SlotName;
    FString Message;
    bool bRecovered = false;
    bool bLegacy = false;
};

// Crash-resilient TWO-slot policy. Always writes the other slot so a good
// prior snapshot survives failed writes; validates and re-reads every save.
// Legacy v1 is read-only and is not deleted or overwritten during migration.
class FAIHeroesSaveSystem
{
public:
    static const FString& CampaignPrefix();
    static FString SlotA(const FString& Prefix);
    static FString SlotB(const FString& Prefix);

    static bool Save(
        UAIHeroesCampaignSave& Snapshot,
        const FString& Prefix,
        const FString& LegacySlot,
        FString& ResultMessage);

    static FAIHeroesLoadedCampaign Load(
        const FString& Prefix,
        const FString& LegacySlot);
};
