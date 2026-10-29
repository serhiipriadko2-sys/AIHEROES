#pragma once

#include "CoreMinimal.h"
#include "AIHeroesTypes.h"

struct FAIHeroesBattleRewardPlan
{
    int32 GoldAfter = 0;
    int32 DataAfter = 0;
    int32 GoldReward = 0;
    int32 DataReward = 0;
};

// Pure preflight for the campaign transaction, not combat simulation.
class AIHEROES_API FAIHeroesBattleRewardRules
{
public:
    // No saturation. Invalid inputs/overflow preserve OutPlan; no campaign writes.
    // Defeat and stalemate use bVictory=false and grant no rewards.
    static bool Plan(bool bVictory, EAIHeroesNodeType NodeType,
        int32 Gold, int32 Data, FAIHeroesBattleRewardPlan& OutPlan, FString& Error);
};
