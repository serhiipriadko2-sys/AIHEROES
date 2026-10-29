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
