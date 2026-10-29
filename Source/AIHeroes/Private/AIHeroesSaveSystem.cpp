#include "AIHeroesSaveSystem.h"

#include "AIHeroesCampaignRules.h"
#include "AIHeroesBattleRecord.h"
#include "AIHeroesCampaignSave.h"
#include "Kismet/GameplayStatics.h"

namespace
{
constexpr int32 SaveUserIndex = 0;
constexpr int32 CampaignSaveMaximumSequence = 1000000;

bool SameStack(const FAIHeroesArmyStack& A, const FAIHeroesArmyStack& B)
{
    return A.UnitId == B.UnitId &&
        A.UnitHealth == B.UnitHealth &&
        A.TotalHealth == B.TotalHealth &&
        A.Attack == B.Attack &&
        A.Defense == B.Defense &&
        A.Initiative == B.Initiative;
}

// Semantic read-back rather than trusting a true result from SaveGameToSlot.
// Verify everything required for gameplay, including nodes and battle journal.
bool SameSnapshot(const UAIHeroesCampaignSave& A, const UAIHeroesCampaignSave& B)
{
    if (A.SchemaVersion != B.SchemaVersion ||
        A.SaveSequence != B.SaveSequence ||
        A.BaseTier != B.BaseTier ||
        A.TravelRuleVersion != B.TravelRuleVersion ||
        A.Day != B.Day || A.Gold != B.Gold ||
        A.Wood != B.Wood || A.Ore != B.Ore ||
        A.MovesLeft != B.MovesLeft || A.ProtocolTier != B.ProtocolTier ||
        A.ModelSlotTier != B.ModelSlotTier || A.DataFragments != B.DataFragments ||
        A.bRecruitmentStockInitialized != B.bRecruitmentStockInitialized ||
        A.MilitiaReserve != B.MilitiaReserve || A.ScoutReserve != B.ScoutReserve ||
        A.bFieldDwellingsInitialized != B.bFieldDwellingsInitialized ||
        A.bForestCampClaimed != B.bForestCampClaimed ||
        A.ForestCampUnits != B.ForestCampUnits ||
        A.bOreAerieClaimed != B.bOreAerieClaimed ||
        A.OreAerieUnits != B.OreAerieUnits ||
        A.bFogInitialized != B.bFogInitialized ||
        A.ExploredSitesMask != B.ExploredSitesMask ||
        A.HeroNodeId != B.HeroNodeId || A.SelectedNodeId != B.SelectedNodeId ||
        A.LastCombatStep != B.LastCombatStep ||
        A.BattleJournal != B.BattleJournal ||
        !FAIHeroesBattleRecordRules::Equivalent(A.LastBattleRecord, B.LastBattleRecord) ||
        A.PlayerArmy.Num() != B.PlayerArmy.Num() ||
        A.NodeStates.Num() != B.NodeStates.Num())
    {
        return false;
    }

    for (int32 Index = 0; Index < A.PlayerArmy.Num(); ++Index)
    {
        if (!SameStack(A.PlayerArmy[Index], B.PlayerArmy[Index]))
        {
            return false;
        }
    }
    for (int32 Index = 0; Index < A.NodeStates.Num(); ++Index)
    {
        const FAIHeroesSavedNode& Left = A.NodeStates[Index];
        const FAIHeroesSavedNode& Right = B.NodeStates[Index];
        if (Left.NodeId != Right.NodeId || Left.Type != Right.Type ||
            Left.bOwned != Right.bOwned || Left.Defenders.Num() != Right.Defenders.Num())
        {
            return false;
        }
        for (int32 Defender = 0; Defender < Left.Defenders.Num(); ++Defender)
        {
            if (!SameStack(Left.Defenders[Defender], Right.Defenders[Defender]))
            {
                return false;
            }
        }
    }
    return true;
}

struct FSlotStatus
{
    FString Slot;
    UAIHeroesCampaignSave* Snapshot = nullptr;
    bool bPresent = false;

    bool IsValid() const { return Snapshot != nullptr; }
    int32 Sequence() const { return Snapshot ? Snapshot->SaveSequence : INDEX_NONE; }
};

FSlotStatus ReadSlot(const FString& SlotName, bool bRequireSequence)
{
    FSlotStatus Status;
    Status.Slot = SlotName;
    Status.bPresent = UGameplayStatics::DoesSaveGameExist(SlotName, SaveUserIndex);
    if (!Status.bPresent)
    {
        return Status;
    }

    UAIHeroesCampaignSave* Loaded = Cast<UAIHeroesCampaignSave>(
        UGameplayStatics::LoadGameFromSlot(SlotName, SaveUserIndex));
    FString Error;
    if (!IsValid(Loaded) || !FAIHeroesCampaignRules::Validate(*Loaded, Error))
    {
        return Status;
    }
    if (bRequireSequence && Loaded->SaveSequence <= 0)
    {
        return Status;
    }
    Status.Snapshot = Loaded;
    return Status;
}
}

const FString& FAIHeroesSaveSystem::CampaignPrefix()
{
    static const FString Prefix(TEXT("AIHEROES_Campaign_Resilient_v1"));
    return Prefix;
}

FString FAIHeroesSaveSystem::SlotA(const FString& Prefix)
{
    return Prefix + TEXT("_A");
}

FString FAIHeroesSaveSystem::SlotB(const FString& Prefix)
{
    return Prefix + TEXT("_B");
}

FAIHeroesLoadedCampaign FAIHeroesSaveSystem::Load(
    const FString& Prefix, const FString& LegacySlot)
{
    const FSlotStatus A = ReadSlot(SlotA(Prefix), true);
    const FSlotStatus B = ReadSlot(SlotB(Prefix), true);
    FAIHeroesLoadedCampaign Result;

    // Validated newest wins, with deterministic tie-breaking toward A.
    const FSlotStatus* Chosen = nullptr;
    if (A.IsValid() && B.IsValid())
    {
        Chosen = A.Sequence() >= B.Sequence() ? &A : &B;
    }
    else if (A.IsValid())
    {
        Chosen = &A;
    }
    else if (B.IsValid())
    {
        Chosen = &B;
    }

    if (Chosen)
    {
        Result.Snapshot = Chosen->Snapshot;
        Result.SlotName = Chosen->Slot;
        const bool bRejectedOther = (&A == Chosen) ?
            (B.bPresent && !B.IsValid()) : (A.bPresent && !A.IsValid());
        Result.bRecovered = bRejectedOther;
        Result.Message = bRejectedOther
            ? TEXT("RECOVERED: damaged alternate save ignored; last valid campaign restored.")
            : TEXT("Loaded newest validated campaign snapshot.");
        return Result;
    }

    // Legacy v1 is never overwritten. It remains the final recovery option.
    const FSlotStatus Legacy = LegacySlot.IsEmpty() ? FSlotStatus() :
        ReadSlot(LegacySlot, false);
    if (Legacy.IsValid())
    {
        Result.Snapshot = Legacy.Snapshot;
        Result.SlotName = LegacySlot;
        Result.bLegacy = true;
        Result.bRecovered = A.bPresent || B.bPresent;
        Result.Message = Result.bRecovered
            ? TEXT("RECOVERED: no valid new slots; opened legacy v1 save.")
            : TEXT("Loaded legacy v1 save; the next save will create a protected copy.");
        return Result;
    }

    Result.Message = A.bPresent || B.bPresent || Legacy.bPresent
        ? TEXT("No valid campaign: new and legacy slots failed validation; game unchanged.")
        : TEXT("No campaign save found. Press K to create the first one.");
    return Result;
}

bool FAIHeroesSaveSystem::Save(
    UAIHeroesCampaignSave& Snapshot,
    const FString& Prefix, const FString& LegacySlot,
    FString& ResultMessage)
{
    const FAIHeroesLoadedCampaign Previous = Load(Prefix, LegacySlot);
    // If modern A/B files exist but neither validates, do not overwrite
    // potentially recoverable player data. A legacy fallback is read-only.
    const bool bModernFilesExist =
        UGameplayStatics::DoesSaveGameExist(SlotA(Prefix), SaveUserIndex) ||
        UGameplayStatics::DoesSaveGameExist(SlotB(Prefix), SaveUserIndex);
    if (bModernFilesExist && (!Previous.Snapshot || Previous.bLegacy))
    {
        ResultMessage = TEXT(
            "SAVE BLOCKED: damaged A/B files preserved. "
            "Select a different campaign slot for new progress.");
        return false;
    }
    const int32 LastSequence = Previous.Snapshot ?
        Previous.Snapshot->SaveSequence : 0;
    if (LastSequence >= CampaignSaveMaximumSequence)
    {
        ResultMessage = TEXT("SAVE BLOCKED: snapshot sequence limit reached.");
        return false;
    }

    const int32 OriginalSequence = Snapshot.SaveSequence;
    Snapshot.SaveSequence = LastSequence + 1;
    FString ValidationError;
    if (!FAIHeroesCampaignRules::Validate(Snapshot, ValidationError))
    {
        Snapshot.SaveSequence = OriginalSequence;
        ResultMessage = TEXT("SAVE BLOCKED: ") + ValidationError;
        return false;
    }

    // Never overwrite the last valid new slot. If only a legacy save exists,
    // create slot A instead of modifying that legacy file.
    const FString Target = Previous.SlotName == SlotA(Prefix) ?
        SlotB(Prefix) : SlotA(Prefix);
    if (!UGameplayStatics::SaveGameToSlot(&Snapshot, Target, SaveUserIndex))
    {
        Snapshot.SaveSequence = OriginalSequence;
        ResultMessage = TEXT("SAVE FAILED: engine could not write the alternate slot.");
        return false;
    }

    const FSlotStatus Verified = ReadSlot(Target, true);
    if (!Verified.IsValid() || !SameSnapshot(Snapshot, *Verified.Snapshot))
    {
        Snapshot.SaveSequence = OriginalSequence;
        ResultMessage = TEXT("SAVE FAILED: disk read-back mismatch; prior save preserved.");
        return false;
    }

    ResultMessage = FString::Printf(
        TEXT("GAME SAVED [%s] day %d, sequence %d. %s"),
        Target == SlotA(Prefix) ? TEXT("A") : TEXT("B"),
        Snapshot.Day,
        Snapshot.SaveSequence,
        Previous.Snapshot
            ? TEXT("Previous valid save retained for recovery.")
            : TEXT("First recovery copy is created on next save."));
    return true;
}
