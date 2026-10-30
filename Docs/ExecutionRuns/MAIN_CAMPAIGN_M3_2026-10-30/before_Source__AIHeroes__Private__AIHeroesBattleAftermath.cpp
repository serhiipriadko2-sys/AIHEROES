#include "AIHeroesBattleAftermath.h"

#include "AIHeroesBaseRules.h"
#include "AIHeroesBattleArt.h"
#include "AIHeroesBattleRecord.h"

namespace
{
bool CountLiving(const TArray<FAIHeroesArmyStack>& Army, int64& OutCount)
{
    int64 Count = 0;
    for (const FAIHeroesArmyStack& Stack : Army)
    {
        if (Stack.UnitId.IsNone() || Stack.UnitHealth <= 0 || Stack.TotalHealth < 0)
            return false;
        if (Stack.TotalHealth > 0)
            Count += (static_cast<int64>(Stack.TotalHealth) + Stack.UnitHealth - 1)
                / Stack.UnitHealth;
    }
    OutCount = Count;
    return true;
}

int64 Living(const FAIHeroesArmyStack& Stack)
{
    return Stack.TotalHealth > 0
        ? (static_cast<int64>(Stack.TotalHealth) + Stack.UnitHealth - 1)
            / Stack.UnitHealth
        : 0;
}

bool MatchesCommit(const FAIHeroesBattleRecord& Battle,
                   const FAIHeroesBattleCommitReceipt* Receipt)
{
    if (!Receipt || !Receipt->bApplied || Receipt->NodeId != Battle.NodeId ||
        Receipt->Day != Battle.Day || Receipt->RuleVersion != Battle.RuleVersion ||
        Receipt->MovesSpent < 0 || Receipt->MovesSpent > 4 ||
        Receipt->GoldGranted < 0 || Receipt->DataGranted < 0 ||
        Receipt->GoldGranted > FAIHeroesBaseRules::MaxResource ||
        Receipt->DataGranted > FAIHeroesBaseRules::MaxResource)
        return false;

    // Non-victories cannot have been awarded a successful-capture bonus.
    if (!Battle.bVictory &&
        (Receipt->GoldGranted > 0 || Receipt->DataGranted > 0))
        return false;
    return true;
}
}

FAIHeroesBattleAftermathView FAIHeroesBattleAftermath::Build(
    const FAIHeroesBattleRecord& CommittedRecord,
    const FAIHeroesBattleCommitReceipt* AppliedReward)
{
    FAIHeroesBattleAftermathView View;
    View.PlayerLine = TEXT("На карту — продолжить. Данные о потерях недоступны.");
    View.RewardLine = TEXT("Награды: нет подтверждённой записи о выдаче.");

    // Legacy/invalid records never produce casualties or invented rewards.
    if (CommittedRecord.NodeId.IsNone() || CommittedRecord.Day < 1 ||
        (CommittedRecord.RuleVersion != 1 && CommittedRecord.RuleVersion != 2) ||
        (CommittedRecord.bVictory && CommittedRecord.bDefeat) ||
        CommittedRecord.PlayerBefore.IsEmpty() ||
        CommittedRecord.PlayerBefore.Num() > 2 ||
        CommittedRecord.PlayerAfter.Num() > CommittedRecord.PlayerBefore.Num())
        return View;

    int64 Before = 0, After = 0;
    if (!CountLiving(CommittedRecord.PlayerBefore, Before) ||
        !CountLiving(CommittedRecord.PlayerAfter, After) ||
        Before == 0 || After > Before)
        return View;

    // Use the original stack INDEX, not UnitId, as the group identity.
    // Two same-name stacks must not collapse into one casualty line.
    TArray<FString> Groups;
    for (int32 Index = 0; Index < CommittedRecord.PlayerBefore.Num(); ++Index)
    {
        const FAIHeroesArmyStack& Original = CommittedRecord.PlayerBefore[Index];
        const FAIHeroesArmyStack* Final =
            CommittedRecord.PlayerAfter.IsValidIndex(Index)
                ? &CommittedRecord.PlayerAfter[Index] : nullptr;
        if (Final &&
            (Final->UnitId != Original.UnitId ||
             Final->UnitHealth != Original.UnitHealth ||
             Final->TotalHealth > Original.TotalHealth))
            return View;

        const int64 Start = Living(Original);
        const int64 End = Final ? Living(*Final) : 0;
        Groups.Add(FString::Printf(
            TEXT("A%d %s: %lld из %lld, потери %lld; ОЗ %d"),
            Index + 1, *FAIHeroesBattleArt::UnitName(Original.UnitId),
            static_cast<long long>(End), static_cast<long long>(Start),
            static_cast<long long>(Start - End),
            Final ? Final->TotalHealth : 0));
    }

    View.bCountsAvailable = true;
    View.PlayerGroups = MoveTemp(Groups);
    View.PlayerLine = FString::Printf(
        TEXT("На карту — продолжить. Бойцов: %lld из %lld; потери: %lld."),
        static_cast<long long>(After),
        static_cast<long long>(Before),
        static_cast<long long>(Before - After));

    // Only the CURRENT session's one-time committed plan may supply rewards.
    // Historic SaveGame records have no receipt, and keep the fallback text.
    if (MatchesCommit(CommittedRecord, AppliedReward))
    {
        View.bRewardsVerified = true;
        View.RewardLine = FString::Printf(
            TEXT("Получено: +%d золота, +%d данных  |  Потрачено ходов: %d"),
            AppliedReward->GoldGranted,
            AppliedReward->DataGranted,
            AppliedReward->MovesSpent);
    }
    return View;
}
