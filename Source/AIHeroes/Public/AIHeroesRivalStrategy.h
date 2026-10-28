#pragma once

#include "CoreMinimal.h"
#include "AIHeroesTypes.h"

// An original, bounded NPC faction. The human still authorizes every hero
// move, recruitment and attack. NPC turns are resolved only at EndTurn.
enum class EAIHeroesRivalAction : uint8
{
    None, RecruitAtOutpost, RaidForest, RaidOre
};

struct FAIHeroesRivalSnapshot
{
    int32 Day = 1;
    int32 Gold = 0;
    int32 Wood = 0;
    int32 Ore = 0;
    bool bOutpostPresent = false;
    bool bOutpostOwned = false;
    bool bForestOwned = false;
    bool bOreOwned = false;
    TArray<FAIHeroesArmyStack> OutpostDefenders;
};

struct FAIHeroesRivalTurn
{
    EAIHeroesRivalAction Action = EAIHeroesRivalAction::None;
    int32 DefenderIndex = INDEX_NONE; // May equal Num() when creating a new stack.
    int32 Amount = 0;
    FString Description;

    bool IsActive() const { return Action != EAIHeroesRivalAction::None; }
};

// Pure plan/resolve. There is no randomness, file IO, mutation of the input,
// model call, or authority to advance the human player's turn.
class AIHEROES_API FAIHeroesRivalStrategy
{
public:
    static constexpr int32 MaxRaidersPerStack = 16;
    static constexpr int32 WeeklyRecruits = 2;
    static constexpr int32 ForestRaidLimit = 6;
    static constexpr int32 OreRaidLimit = 4;

    // Input.Day denotes the next day AFTER its ordinary passive income.
    static bool Plan(const FAIHeroesRivalSnapshot& Input,
        FAIHeroesRivalTurn& Turn, FString& Error);

    static bool Resolve(const FAIHeroesRivalSnapshot& Input,
        FAIHeroesRivalSnapshot& After, FAIHeroesRivalTurn& Turn, FString& Error);
};
