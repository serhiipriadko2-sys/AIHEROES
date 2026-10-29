#pragma once

#include "CoreMinimal.h"

// One pure placement/hit-test contract for Canvas, mouse and screen touch.
// Pixels are viewport-relative (NOT UMG DPI units). The fixed margins are
// fallback padding, NOT a verified platform notch/safe-area inset.
enum class EAIHeroesZoneScreenAction : uint8
{
    None,
    NextSite,
    HeroAdvice,
    InspectSite,
    CompareRoutes,
    ConfirmMove,
    EndDay,
    Recruit,
    UpgradeProtocol,
    Save,
    Load,
    CloseComparison
};

struct FAIHeroesZoneScreenRect
{
    float X = 0.f, Y = 0.f, Width = 0.f, Height = 0.f;

    bool Contains(const FVector2D& Point) const
    {
        return Point.X >= X && Point.Y >= Y &&
            Point.X < X + Width && Point.Y < Y + Height;
    }
};

struct FAIHeroesZoneScreenButton
{
    EAIHeroesZoneScreenAction Action = EAIHeroesZoneScreenAction::None;
    FAIHeroesZoneScreenRect Rect;
    const TCHAR* Label = TEXT("");
    bool bHighImpact = false;
};

struct AIHEROES_API FAIHeroesZoneScreenLayout
{
    // 1280x720 and 960x540 supported; tiny viewports require resize.
    // A modal contains exactly one CLOSE action. All gameplay actions
    // are absent, not simply hidden behind a visually dimmed button.
    static FAIHeroesZoneScreenLayout Build(
        const FVector2D& Viewport, bool bComparisonOpen);
    static const TCHAR* Name(EAIHeroesZoneScreenAction Action);

    bool bSupported = false;
    bool bComparisonOpen = false;
    FVector2D Viewport = FVector2D::ZeroVector;
    FAIHeroesZoneScreenRect ComparisonPanel;
    TArray<FAIHeroesZoneScreenButton> Buttons;

    EAIHeroesZoneScreenAction HitTest(const FVector2D& Point) const
    {
        if (!bSupported || !FMath::IsFinite(Point.X) ||
            !FMath::IsFinite(Point.Y)) return EAIHeroesZoneScreenAction::None;
        for (const auto& Button : Buttons)
            if (Button.Rect.Contains(Point)) return Button.Action;
        return EAIHeroesZoneScreenAction::None;
    }
};

