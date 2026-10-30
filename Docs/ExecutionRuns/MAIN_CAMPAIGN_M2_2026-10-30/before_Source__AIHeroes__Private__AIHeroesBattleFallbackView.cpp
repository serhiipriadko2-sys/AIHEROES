#include "AIHeroesBattleFallbackView.h"

namespace
{
TArray<FString> LocalizedRoster(const FString& Source)
{
    TArray<FString> Raw;
    Source.ParseIntoArray(Raw, TEXT(" | "), true);

    TArray<FString> Result;
    for (const FString& Part : Raw)
    {
        FString Line = Part.TrimStartAndEnd();
        if (Line.IsEmpty()) continue;
        // The GameMode owns the combat-state string; this is a display copy.
        Line.ReplaceInline(TEXT(" units / "), TEXT(" бойцов / "));
        Line.ReplaceInline(TEXT(" HP / INIT "), TEXT(" ОЗ / инициатива "));
        Result.Add(Line.Left(200));
        if (Result.Num() >= 2) break; // Grid V2 has at most two stacks per side.
    }
    if (Result.IsEmpty())
        Result.Add(TEXT("Сведения об отрядах пока недоступны."));
    return Result;
}
}

FAIHeroesBattleFallbackView FAIHeroesBattleFallbackView::Build(
    bool bSceneAvailable, bool bResult, bool bPaused,
    int32 CommittedActions,
    const FString& PlayerStatus, const FString& EnemyStatus)
{
    FAIHeroesBattleFallbackView View;
    if (bSceneAvailable)
        return View;

    View.bVisible = true;
    View.ActionLabel = FString::Printf(
        TEXT("Действий: %d"), FMath::Max(0, CommittedActions));

    if (bResult)
        View.Status = TEXT("Бой завершён. Итог применён, вернитесь на карту.");
    else if (bPaused)
        View.Status = TEXT("3D-сцена недоступна. Пауза: герой ждёт продолжения.");
    else
        View.Status = TEXT("3D-сцена недоступна. Герой продолжает бой по правилам V2.");

    View.PlayerRoster = LocalizedRoster(PlayerStatus);
    View.EnemyRoster = LocalizedRoster(EnemyStatus);
    return View;
}
