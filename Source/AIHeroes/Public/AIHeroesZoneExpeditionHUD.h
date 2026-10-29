#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "AIHeroesZoneExpeditionHUD.generated.h"

// Minimal readable Windows-mode HUD. Nothing in HUD is authoritative.
UCLASS()
class AIHEROES_API AAIHeroesZoneExpeditionHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
};
