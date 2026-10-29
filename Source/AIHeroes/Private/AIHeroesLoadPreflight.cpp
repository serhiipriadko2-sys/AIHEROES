#include "AIHeroesLoadPreflight.h"

#include "AIHeroesCampaignSave.h"

namespace
{
bool IsLoadNodeTypeValid(EAIHeroesNodeType Type)
{
    switch (Type)
    {
    case EAIHeroesNodeType::Headquarters:
    case EAIHeroesNodeType::ForestMine:
    case EAIHeroesNodeType::OreMine:
    case EAIHeroesNodeType::Outpost:
    case EAIHeroesNodeType::Ruins:
        return true;
    default:
        return false;
    }
}
}

bool FAIHeroesLoadPreflight::Validate(
    const TArray<FAIHeroesActiveNodeIdentity>& ActiveNodes,
    const TArray<FAIHeroesSavedNode>& SavedNodes, FName HeroNodeId,
    FName SelectedNodeId, FString& Error)
{
    Error.Reset();
    if (ActiveNodes.Num() != SavedNodes.Num() || ActiveNodes.IsEmpty())
    {
        Error = TEXT("Active map layout differs from the save.");
        return false;
    }

    TMap<FName, EAIHeroesNodeType> SavedById;
    for (const FAIHeroesSavedNode& Saved : SavedNodes)
    {
        if (Saved.NodeId.IsNone() || SavedById.Contains(Saved.NodeId) ||
            !IsLoadNodeTypeValid(Saved.Type))
        {
            Error = TEXT("Save contains an empty/duplicate node identifier or invalid type.");
            return false;
        }
        SavedById.Add(Saved.NodeId, Saved.Type);
    }

    TSet<FName> ActiveIds;
    for (const FAIHeroesActiveNodeIdentity& Active : ActiveNodes)
    {
        if (Active.Id.IsNone() || ActiveIds.Contains(Active.Id))
        {
            Error = TEXT("Active map contains an empty or duplicate node identifier.");
            return false;
        }
        const EAIHeroesNodeType* SavedType = SavedById.Find(Active.Id);
        if (!SavedType || !IsLoadNodeTypeValid(Active.Type) || *SavedType != Active.Type)
        {
            Error = TEXT("Node identifiers or types do not match this map.");
            return false;
        }
        ActiveIds.Add(Active.Id);
    }

    // Equal counts + unique IDs + every active ID present implies exact coverage.
    if (HeroNodeId.IsNone() || !ActiveIds.Contains(HeroNodeId))
    {
        Error = TEXT("Hero position cannot be restored.");
        return false;
    }
    if (!SelectedNodeId.IsNone() && !ActiveIds.Contains(SelectedNodeId))
    {
        Error = TEXT("Selected node cannot be restored.");
        return false;
    }
    return true;
}
