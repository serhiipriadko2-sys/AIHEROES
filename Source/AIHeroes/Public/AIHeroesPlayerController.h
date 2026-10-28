#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AIHeroesPlayerController.generated.h"

class AAIHeroesGameMode;

UCLASS()
class AIHEROES_API AAIHeroesPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AAIHeroesPlayerController();
    virtual void SetupInputComponent() override;

private:
    AAIHeroesGameMode* Rules() const;
    void SelectNode();
    void SelectAtScreenPoint(const FVector2D& ScreenPoint);
    void SelectByTouch(ETouchIndex::Type FingerIndex, FVector Location);
    void ConfirmSelection();
    void HandleHudAction(int32 ActionIndex);
    void RecruitMilitia();
    void RecruitScouts();
    void UpgradeProtocol();
    void AskAdvisor();
    void EndTurn();
};
