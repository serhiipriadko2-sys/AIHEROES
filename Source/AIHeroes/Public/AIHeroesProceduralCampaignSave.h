#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "AIHeroesAdventurePath.h"
#include "AIHeroesTypes.h"
#include "AIHeroesProceduralCampaignSave.generated.h"

class UAIHeroesCampaignSave;

// Independent future-world SaveGame schema. It is NEVER loaded by the
// existing five-node v1 game, and has NO live player slot or auto-save.
// Persistence tests must use GUID-specific disposable slots exclusively.
USTRUCT()
struct FAIHeroesProceduralSavedZone
{
    GENERATED_BODY()

    UPROPERTY(SaveGame)
    FName ZoneId = NAME_None;

    UPROPERTY(SaveGame)
    bool bDiscovered = false;

    UPROPERTY(SaveGame)
    bool bOwned = false;

    UPROPERTY(SaveGame)
    bool bGuardCleared = false;
};

UCLASS()
class AIHEROES_API UAIHeroesProceduralCampaignSave : public USaveGame
{
    GENERATED_BODY()

public:
    static constexpr int32 CurrentSchema = 2;

    UPROPERTY(SaveGame)
    int32 SchemaVersion = CurrentSchema;

    UPROPERTY(SaveGame)
    int32 GeneratorSchema = 1;

    UPROPERTY(SaveGame)
    uint32 Seed = 0;

    UPROPERTY(SaveGame)
    int32 ExtraRoutes = 0;

    // Exact generated-world topology fingerprint (non-cryptographic).
    UPROPERTY(SaveGame)
    uint32 TopologyRevision = 0;

    UPROPERTY(SaveGame)
    int32 SaveSequence = 0;

    // Stale-proposal detector, not AI authority or human confirmation.
    UPROPERTY(SaveGame)
    int32 ActionRevision = 0;

    UPROPERTY(SaveGame)
    int32 Day = 1;

    UPROPERTY(SaveGame)
    int32 MovesLeft = 4;

    UPROPERTY(SaveGame)
    int32 Gold = 240;

    UPROPERTY(SaveGame)
    int32 Wood = 35;

    UPROPERTY(SaveGame)
    int32 Ore = 30;

    UPROPERTY(SaveGame)
    int32 ProtocolTier = 0;

    UPROPERTY(SaveGame)
    int32 ModelSlotTier = 0;

    UPROPERTY(SaveGame)
    int32 DataFragments = 0;

    UPROPERTY(SaveGame)
    FName HeroZoneId = FName(TEXT("HQ"));

    UPROPERTY(SaveGame)
    FName SelectedZoneId = NAME_None;

    UPROPERTY(SaveGame)
    TArray<FAIHeroesProceduralSavedZone> ZoneStates;

    UPROPERTY(SaveGame)
    TArray<FAIHeroesArmyStack> PlayerArmy;
};

enum class EAIHeroesProceduralLegacyDecision : uint8
{
    InvalidLegacy,
    RequiresNewCampaign
};

// All operations are in-memory and authority-free. The adapter validates
// against the SAME portable generator as the 3D preview, never an inferred
// grid or actor pointer. It never calls UGameplayStatics Save/Load.
class AIHEROES_API FAIHeroesProceduralCampaignRules
{
public:
    static bool CreateNew(uint32 Seed, int32 ExtraRoutes,
                          UAIHeroesProceduralCampaignSave& Out,
                          FString& Error);

    static bool Validate(const UAIHeroesProceduralCampaignSave& Save,
                         FString& Error);

    static bool BuildPlayerGraph(
        const UAIHeroesProceduralCampaignSave& Save,
        FAIHeroesAdventureGraph& OutGraph, FString& Error);

    // Semantic read-back equality; independent of serialization success.
    static bool Equivalent(const UAIHeroesProceduralCampaignSave& A,
                           const UAIHeroesProceduralCampaignSave& B);

    // V1 has five fixed sites: there is no lossless automatic conversion.
    // Classification is read-only and deliberately never returns "migrated".
    static EAIHeroesProceduralLegacyDecision AssessLegacy(
        const UAIHeroesCampaignSave& Legacy, FString& Reason);
};
