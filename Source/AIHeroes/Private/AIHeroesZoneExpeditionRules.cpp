#include "AIHeroesZoneExpeditionRules.h"

#include "AIHeroesBattleEngine.h"
#include "AIHeroesGridBattle.h"

namespace
{
constexpr int32 MaxArmyHP = 500000;
constexpr int32 MaximumEvents = 300;

bool Fail(FString& Error, const TCHAR* Message)
{
    Error = Message;
    return false;
}

int32 FindIndex(const FAIHeroesGeneratedWorld& W, FName Site)
{
    for (int32 I = 0; I < W.Zones.Num(); ++I)
    {
        if (W.Zones[I].Id == Site) return I;
    }
    return INDEX_NONE;
}

uint32 Flag(int32 Index)
{
    return Index < 0 || Index >= 31 ? 0u : 1u << Index;
}

bool UnitAllowed(const FAIHeroesArmyStack& Unit)
{
    if (Unit.TotalHealth < 0 || Unit.TotalHealth > MaxArmyHP) return false;
    if (Unit.UnitId == FName(TEXT("Militia")))
        return Unit.UnitHealth == 12 && Unit.Attack == 4 &&
            Unit.Defense == 2 && Unit.Initiative == 3;
    if (Unit.UnitId == FName(TEXT("Scout")))
        return Unit.UnitHealth == 8 && Unit.Attack == 6 &&
            Unit.Defense == 1 && Unit.Initiative == 7;
    return false;
}

bool Living(const TArray<FAIHeroesArmyStack>& Army)
{
    for (const auto& Unit : Army)
        if (Unit.IsAlive()) return true;
    return false;
}

// The generated template is used exclusively as a bounded authority
// for topology. Hidden nodes/edges never leave BuildKnownGraph.
bool BuildWorld(const FAIHeroesZoneExpeditionState& S,
    FAIHeroesGeneratedWorld& Out, FString& Error)
{
    if (S.Schema != FAIHeroesZoneExpeditionRules::Schema ||
        S.BranchCount < 0 || S.BranchCount > FAIHeroesZoneTemplate::MaxExtraRoutes ||
        !FAIHeroesZoneTemplate::Generate(S.Seed, S.BranchCount, Out, Error))
    {
        if (Error.IsEmpty()) Error = TEXT("Unsupported expedition map schema.");
        return false;
    }
    if (S.MapRevision != Out.Revision)
        return Fail(Error, TEXT("Generated map revision does not match seed."));
    return true;
}

uint32 ExpectedExploration(const FAIHeroesGeneratedWorld& W, uint32 Cleared)
{
    uint32 Result = FAIHeroesZoneExpeditionRules::StartingKnownMask;
    for (const auto& Link : W.Adventure.Links)
    {
        const int32 A = FindIndex(W, Link.From);
        const int32 B = FindIndex(W, Link.To);
        if (A != INDEX_NONE && B != INDEX_NONE && (Cleared & Flag(A)) != 0)
            Result |= Flag(B);
    }
    return Result;
}

bool GenerateVisible(const FAIHeroesGeneratedWorld& W,
    const FAIHeroesZoneExpeditionState& S, FAIHeroesAdventureGraph& Out)
{
    Out = FAIHeroesAdventureGraph();
    Out.Revision = W.Revision ^ S.DiscoveredMask * 2654435761u ^
        S.ClearedMask * 2246822519u;
    if (Out.Revision == 0) Out.Revision = 1;
    for (int32 I = 0; I < W.Zones.Num(); ++I)
    {
        const uint32 Bit = Flag(I);
        if ((S.DiscoveredMask & Bit) == 0) continue;
        const auto& Zone = W.Zones[I];
        Out.Nodes.Add({Zone.Id, true, true,
            Zone.bGuarded && (S.ClearedMask & Bit) == 0});
    }
    for (const auto& Link : W.Adventure.Links)
    {
        const int32 A = FindIndex(W, Link.From);
        const int32 B = FindIndex(W, Link.To);
        if (A == INDEX_NONE || B == INDEX_NONE) return false;
        if ((S.DiscoveredMask & Flag(A)) != 0 &&
            (S.DiscoveredMask & Flag(B)) != 0)
            Out.Links.Add(Link);
    }
    return true;
}

bool CanReachCleared(const FAIHeroesGeneratedWorld& W,
    const FAIHeroesZoneExpeditionState& S)
{
    FAIHeroesAdventureGraph Graph;
    if (!GenerateVisible(W, S, Graph)) return false;
    for (int32 I = 0; I < W.Zones.Num(); ++I)
    {
        if ((S.ClearedMask & Flag(I)) == 0) continue;
        const auto Path = FAIHeroesAdventurePath::Find(
            Graph, FName(TEXT("HQ")), W.Zones[I].Id, 1000);
        if (!Path.IsReachableThisTurn()) return false;
    }
    return true;
}

TArray<FAIHeroesArmyStack> EncounterFor(const FAIHeroesGeneratedZone& Zone)
{
    switch (Zone.Role)
    {
        case EAIHeroesZoneRole::Resource:
            if (Zone.Id == FName(TEXT("FOREST")))
                return {FAIHeroesArmyStack(TEXT("Wolf"), 4, 9, 3, 1, 8)};
            return {FAIHeroesArmyStack(TEXT("Bandit"), 6, 10, 4, 2, 4)};
        case EAIHeroesZoneRole::Checkpoint:
            return {FAIHeroesArmyStack(TEXT("Sentinel"), 7, 13, 5, 2, 4)};
        case EAIHeroesZoneRole::Objective:
            if (Zone.Id == FName(TEXT("OUTPOST")))
                return {FAIHeroesArmyStack(TEXT("Raider"), 7, 11, 5, 2, 5)};
            return {FAIHeroesArmyStack(TEXT("Guardian"), 7, 24, 7, 4, 2)};
        default:
            return {};
    }
}

void GrantReward(FAIHeroesZoneExpeditionState& S, const FAIHeroesGeneratedZone& Z)
{
    if (Z.Id == FName(TEXT("FOREST"))) S.Wood += 12;
    else if (Z.Id == FName(TEXT("ORE"))) S.Ore += 10;
    else if (Z.Id == FName(TEXT("PASS"))) S.Gold += 60;
    else if (Z.Id == FName(TEXT("OUTPOST"))) { S.Gold += 120; S.DataFragments += 3; }
    else if (Z.Id == FName(TEXT("RUINS"))) { S.Gold += 80; S.DataFragments += 5; }
    else if (Z.Role == EAIHeroesZoneRole::SideRoute) S.Gold += 15;
    else if (Z.Role == EAIHeroesZoneRole::Hub) S.Gold += 20;
    if (Z.bGuarded && Z.Role != EAIHeroesZoneRole::Objective)
        S.DataFragments += 2;
}

int32 CountBits(uint32 Bits)
{
    int32 Count = 0;
    for (; Bits; Bits &= Bits - 1) ++Count;
    return Count;
}
}

bool FAIHeroesZoneExpeditionRules::Create(uint32 Seed, int32 Branches,
    FAIHeroesZoneExpeditionState& Out, FString& Error)
{
    Out = FAIHeroesZoneExpeditionState();
    Error.Empty();
    FAIHeroesGeneratedWorld W;
    if (!FAIHeroesZoneTemplate::Generate(Seed, Branches, W, Error)) return false;
    FAIHeroesZoneExpeditionState Initial;
    Initial.Seed = Seed;
    Initial.BranchCount = Branches;
    Initial.MapRevision = W.Revision;
    Initial.HeroSite = FName(TEXT("HQ"));
    Initial.DiscoveredMask = StartingKnownMask;
    Initial.ClearedMask = 1u;
    Initial.PlayerArmy.Add(FAIHeroesArmyStack(TEXT("Militia"), 8, 12, 4, 2, 3));
    Initial.PlayerArmy.Add(FAIHeroesArmyStack(TEXT("Scout"), 3, 8, 6, 1, 7));
    Initial.LastEvent = TEXT("Expedition ready. Only the player can confirm movement or battle.");
    if (!Validate(Initial, Error)) return false;
    Out = MoveTemp(Initial);
    return true;
}

bool FAIHeroesZoneExpeditionRules::Validate(
    const FAIHeroesZoneExpeditionState& S, FString& Error)
{
    Error.Empty();
    FAIHeroesGeneratedWorld W;
    if (!BuildWorld(S, W, Error)) return false;
    const uint32 All = (1u << W.Zones.Num()) - 1u;
    if ((S.DiscoveredMask & StartingKnownMask) != StartingKnownMask ||
        (S.DiscoveredMask & ~All) != 0 ||
        (S.ClearedMask & ~S.DiscoveredMask) != 0 ||
        (S.ClearedMask & 1u) == 0 ||
        ExpectedExploration(W, S.ClearedMask) != S.DiscoveredMask)
        return Fail(Error, TEXT("Forged or inconsistent discovered/cleared zone mask."));
    const int32 Hero = FindIndex(W, S.HeroSite);
    if (Hero == INDEX_NONE || (S.ClearedMask & Flag(Hero)) == 0)
        return Fail(Error, TEXT("Hero is outside controlled known territory."));
    if (!CanReachCleared(W, S))
        return Fail(Error, TEXT("Expedition clearance bypasses a live guarded checkpoint."));
    if (S.Day < 1 || S.Day > MaxDay ||
        S.MovesLeft < 0 || S.MovesLeft > MovesPerDay ||
        S.Gold < 0 || S.Gold > MaxResource ||
        S.Wood < 0 || S.Wood > MaxResource ||
        S.Ore < 0 || S.Ore > MaxResource ||
        S.DataFragments < 0 || S.DataFragments > MaxResource ||
        S.ProtocolTier < 0 || S.ProtocolTier > 3 ||
        S.PlayerArmy.Num() > 2 || S.LastEvent.Len() > MaximumEvents)
        return Fail(Error, TEXT("Expedition economy, time, army or protocol out of bounds."));
    TSet<FName> Units;
    for (const auto& Unit : S.PlayerArmy)
    {
        if (!UnitAllowed(Unit) || Units.Contains(Unit.UnitId))
            return Fail(Error, TEXT("Forged or duplicate friendly army stack."));
        Units.Add(Unit.UnitId);
    }
    return true;
}

bool FAIHeroesZoneExpeditionRules::PlayerView(
    const FAIHeroesZoneExpeditionState& S,
    FAIHeroesAdventureGraph& Out, FString& Error)
{
    Out = FAIHeroesAdventureGraph();
    if (!Validate(S, Error)) return false;
    FAIHeroesGeneratedWorld W;
    if (!BuildWorld(S, W, Error) || !GenerateVisible(W, S, Out))
        return Fail(Error, TEXT("Cannot prepare visible expedition graph."));
    return true;
}

FAIHeroesAdventurePathResult FAIHeroesZoneExpeditionRules::Preview(
    const FAIHeroesZoneExpeditionState& S, FName Target)
{
    FAIHeroesAdventurePathResult Out;
    Out.Reason = TEXT("Expedition target not discovered or state invalid.");
    FAIHeroesAdventureGraph View;
    FString Error;
    if (!PlayerView(S, View, Error)) return Out;
    if (!View.Nodes.ContainsByPredicate(
        [Target](const auto& N) { return N.Id == Target; })) return Out;
    return FAIHeroesAdventurePath::Find(View, S.HeroSite, Target, S.MovesLeft);
}

bool FAIHeroesZoneExpeditionRules::ConfirmPlayerDestination(
    FAIHeroesZoneExpeditionState& S, FName Target,
    FAIHeroesZoneExpeditionResult& Result)
{
    Result = FAIHeroesZoneExpeditionResult();
    const auto Path = Preview(S, Target);
    if (!Path.IsReachableThisTurn())
    {
        Result.Message = Path.Reason;
        return false;
    }
    FAIHeroesGeneratedWorld W;
    FString Error;
    if (!BuildWorld(S, W, Error))
    {
        Result.Message = TEXT("Expedition topology failed preflight.");
        return false;
    }
    const int32 Index = FindIndex(W, Target);
    if (Index == INDEX_NONE)
    {
        Result.Message = TEXT("Unknown target.");
        return false;
    }
    const auto& Zone = W.Zones[Index];
    const bool bBattle = Zone.bGuarded && (S.ClearedMask & Flag(Index)) == 0;
    if (bBattle && !Living(S.PlayerArmy))
    {
        Result.Message = TEXT("No living troops. Only the player can recruit at HQ.");
        return false;
    }
    FAIHeroesZoneExpeditionState Candidate = S;
    Candidate.MovesLeft -= Path.TotalCost;
    Result.bBattle = bBattle;
    Result.MovementSpent = Path.TotalCost;

    if (bBattle)
    {
        const TArray<FAIHeroesArmyStack> Defenders = EncounterFor(Zone);
        if (Defenders.IsEmpty())
        {
            Result.Message = TEXT("Guard generation failed closed.");
            return false;
        }
        const FAIHeroesBattleOutcome Outcome = FAIHeroesGridBattle::RunToEnd(
            Candidate.PlayerArmy, Defenders, Candidate.ProtocolTier);
        if (!Outcome.bVictory && !Outcome.bDefeat)
        {
            Result.Message = TEXT("Grid battle has no validated terminal outcome.");
            return false;
        }
        Candidate.PlayerArmy = Outcome.PlayerAfter;
        Result.bVictory = Outcome.bVictory;
        if (!Outcome.bVictory)
        {
            Candidate.LastEvent = FString::Printf(
                TEXT("%s: battle lost. %d moves consumed; site NOT captured."),
                *Target.ToString(), Path.TotalCost).Left(MaximumEvents);
        }
    }
    else
    {
        Result.bVictory = true;
    }

    if (Result.bVictory)
    {
        const bool bNewClaim = (Candidate.ClearedMask & Flag(Index)) == 0;
        Candidate.ClearedMask |= Flag(Index);
        Candidate.HeroSite = Target;
        if (bNewClaim) GrantReward(Candidate, Zone);
        Candidate.DiscoveredMask = ExpectedExploration(W, Candidate.ClearedMask);
        Candidate.LastEvent = FString::Printf(
            TEXT("PLAYER MOVED: %s | %d moves | known %d/%d | cleared %d/%d"),
            *Target.ToString(), Path.TotalCost,
            CountBits(Candidate.DiscoveredMask), W.Zones.Num(),
            CountBits(Candidate.ClearedMask), W.Zones.Num());
    }
    if (!Validate(Candidate, Error))
    {
        Result.Message = TEXT("Action blocked; no campaign change: ") + Error;
        return false;
    }
    S = MoveTemp(Candidate); // Last, single authoritative commit.
    Result.bAccepted = true;
    Result.DiscoveredCount = CountBits(S.DiscoveredMask);
    Result.Message = S.LastEvent;
    return true;
}

bool FAIHeroesZoneExpeditionRules::EndPlayerDay(
    FAIHeroesZoneExpeditionState& S, FString& Error)
{
    if (!Validate(S, Error)) return false;
    if (S.Day >= MaxDay) return Fail(Error, TEXT("Day limit reached."));
    FAIHeroesZoneExpeditionState Candidate = S;
    ++Candidate.Day;
    Candidate.MovesLeft = MovesPerDay;
    Candidate.Gold += 40 + (IsCleared(S, FName(TEXT("OUTPOST"))) ? 30 : 0);
    if (IsCleared(S, FName(TEXT("FOREST")))) Candidate.Wood += 8;
    if (IsCleared(S, FName(TEXT("ORE")))) Candidate.Ore += 7;
    Candidate.LastEvent = FString::Printf(
        TEXT("DAY %d: player ended turn. Movement restored; owned sites paid income."),
        Candidate.Day);
    if (!Validate(Candidate, Error)) return false;
    S = MoveTemp(Candidate);
    return true;
}

bool FAIHeroesZoneExpeditionRules::RecruitAtHeadquarters(
    FAIHeroesZoneExpeditionState& S, FString& Error)
{
    if (!Validate(S, Error)) return false;
    if (S.HeroSite != FName(TEXT("HQ")) || S.Gold < 50 || S.Wood < 5)
        return Fail(Error, TEXT("Recruiting 5 Militia requires HQ and 50 Gold, 5 Wood."));
    FAIHeroesZoneExpeditionState Candidate = S;
    bool bFound = false;
    for (auto& Unit : Candidate.PlayerArmy)
        if (Unit.UnitId == FName(TEXT("Militia")))
        {
            if (Unit.TotalHealth > MaxArmyHP - 60)
                return Fail(Error, TEXT("Army health cap prevents recruiting."));
            Unit.TotalHealth += 60;
            bFound = true;
            break;
        }
    if (!bFound)
    {
        if (Candidate.PlayerArmy.Num() >= 2)
            return Fail(Error, TEXT("Army stack capacity reached."));
        Candidate.PlayerArmy.Add(
            FAIHeroesArmyStack(TEXT("Militia"), 5, 12, 4, 2, 3));
    }
    Candidate.Gold -= 50;
    Candidate.Wood -= 5;
    Candidate.LastEvent = TEXT("PLAYER RECRUITED 5 Militia at HQ; costs charged once.");
    if (!Validate(Candidate, Error)) return false;
    S = MoveTemp(Candidate);
    return true;
}

bool FAIHeroesZoneExpeditionRules::UpgradeProtocol(
    FAIHeroesZoneExpeditionState& S, FString& Error)
{
    if (!Validate(S, Error)) return false;
    if (S.ProtocolTier >= 3)
        return Fail(Error, TEXT("All three offline protocols already unlocked."));
    const int32 GoldPrices[] = {75, 115, 160};
    const int32 DataPrices[] = {2, 4, 7};
    const int32 Tier = S.ProtocolTier;
    if (S.Gold < GoldPrices[Tier] || S.DataFragments < DataPrices[Tier])
        return Fail(Error, TEXT("Insufficient Gold or DATA for protocol upgrade."));
    FAIHeroesZoneExpeditionState Candidate = S;
    Candidate.Gold -= GoldPrices[Tier];
    Candidate.DataFragments -= DataPrices[Tier];
    ++Candidate.ProtocolTier;
    Candidate.LastEvent = FString::Printf(
        TEXT("PLAYER UNLOCKED offline protocol tier %d; no model loaded."), Candidate.ProtocolTier);
    if (!Validate(Candidate, Error)) return false;
    S = MoveTemp(Candidate);
    return true;
}

bool FAIHeroesZoneExpeditionRules::IsDiscovered(
    const FAIHeroesZoneExpeditionState& S, FName Site)
{
    FAIHeroesGeneratedWorld W;
    FString Error;
    if (!BuildWorld(S, W, Error)) return false;
    const int32 Idx = FindIndex(W, Site);
    return Idx != INDEX_NONE && (S.DiscoveredMask & Flag(Idx)) != 0;
}

bool FAIHeroesZoneExpeditionRules::IsCleared(
    const FAIHeroesZoneExpeditionState& S, FName Site)
{
    FAIHeroesGeneratedWorld W;
    FString Error;
    if (!BuildWorld(S, W, Error)) return false;
    const int32 Idx = FindIndex(W, Site);
    return Idx != INDEX_NONE && (S.ClearedMask & Flag(Idx)) != 0;
}

bool FAIHeroesZoneExpeditionRules::HasWon(
    const FAIHeroesZoneExpeditionState& S)
{
    FString Error;
    if (!Validate(S, Error)) return false;
    return IsCleared(S, FName(TEXT("OUTPOST"))) &&
        IsCleared(S, FName(TEXT("RUINS")));
}

int32 FAIHeroesZoneExpeditionRules::DiscoveredCount(
    const FAIHeroesZoneExpeditionState& S)
{
    return CountBits(S.DiscoveredMask);
}
