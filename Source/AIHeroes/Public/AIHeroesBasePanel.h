#pragma once

#include "CoreMinimal.h"
#include "AIHeroesTypes.h"
#include "AIHeroesHUDLayout.h"

enum class EAIHeroesBasePurchase : uint8
{
    Headquarters, Militia, Scouts, Protocol, ModelSlot, None = 255
};

struct FAIHeroesBasePanelState
{
    int32 Gold = 0, Wood = 0, Ore = 0, Data = 0;
    int32 BaseTier = 0, ProtocolTier = 0, ModelSlotTier = 0;
    bool bAtSelectedOwnedHQ = false;
    TArray<FAIHeroesArmyStack> Army;
};

struct FAIHeroesBasePanelOffer
{
    EAIHeroesBasePurchase Kind = EAIHeroesBasePurchase::None;
    FString Title;
    FString Detail;
    FString BlockReason;
    int32 Gold = 0, Wood = 0, Ore = 0, Data = 0;
    bool bAvailable = false;
    bool bAffordable = false;
    bool CanBuy() const { return bAvailable && bAffordable && BlockReason.IsEmpty(); }
    FString Price() const;
    bool SameQuote(const FAIHeroesBasePanelOffer& Other) const;
};

class AIHEROES_API FAIHeroesBasePanelRules
{
public:
    static constexpr int32 ToggleAction = 400;
    static constexpr int32 CloseAction = 401;
    static constexpr int32 ConfirmAction = 402;
    static constexpr int32 ConsumeAction = 403;
    static constexpr int32 FirstOfferAction = 410;
    static constexpr int32 OfferCount = 5;
    static TArray<FAIHeroesBasePanelOffer> Offers(const FAIHeroesBasePanelState& State);
    static FAIHeroesHUDActionRect Panel(const FVector2D& Viewport);
    static FAIHeroesHUDActionRect Button(int32 Action, const FVector2D& Viewport);
    static FAIHeroesHUDActionRect OpenButton(const FVector2D& Viewport);
    static int32 HitTest(const FVector2D& Point, const FVector2D& Viewport);
};
