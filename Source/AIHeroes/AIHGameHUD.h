#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "AIHGameHUD.generated.h"

// Canvas HUD keeps the first playable slice independent of UI assets.
UCLASS()
class AIHEROES_API AAIHGameHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;
    int32 GetActionAt(const FVector2D& ScreenPoint) const;

private:
    void GetButtonRect(int32 Index, float& X, float& Y, float& W, float& H) const;
};
