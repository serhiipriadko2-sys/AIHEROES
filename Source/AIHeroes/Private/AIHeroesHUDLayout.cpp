#include "AIHeroesHUDLayout.h"

namespace
{
FAIHeroesHUDActionRect BaseStrategicRect(
    int32 ActionId, const FVector2D& Viewport)
{
    FAIHeroesHUDActionRect Rect;
    const int32 Columns = Viewport.X >= 1150.f ? 4 :
        (Viewport.X >= 800.f ? 3 : 2);
    const float Gap = 10.f;
    Rect.W = FMath::Min(260.f,
        (Viewport.X - 28.f - (Columns - 1) * Gap) / Columns);
    Rect.H = 43.f;
    if (Rect.W <= 0.f)
    {
        return FAIHeroesHUDActionRect();
    }
    const float GridWidth = Columns * Rect.W + (Columns - 1) * Gap;
    const float Left = FMath::Max(14.f, (Viewport.X - GridWidth) * 0.5f);
    const int32 GridIndex = ActionId == 12 ? 7 : ActionId;
    Rect.X = Left + (GridIndex % Columns) * (Rect.W + Gap);
    Rect.Y = Viewport.Y - FAIHeroesHUDLayout::BottomPanelHeight(Viewport.X) +
        113.f + (GridIndex / Columns) * (Rect.H + Gap);
    return Rect;
}

float RecruitTipWidth(float BaseWidth)
{
    // Previously the ASK HERO button occupied its entire cell. Preserve its
    // original CENTER pixel even after adding the recruiting tip on the right.
    return FMath::Min(FMath::Min(90.f, FMath::Max(46.f, BaseWidth * 0.32f)),
        BaseWidth * 0.4f);
}
}

float FAIHeroesHUDLayout::BottomPanelHeight(float ViewportWidth)
{
    const int32 Columns = ViewportWidth >= 1150.f ? 4 :
        (ViewportWidth >= 800.f ? 3 : 2);
    return 120.f + 53.f * ((8 + Columns - 1) / Columns);
}

FAIHeroesHUDActionRect FAIHeroesHUDLayout::Rect(
    int32 ActionId, const FVector2D& Viewport, bool bLiveBattle)
{
    FAIHeroesHUDActionRect Rect;
    if (Viewport.X <= 0.f || Viewport.Y <= 0.f)
    {
        return Rect;
    }

    if (bLiveBattle)
    {
        // During the live battle all strategic controls disappear.
        if (ActionId < 100 || ActionId > 103)
        {
            return Rect;
        }
        Rect.W = FMath::Min(235.f, (Viewport.X - 50.f) / 4.f);
        Rect.H = 46.f;
        Rect.X = (Viewport.X - (4.f * Rect.W + 30.f)) * 0.5f +
            (ActionId - 100) * (Rect.W + 10.f);
        Rect.Y = Viewport.Y - 62.f;
        return Rect;
    }

    if (ActionId == 200 || ActionId == 201)
    {
        const FAIHeroesHUDActionRect AskCell = BaseStrategicRect(ActionId == 200 ? 1 : 6, Viewport);
        Rect.W = RecruitTipWidth(AskCell.W);
        Rect.H = AskCell.H;
        Rect.X = AskCell.X + AskCell.W - Rect.W;
        Rect.Y = AskCell.Y;
        return Rect;
    }
    // Portrait widths cannot fit FOUR 100px touch targets side by side.
    // Reflow to two rows/two columns, preserving action IDs and sharing the
    // identical geometry with the mouse/touch HitTest implementation.
    if (Viewport.X < 480.f && ActionId >= 202 && ActionId <= 205)
    {
        constexpr float Outer = 8.f;
        constexpr float GapX = 8.f;
        constexpr float RowStep = 32.f;
        const int32 Index = ActionId - 202;
        Rect.W = (Viewport.X - Outer * 2.f - GapX) * 0.5f;
        Rect.H = 29.f;
        Rect.X = Outer + (Index % 2) * (Rect.W + GapX);
        Rect.Y = Viewport.Y - BottomPanelHeight(Viewport.X) - 65.f +
            (Index / 2) * RowStep;
        if (Rect.W < 100.f || Rect.Y < 185.f ||
            Rect.X + Rect.W > Viewport.X - Outer ||
            Rect.Y + Rect.H > Viewport.Y)
            return FAIHeroesHUDActionRect();
        return Rect;
    }
    if (ActionId == 205)
    {
        const FAIHeroesHUDActionRect Field = FAIHeroesHUDLayout::Rect(204, Viewport, false);
        if (Field.W <= 0.f) return Rect;
        Rect.W = Viewport.X < 700.f ? 110.f : 128.f;
        Rect.H = Field.H;
        Rect.X = Field.X - Rect.W - 8.f;
        Rect.Y = Field.Y;
        if (Rect.X < 8.f) return FAIHeroesHUDActionRect();
        return Rect;
    }
    if (ActionId == 204)
    {
        const FAIHeroesHUDActionRect Lab = FAIHeroesHUDLayout::Rect(203, Viewport, false);
        if (Lab.W <= 0.f) return Rect;
        Rect.W = Viewport.X < 700.f ? 118.f : 128.f;
        Rect.H = Lab.H;
        Rect.X = Lab.X - Rect.W - 8.f;
        Rect.Y = Lab.Y;
        if (Rect.X < 8.f) return FAIHeroesHUDActionRect();
        return Rect;
    }
    if (ActionId == 203)
    {
        const FAIHeroesHUDActionRect Goals = FAIHeroesHUDLayout::Rect(202, Viewport, false);
        if (Goals.W <= 0.f) return Rect;
        Rect.W = Viewport.X < 700.f ? 100.f : 128.f;
        Rect.H = Goals.H;
        Rect.X = Goals.X - Rect.W - 8.f;
        Rect.Y = Goals.Y;
        if (Rect.X < 8.f) return FAIHeroesHUDActionRect();
        return Rect;
    }
    if (ActionId == 202)
    {
        Rect.W = Viewport.X < 700.f ? 108.f : 144.f;
        Rect.H = 29.f;
        Rect.X = FMath::Max(8.f, Viewport.X - Rect.W - 12.f);
        Rect.Y = Viewport.Y - BottomPanelHeight(Viewport.X) - 33.f;
        if (Rect.Y < 185.f) return FAIHeroesHUDActionRect();
        return Rect;
    }
    if (ActionId < 0 || ActionId >= 13)
    {
        return Rect;
    }
    if (ActionId >= 7 && ActionId <= 11)
    {
        Rect.W = Viewport.X >= 700.f ? 116.f : 91.f;
        Rect.H = 30.f;
        if (Viewport.X >= 1050.f)
        {
            Rect.X = Viewport.X - (12 - ActionId) * (Rect.W + 6.f) - 8.f;
            Rect.Y = 7.f;
        }
        else
        {
            const int32 Utility = ActionId - 7;
            Rect.X = Viewport.X - (2 - Utility % 2) * (Rect.W + 6.f) - 8.f;
            Rect.Y = 5.f + (Utility / 2) * 35.f;
        }
        return Rect;
    }

    Rect = BaseStrategicRect(ActionId, Viewport);
    if (ActionId == 1 || ActionId == 6)
    {
        Rect.W -= RecruitTipWidth(Rect.W) + 6.f;
    }
    return Rect;
}

int32 FAIHeroesHUDLayout::HitTest(
    const FVector2D& Point, const FVector2D& Viewport, bool bLiveBattle)
{
    if (bLiveBattle)
    {
        for (int32 Action = 100; Action <= 103; ++Action)
        {
            if (Rect(Action, Viewport, true).Contains(Point))
            {
                return Action;
            }
        }
        return INDEX_NONE;
    }

    // This tip is a distinct control; it does not change the old ASK HERO
    // center, which pre-existing mouse QA targets at 1280x720 coordinate 505.
    if (Rect(200, Viewport).Contains(Point))
    {
        return 200;
    }
    if (Rect(201, Viewport).Contains(Point)) return 201;
    if (Rect(202, Viewport).Contains(Point)) return 202;
    if (Rect(203, Viewport).Contains(Point)) return 203;
    if (Rect(204, Viewport).Contains(Point)) return 204;
    if (Rect(205, Viewport).Contains(Point)) return 205;
    for (int32 Action = 0; Action < 13; ++Action)
    {
        if (Rect(Action, Viewport).Contains(Point))
        {
            return Action;
        }
    }
    return INDEX_NONE;
}
