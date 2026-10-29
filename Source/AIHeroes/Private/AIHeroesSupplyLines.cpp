#include "AIHeroesSupplyLines.h"

#include "AIHeroesRecruitmentAdvisor.h"

namespace
{
bool ValidSite(EAIHeroesFieldSite Site)
{
    return Site == EAIHeroesFieldSite::ForestCamp ||
        Site == EAIHeroesFieldSite::OreAerie;
}
}

FString FAIHeroesSupplyQuote::Summary() const
{
    if (!bReady) return Reason;
    return FString::Printf(
        TEXT("CONVOY READY: %d troops; %dG %dW %dO, %d move. X to preview/confirm."),
        Units, Gold, Wood, Ore, MovementCost);
}

FAIHeroesSupplyQuote FAIHeroesSupplyLines::Quote(
    const FAIHeroesSupplyAccess& Access,
    const FAIHeroesFieldDwellings& Sites,
    int32 Gold, int32 Wood, int32 Ore,
    const TArray<FAIHeroesArmyStack>& Army)
{
    FAIHeroesSupplyQuote Out;
    Out.Reason = TEXT("CONVOY BLOCKED: invalid supply context. No changes.");
    if (!ValidSite(Access.Site)) return Out;
    // Do not reveal even the existence of an unknown, unseen field site.
    if (!Access.bSourceDiscovered)
    {
        Out.Reason = TEXT("CONVOY UNKNOWN: scout this territory first. Nothing spent.");
        return Out;
    }
    if (!Access.bHeadquartersOwned || !Access.bSourceOwned)
    {
        Out.Reason = TEXT("CONVOY BLOCKED: own HQ and the destination territory first.");
        return Out;
    }
    if (!FAIHeroesFieldDwellingRules::IsValid(Sites))
    {
        Out.Reason = TEXT("CONVOY BLOCKED: invalid field stock; nothing changed.");
        return Out;
    }
    if (!FAIHeroesFieldDwellingRules::At(Sites, Access.Site).bClaimed)
    {
        Out.Reason = TEXT("CONVOY BLOCKED: visit and claim the field site with 3 first.");
        return Out;
    }
    if (!Access.bRoadOpen)
    {
        Out.Reason = TEXT("CONVOY BLOCKED: no valid HQ road to selected field.");
        return Out;
    }
    if (!Access.bOutpostOwned)
    {
        Out.Reason = TEXT("CONVOY BLOCKED: OUTPOST raiders threaten supply. Secure OUTPOST first.");
        return Out;
    }
    if (!Access.bHeroAtHeadquarters)
    {
        Out.Reason = TEXT("CONVOY BLOCKED: return hero to HQ, keep source selected.");
        return Out;
    }
    if (Access.MovesLeft < ShipmentMoves || Access.MovesLeft > 4)
    {
        Out.Reason = TEXT("CONVOY BLOCKED: need one remaining move this day.");
        return Out;
    }

    // Same canonical pack and army validation as local E. Evaluate a staged
    // copy so displaying a price can never consume resources or recruits.
    FAIHeroesFieldDwellings TestSites = Sites;
    TArray<FAIHeroesArmyStack> TestArmy = Army;
    int32 TestGold = Gold, TestWood = Wood, TestOre = Ore;
    FString Error;
    if (!FAIHeroesFieldDwellingRules::BuyAuthorized(
            Access.Site, TestSites, TestGold, TestWood, TestOre,
            TestArmy, ShippingGoldFee, Error))
    {
        Out.Reason = TEXT("CONVOY BLOCKED: ") + Error;
        return Out;
    }

    const EAIHeroesRecruitChoice Choice =
        Access.Site == EAIHeroesFieldSite::ForestCamp
        ? EAIHeroesRecruitChoice::Militia : EAIHeroesRecruitChoice::Scouts;
    const FAIHeroesRecruitOffer Offer =
        FAIHeroesRecruitmentAdvisor::GetOffer(Choice);
    Out.Units = Offer.Units.Count();
    Out.Gold = Offer.Gold + ShippingGoldFee;
    Out.Wood = Offer.Wood;
    Out.Ore = Offer.Ore;
    Out.MovementCost = ShipmentMoves;
    Out.bReady = true;
    Out.Reason.Empty();
    return Out;
}

bool FAIHeroesSupplyLines::Dispatch(
    const FAIHeroesSupplyAccess& Access,
    FAIHeroesFieldDwellings& Sites, int32& MovesLeft,
    int32& Gold, int32& Wood, int32& Ore,
    TArray<FAIHeroesArmyStack>& Army, FString& Error)
{
    Error.Empty();
    // Reject a stale caller snapshot even if its quoted movement was valid.
    if (Access.MovesLeft != MovesLeft)
    {
        Error = TEXT("CONVOY BLOCKED: stale movement budget. Quote again.");
        return false;
    }
    const FAIHeroesSupplyQuote QuoteNow = Quote(
        Access, Sites, Gold, Wood, Ore, Army);
    if (!QuoteNow.bReady)
    {
        Error = QuoteNow.Reason;
        return false;
    }

    FAIHeroesFieldDwellings NextSites = Sites;
    TArray<FAIHeroesArmyStack> NextArmy = Army;
    int32 NextGold = Gold, NextWood = Wood, NextOre = Ore;
    if (!FAIHeroesFieldDwellingRules::BuyAuthorized(
            Access.Site, NextSites, NextGold, NextWood, NextOre,
            NextArmy, ShippingGoldFee, Error))
    {
        return false;
    }

    // One commit boundary, no fallible operations after this point.
    Sites = MoveTemp(NextSites);
    Gold = NextGold;
    Wood = NextWood;
    Ore = NextOre;
    Army = MoveTemp(NextArmy);
    MovesLeft -= ShipmentMoves;
    return true;
}
