#include "AIHSimulation.h"

FAIHCombatReport FAIHCombatEngine::Resolve(
    const FAIHArmy& Army, int32 EnemyPower, int32 ProtocolRank)
{
    FAIHCombatReport Result;
    Result.EnemyPower = FMath::Max(0, EnemyPower);
    Result.Survivors = Army;

    const int32 Rank = FMath::Clamp(ProtocolRank, 0, 5);
    if (Army.Count() <= 0)
    {
        Result.Tactic = TEXT("Retreat");
        Result.Explanation = TEXT("No units available. The hero cannot attack.");
        return Result;
    }

    if (Rank >= 4 && Result.EnemyPower > Army.Strength())
    {
        Result.Tactic = TEXT("Guarded probe");
    }
    else if (Army.Guardians >= 2)
    {
        Result.Tactic = TEXT("Shield wall");
    }
    else if (Army.Scouts >= 3)
    {
        Result.Tactic = TEXT("Flanking maneuver");
    }
    else
    {
        Result.Tactic = TEXT("Careful advance");
    }

    const int32 FormationBonus = (Army.Scouts >= 3 ? 2 : 0) +
        (Army.Guardians >= 2 ? 3 : 0);
    Result.OurPower = Army.Strength() + FormationBonus + Rank * 2;
    Result.bVictory = Result.OurPower >= Result.EnemyPower;

    const int32 Divisor = Result.bVictory ? 10 : 6;
    const int32 RequestedLosses = Result.EnemyPower == 0 ? 0 :
        FMath::Max(1, Result.EnemyPower / Divisor);
    Result.Casualties = FMath::Min(Army.Count(), RequestedLosses);
    int32 LossesLeft = Result.Casualties;

    const int32 ScoutLosses = FMath::Min(Result.Survivors.Scouts, LossesLeft);
    Result.Survivors.Scouts -= ScoutLosses;
    LossesLeft -= ScoutLosses;
    const int32 SparkLosses = FMath::Min(Result.Survivors.Sparks, LossesLeft);
    Result.Survivors.Sparks -= SparkLosses;
    LossesLeft -= SparkLosses;
    Result.Survivors.Guardians = FMath::Max(0, Result.Survivors.Guardians - LossesLeft);

    Result.Explanation = FString::Printf(
        TEXT("%s: %s | power %d vs %d | lost %d units."),
        Result.bVictory ? TEXT("VICTORY") : TEXT("DEFEAT"),
        *Result.Tactic, Result.OurPower, Result.EnemyPower, Result.Casualties);
    return Result;
}
