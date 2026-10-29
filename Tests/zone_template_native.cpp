#include "AIHeroesZoneTemplateCore.h"

#include <iostream>
#include <stdexcept>
#include <string>

using namespace AIHeroesZoneTemplateCore;

static int Checks = 0;
static void Check(bool Condition, const char* Message)
{
    ++Checks;
    if (!Condition) throw std::runtime_error(Message);
}
static bool CanReach(const AIHeroesAdventureCore::Graph& G, const char* Goal)
{
    return AIHeroesAdventureCore::Find(G, "HQ", Goal, ProofBudget).State ==
        AIHeroesAdventureCore::Status::Ready;
}
int main()
{
    try
    {
        World A, B;
        std::string Error;
        Check(!Generate(11u, MaxBranches + 1u, A, Error) && A.Zones.empty() &&
              !Error.empty(), "invalid branch count must fail closed");
        for (std::uint32_t Seed = 0; Seed < 1024u; ++Seed)
        {
            for (std::uint32_t Branches = 0; Branches <= MaxBranches; ++Branches)
            {
                Check(Generate(Seed, Branches, A, Error), "valid generator");
                Check(Generate(Seed, Branches, B, Error), "repeat generator");
                Check(A.Revision == B.Revision && A.Revision != 0,
                      "same seed and branches must reproduce hash");
                Check(A.Zones.size() == BaseZones + Branches &&
                      A.Roads.size() >= 14u + 4u * Branches,
                      "bounded expected layout and roads");
                Check(Validate(A, Error), "semantic validator");
                const auto G = AsAdventure(A);
                Check(CanReach(G, "FOREST") && CanReach(G, "ORE"),
                      "two initially playable guarded choices");
                Check(!CanReach(G, "PASS") &&
                      !CanReach(G, "OUTPOST") && !CanReach(G, "RUINS"),
                      "guarded gates cannot be silently crossed");
                auto AfterForest = G;
                for (auto& N : AfterForest.Nodes)
                    if (N.Id == "FOREST") N.Guard = false;
                Check(CanReach(AfterForest, "PASS") &&
                      !CanReach(AfterForest, "OUTPOST"), "clear first guard");
                for (auto& N : AfterForest.Nodes)
                    if (N.Id == "PASS") N.Guard = false;
                Check(CanReach(AfterForest, "OUTPOST") &&
                      CanReach(AfterForest, "RUINS"), "late goals after checkpoint");
                for (auto& N : AfterForest.Nodes) N.Guard = false;
                bool AllReachable = true;
                for (const auto& N : AfterForest.Nodes)
                    AllReachable = AllReachable && CanReach(AfterForest, N.Id.c_str());
                Check(AllReachable, "every extra side route is reachable");
            }
        }
        Check(Generate(200u, 4u, A, Error), "can create max branch world");
        Check(Generate(201u, 4u, B, Error) &&
              A.Revision != B.Revision, "different seeds vary the layout");

        const auto Malformed = [&](World W, const char* Label)
        {
            W.Revision = Hash(W); // Recomputed hash should not bypass topology checks.
            std::string Why;
            Check(!Validate(W, Why) && !Why.empty(), Label);
        };
        {
            World Bad = A; Bad.Version = 2; Malformed(Bad, "wrong schema");
            Bad = A; Bad.Zones[0].Id = "BAD"; Malformed(Bad, "wrong HQ");
            Bad = A; Bad.Zones[1].Id = "HQ"; Malformed(Bad, "duplicate HQ");
            Bad = A; Bad.Zones[2].Guarded = false; Malformed(Bad, "missing forest guard");
            Bad = A; Bad.Zones[4].Guarded = false; Malformed(Bad, "missing pass guard");
            Bad = A; Bad.Zones[5].Type = Role::SideRoute; Malformed(Bad, "wrong objective role");
            Bad = A; Bad.Zones[3].Danger = 123; Malformed(Bad, "fake danger tier");
            Bad = A; Bad.Zones[3].X = Bad.Zones[2].X;
            Bad.Zones[3].Y = Bad.Zones[2].Y; Malformed(Bad, "duplicate coordinate");
            Bad = A; Bad.Zones[3].X = 900; Malformed(Bad, "out-of-bounds coordinate");
            Bad = A; Bad.Roads[0].Cost = 0; Malformed(Bad, "zero cost");
            Bad = A; Bad.Roads[0].Cost = 5; Malformed(Bad, "excessive cost");
            Bad = A; Bad.Roads[0].To = "RUINS"; Malformed(Bad, "fake shortcut");
            Bad = A; Bad.Roads[1].Cost = 3 + Bad.Roads[0].Cost;
            if (Bad.Roads[1].Cost > 4) Bad.Roads[1].Cost = 1;
            if (Bad.Roads[1].Cost == Bad.Roads[0].Cost)
                Bad.Roads[1].Cost = Bad.Roads[0].Cost == 1 ? 2 : 1;
            Malformed(Bad, "asymmetric road weights");
            Bad = A; Bad.Roads.pop_back(); Malformed(Bad, "missing reciprocal");
            Bad = A; Bad.Roads.push_back(Bad.Roads[0]); Malformed(Bad, "duplicate edge");
            Bad = A; Bad.Roads.push_back({"HQ","RUINS",1}); Malformed(Bad, "guard bypass");
            Bad = A; Bad.Zones.pop_back(); Malformed(Bad, "missing side zone");
            Bad = A; Bad.Zones.push_back(Bad.Zones[0]); Malformed(Bad, "extra fake HQ");
        }
        Check(!Validate(World(), Error), "empty candidate rejected");
        const auto Initial = AsAdventure(A);
        auto Hidden = Initial;
        for (auto& N : Hidden.Nodes) if (N.Id == "FOREST") N.Discovered = false;
        const auto HiddenResult =
            AIHeroesAdventureCore::Find(Hidden, "HQ", "FOREST", ProofBudget);
        Check(HiddenResult.State == AIHeroesAdventureCore::Status::Unreachable &&
              HiddenResult.NodeIndices.empty() && HiddenResult.Cost == 0,
              "fog-of-war still hides undiscovered destinations");

        std::cout << "ZONE_TEMPLATE_NATIVE_PASS seeds=1024 variants=5 worlds=5120 checks="
                  << Checks << " schema=1" << '\n';
        return 0;
    }
    catch (const std::exception& Ex)
    {
        std::cerr << "ZONE_TEMPLATE_NATIVE_FAIL checks=" << Checks
                  << " reason=" << Ex.what() << '\n';
        return 1;
    }
}
