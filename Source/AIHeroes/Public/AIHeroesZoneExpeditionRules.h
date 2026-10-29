#pragma once

#include "CoreMinimal.h"
#include "AIHeroesAdventurePath.h"
#include "AIHeroesZoneExpeditionSave.h"
#include "AIHeroesZoneTemplate.h"

struct FAIHeroesZoneExpeditionResult
{
    bool bAccepted = false;
    bool bBattle = false;
    bool bVictory = false;
    int32 MovementSpent = 0;
    int32 DiscoveredCount = 0;
    FString Message;
};

// Authoritative deterministic expedition rules for 7..11 generated sites.
// No actors, UI, files, networking, real LLM or HotA assets.
// Only ConfirmPlayerDestination executes movement/combats. Read-only
// Preview and PlayerView NEVER mutate state or expose unknown graph nodes.
class AIHEROES_API FAIHeroesZoneExpeditionRules
{
public:
    static constexpr int32 Schema = 2;
    static constexpr int32 MovesPerDay = 8;
    static constexpr int32 MaxDay = 365;
    static constexpr int32 MaxResource = 1000000;
    static constexpr uint32 StartingKnownMask = 0x0fu; // HQ,SUPPLY,FOREST,ORE

    static bool Create(uint32 Seed, int32 Branches,
        FAIHeroesZoneExpeditionState& Out, FString& Error);
    static bool Validate(const FAIHeroesZoneExpeditionState& S, FString& Error);
    static bool PlayerView(const FAIHeroesZoneExpeditionState& S,
        FAIHeroesAdventureGraph& Out, FString& Error);
    static FAIHeroesAdventurePathResult Preview(
        const FAIHeroesZoneExpeditionState& S, FName Target);
    static bool ConfirmPlayerDestination(
        FAIHeroesZoneExpeditionState& S, FName Target,
        FAIHeroesZoneExpeditionResult& Result);
    static bool EndPlayerDay(FAIHeroesZoneExpeditionState& S, FString& Error);
    static bool RecruitAtHeadquarters(
        FAIHeroesZoneExpeditionState& S, FString& Error);
    static bool UpgradeProtocol(
        FAIHeroesZoneExpeditionState& S, FString& Error);

    static bool IsDiscovered(const FAIHeroesZoneExpeditionState& S, FName Site);
    static bool IsCleared(const FAIHeroesZoneExpeditionState& S, FName Site);
    static bool HasWon(const FAIHeroesZoneExpeditionState& S);
    static int32 DiscoveredCount(const FAIHeroesZoneExpeditionState& S);
};
