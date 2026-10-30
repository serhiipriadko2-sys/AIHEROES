#include "AIHeroesExplorerCopy.h"

namespace AIHeroesExplorerCopyDetail
{
const TCHAR* KnownSite(const FString& Id)
{
    if (Id == TEXT("HQ")) return TEXT("Цитадель");
    if (Id == TEXT("FOREST")) return TEXT("Лесопилка");
    if (Id == TEXT("ORE")) return TEXT("Рудник");
    if (Id == TEXT("OUTPOST")) return TEXT("Сторожевая башня");
    if (Id == TEXT("RUINS")) return TEXT("Древний архив");
    return nullptr;
}
}

FString FAIHeroesExplorerCopy::DockLabel(
    int32 Action, float ButtonWidth, const FString& FullLabel)
{
    if (!FMath::IsFinite(ButtonWidth) || ButtonWidth <= 0)
        return FullLabel;

    // Four primary actions retain the same IDs and hit rectangles. Short
    // labels only remove PC hotkey hints on narrow touch-friendly panels.
    if (ButtonWidth >= 168.f) return FullLabel;
    switch (Action)
    {
        case 400: return TEXT("База");
        case 203: return TEXT("Тактики");
        case 1: return TEXT("Совет");
        case 2: return TEXT("Ход");
        default: return FullLabel;
    }
}

FString FAIHeroesExplorerCopy::EventText(
    const FString& EngineEvent, float ViewWidth)
{
    const bool Compact = FMath::IsFinite(ViewWidth) && ViewWidth < 900.f;
    if (EngineEvent == TEXT(
        "Select a node, ASK HERO (H), then CONFIRM (Enter). Battles are AI-controlled."))
    {
        return Compact
            ? TEXT("Выберите цель · Совет · Подтвердить")
            : TEXT("Выберите цель, спросите героя, затем подтвердите поход. ИИ ведёт бой.");
    }
    if (EngineEvent == TEXT(
        "No living army. Recruit units before confirming an attack."))
    {
        // Exact GameMode denial only. Unknown engine errors still pass
        // through unchanged; this cannot enable an illegal attack.
        return Compact
            ? TEXT("Нет живых бойцов · найм на базе [G]")
            : TEXT("Атака отменена: нет живых бойцов. Наймите армию на базе [G].");
    }

    // Only exact, publicly-known destination IDs and the authoritative
    // 0..4 movement budget are eligible for a localized arrival toast.
    constexpr const TCHAR* ArrivalPrefix = TEXT("Hero moved to ");
    const FString ArrivalTail(TEXT(". Remaining moves: "));
    if (EngineEvent.StartsWith(ArrivalPrefix, ESearchCase::CaseSensitive) &&
        EngineEvent.EndsWith(TEXT("."), ESearchCase::CaseSensitive))
    {
        const FString Tail = EngineEvent.Mid(FCString::Strlen(ArrivalPrefix));
        FString Id, Remaining;
        if (Tail.Split(ArrivalTail, &Id, &Remaining, ESearchCase::CaseSensitive) &&
            Remaining.Len() == 2 && Remaining[1] == TCHAR('.') &&
            Remaining[0] >= TCHAR('0') && Remaining[0] <= TCHAR('4'))
        {
            if (const TCHAR* Site = AIHeroesExplorerCopyDetail::KnownSite(Id))
            {
                const int32 Moves = Remaining[0] - TCHAR('0');
                return Compact
                    ? FString::Printf(TEXT("%s · герой прибыл · ходы %d/4"), Site, Moves)
                    : FString::Printf(TEXT("Герой прибыл: %s. Осталось ходов: %d из 4."), Site, Moves);
            }
        }
    }

    constexpr const TCHAR* SelectedSuffix =
        TEXT(" selected. Ask H for advice; Enter to move/attack.");
    const FString Suffix(SelectedSuffix);
    if (EngineEvent.EndsWith(Suffix, ESearchCase::CaseSensitive))
    {
        const FString NodeId = EngineEvent.Left(
            EngineEvent.Len() - Suffix.Len());
        if (const TCHAR* Site = AIHeroesExplorerCopyDetail::KnownSite(NodeId))
        {
            return Compact
                ? FString::Printf(TEXT("%s выбрана · Совет → В путь"), Site)
                : FString::Printf(
                    TEXT("%s выбрана. Спросите героя, затем подтвердите поход."), Site);
        }
    }
    // Unknown error/diagnostic text passes through unchanged: no fabricated
    // translation, no claims about rewards, opponent guards or move costs.
    return EngineEvent;
}
