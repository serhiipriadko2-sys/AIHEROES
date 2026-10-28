#include "AIHPlayerController.h"

#include "AIHGameHUD.h"
#include "AIHGameMode.h"
#include "AIHWorldNode.h"
#include "Components/InputComponent.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"

void AAIHPlayerController::BeginPlay()
{
    Super::BeginPlay();
    bShowMouseCursor = true;
    bEnableClickEvents = true;
    bEnableTouchEvents = true;
    FInputModeGameAndUI Mode;
    Mode.SetHideCursorDuringCapture(false);
    SetInputMode(Mode);
}

void AAIHPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindAction(TEXT("SelectNode"), IE_Pressed, this,
        &AAIHPlayerController::OnMouseSelect);
    InputComponent->BindTouch(IE_Pressed, this,
        &AAIHPlayerController::OnTouchSelect);
}

void AAIHPlayerController::OnMouseSelect()
{
    float X = 0.f;
    float Y = 0.f;
    if (GetMousePosition(X, Y))
    {
        SelectAtScreenPoint(FVector2D(X, Y));
    }
}

void AAIHPlayerController::OnTouchSelect(ETouchIndex::Type FingerIndex, FVector Location)
{
    SelectAtScreenPoint(FVector2D(Location.X, Location.Y));
}

void AAIHPlayerController::SelectAtScreenPoint(const FVector2D& ScreenPoint)
{
    AAIHGameMode* Game = GetWorld() ? GetWorld()->GetAuthGameMode<AAIHGameMode>() : nullptr;
    if (!Game)
    {
        return;
    }

    if (const AAIHGameHUD* GameHUD = Cast<AAIHGameHUD>(GetHUD()))
    {
        const int32 Action = GameHUD->GetActionAt(ScreenPoint);
        if (Action >= 0)
        {
            Game->PerformAction(Action);
            return;
        }
    }

    FHitResult Hit;
    if (GetHitResultAtScreenPosition(ScreenPoint, ECC_Visibility, false, Hit))
    {
        if (AAIHWorldNode* Node = Cast<AAIHWorldNode>(Hit.GetActor()))
        {
            Game->SelectNode(Node);
        }
    }
}
