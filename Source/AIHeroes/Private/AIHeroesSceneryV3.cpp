#include "AIHeroesSceneryV3.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"

TArray<FTransform> FAIHeroesSceneryV3::RidgeTransforms()
{
    const FVector2D Places[]={
        {-1540,-1150},{-1490,-440},{-1380,1010},{-940,1470},
        {1430,-1450},{1540,-420},{1520,520},{1190,1430}
    };
    TArray<FTransform> Out;Out.Reserve(8);
    for(int32 I=0;I<8;++I)
    {
        const float Scale=.55f+.035f*(I%3);
        Out.Add(FTransform(FRotator(0,I*43.f,0),
            FVector(Places[I].X,Places[I].Y,-100.f),
            FVector(Scale,Scale,.72f+(I%3)*.06f)));
    }
    return Out;
}
TArray<FTransform> FAIHeroesSceneryV3::WaystoneTransforms()
{
    return {
        FTransform(FRotator(0,18,0),FVector(-500,-260,-12),FVector(.70)),
        FTransform(FRotator(0,-28,0),FVector(-380,280,-12),FVector(.70))
    };
}
FTransform FAIHeroesSceneryV3::WaystationTransform()
{
    return FTransform(FRotator(0,34,0),FVector(-1070,260,-12),FVector(.76));
}
bool FAIHeroesSceneryV3::Attach(AActor& Environment)
{
    auto* Root=Environment.GetRootComponent();
    if(!Root||!Environment.GetWorld())return false;
    struct FGroup{const TCHAR* Mesh;const TCHAR* Prefix;TArray<FTransform> Poses;};
    const FGroup Groups[]={
        {TEXT("SM_RidgeOutcrop"),TEXT("ScenicRidge"),RidgeTransforms()},
        {TEXT("SM_Waystone"),TEXT("ScenicWaystone"),WaystoneTransforms()},
        {TEXT("SM_Waystation"),TEXT("ScenicWaystation"),{WaystationTransform()}}
    };
    for(const auto& Group:Groups)
    {
        auto* Mesh=LoadObject<UStaticMesh>(nullptr,
            *FString::Printf(TEXT("/Game/AmberMarch/Meshes/%s.%s"),Group.Mesh,Group.Mesh));
        if(!Mesh)return false;
        for(int32 I=0;I<Group.Poses.Num();++I)
        {
            const FName Name(*FString::Printf(TEXT("%s_%02d"),Group.Prefix,I));
            TArray<UStaticMeshComponent*> All;
            Environment.GetComponents(All);
            UStaticMeshComponent* C=nullptr;
            for(auto* Current:All)if(Current->GetFName()==Name){C=Current;break;}
            if(!C)
            {
                C=NewObject<UStaticMeshComponent>(&Environment,Name);
                Environment.AddInstanceComponent(C);
                C->SetupAttachment(Root);
                C->SetMobility(EComponentMobility::Movable);
                C->SetStaticMesh(Mesh);
                C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                C->SetGenerateOverlapEvents(false);
                C->SetCanEverAffectNavigation(false);
                C->SetRelativeTransform(Group.Poses[I]);
                C->RegisterComponent();
            }
            if(C->GetStaticMesh()!=Mesh ||
               C->GetCollisionEnabled()!=ECollisionEnabled::NoCollision)return false;
        }
    }
    return true;
}