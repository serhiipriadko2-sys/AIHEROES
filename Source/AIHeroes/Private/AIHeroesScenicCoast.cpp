#include "AIHeroesScenicCoast.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/StaticMesh.h"
#include "Math/RandomStream.h"

TArray<FTransform> FAIHeroesScenicCoast::Transforms()
{
    TArray<FTransform> Out;Out.Reserve(64);FRandomStream Random(710301);
    for(int32 Side=0;Side<4;++Side)
    for(int32 I=0;I<16;++I)
    {
        const float T=-1550.f+I*(3100.f/15.f);
        const float Edge=1610.f+Random.FRandRange(-38.f,38.f);
        const FVector At=Side==0?FVector(-Edge,T,-175):Side==1?FVector(Edge,T,-175):
            Side==2?FVector(T,-Edge,-175):FVector(T,Edge,-175);
        Out.Add(FTransform(FRotator(Random.FRandRange(-8,8),Random.FRandRange(0,360),0),At,
            FVector(Random.FRandRange(2.3f,3.1f),Random.FRandRange(2.f,2.8f),Random.FRandRange(2.4f,3.4f))));
    }
    return Out;
}
bool FAIHeroesScenicCoast::Attach(AActor& Environment)
{
    if(!Environment.GetRootComponent()||!Environment.GetWorld())return false;
    TArray<UInstancedStaticMeshComponent*> Existing;Environment.GetComponents(Existing);
    for(auto* C:Existing)if(C->GetFName()==FName(TEXT("ReferenceCoast")))return true;
    auto* Mesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/AmberMarch/Meshes/SM_Boulder.SM_Boulder"));
    if(!Mesh)return false;
    auto* C=NewObject<UInstancedStaticMeshComponent>(&Environment,TEXT("ReferenceCoast"));
    Environment.AddInstanceComponent(C);C->SetupAttachment(Environment.GetRootComponent());
    C->SetStaticMesh(Mesh);C->SetMobility(EComponentMobility::Movable);
    C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetGenerateOverlapEvents(false);
    C->SetCanEverAffectNavigation(false);C->RegisterComponent();
    for(const auto& T:Transforms())C->AddInstance(T,false);
    return C->GetInstanceCount()==64;
}
