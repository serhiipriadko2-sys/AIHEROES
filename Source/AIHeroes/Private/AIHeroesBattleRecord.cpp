#include "AIHeroesBattleRecord.h"

#include "AIHeroesBattleEngine.h"
#include "AIHeroesGridBattle.h"

namespace
{
bool EqualArmy(const TArray<FAIHeroesArmyStack>& Left, const TArray<FAIHeroesArmyStack>& Right)
{
    if (Left.Num() != Right.Num())
    {
        return false;
    }
    for (int32 Index = 0; Index < Left.Num(); ++Index)
    {
        const FAIHeroesArmyStack& A = Left[Index];
        const FAIHeroesArmyStack& B = Right[Index];
        if (A.UnitId != B.UnitId ||
            A.TotalHealth != B.TotalHealth ||
            A.UnitHealth != B.UnitHealth ||
            A.Attack != B.Attack ||
            A.Defense != B.Defense ||
            A.Initiative != B.Initiative)
        {
            return false;
        }
    }
    return true;
}
}

FAIHeroesBattleRecord FAIHeroesBattleRecordRules::Capture(
    FName NodeId, int32 Day, int32 ProtocolTier,
    const TArray<FAIHeroesArmyStack>& PlayerBefore,
    const TArray<FAIHeroesArmyStack>& EnemyBefore,
    const FAIHeroesBattleOutcome& Outcome, int32 RuleVersion)
{
    FAIHeroesBattleRecord Record;
    Record.NodeId = NodeId;
    Record.Day = Day;
    Record.ProtocolTier = ProtocolTier;
    Record.RuleVersion = RuleVersion;
    Record.PlayerBefore = PlayerBefore;
    Record.EnemyBefore = EnemyBefore;
    Record.PlayerAfter = Outcome.PlayerAfter;
    Record.EnemyAfter = Outcome.EnemyAfter;
    Record.Trace = Outcome.Trace;
    Record.Summary = Outcome.Summary;
    Record.Rounds = Outcome.Rounds;
    Record.bVictory = Outcome.bVictory;
    Record.bDefeat = Outcome.bDefeat;
    return Record;
}

bool FAIHeroesBattleRecordRules::Verify(const FAIHeroesBattleRecord& Record, FString& Error)
{
    Error.Empty();
    if (Record.RuleVersion != 1 && Record.RuleVersion != 2)
    {
        Error = TEXT("Unsupported battle rule version.");
        return false;
    }
    if (Record.NodeId.IsNone())
    {
        if (Record.RuleVersion != 1 || Record.Day != 0 || Record.ProtocolTier != 0 || Record.Rounds != 0 ||
            Record.bVictory || Record.bDefeat || !Record.PlayerBefore.IsEmpty() ||
            !Record.EnemyBefore.IsEmpty() || !Record.PlayerAfter.IsEmpty() ||
            !Record.EnemyAfter.IsEmpty() || !Record.Trace.IsEmpty() ||
            !Record.Summary.IsEmpty())
        {
            Error = TEXT("Unidentified replay contains unexpected data.");
            return false;
        }
        return true;
    }
    const int32 MaximumEnemyStacks = Record.RuleVersion == 2 ? 2 : 1;
    const int32 MaximumTraceEvents = Record.RuleVersion == 2 ? 256 : 128;
    if (Record.Day < 1 || Record.Day > 1000000 ||
        Record.ProtocolTier < 0 || Record.ProtocolTier > 3 ||
        Record.PlayerBefore.Num() > 2 || Record.EnemyBefore.Num() > MaximumEnemyStacks ||
        Record.Trace.IsEmpty() || Record.Trace.Num() > MaximumTraceEvents ||
        Record.Summary.Len() > 300)
    {
        Error = TEXT("Replay exceeds limits or has invalid metadata.");
        return false;
    }
    for (const FString& Step : Record.Trace)
    {
        if (Step.IsEmpty() || Step.Len() > 200)
        {
            Error = TEXT("Invalid replay step.");
            return false;
        }
    }

    // Checking a proposed transcript never changes strategic game state.
    // The transcript is evidence, never a parser input or a campaign command.
    // Preserve the old resolver so pre-grid records keep their original rules.
    const FAIHeroesBattleOutcome Recomputed = Record.RuleVersion == 2
        ? FAIHeroesGridBattle::RunToEnd(
            Record.PlayerBefore, Record.EnemyBefore, Record.ProtocolTier)
        : FAIHeroesBattleEngine::Resolve(
            Record.PlayerBefore, Record.EnemyBefore, Record.ProtocolTier);
    if (Record.Rounds != Recomputed.Rounds ||
        Record.bVictory != Recomputed.bVictory ||
        Record.bDefeat != Recomputed.bDefeat ||
        Record.Summary != Recomputed.Summary ||
        Record.Trace != Recomputed.Trace ||
        !EqualArmy(Record.PlayerAfter, Recomputed.PlayerAfter) ||
        !EqualArmy(Record.EnemyAfter, Recomputed.EnemyAfter))
    {
        Error = TEXT("Replay does not match a deterministic battle.");
        return false;
    }
    return true;
}

bool FAIHeroesBattleRecordRules::Equivalent(
    const FAIHeroesBattleRecord& A, const FAIHeroesBattleRecord& B)
{
    return A.NodeId == B.NodeId && A.Day == B.Day &&
        A.ProtocolTier == B.ProtocolTier && A.RuleVersion == B.RuleVersion &&
        EqualArmy(A.PlayerBefore, B.PlayerBefore) &&
        EqualArmy(A.EnemyBefore, B.EnemyBefore) &&
        EqualArmy(A.PlayerAfter, B.PlayerAfter) &&
        EqualArmy(A.EnemyAfter, B.EnemyAfter) &&
        A.Trace == B.Trace && A.Summary == B.Summary &&
        A.Rounds == B.Rounds && A.bVictory == B.bVictory &&
        A.bDefeat == B.bDefeat;
}

FString FAIHeroesBattleRecordRules::DescribeStep(
    const FAIHeroesBattleRecord& Record, int32 Index)
{
    if (!Record.Trace.IsValidIndex(Index))
    {
        return TEXT("REPLAY: no battle step at this position.");
    }
    return FString::Printf(TEXT("REPLAY %s %d/%d: %s"),
        *Record.NodeId.ToString(), Index + 1, Record.Trace.Num(),
        *Record.Trace[Index]);
}
