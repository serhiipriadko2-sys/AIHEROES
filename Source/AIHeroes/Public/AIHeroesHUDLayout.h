#pragma once

#include "CoreMinimal.h"

// A single authoritative layout for both drawing and mouse/touch hit tests.
// Action IDs 0..11 are stable. ID 12 occupies the previously blank eighth
// strategic grid cell without shifting SAVE/LOAD/REPLAY utilities.
struct FAIHeroesHUDActionRect
{
    float X = 0.f;
    float Y = 0.f;
    float W = 0.f;
    float H = 0.f;

    bool Contains(const FVector2D& Point) const
    {
        return W > 0.f && H > 0.f &&
            Point.X >= X && Point.X <= X + W &&
            Point.Y >= Y && Point.Y <= Y + H;
    }
};

class AIHEROES_API FAIHeroesHUDLayout
{
public:
    // bLiveBattle replaces the strategic buttons with four battle controls.
    // Strategic action 200 is the optional recruitment tip beside ASK HERO.
    static FAIHeroesHUDActionRect Rect(
        int32 ActionId, const FVector2D& Viewport, bool bLiveBattle = false);
    static int32 HitTest(
        const FVector2D& Point, const FVector2D& Viewport, bool bLiveBattle = false);
    static float BottomPanelHeight(float ViewportWidth);
};
