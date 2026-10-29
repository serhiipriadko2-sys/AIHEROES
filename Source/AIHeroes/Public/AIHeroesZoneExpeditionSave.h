#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "AIHeroesTypes.h"
#include "AIHeroesZoneExpeditionSave.generated.h"

// Independent, opt-in zone-campaign state. Its schema and slot namespace
// MUST NOT be interpreted as or written to the 5-site CampaignSave v1.
// All identities are generated from Seed + BranchCount + MapRevision.
USTRUCT()
struct FAIHeroesZoneExpeditionState
{
    GENERATED_BODY()

    UPROPERTY(SaveGame) int32 Schema = 2;
    UPROPERTY(SaveGame) int32 BranchCount = 0;
    UPROPERTY(SaveGame) uint32 Seed = 0;
    UPROPERTY(SaveGame) uint32 MapRevision = 0;
    UPROPERTY(SaveGame) uint32 DiscoveredMask = 0;
    UPROPERTY(SaveGame) uint32 ClearedMask = 0;
    UPROPERTY(SaveGame) FName HeroSite = NAME_None;
    UPROPERTY(SaveGame) int32 Day = 1;
    UPROPERTY(SaveGame) int32 MovesLeft = 8;
    UPROPERTY(SaveGame) int32 Gold = 240;
    UPROPERTY(SaveGame) int32 Wood = 35;
    UPROPERTY(SaveGame) int32 Ore = 30;
    UPROPERTY(SaveGame) int32 DataFragments = 0;
    UPROPERTY(SaveGame) int32 ProtocolTier = 0;
    UPROPERTY(SaveGame) TArray<FAIHeroesArmyStack> PlayerArmy;
    UPROPERTY(SaveGame) FString LastEvent;
};

// Separate class and schema-2 A/B file names. No implicit v1 migration.
// Only a future explicit human Save command should use the production prefix.
UCLASS()
class AIHEROES_API UAIHeroesZoneExpeditionSave : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame) int32 SaveSequence = 0;
    UPROPERTY(SaveGame) FAIHeroesZoneExpeditionState State;
};
