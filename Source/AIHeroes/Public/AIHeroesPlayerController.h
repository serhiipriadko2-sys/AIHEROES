#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AIHeroesPlayerController.generated.h"

class AAIHeroesGameMode;
struct FKey;

UCLASS()
class AIHEROES_API AAIHeroesPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AAIHeroesPlayerController();
    virtual void SetupInputComponent() override;
    // Q/E belong exclusively to camera polling; field orders use a distinct key.
    static const FKey& FieldInteractionKey();

private:
    AAIHeroesGameMode* Rules() const;
    void SelectNode();
    void SelectAtScreenPoint(const FVector2D& ScreenPoint);
    void SelectByTouch(ETouchIndex::Type FingerIndex, FVector Location);
    void ConfirmSelection();
    void HandleHudAction(int32 ActionIndex);
    void RecruitMilitia();
    void RecruitScouts();
    void InteractFieldDwelling();
    void RequestSupplyConvoy();
    void UpgradeHeadquarters();
    void ToggleBasePanel();
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
    void ShowCampaignObjectives();
    void OpenProtocolLab();
    void ToggleAdvisorDossier();
    void PreviousAdvisorDossierPage();
    void NextAdvisorDossierPage();
    void PlanRoute();
    void EndTurn();
    void CycleBattleSpeed();
    void FastFinishBattle();
};
