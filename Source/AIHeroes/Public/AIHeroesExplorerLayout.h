#pragma once
#include "CoreMinimal.h"
#include "AIHeroesHUDLayout.h"

// Presentation-only shell. IDs still dispatch through the existing controller.
struct FAIHeroesExplorerFrame
{
    FAIHeroesHUDActionRect Header, Dock, Inspector, Menu, Map, Mission;
    bool bWide = false;
    bool bSupported = false;
};
class AIHEROES_API FAIHeroesExplorerLayout
{
public:
    static constexpr int32 MenuAction = 700;
    static constexpr int32 ConsumeAction = 219;
    static FAIHeroesExplorerFrame Frame(const FVector2D& View);
    static FAIHeroesHUDActionRect Button(int32 Action, const FVector2D& View, bool MenuOpen);
    static int32 Hit(const FVector2D& Point, const FVector2D& View, bool MenuOpen);
    static TArray<int32> Actions(bool MenuOpen);
};
