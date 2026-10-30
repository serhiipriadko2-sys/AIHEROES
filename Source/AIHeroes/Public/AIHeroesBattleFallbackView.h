#pragma once

#include "CoreMinimal.h"

struct FAIHeroesGridAction;

// Presentation-only projection of an already active live combat session.
// It never simulates actions, resolves combat, or mutates the campaign.
struct AIHEROES_API FAIHeroesBattleFallbackView
{
    bool bVisible = false;
    FString ActionLabel;
    FString Status;
    FString DecisionReason;
    TArray<FString> PlayerRoster;
    TArray<FString> EnemyRoster;

    static FAIHeroesBattleFallbackView Build(
        bool bSceneAvailable, bool bResult, bool bPaused,
        int32 CommittedActions,
        const FString& PlayerStatus, const FString& EnemyStatus,
        const FAIHeroesGridAction* LastAction = nullptr);
};
