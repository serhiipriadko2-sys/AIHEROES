#pragma once

#include "CoreMinimal.h"
#include "AIHeroesTypes.h"
#include "AIHeroesBattleRecord.generated.h"

struct FAIHeroesBattleOutcome;

// An evidence bundle for the LAST battle, not a command to execute again in the world.
// Inputs + tier are stored so the deterministic combat engine can verify the trace.
USTRUCT()
struct FAIHeroesBattleRecord
{
    GENERATED_BODY()

    UPROPERTY(SaveGame)
    FName NodeId = NAME_None;

    UPROPERTY(SaveGame)
    int32 Day = 0;

    UPROPERTY(SaveGame)
    int32 ProtocolTier = 0;

    // Missing metadata in existing saves must continue to mean legacy combat.
    UPROPERTY(SaveGame)
    int32 RuleVersion = 1;

    UPROPERTY(SaveGame)
    TArray<FAIHeroesArmyStack> PlayerBefore;

    UPROPERTY(SaveGame)
    TArray<FAIHeroesArmyStack> EnemyBefore;

    UPROPERTY(SaveGame)
    TArray<FAIHeroesArmyStack> PlayerAfter;

    UPROPERTY(SaveGame)
    TArray<FAIHeroesArmyStack> EnemyAfter;

    UPROPERTY(SaveGame)
    TArray<FString> Trace;

    UPROPERTY(SaveGame)
    FString Summary;

    UPROPERTY(SaveGame)
    int32 Rounds = 0;

    UPROPERTY(SaveGame)
    bool bVictory = false;

    UPROPERTY(SaveGame)
    bool bDefeat = false;
};

class AIHEROES_API FAIHeroesBattleRecordRules
{
public:
    static FAIHeroesBattleRecord Capture(
        FName NodeId, int32 Day, int32 ProtocolTier,
        const TArray<FAIHeroesArmyStack>& PlayerBefore,
        const TArray<FAIHeroesArmyStack>& EnemyBefore,
        const FAIHeroesBattleOutcome& Outcome, int32 RuleVersion = 1);

    // An empty record means a legacy save/no battles. It is deliberately valid.
    static bool Verify(const FAIHeroesBattleRecord& Record, FString& Error);
    static bool Equivalent(const FAIHeroesBattleRecord& A, const FAIHeroesBattleRecord& B);

    static FString DescribeStep(const FAIHeroesBattleRecord& Record, int32 Index);
};
