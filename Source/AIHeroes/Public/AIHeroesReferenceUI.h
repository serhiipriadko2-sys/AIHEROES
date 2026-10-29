#pragma once
#include "CoreMinimal.h"
#include "AIHeroesAdvisor.h"

// Original geometric UI symbols, not extracted pixels from reference art.
enum class EAIHeroesGlyph : uint8
{ Compass, Sun, Gold, Wood, Ore, Data, Castle, Hero, Army, Scroll, Hourglass };
struct FAIHeroesGlyphLine { FVector2D A, B; };
struct FAIHeroesOverviewMark
{
    FName Id;
    EAIHeroesNodeType Type = EAIHeroesNodeType::Headquarters;
    FVector2D UV = FVector2D::ZeroVector;
    bool bOwned = false;
    bool bGuarded = false;
};
struct FAIHeroesOverview
{
    bool bValid = false;
    FVector2D HeroUV = FVector2D::ZeroVector;
    TArray<FAIHeroesOverviewMark> Marks;
    TArray<FIntPoint> Links;
};
class AIHEROES_API FAIHeroesReferenceUI
{
public:
    static TArray<FAIHeroesGlyphLine> Glyph(EAIHeroesGlyph Kind);
    // Main five-site map only. No hidden nodes, no invented legacy roads.
    static FAIHeroesOverview Overview(const FAIHeroesAdvisorSnapshot& Snapshot);
    static FVector2D Project(const FVector& Position);
};
