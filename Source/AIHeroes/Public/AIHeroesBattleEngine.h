#pragma once

#include "CoreMinimal.h"
#include "AIHeroesTypes.h"

// A pure, deterministic combat receipt. Every animation is derived from this
// record, never the other way around. Untrusted AI output cannot author it.
struct FAIHeroesBattleAction
{
    int32 Round = 0;
    bool bPlayerAttacker = false;
    int32 AttackerStackIndex = INDEX_NONE;
    int32 TargetStackIndex = INDEX_NONE;
    FName AttackerId = NAME_None;
    FName TargetId = NAME_None;
    int32 Damage = 0;
    int32 TargetHealthBefore = 0;
    int32 TargetHealthAfter = 0;

    FString ToLogLine() const;
};

struct FAIHeroesBattleOutcome
{
    bool bVictory = false;
    bool bDefeat = false;
    int32 Rounds = 0;
    TArray<FAIHeroesArmyStack> PlayerAfter;
    TArray<FAIHeroesArmyStack> EnemyAfter;
    TArray<FAIHeroesBattleAction> Actions;
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

    // Independent integrity gate for battle playback and future replay export.
    // Replays never write authority back into the campaign.
    static bool VerifyReplay(
        const TArray<FAIHeroesArmyStack>& PlayerInitial,
        const TArray<FAIHeroesArmyStack>& EnemyInitial,
        const FAIHeroesBattleOutcome& Outcome);

private:
    static int32 ChooseTarget(
        const FAIHeroesArmyStack& ActingStack,
        const TArray<FAIHeroesArmyStack>& Opponents,
        int32 StrategyTier);
    static bool AnyAlive(const TArray<FAIHeroesArmyStack>& Army);
};
