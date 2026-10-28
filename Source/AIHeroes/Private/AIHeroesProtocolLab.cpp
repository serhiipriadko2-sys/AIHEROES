#include "AIHeroesProtocolLab.h"

#include "AIHeroesBattleEngine.h"
#include "AIHeroesGridBattle.h"

namespace
{
int64 SumLivingHealth(const TArray<FAIHeroesArmyStack>& Army)
{
    int64 Total = 0;
    for (const FAIHeroesArmyStack& Stack : Army)
    {
        Total += FMath::Max(0, Stack.TotalHealth);
    }
    return Total;
}

int32 FirstDifferentAction(
    const TArray<FString>& Actual, const TArray<FString>& Alternative)
{
    const int32 Shared = FMath::Min(Actual.Num(), Alternative.Num());
    for (int32 Index = 0; Index < Shared; ++Index)
    {
        if (Actual[Index] != Alternative[Index])
        {
            return Index + 1;
        }
    }
    return Actual.Num() == Alternative.Num() ? 0 : Shared + 1;
}

const TCHAR* BattleResult(bool bVictory, bool bDefeat)
{
    if (bVictory) return TEXT("WIN");
    if (bDefeat) return TEXT("LOSS");
    return TEXT("DRAW");
}
}

bool FAIHeroesProtocolLabRules::Analyze(
    const FAIHeroesBattleRecord& Record,
    FAIHeroesProtocolLabReport& OutReport, FString& Error)
{
    OutReport = FAIHeroesProtocolLabReport();
    Error.Empty();

    if (Record.NodeId.IsNone())
    {
        Error = TEXT("No recorded battle. Finish a guarded encounter first.");
        return false;
    }
    if (!FAIHeroesBattleRecordRules::Verify(Record, Error))
    {
        // The saved battle transcript is an input, never a command source.
        return false;
    }
    if (Record.RuleVersion != 1 &&
        Record.RuleVersion != FAIHeroesGridBattle::RuleVersion)
    {
        Error = TEXT("Unsupported historical battle rules.");
        return false;
    }

    FAIHeroesProtocolLabReport Prepared;
    Prepared.NodeId = Record.NodeId;
    Prepared.Day = Record.Day;
    Prepared.RuleVersion = Record.RuleVersion;
    Prepared.ActualProtocolTier = Record.ProtocolTier;
    const int64 HistoricalHealth = SumLivingHealth(Record.PlayerAfter);

    // Four tightly bounded pure replays. They have no game-world, network,
    // SaveGame, input or filesystem authority.
    for (int32 Tier = 0; Tier < ProtocolVariants; ++Tier)
    {
        const FAIHeroesBattleOutcome Outcome =
            Record.RuleVersion == FAIHeroesGridBattle::RuleVersion
            ? FAIHeroesGridBattle::RunToEnd(
                Record.PlayerBefore, Record.EnemyBefore, Tier)
            : FAIHeroesBattleEngine::Resolve(
                Record.PlayerBefore, Record.EnemyBefore, Tier);

        if (Outcome.Trace.IsEmpty() ||
            Outcome.Trace.Num() > (Record.RuleVersion == 2 ? 256 : 128))
        {
            Error = TEXT("A counterfactual replay exceeded its action bound.");
            return false;
        }
        if (Tier == Record.ProtocolTier &&
            (Outcome.Trace != Record.Trace ||
                Outcome.Summary != Record.Summary ||
                Outcome.Rounds != Record.Rounds ||
                Outcome.bVictory != Record.bVictory ||
                Outcome.bDefeat != Record.bDefeat))
        {
            Error = TEXT("Counterfactual baseline differs from verified history.");
            return false;
        }

        FAIHeroesProtocolLabScenario Scenario;
        Scenario.ProtocolTier = Tier;
        Scenario.bActual = Tier == Record.ProtocolTier;
        Scenario.bVictory = Outcome.bVictory;
        Scenario.bDefeat = Outcome.bDefeat;
        Scenario.Rounds = Outcome.Rounds;
        Scenario.TraceActions = Outcome.Trace.Num();
        Scenario.HeroHealth = SumLivingHealth(Outcome.PlayerAfter);
        Scenario.EnemyHealth = SumLivingHealth(Outcome.EnemyAfter);
        Scenario.HeroHpDelta = Scenario.HeroHealth - HistoricalHealth;
        Scenario.FirstDifferentAction =
            FirstDifferentAction(Record.Trace, Outcome.Trace);
        Prepared.Scenarios.Add(Scenario);
    }

    OutReport = MoveTemp(Prepared);
    return true;
}

TArray<FString> FAIHeroesProtocolLabReport::Describe() const
{
    TArray<FString> Lines;
    if (!IsValid()) return Lines;

    Lines.Add(FString::Printf(
        TEXT("PROTOCOL LAB | %s | DAY %d | rules V%d | actual protocol tier %d"),
        *NodeId.ToString(), Day, RuleVersion, ActualProtocolTier));
    Lines.Add(TEXT("EVIDENCE: Verified saved battle. All four runs use the SAME starting armies, node and battle rules."));
    Lines.Add(TEXT("MEANING: A counterfactual measures different rule decisions; it is NOT a prediction or a real neural-network benchmark."));

    for (const FAIHeroesProtocolLabScenario& Scenario : Scenarios)
    {
        const FString ChangedAt = Scenario.FirstDifferentAction == 0
            ? TEXT("same trace") :
            FString::Printf(TEXT("first different action #%d"),
                Scenario.FirstDifferentAction);
        Lines.Add(FString::Printf(
            TEXT("%s TIER %d: %s | hero HP %s (%s%s) | enemy HP %s | rounds %d | actions %d | %s"),
            Scenario.bActual ? TEXT("ACTUAL") : TEXT("WHAT-IF"),
            Scenario.ProtocolTier,
            BattleResult(Scenario.bVictory, Scenario.bDefeat),
            *LexToString(Scenario.HeroHealth),
            Scenario.HeroHpDelta >= 0 ? TEXT("+") : TEXT(""),
            *LexToString(Scenario.HeroHpDelta),
            *LexToString(Scenario.EnemyHealth),
            Scenario.Rounds, Scenario.TraceActions,
            *ChangedAt));
    }

    Lines.Add(TEXT("INTERPRETATION: More protocols do not guarantee higher HP or victory. The four rows are measurements, not a ranking of intelligence."));
    Lines.Add(TEXT("AUTHORITY: Read-only laboratory. No move, attack, recruitment, upgrade, save or battle result was executed."));
    Lines.Add(TEXT("MODEL BOUNDARY: 1B/3B/8B are gameplay slots; no actual LLM checkpoint is loaded here."));
    return Lines;
}
