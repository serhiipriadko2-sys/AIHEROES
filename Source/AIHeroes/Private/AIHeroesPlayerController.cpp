#include "AIHeroesPlayerController.h"

#include "AIHeroesGameMode.h"
#include "AIHeroesWorldNode.h"
#include "InputCoreTypes.h"
#include "Components/InputComponent.h"
#include "Kismet/GameplayStatics.h"

AAIHeroesPlayerController::AAIHeroesPlayerController()
{
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    DefaultMouseCursor = EMouseCursor::Crosshairs;
}

void AAIHeroesPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindKey(EKeys::LeftMouseButton, IE_Pressed, this, &AAIHeroesPlayerController::SelectNode);
    InputComponent->BindKey(EKeys::One, IE_Pressed, this, &AAIHeroesPlayerController::RecruitMilitia);
    InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &AAIHeroesPlayerController::RecruitScouts);
    InputComponent->BindKey(EKeys::P, IE_Pressed, this, &AAIHeroesPlayerController::UpgradeProtocol);
    InputComponent->BindKey(EKeys::H, IE_Pressed, this, &AAIHeroesPlayerController::AskAdvisor);
    InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &AAIHeroesPlayerController::EndTurn);
}

AAIHeroesGameMode* AAIHeroesPlayerController::Rules() const
{
    return Cast<AAIHeroesGameMode>(UGameplayStatics::GetGameMode(this));
}

void AAIHeroesPlayerController::SelectNode()
{
    FHitResult Hit;
    if (GetHitResultUnderCursorByChannel(
        UEngineTypes::ConvertToTraceType(ECC_Visibility), true, Hit))
    {
        if (AAIHeroesWorldNode* Node = Cast<AAIHeroesWorldNode>(Hit.GetActor()))
        {
            if (AAIHeroesGameMode* Game = Rules())
            {
                Game->InteractWithNode(Node);
            }
        }
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
