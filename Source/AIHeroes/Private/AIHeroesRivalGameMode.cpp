#include "AIHeroesGameMode.h"

#include "AIHeroesBaseRules.h"
#include "AIHeroesWorldNode.h"
#include "AIHeroesWorldWeek.h"

bool AAIHeroesGameMode::BuildRivalSnapshot(
    int32 NewDay, int32 NewGold, int32 NewWood, int32 NewOre,
    FAIHeroesRivalSnapshot& Out, FString& Error) const
{
    Out = FAIHeroesRivalSnapshot();
    Error.Empty();
    Out.Day = NewDay;
    Out.Gold = NewGold;
    Out.Wood = NewWood;
    Out.Ore = NewOre;

    int32 Outposts = 0;
    int32 Forests = 0;
    int32 OreMines = 0;
    for (const AAIHeroesWorldNode* Node : Nodes)
    {
        if (!IsValid(Node))
        {
            Error = TEXT("NPC cannot process a missing map node.");
            return false;
        }
        const FName Id(*Node->GetDisplayName());
        if (Id == FName(TEXT("OUTPOST")) &&
            Node->NodeType == EAIHeroesNodeType::Outpost)
        {
            ++Outposts;
            Out.bOutpostPresent = true;
            Out.bOutpostOwned = Node->bOwned;
            Out.OutpostDefenders = Node->Defenders;
        }
        else if (Id == FName(TEXT("FOREST")) &&
                 Node->NodeType == EAIHeroesNodeType::ForestMine)
        {
            ++Forests;
            Out.bForestOwned = Node->bOwned;
        }
        else if (Id == FName(TEXT("ORE")) &&
                 Node->NodeType == EAIHeroesNodeType::OreMine)
        {
            ++OreMines;
            Out.bOreOwned = Node->bOwned;
        }
    }
    if (Outposts != 1 || Forests != 1 || OreMines != 1)
    {
        Error = TEXT("NPC cannot process a duplicate or missing economic node.");
        return false;
    }
    return true;
}

FString AAIHeroesGameMode::GetRivalForecast() const
{
    if (Day < 1 || Day >= FAIHeroesBaseRules::MaxDay ||
        !FAIHeroesBaseRules::IsValidTier(BaseTier))
    {
        return TEXT("RIVAL: preview unavailable at campaign day limit.");
    }
    int64 NextGold = int64(Gold) + FAIHeroesBaseRules::DailyGold(BaseTier);
    int64 NextWood = Wood;
    int64 NextOre = Ore;
    for (const AAIHeroesWorldNode* Node : Nodes)
    {
        if (!IsValid(Node)) return TEXT("RIVAL: map integrity check required.");
        if (!Node->bOwned) continue;
        if (Node->NodeType == EAIHeroesNodeType::ForestMine) NextWood += 15;
        else if (Node->NodeType == EAIHeroesNodeType::OreMine) NextOre += 12;
        else if (Node->NodeType == EAIHeroesNodeType::Outpost) NextGold += 30;
    }
    if (NextGold > FAIHeroesBaseRules::MaxResource ||
        NextWood > FAIHeroesBaseRules::MaxResource ||
        NextOre > FAIHeroesBaseRules::MaxResource)
    {
        return TEXT("RIVAL: preview unavailable at resource limit.");
    }

    FAIHeroesRivalSnapshot Input;
    FString Error;
    if (!BuildRivalSnapshot(Day + 1,
        static_cast<int32>(NextGold), static_cast<int32>(NextWood),
        static_cast<int32>(NextOre), Input, Error))
    {
        return TEXT("RIVAL: invalid map; next day will be blocked.");
    }

    // Match the exact same weekly-preflight order used by EndTurn; a
    // warning must not predict a troop count from stale pre-week guards.
    if (FAIHeroesWorldWeekRules::IsWeekTransition(Day))
    {
        TArray<FAIHeroesWeekNode> Before;
        Before.Reserve(Nodes.Num());
        for (const AAIHeroesWorldNode* Node : Nodes)
        {
            FAIHeroesWeekNode Entry;
            Entry.Id = FName(*Node->GetDisplayName());
            Entry.Type = Node->NodeType;
            Entry.bOwned = Node->bOwned;
            Entry.Guards = Node->Defenders;
            Before.Add(MoveTemp(Entry));
        }
        FAIHeroesWeekPlan Plan;
        if (!FAIHeroesWorldWeekRules::PlanNextDay(Day, Before, Plan, Error))
        {
            return TEXT("RIVAL: world-week validation required.");
        }
        for (const FAIHeroesWeekNode& Entry : Plan.Nodes)
        {
            if (Entry.Id == FName(TEXT("OUTPOST")))
            {
                Input.OutpostDefenders = Entry.Guards;
            }
        }
    }

    FAIHeroesRivalTurn Turn;
    if (!FAIHeroesRivalStrategy::Plan(Input, Turn, Error))
    {
        return TEXT("RIVAL: invalid garrison; next day will be blocked.");
    }
    if (Input.bOutpostOwned)
    {
        return TEXT("RIVAL: OUTPOST CAPTURED | enemy recruitment and raids disabled.");
    }
    return Turn.IsActive()
        ? FString::Printf(TEXT("NEXT RIVAL TURN: %s"), *Turn.Description)
        : FString::Printf(TEXT("RIVAL: OUTPOST scouting | day %d has no scheduled hostile action."),
            Input.Day);
}
