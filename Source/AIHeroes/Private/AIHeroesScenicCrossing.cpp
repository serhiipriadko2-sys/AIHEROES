#include "AIHeroesScenicCrossing.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"

bool FAIHeroesScenicCrossing::CalculateTransform(
    const FVector& RoadStart, const FVector& RoadEnd, FTransform& Out)
{
    Out=FTransform::Identity;
    if(RoadStart.ContainsNaN()||RoadEnd.ContainsNaN())return false;
    if(FMath::Abs(RoadStart.X)>5000.f||FMath::Abs(RoadStart.Y)>5000.f||
       FMath::Abs(RoadEnd.X)>5000.f||FMath::Abs(RoadEnd.Y)>5000.f)return false;
    const FVector2D Road(RoadEnd.X-RoadStart.X,RoadEnd.Y-RoadStart.Y);
    if(Road.SizeSquared()<200.f*200.f || Road.SizeSquared()>2000.f*2000.f)return false;
    // The river flows perpendicular to the road. The bridge's local Y
    // axis lies along that exact road, so the visible bridge is not a fake link.
    const float RiverDegrees=FMath::RadiansToDegrees(
        FMath::Atan2(Road.X,-Road.Y));
    const FVector Mid=(RoadStart+RoadEnd)*.5f;
    Out=FTransform(FRotator(0,RiverDegrees,0),
        FVector(Mid.X,Mid.Y,0.f),FVector(1.f));
    return !Out.ContainsNaN();
}

bool FAIHeroesScenicCrossing::Attach(
    AActor& Environment, const FVector& RoadStart, const FVector& RoadEnd)
{
    USceneComponent* Root=Environment.GetRootComponent();
    FTransform Crossing;
    if(!Root||!Environment.GetWorld()||
        !CalculateTransform(RoadStart,RoadEnd,Crossing))return false;
    struct FPart{const TCHAR* Name;const TCHAR* MeshName;};
    const FPart Parts[]={
        {TEXT("RiverV4_Silverbrook"),TEXT("SM_Silverbrook")},
        {TEXT("RiverV4_BrookBridge"),TEXT("SM_BrookBridge")}
    };
    // Load assets before making any components: failed imports leave no partial crossing.
    UStaticMesh* Meshes[2]={nullptr,nullptr};
    for(int I=0;I<2;++I)
    {
        Meshes[I]=LoadObject<UStaticMesh>(nullptr,
          *FString::Printf(TEXT("/Game/AmberMarch/Meshes/%s.%s"),
            Parts[I].MeshName,Parts[I].MeshName));
        if(!Meshes[I])return false;
    }
    TArray<UStaticMeshComponent*> Existing;
    Environment.GetComponents(Existing);
    for(int32 I=0;I<2;++I)
    {
        const FName ComponentName(Parts[I].Name);
        UStaticMeshComponent* C=nullptr;
        for(auto* Item:Existing)
            if(Item&&Item->GetFName()==ComponentName){C=Item;break;}
        if(!C)
        {
            C=NewObject<UStaticMeshComponent>(&Environment,ComponentName);
            Environment.AddInstanceComponent(C);
            C->SetupAttachment(Root);
            C->SetMobility(EComponentMobility::Movable);
            C->SetStaticMesh(Meshes[I]);
            C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            C->SetGenerateOverlapEvents(false);
            C->SetCanEverAffectNavigation(false);
            C->SetCastShadow(I==1);
            C->SetRelativeTransform(Crossing);
            C->RegisterComponent();
        }
        if(C->GetStaticMesh()!=Meshes[I]||
           C->GetCollisionEnabled()!=ECollisionEnabled::NoCollision||
           C->GetGenerateOverlapEvents()||C->CanEverAffectNavigation())
            return false;
    }
    return true;
}
void FAIHeroesScenicCrossing::SetShown(AActor& Environment, bool bShown)
{
    TArray<UStaticMeshComponent*> Components;
    Environment.GetComponents(Components);
    for(auto* Component:Components)
    {
        if(Component && Component->GetName().StartsWith(TEXT("RiverV4_")))
            Component->SetVisibility(bShown);
    }
}