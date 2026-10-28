#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AIHWorldNode.generated.h"

class USceneComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UPointLightComponent;

UENUM(BlueprintType)
enum class EAIHNodeKind : uint8
{
    Core, Ore, Energy, Data, Neutral, Enemy
};

UCLASS()
class AIHEROES_API AAIHWorldNode : public AActor
{
    GENERATED_BODY()

public:
    AAIHWorldNode();
    void Configure(EAIHNodeKind NewKind, const FString& NewName, int32 NewThreat);
    void Capture();
    bool IsCaptured() const { return bCaptured; }
    EAIHNodeKind GetKind() const { return Kind; }
    int32 GetThreat() const { return Threat; }
    void SetCell(FIntPoint NewCell) { Cell = NewCell; }
    FIntPoint GetCell() const { return Cell; }
    FString Describe() const;
    FString GetNodeName() const { return NodeName; }

private:
    void RefreshAppearance();

    UPROPERTY(VisibleAnywhere)
    TObjectPtr<USceneComponent> Scene;
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UTextRenderComponent> Caption;
    UPROPERTY(VisibleAnywhere)
    TObjectPtr<UPointLightComponent> Beacon;

    EAIHNodeKind Kind = EAIHNodeKind::Neutral;
    FString NodeName;
    int32 Threat = 0;
    FIntPoint Cell = FIntPoint(3, 3);
    bool bCaptured = false;
};
