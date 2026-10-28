#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "AIHeroesHUD.generated.h"

UCLASS()
class AIHEROES_API AAIHeroesHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;
    int32 GetActionAt(const FVector2D& ScreenPoint) const;

private:
    void GetActionRect(int32 Index, float& X, float& Y, float& W, float& H) const;
};
