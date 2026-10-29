#include "AIHeroesBattleRewardRules.h"

#include "AIHeroesBaseRules.h"

bool FAIHeroesBattleRewardRules::Plan(bool bVictory, EAIHeroesNodeType NodeType,
    int32 Gold, int32 Data, FAIHeroesBattleRewardPlan& OutPlan, FString& Error)
{
    if (Gold < 0 || Gold > FAIHeroesBaseRules::MaxResource ||
        Data < 0 || Data > FAIHeroesBaseRules::MaxResource)
    {
        Error = TEXT("Battle resources are outside the campaign's saveable range.");
        return false;
    }
    switch (NodeType)
    {
    case EAIHeroesNodeType::ForestMine:
    case EAIHeroesNodeType::OreMine:
    case EAIHeroesNodeType::Outpost:
    case EAIHeroesNodeType::Ruins:
        break;
    default:
        Error = TEXT("Battle rewards require a supported combat site.");
        return false;
    }

    FAIHeroesBattleRewardPlan Candidate;
    Candidate.GoldReward = bVictory && NodeType == EAIHeroesNodeType::Ruins ? 100 : 0;
    Candidate.DataReward = bVictory ? (NodeType == EAIHeroesNodeType::Ruins ? 4 : 2) : 0;
    const int64 GoldAfter = static_cast<int64>(Gold) + Candidate.GoldReward;
    const int64 DataAfter = static_cast<int64>(Data) + Candidate.DataReward;
    if (GoldAfter > FAIHeroesBaseRules::MaxResource ||
        DataAfter > FAIHeroesBaseRules::MaxResource)
    {
        Error = TEXT("Battle rewards would exceed the campaign's saveable resource limit. "
            "Spend resources before attacking; no battle result was applied.");
        return false;
    }
    Candidate.GoldAfter = static_cast<int32>(GoldAfter);
    Candidate.DataAfter = static_cast<int32>(DataAfter);
    OutPlan = Candidate;
    Error.Empty();
    return true;
}
