#include "AIHeroesRecruitmentStock.h"

#include "AIHeroesBaseRules.h"

FAIHeroesRecruitmentStock FAIHeroesRecruitmentStockRules::Initial()
{
    return FAIHeroesRecruitmentStock();
}

bool FAIHeroesRecruitmentStockRules::IsValid(
    const FAIHeroesRecruitmentStock& Stock)
{
    return Stock.Militia >= 0 && Stock.Militia <= MaxMilitia &&
        Stock.Scouts >= 0 && Stock.Scouts <= MaxScouts;
}

int32 FAIHeroesRecruitmentStockRules::MilitiaGrowth(int32 HeadquartersTier)
{
    return FAIHeroesBaseRules::IsValidTier(HeadquartersTier)
        ? 5 + 2 * HeadquartersTier : 0;
}

int32 FAIHeroesRecruitmentStockRules::ScoutGrowth(
    int32 HeadquartersTier, bool bOutpostOwned)
{
    return FAIHeroesBaseRules::IsValidTier(HeadquartersTier)
        ? 2 + HeadquartersTier / 2 + (bOutpostOwned ? 2 : 0) : 0;
}

bool FAIHeroesRecruitmentStockRules::PlanNextDay(
    int32 CurrentDay, int32 HeadquartersTier, bool bOutpostOwned,
    const FAIHeroesRecruitmentStock& Before,
    FAIHeroesRecruitmentStock& Out, FString& Error)
{
    Out = FAIHeroesRecruitmentStock();
    Out.Militia = 0;
    Out.Scouts = 0;
    Error.Empty();
    if (CurrentDay < 1 || CurrentDay >= MaxDay ||
        !FAIHeroesBaseRules::IsValidTier(HeadquartersTier) || !IsValid(Before))
    {
        Error = TEXT("Recruitment production blocked: invalid day, HQ tier or reserves.");
        return false;
    }

    FAIHeroesRecruitmentStock Proposed = Before;
    if (CurrentDay % DaysPerWeek == 0)
    {
        // Saturating arithmetic: no overflow, and the cap is honest in UI.
        Proposed.Militia = FMath::Min(
            MaxMilitia, Before.Militia + MilitiaGrowth(HeadquartersTier));
        Proposed.Scouts = FMath::Min(
            MaxScouts, Before.Scouts +
            ScoutGrowth(HeadquartersTier, bOutpostOwned));
    }

    Out = Proposed;
    return true;
}

int32 FAIHeroesRecruitmentStockRules::Available(
    const FAIHeroesRecruitmentStock& Stock, EAIHeroesRecruitChoice Choice)
{
    if (!IsValid(Stock)) return INDEX_NONE;
    switch (Choice)
    {
        case EAIHeroesRecruitChoice::Militia: return Stock.Militia;
        case EAIHeroesRecruitChoice::Scouts: return Stock.Scouts;
        default: return INDEX_NONE;
    }
}

bool FAIHeroesRecruitmentStockRules::CanBuy(
    const FAIHeroesRecruitmentStock& Stock,
    EAIHeroesRecruitChoice Choice, int32 Count, FString& Error)
{
    Error.Empty();
    const int32 AvailableUnits = Available(Stock, Choice);
    if (AvailableUnits == INDEX_NONE || Count <= 0)
    {
        Error = TEXT("RECRUIT BLOCKED: invalid stock or offer. No resources spent.");
        return false;
    }
    if (AvailableUnits < Count)
    {
        Error = FString::Printf(
            TEXT("RECRUIT BLOCKED: only %d units available, need %d. "
                 "Stock replenishes on day 8, 15, 22... No resources spent."),
            AvailableUnits, Count);
        return false;
    }
    return true;
}

bool FAIHeroesRecruitmentStockRules::Consume(
    FAIHeroesRecruitmentStock& Stock,
    EAIHeroesRecruitChoice Choice, int32 Count, FString& Error)
{
    if (!CanBuy(Stock, Choice, Count, Error)) return false;
    // All preconditions were checked; mutation cannot fail or underflow.
    if (Choice == EAIHeroesRecruitChoice::Militia) Stock.Militia -= Count;
    else Stock.Scouts -= Count;
    return true;
}
