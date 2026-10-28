#pragma once

#include "CoreMinimal.h"
#include "AIHeroesBattleRecord.h"

// Read-only counterfactual evidence calculated from a verified historical
// battle. No model inference, SaveGame changes, AI orders or world actors.
struct FAIHeroesProtocolLabScenario
{
    int32 ProtocolTier = 0;
    bool bActual = false;
    bool bVictory = false;
    bool bDefeat = false;
    int32 Rounds = 0;
    int32 TraceActions = 0;
    int64 HeroHealth = 0;
    int64 EnemyHealth = 0;
    int64 HeroHpDelta = 0;
    // 0 means identical transcript; otherwise 1-based first different action.
    int32 FirstDifferentAction = 0;
};

struct FAIHeroesProtocolLabReport
{
    FName NodeId = NAME_None;
    int32 Day = 0;
    int32 RuleVersion = 0;
    int32 ActualProtocolTier = 0;
    TArray<FAIHeroesProtocolLabScenario> Scenarios;

    bool IsValid() const
    {
        return !NodeId.IsNone() && Day >= 1 && Scenarios.Num() == 4;
    }
    TArray<FString> Describe() const;
};

// Simulates the SAME starting battle under the four deterministic rule tiers.
// The historical outcome is not altered and no counterfactual is presented
// as something that actually happened. Invalid records fail closed.
class AIHEROES_API FAIHeroesProtocolLabRules
{
public:
    static constexpr int32 ProtocolVariants = 4;

    static bool Analyze(const FAIHeroesBattleRecord& Record,
        FAIHeroesProtocolLabReport& OutReport, FString& Error);
};
