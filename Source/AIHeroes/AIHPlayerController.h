#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AIHPlayerController.generated.h"

UCLASS()
class AIHEROES_API AAIHPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;

private:
    void OnMouseSelect();
    void OnTouchSelect(ETouchIndex::Type FingerIndex, FVector Location);
    void SelectAtScreenPoint(const FVector2D& ScreenPoint);
};
