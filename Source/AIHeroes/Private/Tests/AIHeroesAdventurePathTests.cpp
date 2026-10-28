#include "AIHeroesAdventurePath.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

namespace
{
FAIHeroesAdventureGraph TravelFixture()
{
    FAIHeroesAdventureGraph G;
    G.Revision = 7;
    G.Nodes = {{TEXT("HQ"), true, true, false}, {TEXT("ROAD"), true, true, false},
               {TEXT("ORE"), true, true, false}};
    G.Links = {{TEXT("HQ"), TEXT("ORE"), 9, true},
               {TEXT("HQ"), TEXT("ROAD"), 2, true},
               {TEXT("ROAD"), TEXT("ORE"), 2, true}};
    return G;
}
FString GraphEvidence(const FAIHeroesAdventureGraph& G)
{
    FString S = FString::Printf(TEXT("revision=%u;"), G.Revision);
    for (const auto& N : G.Nodes)
        S += FString::Printf(TEXT("%s/%d/%d/%d;"), *N.Id.ToString(),
            N.bDiscovered, N.bPassable, N.bHostileGuard);
    for (const auto& L : G.Links)
        S += FString::Printf(TEXT("%s/%s/%d/%d;"), *L.From.ToString(),
            *L.To.ToString(), L.MoveCost, L.bEnabled);
    return S;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHTravelWeightedTest,
    "AIHeroes.AdventurePath.WeightedDetour", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHTravelWeightedTest::RunTest(const FString& Parameters)
{
    const auto G = TravelFixture();
    const auto R = FAIHeroesAdventurePath::Find(G, TEXT("HQ"), TEXT("ORE"), 4);
    TestTrue(TEXT("Affordable road route is ready"), R.IsReachableThisTurn());
    TestEqual(TEXT("Cheaper route can have more links"), R.TotalCost, 4);
    TestEqual(TEXT("Route includes origin and destination"), R.Nodes.Num(), 3);
    if (R.Nodes.Num() == 3) TestEqual(TEXT("Uses road rather than expensive direct edge"), R.Nodes[1], FName(TEXT("ROAD")));
    TestEqual(TEXT("Revision is evidence, not an execution token"), R.SourceRevision, uint32(7));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHTravelGuardsTest,
    "AIHeroes.AdventurePath.GuardsAreStops", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHTravelGuardsTest::RunTest(const FString& Parameters)
{
    auto G = TravelFixture();
    G.Links.RemoveAt(0); // Only path is HQ -> ROAD -> ORE.
    G.Nodes[1].bHostileGuard = true;
    const auto Beyond = FAIHeroesAdventurePath::Find(G, TEXT("HQ"), TEXT("ORE"), 100);
    TestTrue(TEXT("Guard cannot be silently crossed"), Beyond.Status == EAIHeroesAdventurePathStatus::Unreachable);
    TestTrue(TEXT("Failure contains no partial move"), Beyond.Nodes.IsEmpty());
    const auto AtGuard = FAIHeroesAdventurePath::Find(G, TEXT("HQ"), TEXT("ROAD"), 2);
    TestTrue(TEXT("Guard itself can be proposed as destination"), AtGuard.IsReachableThisTurn());
    TestTrue(TEXT("Combat requirement stays explicit"), AtGuard.bBattleAtDestination);
    TestTrue(TEXT("No victory is promised"), AtGuard.Reason.Contains(TEXT("outcome not predicted")));
    G.Links.Add({TEXT("HQ"), TEXT("ORE"), 9, true});
    TestEqual(TEXT("Legal bypass is used rather than crossing guard"),
        FAIHeroesAdventurePath::Find(G, TEXT("HQ"), TEXT("ORE"), 10).TotalCost, 9);
    G.Nodes[0].bHostileGuard = true;
    TestTrue(TEXT("Impossible hostile origin is rejected"),
        FAIHeroesAdventurePath::Find(G, TEXT("HQ"), TEXT("ORE"), 10).Status == EAIHeroesAdventurePathStatus::InvalidInput);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHTravelKnowledgeTest,
    "AIHeroes.AdventurePath.VisibilityPassabilityAndDirection", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHTravelKnowledgeTest::RunTest(const FString& Parameters)
{
    auto G = TravelFixture();
    G.Links.RemoveAt(0);
    G.Nodes[1].bDiscovered = false;
    TestTrue(TEXT("Undiscovered corridor cannot guide the advisor"),
        FAIHeroesAdventurePath::Find(G, TEXT("HQ"), TEXT("ORE"), 100).Status == EAIHeroesAdventurePathStatus::Unreachable);
    const auto Hidden = FAIHeroesAdventurePath::Find(G, TEXT("HQ"), TEXT("ROAD"), 100);
    TestTrue(TEXT("Hidden destination discloses no route or cost"), Hidden.Nodes.IsEmpty() && Hidden.TotalCost == 0);
    G.Nodes[1].bDiscovered = true;
    G.Nodes[1].bPassable = false;
    TestTrue(TEXT("Blocked ground has no route"), !FAIHeroesAdventurePath::Find(G, TEXT("HQ"), TEXT("ORE"), 100).IsReachableThisTurn());
    G.Nodes[1].bPassable = true;
    G.Links[1].bEnabled = false;
    TestTrue(TEXT("Closed crossing has no route"), !FAIHeroesAdventurePath::Find(G, TEXT("HQ"), TEXT("ORE"), 100).IsReachableThisTurn());
    G.Links[1].bEnabled = true;
    TestTrue(TEXT("Open known corridor works"), FAIHeroesAdventurePath::Find(G, TEXT("HQ"), TEXT("ORE"), 4).IsReachableThisTurn());
    TestTrue(TEXT("One-way links do not invent a return path"),
        FAIHeroesAdventurePath::Find(G, TEXT("ORE"), TEXT("HQ"), 100).Status == EAIHeroesAdventurePathStatus::Unreachable);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHTravelBudgetTest,
    "AIHeroes.AdventurePath.BudgetAndNoOp", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHTravelBudgetTest::RunTest(const FString& Parameters)
{
    const auto G = TravelFixture();
    const auto R = FAIHeroesAdventurePath::Find(G, TEXT("HQ"), TEXT("ORE"), 3);
    TestTrue(TEXT("A route is not an affordable move"), R.Status == EAIHeroesAdventurePathStatus::NeedsMoreMoves);
    TestEqual(TEXT("Missing movement is exact"), R.MissingMoves, 1);
    TestEqual(TEXT("Advisory future route is retained"), R.Nodes.Num(), 3);
    const auto Stay = FAIHeroesAdventurePath::Find(G, TEXT("HQ"), TEXT("HQ"), 0);
    TestTrue(TEXT("Staying is valid with no movement"), Stay.IsReachableThisTurn());
    TestEqual(TEXT("Staying costs zero"), Stay.TotalCost, 0);
    TestEqual(TEXT("Staying is a single-node route"), Stay.Nodes.Num(), 1);
    TestTrue(TEXT("Negative budget is invalid"),
        FAIHeroesAdventurePath::Find(G, TEXT("HQ"), TEXT("ORE"), -1).Status == EAIHeroesAdventurePathStatus::InvalidInput);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHTravelStableTest,
    "AIHeroes.AdventurePath.StableTiesAndNoMutation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHTravelStableTest::RunTest(const FString& Parameters)
{
    auto G = TravelFixture();
    G.Nodes.Add({TEXT("BRIDGE"), true, true, false});
    G.Links.Add({TEXT("HQ"), TEXT("BRIDGE"), 2, true});
    G.Links.Add({TEXT("BRIDGE"), TEXT("ORE"), 2, true});
    const FString Before = GraphEvidence(G);
    const auto A = FAIHeroesAdventurePath::Find(G, TEXT("HQ"), TEXT("ORE"), 4);
    TestEqual(TEXT("Every input field unchanged"), GraphEvidence(G), Before);
    G.Nodes.Swap(0, 3); G.Nodes.Swap(1, 2);
    G.Links.Swap(0, 4); G.Links.Swap(1, 3);
    const auto B = FAIHeroesAdventurePath::Find(G, TEXT("HQ"), TEXT("ORE"), 4);
    TestTrue(TEXT("Equal cost ties independent of input order"), A.Nodes == B.Nodes);
    if (A.Nodes.Num() == 3) TestEqual(TEXT("Stable lexical settlement picks BRIDGE"), A.Nodes[1], FName(TEXT("BRIDGE")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHTravelValidationTest,
    "AIHeroes.AdventurePath.InvalidGraphFailsClosed", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHTravelValidationTest::RunTest(const FString& Parameters)
{
    for (int32 Case = 0; Case < 10; ++Case)
    {
        auto G = TravelFixture();
        switch (Case)
        {
            case 0: G.Nodes.Reset(); break;
            case 1: G.Nodes[1].Id = G.Nodes[0].Id; break;
            case 2: G.Nodes[1].Id = NAME_None; break;
            case 3: G.Links[0].To = TEXT("MISSING"); break;
            case 4: G.Links[0].MoveCost = 0; break;
            case 5: G.Links[0].MoveCost = -3; break;
            case 6: G.Links[0].MoveCost = MAX_int32; break;
            case 7: G.Links[0].To = G.Links[0].From; break;
            case 8: { const auto Copy = G.Links[0]; G.Links.Add(Copy); break; }
            case 9: G.Nodes.SetNum(FAIHeroesAdventurePath::MaxNodes + 1); break;
        }
        const auto R = FAIHeroesAdventurePath::Find(G, TEXT("HQ"), TEXT("ORE"), 100);
        TestTrue(*FString::Printf(TEXT("Malformed case %d is rejected without a path"), Case),
            R.Status == EAIHeroesAdventurePathStatus::InvalidInput && R.Nodes.IsEmpty());
    }
    auto G = TravelFixture();
    G.Links.SetNum(FAIHeroesAdventurePath::MaxLinks + 1);
    TestTrue(TEXT("Excessive graph work is bounded"),
        FAIHeroesAdventurePath::Find(G, TEXT("HQ"), TEXT("ORE"), 100).Status == EAIHeroesAdventurePathStatus::InvalidInput);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHTravelBoundTest,
    "AIHeroes.AdventurePath.BoundedLongRoute", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHTravelBoundTest::RunTest(const FString& Parameters)
{
    FAIHeroesAdventureGraph G;
    for (int32 I = 0; I < FAIHeroesAdventurePath::MaxNodes; ++I)
    {
        const FName Id(*FString::Printf(TEXT("N%03d"), I));
        G.Nodes.Add({Id, true, true, false});
        if (I > 0) G.Links.Add({G.Nodes[I - 1].Id, Id, FAIHeroesAdventurePath::MaxLinkCost, true});
    }
    const auto R = FAIHeroesAdventurePath::Find(G, G.Nodes[0].Id, G.Nodes.Last().Id, MAX_int32);
    TestTrue(TEXT("Maximum supported chain terminates"), R.IsReachableThisTurn());
    TestEqual(TEXT("All nodes retained"), R.Nodes.Num(), FAIHeroesAdventurePath::MaxNodes);
    TestEqual(TEXT("No integer overflow in long path"), R.TotalCost,
        (FAIHeroesAdventurePath::MaxNodes - 1) * FAIHeroesAdventurePath::MaxLinkCost);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHTravelRevisionTest,
    "AIHeroes.AdventurePath.RevalidationAfterWorldChange", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHTravelRevisionTest::RunTest(const FString& Parameters)
{
    auto G = TravelFixture();
    G.Links.RemoveAt(0);
    const auto Old = FAIHeroesAdventurePath::Find(G, TEXT("HQ"), TEXT("ORE"), 4);
    ++G.Revision;
    G.Nodes[1].bHostileGuard = true;
    const auto Now = FAIHeroesAdventurePath::Find(G, TEXT("HQ"), TEXT("ORE"), 4);
    TestTrue(TEXT("Stale recommendation is visibly bound to an old revision"), Old.SourceRevision != G.Revision);
    TestTrue(TEXT("A fresh query cannot reuse an invalid route"), Now.Status == EAIHeroesAdventurePathStatus::Unreachable);
    TestEqual(TEXT("Query does not clear guard or modify revision"), G.Revision, uint32(8));
    TestTrue(TEXT("Guard remains"), G.Nodes[1].bHostileGuard);
    return true;
}
#endif
