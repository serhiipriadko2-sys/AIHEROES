#include "AIHeroesPlayerController.h"

#include "AIHeroesAdvisorDossier.h"
#include "AIHeroesGameMode.h"
#include "AIHeroesHUD.h"
#include "AIHeroesWorldNode.h"
#include "Components/InputComponent.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"

const FKey& AAIHeroesPlayerController::FieldInteractionKey()
{
    return EKeys::Three;
}

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
    InputComponent->BindKey(FieldInteractionKey(), IE_Pressed,
        this, &AAIHeroesPlayerController::InteractFieldDwelling);
    InputComponent->BindKey(EKeys::X, IE_Pressed,
        this, &AAIHeroesPlayerController::RequestSupplyConvoy);
    InputComponent->BindKey(EKeys::G, IE_Pressed,
        this, &AAIHeroesPlayerController::ToggleBasePanel);
    InputComponent->BindKey(EKeys::U, IE_Pressed,
        this, &AAIHeroesPlayerController::UpgradeHeadquarters);
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
    InputComponent->BindKey(EKeys::N, IE_Pressed,
        this, &AAIHeroesPlayerController::AskRecruitmentAdvisor);
    InputComponent->BindKey(EKeys::H, IE_Pressed,
        this, &AAIHeroesPlayerController::AskAdvisor);
    InputComponent->BindKey(EKeys::I, IE_Pressed,
        this, &AAIHeroesPlayerController::ToggleAdvisorDossier);
    InputComponent->BindKey(EKeys::O, IE_Pressed,
        this, &AAIHeroesPlayerController::ShowCampaignObjectives);
    InputComponent->BindKey(EKeys::Y, IE_Pressed,
        this, &AAIHeroesPlayerController::OpenProtocolLab);
    InputComponent->BindKey(EKeys::PageUp, IE_Pressed,
        this, &AAIHeroesPlayerController::PreviousAdvisorDossierPage);
    InputComponent->BindKey(EKeys::PageDown, IE_Pressed,
        this, &AAIHeroesPlayerController::NextAdvisorDossierPage);
    InputComponent->BindKey(EKeys::Z, IE_Pressed,
        this, &AAIHeroesPlayerController::PlanRoute);
    InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed,
        this, &AAIHeroesPlayerController::EndTurn);
    InputComponent->BindKey(EKeys::T, IE_Pressed,
        this, &AAIHeroesPlayerController::CycleBattleSpeed);
    InputComponent->BindKey(EKeys::B, IE_Pressed,
        this, &AAIHeroesPlayerController::FastFinishBattle);
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

    // No map click-through, including when the HUD is temporarily unavailable.
    if (AAIHeroesGameMode* Game = Rules(); Game &&
        (Game->IsBattleReplayActive() || Game->IsLiveBattleActive() ||
         Game->IsAdvisorDossierVisible() || Game->IsBasePanelVisible()))
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
    if (AAIHeroesGameMode* Game = Rules(); Game && Game->IsLiveBattleActive())
    {
        switch (ActionIndex)
        {
            case 100: Game->ToggleLiveBattlePause(); break;
            case 101: Game->CycleLiveBattleSpeed(); break;
            case 102: Game->FastFinishLiveBattle(); break;
            case 103: Game->ReturnFromLiveBattle(); break;
            default: break;
        }
        return;
    }
    if (AAIHeroesGameMode* Game = Rules(); Game && Game->IsBattleReplayActive())
    {
        if (ActionIndex == 0 || ActionIndex == 11)
        {
            Game->SkipBattleReplay();
        }
        return;
    }
    if (AAIHeroesGameMode* Game = Rules())
    {
        switch (ActionIndex)
        {
            case FAIHeroesAdvisorDossierRules::ToggleAction: ToggleAdvisorDossier(); return;
            case FAIHeroesAdvisorDossierRules::PreviousAction: PreviousAdvisorDossierPage(); return;
            case FAIHeroesAdvisorDossierRules::NextAction: NextAdvisorDossierPage(); return;
            case FAIHeroesAdvisorDossierRules::RefreshAction: Game->RefreshAdvisorDossier(); return;
            case FAIHeroesAdvisorDossierRules::CloseAction: Game->CloseAdvisorDossier(); return;
            case FAIHeroesAdvisorDossierRules::ConsumeAction: return;
            default: break;
        }
        if (Game->IsAdvisorDossierVisible()) return;
    }
    if (AAIHeroesGameMode* Game = Rules())
    {
        if (Game->IsBasePanelVisible())
        {
            if (ActionIndex == FAIHeroesBasePanelRules::CloseAction) Game->CloseBasePanel();
            else if (ActionIndex == FAIHeroesBasePanelRules::ConfirmAction) Game->ConfirmBasePurchase();
            else if (ActionIndex >= FAIHeroesBasePanelRules::FirstOfferAction &&
                ActionIndex < FAIHeroesBasePanelRules::FirstOfferAction + FAIHeroesBasePanelRules::OfferCount)
                Game->PreviewBasePurchase(static_cast<EAIHeroesBasePurchase>(
                    ActionIndex - FAIHeroesBasePanelRules::FirstOfferAction));
            return;
        }
        if (ActionIndex == FAIHeroesBasePanelRules::ToggleAction) { Game->ToggleBasePanel(); return; }
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
        case 200: AskRecruitmentAdvisor(); break;
        case 201: UpgradeHeadquarters(); break;
        case 202: ShowCampaignObjectives(); break;
        case 203: OpenProtocolLab(); break;
        case 204: InteractFieldDwelling(); break;
        case 205: RequestSupplyConvoy(); break;
        default: break;
    }
}

void AAIHeroesPlayerController::ConfirmSelection()
{
    if (AAIHeroesGameMode* Game = Rules())
    {
        if (Game->IsBasePanelVisible()) Game->ConfirmBasePurchase();
        else if (Game->IsLiveBattleActive()) Game->ReturnFromLiveBattle();
        else if (Game->IsBattleReplayActive()) Game->SkipBattleReplay();
        else Game->ConfirmSelectedNode();
    }
}

void AAIHeroesPlayerController::RecruitMilitia()
{
    if (AAIHeroesGameMode* Game = Rules(); Game && !Game->IsBattleReplayActive())
    {
        if (Game->IsBasePanelVisible()) Game->PreviewBasePurchase(EAIHeroesBasePurchase::Militia);
        else Game->RecruitMilitia();
    }
}

void AAIHeroesPlayerController::RecruitScouts()
{
    if (AAIHeroesGameMode* Game = Rules(); Game && !Game->IsBattleReplayActive())
    {
        if (Game->IsBasePanelVisible()) Game->PreviewBasePurchase(EAIHeroesBasePurchase::Scouts);
        else Game->RecruitScouts();
    }
}

void AAIHeroesPlayerController::RequestSupplyConvoy()
{
    if (AAIHeroesGameMode* Game = Rules(); Game &&
        !Game->IsLiveBattleActive() && !Game->IsBattleReplayActive())
        Game->RequestSupplyConvoy();
}

void AAIHeroesPlayerController::InteractFieldDwelling()
{
    if (AAIHeroesGameMode* Game = Rules(); Game &&
        !Game->IsLiveBattleActive() && !Game->IsBattleReplayActive())
        Game->InteractFieldDwelling();
}

void AAIHeroesPlayerController::ToggleBasePanel()
{
    if (AAIHeroesGameMode* Game = Rules()) Game->ToggleBasePanel();
}

void AAIHeroesPlayerController::UpgradeHeadquarters()
{
    if (AAIHeroesGameMode* Game = Rules())
    {
        if (Game->IsBasePanelVisible()) Game->PreviewBasePurchase(EAIHeroesBasePurchase::Headquarters);
        else Game->UpgradeHeadquarters();
    }
}

void AAIHeroesPlayerController::UpgradeProtocol()
{
    if (AAIHeroesGameMode* Game = Rules(); Game && !Game->IsBattleReplayActive())
    {
        if (Game->IsBasePanelVisible()) Game->PreviewBasePurchase(EAIHeroesBasePurchase::Protocol);
        else Game->UpgradeProtocol();
    }
}

void AAIHeroesPlayerController::UpgradeModelSlot()
{
    if (AAIHeroesGameMode* Game = Rules(); Game && !Game->IsBattleReplayActive())
    {
        if (Game->IsBasePanelVisible()) Game->PreviewBasePurchase(EAIHeroesBasePurchase::ModelSlot);
        else Game->UpgradeModelSlot();
    }
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
    if (AAIHeroesGameMode* Game = Rules())
    {
        if (Game->IsBasePanelVisible()) Game->CloseBasePanel();
        else if (Game->IsAdvisorDossierVisible()) Game->CloseAdvisorDossier();
        else if (Game->IsLiveBattleActive()) Game->ToggleLiveBattlePause();
        // A quoted purchase requires explicit X confirmation; Esc disarms it.
        else if (!Game->CancelSupplyConvoyQuote()) Game->SkipBattleReplay();
    }
}

void AAIHeroesPlayerController::AskRecruitmentAdvisor()
{
    if (AAIHeroesGameMode* Game = Rules(); Game && !Game->IsBattleReplayActive())
        Game->AskRecruitmentAdvisor();
}

void AAIHeroesPlayerController::AskAdvisor()
{
    if (AAIHeroesGameMode* Game = Rules(); Game && !Game->IsBattleReplayActive()) Game->AskAdvisor();
}

void AAIHeroesPlayerController::ShowCampaignObjectives()
{
    if (AAIHeroesGameMode* Game = Rules()) Game->ShowCampaignObjectives();
}

void AAIHeroesPlayerController::OpenProtocolLab()
{
    if (AAIHeroesGameMode* Game = Rules()) Game->OpenProtocolLab();
}

void AAIHeroesPlayerController::ToggleAdvisorDossier()
{
    if (AAIHeroesGameMode* Game = Rules()) Game->ToggleAdvisorDossier();
}

void AAIHeroesPlayerController::PreviousAdvisorDossierPage()
{
    if (AAIHeroesHUD* GameHUD = Cast<AAIHeroesHUD>(GetHUD())) GameHUD->PageAdvisorDossier(-1);
}

void AAIHeroesPlayerController::NextAdvisorDossierPage()
{
    if (AAIHeroesHUD* GameHUD = Cast<AAIHeroesHUD>(GetHUD())) GameHUD->PageAdvisorDossier(1);
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
    if (AAIHeroesGameMode* Game = Rules())
    {
        if (Game->IsBattleReplayActive()) Game->ToggleGridReplayPause();
        else if (Game->IsLiveBattleActive()) Game->ToggleLiveBattlePause();
        else Game->EndTurn();
    }
}

void AAIHeroesPlayerController::CycleBattleSpeed()
{
    if (AAIHeroesGameMode* Game = Rules())
    {
        if (Game->IsBattleReplayActive()) Game->CycleGridReplaySpeed();
        else Game->CycleLiveBattleSpeed();
    }
}

void AAIHeroesPlayerController::FastFinishBattle()
{
    if (AAIHeroesGameMode* Game = Rules())
    {
        if (Game->IsBattleReplayActive()) Game->SkipBattleReplay();
        else Game->FastFinishLiveBattle();
    }
}
