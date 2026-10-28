#pragma once

#include "CoreMinimal.h"
#include "AIHeroesAdvisor.h"

enum class EAIHeroesRecruitChoice : uint8 { KeepArmy, Militia, Scouts };

// Shared by actual purchases and the read-only advisor: prices cannot drift.
struct FAIHeroesRecruitOffer
{
    EAIHeroesRecruitChoice Choice = EAIHeroesRecruitChoice::KeepArmy;
    FString Label;
    int32 Gold = 0;
    int32 Wood = 0;
    int32 Ore = 0;
    FAIHeroesArmyStack Units;
};

struct FAIHeroesRecruitOption
{
    FAIHeroesRecruitOffer Offer;
    bool bAffordable = false;
    bool bForecastAvailable = false;
    bool bVictory = false;
    int32 LostHP = 0;
    int32 Survivors = 0;
    int32 Rounds = 0;
    int64 BurdenScore = 0;
};

struct FAIHeroesRecruitAdvice
{
    bool bValidInput = false;
    bool bTargetReachable = false;
    int32 RecommendedIndex = INDEX_NONE;
    TArray<FAIHeroesRecruitOption> Options;
    FString Summary;
    FString Detail;
};

// No actors, writes, file access, network, model weights or purchase authority.
class AIHEROES_API FAIHeroesRecruitmentAdvisor
{
public:
    static FAIHeroesRecruitOffer GetOffer(EAIHeroesRecruitChoice Choice);
    static FAIHeroesRecruitAdvice Evaluate(const FAIHeroesAdvisorSnapshot& Snapshot);
};
