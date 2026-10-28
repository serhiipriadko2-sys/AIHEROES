#include "AIHeroesGridBattle.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

namespace
{
FAIHeroesArmyStack Militia(int32 Count = 8)
{
    return FAIHeroesArmyStack(TEXT("Militia"), Count, 12, 4, 2, 3);
}

FAIHeroesArmyStack Scout(int32 Count = 3)
{
    return FAIHeroesArmyStack(TEXT("Scout"), Count, 8, 6, 1, 7);
}

FAIHeroesArmyStack Raider(int32 Count = 6)
{
    return FAIHeroesArmyStack(TEXT("Raider"), Count, 11, 5, 2, 5);
}

FAIHeroesArmyStack Guardian(int32 Count = 7)
{
    return FAIHeroesArmyStack(TEXT("Guardian"), Count, 24, 7, 4, 2);
}

int32 Distance(const FIntPoint& A, const FIntPoint& B)
{
    return FMath::Abs(A.X - B.X) + FMath::Abs(A.Y - B.Y);
}
}

// Fails if Begin precomputes attacks, changes HP, or uses an alternate deployment.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesGridIncrementalTest,
    "AIHeroes.Grid.IncrementalStart",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesGridIncrementalTest::RunTest(const FString& Parameters)
{
    const TArray<FAIHeroesArmyStack> Players = {Militia(), Scout()};
    const TArray<FAIHeroesArmyStack> Enemies = {Raider(), Raider(4)};
    FAIHeroesGridSession Session;
    FString Error;
    TestTrue(TEXT("Known armies begin"),
        FAIHeroesGridBattle::Begin(Players, Enemies, 1, Session, Error));
    TestEqual(TEXT("No rounds calculated at begin"), Session.Round, 0);
    TestEqual(TEXT("No future actions at begin"), Session.Actions.Num(), 0);
    TestFalse(TEXT("Live battle is not completed"), Session.bCompleted);
    TestEqual(TEXT("Player HP unchanged"), Session.PlayerArmy[0].TotalHealth, Players[0].TotalHealth);
    TestEqual(TEXT("Enemy HP unchanged"), Session.EnemyArmy[0].TotalHealth, Enemies[0].TotalHealth);
    TestTrue(TEXT("Player deployment"), Session.PlayerCells[0] == FIntPoint(0, 2) &&
        Session.PlayerCells[1] == FIntPoint(0, 3));
    TestTrue(TEXT("Enemy deployment"), Session.EnemyCells[0] == FIntPoint(7, 2) &&
        Session.EnemyCells[1] == FIntPoint(7, 3));
    TestFalse(TEXT("Incomplete session cannot claim victory"),
        FAIHeroesGridBattle::GetOutcome(Session).bVictory);

    FAIHeroesGridAction Action;
    TestTrue(TEXT("One runtime action produced"),
        FAIHeroesGridBattle::Step(Session, Action, Error) == EAIHeroesGridStepResult::ActionProduced);
    TestEqual(TEXT("Exactly one receipt added"), Session.Actions.Num(), 1);
    TestTrue(TEXT("Scout must move before attacking at distance seven"),
        Action.Kind == EAIHeroesGridActionKind::Move && Action.bPlayerAttacker &&
        Action.AttackerStackIndex == 1 && Action.Path.Num() <= 3);
    TestEqual(TEXT("First move causes no damage"), Action.Damage, 0);
    TestEqual(TEXT("Caller player input remains untouched"), Players[0].TotalHealth, 96);
    return true;
}

// Fails if movement skips occupied cells or diagonal steps, or if a dead stack
// continues blocking its former tile.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesGridPathTest,
    "AIHeroes.Grid.DeterministicPathAndOccupancy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesGridPathTest::RunTest(const FString& Parameters)
{
    auto Prepare = [](bool bBlockerAlive)
    {
        FAIHeroesGridSession Session;
        FString Error;
        FAIHeroesGridBattle::Begin({Militia(), Guardian(1)}, {Guardian()}, 0, Session, Error);
        Session.PlayerCells[0] = FIntPoint(0, 2);
        Session.PlayerCells[1] = FIntPoint(1, 2);
        Session.EnemyCells[0] = FIntPoint(4, 2);
        if (!bBlockerAlive) Session.PlayerArmy[1].TotalHealth = 0;
        return Session;
    };
    FString Error;
    FAIHeroesGridSession Blocked = Prepare(true);
    FAIHeroesGridAction Detour;
    TestTrue(TEXT("Detour action produced"),
        FAIHeroesGridBattle::Step(Blocked, Detour, Error) == EAIHeroesGridStepResult::ActionProduced);
    TestTrue(TEXT("Militia moves up to two cells"),
        Detour.Kind == EAIHeroesGridActionKind::Move && Detour.Path.Num() == 2);
    TestFalse(TEXT("Path avoids a live friendly stack"), Detour.Path.Contains(FIntPoint(1, 2)));
    TestTrue(TEXT("Equal BFS goals use row-major tie break"),
        Detour.Path.Num() == 2 && Detour.Path[0] == FIntPoint(0, 1) &&
        Detour.Path[1] == FIntPoint(1, 1));

    FAIHeroesGridSession Again = Prepare(true);
    FAIHeroesGridAction Repeated;
    FAIHeroesGridBattle::Step(Again, Repeated, Error);
    TestTrue(TEXT("Path repeat is identical"), Detour.Path == Repeated.Path);

    FAIHeroesGridSession Freed = Prepare(false);
    FAIHeroesGridAction Direct;
    FAIHeroesGridBattle::Step(Freed, Direct, Error);
    TestTrue(TEXT("Dead friendly unit releases its cell"),
        Direct.Kind == EAIHeroesGridActionKind::Move &&
        Direct.Path.Contains(FIntPoint(1, 2)));
    return true;
}

// Fails if ranged fire incorrectly needs adjacent cells/LOS, or if a move also
// precomputes and applies the subsequent attack in the same Step.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesGridRangeTest,
    "AIHeroes.Grid.RangeAndRuntimeAttack",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesGridRangeTest::RunTest(const FString& Parameters)
{
    FString Error;
    FAIHeroesGridSession Ranged;
    FAIHeroesGridBattle::Begin({Scout(), Militia()}, {Guardian()}, 1, Ranged, Error);
    Ranged.PlayerCells[0] = FIntPoint(0, 2);
    Ranged.PlayerCells[1] = FIntPoint(1, 2);
    Ranged.EnemyCells[0] = FIntPoint(3, 2);
    FAIHeroesGridAction Shot;
    TestTrue(TEXT("Scout action produced"),
        FAIHeroesGridBattle::Step(Ranged, Shot, Error) == EAIHeroesGridStepResult::ActionProduced);
    TestTrue(TEXT("Scout attacks at Manhattan distance three through friendly occupancy"),
        Shot.Kind == EAIHeroesGridActionKind::Attack && Distance(Shot.FromCell, Shot.ToCell) == 3);
    TestEqual(TEXT("Scout did not move"), Ranged.PlayerCells[0].X, 0);

    FAIHeroesGridSession Melee;
    FAIHeroesGridBattle::Begin({Militia()}, {Guardian()}, 0, Melee, Error);
    Melee.EnemyCells[0] = FIntPoint(2, 2);
    FAIHeroesGridAction Move;
    FAIHeroesGridBattle::Step(Melee, Move, Error);
    TestTrue(TEXT("Melee must approach distance two"), Move.Kind == EAIHeroesGridActionKind::Move);
    TestEqual(TEXT("Movement has no attack effect"), Melee.EnemyArmy[0].TotalHealth, 168);
    TestEqual(TEXT("Move computes one receipt only"), Melee.Actions.Num(), 1);
    FAIHeroesGridAction Hit;
    FAIHeroesGridBattle::Step(Melee, Hit, Error);
    TestTrue(TEXT("Attack is calculated on the following step"),
        Hit.Kind == EAIHeroesGridActionKind::Attack && Distance(Hit.FromCell, Hit.ToCell) == 1);
    TestEqual(TEXT("Same activation performs move then attack"), Hit.AttackerStackIndex, Move.AttackerStackIndex);
    TestEqual(TEXT("Attack round matches movement round"), Hit.Round, Move.Round);

    FAIHeroesGridSession NoCachedAttack;
    FAIHeroesGridBattle::Begin({Militia()}, {Guardian()}, 0, NoCachedAttack, Error);
    NoCachedAttack.EnemyCells[0] = FIntPoint(2, 2);
    FAIHeroesGridBattle::Step(NoCachedAttack, Move, Error);
    NoCachedAttack.EnemyArmy[0].TotalHealth = 0;
    TestTrue(TEXT("Next step observes live state, never a cached future attack"),
        FAIHeroesGridBattle::Step(NoCachedAttack, Hit, Error) == EAIHeroesGridStepResult::Completed);
    TestEqual(TEXT("No new attack after opponents disappear"), NoCachedAttack.Actions.Num(), 1);
    return true;
}

// Fails if duplicate UnitId merges the neutral stacks, or if protocol upgrades
// do not affect target selection at the actual activation.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesGridProtocolTest,
    "AIHeroes.Grid.ProtocolTargetDivergence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesGridProtocolTest::RunTest(const FString& Parameters)
{
    auto FirstHit = [](int32 Tier)
    {
        FAIHeroesGridSession Session;
        FString Error;
        FAIHeroesGridBattle::Begin({Scout()}, {Raider(6), Raider(1)}, Tier, Session, Error);
        Session.PlayerCells[0] = FIntPoint(2, 2);
        Session.EnemyCells[0] = FIntPoint(3, 2);
        Session.EnemyCells[1] = FIntPoint(2, 3);
        FAIHeroesGridAction Action;
        FAIHeroesGridBattle::Step(Session, Action, Error);
        return Action;
    };
    const FAIHeroesGridAction Seed = FirstHit(0);
    const FAIHeroesGridAction Planner = FirstHit(3);
    TestTrue(TEXT("Seed attacks the first legal stack"),
        Seed.Kind == EAIHeroesGridActionKind::Attack && Seed.TargetStackIndex == 0);
    TestTrue(TEXT("Planner attacks the separately indexed lethal target"),
        Planner.Kind == EAIHeroesGridActionKind::Attack && Planner.TargetStackIndex == 1);
    TestEqual(TEXT("Both targets retain the same unit type"), Seed.TargetId, Planner.TargetId);
    TestEqual(TEXT("Planner actually eliminates the weak stack"), Planner.TargetHealthAfter, 0);
    TestTrue(TEXT("Receipts explain the chosen policy"),
        Seed.ToLogLine().Contains(TEXT("first legal target")) &&
        Planner.ToLogLine().Contains(TEXT("lethal priority")));
    return true;
}

// Fails if the headless forecast has separate combat rules or if a caller's
// pacing/pauses change the next legal action.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesGridRunnerTest,
    "AIHeroes.Grid.StepMatchesForecast",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesGridRunnerTest::RunTest(const FString& Parameters)
{
    const TArray<FAIHeroesArmyStack> Players = {Militia(), Scout()};
    const TArray<FAIHeroesArmyStack> Enemies = {Raider(6), Raider(4)};
    for (int32 Tier = 0; Tier <= 3; ++Tier)
    {
        FAIHeroesGridSession Session;
        FString Error;
        TestTrue(TEXT("Incremental session begins"),
            FAIHeroesGridBattle::Begin(Players, Enemies, Tier, Session, Error));
        int32 Count = 0;
        while (!Session.bCompleted && Count++ <= FAIHeroesGridBattle::MaxActions)
        {
            const FAIHeroesGridSession Before = Session;
            FAIHeroesGridAction Action;
            const EAIHeroesGridStepResult Result = FAIHeroesGridBattle::Step(Session, Action, Error);
            TestTrue(TEXT("Every step is valid"), Result != EAIHeroesGridStepResult::Error);
            if (Result == EAIHeroesGridStepResult::Error) break;
            if (Result == EAIHeroesGridStepResult::Completed) break;
            TestEqual(TEXT("One event per call"), Session.Actions.Num(), Before.Actions.Num() + 1);
            if (Action.Kind == EAIHeroesGridActionKind::Move)
            {
                FIntPoint Previous = Action.FromCell;
                TestTrue(TEXT("Bounded movement"), Action.Path.Num() >= 1 &&
                    Action.Path.Num() <= (Action.AttackerId == FName(TEXT("Scout")) ? 3 : 2));
                for (const FIntPoint& Cell : Action.Path)
                {
                    TestEqual(TEXT("Only orthogonal neighbor steps"), Distance(Previous, Cell), 1);
                    TestTrue(TEXT("Movement remains in bounds"),
                        Cell.X >= 0 && Cell.X < 8 && Cell.Y >= 0 && Cell.Y < 6);
                    Previous = Cell;
                }
                TestTrue(TEXT("Movement endpoint matches path"), Previous == Action.ToCell);
            }
            if (Action.Kind == EAIHeroesGridActionKind::Attack)
            {
                TestTrue(TEXT("Attack is in range"), Distance(Action.FromCell, Action.ToCell) <=
                    (Action.AttackerId == FName(TEXT("Scout")) ? 3 : 1));
                TestTrue(TEXT("Positive damage, clamped HP"), Action.Damage > 0 &&
                    Action.TargetHealthAfter >= 0 && Action.TargetHealthAfter < Action.TargetHealthBefore);
            }

            // Copying/storing the current session represents arbitrary pause and
            // fast-forward scheduling; neither operation calculates a new event.
            FAIHeroesGridSession Alternate = Before;
            FAIHeroesGridAction SameAction;
            FAIHeroesGridBattle::Step(Alternate, SameAction, Error);
            TestEqual(TEXT("Next action depends on state, not time"), Action.ToLogLine(), SameAction.ToLogLine());
        }
        TestTrue(TEXT("Session terminates within event bound"), Session.bCompleted);
        const FAIHeroesBattleOutcome Live = FAIHeroesGridBattle::GetOutcome(Session);
        const FAIHeroesBattleOutcome Forecast = FAIHeroesGridBattle::RunToEnd(Players, Enemies, Tier);
        TestTrue(TEXT("All canonical events match forecast"), Live.Trace == Forecast.Trace);
        TestEqual(TEXT("Final summary matches forecast"), Live.Summary, Forecast.Summary);
        TestEqual(TEXT("Winner matches forecast"), Live.bVictory, Forecast.bVictory);
        TestEqual(TEXT("Round count matches forecast"), Live.Rounds, Forecast.Rounds);
        TestTrue(TEXT("V2 does not impersonate legacy attack-only receipts"), Live.Actions.IsEmpty());
        for (int32 Index = 0; Index < Live.PlayerAfter.Num(); ++Index)
            TestEqual(TEXT("Player survivors match"), Live.PlayerAfter[Index].TotalHealth,
                Forecast.PlayerAfter[Index].TotalHealth);
        for (int32 Index = 0; Index < Live.EnemyAfter.Num(); ++Index)
            TestEqual(TEXT("Enemy survivors match"), Live.EnemyAfter[Index].TotalHealth,
                Forecast.EnemyAfter[Index].TotalHealth);
    }
    return true;
}

// Fails if malformed units/tier/deployment can produce an authoritative winner.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesGridValidationTest,
    "AIHeroes.Grid.ValidationAndTerminalStates",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesGridValidationTest::RunTest(const FString& Parameters)
{
    FString Error;
    FAIHeroesGridSession Session;
    TestFalse(TEXT("Unknown model/unit profile rejected"), FAIHeroesGridBattle::Begin(
        {FAIHeroesArmyStack(TEXT("Unknown"), 1, 12, 4, 2, 3)}, {Raider()}, 0, Session, Error));
    FAIHeroesArmyStack Modified = Militia();
    Modified.Attack = MAX_int32;
    TestFalse(TEXT("Modified stats rejected before arithmetic"),
        FAIHeroesGridBattle::Begin({Modified}, {Raider()}, 0, Session, Error));
    Modified = Militia();
    Modified.TotalHealth = -1;
    TestFalse(TEXT("Negative HP rejected"),
        FAIHeroesGridBattle::Begin({Modified}, {Raider()}, 0, Session, Error));
    TestFalse(TEXT("More than two stacks rejected"), FAIHeroesGridBattle::Begin(
        {Militia(), Militia(), Militia()}, {Raider()}, 0, Session, Error));
    TestFalse(TEXT("Out of range tier rejected"),
        FAIHeroesGridBattle::Begin({Militia()}, {Raider()}, 4, Session, Error));
    TestFalse(TEXT("Invalid runner cannot claim victory"),
        FAIHeroesGridBattle::RunToEnd({Modified}, {Raider()}, 0).bVictory);

    FAIHeroesGridAction Action;
    FAIHeroesGridSession Uninitialized;
    TestTrue(TEXT("Uninitialized session rejected"), FAIHeroesGridBattle::Step(
        Uninitialized, Action, Error) == EAIHeroesGridStepResult::Error);
    FAIHeroesGridBattle::Begin({Militia()}, {Raider()}, 0, Session, Error);
    Session.EnemyCells[0] = Session.PlayerCells[0];
    TestTrue(TEXT("Overlapping live cells rejected"),
        FAIHeroesGridBattle::Step(Session, Action, Error) == EAIHeroesGridStepResult::Error);
    TestFalse(TEXT("Invalid state cannot produce winner"), FAIHeroesGridBattle::GetOutcome(Session).bVictory);
    FAIHeroesGridBattle::Begin({Militia()}, {Raider()}, 0, Session, Error);
    Session.PlayerCells[0] = FIntPoint(-1, 0);
    TestTrue(TEXT("Outside arena rejected"),
        FAIHeroesGridBattle::Step(Session, Action, Error) == EAIHeroesGridStepResult::Error);
    FAIHeroesGridBattle::Begin({Militia()}, {Raider()}, 0, Session, Error);
    Session.ProtocolTier = 3;
    TestTrue(TEXT("Midbattle policy replacement rejected"),
        FAIHeroesGridBattle::Step(Session, Action, Error) == EAIHeroesGridStepResult::Error);

    TestTrue(TEXT("Empty player army is a valid terminal defeat"),
        FAIHeroesGridBattle::Begin({}, {Raider()}, 0, Session, Error));
    TestTrue(TEXT("Terminal state completes without any attacks"), Session.bCompleted && Session.Actions.IsEmpty());
    TestTrue(TEXT("Empty army loses"), FAIHeroesGridBattle::GetOutcome(Session).bDefeat);
    TestTrue(TEXT("No round needed"), FAIHeroesGridBattle::GetOutcome(Session).Rounds == 0);
    FAIHeroesGridBattle::Begin({Militia()}, {}, 0, Session, Error);
    TestTrue(TEXT("Empty enemy army is a victory"), FAIHeroesGridBattle::GetOutcome(Session).bVictory);
    TestTrue(TEXT("Stepping completed state is idempotent"),
        FAIHeroesGridBattle::Step(Session, Action, Error) == EAIHeroesGridStepResult::Completed);
    TestEqual(TEXT("Completed repeat adds no receipt"), Session.Actions.Num(), 0);
    return true;
}

// Fails if the round cap is ignored, or if a repeat terminal callback emits
// additional attacks. Only the round counter is advanced to exercise the cap.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesGridRoundLimitTest,
    "AIHeroes.Grid.RoundLimitAndInitiative",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesGridRoundLimitTest::RunTest(const FString& Parameters)
{
    FString Error;
    FAIHeroesGridSession Session;
    FAIHeroesGridBattle::Begin({Guardian(1)}, {Guardian(1)}, 0, Session, Error);
    Session.Round = FAIHeroesGridBattle::MaxRounds - 1;
    FAIHeroesGridAction First;
    FAIHeroesGridBattle::Step(Session, First, Error);
    TestTrue(TEXT("Player acts first on equal initiative"), First.bPlayerAttacker);
    int32 Count = 0;
    while (!Session.bCompleted && Count++ < 8)
    {
        FAIHeroesGridAction Action;
        if (FAIHeroesGridBattle::Step(Session, Action, Error) == EAIHeroesGridStepResult::Error) break;
    }
    const FAIHeroesBattleOutcome Outcome = FAIHeroesGridBattle::GetOutcome(Session);
    TestTrue(TEXT("Battle reaches round cap"), Session.bCompleted && Outcome.Rounds == 30);
    TestFalse(TEXT("Surviving sides are not an invented victory"), Outcome.bVictory);
    TestFalse(TEXT("Surviving sides are not an invented defeat"), Outcome.bDefeat);
    TestTrue(TEXT("Round cap explicitly reports stalemate"), Outcome.Summary.Contains(TEXT("STALEMATE")));
    const int32 ActionCount = Session.Actions.Num();
    FAIHeroesGridAction Again;
    TestTrue(TEXT("Completed battle cannot calculate further damage"),
        FAIHeroesGridBattle::Step(Session, Again, Error) == EAIHeroesGridStepResult::Completed);
    TestEqual(TEXT("No duplicate terminal events"), Session.Actions.Num(), ActionCount);
    return true;
}

// Fails if a unit killed earlier in the round still executes its queued turn,
// or if equal score/initiative stacks are not kept in stable index order.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesGridDeadTurnTest,
    "AIHeroes.Grid.DeadTurnsAndStableIndices",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesGridDeadTurnTest::RunTest(const FString& Parameters)
{
    FAIHeroesGridSession Session;
    FString Error;
    TestTrue(TEXT("Duplicate neutral types stay as independent stacks"),
        FAIHeroesGridBattle::Begin({Scout(100)}, {Raider(1), Raider(1)}, 1, Session, Error));
    Session.PlayerCells[0] = FIntPoint(0, 2);
    Session.EnemyCells[0] = FIntPoint(2, 2);
    Session.EnemyCells[1] = FIntPoint(2, 3);
    FAIHeroesGridAction Kill;
    FAIHeroesGridBattle::Step(Session, Kill, Error);
    TestTrue(TEXT("Equal threat resolves to lower stack index"),
        Kill.Kind == EAIHeroesGridActionKind::Attack && Kill.TargetStackIndex == 0);
    TestEqual(TEXT("First enemy dies before its activation"), Session.EnemyArmy[0].TotalHealth, 0);
    FAIHeroesGridAction Next;
    FAIHeroesGridBattle::Step(Session, Next, Error);
    TestTrue(TEXT("Dead queued enemy skipped; surviving second stack acts"),
        !Next.bPlayerAttacker && Next.AttackerStackIndex == 1 && Next.Round == 1);
    TestTrue(TEXT("No dead actor receipt added"), Session.Actions.Num() == 2);
    return true;
}

// Fails if the public read-back completion flag alone can authorize an outcome
// while both armies are alive and the engine has not finished any round.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesGridForgedCompletionTest,
    "AIHeroes.Grid.RejectForgedCompletion",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesGridForgedCompletionTest::RunTest(const FString& Parameters)
{
    FString Error;
    FAIHeroesGridSession Session;
    FAIHeroesGridBattle::Begin({Militia()}, {Raider()}, 0, Session, Error);
    Session.bCompleted = true;
    const FAIHeroesBattleOutcome Forged = FAIHeroesGridBattle::GetOutcome(Session);
    TestTrue(TEXT("Forged completion has no authoritative result"),
        Forged.Summary.Contains(TEXT("unavailable")) && Forged.Trace.IsEmpty() &&
        Forged.PlayerAfter.IsEmpty() && Forged.EnemyAfter.IsEmpty());
    FAIHeroesGridAction Action;
    TestTrue(TEXT("Step rejects an externally modified completion flag"),
        FAIHeroesGridBattle::Step(Session, Action, Error) == EAIHeroesGridStepResult::Error);
    TestFalse(TEXT("Invalid completion is cleared"), Session.bCompleted);
    TestEqual(TEXT("Forgery cannot add an action"), Session.Actions.Num(), 0);

    // A round number alone is also insufficient: its internal activation queue
    // must actually have finished the final round.
    FAIHeroesGridBattle::Begin({Militia()}, {Raider()}, 0, Session, Error);
    Session.Round = FAIHeroesGridBattle::MaxRounds;
    TestTrue(TEXT("Empty queue at forged round thirty is rejected"),
        FAIHeroesGridBattle::Step(Session, Action, Error) == EAIHeroesGridStepResult::Error);
    TestTrue(TEXT("Round-counter forgery has no outcome"),
        FAIHeroesGridBattle::GetOutcome(Session).Summary.Contains(TEXT("unavailable")));

    // A genuine terminal army snapshot cannot be made nonterminal by toggling
    // only the public flag either.
    FAIHeroesGridBattle::Begin({Militia()}, {}, 0, Session, Error);
    Session.bCompleted = false;
    TestTrue(TEXT("Completed flag cannot be externally reset"),
        FAIHeroesGridBattle::Step(Session, Action, Error) == EAIHeroesGridStepResult::Error);
    return true;
}
#endif
