#include "AIHeroesFieldDwelling.h"

#include "AIHeroesRecruitmentAdvisor.h"

namespace
{
bool ValidSite(EAIHeroesFieldSite Site)
{
    return Site == EAIHeroesFieldSite::ForestCamp ||
        Site == EAIHeroesFieldSite::OreAerie;
}

FAIHeroesFieldSiteStock& MutableSite(
    FAIHeroesFieldDwellings& State, EAIHeroesFieldSite Site)
{
    return Site == EAIHeroesFieldSite::ForestCamp ? State.Forest : State.Ore;
}

bool ValidFriendlyArmy(const TArray<FAIHeroesArmyStack>& Army)
{
    if (Army.Num() > 2) return false;
    TSet<FName> Seen;
    for (const auto& S : Army)
    {
        if (S.UnitId.IsNone() || Seen.Contains(S.UnitId) ||
            S.TotalHealth < 0 || S.TotalHealth > 500000) return false;
        const bool bMilitia = S.UnitId == FName(TEXT("Militia"));
        const bool bScout = S.UnitId == FName(TEXT("Scout"));
        if ((!bMilitia && !bScout) ||
            S.UnitHealth != (bMilitia ? 12 : 8) ||
            S.Attack != (bMilitia ? 4 : 6) ||
            S.Defense != (bMilitia ? 2 : 1) ||
            S.Initiative != (bMilitia ? 3 : 7)) return false;
        Seen.Add(S.UnitId);
    }
    return true;
}
}

FAIHeroesFieldDwellings FAIHeroesFieldDwellingRules::Initial()
{
    return FAIHeroesFieldDwellings();
}

bool FAIHeroesFieldDwellingRules::IsValid(const FAIHeroesFieldDwellings& S)
{
    return S.Forest.Available >= 0 && S.Forest.Available <= ForestCap &&
        S.Ore.Available >= 0 && S.Ore.Available <= OreCap;
}

FName FAIHeroesFieldDwellingRules::ParentNodeId(EAIHeroesFieldSite Site)
{
    if (!ValidSite(Site)) return NAME_None;
    return Site == EAIHeroesFieldSite::ForestCamp
        ? FName(TEXT("FOREST")) : FName(TEXT("ORE"));
}

int32 FAIHeroesFieldDwellingRules::Starter(EAIHeroesFieldSite Site)
{
    return Site == EAIHeroesFieldSite::ForestCamp ? 5 : 2;
}

int32 FAIHeroesFieldDwellingRules::WeeklyGrowth(EAIHeroesFieldSite Site)
{
    return Site == EAIHeroesFieldSite::ForestCamp ? 5 : 2;
}

int32 FAIHeroesFieldDwellingRules::Capacity(EAIHeroesFieldSite Site)
{
    return Site == EAIHeroesFieldSite::ForestCamp ? ForestCap : OreCap;
}

const FAIHeroesFieldSiteStock& FAIHeroesFieldDwellingRules::At(
    const FAIHeroesFieldDwellings& S, EAIHeroesFieldSite Site)
{
    return Site == EAIHeroesFieldSite::ForestCamp ? S.Forest : S.Ore;
}

bool FAIHeroesFieldDwellingRules::PlanNextDay(int32 CurrentDay,
    bool bForestOwned, bool bOreOwned,
    const FAIHeroesFieldDwellings& Before,
    FAIHeroesFieldDwellings& After, FString& Error)
{
    After = FAIHeroesFieldDwellings();
    After.Forest.Available = 0;
    After.Ore.Available = 0;
    Error.Empty();
    if (CurrentDay < 1 || CurrentDay >= MaxDay || !IsValid(Before))
    {
        Error = TEXT("FIELD DAY BLOCKED: invalid day or dwelling inventory.");
        return false;
    }

    FAIHeroesFieldDwellings Proposed = Before;
    const bool bNewWeek = CurrentDay % 7 == 0;
    for (const EAIHeroesFieldSite Site :
        {EAIHeroesFieldSite::ForestCamp, EAIHeroesFieldSite::OreAerie})
    {
        FAIHeroesFieldSiteStock& Stock = MutableSite(Proposed, Site);
        const bool bParentOwned = Site == EAIHeroesFieldSite::ForestCamp
            ? bForestOwned : bOreOwned;
        if (!bParentOwned)
        {
            Stock.bClaimed = false; // Lost terrain cannot retain a dwelling.
        }
        if (!bNewWeek) continue;
        if (Stock.bClaimed)
            Stock.Available = FMath::Min(
                Capacity(Site), Stock.Available + WeeklyGrowth(Site));
        else
            Stock.Available = Starter(Site); // Neutral resets, not accumulates.
    }
    After = Proposed;
    return true;
}

bool FAIHeroesFieldDwellingRules::TryClaim(EAIHeroesFieldSite Site,
    bool bTerritoryOwned, bool bHeroAtSite,
    FAIHeroesFieldDwellings& State, FString& Error)
{
    Error.Empty();
    if (!ValidSite(Site) || !IsValid(State) || !bTerritoryOwned || !bHeroAtSite)
    {
        Error = TEXT("FIELD CLAIM BLOCKED: clear the site and bring the hero here. No changes.");
        return false;
    }
    auto& Stock = MutableSite(State, Site);
    if (Stock.bClaimed)
    {
        Error = TEXT("FIELD CLAIM: already owned. E now recruits available units.");
        return false;
    }
    Stock.bClaimed = true;
    return true;
}

bool FAIHeroesFieldDwellingRules::TryRecruit(EAIHeroesFieldSite Site,
    bool bTerritoryOwned, bool bHeroAtSite,
    FAIHeroesFieldDwellings& State,
    int32& Gold, int32& Wood, int32& Ore,
    TArray<FAIHeroesArmyStack>& Army, FString& Error)
{
    Error.Empty();
    if (!ValidSite(Site) || !IsValid(State) || !bTerritoryOwned || !bHeroAtSite ||
        !At(State, Site).bClaimed)
    {
        Error = TEXT("FIELD RECRUIT BLOCKED: dwelling is not secured and visited. Nothing spent.");
        return false;
    }
    return BuyAuthorized(Site, State, Gold, Wood, Ore, Army, 0, Error);
}

bool FAIHeroesFieldDwellingRules::BuyAuthorized(EAIHeroesFieldSite Site,
    FAIHeroesFieldDwellings& State,
    int32& Gold, int32& Wood, int32& Ore,
    TArray<FAIHeroesArmyStack>& Army, int32 ExtraGold, FString& Error)
{
    Error.Empty();
    if (!ValidSite(Site) || !IsValid(State) || !At(State, Site).bClaimed ||
        ExtraGold < 0 || ExtraGold > 1000)
    {
        Error = TEXT("FIELD RECRUIT BLOCKED: invalid authorization or stock. Nothing spent.");
        return false;
    }
    if (Gold < 0 || Gold > 500000000 || Wood < 0 || Wood > 500000000 ||
        Ore < 0 || Ore > 500000000 || !ValidFriendlyArmy(Army))
    {
        Error = TEXT("FIELD RECRUIT BLOCKED: invalid resources or army. Nothing spent.");
        return false;
    }
    const EAIHeroesRecruitChoice Choice = Site == EAIHeroesFieldSite::ForestCamp
        ? EAIHeroesRecruitChoice::Militia : EAIHeroesRecruitChoice::Scouts;
    const FAIHeroesRecruitOffer Offer = FAIHeroesRecruitmentAdvisor::GetOffer(Choice);
    const int32 Count = Offer.Units.Count();
    if (At(State, Site).Available < Count)
    {
        Error = FString::Printf(
            TEXT("FIELD RECRUIT BLOCKED: only %d/%d units at this site. "
                 "New supply arrives next week. Nothing spent."),
            At(State, Site).Available, Count);
        return false;
    }
    if (Gold < Offer.Gold + ExtraGold || Wood < Offer.Wood || Ore < Offer.Ore)
    {
        Error = FString::Printf(
            TEXT("FIELD RECRUIT BLOCKED: need %dG/%dW/%dO. Nothing spent."),
            Offer.Gold + ExtraGold, Offer.Wood, Offer.Ore);
        return false;
    }

    // Preflight the full resulting army before touching any caller state.
    TArray<FAIHeroesArmyStack> ProposedArmy = Army;
    bool bMerged = false;
    for (auto& Stack : ProposedArmy)
    {
        if (Stack.UnitId == Offer.Units.UnitId)
        {
            if (Stack.TotalHealth > 500000 - Offer.Units.TotalHealth)
            {
                Error = TEXT("FIELD RECRUIT BLOCKED: army stack capacity. Nothing spent.");
                return false;
            }
            Stack.TotalHealth += Offer.Units.TotalHealth;
            bMerged = true;
            break;
        }
    }
    if (!bMerged)
    {
        if (ProposedArmy.Num() >= 2)
        {
            Error = TEXT("FIELD RECRUIT BLOCKED: no free army stack. Nothing spent.");
            return false;
        }
        ProposedArmy.Add(Offer.Units);
    }

    // All preconditions passed. No fallible operations after this point.
    Gold -= Offer.Gold + ExtraGold;
    Wood -= Offer.Wood;
    Ore -= Offer.Ore;
    MutableSite(State, Site).Available -= Count;
    Army = MoveTemp(ProposedArmy);
    return true;
}
