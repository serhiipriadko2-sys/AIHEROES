#include "AIHeroesWorldWeek.h"

namespace
{
const TCHAR* ExpectedNodeId(EAIHeroesNodeType Type)
{
    switch (Type)
    {
        case EAIHeroesNodeType::Headquarters: return TEXT("HQ");
        case EAIHeroesNodeType::ForestMine: return TEXT("FOREST");
        case EAIHeroesNodeType::OreMine: return TEXT("ORE");
        case EAIHeroesNodeType::Outpost: return TEXT("OUTPOST");
        case EAIHeroesNodeType::Ruins: return TEXT("RUINS");
        default: return nullptr;
    }
}

FName ExpectedGuard(EAIHeroesNodeType Type)
{
    switch (Type)
    {
        case EAIHeroesNodeType::ForestMine: return TEXT("Wolf");
        case EAIHeroesNodeType::OreMine: return TEXT("Bandit");
        case EAIHeroesNodeType::Outpost: return TEXT("Raider");
        case EAIHeroesNodeType::Ruins: return TEXT("Guardian");
        default: return NAME_None;
    }
}

bool WorldWeekFail(FString& Error, const TCHAR* Message)
{
    Error = Message;
    return false;
}

bool ValidNode(const FAIHeroesWeekNode& Node, TSet<FName>& Seen)
{
    const TCHAR* Expected = ExpectedNodeId(Node.Type);
    if (!Expected || Node.Id != FName(Expected) || Seen.Contains(Node.Id) ||
        Node.Guards.Num() > 2 ||
        (Node.Type == EAIHeroesNodeType::Headquarters &&
            (!Node.bOwned || !Node.Guards.IsEmpty())))
    {
        return false;
    }
    Seen.Add(Node.Id);

    const FName Allowed = ExpectedGuard(Node.Type);
    for (const FAIHeroesArmyStack& Stack : Node.Guards)
    {
        if (Allowed.IsNone() || Stack.UnitId != Allowed ||
            Stack.UnitHealth <= 0 || Stack.UnitHealth > 500000 ||
            Stack.TotalHealth < 0 ||
            Stack.TotalHealth > FAIHeroesWorldWeekRules::MaxUnitHealth ||
            Stack.Attack < 0 || Stack.Defense < 0 || Stack.Initiative < 0 ||
            (Node.bOwned && Stack.IsAlive()))
        {
            return false;
        }
    }
    return true;
}
}

int32 FAIHeroesWorldWeekRules::WeekNumber(int32 Day)
{
    return Day >= 1 && Day <= MaxDay ? (Day - 1) / DaysPerWeek + 1 : INDEX_NONE;
}

int32 FAIHeroesWorldWeekRules::DayInWeek(int32 Day)
{
    return Day >= 1 && Day <= MaxDay ? (Day - 1) % DaysPerWeek + 1 : INDEX_NONE;
}

bool FAIHeroesWorldWeekRules::IsWeekTransition(int32 CurrentDay)
{
    return CurrentDay >= 1 && CurrentDay < MaxDay &&
        CurrentDay % DaysPerWeek == 0;
}

int32 FAIHeroesWorldWeekRules::WeeklyGrowth(EAIHeroesNodeType Type)
{
    switch (Type)
    {
        // AI HEROES balance tuning, NOT copied HotA troop quantities.
        case EAIHeroesNodeType::ForestMine: return 2;
        case EAIHeroesNodeType::OreMine: return 2;
        case EAIHeroesNodeType::Outpost: return 1;
        case EAIHeroesNodeType::Ruins: return 1;
        default: return 0;
    }
}

bool FAIHeroesWorldWeekRules::PlanNextDay(
    int32 CurrentDay,
    const TArray<FAIHeroesWeekNode>& CurrentNodes,
    FAIHeroesWeekPlan& OutPlan,
    FString& Error)
{
    OutPlan = FAIHeroesWeekPlan();
    Error.Empty();

    if (CurrentDay < 1 || CurrentDay >= MaxDay)
    {
        return WorldWeekFail(Error, TEXT("World week cannot advance past the campaign day boundary."));
    }
    if (CurrentNodes.Num() != 5)
    {
        return WorldWeekFail(Error, TEXT("World week requires the five known campaign nodes."));
    }

    TSet<FName> Seen;
    for (const FAIHeroesWeekNode& Node : CurrentNodes)
    {
        if (!ValidNode(Node, Seen))
        {
            return WorldWeekFail(Error, TEXT("Malformed or duplicate node; week advance denied."));
        }
    }

    for (const EAIHeroesNodeType Type : {
            EAIHeroesNodeType::Headquarters, EAIHeroesNodeType::ForestMine,
            EAIHeroesNodeType::OreMine, EAIHeroesNodeType::Outpost,
            EAIHeroesNodeType::Ruins})
    {
        if (!Seen.Contains(FName(ExpectedNodeId(Type))))
        {
            return WorldWeekFail(Error, TEXT("Campaign zone missing; world-week advance denied."));
        }
    }

    FAIHeroesWeekPlan Proposed;
    Proposed.NewDay = CurrentDay + 1;
    Proposed.Week = WeekNumber(Proposed.NewDay);
    Proposed.DayInWeek = DayInWeek(Proposed.NewDay);
    Proposed.bNewWeek = IsWeekTransition(CurrentDay);
    Proposed.Nodes = CurrentNodes;

    if (Proposed.bNewWeek)
    {
        for (FAIHeroesWeekNode& Node : Proposed.Nodes)
        {
            if (Node.bOwned) continue;
            const int32 Growth = WeeklyGrowth(Node.Type);
            if (Growth == 0) continue;
            for (FAIHeroesArmyStack& Stack : Node.Guards)
            {
                // A destroyed stack stays dead. No resurrection, no fractional
                // healing; reinforcement adds only full fresh unit HP.
                if (!Stack.IsAlive()) continue;
                const int32 Available = FMath::Max(
                    0, MaxGuardsPerStack - Stack.Count());
                const int32 NewUnits = FMath::Min(Growth, Available);
                const int64 FutureHealth = int64(Stack.TotalHealth) +
                    int64(NewUnits) * Stack.UnitHealth;
                if (FutureHealth > MaxUnitHealth)
                {
                    return WorldWeekFail(Error, TEXT("Neutral guard health would exceed save limits."));
                }
                Stack.TotalHealth = static_cast<int32>(FutureHealth);
                Proposed.NewNeutralUnits += NewUnits;
            }
        }
    }

    OutPlan = MoveTemp(Proposed);
    return true;
}
