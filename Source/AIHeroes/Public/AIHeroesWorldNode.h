#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AIHeroesTypes.h"
#include "AIHeroesWorldNode.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UPointLightComponent;

UCLASS()
class AIHEROES_API AAIHeroesWorldNode : public AActor
{
    GENERATED_BODY()

public:
    AAIHeroesWorldNode();
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<UStaticMeshComponent> Visual;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<UTextRenderComponent> Label;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TObjectPtr<UPointLightComponent> Beacon;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    EAIHeroesNodeType NodeType = EAIHeroesNodeType::Headquarters;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    bool bOwned = false;

    UPROPERTY()
    TArray<FAIHeroesArmyStack> Defenders;

    void Configure(EAIHeroesNodeType InType, const FString& InName, const TArray<FAIHeroesArmyStack>& InDefenders);
    void SetOwned(bool bNewOwned);
    FString GetDisplayName() const;

private:
    FString DisplayName;
    void RefreshVisual();
};
