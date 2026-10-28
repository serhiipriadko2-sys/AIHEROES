#include "AIHWorldNode.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"

AAIHWorldNode::AAIHWorldNode()
{
    PrimaryActorTick.bCanEverTick = false;
    Scene = CreateDefaultSubobject<USceneComponent>(TEXT("Scene"));
    RootComponent = Scene;
    Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
    Body->SetupAttachment(Scene);
    Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Body->SetCollisionResponseToAllChannels(ECR_Ignore);
    Body->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    Caption = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Caption"));
    Caption->SetupAttachment(Scene);
    Caption->SetRelativeLocation(FVector(0.f, 0.f, 150.f));
    Caption->SetWorldSize(30.f);
    Caption->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
    Beacon = CreateDefaultSubobject<UPointLightComponent>(TEXT("Beacon"));
    Beacon->SetupAttachment(Scene);
    Beacon->SetRelativeLocation(FVector(0.f, 0.f, 110.f));
    Beacon->SetIntensity(2500.f);
    Beacon->SetAttenuationRadius(320.f);
}

void AAIHWorldNode::Configure(
    EAIHNodeKind NewKind, const FString& NewName, int32 NewThreat)
{
    Kind = NewKind;
    NodeName = NewName;
    Threat = FMath::Max(0, NewThreat);
    bCaptured = NewKind == EAIHNodeKind::Core;
    RefreshAppearance();
}

void AAIHWorldNode::Capture()
{
    bCaptured = true;
    Threat = 0;
    RefreshAppearance();
}

FString AAIHWorldNode::Describe() const
{
    return FString::Printf(TEXT("%s | threat %d | %s"),
        *NodeName, Threat, bCaptured ? TEXT("controlled") : TEXT("unclaimed"));
}

void AAIHWorldNode::RefreshAppearance()
{
    const bool bSphere = Kind == EAIHNodeKind::Energy || Kind == EAIHNodeKind::Data;
    const bool bCylinder = Kind == EAIHNodeKind::Ore || Kind == EAIHNodeKind::Neutral;
    const TCHAR* AssetPath = bSphere
        ? TEXT("/Engine/BasicShapes/Sphere.Sphere")
        : bCylinder ? TEXT("/Engine/BasicShapes/Cylinder.Cylinder")
        : TEXT("/Engine/BasicShapes/Cube.Cube");

    if (UStaticMesh* Shape = LoadObject<UStaticMesh>(nullptr, AssetPath))
    {
        Body->SetStaticMesh(Shape);
    }
    Body->SetRelativeScale3D(Kind == EAIHNodeKind::Core
        ? FVector(1.5f, 1.5f, 1.6f) : FVector(1.1f, 1.1f, 1.1f));
    Body->SetRelativeLocation(FVector(0.f, 0.f, 55.f));
    const FLinearColor Color = bCaptured ? FLinearColor(0.1f, 0.9f, 0.5f)
        : Kind == EAIHNodeKind::Enemy ? FLinearColor(1.f, 0.1f, 0.15f)
        : Kind == EAIHNodeKind::Ore ? FLinearColor(1.f, 0.45f, 0.1f)
        : Kind == EAIHNodeKind::Energy ? FLinearColor(1.f, 0.9f, 0.2f)
        : Kind == EAIHNodeKind::Data ? FLinearColor(0.6f, 0.3f, 1.f)
        : FLinearColor(0.6f, 0.75f, 1.f);
    Beacon->SetLightColor(Color);
    Caption->SetText(FText::FromString(
        NodeName + (bCaptured ? TEXT(" [OURS]") : TEXT(""))));
}
