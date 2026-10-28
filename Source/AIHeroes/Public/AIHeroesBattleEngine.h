#pragma once

#include "CoreMinimal.h"
#include "AIHeroesTypes.h"

struct FAIHeroesBattleOutcome
{
    bool bVictory = false;
    bool bDefeat = false;
    int32 Rounds = 0;
    TArray<FAIHeroesArmyStack> PlayerAfter;
    TArray<FAIHeroesArmyStack> EnemyAfter;
    TArray<FString> Trace;
    FString Summary;
};

class FAIHeroesBattleEngine
{
public:
    // No randomness, no LLM bypass: same input + tier => same result.
    static FAIHeroesBattleOutcome Resolve(
        const TArray<FAIHeroesArmyStack>& PlayerArmy,
        const TArray<FAIHeroesArmyStack>& EnemyArmy,
        int32 ProtocolTier);

private:
    static int32 ChooseTarget(
        const FAIHeroesArmyStack& ActingStack,
        const TArray<FAIHeroesArmyStack>& Opponents,
        int32 StrategyTier);
    static bool AnyAlive(const TArray<FAIHeroesArmyStack>& Army);
};
