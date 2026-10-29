#pragma once

#include "CoreMinimal.h"
#include "Templates/Function.h"

// Display-only text fitting. No world, orders, save files or font-size changes.
class AIHEROES_API FAIHeroesHUDTextLayout
{
public:
    static constexpr int32 MaxSourceChars = 8192;

    // Actual rendered widths, including the ellipsis, decide what fits.
    // Reject invalid dimensions/measurements and oversized input. The input
    // remains untouched; UTF-16 surrogate pairs are not cut in half.
    static FString FitLine(const FString& Text, float MaxWidth,
        TFunctionRef<float(const FString&)> Measure);

    // Left header starts at x=18. Reserve existing utility-button rectangles
    // only on intersecting rows. Does not move buttons or change hit testing.
    static float HeaderLineWidth(const FVector2D& Viewport,
        float Y, float TextHeight);
};
