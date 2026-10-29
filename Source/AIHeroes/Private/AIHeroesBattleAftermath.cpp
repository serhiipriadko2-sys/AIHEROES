#include "AIHeroesBattleAftermath.h"

#include "AIHeroesBattleRecord.h"

namespace
{
bool CountLiving(const TArray<FAIHeroesArmyStack>& Army, int64& OutCount)
{
    int64 Count = 0;
    for (const FAIHeroesArmyStack& Stack : Army)
    {
        if (Stack.UnitId.IsNone() || Stack.UnitHealth <= 0 || Stack.TotalHealth < 0)
        {
            return false;
        }
        if (Stack.TotalHealth > 0)
        {
            // Use 64-bit arithmetic: TotalHealth + UnitHealth - 1 can overflow int32.
            Count += (static_cast<int64>(Stack.TotalHealth) + Stack.UnitHealth - 1)
                / Stack.UnitHealth;
        }
    }
    OutCount = Count;
    return true;
}
}

FAIHeroesBattleAftermathView FAIHeroesBattleAftermath::Build(
    const FAIHeroesBattleRecord& CommittedRecord)
{
    FAIHeroesBattleAftermathView View;
    View.PlayerLine = TEXT("На карту — продолжить. Данные о потерях недоступны.");

    // This is a read-only view, never an attempt to infer campaign rewards.
    // The caller must only show it after the live battle has been committed.
    if (CommittedRecord.NodeId.IsNone() ||
        CommittedRecord.Day < 1 ||
        (CommittedRecord.RuleVersion != 1 && CommittedRecord.RuleVersion != 2) ||
        (CommittedRecord.bVictory && CommittedRecord.bDefeat) ||
        CommittedRecord.PlayerBefore.IsEmpty())
    {
        return View;
    }

    int64 Before = 0;
    int64 After = 0;
    if (!CountLiving(CommittedRecord.PlayerBefore, Before) ||
        !CountLiving(CommittedRecord.PlayerAfter, After) ||
        Before == 0 || After > Before)
    {
        // Missing/corrupt historical records must not create invented casualties.
        return View;
    }

    View.bCountsAvailable = true;
    View.PlayerLine = FString::Printf(
        TEXT("На карту — продолжить. Бойцов: %lld из %lld; потери: %lld."),
        static_cast<long long>(After),
        static_cast<long long>(Before),
        static_cast<long long>(Before - After));
    return View;
}
