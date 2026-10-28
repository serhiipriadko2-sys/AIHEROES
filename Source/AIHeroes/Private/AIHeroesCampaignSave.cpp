#include "AIHeroesCampaignSave.h"

const FString& UAIHeroesCampaignSave::SlotName()
{
    static const FString Slot(TEXT("AIHEROES_Campaign_v1"));
    return Slot;
}
