#include "AIHeroesWorldWeek.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AIHeroesCampaignRules.h"
#include "AIHeroesCampaignSave.h"
#include "Misc/AutomationTest.h"

namespace
{
TArray<FAIHeroesWeekNode> FiveNodes()
{
    TArray<FAIHeroesWeekNode> Nodes;
    const auto Add = [&Nodes](const TCHAR* Id, EAIHeroesNodeType Type,
                             bool bOwned, const TArray<FAIHeroesArmyStack>& Guards)
    {
        FAIHeroesWeekNode Node;
        Node.Id = FName(Id);
        Node.Type = Type;
        Node.bOwned = bOwned;
        Node.Guards = Guards;
        Nodes.Add(MoveTemp(Node));
    };
    Add(TEXT("HQ"), EAIHeroesNodeType::Headquarters, true, {});
    Add(TEXT("FOREST"), EAIHeroesNodeType::ForestMine, false,
        {FAIHeroesArmyStack(TEXT("Wolf"), 4, 9, 3, 1, 8)});
    Add(TEXT("ORE"), EAIHeroesNodeType::OreMine, false,
        {FAIHeroesArmyStack(TEXT("Bandit"), 3, 10, 4, 2, 4)});
    Add(TEXT("OUTPOST"), EAIHeroesNodeType::Outpost, false,
        {FAIHeroesArmyStack(TEXT("Raider"), 6, 11, 5, 2, 5),
         FAIHeroesArmyStack(TEXT("Raider"), 4, 11, 5, 2, 5)});
    Add(TEXT("RUINS"), EAIHeroesNodeType::Ruins, false,
        {FAIHeroesArmyStack(TEXT("Guardian"), 2, 24, 7, 4, 2)});
    return Nodes;
}

bool Simulate(const TArray<FAIHeroesWeekNode>& Nodes,
              int32 Day, FAIHeroesWeekPlan& Out)
{
    FString Error;
    return FAIHeroesWorldWeekRules::PlanNextDay(Day, Nodes, Out, Error);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesWeekCalendarTest, "AIHeroes.WorldWeek.CalendarBoundaries",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesWeekCalendarTest::RunTest(const FString&)
{
    TestEqual(TEXT("Week one begins day 1"),
        FAIHeroesWorldWeekRules::WeekNumber(1), 1);
    TestEqual(TEXT("Day 7 closes week one"),
        FAIHeroesWorldWeekRules::DayInWeek(7), 7);
    TestEqual(TEXT("Day 8 starts week two"),
        FAIHeroesWorldWeekRules::WeekNumber(8), 2);
    TestEqual(TEXT("Day 29 starts week five"),
        FAIHeroesWorldWeekRules::WeekNumber(29), 5);
    TestTrue(TEXT("Only end of week opens new one"),
        FAIHeroesWorldWeekRules::IsWeekTransition(7));
    TestTrue(TEXT("Second week also ticks"),
        FAIHeroesWorldWeekRules::IsWeekTransition(14));
    TestFalse(TEXT("Day six cannot trigger growth"),
        FAIHeroesWorldWeekRules::IsWeekTransition(6));
    TestFalse(TEXT("Week never advances at maximum campaign day"),
        FAIHeroesWorldWeekRules::IsWeekTransition(1000000));
    TestEqual(TEXT("Invalid day does not invent a week"),
        FAIHeroesWorldWeekRules::WeekNumber(0), INDEX_NONE);
    FAIHeroesWeekPlan Output;
    TestFalse(TEXT("Day zero is rejected"), Simulate(FiveNodes(), 0, Output));
    TestFalse(TEXT("Day limit is rejected"),
        Simulate(FiveNodes(), 1000000, Output));
    TestTrue(TEXT("Rejected plan has no partial mutations"), Output.Nodes.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesWeekGrowthTest, "AIHeroes.WorldWeek.GuardGrowthAndNoResurrection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesWeekGrowthTest::RunTest(const FString&)
{
    const TArray<FAIHeroesWeekNode> Initial = FiveNodes();
    FAIHeroesWeekPlan Plan;
    if (!TestTrue(TEXT("Week transition succeeds"),
        Simulate(Initial, 7, Plan))) return false;

    TestEqual(TEXT("New calendar day"), Plan.NewDay, 8);
    TestEqual(TEXT("New week"), Plan.Week, 2);
    TestEqual(TEXT("New week's first day"), Plan.DayInWeek, 1);
    TestTrue(TEXT("Exactly one week transition"), Plan.bNewWeek);
    TestEqual(TEXT("Four hostile sites gain seven neutral units"),
        Plan.NewNeutralUnits, 7);
    TestEqual(TEXT("Forest gains two full Wolves"),
        Plan.Nodes[1].Guards[0].TotalHealth,
        Initial[1].Guards[0].TotalHealth + 2 * 9);
    TestEqual(TEXT("Ore gains two full Bandits"),
        Plan.Nodes[2].Guards[0].TotalHealth,
        Initial[2].Guards[0].TotalHealth + 2 * 10);
    TestEqual(TEXT("Two separate Outpost formations each gain one"),
        Plan.Nodes[3].Guards[0].TotalHealth + Plan.Nodes[3].Guards[1].TotalHealth,
        Initial[3].Guards[0].TotalHealth + Initial[3].Guards[1].TotalHealth + 22);
    TestEqual(TEXT("Ruins Guardians grow one"),
        Plan.Nodes[4].Guards[0].TotalHealth,
        Initial[4].Guards[0].TotalHealth + 24);
    TestEqual(TEXT("HQ never gets a hostile army"), Plan.Nodes[0].Guards.Num(), 0);
    TestEqual(TEXT("Input state remains entirely untouched"),
        Initial[1].Guards[0].Count(), 4);

    FAIHeroesWeekPlan Next;
    TestTrue(TEXT("Ordinary next day after week starts"), Simulate(Plan.Nodes, 8, Next));
    TestFalse(TEXT("No repeat growth on day 9"), Next.bNewWeek);
    TestEqual(TEXT("Day 9 produces no fresh guards"), Next.NewNeutralUnits, 0);
    TestEqual(TEXT("Forest count unchanged between weekly ticks"),
        Next.Nodes[1].Guards[0].Count(), 6);

    TArray<FAIHeroesWeekNode> Captured = FiveNodes();
    Captured[1].bOwned = true;
    Captured[1].Guards.Empty();
    Captured[4].Guards[0].TotalHealth = 0; // previously destroyed, not captured
    TestTrue(TEXT("Captured and destroyed nodes can advance"), Simulate(Captured, 7, Plan));
    TestTrue(TEXT("Captured forest stays empty"), Plan.Nodes[1].Guards.IsEmpty());
    TestEqual(TEXT("Dead neutral Guardians cannot resurrect"),
        Plan.Nodes[4].Guards[0].TotalHealth, 0);
    TestEqual(TEXT("Only surviving sites recruit"), Plan.NewNeutralUnits, 4);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesWeekAtomicTest, "AIHeroes.WorldWeek.AtomicFailureAndCap",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesWeekAtomicTest::RunTest(const FString&)
{
    TArray<FAIHeroesWeekNode> Bad = FiveNodes();
    FAIHeroesWeekPlan Plan;
    FString Error;
    Bad[1].Guards[0].UnitHealth = 500000;
    Bad[1].Guards[0].TotalHealth = 500000;
    TestFalse(TEXT("Overflowing next-week reinforcement fails closed"),
        FAIHeroesWorldWeekRules::PlanNextDay(7, Bad, Plan, Error));
    TestTrue(TEXT("Overflow failure carries reason"), !Error.IsEmpty());
    TestTrue(TEXT("Overflow does not provide partial node state"), Plan.Nodes.IsEmpty());
    TestEqual(TEXT("Existing input is untouched by failed plan"),
        Bad[1].Guards[0].TotalHealth, 500000);

    Bad = FiveNodes();
    Bad[2].Id = TEXT("FOREST");
    TestFalse(TEXT("Duplicate zone ID refused"), Simulate(Bad, 7, Plan));
    Bad = FiveNodes();
    Bad[1].Guards[0].UnitId = TEXT("NOT_A_WOLF");
    TestFalse(TEXT("Guard type mismatch refused"), Simulate(Bad, 7, Plan));
    Bad = FiveNodes();
    Bad[3].Guards.Add(FAIHeroesArmyStack(TEXT("Raider"), 1, 11, 5, 2, 5));
    TestFalse(TEXT("Third hostile formation refused"), Simulate(Bad, 7, Plan));
    Bad = FiveNodes();
    Bad[0].bOwned = false;
    TestFalse(TEXT("Enemy cannot steal starting HQ"), Simulate(Bad, 7, Plan));
    Bad = FiveNodes();
    Bad[1].bOwned = true; // an invalid captured mine still has living guards
    TestFalse(TEXT("Captured node cannot hold living enemy guards"),
        Simulate(Bad, 7, Plan));
    TestEqual(TEXT("No bad plan can leak prepared nodes"), Plan.Nodes.Num(), 0);

    Bad = FiveNodes();
    Bad[1].Guards[0].TotalHealth = 39 * 9; // near the 40-unit stack cap
    if (!TestTrue(TEXT("Maximum growth clamps safely"),
        Simulate(Bad, 7, Plan))) return false;
    TestEqual(TEXT("At most one unit added at cap"),
        Plan.Nodes[1].Guards[0].Count(), 40);
    Bad[1].Guards[0].TotalHealth = 40 * 9;
    TestTrue(TEXT("At cap, plan succeeds with zero addition at this site"),
        Simulate(Bad, 7, Plan));
    TestEqual(TEXT("At cap, count never exceeds 40"),
        Plan.Nodes[1].Guards[0].Count(), 40);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesWeekSaveTest, "AIHeroes.WorldWeek.SaveSchemaCompatibility",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesWeekSaveTest::RunTest(const FString&)
{
    FAIHeroesWeekPlan Plan;
    if (!TestTrue(TEXT("Realistic week transition succeeds"),
        Simulate(FiveNodes(), 7, Plan))) return false;
    UAIHeroesCampaignSave* Save = NewObject<UAIHeroesCampaignSave>(GetTransientPackage());
    Save->Day = Plan.NewDay;
    Save->HeroNodeId = TEXT("HQ");
    Save->SelectedNodeId = TEXT("FOREST");
    Save->PlayerArmy.Add(FAIHeroesArmyStack(TEXT("Militia"), 8, 12, 4, 2, 3));
    Save->PlayerArmy.Add(FAIHeroesArmyStack(TEXT("Scout"), 3, 8, 6, 1, 7));
    for (const FAIHeroesWeekNode& World : Plan.Nodes)
    {
        FAIHeroesSavedNode Node;
        Node.NodeId = World.Id;
        Node.Type = World.Type;
        Node.bOwned = World.bOwned;
        Node.Defenders = World.Guards;
        Save->NodeStates.Add(MoveTemp(Node));
    }

    FString Error;
    TestTrue(TEXT("Every grown army validates in existing SaveGame schema"),
        FAIHeroesCampaignRules::Validate(*Save, Error));
    TestEqual(TEXT("Legacy schema number stays unchanged"),
        Save->SchemaVersion, UAIHeroesCampaignSave::CurrentSchema);
    TestEqual(TEXT("New week persists only ordinary day/node state"),
        Save->Day, 8);
    TestEqual(TEXT("Growth persists as original saved guard HP"),
        Save->NodeStates[1].Defenders[0].Count(), 6);
    return true;
}
#endif
