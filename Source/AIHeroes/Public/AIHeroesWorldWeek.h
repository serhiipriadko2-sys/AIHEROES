#pragma once

#include "CoreMinimal.h"
#include "AIHeroesTypes.h"

// The original AI HEROES week cadence is inspired by the *design principle*
// of strategic weekly rhythms, NOT an implementation of HotA's exact rules.
// World-state input is copied. Only the campaign authority applies the plan.
struct FAIHeroesWeekNode
{
    FName Id = NAME_None;
    EAIHeroesNodeType Type = EAIHeroesNodeType::Headquarters;
    bool bOwned = false;
    TArray<FAIHeroesArmyStack> Guards;
};

struct FAIHeroesWeekPlan
{
    int32 NewDay = 0;
    int32 Week = 0;
    int32 DayInWeek = 0;
    bool bNewWeek = false;
    int32 NewNeutralUnits = 0;
    TArray<FAIHeroesWeekNode> Nodes;
};

// No Actors, randomness, filesystem, save writes or AI execution.
// Fails closed and leaves the caller's game state untouched.
class AIHEROES_API FAIHeroesWorldWeekRules
{
public:
    static constexpr int32 DaysPerWeek = 7;
    static constexpr int32 MaxDay = 1000000;
    static constexpr int32 MaxUnitHealth = 500000;
    static constexpr int32 MaxGuardsPerStack = 40;

    static int32 WeekNumber(int32 Day);
    static int32 DayInWeek(int32 Day);
    static bool IsWeekTransition(int32 CurrentDay);
    static int32 WeeklyGrowth(EAIHeroesNodeType Type);

    // Validate all nodes, calculate a deterministic transition and return
    // a fully prepared snapshot. No in-place changes on failures.
    static bool PlanNextDay(
        int32 CurrentDay,
        const TArray<FAIHeroesWeekNode>& CurrentNodes,
        FAIHeroesWeekPlan& OutPlan,
        FString& Error);
};
