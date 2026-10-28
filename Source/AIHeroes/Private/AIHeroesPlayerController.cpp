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
    InputComponent->BindKey(EKeys::H, IE_Pressed,
        this, &AAIHeroesPlayerController::AskAdvisor);
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
    switch (ActionIndex)
    {
        case 0: ConfirmSelection(); break;
        case 1: AskAdvisor(); break;
        case 2: EndTurn(); break;
        case 3: RecruitMilitia(); break;
        case 4: RecruitScouts(); break;
        case 5: UpgradeProtocol(); break;
        default: break;
    }
}

void AAIHeroesPlayerController::ConfirmSelection()
{
    if (AAIHeroesGameMode* Game = Rules())
    {
        Game->ConfirmSelectedNode();
    }
}

void AAIHeroesPlayerController::RecruitMilitia()
{
    if (AAIHeroesGameMode* Game = Rules()) Game->RecruitMilitia();
}

void AAIHeroesPlayerController::RecruitScouts()
{
    if (AAIHeroesGameMode* Game = Rules()) Game->RecruitScouts();
}

void AAIHeroesPlayerController::UpgradeProtocol()
{
    if (AAIHeroesGameMode* Game = Rules()) Game->UpgradeProtocol();
}

void AAIHeroesPlayerController::AskAdvisor()
{
    if (AAIHeroesGameMode* Game = Rules()) Game->AskAdvisor();
}

void AAIHeroesPlayerController::EndTurn()
{
    if (AAIHeroesGameMode* Game = Rules()) Game->EndTurn();
}
