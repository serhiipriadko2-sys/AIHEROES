#include "AIHeroesAdventurePathCore.h"
#include <iostream>
#include <random>
#include <stdexcept>

using namespace AIHeroesAdventureCore;
static int Checks = 0;
static void Check(bool Passed, const char* Message)
{
    ++Checks;
    if (!Passed) throw std::runtime_error(Message);
}
static Graph Sample()
{
    return {7, {{"HQ", true, true, false}, {"ROAD", true, true, false},
        {"ORE", true, true, false}}, {{"HQ", "ORE", 9, true},
        {"HQ", "ROAD", 2, true}, {"ROAD", "ORE", 2, true}}};
}
static std::vector<std::string> Names(const Graph& G, const Result& R)
{
    std::vector<std::string> Out;
    for (auto I : R.NodeIndices) Out.push_back(Key(G.Nodes.at(I).Id));
    return Out;
}
int main()
{
    try
    {
        auto G = Sample();
        auto R = Find(G, "hq", "ore", 4);
        Check(R.State == Status::Ready && R.Cost == 4 && R.Revision == 7, "weighted path");
        Check(Names(G, R) == std::vector<std::string>({"hq", "road", "ore"}), "path reconstruction");
        R = Find(G, "HQ", "ORE", 3);
        Check(R.State == Status::NeedsMoreMoves && R.Missing == 1, "movement budget");
        Check(Find(G, "HQ", "HQ", 0).Cost == 0, "no-op cost");
        Check(Find(G, "HQ", "HQ", 0).NodeIndices.size() == 1, "no-op path");
        Check(Find(G, "HQ", "ORE", -1).State == Status::InvalidInput, "negative budget");
        Check(Find(G, "ORE", "HQ", 100).State == Status::Unreachable, "directed edges");
        G.Nodes[1].Guard = true;
        Check(Find(G, "HQ", "ORE", 100).Cost == 9, "guard requires bypass");
        R = Find(G, "HQ", "ROAD", 2);
        Check(R.State == Status::Ready && R.Battle, "guard is legal destination only");
        G.Links.erase(G.Links.begin());
        Check(Find(G, "HQ", "ORE", 100).State == Status::Unreachable, "cannot cross guard");
        G.Nodes[1].Guard = false;
        for (int Kind = 0; Kind < 3; ++Kind)
        {
            auto B = G;
            if (Kind == 0) B.Nodes[1].Discovered = false;
            if (Kind == 1) B.Nodes[1].Passable = false;
            if (Kind == 2) B.Links[1].Enabled = false;
            R = Find(B, "HQ", "ORE", 100);
            Check(R.State == Status::Unreachable && R.NodeIndices.empty() && R.Cost == 0,
                "hidden, blocked or closed corridor");
        }
        for (int Kind = 0; Kind < 13; ++Kind)
        {
            auto B = Sample();
            switch (Kind)
            {
                case 0: B.Nodes.clear(); break;
                case 1: B.Nodes[1].Id = "hQ"; break;
                case 2: B.Nodes[1].Id.clear(); break;
                case 3: B.Links[0].To = "UNKNOWN"; break;
                case 4: B.Links[0].Cost = 0; break;
                case 5: B.Links[0].Cost = -1; break;
                case 6: B.Links[0].Cost = std::numeric_limits<std::int32_t>::max(); break;
                case 7: B.Links[0].To = B.Links[0].From; break;
                case 8: { const auto Copy = B.Links[0]; B.Links.push_back(Copy); break; }
                case 9: B.Nodes.resize(MaxNodes + 1); break;
                case 10: B.Links.resize(MaxLinks + 1); break;
                case 11: B.Nodes[1].Id = "bad id"; break;
                case 12: B.Nodes[1].Id = "None"; break;
            }
            R = Find(B, "HQ", "ORE", 100);
            Check(R.State == Status::InvalidInput && R.NodeIndices.empty(), "invalid input fails closed");
        }
        auto Tie = Sample();
        Tie.Nodes.push_back({"BRIDGE", true, true, false});
        Tie.Links.push_back({"HQ", "BRIDGE", 2, true});
        Tie.Links.push_back({"BRIDGE", "ORE", 2, true});
        const auto ExpectedNames = Names(Tie, Find(Tie, "HQ", "ORE", 4));
        Check(ExpectedNames[1] == "bridge", "lexical tie");
        std::reverse(Tie.Nodes.begin(), Tie.Nodes.end());
        std::reverse(Tie.Links.begin(), Tie.Links.end());
        Check(Names(Tie, Find(Tie, "HQ", "ORE", 4)) == ExpectedNames, "permutation invariant");
        Graph Long;
        for (std::size_t I = 0; I < MaxNodes; ++I)
        {
            Long.Nodes.push_back({"N" + std::to_string(I), true, true, false});
            if (I > 0) Long.Links.push_back({Long.Nodes[I-1].Id, Long.Nodes[I].Id, MaxLinkCost, true});
        }
        R = Find(Long, "N0", Long.Nodes.back().Id, std::numeric_limits<std::int32_t>::max());
        Check(R.State == Status::Ready && R.Cost == 127000000 && R.NodeIndices.size() == 128,
            "maximum graph path has no overflow");
        const auto Old = Find(G, "HQ", "ORE", 4);
        ++G.Revision; G.Nodes[1].Guard = true;
        Check(Old.Revision != G.Revision && Find(G, "HQ", "ORE", 4).State == Status::Unreachable,
            "revision and changed guards");

        // Independent oracle: Floyd-Warshall, not another Dijkstra implementation.
        // Generated examples exercise visibility, cycles, guards, costs and budgets.
        std::mt19937 Random(20261028);
        constexpr int N = 8, Inf = 1000000;
        for (int Scenario = 0; Scenario < 200; ++Scenario)
        {
            Graph F;
            F.Revision = static_cast<std::uint32_t>(Scenario);
            int Distance[N][N];
            for (int I = 0; I < N; ++I)
            {
                F.Nodes.push_back({"N" + std::to_string(I), I == 0 || Random()%5 != 0,
                    I == 0 || Random()%7 != 0, I != 0 && Random()%4 == 0});
                for (int J = 0; J < N; ++J) Distance[I][J] = I == J ? 0 : Inf;
            }
            for (int I = 0; I < N; ++I) for (int J = 0; J < N; ++J)
            {
                if (I == J || Random()%3 != 0) continue;
                const int Cost = static_cast<int>(1 + Random()%9);
                const bool Enabled = Random()%5 != 0;
                F.Links.push_back({F.Nodes[I].Id, F.Nodes[J].Id, Cost, Enabled});
                if (Enabled && F.Nodes[I].Discovered && F.Nodes[I].Passable &&
                    F.Nodes[J].Discovered && F.Nodes[J].Passable) Distance[I][J] = Cost;
            }
            for (int K = 0; K < N; ++K)
            {
                if (F.Nodes[K].Guard || !F.Nodes[K].Discovered || !F.Nodes[K].Passable) continue;
                for (int I = 0; I < N; ++I) for (int J = 0; J < N; ++J)
                    Distance[I][J] = std::min(Distance[I][J], Distance[I][K] + Distance[K][J]);
            }
            for (int Goal = 0; Goal < N; ++Goal)
            {
                const int Budget = static_cast<int>(Random()%16);
                const auto A = Find(F, "N0", F.Nodes[Goal].Id, Budget);
                const bool Reachable = F.Nodes[Goal].Discovered && F.Nodes[Goal].Passable && Distance[0][Goal] < Inf;
                if (!Reachable)
                {
                    Check(A.State == Status::Unreachable && A.NodeIndices.empty(), "oracle unreachable");
                    continue;
                }
                Check(A.Cost == Distance[0][Goal], "oracle shortest cost");
                Check(A.State == (A.Cost <= Budget ? Status::Ready : Status::NeedsMoreMoves), "oracle budget");
                Check(A.Missing == std::max(0, A.Cost-Budget), "oracle missing moves");
                Check(A.Battle == F.Nodes[Goal].Guard, "oracle combat boundary");
                int Sum = 0;
                for (std::size_t I = 1; I < A.NodeIndices.size(); ++I)
                {
                    const auto Prev = A.NodeIndices[I-1], Next = A.NodeIndices[I];
                    Check(!F.Nodes[Prev].Guard, "no intermediate guard");
                    bool Found = false;
                    for (const auto& L : F.Links)
                        if (L.From == F.Nodes[Prev].Id && L.To == F.Nodes[Next].Id && L.Enabled)
                        { Sum += L.Cost; Found = true; break; }
                    Check(Found, "each route edge exists and is enabled");
                }
                Check(Sum == A.Cost, "path receipt cost agrees");
            }
        }
        std::cout << "ADVENTURE_NATIVE_PASS checks=" << Checks
                  << " generated_graphs=200 generated_queries=1600 seed=20261028\n";
        return 0;
    }
    catch (const std::exception& Error)
    {
        std::cerr << "ADVENTURE_NATIVE_FAIL after_checks=" << Checks << " reason=" << Error.what() << '\n';
        return 1;
    }
}
