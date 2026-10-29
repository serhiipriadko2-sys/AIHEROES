#pragma once

#include "AIHeroesZoneTemplateCore.h"
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

// Portable C++17 state contract for a FUTURE procedural campaign.
// No UE, SaveGame slots, IO, wall-clock, LLM or player-action authority.
// V1 fixed-map saves are not convertible by this contract.
namespace AIHeroesProceduralCampaignCore
{
constexpr std::uint32_t SaveSchema = 2;
constexpr std::int32_t MaximumDay = 1000000;
constexpr std::int32_t MaximumSequence = 1000000;
constexpr std::int32_t MaximumResource = 500000000;
constexpr std::int32_t MaximumUnitHealth = 500000;
constexpr std::int32_t DailyMovement = 4;

struct ZoneState
{
    std::string Id;
    bool Discovered = false;
    bool Owned = false;
    bool GuardCleared = false;
};

struct FriendlyStack
{
    std::string UnitId;
    std::int32_t UnitHealth = 0;
    std::int32_t TotalHealth = 0;
    std::int32_t Attack = 0;
    std::int32_t Defense = 0;
    std::int32_t Initiative = 0;
};

struct State
{
    std::uint32_t SchemaVersion = SaveSchema;
    std::uint32_t GeneratorSchema = AIHeroesZoneTemplateCore::Schema;
    std::uint32_t Seed = 0;
    std::uint32_t Branches = 0;
    std::uint32_t TopologyRevision = 0; // Generator fingerprint, NOT security.
    std::int32_t SaveSequence = 0;
    std::int32_t ActionRevision = 0;     // Staleness marker, NOT consent.
    std::int32_t Day = 1;
    std::int32_t MovesLeft = DailyMovement;
    std::int32_t Gold = 240;
    std::int32_t Wood = 35;
    std::int32_t Ore = 30;
    std::int32_t ProtocolTier = 0;
    std::int32_t ModelSlotTier = 0;
    std::int32_t DataFragments = 0;
    std::string HeroZoneId = "HQ";
    std::string SelectedZoneId;
    std::vector<ZoneState> Zones;
    std::vector<FriendlyStack> PlayerArmy;
};

inline bool Fail(std::string& Error, const char* Why)
{
    Error = Why;
    return false;
}

inline bool MakeNew(std::uint32_t Seed, std::uint32_t Branches,
                    State& Out, std::string& Error)
{
    Out = State();
    Error.clear();
    AIHeroesZoneTemplateCore::World World;
    if (!AIHeroesZoneTemplateCore::Generate(Seed, Branches, World, Error))
        return false;

    State Proposed;
    Proposed.Seed = Seed;
    Proposed.Branches = Branches;
    Proposed.TopologyRevision = World.Revision;
    for (const auto& Zone : World.Zones)
    {
        Proposed.Zones.push_back({
            Zone.Id,
            Zone.Id == "HQ" || Zone.Id == "SUPPLY" ||
                Zone.Id == "FOREST" || Zone.Id == "ORE",
            Zone.Id == "HQ",
            !Zone.Guarded
        });
    }
    Proposed.PlayerArmy.push_back({"Militia", 12, 96, 4, 2, 3});
    Proposed.PlayerArmy.push_back({"Scout", 8, 24, 6, 1, 7});
    Out = std::move(Proposed);
    return true;
}

inline bool ValidFriendly(const FriendlyStack& Unit)
{
    if (Unit.TotalHealth < 0 || Unit.TotalHealth > MaximumUnitHealth)
        return false;
    if (Unit.UnitId == "Militia")
        return Unit.UnitHealth == 12 && Unit.Attack == 4 &&
            Unit.Defense == 2 && Unit.Initiative == 3;
    if (Unit.UnitId == "Scout")
        return Unit.UnitHealth == 8 && Unit.Attack == 6 &&
            Unit.Defense == 1 && Unit.Initiative == 7;
    return false;
}

inline bool Validate(const State& S, std::string& Error)
{
    Error.clear();
    if (S.SchemaVersion != SaveSchema ||
        S.GeneratorSchema != AIHeroesZoneTemplateCore::Schema ||
        S.Branches > AIHeroesZoneTemplateCore::MaxBranches ||
        S.TopologyRevision == 0 ||
        S.Zones.size() != AIHeroesZoneTemplateCore::BaseZones + S.Branches ||
        S.PlayerArmy.size() > 2u)
        return Fail(Error, "save schema, bounds or topology version invalid");

    AIHeroesZoneTemplateCore::World Expected;
    if (!AIHeroesZoneTemplateCore::Generate(
            S.Seed, S.Branches, Expected, Error) ||
        Expected.Revision != S.TopologyRevision)
        return Fail(Error, "seed and generator topology revision mismatch");

    if (S.SaveSequence < 0 || S.SaveSequence > MaximumSequence ||
        S.ActionRevision < 0 || S.ActionRevision > MaximumSequence ||
        S.Day < 1 || S.Day > MaximumDay ||
        S.MovesLeft < 0 || S.MovesLeft > DailyMovement ||
        S.ProtocolTier < 0 || S.ProtocolTier > 3 ||
        S.ModelSlotTier < 0 || S.ModelSlotTier > 2 ||
        S.DataFragments < 0 || S.DataFragments > MaximumResource ||
        S.Gold < 0 || S.Gold > MaximumResource ||
        S.Wood < 0 || S.Wood > MaximumResource ||
        S.Ore < 0 || S.Ore > MaximumResource)
        return Fail(Error, "turn, resources or cognitive progress outside limits");

    bool SeenMilitia = false;
    bool SeenScout = false;
    for (const auto& Unit : S.PlayerArmy)
    {
        if (!ValidFriendly(Unit))
            return Fail(Error, "unknown or forged friendly unit");
        if (Unit.UnitId == "Militia")
        {
            if (SeenMilitia) return Fail(Error, "duplicate militia stack");
            SeenMilitia = true;
        }
        else
        {
            if (SeenScout) return Fail(Error, "duplicate scout stack");
            SeenScout = true;
        }
    }

    bool HeroAtOwned = false;
    bool SelectionKnown = S.SelectedZoneId.empty();
    bool AnyGuardDefeated = false;
    auto Full = AIHeroesZoneTemplateCore::AsAdventure(Expected);
    for (std::size_t I = 0; I < Expected.Zones.size(); ++I)
    {
        const auto& Def = Expected.Zones[I];
        const auto& Z = S.Zones[I];
        if (Z.Id != Def.Id)
            return Fail(Error, "zone IDs are not canonical or duplicated");
        if (I < 4u && !Z.Discovered)
            return Fail(Error, "starting scouting knowledge was concealed");
        if ((!Def.Guarded && !Z.GuardCleared) ||
            (Z.Owned && (!Z.Discovered || !Z.GuardCleared)) ||
            (Def.Guarded && Z.GuardCleared && (!Z.Discovered || !Z.Owned)))
            return Fail(Error, "impossible ownership, exploration or guard state");
        if (Z.Id == "HQ" && (!Z.Owned || !Z.Discovered || !Z.GuardCleared))
            return Fail(Error, "headquarters must be held and safe");
        if (Z.Id == S.HeroZoneId && Z.Owned)
            HeroAtOwned = true;
        if (Z.Id == S.SelectedZoneId && Z.Discovered)
            SelectionKnown = true;
        if (Def.Guarded && Z.GuardCleared)
            AnyGuardDefeated = true;
        Full.Nodes[I].Discovered = Z.Discovered;
        Full.Nodes[I].Guard = Def.Guarded && !Z.GuardCleared;
    }
    if (!HeroAtOwned || !SelectionKnown)
        return Fail(Error, "hero or selection is not discovered and owned");
    if (AnyGuardDefeated && S.ActionRevision == 0)
        return Fail(Error, "guard cannot be cleared before first authoritative action");

    // A claimed zone must be reachable from HQ without crossing an
    // undefeated guard. A topology hash alone never proves this.
    for (const auto& Z : S.Zones)
    {
        if (!Z.Owned) continue;
        const auto Path = AIHeroesAdventureCore::Find(
            Full, "HQ", Z.Id, AIHeroesZoneTemplateCore::ProofBudget);
        if (Path.State != AIHeroesAdventureCore::Status::Ready ||
            Path.Battle)
            return Fail(Error, "captured territory bypasses a guarded frontier");
    }
    return true;
}

inline bool BuildPlayerGraph(const State& S,
                             AIHeroesAdventureCore::Graph& Out,
                             std::string& Error)
{
    Out = AIHeroesAdventureCore::Graph();
    if (!Validate(S, Error)) return false;

    AIHeroesZoneTemplateCore::World World;
    if (!AIHeroesZoneTemplateCore::Generate(
            S.Seed, S.Branches, World, Error))
        return false;

    AIHeroesAdventureCore::Graph Proposed;
    // Changes whenever the snapshot's action epoch changes. This remains
    // a STALE-PREVIEW detector, NOT permission to perform an action.
    Proposed.Revision = World.Revision ^
        (static_cast<std::uint32_t>(S.ActionRevision) * 0x9e3779b9u);
    for (std::size_t I = 0; I < World.Zones.size(); ++I)
    {
        const auto& Z = S.Zones[I];
        if (!Z.Discovered) continue; // Hidden IDs are omitted entirely.
        Proposed.Nodes.push_back({
            Z.Id, true, true, World.Zones[I].Guarded && !Z.GuardCleared
        });
    }
    for (const auto& Road : World.Roads)
    {
        const auto From = std::find_if(S.Zones.begin(), S.Zones.end(),
            [&Road](const ZoneState& Z)
            { return Z.Id == Road.From && Z.Discovered; });
        const auto To = std::find_if(S.Zones.begin(), S.Zones.end(),
            [&Road](const ZoneState& Z)
            { return Z.Id == Road.To && Z.Discovered; });
        if (From != S.Zones.end() && To != S.Zones.end())
            Proposed.Links.push_back({
                Road.From, Road.To, Road.Cost, true
            });
    }
    Out = std::move(Proposed);
    return true;
}
} // namespace AIHeroesProceduralCampaignCore
