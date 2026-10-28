#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "AIHeroesTypes.h"
#include "AIHeroesBattleRecord.h"
#include "AIHeroesCampaignSave.generated.h"

// Stable map IDs, not actor pointers, are persisted across game sessions.
USTRUCT()
struct FAIHeroesSavedNode
{
    GENERATED_BODY()

    UPROPERTY(SaveGame)
    FName NodeId = NAME_None;

    UPROPERTY(SaveGame)
    EAIHeroesNodeType Type = EAIHeroesNodeType::Headquarters;

    UPROPERTY(SaveGame)
    bool bOwned = false;

    UPROPERTY(SaveGame)
    TArray<FAIHeroesArmyStack> Defenders;
};

UCLASS()
class AIHEROES_API UAIHeroesCampaignSave : public USaveGame
{
    GENERATED_BODY()

public:
    static constexpr int32 CurrentSchema = 1;

    // Only a fixed local slot is used. No arbitrary paths or user files.
    static const FString& SlotName();

    UPROPERTY(SaveGame)
    int32 SchemaVersion = CurrentSchema;

    // Optional for legacy v1 saves (defaults to 0), used to rank two
    // validated rotating snapshots without overwriting the previous one.
    UPROPERTY(SaveGame)
    int32 SaveSequence = 0;

    UPROPERTY(SaveGame)
    int32 Day = 1;

    UPROPERTY(SaveGame)
    int32 Gold = 240;

    UPROPERTY(SaveGame)
    int32 Wood = 35;

    UPROPERTY(SaveGame)
    int32 Ore = 30;

    // Optional additive schema-1 field: legacy saves default to level zero.
    UPROPERTY(SaveGame)
    int32 BaseTier = 0;

    UPROPERTY(SaveGame)
    int32 MovesLeft = 4;

    UPROPERTY(SaveGame)
    int32 ProtocolTier = 0;

    // In-game slots are cosmetic capacity unlocks, never actual model weights.
    UPROPERTY(SaveGame)
    int32 ModelSlotTier = 0;

    UPROPERTY(SaveGame)
    int32 DataFragments = 0;

    UPROPERTY(SaveGame)
    FName HeroNodeId = NAME_None;

    UPROPERTY(SaveGame)
    FName SelectedNodeId = NAME_None;

    UPROPERTY(SaveGame)
    TArray<FAIHeroesArmyStack> PlayerArmy;

    UPROPERTY(SaveGame)
    TArray<FAIHeroesSavedNode> NodeStates;

    UPROPERTY(SaveGame)
    TArray<FString> BattleJournal;

    UPROPERTY(SaveGame)
    FString LastCombatStep;

    // Optional for old v1 saves; schema stays compatible.
    UPROPERTY(SaveGame)
    FAIHeroesBattleRecord LastBattleRecord;
};
