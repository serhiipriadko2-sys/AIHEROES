#include "AIHeroesProceduralCampaignSave.h"

#include "AIHeroesCampaignRules.h"
#include "AIHeroesCampaignSave.h"
#include "AIHeroesProceduralCampaignCore.h"

namespace
{
using namespace AIHeroesProceduralCampaignCore;

std::string Utf8Name(const FName Name)
{
    return std::string(TCHAR_TO_UTF8(*Name.ToString()));
}

FAIHeroesArmyStack ToUnrealArmy(const FriendlyStack& Unit)
{
    FAIHeroesArmyStack Stack;
    Stack.UnitId = FName(UTF8_TO_TCHAR(Unit.UnitId.c_str()));
    Stack.UnitHealth = Unit.UnitHealth;
    Stack.TotalHealth = Unit.TotalHealth;
    Stack.Attack = Unit.Attack;
    Stack.Defense = Unit.Defense;
    Stack.Initiative = Unit.Initiative;
    return Stack;
}

State ToPortable(const UAIHeroesProceduralCampaignSave& Save)
{
    State S;
    S.SchemaVersion = static_cast<uint32>(Save.SchemaVersion);
    S.GeneratorSchema = static_cast<uint32>(Save.GeneratorSchema);
    S.Seed = Save.Seed;
    S.Branches = static_cast<uint32>(Save.ExtraRoutes);
    S.TopologyRevision = Save.TopologyRevision;
    S.SaveSequence = Save.SaveSequence;
    S.ActionRevision = Save.ActionRevision;
    S.Day = Save.Day;
    S.MovesLeft = Save.MovesLeft;
    S.Gold = Save.Gold;
    S.Wood = Save.Wood;
    S.Ore = Save.Ore;
    S.ProtocolTier = Save.ProtocolTier;
    S.ModelSlotTier = Save.ModelSlotTier;
    S.DataFragments = Save.DataFragments;
    S.HeroZoneId = Utf8Name(Save.HeroZoneId);
    S.SelectedZoneId = Save.SelectedZoneId.IsNone()
        ? std::string() : Utf8Name(Save.SelectedZoneId);
    for (const auto& Z : Save.ZoneStates)
        S.Zones.push_back({
            Utf8Name(Z.ZoneId), Z.bDiscovered, Z.bOwned, Z.bGuardCleared
        });
    for (const auto& U : Save.PlayerArmy)
        S.PlayerArmy.push_back({
            Utf8Name(U.UnitId), U.UnitHealth, U.TotalHealth,
            U.Attack, U.Defense, U.Initiative
        });
    return S;
}

void Populate(const State& S, UAIHeroesProceduralCampaignSave& Out)
{
    Out.SchemaVersion = static_cast<int32>(S.SchemaVersion);
    Out.GeneratorSchema = static_cast<int32>(S.GeneratorSchema);
    Out.Seed = S.Seed;
    Out.ExtraRoutes = static_cast<int32>(S.Branches);
    Out.TopologyRevision = S.TopologyRevision;
    Out.SaveSequence = S.SaveSequence;
    Out.ActionRevision = S.ActionRevision;
    Out.Day = S.Day;
    Out.MovesLeft = S.MovesLeft;
    Out.Gold = S.Gold;
    Out.Wood = S.Wood;
    Out.Ore = S.Ore;
    Out.ProtocolTier = S.ProtocolTier;
    Out.ModelSlotTier = S.ModelSlotTier;
    Out.DataFragments = S.DataFragments;
    Out.HeroZoneId = FName(UTF8_TO_TCHAR(S.HeroZoneId.c_str()));
    Out.SelectedZoneId = S.SelectedZoneId.empty()
        ? NAME_None : FName(UTF8_TO_TCHAR(S.SelectedZoneId.c_str()));
    Out.ZoneStates.Reset();
    for (const ZoneState& Z : S.Zones)
    {
        FAIHeroesProceduralSavedZone Record;
        Record.ZoneId = FName(UTF8_TO_TCHAR(Z.Id.c_str()));
        Record.bDiscovered = Z.Discovered;
        Record.bOwned = Z.Owned;
        Record.bGuardCleared = Z.GuardCleared;
        Out.ZoneStates.Add(Record);
    }
    Out.PlayerArmy.Reset();
    for (const FriendlyStack& Unit : S.PlayerArmy)
        Out.PlayerArmy.Add(ToUnrealArmy(Unit));
}

bool SameArmy(const FAIHeroesArmyStack& A, const FAIHeroesArmyStack& B)
{
    return A.UnitId == B.UnitId &&
        A.UnitHealth == B.UnitHealth &&
        A.TotalHealth == B.TotalHealth &&
        A.Attack == B.Attack &&
        A.Defense == B.Defense &&
        A.Initiative == B.Initiative;
}
}

bool FAIHeroesProceduralCampaignRules::CreateNew(
    uint32 Seed, int32 ExtraRoutes, UAIHeroesProceduralCampaignSave& Out,
    FString& Error)
{
    Error.Empty();
    if (ExtraRoutes < 0 || ExtraRoutes > 4)
    {
        Error = TEXT("Procedural route count must be between zero and four.");
        return false; // Reject before changing the supplied SaveGame object.
    }
    State Proposed;
    std::string Why;
    if (!MakeNew(Seed, static_cast<uint32>(ExtraRoutes), Proposed, Why) ||
        !AIHeroesProceduralCampaignCore::Validate(Proposed, Why))
    {
        Error = UTF8_TO_TCHAR(Why.c_str());
        return false;
    }
    Populate(Proposed, Out);
    return true;
}

bool FAIHeroesProceduralCampaignRules::Validate(
    const UAIHeroesProceduralCampaignSave& Save, FString& Error)
{
    Error.Empty();
    if (Save.ZoneStates.Num() < 7 || Save.ZoneStates.Num() > 11 ||
        Save.PlayerArmy.Num() > 2 || Save.ExtraRoutes < 0 ||
        Save.ExtraRoutes > 4)
    {
        Error = TEXT("Procedural snapshot exceeds zone, unit or route limits.");
        return false;
    }
    const State S = ToPortable(Save);
    std::string Why;
    if (!AIHeroesProceduralCampaignCore::Validate(S, Why))
    {
        Error = UTF8_TO_TCHAR(Why.c_str());
        return false;
    }
    return true;
}

bool FAIHeroesProceduralCampaignRules::BuildPlayerGraph(
    const UAIHeroesProceduralCampaignSave& Save,
    FAIHeroesAdventureGraph& OutGraph, FString& Error)
{
    OutGraph = FAIHeroesAdventureGraph();
    // Validate FIRST: generating a complete map is not permission to
    // expose undiscovered identities, edge costs or guarded objectives.
    if (!Validate(Save, Error)) return false;

    AIHeroesAdventureCore::Graph Hidden;
    std::string Why;
    if (!AIHeroesProceduralCampaignCore::BuildPlayerGraph(
            ToPortable(Save), Hidden, Why))
    {
        Error = UTF8_TO_TCHAR(Why.c_str());
        return false;
    }
    FAIHeroesAdventureGraph Proposed;
    Proposed.Revision = Hidden.Revision;
    for (const auto& Z : Hidden.Nodes)
        Proposed.Nodes.Add({
            FName(UTF8_TO_TCHAR(Z.Id.c_str())),
            Z.Discovered, Z.Passable, Z.Guard
        });
    for (const auto& Road : Hidden.Links)
        Proposed.Links.Add({
            FName(UTF8_TO_TCHAR(Road.From.c_str())),
            FName(UTF8_TO_TCHAR(Road.To.c_str())),
            Road.Cost, Road.Enabled
        });
    OutGraph = MoveTemp(Proposed);
    return true;
}

bool FAIHeroesProceduralCampaignRules::Equivalent(
    const UAIHeroesProceduralCampaignSave& A,
    const UAIHeroesProceduralCampaignSave& B)
{
    if (A.SchemaVersion != B.SchemaVersion ||
        A.GeneratorSchema != B.GeneratorSchema ||
        A.Seed != B.Seed || A.ExtraRoutes != B.ExtraRoutes ||
        A.TopologyRevision != B.TopologyRevision ||
        A.SaveSequence != B.SaveSequence ||
        A.ActionRevision != B.ActionRevision ||
        A.Day != B.Day || A.MovesLeft != B.MovesLeft ||
        A.Gold != B.Gold || A.Wood != B.Wood || A.Ore != B.Ore ||
        A.ProtocolTier != B.ProtocolTier ||
        A.ModelSlotTier != B.ModelSlotTier ||
        A.DataFragments != B.DataFragments ||
        A.HeroZoneId != B.HeroZoneId ||
        A.SelectedZoneId != B.SelectedZoneId ||
        A.ZoneStates.Num() != B.ZoneStates.Num() ||
        A.PlayerArmy.Num() != B.PlayerArmy.Num())
        return false;

    for (int32 I = 0; I < A.ZoneStates.Num(); ++I)
    {
        const auto& L = A.ZoneStates[I];
        const auto& R = B.ZoneStates[I];
        if (L.ZoneId != R.ZoneId ||
            L.bDiscovered != R.bDiscovered ||
            L.bOwned != R.bOwned ||
            L.bGuardCleared != R.bGuardCleared)
            return false;
    }
    for (int32 I = 0; I < A.PlayerArmy.Num(); ++I)
        if (!SameArmy(A.PlayerArmy[I], B.PlayerArmy[I])) return false;
    return true;
}

EAIHeroesProceduralLegacyDecision
FAIHeroesProceduralCampaignRules::AssessLegacy(
    const UAIHeroesCampaignSave& Legacy, FString& Reason)
{
    Reason.Empty();
    if (!FAIHeroesCampaignRules::Validate(Legacy, Reason))
    {
        Reason = TEXT("V1 VALIDATION FAILED: ") + Reason;
        return EAIHeroesProceduralLegacyDecision::InvalidLegacy;
    }
    Reason = TEXT(
        "NEW CAMPAIGN REQUIRED: validated five-site v1 progression cannot "
        "be mapped to a generated 7-11-zone v2 world without inventing "
        "ownership, guards, routes, fog and battle history. "
        "The original v1 snapshot stays read-only and unchanged.");
    return EAIHeroesProceduralLegacyDecision::RequiresNewCampaign;
}
