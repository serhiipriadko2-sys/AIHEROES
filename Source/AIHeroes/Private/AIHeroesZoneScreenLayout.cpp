#include "AIHeroesZoneScreenLayout.h"

const TCHAR* FAIHeroesZoneScreenLayout::Name(EAIHeroesZoneScreenAction Action)
{
    switch (Action)
    {
        case EAIHeroesZoneScreenAction::NextSite: return TEXT("NEXT");
        case EAIHeroesZoneScreenAction::HeroAdvice: return TEXT("HERO");
        case EAIHeroesZoneScreenAction::InspectSite: return TEXT("INSPECT");
        case EAIHeroesZoneScreenAction::CompareRoutes: return TEXT("ROUTES");
        case EAIHeroesZoneScreenAction::ConfirmMove: return TEXT("MOVE / FIGHT");
        case EAIHeroesZoneScreenAction::EndDay: return TEXT("NEXT DAY");
        case EAIHeroesZoneScreenAction::Recruit: return TEXT("RECRUIT");
        case EAIHeroesZoneScreenAction::UpgradeProtocol: return TEXT("PROTOCOL");
        case EAIHeroesZoneScreenAction::Save: return TEXT("SAVE");
        case EAIHeroesZoneScreenAction::Load: return TEXT("LOAD");
        case EAIHeroesZoneScreenAction::CloseComparison: return TEXT("CLOSE ROUTES");
        default: return TEXT("NONE");
    }
}

bool FAIHeroesZoneScreenLayout::ContainsChrome(const FVector2D& Point) const
{
    if (!bSupported || !bWideChrome ||
        !FMath::IsFinite(Point.X) || !FMath::IsFinite(Point.Y)) return false;
    return MissionPanel.Contains(Point) || InspectorPanel.Contains(Point) ||
        MiniMapPanel.Contains(Point);
}

const TCHAR* FAIHeroesZoneScreenLayout::DisplayName(EAIHeroesZoneScreenAction Action)
{
    switch (Action)
    {
        case EAIHeroesZoneScreenAction::NextSite: return TEXT("СЛЕД. МЕСТО");
        case EAIHeroesZoneScreenAction::HeroAdvice: return TEXT("СОВЕТ ГЕРОЯ");
        case EAIHeroesZoneScreenAction::InspectSite: return TEXT("ОБЪЕКТ");
        case EAIHeroesZoneScreenAction::CompareRoutes: return TEXT("МАРШРУТЫ");
        case EAIHeroesZoneScreenAction::ConfirmMove: return TEXT("ИДТИ / БОЙ");
        case EAIHeroesZoneScreenAction::EndDay: return TEXT("КОНЕЦ ДНЯ");
        case EAIHeroesZoneScreenAction::Recruit: return TEXT("НАЁМ");
        case EAIHeroesZoneScreenAction::UpgradeProtocol: return TEXT("РАЗВИТИЕ ИИ");
        case EAIHeroesZoneScreenAction::Save: return TEXT("СОХРАНИТЬ");
        case EAIHeroesZoneScreenAction::Load: return TEXT("ЗАГРУЗИТЬ");
        case EAIHeroesZoneScreenAction::CloseComparison: return TEXT("ЗАКРЫТЬ");
        default: return TEXT("");
    }
}

FAIHeroesZoneScreenLayout FAIHeroesZoneScreenLayout::Build(
    const FVector2D& Viewport, bool bComparisonOpen)
{
    FAIHeroesZoneScreenLayout Result;
    Result.Viewport = Viewport;
    Result.bComparisonOpen = bComparisonOpen;
    // Too-small viewports fail closed, not into 10 overlapping hit areas.
    if (!FMath::IsFinite(Viewport.X) || !FMath::IsFinite(Viewport.Y) ||
        Viewport.X < 640.f || Viewport.Y < 520.f ||
        Viewport.X > 10000.f || Viewport.Y > 10000.f)
        return Result;
    Result.bSupported = true;
    Result.bWideChrome = Viewport.X >= 1200.f && Viewport.Y >= 700.f;
    if (Result.bWideChrome)
    {
        Result.MissionPanel = {18.f, 145.f, 246.f, 144.f};
        Result.MiniMapPanel = {18.f, 302.f, 210.f, 158.f};
        Result.InspectorPanel = {static_cast<float>(Viewport.X) - 280.f, 145.f, 262.f, 254.f};
    }

    const float Margin = FMath::Clamp(Viewport.X * .03f, 18.f, 42.f);
    const float Gap = 8.f;
    const float ButtonHeight = FMath::Clamp(Viewport.Y * .072f, 42.f, 54.f);
    const float RowTop = Viewport.Y - 98.f - ButtonHeight * 2.f - Gap;
    const float UsableWidth = Viewport.X - Margin * 2.f;
    const float ButtonWidth = (UsableWidth - Gap * 4.f) / 5.f;
    const float PanelWidth = FMath::Min(UsableWidth, 885.f);
    Result.ComparisonPanel = {
        (static_cast<float>(Viewport.X) - PanelWidth) * .5f,
        144.f,
        PanelWidth,
        FMath::Max(0.f, FMath::Min(357.f, RowTop - 154.f))
    };

    if (bComparisonOpen)
    {
        // Modal makes it structurally impossible to hit MOVE, K/L or DAY.
        const float CloseWidth = FMath::Min(320.f, UsableWidth);
        Result.Buttons.Add({
            EAIHeroesZoneScreenAction::CloseComparison,
            {(static_cast<float>(Viewport.X) - CloseWidth) * .5f,
             RowTop + ButtonHeight + Gap,
             CloseWidth, ButtonHeight},
            DisplayName(EAIHeroesZoneScreenAction::CloseComparison), false
        });
        return Result;
    }
    static constexpr EAIHeroesZoneScreenAction Actions[10] = {
        EAIHeroesZoneScreenAction::NextSite,
        EAIHeroesZoneScreenAction::HeroAdvice,
        EAIHeroesZoneScreenAction::InspectSite,
        EAIHeroesZoneScreenAction::CompareRoutes,
        EAIHeroesZoneScreenAction::ConfirmMove,
        EAIHeroesZoneScreenAction::EndDay,
        EAIHeroesZoneScreenAction::Recruit,
        EAIHeroesZoneScreenAction::UpgradeProtocol,
        EAIHeroesZoneScreenAction::Save,
        EAIHeroesZoneScreenAction::Load
    };
    Result.Buttons.Reserve(10);
    for (int32 I = 0; I < 10; ++I)
    {
        const int32 Row = I / 5;
        const int32 Column = I % 5;
        const auto Action = Actions[I];
        const bool bHighImpact = Action ==
            EAIHeroesZoneScreenAction::ConfirmMove ||
            Action == EAIHeroesZoneScreenAction::Save ||
            Action == EAIHeroesZoneScreenAction::Load;
        Result.Buttons.Add({
            Action,
            {Margin + (ButtonWidth + Gap) * Column,
             RowTop + (ButtonHeight + Gap) * Row,
             ButtonWidth, ButtonHeight},
            DisplayName(Action), bHighImpact
        });
    }
    return Result;
}
