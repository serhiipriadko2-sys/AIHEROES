#include "AIHeroesGridBattle.h"

namespace
{
struct FGridProfile
{
    const TCHAR* Id;
    int32 Health;
    int32 Attack;
    int32 Defense;
    int32 Initiative;
    int32 Speed;
    int32 Range;
};

// Frozen GridRulesV2 profiles. Changing these requires another rule version.
const FGridProfile Profiles[] = {
    {TEXT("Militia"), 12, 4, 2, 3, 2, 1},
    {TEXT("Scout"), 8, 6, 1, 7, 3, 3},
    {TEXT("Wolf"), 9, 3, 1, 8, 3, 1},
    {TEXT("Bandit"), 10, 4, 2, 4, 2, 1},
    {TEXT("Raider"), 11, 5, 2, 5, 2, 1},
    {TEXT("Guardian"), 24, 7, 4, 2, 1, 1}
};

const FGridProfile* FindProfile(FName Id)
{
    for (const FGridProfile& Profile : Profiles)
    {
        if (Id == FName(Profile.Id))
        {
            return &Profile;
        }
    }
    return nullptr;
}

bool AnyAlive(const TArray<FAIHeroesArmyStack>& Army)
{
    for (const FAIHeroesArmyStack& Stack : Army)
    {
        if (Stack.IsAlive()) return true;
    }
    return false;
}

int32 LivingUnits(const TArray<FAIHeroesArmyStack>& Army)
{
    int32 Count = 0;
    for (const FAIHeroesArmyStack& Stack : Army) Count += Stack.Count();
    return Count;
}

bool InBounds(const FIntPoint& Cell)
{
    return Cell.X >= 0 && Cell.X < FAIHeroesGridBattle::Width &&
        Cell.Y >= 0 && Cell.Y < FAIHeroesGridBattle::Height;
}

int32 Manhattan(const FIntPoint& A, const FIntPoint& B)
{
    return FMath::Abs(A.X - B.X) + FMath::Abs(A.Y - B.Y);
}

bool ValidateArmy(const TArray<FAIHeroesArmyStack>& Army, FString& Error)
{
    if (Army.Num() > 2)
    {
        Error = TEXT("Grid V2 supports at most two stacks per side.");
        return false;
    }
    for (const FAIHeroesArmyStack& Stack : Army)
    {
        const FGridProfile* Profile = FindProfile(Stack.UnitId);
        if (!Profile || Stack.TotalHealth < 0 || Stack.TotalHealth > 500000 ||
            Stack.UnitHealth != Profile->Health || Stack.Attack != Profile->Attack ||
            Stack.Defense != Profile->Defense || Stack.Initiative != Profile->Initiative)
        {
            Error = TEXT("Grid V2 rejects unknown units, modified profiles or invalid HP.");
            return false;
        }
    }
    return true;
}

bool ValidateState(const FAIHeroesGridSession& Session, FString& Error)
{
    if (Session.Round < 0 || Session.Round > FAIHeroesGridBattle::MaxRounds ||
        Session.ProtocolTier < 0 || Session.ProtocolTier > 3 ||
        Session.Actions.Num() > FAIHeroesGridBattle::MaxActions ||
        Session.PlayerCells.Num() != Session.PlayerArmy.Num() ||
        Session.EnemyCells.Num() != Session.EnemyArmy.Num())
    {
        Error = TEXT("Invalid grid session bounds or deployment.");
        return false;
    }
    if (!ValidateArmy(Session.PlayerArmy, Error) ||
        !ValidateArmy(Session.EnemyArmy, Error)) return false;

    TArray<FIntPoint> Occupied;
    auto ValidateCells = [&Occupied, &Error](const TArray<FAIHeroesArmyStack>& Army,
                                           const TArray<FIntPoint>& Cells)
    {
        for (int32 Index = 0; Index < Army.Num(); ++Index)
        {
            if (!InBounds(Cells[Index]))
            {
                Error = TEXT("A grid unit is outside the arena.");
                return false;
            }
            if (Army[Index].IsAlive())
            {
                if (Occupied.Contains(Cells[Index]))
                {
                    Error = TEXT("Live grid units overlap.");
                    return false;
                }
                Occupied.Add(Cells[Index]);
            }
        }
        return true;
    };
    return ValidateCells(Session.PlayerArmy, Session.PlayerCells) &&
        ValidateCells(Session.EnemyArmy, Session.EnemyCells);
}

int32 TargetScore(const FAIHeroesArmyStack& Attacker,
                  const FAIHeroesArmyStack& Target, int32 Tier)
{
    if (Tier <= 0) return 0; // Equal scores preserve the original stack order.
    const int32 Threat = Target.Count() * Target.Attack;
    if (Tier == 1) return Threat;
    if (Tier == 2) return Threat * 2 - Target.TotalHealth;
    const int32 Damage = Attacker.Count() * FMath::Max(1, Attacker.Attack - Target.Defense / 2);
    return Threat * 3 - Target.TotalHealth / 2 + (Damage >= Target.TotalHealth ? 1000 : 0);
}

FString PolicyReason(int32 Tier)
{
    switch (Tier)
    {
        case 0: return TEXT("first legal target");
        case 1: return TEXT("highest threat");
        case 2: return TEXT("finish damaged stack");
        default: return TEXT("lethal priority");
    }
}

int32 ChooseInRange(const FAIHeroesArmyStack& Attacker, const FIntPoint& Cell,
                    const TArray<FAIHeroesArmyStack>& Targets,
                    const TArray<FIntPoint>& TargetCells, int32 Range, int32 Tier)
{
    int32 Best = INDEX_NONE;
    int32 BestScore = MIN_int32;
    for (int32 Index = 0; Index < Targets.Num(); ++Index)
    {
        if (!Targets[Index].IsAlive() || Manhattan(Cell, TargetCells[Index]) > Range) continue;
        const int32 Score = TargetScore(Attacker, Targets[Index], Tier);
        if (Best == INDEX_NONE || Score > BestScore)
        {
            Best = Index;
            BestScore = Score;
        }
    }
    return Best;
}

bool OccupiedByLiving(const FAIHeroesGridSession& Session, const FIntPoint& Cell,
                      const FIntPoint& Source)
{
    if (Cell == Source) return false;
    for (int32 Index = 0; Index < Session.PlayerArmy.Num(); ++Index)
    {
        if (Session.PlayerArmy[Index].IsAlive() && Session.PlayerCells[Index] == Cell) return true;
    }
    for (int32 Index = 0; Index < Session.EnemyArmy.Num(); ++Index)
    {
        if (Session.EnemyArmy[Index].IsAlive() && Session.EnemyCells[Index] == Cell) return true;
    }
    return false;
}

bool FindApproachPath(const FAIHeroesGridSession& Session, const FIntPoint& Source,
                       const FIntPoint& Target, int32 Range, TArray<FIntPoint>& OutPath)
{
    constexpr int32 CellCount = FAIHeroesGridBattle::Width * FAIHeroesGridBattle::Height;
    int32 Distance[CellCount];
    int32 Parent[CellCount];
    for (int32 Index = 0; Index < CellCount; ++Index)
    {
        Distance[Index] = INDEX_NONE;
        Parent[Index] = INDEX_NONE;
    }
    auto ToIndex = [](const FIntPoint& Cell) { return Cell.Y * FAIHeroesGridBattle::Width + Cell.X; };
    auto FromIndex = [](int32 Index) { return FIntPoint(Index % FAIHeroesGridBattle::Width,
                                                       Index / FAIHeroesGridBattle::Width); };
    const int32 Start = ToIndex(Source);
    TArray<int32> Queue;
    Queue.Add(Start);
    Distance[Start] = 0;
    // This order and the row-major goal tie-break are part of GridRulesV2.
    const FIntPoint Neighbors[] = {FIntPoint(1, 0), FIntPoint(0, 1),
                                  FIntPoint(-1, 0), FIntPoint(0, -1)};
    for (int32 Cursor = 0; Cursor < Queue.Num(); ++Cursor)
    {
        const int32 CurrentIndex = Queue[Cursor];
        const FIntPoint Current = FromIndex(CurrentIndex);
        for (const FIntPoint& Offset : Neighbors)
        {
            const FIntPoint Next = Current + Offset;
            if (!InBounds(Next) || OccupiedByLiving(Session, Next, Source)) continue;
            const int32 NextIndex = ToIndex(Next);
            if (Distance[NextIndex] != INDEX_NONE) continue;
            Distance[NextIndex] = Distance[CurrentIndex] + 1;
            Parent[NextIndex] = CurrentIndex;
            Queue.Add(NextIndex);
        }
    }

    int32 Goal = INDEX_NONE;
    int32 BestDistance = MAX_int32;
    for (int32 Index = 0; Index < CellCount; ++Index)
    {
        const int32 RangeToTarget = Manhattan(FromIndex(Index), Target);
        if (Distance[Index] >= 0 && RangeToTarget >= 1 && RangeToTarget <= Range &&
            Distance[Index] < BestDistance)
        {
            Goal = Index;
            BestDistance = Distance[Index];
        }
    }
    OutPath.Reset();
    if (Goal == INDEX_NONE) return false;
    for (int32 Current = Goal; Current != Start; Current = Parent[Current])
    {
        if (Current == INDEX_NONE || OutPath.Num() >= CellCount) return false;
        OutPath.Insert(FromIndex(Current), 0);
    }
    return true;
}
}

FString FAIHeroesGridAction::ToLogLine() const
{
    const TCHAR* Side = bPlayerAttacker ? TEXT("HERO") : TEXT("NPC");
    if (Kind == EAIHeroesGridActionKind::Move)
    {
        FString Cells;
        for (const FIntPoint& Cell : Path)
        {
            Cells += FString::Printf(TEXT(" (%d,%d)"), Cell.X, Cell.Y);
        }
        return FString::Printf(TEXT("R%d %s[%d] %s MOVE (%d,%d) ->%s | %s"),
            Round, Side, AttackerStackIndex, *AttackerId.ToString(), FromCell.X, FromCell.Y,
            *Cells, *Reason);
    }
    if (Kind == EAIHeroesGridActionKind::Attack)
    {
        return FString::Printf(TEXT("R%d %s[%d] %s ATTACK -> [%d] %s (-%d HP, %d->%d) | %s"),
            Round, Side, AttackerStackIndex, *AttackerId.ToString(), TargetStackIndex,
            *TargetId.ToString(), Damage, TargetHealthBefore, TargetHealthAfter, *Reason);
    }
    return FString::Printf(TEXT("R%d %s[%d] %s WAIT (%d,%d) | %s"),
        Round, Side, AttackerStackIndex, *AttackerId.ToString(), FromCell.X, FromCell.Y, *Reason);
}

bool FAIHeroesGridBattle::Begin(const TArray<FAIHeroesArmyStack>& PlayerArmy,
                               const TArray<FAIHeroesArmyStack>& EnemyArmy,
                               int32 ProtocolTier, FAIHeroesGridSession& OutSession,
                               FString& Error)
{
    Error.Empty();
    OutSession = FAIHeroesGridSession();
    if (ProtocolTier < 0 || ProtocolTier > 3 ||
        !ValidateArmy(PlayerArmy, Error) || !ValidateArmy(EnemyArmy, Error))
    {
        OutSession.bInvalid = true;
        if (Error.IsEmpty()) Error = TEXT("Invalid grid protocol tier.");
        return false;
    }
    OutSession.PlayerArmy = PlayerArmy;
    OutSession.EnemyArmy = EnemyArmy;
    OutSession.ProtocolTier = ProtocolTier;
    OutSession.FrozenProtocolTier = ProtocolTier;
    for (int32 Index = 0; Index < PlayerArmy.Num(); ++Index)
        OutSession.PlayerCells.Add(FIntPoint(0, 2 + Index));
    for (int32 Index = 0; Index < EnemyArmy.Num(); ++Index)
        OutSession.EnemyCells.Add(FIntPoint(Width - 1, 2 + Index));
    OutSession.bStarted = true;
    OutSession.bCompleted = !AnyAlive(PlayerArmy) || !AnyAlive(EnemyArmy);
    OutSession.bTerminal = OutSession.bCompleted;
    return true;
}

bool FAIHeroesGridBattle::HasReachedTerminalState(const FAIHeroesGridSession& Session)
{
    return !AnyAlive(Session.PlayerArmy) || !AnyAlive(Session.EnemyArmy) ||
        (Session.Round == MaxRounds && !Session.TurnOrder.IsEmpty() &&
         Session.TurnCursor == Session.TurnOrder.Num() && !Session.bAfterMove);
}

EAIHeroesGridStepResult FAIHeroesGridBattle::Step(
    FAIHeroesGridSession& Session, FAIHeroesGridAction& OutAction, FString& Error)
{
    Error.Empty();
    OutAction = FAIHeroesGridAction();
    if (!Session.bStarted || Session.bInvalid ||
        Session.bCompleted != Session.bTerminal ||
        (Session.bTerminal && !HasReachedTerminalState(Session)) ||
        Session.ProtocolTier != Session.FrozenProtocolTier ||
        !ValidateState(Session, Error) || Session.TurnCursor < 0 ||
        Session.TurnCursor > Session.TurnOrder.Num())
    {
        Session.bInvalid = true;
        Session.bCompleted = false;
        if (Error.IsEmpty()) Error = TEXT("Uninitialized, modified or invalid grid session.");
        return EAIHeroesGridStepResult::Error;
    }
    if (Session.bCompleted) return EAIHeroesGridStepResult::Completed;

    // At most four actors per round. Skipping a dead actor never emits an attack.
    while (true)
    {
        if (!AnyAlive(Session.PlayerArmy) || !AnyAlive(Session.EnemyArmy))
        {
            Session.bCompleted = true;
            Session.bTerminal = true;
            return EAIHeroesGridStepResult::Completed;
        }
        if (Session.TurnCursor >= Session.TurnOrder.Num())
        {
            if (Session.Round >= MaxRounds)
            {
                if (!HasReachedTerminalState(Session))
                {
                    Session.bInvalid = true;
                    Error = TEXT("Grid round limit has no completed activation queue.");
                    return EAIHeroesGridStepResult::Error;
                }
                Session.bCompleted = true;
                Session.bTerminal = true;
                return EAIHeroesGridStepResult::Completed;
            }
            ++Session.Round;
            Session.TurnOrder.Reset();
            Session.TurnCursor = 0;
            Session.bAfterMove = false;
            for (int32 Index = 0; Index < Session.PlayerArmy.Num(); ++Index)
                if (Session.PlayerArmy[Index].IsAlive())
                    Session.TurnOrder.Add({true, Index, Session.PlayerArmy[Index].Initiative});
            for (int32 Index = 0; Index < Session.EnemyArmy.Num(); ++Index)
                if (Session.EnemyArmy[Index].IsAlive())
                    Session.TurnOrder.Add({false, Index, Session.EnemyArmy[Index].Initiative});
            Session.TurnOrder.Sort([](const FAIHeroesGridSession::FTurn& A,
                                      const FAIHeroesGridSession::FTurn& B)
            {
                if (A.Initiative != B.Initiative) return A.Initiative > B.Initiative;
                if (A.bPlayer != B.bPlayer) return A.bPlayer;
                return A.StackIndex < B.StackIndex;
            });
        }

        const FAIHeroesGridSession::FTurn& Turn = Session.TurnOrder[Session.TurnCursor];
        TArray<FAIHeroesArmyStack>& Allies = Turn.bPlayer ? Session.PlayerArmy : Session.EnemyArmy;
        TArray<FAIHeroesArmyStack>& Targets = Turn.bPlayer ? Session.EnemyArmy : Session.PlayerArmy;
        TArray<FIntPoint>& AllyCells = Turn.bPlayer ? Session.PlayerCells : Session.EnemyCells;
        const TArray<FIntPoint>& TargetCells = Turn.bPlayer ? Session.EnemyCells : Session.PlayerCells;
        if (!Allies.IsValidIndex(Turn.StackIndex))
        {
            Session.bInvalid = true;
            Error = TEXT("Grid activation references an unknown stack.");
            return EAIHeroesGridStepResult::Error;
        }
        if (!Allies[Turn.StackIndex].IsAlive())
        {
            ++Session.TurnCursor;
            Session.bAfterMove = false;
            continue;
        }
        if (Session.Actions.Num() >= MaxActions)
        {
            Session.bInvalid = true;
            Error = TEXT("Grid action bound exhausted before completion.");
            return EAIHeroesGridStepResult::Error;
        }

        const FAIHeroesArmyStack& Attacker = Allies[Turn.StackIndex];
        const FGridProfile& Profile = *FindProfile(Attacker.UnitId);
        const int32 Tier = Turn.bPlayer ? Session.ProtocolTier : 1;
        OutAction.Round = Session.Round;
        OutAction.bPlayerAttacker = Turn.bPlayer;
        OutAction.AttackerStackIndex = Turn.StackIndex;
        OutAction.AttackerId = Attacker.UnitId;
        OutAction.FromCell = AllyCells[Turn.StackIndex];
        OutAction.ToCell = OutAction.FromCell;
        OutAction.Reason = PolicyReason(Tier);

        const int32 TargetIndex = ChooseInRange(Attacker, OutAction.FromCell,
            Targets, TargetCells, Profile.Range, Tier);
        if (TargetIndex != INDEX_NONE)
        {
            FAIHeroesArmyStack& Target = Targets[TargetIndex];
            OutAction.Kind = EAIHeroesGridActionKind::Attack;
            OutAction.TargetStackIndex = TargetIndex;
            OutAction.TargetId = Target.UnitId;
            OutAction.ToCell = TargetCells[TargetIndex];
            OutAction.TargetHealthBefore = Target.TotalHealth;
            OutAction.Damage = Attacker.Count() * FMath::Max(1, Attacker.Attack - Target.Defense / 2);
            Target.TotalHealth = FMath::Max(0, Target.TotalHealth - OutAction.Damage);
            OutAction.TargetHealthAfter = Target.TotalHealth;
            ++Session.TurnCursor;
            Session.bAfterMove = false;
        }
        else if (!Session.bAfterMove)
        {
            int32 BestTarget = INDEX_NONE;
            int32 BestScore = MIN_int32;
            TArray<FIntPoint> BestPath;
            for (int32 Index = 0; Index < Targets.Num(); ++Index)
            {
                if (!Targets[Index].IsAlive()) continue;
                TArray<FIntPoint> Path;
                if (!FindApproachPath(Session, OutAction.FromCell, TargetCells[Index], Profile.Range, Path) ||
                    Path.IsEmpty()) continue;
                const int32 Score = TargetScore(Attacker, Targets[Index], Tier);
                if (BestTarget == INDEX_NONE || Score > BestScore)
                {
                    BestTarget = Index;
                    BestScore = Score;
                    BestPath = MoveTemp(Path);
                }
            }
            if (BestTarget != INDEX_NONE)
            {
                OutAction.Kind = EAIHeroesGridActionKind::Move;
                OutAction.TargetStackIndex = BestTarget;
                OutAction.TargetId = Targets[BestTarget].UnitId;
                BestPath.SetNum(FMath::Min(Profile.Speed, BestPath.Num()));
                OutAction.Path = MoveTemp(BestPath);
                OutAction.ToCell = OutAction.Path.Last();
                AllyCells[Turn.StackIndex] = OutAction.ToCell;
                Session.bAfterMove = true;
            }
            else
            {
                OutAction.Kind = EAIHeroesGridActionKind::Wait;
                OutAction.Reason = TEXT("no reachable target");
                ++Session.TurnCursor;
            }
        }
        else
        {
            OutAction.Kind = EAIHeroesGridActionKind::Wait;
            OutAction.Reason = TEXT("no target in range after movement");
            ++Session.TurnCursor;
            Session.bAfterMove = false;
        }

        Session.Actions.Add(OutAction);
        if (!AnyAlive(Session.PlayerArmy) || !AnyAlive(Session.EnemyArmy) ||
            (Session.Round >= MaxRounds && Session.TurnCursor >= Session.TurnOrder.Num()))
        {
            Session.bCompleted = true;
            Session.bTerminal = true;
        }
        return EAIHeroesGridStepResult::ActionProduced;
    }
}

FAIHeroesBattleOutcome FAIHeroesGridBattle::GetOutcome(const FAIHeroesGridSession& Session)
{
    FAIHeroesBattleOutcome Outcome;
    FString Error;
    if (!Session.bStarted || Session.bInvalid || !Session.bCompleted || !Session.bTerminal ||
        !HasReachedTerminalState(Session) ||
        Session.ProtocolTier != Session.FrozenProtocolTier || !ValidateState(Session, Error))
    {
        Outcome.Summary = TEXT("Grid outcome unavailable: session incomplete or invalid.");
        return Outcome;
    }
    Outcome.Rounds = Session.Round;
    Outcome.PlayerAfter = Session.PlayerArmy;
    Outcome.EnemyAfter = Session.EnemyArmy;
    Outcome.bVictory = AnyAlive(Session.PlayerArmy) && !AnyAlive(Session.EnemyArmy);
    Outcome.bDefeat = !AnyAlive(Session.PlayerArmy);
    for (const FAIHeroesGridAction& Action : Session.Actions) Outcome.Trace.Add(Action.ToLogLine());
    Outcome.Summary = FString::Printf(TEXT("%s | %d rounds | your units: %d | enemy units: %d"),
        Outcome.bVictory ? TEXT("VICTORY") : Outcome.bDefeat ? TEXT("DEFEAT") : TEXT("STALEMATE"),
        Outcome.Rounds, LivingUnits(Session.PlayerArmy), LivingUnits(Session.EnemyArmy));
    // Legacy Outcome.Actions contains attack-only V1 receipts and is intentionally unused.
    return Outcome;
}

FAIHeroesBattleOutcome FAIHeroesGridBattle::RunToEnd(
    const TArray<FAIHeroesArmyStack>& PlayerArmy,
    const TArray<FAIHeroesArmyStack>& EnemyArmy, int32 ProtocolTier)
{
    FAIHeroesGridSession Session;
    FString Error;
    if (!Begin(PlayerArmy, EnemyArmy, ProtocolTier, Session, Error))
    {
        FAIHeroesBattleOutcome Invalid;
        Invalid.Summary = TEXT("Grid battle rejected: ") + Error;
        return Invalid;
    }
    for (int32 Count = 0; Count <= MaxActions; ++Count)
    {
        FAIHeroesGridAction Action;
        const EAIHeroesGridStepResult Result = Step(Session, Action, Error);
        if (Result == EAIHeroesGridStepResult::Completed) return GetOutcome(Session);
        if (Result == EAIHeroesGridStepResult::Error)
        {
            FAIHeroesBattleOutcome Invalid;
            Invalid.Summary = TEXT("Grid battle failed: ") + Error;
            return Invalid;
        }
    }
    FAIHeroesBattleOutcome Invalid;
    Invalid.Summary = TEXT("Grid battle failed: completion bound exhausted.");
    return Invalid;
}
