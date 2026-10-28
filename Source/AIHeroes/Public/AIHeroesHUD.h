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
};
