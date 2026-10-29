#pragma once

#include "AIHeroesAdventurePathCore.h"
#include <algorithm>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

// Pure C++17 generator of a *future* AI HEROES zone graph.
// Not the active five-node campaign and not imported HotA code/assets.
// No time, IO, global RNG, engine objects, LLM calls or player authority.
namespace AIHeroesZoneTemplateCore
{
constexpr std::uint32_t Schema = 1;
constexpr std::uint32_t MaxBranches = 4;
constexpr std::size_t BaseZones = 7;
constexpr std::size_t MaxZones = BaseZones + MaxBranches;
constexpr std::int32_t MaxRoadCost = 4;
constexpr std::int32_t ProofBudget = 1000;

enum class Role : std::uint8_t
{
    Headquarters, Hub, Resource, Checkpoint, Objective, SideRoute
};

struct Zone
{
    std::string Id;
    Role Type = Role::SideRoute;
    std::int32_t X = 0;
    std::int32_t Y = 0;
    std::int32_t Danger = 0;
    bool Guarded = false;
};

struct Road
{
    std::string From;
    std::string To;
    std::int32_t Cost = 1;
};

struct World
{
    std::uint32_t Version = Schema;
    std::uint32_t Seed = 0;
    std::uint32_t Revision = 0; // Change detector, not a signed authorization.
    std::vector<Zone> Zones;
    std::vector<Road> Roads; // Every road is explicitly directed.
};

inline std::uint32_t Next(std::uint32_t& State)
{
    // Fixed-width SplitMix32. Identical output on native and UE platforms.
    State += 0x9e3779b9u;
    std::uint32_t X = State;
    X = (X ^ (X >> 16u)) * 0x85ebca6bu;
    X = (X ^ (X >> 13u)) * 0xc2b2ae35u;
    return X ^ (X >> 16u);
}

inline std::uint32_t Hash(const World& W)
{
    std::uint32_t H = 2166136261u;
    const auto Byte = [&H](std::uint8_t B) { H = (H ^ B) * 16777619u; };
    const auto Number = [&Byte](std::uint32_t X)
    {
        for (int Shift = 0; Shift < 32; Shift += 8)
            Byte(static_cast<std::uint8_t>(X >> Shift));
    };
    const auto Text = [&Byte, &Number](const std::string& S)
    {
        Number(static_cast<std::uint32_t>(S.size()));
        for (unsigned char C : S) Byte(C);
    };
    Number(W.Version); Number(W.Seed);
    Number(static_cast<std::uint32_t>(W.Zones.size()));
    Number(static_cast<std::uint32_t>(W.Roads.size()));
    for (const auto& Z : W.Zones)
    {
        Text(Z.Id);
        Byte(static_cast<std::uint8_t>(Z.Type));
        Number(static_cast<std::uint32_t>(Z.X));
        Number(static_cast<std::uint32_t>(Z.Y));
        Number(static_cast<std::uint32_t>(Z.Danger));
        Byte(Z.Guarded ? 1 : 0);
    }
    for (const auto& R : W.Roads)
    {
        Text(R.From); Text(R.To);
        Number(static_cast<std::uint32_t>(R.Cost));
    }
    return H == 0 ? 1 : H;
}

inline void AddRoad(World& W, const char* A, const char* B, std::uint32_t& State)
{
    const auto Cost = static_cast<std::int32_t>(1u + Next(State) % MaxRoadCost);
    W.Roads.push_back({A, B, Cost});
    W.Roads.push_back({B, A, Cost});
}

inline AIHeroesAdventureCore::Graph AsAdventure(const World& W)
{
    AIHeroesAdventureCore::Graph G;
    G.Revision = W.Revision;
    for (const auto& Z : W.Zones)
        G.Nodes.push_back({Z.Id, true, true, Z.Guarded});
    for (const auto& R : W.Roads)
        G.Links.push_back({R.From, R.To, R.Cost, true});
    return G; // For validation only. Live fog-of-war must be filtered separately.
}

inline bool IsReady(const AIHeroesAdventureCore::Graph& G, const char* Destination)
{
    return AIHeroesAdventureCore::Find(G, "HQ", Destination, ProofBudget).State ==
        AIHeroesAdventureCore::Status::Ready;
}

inline bool IsUnreachable(const AIHeroesAdventureCore::Graph& G, const char* Destination)
{
    return AIHeroesAdventureCore::Find(G, "HQ", Destination, ProofBudget).State ==
        AIHeroesAdventureCore::Status::Unreachable;
}

inline bool IsAllowedPair(const std::string& A, const std::string& B,
                          std::uint32_t Branches)
{
    const auto Same = [&A, &B](const char* P, const char* Q)
    {
        return (A == P && B == Q) || (A == Q && B == P);
    };
    if (Same("HQ", "SUPPLY") || Same("SUPPLY", "FOREST") ||
        Same("SUPPLY", "ORE") || Same("FOREST", "PASS") ||
        Same("ORE", "PASS") || Same("PASS", "OUTPOST") ||
        Same("PASS", "RUINS") || Same("FOREST", "ORE") ||
        Same("OUTPOST", "RUINS"))
    {
        return true;
    }
    if (Branches >= 1 &&
        (Same("SUPPLY", "WAY_01") || Same("WAY_01", "FOREST"))) return true;
    if (Branches >= 2 &&
        (Same("SUPPLY", "WAY_02") || Same("WAY_02", "ORE"))) return true;
    if (Branches >= 3 &&
        (Same("PASS", "WAY_03") || Same("WAY_03", "OUTPOST"))) return true;
    if (Branches >= 4 &&
        (Same("PASS", "WAY_04") || Same("WAY_04", "RUINS"))) return true;
    return false;
}

inline bool Validate(const World& W, std::string& Error)
{
    Error.clear();
    const auto Fail = [&Error](const char* Why)
    {
        Error = Why;
        return false;
    };
    if (W.Version != Schema || W.Zones.size() < BaseZones ||
        W.Zones.size() > MaxZones || W.Roads.size() > 48 ||
        W.Revision == 0 || Hash(W) != W.Revision)
        return Fail("schema, graph bounds, or revision mismatch");

    const std::uint32_t Branches =
        static_cast<std::uint32_t>(W.Zones.size() - BaseZones);
    const char* Ids[] = {"HQ", "SUPPLY", "FOREST", "ORE", "PASS", "OUTPOST", "RUINS"};
    const Role Roles[] = {Role::Headquarters, Role::Hub, Role::Resource,
                          Role::Resource, Role::Checkpoint, Role::Objective,
                          Role::Objective};
    const bool Guards[] = {false, false, true, true, true, true, true};
    const std::int32_t Dangers[] = {0, 0, 1, 1, 2, 3, 3};
    std::set<std::pair<std::int32_t, std::int32_t>> Positions;
    for (std::size_t I = 0; I < W.Zones.size(); ++I)
    {
        const auto& Z = W.Zones[I];
        const bool Base = I < BaseZones;
        const std::string ExpectedId = Base ? Ids[I] :
            "WAY_0" + std::to_string(I - BaseZones + 1);
        if (Z.Id != ExpectedId || AIHeroesAdventureCore::Key(Z.Id).empty() ||
            Z.Type != (Base ? Roles[I] : Role::SideRoute) ||
            Z.Guarded != (Base ? Guards[I] : false) ||
            Z.Danger != (Base ? Dangers[I] : 0) ||
            Z.X < -32 || Z.X > 32 || Z.Y < -32 || Z.Y > 32 ||
            !Positions.emplace(Z.X, Z.Y).second)
        {
            return Fail("wrong zone identity, role, guard, tier or unique position");
        }
    }

    std::map<std::pair<std::string, std::string>, std::int32_t> Links;
    for (const auto& R : W.Roads)
    {
        if (!IsAllowedPair(R.From, R.To, Branches) ||
            R.From == R.To || R.Cost < 1 || R.Cost > MaxRoadCost ||
            !Links.emplace(std::make_pair(R.From, R.To), R.Cost).second)
            return Fail("unknown, duplicate, self-loop or over-budget road");
    }
    for (const auto& Pair : Links)
    {
        const auto Reciprocal = Links.find({Pair.first.second, Pair.first.first});
        if (Reciprocal == Links.end() || Reciprocal->second != Pair.second)
            return Fail("roads must be bidirectional and have identical costs");
    }
    const auto Need = [&Links](const char* A, const char* B)
    {
        return Links.find({A, B}) != Links.end() &&
            Links.find({B, A}) != Links.end();
    };
    if (!Need("HQ", "SUPPLY") || !Need("SUPPLY", "FOREST") ||
        !Need("SUPPLY", "ORE") || !Need("FOREST", "PASS") ||
        !Need("ORE", "PASS") || !Need("PASS", "OUTPOST") ||
        !Need("PASS", "RUINS")) return Fail("mandatory zone connection missing");
    const char* BranchIds[] = {"WAY_01", "WAY_02", "WAY_03", "WAY_04"};
    const char* BranchFrom[] = {"SUPPLY", "SUPPLY", "PASS", "PASS"};
    const char* BranchTo[] = {"FOREST", "ORE", "OUTPOST", "RUINS"};
    for (std::uint32_t I = 0; I < Branches; ++I)
    {
        if (!Need(BranchFrom[I], BranchIds[I]) ||
            !Need(BranchIds[I], BranchTo[I]))
            return Fail("side route is disconnected");
    }

    auto G = AsAdventure(W);
    // Initial player has two legal guarded choices, but cannot walk
    // through guards and claim deeper objectives without encounters.
    if (!IsReady(G, "FOREST") || !IsReady(G, "ORE") ||
        !IsUnreachable(G, "PASS") || !IsUnreachable(G, "OUTPOST") ||
        !IsUnreachable(G, "RUINS"))
        return Fail("initial guarded-frontier access contract failed");
    for (auto& N : G.Nodes) if (N.Id == "FOREST") N.Guard = false;
    if (!IsReady(G, "PASS") || !IsUnreachable(G, "OUTPOST") ||
        !IsUnreachable(G, "RUINS"))
        return Fail("checkpoint must be reachable only after early clearance");
    for (auto& N : G.Nodes) if (N.Id == "PASS") N.Guard = false;
    if (!IsReady(G, "OUTPOST") || !IsReady(G, "RUINS"))
        return Fail("all mission goals must be reachable after the checkpoint");
    for (auto& N : G.Nodes) N.Guard = false;
    for (const auto& N : G.Nodes)
        if (!IsReady(G, N.Id.c_str())) return Fail("optional zone disconnected");
    return true;
}

inline bool Generate(std::uint32_t Seed, std::uint32_t Branches,
                     World& Out, std::string& Error)
{
    Out = World();
    Error.clear();
    if (Branches > MaxBranches)
    {
        Error = "too many side routes";
        return false;
    }
    std::uint32_t State = Seed ^ 0x7f4a7c15u;
    World Proposed;
    Proposed.Seed = Seed;
    const auto Add = [&Proposed, &State](
        const char* Id, Role Type, std::int32_t BaseX,
        std::int32_t BaseY, std::int32_t Danger, bool Guarded)
    {
        Proposed.Zones.push_back({Id, Type,
            BaseX + static_cast<std::int32_t>(Next(State) % 3u) - 1,
            BaseY + static_cast<std::int32_t>(Next(State) % 3u) - 1,
            Danger, Guarded});
    };
    Add("HQ", Role::Headquarters, 0, 0, 0, false);
    Add("SUPPLY", Role::Hub, 4, 0, 0, false);
    Add("FOREST", Role::Resource, 8, -4, 1, true);
    Add("ORE", Role::Resource, 8, 4, 1, true);
    Add("PASS", Role::Checkpoint, 12, 0, 2, true);
    Add("OUTPOST", Role::Objective, 16, -4, 3, true);
    Add("RUINS", Role::Objective, 16, 4, 3, true);
    const char* ExtraIds[] = {"WAY_01", "WAY_02", "WAY_03", "WAY_04"};
    const std::int32_t ExtraX[] = {5, 5, 13, 13};
    const std::int32_t ExtraY[] = {-8, 8, -8, 8};
    for (std::uint32_t I = 0; I < Branches; ++I)
        Add(ExtraIds[I], Role::SideRoute, ExtraX[I], ExtraY[I], 0, false);

    AddRoad(Proposed, "HQ", "SUPPLY", State);
    AddRoad(Proposed, "SUPPLY", "FOREST", State);
    AddRoad(Proposed, "SUPPLY", "ORE", State);
    AddRoad(Proposed, "FOREST", "PASS", State);
    AddRoad(Proposed, "ORE", "PASS", State);
    AddRoad(Proposed, "PASS", "OUTPOST", State);
    AddRoad(Proposed, "PASS", "RUINS", State);
    const char* ExtraFrom[] = {"SUPPLY", "SUPPLY", "PASS", "PASS"};
    const char* ExtraTo[] = {"FOREST", "ORE", "OUTPOST", "RUINS"};
    for (std::uint32_t I = 0; I < Branches; ++I)
    {
        AddRoad(Proposed, ExtraFrom[I], ExtraIds[I], State);
        AddRoad(Proposed, ExtraIds[I], ExtraTo[I], State);
    }
    if (Next(State) % 2u == 0u) AddRoad(Proposed, "FOREST", "ORE", State);
    if (Next(State) % 2u == 0u) AddRoad(Proposed, "OUTPOST", "RUINS", State);
    Proposed.Revision = Hash(Proposed);
    if (!Validate(Proposed, Error)) return false;
    Out = std::move(Proposed);
    return true;
}
} // namespace AIHeroesZoneTemplateCore
