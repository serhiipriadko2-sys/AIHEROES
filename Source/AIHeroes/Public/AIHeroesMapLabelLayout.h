#pragma once

#include "CoreMinimal.h"
#include "AIHeroesHUDLayout.h"

// Display-only input: unknown sites must never get labels or reserve space.
struct FAIHeroesMapLabelRequest
{
    FName Id = NAME_None;
    FVector2D Anchor = FVector2D::ZeroVector;
    bool bKnown = false;
    bool bSelected = false;
    bool bHasDetail = false;
};

struct FAIHeroesMapLabelPlacement
{
    FName Id = NAME_None;
    FVector2D Anchor = FVector2D::ZeroVector;
    FAIHeroesHUDActionRect Rect;
    bool bShowDetail = false;
};

class AIHEROES_API FAIHeroesMapLabelLayout
{
public:
    static constexpr int32 MaxLabels = 64;
    // Canvas coordinates, not mobile device safe insets or a UMG DPI scale.
    static FAIHeroesHUDActionRect MapBounds(const FVector2D& Viewport);
    static bool ContainsRect(const FAIHeroesHUDActionRect& Outer,
        const FAIHeroesHUDActionRect& Inner);
    static bool Overlaps(const FAIHeroesHUDActionRect& A,
        const FAIHeroesHUDActionRect& B, float Gap = 6.f);
    // Selected label first, stable ID order after that; never mutates Requests.
    // Crowded/undersized views omit a label rather than paint over a control.
    static TArray<FAIHeroesMapLabelPlacement> Place(const FVector2D& Viewport,
        const TArray<FAIHeroesMapLabelRequest>& Requests);
};
