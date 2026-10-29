#pragma once

#include "CoreMinimal.h"
#include "AIHeroesBasePanel.h"

// Display-only Russian copy for the first playable campaign slice.
// This never authorizes purchases or changes a quoted price.
class AIHEROES_API FAIHeroesBasePanelCopy
{
public:
    static FString Title(EAIHeroesBasePurchase Kind, int32 BaseTier,
        int32 ProtocolTier, int32 ModelSlotTier);
    static FString Detail(EAIHeroesBasePurchase Kind, int32 MilitiaReserve,
        int32 ScoutReserve);
    static FString Price(const FAIHeroesBasePanelOffer& Offer);
    static FString Reason(const FAIHeroesBasePanelOffer& Offer);
    static FString Status(const FAIHeroesBasePanelOffer& Offer);
    static FString Message(const FString& Raw,
        const FAIHeroesBasePanelOffer& Selected);
};
