#pragma once

#include "CoreMinimal.h"
#include "AIHeroesTypes.h"

struct FAIHeroesSavedNode;

// Actor-free identity snapshot. Actor validity is checked by GameMode first.
struct FAIHeroesActiveNodeIdentity
{
    FName Id = NAME_None;
    EAIHeroesNodeType Type = EAIHeroesNodeType::Headquarters;
};

// Read-only map/save matching, before any loaded campaign state is applied.
// This supplements, not replaces, full CampaignRules snapshot validation.
class AIHEROES_API FAIHeroesLoadPreflight
{
public:
    static bool Validate(const TArray<FAIHeroesActiveNodeIdentity>& ActiveNodes,
        const TArray<FAIHeroesSavedNode>& SavedNodes, FName HeroNodeId,
        FName SelectedNodeId, FString& Error);
};
