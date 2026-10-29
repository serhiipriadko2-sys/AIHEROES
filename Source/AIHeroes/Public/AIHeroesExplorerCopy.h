#pragma once

#include "CoreMinimal.h"

// Player-facing wording only. It never performs a HUD action or changes
// campaign state. Exact-known event translations preserve unknown errors.
class AIHEROES_API FAIHeroesExplorerCopy
{
public:
    static FString DockLabel(int32 Action, float ButtonWidth,
        const FString& FullLabel);
    static FString EventText(const FString& EngineEvent, float ViewWidth);
};
