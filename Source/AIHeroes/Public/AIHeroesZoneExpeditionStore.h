#pragma once

#include "CoreMinimal.h"
#include "AIHeroesZoneExpeditionSave.h"

struct FAIHeroesZoneExpeditionLoad
{
    bool bOk = false;
    bool bRecovered = false;
    int32 Sequence = 0;
    FString Slot;
    FString Message;
    FAIHeroesZoneExpeditionState State;
};

// Explicit schema-2 file format and A/B recovery.
// Never sees or mutates any CampaignSave v1 slot. Tests use a GUID
// test-only prefix and delete only the files they created.
class AIHEROES_API FAIHeroesZoneExpeditionStore
{
public:
    static const FString& ProductionPrefix();
    static FString SlotA(const FString& Prefix);
    static FString SlotB(const FString& Prefix);
    static FAIHeroesZoneExpeditionLoad Load(const FString& Prefix);
    static bool Save(const FAIHeroesZoneExpeditionState& State,
        const FString& Prefix, FString& Message);
};
