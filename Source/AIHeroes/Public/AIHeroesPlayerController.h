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
    void UpgradeModelSlot();
    void SaveCampaign();
    void LoadCampaign();
    void CycleSaveSlot();
    void AdvanceBattleReplay();
    void PlayBattleIn3D();
    void SkipBattleReplay();
    void AskAdvisor();
    void AskRecruitmentAdvisor();
    void PlanRoute();
    void EndTurn();
    void CycleBattleSpeed();
    void FastFinishBattle();
};
