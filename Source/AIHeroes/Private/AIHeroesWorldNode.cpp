#include "AIHeroesWorldNode.h"
#include "AIHeroesBaseRules.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "UObject/ConstructorHelpers.h"

AAIHeroesWorldNode::AAIHeroesWorldNode()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    RootComponent = SceneRoot;
    Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
    Visual->SetupAttachment(SceneRoot);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Cube.Succeeded())
    {
        Visual->SetStaticMesh(Cube.Object);
    }
    Visual->SetCollisionResponseToAllChannels(ECR_Ignore);
    Visual->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

    for (int32 Index = 0; Index < 3; ++Index)
    {
        UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(
            *FString::Printf(TEXT("HQUpgradePart%d"), Index));
        Part->SetupAttachment(SceneRoot);
        Part->SetMobility(EComponentMobility::Movable);
        if (Cube.Succeeded()) Part->SetStaticMesh(Cube.Object);
        Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Part->SetVisibility(false);
        Part->SetRelativeLocation(Index < 2
            ? FVector(-170.f, Index == 0 ? -180.f : 180.f, 55.f) : FVector(60.f, 0.f, 235.f));
        Part->SetRelativeScale3D(Index < 2 ? FVector(0.8f, 0.8f, 4.f) : FVector(0.35f, 0.35f, 1.6f));
        HeadquartersParts.Add(Part);
    }

    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("NameLabel"));
    Label->SetupAttachment(SceneRoot); // Do not inherit the scaled mesh transform.
    Label->SetRelativeLocation(FVector(0.f, 0.f, 255.f));
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetWorldSize(32.f);
    Label->SetCastShadow(false); // Labels are now rendered cleanly in the HUD.
    Label->SetVisibility(false);

    Beacon = CreateDefaultSubobject<UPointLightComponent>(TEXT("Beacon"));
    Beacon->SetupAttachment(SceneRoot);
    Beacon->SetRelativeLocation(FVector(0.f, 0.f, 220.f));
    Beacon->SetIntensity(1600.f);
    Beacon->SetAttenuationRadius(520.f);
}

void AAIHeroesWorldNode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!GetWorld())
    {
        return;
    }
    const APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC && PC->PlayerCameraManager)
    {
        const FVector ToCamera =
            PC->PlayerCameraManager->GetCameraLocation() - Label->GetComponentLocation();
        Label->SetWorldRotation(FRotator(0.f, ToCamera.Rotation().Yaw, 0.f));
    }
}

void AAIHeroesWorldNode::Configure(
    EAIHeroesNodeType InType,
    const FString& InName,
    const TArray<FAIHeroesArmyStack>& InDefenders)
{
    NodeType = InType;
    DisplayName = InName;
    Defenders = InDefenders;
    bOwned = (InType == EAIHeroesNodeType::Headquarters);
    RefreshVisual();
}

void AAIHeroesWorldNode::SetOwned(bool bNewOwned)
{
    bOwned = bNewOwned;
    RefreshVisual();
}

FString AAIHeroesWorldNode::GetDisplayName() const
{
    return DisplayName;
}

bool AAIHeroesWorldNode::SetHeadquartersTier(int32 Tier)
{
    if (NodeType != EAIHeroesNodeType::Headquarters || !FAIHeroesBaseRules::IsValidTier(Tier)) return false;
    HeadquartersTier = Tier;
    RefreshVisual();
    return true;
}

int32 AAIHeroesWorldNode::GetVisibleHeadquartersParts() const
{
    int32 Count = 0;
    for (const UStaticMeshComponent* Part : HeadquartersParts)
        if (IsValid(Part) && Part->IsVisible()) ++Count;
    return Count;
}

void AAIHeroesWorldNode::RefreshVisual()
{
    FVector Scale(1.8f, 1.8f, 2.0f);
    switch (NodeType)
    {
        case EAIHeroesNodeType::Headquarters: Scale = FVector(3.f, 3.f, 3.f); break;
        case EAIHeroesNodeType::ForestMine: Scale = FVector(1.5f, 2.f, 2.8f); break;
        case EAIHeroesNodeType::OreMine: Scale = FVector(2.f, 1.5f, 2.f); break;
        case EAIHeroesNodeType::Outpost: Scale = FVector(2.2f, 2.2f, 2.2f); break;
        case EAIHeroesNodeType::Ruins: Scale = FVector(2.8f, 1.8f, 3.4f); break;
    }
    Visual->SetRelativeScale3D(Scale);
    for (int32 Index = 0; Index < HeadquartersParts.Num(); ++Index)
        HeadquartersParts[Index]->SetVisibility(NodeType == EAIHeroesNodeType::Headquarters &&
            bOwned && Index < HeadquartersTier);
    Label->SetText(FText::FromString(FString::Printf(
        TEXT("%s%s"), *DisplayName, bOwned ? TEXT(" [OURS]") : TEXT(""))));
    const FColor StateColor = bOwned ? FColor(70, 255, 150) :
        (Defenders.IsEmpty() ? FColor(240, 240, 100) : FColor(255, 125, 100));
    Label->SetTextRenderColor(StateColor);
    Beacon->SetLightColor(FLinearColor(StateColor));
}
