#include "AIHeroesBasePanel.h"
#include "AIHeroesBaseRules.h"
#include "AIHeroesDevelopmentRules.h"
#include "AIHeroesRecruitmentAdvisor.h"
#include "AIHeroesRecruitmentStock.h"

namespace
{
bool ValidResource(int32 Value)
{
    return Value >= 0 && Value <= FAIHeroesBaseRules::MaxResource;
}

bool ValidArmy(const TArray<FAIHeroesArmyStack>& Army)
{
    if (Army.Num() > 2) return false;
    TSet<FName> Seen;
    for (const auto& S : Army)
    {
        const auto Choice = S.UnitId == FName(TEXT("Militia"))
            ? EAIHeroesRecruitChoice::Militia : EAIHeroesRecruitChoice::Scouts;
        const auto Expected = FAIHeroesRecruitmentAdvisor::GetOffer(Choice).Units;
        if (Seen.Contains(S.UnitId) || S.UnitId != Expected.UnitId ||
            S.UnitHealth != Expected.UnitHealth || S.Attack != Expected.Attack ||
            S.Defense != Expected.Defense || S.Initiative != Expected.Initiative ||
            S.TotalHealth < 0 || S.TotalHealth > 500000) return false;
        Seen.Add(S.UnitId);
    }
    return true;
}
}

FString FAIHeroesBasePanelOffer::Price() const
{
    return FString::Printf(TEXT("%d G / %d W / %d O / %d DATA"), Gold, Wood, Ore, Data);
}

bool FAIHeroesBasePanelOffer::SameQuote(const FAIHeroesBasePanelOffer& Other) const
{
    return Kind == Other.Kind && Title == Other.Title && Detail == Other.Detail &&
        Gold == Other.Gold && Wood == Other.Wood && Ore == Other.Ore && Data == Other.Data &&
        bAvailable == Other.bAvailable && bAffordable == Other.bAffordable &&
        BlockReason == Other.BlockReason;
}

TArray<FAIHeroesBasePanelOffer> FAIHeroesBasePanelRules::Offers(const FAIHeroesBasePanelState& S)
{
    TArray<FAIHeroesBasePanelOffer> Out;
    Out.SetNum(OfferCount);
    for (int32 I = 0; I < OfferCount; ++I) Out[I].Kind = static_cast<EAIHeroesBasePurchase>(I);
    auto& HQ = Out[0];
    const auto Base = FAIHeroesBaseRules::UpgradeCost(S.BaseTier);
    HQ.Title = FString::Printf(TEXT("HEADQUARTERS L%d -> L%d"), S.BaseTier, FMath::Min(3, S.BaseTier + 1));
    HQ.Detail = TEXT("Construction: +20 gold per next day. Combat and model slots unchanged.");
    HQ.Gold = Base.Gold; HQ.Wood = Base.Wood; HQ.Ore = Base.Ore;
    HQ.bAvailable = Base.bAvailable;
    if (!Base.bAvailable) HQ.BlockReason = TEXT("Maximum headquarters level reached.");
    else if (!S.bAtSelectedOwnedHQ) HQ.BlockReason = TEXT("Select owned HQ and return the hero there first.");

    for (int32 I = 1; I <= 2; ++I)
    {
        auto& O = Out[I];
        const auto Recruit = FAIHeroesRecruitmentAdvisor::GetOffer(I == 1
            ? EAIHeroesRecruitChoice::Militia : EAIHeroesRecruitChoice::Scouts);
        O.Title = I == 1 ? TEXT("ARMY: 5 MILITIA") : TEXT("ARMY: 2 SCOUTS");
        O.Detail = I == 1 ? TEXT("12 HP each, attack 4, defense 2, initiative 3. Adds to your current army.")
            : TEXT("8 HP each, attack 6, defense 1, initiative 7. Ranged in Grid V2.");
        O.Gold = Recruit.Gold; O.Wood = Recruit.Wood; O.Ore = Recruit.Ore;
        const int32 Available = I == 1 ? S.MilitiaReserve : S.ScoutReserve;
        O.Detail += FString::Printf(TEXT(" | RESERVE %d / %d needed."),
            Available, Recruit.Units.Count());
        O.bAvailable = true;
        if (Available < Recruit.Units.Count())
            O.BlockReason = TEXT("No recruits in stock. Next growth: days 8, 15, 22...");
        for (const auto& Stack : S.Army)
            if (Stack.UnitId == Recruit.Units.UnitId && Stack.TotalHealth > 500000 - Recruit.Units.TotalHealth)
                O.BlockReason = TEXT("Army stack limit reached. Nothing will be spent.");
    }
    auto& Protocol = Out[3];
    const auto P = FAIHeroesDevelopmentRules::Protocol(S.ProtocolTier);
    Protocol.Title = P.bAvailable ? TEXT("PROTOCOL: ") + P.Name : TEXT("PROTOCOLS: MAXIMUM");
    Protocol.Detail = TEXT("Changes the offline hero policy. This does not install a language model.");
    Protocol.Gold = P.Gold; Protocol.Wood = P.Wood; Protocol.Ore = P.Ore;
    Protocol.bAvailable = P.bAvailable;
    if (!P.bAvailable) Protocol.BlockReason = TEXT("All three prototype protocols are unlocked.");
    auto& Slot = Out[4];
    const auto M = FAIHeroesDevelopmentRules::ModelSlot(S.ModelSlotTier);
    Slot.Title = M.bAvailable ? M.Name : TEXT("MODEL SLOT: 8B MAXIMUM");
    Slot.Detail = TEXT("Capacity theme only. NO MODEL WEIGHTS LOADED. No combat boost from a slot.");
    Slot.Gold = M.Gold; Slot.Data = M.Data;
    Slot.bAvailable = M.bAvailable;
    if (!M.bAvailable) Slot.BlockReason = TEXT("Highest prototype model slot already unlocked.");

    const bool bValid = ValidResource(S.Gold) && ValidResource(S.Wood) &&
        ValidResource(S.Ore) && ValidResource(S.Data) &&
        FAIHeroesRecruitmentStockRules::IsValid(
            FAIHeroesRecruitmentStock{S.MilitiaReserve, S.ScoutReserve}) &&
        FAIHeroesBaseRules::IsValidTier(S.BaseTier) &&
        S.ProtocolTier >= 0 && S.ProtocolTier <= 3 && S.ModelSlotTier >= 0 && S.ModelSlotTier <= 2 && ValidArmy(S.Army);
    for (auto& O : Out)
    {
        if (!bValid)
        {
            O.bAvailable = false;
            O.BlockReason = TEXT("Invalid campaign data. Purchase blocked.");
        }
        O.bAffordable = bValid && S.Gold >= O.Gold && S.Wood >= O.Wood && S.Ore >= O.Ore && S.Data >= O.Data;
        if (O.bAvailable && !O.bAffordable && O.BlockReason.IsEmpty())
            O.BlockReason = TEXT("Not enough resources. No partial purchase.");
    }
    return Out;
}

FAIHeroesHUDActionRect FAIHeroesBasePanelRules::Panel(const FVector2D& V)
{
    if (!FMath::IsFinite(V.X) || !FMath::IsFinite(V.Y) ||
        V.X < 320 || V.Y < 420 || V.X > 100000 || V.Y > 100000) return {};
    FAIHeroesHUDActionRect R;
    R.W = FMath::Min(1040.f, float(V.X) - 24.f);
    R.H = FMath::Min(640.f, float(V.Y) - 24.f);
    R.X = (V.X - R.W) * 0.5f; R.Y = (V.Y - R.H) * 0.5f;
    return R;
}

FAIHeroesHUDActionRect FAIHeroesBasePanelRules::Button(int32 Action, const FVector2D& V)
{
    const auto P = Panel(V);
    FAIHeroesHUDActionRect R;
    if (P.W <= 0) return R;
    if (Action >= FirstOfferAction && Action < FirstOfferAction + OfferCount)
    {
        const float Row = (P.H - 220.f) / OfferCount;
        R.X = P.X + 16; R.Y = P.Y + 100 + (Action - FirstOfferAction) * Row;
        R.W = P.W - 32; R.H = Row - 6;
    }
    else if (Action == CloseAction || Action == ConfirmAction)
    {
        R.W = (P.W - 48) * 0.5f; R.H = 36;
        R.X = P.X + 16 + (Action == CloseAction ? R.W + 16 : 0);
        R.Y = P.Y + P.H - 48;
    }
    return R;
}

FAIHeroesHUDActionRect FAIHeroesBasePanelRules::OpenButton(const FVector2D& V)
{
    if (Panel(V).W <= 0) return {};
    FAIHeroesHUDActionRect R;
    R.W = 112; R.H = 27;
    R.X = V.X - 262; // Beside DOSSIER, not over it or over strategic actions.
    R.Y = V.Y - FAIHeroesHUDLayout::BottomPanelHeight(V.X) + 80;
    return R;
}

int32 FAIHeroesBasePanelRules::HitTest(const FVector2D& Point, const FVector2D& V)
{
    for (int32 Action = FirstOfferAction; Action < FirstOfferAction + OfferCount; ++Action)
        if (Button(Action, V).Contains(Point)) return Action;
    if (Button(CloseAction, V).Contains(Point)) return CloseAction;
    if (Button(ConfirmAction, V).Contains(Point)) return ConfirmAction;
    return ConsumeAction; // No click-through even outside the modal or on invalid geometry.
}
