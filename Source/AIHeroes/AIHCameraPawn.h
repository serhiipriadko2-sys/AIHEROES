#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "AIHCameraPawn.generated.h"

class USceneComponent;
class USpringArmComponent;
class UCameraComponent;

UCLASS()
class AIHEROES_API AAIHCameraPawn : public APawn
{
    GENERATED_BODY()

public:
    AAIHCameraPawn();
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
    void Zoom(float Value);
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> Pivot;
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USpringArmComponent> Arm;
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UCameraComponent> ViewCamera;
};
