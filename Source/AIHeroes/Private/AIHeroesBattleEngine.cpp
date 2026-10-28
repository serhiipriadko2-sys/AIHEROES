#include "AIHeroesBattleEngine.h"

FString FAIHeroesBattleAction::ToLogLine() const
{
    return FString::Printf(TEXT("R%d %s %s -> %s (-%d HP)"),
        Round, bPlayerAttacker ? TEXT("HERO") : TEXT("NPC"),
        *AttackerId.ToString(), *TargetId.ToString(), Damage);
}

namespace
{
struct FAIHeroesTurn
{
    bool bPlayer = false;
    int32 Index = -1;
    int32 Initiative = 0;
};

int32 CountLivingUnits(const TArray<FAIHeroesArmyStack>& Army)
{
    int32 Count = 0;
    for (const FAIHeroesArmyStack& Stack : Army)
    {
        Count += Stack.Count();
    }
    return Count;
}
}

bool FAIHeroesBattleEngine::AnyAlive(const TArray<FAIHeroesArmyStack>& Army)
{
    for (const FAIHeroesArmyStack& Stack : Army)
    {
        if (Stack.IsAlive())
        {
            return true;
        }
    }
    return false;
}

int32 FAIHeroesBattleEngine::ChooseTarget(
    const FAIHeroesArmyStack& ActingStack,
    const TArray<FAIHeroesArmyStack>& Opponents,
    int32 StrategyTier)
{
    int32 BestIndex = INDEX_NONE;
    int32 BestScore = MIN_int32;

    for (int32 Index = 0; Index < Opponents.Num(); ++Index)
    {
        const FAIHeroesArmyStack& Target = Opponents[Index];
        if (!Target.IsAlive())
        {
            continue;
        }

        // Tier 0: untrained AI engages the first surviving enemy.
        if (StrategyTier <= 0)
        {
            return Index;
        }

        const int32 Threat = Target.Count() * Target.Attack;
        int32 Score = Threat;

        if (StrategyTier >= 2)
        {
            // Reduce enemy action economy by finishing damaged stacks.
            Score = Threat * 2 - Target.TotalHealth;
        }

        if (StrategyTier >= 3)
        {
            const int32 ExpectedDamage =
                ActingStack.Count() * FMath::Max(1, ActingStack.Attack - Target.Defense / 2);
            Score = Threat * 3 - Target.TotalHealth / 2;
            if (ExpectedDamage >= Target.TotalHealth)
            {
                Score += 1000;
            }
        }

        if (Score > BestScore)
        {
            BestScore = Score;
            BestIndex = Index;
        }
    }

    return BestIndex;
}

FAIHeroesBattleOutcome FAIHeroesBattleEngine::Resolve(
    const TArray<FAIHeroesArmyStack>& PlayerArmy,
    const TArray<FAIHeroesArmyStack>& EnemyArmy,
    int32 ProtocolTier)
{
    FAIHeroesBattleOutcome Result;
    Result.PlayerAfter = PlayerArmy;
    Result.EnemyAfter = EnemyArmy;

    for (int32 Round = 1; Round <= 30 && AnyAlive(Result.PlayerAfter) && AnyAlive(Result.EnemyAfter); ++Round)
    {
        Result.Rounds = Round;
        TArray<FAIHeroesTurn> Order;

        for (int32 Index = 0; Index < Result.PlayerAfter.Num(); ++Index)
        {
            if (Result.PlayerAfter[Index].IsAlive())
            {
                Order.Add({true, Index, Result.PlayerAfter[Index].Initiative});
            }
        }
        for (int32 Index = 0; Index < Result.EnemyAfter.Num(); ++Index)
        {
            if (Result.EnemyAfter[Index].IsAlive())
            {
                Order.Add({false, Index, Result.EnemyAfter[Index].Initiative});
            }
        }

        Order.Sort([](const FAIHeroesTurn& A, const FAIHeroesTurn& B)
        {
            if (A.Initiative != B.Initiative)
            {
                return A.Initiative > B.Initiative;
            }
            if (A.bPlayer != B.bPlayer)
            {
                return A.bPlayer; // Stable tie-break: player first.
            }
            return A.Index < B.Index;
        });

        for (const FAIHeroesTurn& Action : Order)
        {
            TArray<FAIHeroesArmyStack>& Allies = Action.bPlayer ? Result.PlayerAfter : Result.EnemyAfter;
            TArray<FAIHeroesArmyStack>& Opponents = Action.bPlayer ? Result.EnemyAfter : Result.PlayerAfter;
            if (!Allies.IsValidIndex(Action.Index) || !Allies[Action.Index].IsAlive() || !AnyAlive(Opponents))
            {
                continue;
            }

            const FAIHeroesArmyStack& Attacker = Allies[Action.Index];
            const int32 TargetIndex = ChooseTarget(Attacker, Opponents, Action.bPlayer ? ProtocolTier : 1);
            if (!Opponents.IsValidIndex(TargetIndex))
            {
                continue;
            }

            FAIHeroesArmyStack& Defender = Opponents[TargetIndex];
            const int32 Damage = Attacker.Count() * FMath::Max(1, Attacker.Attack - Defender.Defense / 2);
            const int32 Before = Defender.TotalHealth;
            Defender.TotalHealth = FMath::Max(0, Before - Damage);

            FAIHeroesBattleAction Receipt;
            Receipt.Round = Round;
            Receipt.bPlayerAttacker = Action.bPlayer;
            Receipt.AttackerStackIndex = Action.Index;
            Receipt.TargetStackIndex = TargetIndex;
            Receipt.AttackerId = Attacker.UnitId;
            Receipt.TargetId = Defender.UnitId;
            Receipt.Damage = Damage;
            Receipt.TargetHealthBefore = Before;
            Receipt.TargetHealthAfter = Defender.TotalHealth;
            Result.Trace.Add(Receipt.ToLogLine());
            Result.Actions.Add(MoveTemp(Receipt));
        }
    }

    Result.bVictory = AnyAlive(Result.PlayerAfter) && !AnyAlive(Result.EnemyAfter);
    Result.bDefeat = !AnyAlive(Result.PlayerAfter);
    Result.Summary = FString::Printf(
        TEXT("%s | %d rounds | your units: %d | enemy units: %d"),
        Result.bVictory ? TEXT("VICTORY") : (Result.bDefeat ? TEXT("DEFEAT") : TEXT("STALEMATE")),
        Result.Rounds,
        CountLivingUnits(Result.PlayerAfter),
        CountLivingUnits(Result.EnemyAfter));

    return Result;
}

bool FAIHeroesBattleEngine::VerifyReplay(
    const TArray<FAIHeroesArmyStack>& PlayerInitial,
    const TArray<FAIHeroesArmyStack>& EnemyInitial,
    const FAIHeroesBattleOutcome& Outcome)
{
    if (Outcome.Rounds < 0 || Outcome.Rounds > 30 ||
        Outcome.Actions.Num() != Outcome.Trace.Num() ||
        Outcome.Actions.Num() > 400 ||
        Outcome.PlayerAfter.Num() != PlayerInitial.Num() ||
        Outcome.EnemyAfter.Num() != EnemyInitial.Num())
    {
        return false;
    }

    TArray<FAIHeroesArmyStack> Player = PlayerInitial;
    TArray<FAIHeroesArmyStack> Enemy = EnemyInitial;
    int32 PreviousRound = 0;
    for (int32 Step = 0; Step < Outcome.Actions.Num(); ++Step)
    {
        const FAIHeroesBattleAction& Action = Outcome.Actions[Step];
        if (Action.Round <= 0 || Action.Round > Outcome.Rounds ||
            Action.Round < PreviousRound || Action.Damage <= 0 ||
            Action.ToLogLine() != Outcome.Trace[Step])
        {
            return false;
        }
        PreviousRound = Action.Round;

        const TArray<FAIHeroesArmyStack>& Attackers =
            Action.bPlayerAttacker ? Player : Enemy;
        TArray<FAIHeroesArmyStack>& Targets =
            Action.bPlayerAttacker ? Enemy : Player;

        if (!Attackers.IsValidIndex(Action.AttackerStackIndex) ||
            !Targets.IsValidIndex(Action.TargetStackIndex))
        {
            return false;
        }

        const FAIHeroesArmyStack& Attacker = Attackers[Action.AttackerStackIndex];
        FAIHeroesArmyStack& Target = Targets[Action.TargetStackIndex];
        if (!Attacker.IsAlive() || !Target.IsAlive() ||
            Attacker.UnitId != Action.AttackerId ||
            Target.UnitId != Action.TargetId ||
            Target.TotalHealth != Action.TargetHealthBefore)
        {
            return false;
        }

        const int32 ExpectedDamage = Attacker.Count() *
            FMath::Max(1, Attacker.Attack - Target.Defense / 2);
        const int32 ExpectedAfter = FMath::Max(0, Target.TotalHealth - ExpectedDamage);
        if (Action.Damage != ExpectedDamage ||
            Action.TargetHealthAfter != ExpectedAfter)
        {
            return false;
        }
        Target.TotalHealth = ExpectedAfter;
    }

    // A forged terminal verdict or misleading round count must never
    // pass merely because individual attack HP changes appear valid.
    const bool ExpectedVictory = AnyAlive(Player) && !AnyAlive(Enemy);
    const bool ExpectedDefeat = !AnyAlive(Player);
    if (Outcome.Rounds != PreviousRound ||
        Outcome.bVictory != ExpectedVictory ||
        Outcome.bDefeat != ExpectedDefeat)
    {
        return false;
    }
    const FString ExpectedSummary = FString::Printf(
        TEXT("%s | %d rounds | your units: %d | enemy units: %d"),
        ExpectedVictory ? TEXT("VICTORY") :
            (ExpectedDefeat ? TEXT("DEFEAT") : TEXT("STALEMATE")),
        PreviousRound, CountLivingUnits(Player), CountLivingUnits(Enemy));
    if (Outcome.Summary != ExpectedSummary)
    {
        return false;
    }

    auto IdenticalArmies = [](const TArray<FAIHeroesArmyStack>& Reconstructed,
                            const TArray<FAIHeroesArmyStack>& Reported)
    {
        if (Reconstructed.Num() != Reported.Num())
        {
            return false;
        }
        for (int32 Index = 0; Index < Reconstructed.Num(); ++Index)
        {
            const FAIHeroesArmyStack& A = Reconstructed[Index];
            const FAIHeroesArmyStack& B = Reported[Index];
            if (A.UnitId != B.UnitId ||
                A.UnitHealth != B.UnitHealth ||
                A.TotalHealth != B.TotalHealth ||
                A.Attack != B.Attack ||
                A.Defense != B.Defense ||
                A.Initiative != B.Initiative)
            {
                return false;
            }
        }
        return true;
    };

    return IdenticalArmies(Player, Outcome.PlayerAfter) &&
        IdenticalArmies(Enemy, Outcome.EnemyAfter);
}
