#include "AIHeroesZoneExpeditionStore.h"

#include "AIHeroesZoneExpeditionRules.h"
#include "Kismet/GameplayStatics.h"

namespace
{
constexpr int32 SaveUser = 0;
constexpr int32 MaxSequence = 1000000;

bool AllowedPrefix(const FString& Name)
{
    if (!Name.StartsWith(TEXT("AIHEROES_ZoneExpedition_")) ||
        Name.Len() > 96 || Name.Len() < 26) return false;
    for (TCHAR C : Name)
        if (!((C >= 'a' && C <= 'z') || (C >= 'A' && C <= 'Z') ||
              (C >= '0' && C <= '9') || C == '_')) return false;
    return true;
}

struct FCandidate
{
    FString Slot;
    bool bPresent = false;
    UAIHeroesZoneExpeditionSave* Save = nullptr;
    int32 Seq() const { return Save ? Save->SaveSequence : INDEX_NONE; }
};

FCandidate Read(const FString& Slot)
{
    FCandidate R;
    R.Slot = Slot;
    R.bPresent = UGameplayStatics::DoesSaveGameExist(Slot, SaveUser);
    if (!R.bPresent) return R;
    auto* Obj = Cast<UAIHeroesZoneExpeditionSave>(
        UGameplayStatics::LoadGameFromSlot(Slot, SaveUser));
    FString Error;
    if (IsValid(Obj) && Obj->SaveSequence > 0 &&
        Obj->SaveSequence <= MaxSequence &&
        FAIHeroesZoneExpeditionRules::Validate(Obj->State, Error))
        R.Save = Obj;
    return R;
}

bool ExactArmy(const TArray<FAIHeroesArmyStack>& A,
    const TArray<FAIHeroesArmyStack>& B)
{
    if (A.Num() != B.Num()) return false;
    for (int32 I = 0; I < A.Num(); ++I)
    {
        const auto& X = A[I];
        const auto& Y = B[I];
        if (X.UnitId != Y.UnitId || X.UnitHealth != Y.UnitHealth ||
            X.TotalHealth != Y.TotalHealth || X.Attack != Y.Attack ||
            X.Defense != Y.Defense || X.Initiative != Y.Initiative) return false;
    }
    return true;
}

bool Identical(const FAIHeroesZoneExpeditionState& A,
    const FAIHeroesZoneExpeditionState& B)
{
    return A.Schema == B.Schema && A.BranchCount == B.BranchCount &&
        A.Seed == B.Seed && A.MapRevision == B.MapRevision &&
        A.DiscoveredMask == B.DiscoveredMask &&
        A.ClearedMask == B.ClearedMask &&
        A.HeroSite == B.HeroSite &&
        A.Day == B.Day && A.MovesLeft == B.MovesLeft &&
        A.Gold == B.Gold && A.Wood == B.Wood && A.Ore == B.Ore &&
        A.DataFragments == B.DataFragments &&
        A.ProtocolTier == B.ProtocolTier &&
        A.LastEvent == B.LastEvent &&
        ExactArmy(A.PlayerArmy, B.PlayerArmy);
}
}

const FString& FAIHeroesZoneExpeditionStore::ProductionPrefix()
{
    static const FString P(TEXT("AIHEROES_ZoneExpedition_v2"));
    return P;
}

FString FAIHeroesZoneExpeditionStore::SlotA(const FString& Prefix)
{
    return Prefix + TEXT("_A");
}

FString FAIHeroesZoneExpeditionStore::SlotB(const FString& Prefix)
{
    return Prefix + TEXT("_B");
}

FAIHeroesZoneExpeditionLoad FAIHeroesZoneExpeditionStore::Load(
    const FString& Prefix)
{
    FAIHeroesZoneExpeditionLoad Out;
    if (!AllowedPrefix(Prefix))
    {
        Out.Message = TEXT("Refused unapproved expedition slot namespace.");
        return Out;
    }
    const FCandidate A = Read(SlotA(Prefix));
    const FCandidate B = Read(SlotB(Prefix));
    const FCandidate* Best = nullptr;
    if (A.Save && B.Save) Best = A.Seq() >= B.Seq() ? &A : &B;
    else if (A.Save) Best = &A;
    else if (B.Save) Best = &B;
    if (!Best)
    {
        Out.Message = (A.bPresent || B.bPresent)
            ? TEXT("Both expedition snapshots invalid; player state unchanged.")
            : TEXT("No expedition snapshot yet.");
        return Out;
    }
    Out.bOk = true;
    Out.Sequence = Best->Seq();
    Out.State = Best->Save->State;
    Out.Slot = Best->Slot;
    const FCandidate* Other = Best == &A ? &B : &A;
    Out.bRecovered = Other->bPresent && !Other->Save;
    Out.Message = Out.bRecovered
        ? TEXT("Recovered from last validated expedition copy.")
        : TEXT("Loaded latest verified expedition copy.");
    return Out;
}

bool FAIHeroesZoneExpeditionStore::Save(
    const FAIHeroesZoneExpeditionState& State,
    const FString& Prefix, FString& Message)
{
    if (!AllowedPrefix(Prefix))
    {
        Message = TEXT("SAVE BLOCKED: invalid expedition slot namespace.");
        return false;
    }
    FString Error;
    if (!FAIHeroesZoneExpeditionRules::Validate(State, Error))
    {
        Message = TEXT("SAVE BLOCKED: ") + Error;
        return false;
    }
    const FAIHeroesZoneExpeditionLoad Existing = Load(Prefix);
    const bool bA = UGameplayStatics::DoesSaveGameExist(SlotA(Prefix), SaveUser);
    const bool bB = UGameplayStatics::DoesSaveGameExist(SlotB(Prefix), SaveUser);
    if ((bA || bB) && !Existing.bOk)
    {
        Message = TEXT("SAVE BLOCKED: do not overwrite both invalid expedition copies.");
        return false;
    }
    if (Existing.Sequence >= MaxSequence)
    {
        Message = TEXT("SAVE BLOCKED: snapshot sequence limit.");
        return false;
    }
    const FString Target = Existing.Slot == SlotA(Prefix)
        ? SlotB(Prefix) : SlotA(Prefix);
    auto* Snapshot = Cast<UAIHeroesZoneExpeditionSave>(
        UGameplayStatics::CreateSaveGameObject(UAIHeroesZoneExpeditionSave::StaticClass()));
    if (!Snapshot)
    {
        Message = TEXT("SAVE FAILED: object allocation.");
        return false;
    }
    Snapshot->SaveSequence = Existing.Sequence + 1;
    Snapshot->State = State;
    if (!UGameplayStatics::SaveGameToSlot(Snapshot, Target, SaveUser))
    {
        Message = TEXT("SAVE FAILED: engine write refused; last valid copy untouched.");
        return false;
    }
    const FCandidate Confirmed = Read(Target);
    if (!Confirmed.Save || Confirmed.Seq() != Snapshot->SaveSequence ||
        !Identical(Confirmed.Save->State, State))
    {
        Message = TEXT("SAVE FAILED: disk semantic read-back mismatch.");
        return false;
    }
    Message = FString::Printf(TEXT("EXPEDITION SAVED sequence=%d copy=%s"),
        Snapshot->SaveSequence, Target.EndsWith(TEXT("_A")) ? TEXT("A") : TEXT("B"));
    return true;
}
