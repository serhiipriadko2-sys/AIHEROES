#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

// Portable production kernel. No Unreal, clock, RNG, filesystem or network.
// Stable IDs are 1..64 ASCII letters/digits/underscore/hyphen, case-insensitive.
namespace AIHeroesAdventureCore
{
constexpr std::size_t MaxNodes = 128, MaxLinks = 1024;
constexpr std::int32_t MaxLinkCost = 1000000;
struct Node { std::string Id; bool Discovered = false, Passable = true, Guard = false; };
struct Link { std::string From, To; std::int32_t Cost = 1; bool Enabled = true; };
struct Graph { std::uint32_t Revision = 0; std::vector<Node> Nodes; std::vector<Link> Links; };
enum class Status { InvalidInput, Unreachable, NeedsMoreMoves, Ready };
struct Result
{
    Status State = Status::InvalidInput;
    std::vector<std::int32_t> NodeIndices;
    std::int32_t Cost = 0, Missing = 0;
    std::uint32_t Revision = 0;
    bool Battle = false;
};
inline std::string Key(const std::string& Id)
{
    if (Id.empty() || Id.size() > 64) return {};
    std::string Out;
    for (unsigned char C : Id)
    {
        if (C >= 'A' && C <= 'Z') C = static_cast<unsigned char>(C + ('a' - 'A'));
        if (!((C >= 'a' && C <= 'z') || (C >= '0' && C <= '9') || C == '_' || C == '-')) return {};
        Out.push_back(static_cast<char>(C));
    }
    return Out == "none" ? std::string{} : Out;
}
inline Result Find(const Graph& G, const std::string& Start, const std::string& Goal,
                   std::int32_t Budget)
{
    Result R;
    R.Revision = G.Revision;
    if (Budget < 0 || G.Nodes.empty() || G.Nodes.size() > MaxNodes || G.Links.size() > MaxLinks) return R;
    std::map<std::string, std::int32_t> ById;
    for (std::size_t I = 0; I < G.Nodes.size(); ++I)
    {
        const auto K = Key(G.Nodes[I].Id);
        if (K.empty() || !ById.emplace(K, static_cast<std::int32_t>(I)).second) return R;
    }
    const auto S = ById.find(Key(Start)), T = ById.find(Key(Goal));
    if (S == ById.end() || T == ById.end()) return R;
    const auto Source = S->second, Target = T->second;
    struct Edge { std::int32_t To, Cost; bool Enabled; };
    std::vector<std::vector<Edge>> Adjacency(G.Nodes.size());
    std::set<std::pair<std::int32_t, std::int32_t>> Seen;
    for (const auto& L : G.Links)
    {
        const auto A = ById.find(Key(L.From)), B = ById.find(Key(L.To));
        if (A == ById.end() || B == ById.end() || A->second == B->second ||
            L.Cost <= 0 || L.Cost > MaxLinkCost || !Seen.emplace(A->second, B->second).second) return R;
        Adjacency[A->second].push_back({B->second, L.Cost, L.Enabled});
    }
    const auto& Origin = G.Nodes[Source];
    if (!Origin.Discovered || !Origin.Passable || Origin.Guard) return R;
    R.State = Status::Unreachable;
    if (!G.Nodes[Target].Discovered || !G.Nodes[Target].Passable) return R;
    constexpr auto Infinity = std::numeric_limits<std::int64_t>::max() / 4;
    std::vector<std::int64_t> Distance(G.Nodes.size(), Infinity);
    std::vector<std::int32_t> Previous(G.Nodes.size(), -1);
    std::vector<bool> Settled(G.Nodes.size(), false);
    Distance[Source] = 0;
    for (std::size_t Iteration = 0; Iteration < G.Nodes.size(); ++Iteration)
    {
        std::int32_t Current = -1;
        // std::map supplies stable case-normalized lexical tie order.
        for (const auto& Entry : ById)
        {
            const auto Candidate = Entry.second;
            if (!Settled[Candidate] && Distance[Candidate] != Infinity &&
                (Current < 0 || Distance[Candidate] < Distance[Current])) Current = Candidate;
        }
        if (Current < 0) break;
        Settled[Current] = true;
        if (Current == Target) break;
        if (G.Nodes[Current].Guard) continue;
        for (const auto& E : Adjacency[Current])
        {
            const auto& N = G.Nodes[E.To];
            if (!E.Enabled || Settled[E.To] || !N.Discovered || !N.Passable) continue;
            const auto CandidateCost = Distance[Current] + E.Cost;
            if (CandidateCost < Distance[E.To])
            {
                Distance[E.To] = CandidateCost;
                Previous[E.To] = Current;
            }
        }
    }
    if (!Settled[Target]) return R;
    std::vector<std::int32_t> Path;
    auto Cursor = Target;
    for (std::size_t I = 0; I < G.Nodes.size(); ++I)
    {
        Path.push_back(Cursor);
        if (Cursor == Source) break;
        Cursor = Previous[Cursor];
        if (Cursor < 0 || static_cast<std::size_t>(Cursor) >= G.Nodes.size()) return R;
    }
    if (Path.empty() || Path.back() != Source || Distance[Target] > std::numeric_limits<std::int32_t>::max()) return R;
    std::reverse(Path.begin(), Path.end());
    R.NodeIndices = std::move(Path);
    R.Cost = static_cast<std::int32_t>(Distance[Target]);
    R.Missing = std::max<std::int32_t>(0, R.Cost - Budget);
    R.Battle = G.Nodes[Target].Guard;
    R.State = R.Missing > 0 ? Status::NeedsMoreMoves : Status::Ready;
    return R;
}
} // namespace AIHeroesAdventureCore
