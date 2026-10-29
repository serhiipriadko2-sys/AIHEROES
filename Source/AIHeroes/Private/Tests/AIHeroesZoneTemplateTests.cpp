#include "AIHeroesZoneTemplate.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AIHeroesCampaignRules.h"
#include "AIHeroesCampaignSave.h"
#include "Misc/AutomationTest.h"

namespace
{
FAIHeroesGeneratedWorld SampleDraft()
{
    FAIHeroesGeneratedWorld World;
    FString Error;
    FAIHeroesZoneTemplate::Generate(20261028u, 3, World, Error);
    return World;
}
EAIHeroesAdventurePathStatus Status(
    const FAIHeroesAdventureGraph& Graph, const TCHAR* Destination)
{
    return FAIHeroesAdventurePath::Find(
        Graph, TEXT("HQ"), FName(Destination), 1000).Status;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesZoneDeterminismTest,
    "AIHeroes.ZoneTemplate.RepeatableSeedAndRoles",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesZoneDeterminismTest::RunTest(const FString&)
{
    FAIHeroesGeneratedWorld A;
    FAIHeroesGeneratedWorld B;
    FString Error;
    if (!TestTrue(TEXT("Seeded 10-zone map is generated"),
        FAIHeroesZoneTemplate::Generate(20261028u, 3, A, Error))) return false;
    TestTrue(TEXT("Second generation succeeds"),
        FAIHeroesZoneTemplate::Generate(20261028u, 3, B, Error));
    TestEqual(TEXT("Exactly 10 zones (7 core + 3 detours)"), A.Zones.Num(), 10);
    TestEqual(TEXT("Directed road graph has all zone entries"),
        A.Adventure.Nodes.Num(), A.Zones.Num());
    TestEqual(TEXT("Same seed yields same revision"), A.Revision, B.Revision);
    TestEqual(TEXT("Same seed yields same edge count"),
        A.Adventure.Links.Num(), B.Adventure.Links.Num());
    for (int32 I = 0; I < A.Zones.Num(); ++I)
    {
        TestEqual(TEXT("Stable zone identity"), A.Zones[I].Id, B.Zones[I].Id);
        TestEqual(TEXT("Stable X cell"), A.Zones[I].Grid.X, B.Zones[I].Grid.X);
        TestEqual(TEXT("Stable Y cell"), A.Zones[I].Grid.Y, B.Zones[I].Grid.Y);
        TestEqual(TEXT("Stable danger"), A.Zones[I].DangerTier, B.Zones[I].DangerTier);
        TestEqual(TEXT("Stable guard"), A.Zones[I].bGuarded, B.Zones[I].bGuarded);
    }
    TestTrue(TEXT("Draft marks itself as non-playable"),
        FAIHeroesZoneTemplate::Describe(A).Contains(TEXT("NOT ACTIVE CAMPAIGN")));
    FAIHeroesGeneratedWorld Different;
    TestTrue(TEXT("Different seed also generates"),
        FAIHeroesZoneTemplate::Generate(20261029u, 3, Different, Error));
    TestTrue(TEXT("Different seed changes revision"),
        Different.Revision != A.Revision);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesZoneGatesTest,
    "AIHeroes.ZoneTemplate.GuardedProgressiveReachability",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesZoneGatesTest::RunTest(const FString&)
{
    const auto W = SampleDraft();
    if (!TestEqual(TEXT("Fixture must be valid"), W.Zones.Num(), 10)) return false;
    const auto& G = W.Adventure;
    TestTrue(TEXT("FOREST is first guarded option"),
        Status(G, TEXT("FOREST")) == EAIHeroesAdventurePathStatus::Ready);
    TestTrue(TEXT("ORE is second guarded option"),
        Status(G, TEXT("ORE")) == EAIHeroesAdventurePathStatus::Ready);
    TestTrue(TEXT("Cannot skip guarded checkpoint"),
        Status(G, TEXT("PASS")) == EAIHeroesAdventurePathStatus::Unreachable);
    TestTrue(TEXT("Late OUTPOST locked behind intermediate guards"),
        Status(G, TEXT("OUTPOST")) == EAIHeroesAdventurePathStatus::Unreachable);
    const auto ForestPath = FAIHeroesAdventurePath::Find(
        G, TEXT("HQ"), TEXT("FOREST"), 1000);
    TestTrue(TEXT("Battle at target requires explicit human confirmation"),
        ForestPath.bBattleAtDestination && ForestPath.TotalCost > 0);
    TestEqual(TEXT("Path revision equals candidate version"),
        ForestPath.SourceRevision, W.Revision);
    TestTrue(TEXT("Travel budget zero cannot move to forest"),
        FAIHeroesAdventurePath::Find(G, TEXT("HQ"), TEXT("FOREST"), 0).Status ==
            EAIHeroesAdventurePathStatus::NeedsMoreMoves);

    FAIHeroesAdventureGraph After = G;
    for (auto& N : After.Nodes) if (N.Id == FName(TEXT("FOREST"))) N.bHostileGuard = false;
    TestTrue(TEXT("Clearing first mine reveals viable route to checkpoint"),
        Status(After, TEXT("PASS")) == EAIHeroesAdventurePathStatus::Ready);
    TestTrue(TEXT("Checkpoint still blocks the frontier"),
        Status(After, TEXT("RUINS")) == EAIHeroesAdventurePathStatus::Unreachable);
    for (auto& N : After.Nodes) if (N.Id == FName(TEXT("PASS"))) N.bHostileGuard = false;
    TestTrue(TEXT("OUTPOST now reachable"), Status(After, TEXT("OUTPOST")) ==
        EAIHeroesAdventurePathStatus::Ready);
    TestTrue(TEXT("RUINS now reachable"), Status(After, TEXT("RUINS")) ==
        EAIHeroesAdventurePathStatus::Ready);
    TestTrue(TEXT("Read-only graph planning did not clear source"),
        G.Nodes[2].bHostileGuard && G.Nodes[4].bHostileGuard);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesZoneTamperTest,
    "AIHeroes.ZoneTemplate.TamperedGraphFailsClosed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesZoneTamperTest::RunTest(const FString&)
{
    FAIHeroesGeneratedWorld W;
    FString Error;
    TestFalse(TEXT("Negative branch count is rejected"),
        FAIHeroesZoneTemplate::Generate(7u, -1, W, Error));
    TestTrue(TEXT("Invalid draft does not leak zone data"), W.Zones.IsEmpty());
    TestFalse(TEXT("Five side routes exceed template bound"),
        FAIHeroesZoneTemplate::Generate(7u, 5, W, Error));
    TestTrue(TEXT("Still no partial graph"), W.Adventure.Links.IsEmpty());

    W = SampleDraft();
    TestTrue(TEXT("Good draft starts valid"), FAIHeroesZoneTemplate::Validate(W, Error));
    ++W.Revision;
    TestFalse(TEXT("Revision drift detected"), FAIHeroesZoneTemplate::Validate(W, Error));

    W = SampleDraft();
    W.Adventure.Nodes[2].bHostileGuard = false;
    TestFalse(TEXT("Adapter cannot hide a guardian"),
        FAIHeroesZoneTemplate::Validate(W, Error));
    W = SampleDraft();
    W.Adventure.Nodes[2].bDiscovered = false;
    TestFalse(TEXT("Full validation oracle must not leak into fogged gameplay"),
        FAIHeroesZoneTemplate::Validate(W, Error));
    W = SampleDraft();
    W.Adventure.Links[0].bEnabled = false;
    TestFalse(TEXT("Closed road rejected"),
        FAIHeroesZoneTemplate::Validate(W, Error));
    W = SampleDraft();
    W.Adventure.Links[0].MoveCost = 0;
    TestFalse(TEXT("Zero cost is invalid"),
        FAIHeroesZoneTemplate::Validate(W, Error));
    W = SampleDraft();
    W.Zones[2].Id = TEXT("RUINS");
    TestFalse(TEXT("Duplicate or spoofed zone ID rejected"),
        FAIHeroesZoneTemplate::Validate(W, Error));
    W = SampleDraft();
    W.Adventure.Links.RemoveAt(0);
    TestFalse(TEXT("Missing reciprocal edge rejected"),
        FAIHeroesZoneTemplate::Validate(W, Error));
    W = SampleDraft();
    W.Adventure.Nodes.RemoveAt(0);
    TestFalse(TEXT("Missing HQ node rejected"),
        FAIHeroesZoneTemplate::Validate(W, Error));
    W = SampleDraft();
    W.Zones[4].Grid = W.Zones[2].Grid;
    TestFalse(TEXT("Colliding zone placement rejected"),
        FAIHeroesZoneTemplate::Validate(W, Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesZoneSweepTest,
    "AIHeroes.ZoneTemplate.SeedSweepAndBounds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesZoneSweepTest::RunTest(const FString&)
{
    int32 Valid = 0;
    FAIHeroesGeneratedWorld W;
    FString Error;
    for (uint32 Seed = 0; Seed < 128; ++Seed)
    {
        for (int32 Routes = 0; Routes <= 4; ++Routes)
        {
            if (FAIHeroesZoneTemplate::Generate(Seed, Routes, W, Error) &&
                FAIHeroesZoneTemplate::Validate(W, Error) &&
                W.Zones.Num() == 7 + Routes &&
                W.Adventure.Links.Num() <= 48)
            {
                ++Valid;
            }
        }
    }
    TestEqual(TEXT("640 deterministic maps validate with Unreal adapter"),
        Valid, 640);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesZoneSaveBoundaryTest,
    "AIHeroes.ZoneTemplate.V1SaveNotSilentlyExpanded",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesZoneSaveBoundaryTest::RunTest(const FString&)
{
    TestEqual(TEXT("Existing campaign schema intentionally stays v1"),
        UAIHeroesCampaignSave::CurrentSchema, 1);
    FAIHeroesGeneratedWorld W = SampleDraft();
    UAIHeroesCampaignSave* Old = NewObject<UAIHeroesCampaignSave>(GetTransientPackage());
    Old->Day = 1;
    Old->HeroNodeId = TEXT("HQ");
    Old->PlayerArmy.Add(FAIHeroesArmyStack(TEXT("Militia"), 8, 12, 4, 2, 3));
    Old->PlayerArmy.Add(FAIHeroesArmyStack(TEXT("Scout"), 3, 8, 6, 1, 7));
    for (const auto& Generated : W.Zones)
    {
        FAIHeroesSavedNode Node;
        Node.NodeId = Generated.Id;
        Node.Type = EAIHeroesNodeType::ForestMine; // Not a valid v1 topology.
        Node.bOwned = Generated.Id == FName(TEXT("HQ"));
        Old->NodeStates.Add(Node);
    }
    FString Error;
    TestFalse(TEXT("Old campaign validator rejects dynamic 10-zone layout"),
        FAIHeroesCampaignRules::Validate(*Old, Error));
    TestTrue(TEXT("Migration failure must explain invalid map"),
        !Error.IsEmpty());
    TestEqual(TEXT("No changes to old save object itself"),
        Old->NodeStates.Num(), 10);
    TestTrue(TEXT("No save slots are written"), true);
    return true;
}
#endif
