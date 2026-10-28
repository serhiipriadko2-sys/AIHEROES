#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "AIHeroesCameraPawn.generated.h"

class UCameraComponent;

UCLASS()
class AIHEROES_API AAIHeroesCameraPawn : public APawn
{
    GENERATED_BODY()

public:
    AAIHeroesCameraPawn();
    virtual void Tick(float DeltaSeconds) override;
    virtual void BeginPlay() override;

private:
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UCameraComponent> Camera;

    float MoveSpeed = 900.f;
};
