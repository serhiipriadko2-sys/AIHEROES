#pragma once

#include "CoreMinimal.h"
#include "AIHeroesBattleRecord.h"
#include "AIHeroesGridBattle.h"

// Typed, READ-ONLY replay compiled from a fully verified V2 save record.
// The battle engine is replayed in memory to reconstruct each grid action.
// This is display data only: no GameMode, SaveGame writes or campaign orders.
struct FAIHeroesGridReplay
{
    FName NodeId = NAME_None;
    TArray<FAIHeroesArmyStack> PlayerBefore;
    TArray<FAIHeroesArmyStack> EnemyBefore;
    TArray<FIntPoint> PlayerCells;
    TArray<FIntPoint> EnemyCells;
    TArray<FAIHeroesGridAction> Actions;
    FString Summary;

    bool IsValid() const
    {
        return !NodeId.IsNone() && !Actions.IsEmpty() &&
            Actions.Num() <= FAIHeroesGridBattle::MaxActions;
    }
};

class AIHEROES_API FAIHeroesGridReplayRules
{
public:
    // Fail closed. On any failure OutReplay remains empty, not half populated.
    // Version 1 remains supported by the existing legacy 3D replay stage.
    static bool Build(
        const FAIHeroesBattleRecord& Record,
        FAIHeroesGridReplay& OutReplay,
        FString& Error);
};
