#include "AIHeroesProceduralCampaignCore.h"

#include <iostream>
#include <stdexcept>
#include <string>

namespace Core = AIHeroesProceduralCampaignCore;
namespace World = AIHeroesZoneTemplateCore;
namespace Path = AIHeroesAdventureCore;

static int Checks = 0;

static void Check(bool Condition, const char* Reason)
{
    ++Checks;
    if (!Condition) throw std::runtime_error(Reason);
}

static bool Rejected(const Core::State& S)
{
    std::string Error;
    return !Core::Validate(S, Error) && !Error.empty();
}

static const Core::ZoneState* At(const Core::State& S, const char* Id)
{
    for (const auto& Z : S.Zones)
        if (Z.Id == Id) return &Z;
    return nullptr;
}

static Core::ZoneState* Edit(Core::State& S, const char* Id)
{
    for (auto& Z : S.Zones)
        if (Z.Id == Id) return &Z;
    return nullptr;
}

static bool ContainsNode(const Path::Graph& G, const char* Id)
{
    for (const auto& Z : G.Nodes)
        if (Z.Id == Id) return true;
    return false;
}

int main()
{
    try
    {
        Core::State S;
        std::string Error;
        Check(!Core::MakeNew(0, World::MaxBranches + 1u, S, Error) &&
            S.Zones.empty() && !Error.empty(),
            "out-of-range branches reject without partial data");

        for (std::uint32_t Seed = 0; Seed < 1024u; ++Seed)
        {
            for (std::uint32_t Branches = 0;
                 Branches <= World::MaxBranches; ++Branches)
            {
                Check(Core::MakeNew(Seed, Branches, S, Error),
                    "all bounded seeds initialize");
                Check(S.SchemaVersion == 2 &&
                    S.GeneratorSchema == World::Schema &&
                    S.Zones.size() == World::BaseZones + Branches,
                    "v2 schema/zone count");
                Check(Core::Validate(S, Error),
                    "all initialized campaigns pass validation");
                Check(At(S, "HQ") && At(S, "HQ")->Owned &&
                    At(S, "FOREST") && At(S, "FOREST")->Discovered &&
                    !At(S, "FOREST")->GuardCleared,
                    "initial HQ and guarded starting choice");
                Path::Graph G;
                Check(Core::BuildPlayerGraph(S, G, Error),
                    "every start projects a valid player graph");
                Check(G.Nodes.size() == 4u && ContainsNode(G, "HQ") &&
                    ContainsNode(G, "FOREST") && ContainsNode(G, "ORE") &&
                    !ContainsNode(G, "RUINS") && !ContainsNode(G, "PASS"),
                    "unknown IDs, objectives and route costs are not projected");
                const auto Forest = Path::Find(G, "HQ", "FOREST", 1000);
                const auto Ore = Path::Find(G, "HQ", "ORE", 1000);
                Check(Forest.State == Path::Status::Ready && Forest.Battle &&
                    Ore.State == Path::Status::Ready && Ore.Battle,
                    "two visible guarded encounters remain possible");
                const auto Hidden = Path::Find(G, "HQ", "RUINS", 1000);
                Check(Hidden.State == Path::Status::InvalidInput &&
                    Hidden.NodeIndices.empty() && Hidden.Cost == 0,
                    "hidden destination cannot leak reachability or cost");
            }
        }

        Check(Core::MakeNew(20261028u, 3u, S, Error),
            "fixed test campaign generated");
        const auto Corrupt = [](Core::State Bad, const char* Message)
        {
            Check(Rejected(Bad), Message);
            Path::Graph Out;
            Out.Nodes.push_back({"SHOULD_NOT_SURVIVE",true,true,false});
            std::string Why;
            Check(!Core::BuildPlayerGraph(Bad, Out, Why) &&
                Out.Nodes.empty() && Out.Links.empty(),
                "rejected snapshot produces an empty player graph");
        };
        {
            Core::State Bad = S;
            Bad.SchemaVersion = 1;
            Corrupt(Bad, "must not treat the old schema as v2");
            Bad = S; Bad.SchemaVersion = 3;
            Corrupt(Bad, "future schema blocked");
            Bad = S; Bad.GeneratorSchema = 2;
            Corrupt(Bad, "generator semantic version drift blocked");
            Bad = S; Bad.Seed++;
            Corrupt(Bad, "different seed with old topology hash blocked");
            Bad = S; Bad.TopologyRevision++;
            Corrupt(Bad, "tampered map revision blocked");
            Bad = S; Bad.Branches++;
            Corrupt(Bad, "wrong branch count blocked");
            Bad = S; Bad.Day = 0;
            Corrupt(Bad, "zero day blocked");
            Bad = S; Bad.Day = Core::MaximumDay + 1;
            Corrupt(Bad, "day overflow blocked");
            Bad = S; Bad.MovesLeft = 5;
            Corrupt(Bad, "extra movement blocked");
            Bad = S; Bad.Gold = -1;
            Corrupt(Bad, "negative gold blocked");
            Bad = S; Bad.Wood = Core::MaximumResource + 1;
            Corrupt(Bad, "resource overflow blocked");
            Bad = S; Bad.ProtocolTier = 4;
            Corrupt(Bad, "forged reasoning tier blocked");
            Bad = S; Bad.ModelSlotTier = 3;
            Corrupt(Bad, "forged model slot blocked");
            Bad = S; Bad.DataFragments = -1;
            Corrupt(Bad, "negative data blocked");
            Bad = S; Bad.SaveSequence = -1;
            Corrupt(Bad, "negative save sequence blocked");
            Bad = S; Bad.ActionRevision = Core::MaximumSequence + 1;
            Corrupt(Bad, "action sequence overflow blocked");
            Bad = S; Bad.Zones[2].Id = "RUINS";
            Corrupt(Bad, "out of order/spoofed zone ID blocked");
            Bad = S; Bad.Zones[3] = Bad.Zones[2];
            Corrupt(Bad, "duplicate zone IDs blocked");
            Bad = S; Bad.Zones.pop_back();
            Corrupt(Bad, "missing world zone blocked");
            Bad = S; Edit(Bad, "HQ")->Owned = false;
            Corrupt(Bad, "cannot lose HQ in v2 contract");
            Bad = S; Edit(Bad, "SUPPLY")->GuardCleared = false;
            Corrupt(Bad, "cannot create guardian on safe supply");
            Bad = S; Edit(Bad, "FOREST")->Discovered = false;
            Corrupt(Bad, "initial forest scouting cannot be concealed");
            Bad = S; Edit(Bad, "FOREST")->Owned = true;
            Corrupt(Bad, "cannot own living guarded forest");
            Bad = S; Edit(Bad, "PASS")->GuardCleared = true;
            Corrupt(Bad, "hidden guard cannot be defeated");
            Bad = S; Bad.HeroZoneId = "RUINS";
            Corrupt(Bad, "cannot teleport to hidden enemy ruins");
            Bad = S; Bad.SelectedZoneId = "PASS";
            Corrupt(Bad, "selection of fogged checkpoint not persisted");
            Bad = S; Bad.SelectedZoneId = "UNKNOWN";
            Corrupt(Bad, "unknown selected site blocked");
            Bad = S; Bad.PlayerArmy[0].Attack = 4000;
            Corrupt(Bad, "forged hero attack blocked");
            Bad = S; Bad.PlayerArmy[0].UnitId = "Wolf";
            Corrupt(Bad, "enemy unit injection blocked");
            Bad = S; Bad.PlayerArmy.push_back(Bad.PlayerArmy[0]);
            Corrupt(Bad, "extra stack blocked");
            Bad = S; Bad.PlayerArmy[1] = Bad.PlayerArmy[0];
            Corrupt(Bad, "duplicate friendly stack blocked");
            Bad = S; Bad.PlayerArmy[0].TotalHealth = -7;
            Corrupt(Bad, "negative friendly HP blocked");
            Bad = S; Bad.PlayerArmy[0].TotalHealth = Core::MaximumUnitHealth + 1;
            Corrupt(Bad, "unbounded friendly HP blocked");
            Bad = S; Bad.ActionRevision = 1;
            Edit(Bad, "OUTPOST")->Discovered = true;
            Edit(Bad, "OUTPOST")->Owned = true;
            Edit(Bad, "OUTPOST")->GuardCleared = true;
            Corrupt(Bad, "late capture cannot bypass uncleared PASS");
        }

        // Positive staged progression: this is a test fixture of state
        // after authoritative battle results, NOT battle authorization.
        S.ActionRevision = 1;
        Edit(S, "FOREST")->GuardCleared = true;
        Edit(S, "FOREST")->Owned = true;
        Edit(S, "PASS")->Discovered = true;
        Check(Core::Validate(S, Error), "first won guard can unlock pass");
        Path::Graph First;
        Check(Core::BuildPlayerGraph(S, First, Error),
            "view after first authoritative encounter");
        const auto Checkpoint = Path::Find(First, "HQ", "PASS", 1000);
        Check(Checkpoint.State == Path::Status::Ready && Checkpoint.Battle,
            "next checkpoint remains guarded, never auto-conquered");

        const std::uint32_t BeforeRevision = First.Revision;
        S.ActionRevision = 2;
        Edit(S, "PASS")->GuardCleared = true;
        Edit(S, "PASS")->Owned = true;
        Edit(S, "OUTPOST")->Discovered = true;
        Edit(S, "RUINS")->Discovered = true;
        Check(Core::Validate(S, Error),
            "cleared checkpoint permits discovered late destinations");
        Path::Graph Second;
        Check(Core::BuildPlayerGraph(S, Second, Error),
            "project complete known frontier");
        Check(Second.Revision != BeforeRevision,
            "state revision invalidates stale path proposals");
        const auto Ruins = Path::Find(Second, "HQ", "RUINS", 1000);
        Check(Ruins.State == Path::Status::Ready && Ruins.Battle,
            "final objective still requires separate player-confirmed battle");
        Check(!ContainsNode(Second, "WAY_03") &&
            !ContainsNode(Second, "WAY_01"),
            "undiscovered optional short-cuts stay hidden");
        Check(Core::Validate(S, Error), "planning did not change saved state");

        std::cout << "PROCEDURAL_V2_NATIVE_PASS seeds=1024 variants=5"
            << " states=5120 checks=" << Checks << " schema=2" << '\n';
        return 0;
    }
    catch (const std::exception& Ex)
    {
        std::cerr << "PROCEDURAL_V2_NATIVE_FAIL checks=" << Checks
            << " reason=" << Ex.what() << '\n';
        return 1;
    }
}
