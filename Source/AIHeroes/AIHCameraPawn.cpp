#include "AIHCameraPawn.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/SceneComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

AAIHCameraPawn::AAIHCameraPawn()
{
    PrimaryActorTick.bCanEverTick = true;
    Pivot = CreateDefaultSubobject<USceneComponent>(TEXT("CameraPivot"));
    RootComponent = Pivot;
    Arm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
    Arm->SetupAttachment(Pivot);
    Arm->TargetArmLength = 1600.f;
    Arm->SetRelativeRotation(FRotator(-55.f, 0.f, 0.f));
    Arm->bDoCollisionTest = false;
    ViewCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("PerspectiveCamera"));
    ViewCamera->SetupAttachment(Arm, USpringArmComponent::SocketName);
    ViewCamera->ProjectionMode = ECameraProjectionMode::Perspective;
    ViewCamera->FieldOfView = 70.f;
}

void AAIHCameraPawn::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);
    Input->BindAxis(TEXT("CameraZoom"), this, &AAIHCameraPawn::Zoom);
}

void AAIHCameraPawn::Zoom(float Value)
{
    if (FMath::Abs(Value) > 0.01f)
    {
        Arm->TargetArmLength = FMath::Clamp(
            Arm->TargetArmLength - Value * 130.f, 500.f, 2600.f);
    }
}

void AAIHCameraPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC) return;

    FVector Step = FVector::ZeroVector;
    if (PC->IsInputKeyDown(EKeys::W)) Step.X += 1.f;
    if (PC->IsInputKeyDown(EKeys::S)) Step.X -= 1.f;
    if (PC->IsInputKeyDown(EKeys::D)) Step.Y += 1.f;
    if (PC->IsInputKeyDown(EKeys::A)) Step.Y -= 1.f;
    const float Turn = (PC->IsInputKeyDown(EKeys::E) ? 1.f : 0.f)
        - (PC->IsInputKeyDown(EKeys::Q) ? 1.f : 0.f);
    Arm->AddRelativeRotation(FRotator(0.f, Turn * 65.f * DeltaSeconds, 0.f));

    if (!Step.IsNearlyZero())
    {
        const FVector WorldStep = FRotator(0.f, Arm->GetRelativeRotation().Yaw, 0.f)
            .RotateVector(Step.GetSafeNormal());
        FVector Target = GetActorLocation() + WorldStep * 650.f * DeltaSeconds;
        Target.X = FMath::Clamp(Target.X, -1100.f, 1100.f);
        Target.Y = FMath::Clamp(Target.Y, -1100.f, 1100.f);
        SetActorLocation(Target);
    }
}
