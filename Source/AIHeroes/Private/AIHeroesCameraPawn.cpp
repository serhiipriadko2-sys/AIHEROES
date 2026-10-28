#include "AIHeroesCameraPawn.h"
#include "AIHeroesGameMode.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"

AAIHeroesCameraPawn::AAIHeroesCameraPawn()
{
    PrimaryActorTick.bCanEverTick = true;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("CameraRoot"));
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("PerspectiveCamera"));
    Camera->SetupAttachment(RootComponent);
    Camera->SetRelativeRotation(FRotator(-43.f, 0.f, 0.f));
    Camera->FieldOfView = 82.f;
    AutoPossessPlayer = EAutoReceiveInput::Player0;
}

void AAIHeroesCameraPawn::BeginPlay()
{
    Super::BeginPlay();
    // From outside the board toward its centre: a rotatable 3D perspective.
    SetActorLocation(FVector(-1850.f, 0.f, 1850.f));
}

void AAIHeroesCameraPawn::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    // A tactical replay owns the perspective temporarily.
    if (const AAIHeroesGameMode* Game = Cast<AAIHeroesGameMode>(UGameplayStatics::GetGameMode(this));
        Game && (Game->IsBattleReplayActive() || Game->IsLiveBattleActive() ||
            Game->IsBasePanelVisible() || Game->IsAdvisorDossierVisible()))
    {
        return;
    }
    APlayerController* PC = Cast<APlayerController>(GetController());
    if (!PC)
    {
        return;
    }

    const float Forward = (PC->IsInputKeyDown(EKeys::W) ? 1.f : 0.f) -
        (PC->IsInputKeyDown(EKeys::S) ? 1.f : 0.f);
    const float Right = (PC->IsInputKeyDown(EKeys::D) ? 1.f : 0.f) -
        (PC->IsInputKeyDown(EKeys::A) ? 1.f : 0.f);
    const float Up = (PC->IsInputKeyDown(EKeys::R) ? 1.f : 0.f) -
        (PC->IsInputKeyDown(EKeys::F) ? 1.f : 0.f);
    const float Turn = (PC->IsInputKeyDown(EKeys::E) ? 1.f : 0.f) -
        (PC->IsInputKeyDown(EKeys::Q) ? 1.f : 0.f);

    if (!FMath::IsNearlyZero(Turn))
    {
        AddActorWorldRotation(FRotator(0.f, Turn * 65.f * DeltaSeconds, 0.f));
    }

    FVector Move = GetActorForwardVector() * Forward + GetActorRightVector() * Right;
    Move.Z = 0.f;
    Move = Move.GetClampedToMaxSize(1.f);
    FVector NewLocation = GetActorLocation() + Move * MoveSpeed * DeltaSeconds;
    NewLocation.Z = FMath::Clamp(NewLocation.Z + Up * MoveSpeed * DeltaSeconds, 600.f, 2700.f);
    NewLocation.X = FMath::Clamp(NewLocation.X, -2600.f, 2600.f);
    NewLocation.Y = FMath::Clamp(NewLocation.Y, -2200.f, 2200.f);
    SetActorLocation(NewLocation);
}
