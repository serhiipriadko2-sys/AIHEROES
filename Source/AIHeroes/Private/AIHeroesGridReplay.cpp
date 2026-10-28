#include "AIHeroesGridReplay.h"

#include "AIHeroesBattleRecord.h"

bool FAIHeroesGridReplayRules::Build(
    const FAIHeroesBattleRecord& Record,
    FAIHeroesGridReplay& OutReplay,
    FString& Error)
{
    OutReplay = FAIHeroesGridReplay();
    Error.Empty();
    if (Record.RuleVersion != FAIHeroesGridBattle::RuleVersion ||
        Record.NodeId.IsNone())
    {
        Error = TEXT("3D grid replay requires a completed V2 battle.");
        return false;
    }

    // Recompute and verify ALL persisted outcomes before constructing
    // a visual action. The trace is evidence, never executable instructions.
    if (!FAIHeroesBattleRecordRules::Verify(Record, Error))
    {
        return false;
    }

    FAIHeroesGridSession Session;
    if (!FAIHeroesGridBattle::Begin(
        Record.PlayerBefore, Record.EnemyBefore,
        Record.ProtocolTier, Session, Error))
    {
        return false;
    }

    FAIHeroesGridReplay Prepared;
    Prepared.NodeId = Record.NodeId;
    Prepared.PlayerBefore = Record.PlayerBefore;
    Prepared.EnemyBefore = Record.EnemyBefore;
    Prepared.PlayerCells = Session.PlayerCells;
    Prepared.EnemyCells = Session.EnemyCells;
    Prepared.Summary = Record.Summary;

    bool bCompleted = false;
    for (int32 StepIndex = 0; StepIndex <= FAIHeroesGridBattle::MaxActions; ++StepIndex)
    {
        FAIHeroesGridAction Action;
        const EAIHeroesGridStepResult Result =
            FAIHeroesGridBattle::Step(Session, Action, Error);
        if (Result == EAIHeroesGridStepResult::Completed)
        {
            bCompleted = true;
            break;
        }
        if (Result != EAIHeroesGridStepResult::ActionProduced ||
            StepIndex >= FAIHeroesGridBattle::MaxActions)
        {
            Error = Error.IsEmpty() ? TEXT("Grid replay action limit exceeded.") : Error;
            return false;
        }
        if (!Record.Trace.IsValidIndex(StepIndex) ||
            Action.ToLogLine() != Record.Trace[StepIndex])
        {
            Error = TEXT("Grid replay action differs from verified transcript.");
            return false;
        }
        Prepared.Actions.Add(MoveTemp(Action));
    }

    if (!bCompleted || !Session.bCompleted ||
        Prepared.Actions.IsEmpty() ||
        Prepared.Actions.Num() != Record.Trace.Num())
    {
        Error = TEXT("Grid replay transcript did not end cleanly.");
        return false;
    }
    const FAIHeroesBattleOutcome Outcome = FAIHeroesGridBattle::GetOutcome(Session);
    if (Outcome.Trace != Record.Trace ||
        Outcome.Summary != Record.Summary ||
        Outcome.Rounds != Record.Rounds ||
        Outcome.bVictory != Record.bVictory ||
        Outcome.bDefeat != Record.bDefeat)
    {
        Error = TEXT("Grid replay final receipt differs from saved record.");
        return false;
    }

    OutReplay = MoveTemp(Prepared);
    return true;
}
