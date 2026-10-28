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
    void RecruitMilitia();
    void RecruitScouts();
    void UpgradeProtocol();
    void AskAdvisor();
    void EndTurn();
};
