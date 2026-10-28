#include "AIHeroesPlayerController.h"

#include "AIHeroesGameMode.h"
#include "AIHeroesHUD.h"
#include "AIHeroesWorldNode.h"
#include "Components/InputComponent.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"

AAIHeroesPlayerController::AAIHeroesPlayerController()
{
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableTouchEvents = true;
    DefaultMouseCursor = EMouseCursor::Crosshairs;
}

void AAIHeroesPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed,
        this, &AAIHeroesPlayerController::SelectNode);
    InputComponent->BindTouch(IE_Pressed,
        this, &AAIHeroesPlayerController::SelectByTouch);
    InputComponent->BindKey(EKeys::Enter, IE_Pressed,
        this, &AAIHeroesPlayerController::ConfirmSelection);
    InputComponent->BindKey(EKeys::One, IE_Pressed,
        this, &AAIHeroesPlayerController::RecruitMilitia);
    InputComponent->BindKey(EKeys::Two, IE_Pressed,
        this, &AAIHeroesPlayerController::RecruitScouts);
    InputComponent->BindKey(EKeys::P, IE_Pressed,
        this, &AAIHeroesPlayerController::UpgradeProtocol);
    InputComponent->BindKey(EKeys::M, IE_Pressed,
        this, &AAIHeroesPlayerController::UpgradeModelSlot);
    InputComponent->BindKey(EKeys::K, IE_Pressed,
        this, &AAIHeroesPlayerController::SaveCampaign);
    InputComponent->BindKey(EKeys::L, IE_Pressed,
        this, &AAIHeroesPlayerController::LoadCampaign);
    InputComponent->BindKey(EKeys::C, IE_Pressed,
        this, &AAIHeroesPlayerController::CycleSaveSlot);
    InputComponent->BindKey(EKeys::J, IE_Pressed,
        this, &AAIHeroesPlayerController::AdvanceBattleReplay);
    InputComponent->BindKey(EKeys::V, IE_Pressed,
        this, &AAIHeroesPlayerController::PlayBattleIn3D);
    InputComponent->BindKey(EKeys::Escape, IE_Pressed,
        this, &AAIHeroesPlayerController::SkipBattleReplay);
    InputComponent->BindKey(EKeys::H, IE_Pressed,
        this, &AAIHeroesPlayerController::AskAdvisor);
    InputComponent->BindKey(EKeys::Z, IE_Pressed,
        this, &AAIHeroesPlayerController::PlanRoute);
    InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed,
        this, &AAIHeroesPlayerController::EndTurn);
}

AAIHeroesGameMode* AAIHeroesPlayerController::Rules() const
{
    return Cast<AAIHeroesGameMode>(UGameplayStatics::GetGameMode(this));
}

void AAIHeroesPlayerController::SelectNode()
{
    float X = 0.f;
    float Y = 0.f;
    if (GetMousePosition(X, Y))
    {
        SelectAtScreenPoint(FVector2D(X, Y));
    }
}

void AAIHeroesPlayerController::SelectByTouch(
    ETouchIndex::Type FingerIndex, FVector Location)
{
    SelectAtScreenPoint(FVector2D(Location.X, Location.Y));
}

void AAIHeroesPlayerController::SelectAtScreenPoint(const FVector2D& ScreenPoint)
{
    if (const AAIHeroesHUD* GameHUD = Cast<AAIHeroesHUD>(GetHUD()))
    {
        const int32 Action = GameHUD->GetActionAt(ScreenPoint);
        if (Action != INDEX_NONE)
        {
            HandleHudAction(Action);
            return;
        }
    }

    // Never select a strategic target through a cinematic replay stage.
    if (AAIHeroesGameMode* Game = Rules(); Game && Game->IsBattleReplayActive())
    {
        return;
    }

    FHitResult Hit;
    if (GetHitResultAtScreenPosition(ScreenPoint, ECC_Visibility, true, Hit))
    {
        if (AAIHeroesWorldNode* Node = Cast<AAIHeroesWorldNode>(Hit.GetActor()))
        {
            if (AAIHeroesGameMode* Game = Rules())
            {
                Game->SelectNode(Node);
            }
        }
    }
}

void AAIHeroesPlayerController::HandleHudAction(int32 ActionIndex)
{
    UE_LOG(LogTemp, Display, TEXT("[AI HEROES UI] Button action=%d"), ActionIndex);
    if (AAIHeroesGameMode* Game = Rules(); Game && Game->IsBattleReplayActive())
    {
        if (ActionIndex == 0 || ActionIndex == 11)
        {
            Game->SkipBattleReplay();
        }
        return;
    }
    switch (ActionIndex)
    {
        case 0: ConfirmSelection(); break;
        case 1: AskAdvisor(); break;
        case 2: EndTurn(); break;
        case 3: RecruitMilitia(); break;
        case 4: RecruitScouts(); break;
        case 5: UpgradeProtocol(); break;
        case 6: UpgradeModelSlot(); break;
        case 7: SaveCampaign(); break;
        case 8: LoadCampaign(); break;
        case 9: CycleSaveSlot(); break;
        case 10: AdvanceBattleReplay(); break;
        case 11: PlayBattleIn3D(); break;
        case 12: PlanRoute(); break;
        default: break;
    }
}

void AAIHeroesPlayerController::ConfirmSelection()
{
    if (AAIHeroesGameMode* Game = Rules())
    {
        if (Game->IsBattleReplayActive()) Game->SkipBattleReplay();
        else Game->ConfirmSelectedNode();
    }
}

void AAIHeroesPlayerController::RecruitMilitia()
{
    if (AAIHeroesGameMode* Game = Rules(); Game && !Game->IsBattleReplayActive()) Game->RecruitMilitia();
}

void AAIHeroesPlayerController::RecruitScouts()
{
    if (AAIHeroesGameMode* Game = Rules(); Game && !Game->IsBattleReplayActive()) Game->RecruitScouts();
}

void AAIHeroesPlayerController::UpgradeProtocol()
{
    if (AAIHeroesGameMode* Game = Rules(); Game && !Game->IsBattleReplayActive()) Game->UpgradeProtocol();
}

void AAIHeroesPlayerController::UpgradeModelSlot()
{
    if (AAIHeroesGameMode* Game = Rules(); Game && !Game->IsBattleReplayActive()) Game->UpgradeModelSlot();
}

void AAIHeroesPlayerController::SaveCampaign()
{
    if (AAIHeroesGameMode* Game = Rules(); Game && !Game->IsBattleReplayActive()) Game->SaveCampaign();
}

void AAIHeroesPlayerController::LoadCampaign()
{
    if (AAIHeroesGameMode* Game = Rules(); Game && !Game->IsBattleReplayActive()) Game->LoadCampaign();
}

void AAIHeroesPlayerController::CycleSaveSlot()
{
    if (AAIHeroesGameMode* Game = Rules(); Game && !Game->IsBattleReplayActive()) Game->CycleSaveSlot();
}

void AAIHeroesPlayerController::AdvanceBattleReplay()
{
    if (AAIHeroesGameMode* Game = Rules(); Game && !Game->IsBattleReplayActive()) Game->AdvanceBattleReplay();
}

void AAIHeroesPlayerController::PlayBattleIn3D()
{
    if (AAIHeroesGameMode* Game = Rules())
    {
        if (Game->IsBattleReplayActive()) Game->SkipBattleReplay();
        else Game->PlayLastBattleIn3D();
    }
}

void AAIHeroesPlayerController::SkipBattleReplay()
{
    if (AAIHeroesGameMode* Game = Rules()) Game->SkipBattleReplay();
}

void AAIHeroesPlayerController::AskAdvisor()
{
    if (AAIHeroesGameMode* Game = Rules(); Game && !Game->IsBattleReplayActive()) Game->AskAdvisor();
}

void AAIHeroesPlayerController::PlanRoute()
{
    if (AAIHeroesGameMode* Game = Rules(); Game && !Game->IsBattleReplayActive())
    {
        Game->PlanRoute();
    }
}

void AAIHeroesPlayerController::EndTurn()
{
    if (AAIHeroesGameMode* Game = Rules(); Game && !Game->IsBattleReplayActive()) Game->EndTurn();
}
