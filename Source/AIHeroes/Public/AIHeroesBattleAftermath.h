#pragma once

#include "CoreMinimal.h"
#include "AIHeroesBattleRecord.h"

// A receipt for THIS running session only. Created from the reward plan
// inside the authoritative, one-time battle commit. Never written to SaveGame,
// and never reconstructed by applying today's reward rules to old history.
struct FAIHeroesBattleCommitReceipt
{
    // Full immutable result identity, not merely node/day/version metadata.
    FAIHeroesBattleRecord BoundRecord;
    FName NodeId = NAME_None;
    int32 Day = 0;
    int32 RuleVersion = 0;
    int32 MovesSpent = -1;
    int32 GoldGranted = 0;
    int32 DataGranted = 0;
    bool bApplied = false;
};

// Presentation only: no combat, reward, save, movement or campaign mutation.
struct FAIHeroesBattleAftermathView
{
    FString PlayerLine;
    FString RewardLine;
    TArray<FString> PlayerGroups;
    bool bCountsAvailable = false;
    bool bRewardsVerified = false;
};

class AIHEROES_API FAIHeroesBattleAftermath
{
public:
    // A missing/mismatched transient receipt can NEVER invent old rewards.
    // The original one-argument API stays valid for historic battle views.
    static FAIHeroesBattleAftermathView Build(
        const FAIHeroesBattleRecord& CommittedRecord,
        const FAIHeroesBattleCommitReceipt* AppliedReward = nullptr);
};
