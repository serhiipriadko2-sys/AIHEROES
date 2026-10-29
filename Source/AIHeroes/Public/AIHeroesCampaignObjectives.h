#pragma once

#include "CoreMinimal.h"
#include "AIHeroesTypes.h"

// Read-only strategic evidence. Goals never execute a move or grant a reward.
struct FAIHeroesObjectiveNode
{
    FName Id = NAME_None;
    EAIHeroesNodeType Type = EAIHeroesNodeType::Headquarters;
    bool bOwned = false;
};

struct FAIHeroesCampaignObjectiveInput
{
    int32 ProtocolTier = 0;
    int32 ModelSlotTier = 0;
    TArray<FAIHeroesObjectiveNode> Nodes;
};

struct FAIHeroesCampaignGoal
{
    FName Id = NAME_None;
    FString Title;
    FString Description;
    int32 Current = 0;
    int32 Required = 1;

    bool IsComplete() const { return Required > 0 && Current >= Required; }
    FString ToReportLine() const;
};

struct FAIHeroesCampaignProgress
{
    TArray<FAIHeroesCampaignGoal> Goals;
    int32 Completed = 0;
    int32 Total = 0;
    bool bVictory = false;
    FString NextAction;
    FName NextFocusId = NAME_None; // The canonical next goal, independent of localization.

    FString Headline() const;
    TArray<FString> Report() const;
};

// Derived from existing saved fields. No SaveGame schema migration needed.
// The themed MODEL SLOT goal does NOT claim that any neural weights are loaded.
class AIHEROES_API FAIHeroesCampaignObjectives
{
public:
    static constexpr int32 GoalCount = 4;
    static FAIHeroesCampaignProgress Evaluate(const FAIHeroesCampaignObjectiveInput& Input);
};
