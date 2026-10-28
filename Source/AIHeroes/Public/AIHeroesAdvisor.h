#pragma once

#include "CoreMinimal.h"
#include "AIHeroesTypes.h"

// A read-only, typed snapshot. Neither game state nor actors are exposed to the advisor.
struct FAIHeroesAdvisorNode
{
    FName Id = NAME_None;
    EAIHeroesNodeType Type = EAIHeroesNodeType::Headquarters;
    FVector Location = FVector::ZeroVector;
    bool bOwned = false;
    TArray<FAIHeroesArmyStack> Guards;
};

struct FAIHeroesAdvisorSnapshot
{
    int32 ProtocolTier = 0;
    int32 MovesLeft = 0;
    int32 Gold = 0;
    int32 Wood = 0;
    int32 Ore = 0;
    FVector HeroPosition = FVector::ZeroVector;
    FName SelectedNodeId = NAME_None;
    TArray<FAIHeroesArmyStack> PlayerArmy;
    TArray<FAIHeroesAdvisorNode> Nodes;
    FString LastBattle;
};

// Deterministic offline advisor. It advises strategy and NEVER executes a player action.
class FAIHeroesAdvisor
{
public:
    static FString Advise(const FAIHeroesAdvisorSnapshot& Snapshot);
};
