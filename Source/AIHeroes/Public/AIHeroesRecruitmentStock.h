#pragma once

#include "CoreMinimal.h"
#include "AIHeroesRecruitmentAdvisor.h"

// New AIHEROES-only resource contract, inspired by the strategic cadence of
// week-based recruitment; no HotA creature counts, names or assets are copied.
struct FAIHeroesRecruitmentStock
{
    int32 Militia = 10; // Two starting five-unit purchase packs.
    int32 Scouts = 4;  // Two starting two-unit purchase packs.

    bool operator==(const FAIHeroesRecruitmentStock& Other) const
    {
        return Militia == Other.Militia && Scouts == Other.Scouts;
    }
};

// Pure read-only planning and bounded inventory mutation. No UObject/Actors,
// disk, orders, player money, SaveGame or actual model inference.
class AIHEROES_API FAIHeroesRecruitmentStockRules
{
public:
    static constexpr int32 MaxMilitia = 250;
    static constexpr int32 MaxScouts = 120;
    static constexpr int32 DaysPerWeek = 7;
    static constexpr int32 MaxDay = 1000000;

    static FAIHeroesRecruitmentStock Initial();
    static bool IsValid(const FAIHeroesRecruitmentStock& Stock);

    static int32 MilitiaGrowth(int32 HeadquartersTier);
    static int32 ScoutGrowth(int32 HeadquartersTier, bool bOutpostOwned);

    // Day 7 -> 8, 14 -> 15, etc.; no growth on other day transitions.
    // Unowned OUTPOST produces no player recruits. Separate neutral guard
    // reinforcement remains authoritative in FAIHeroesWorldWeekRules.
    // On failure Out remains empty/default; caller NEVER commits partial data.
    static bool PlanNextDay(
        int32 CurrentDay, int32 HeadquartersTier, bool bOutpostOwned,
        const FAIHeroesRecruitmentStock& Before,
        FAIHeroesRecruitmentStock& Out, FString& Error);

    static int32 Available(
        const FAIHeroesRecruitmentStock& Stock, EAIHeroesRecruitChoice Choice);

    // Exact accepted offer quantity, not an inferred "count of purchases".
    static bool CanBuy(
        const FAIHeroesRecruitmentStock& Stock,
        EAIHeroesRecruitChoice Choice, int32 Count, FString& Error);
    static bool Consume(
        FAIHeroesRecruitmentStock& Stock,
        EAIHeroesRecruitChoice Choice, int32 Count, FString& Error);
};
