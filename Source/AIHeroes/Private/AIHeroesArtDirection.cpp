#include "AIHeroesArtDirection.h"
#include "AIHeroesScenicCoast.h"
#include "AIHeroesSceneryV3.h"
#include "AIHeroesScenicCrossing.h"
#include "AIHeroesTravelRules.h"
#include "AIHeroesWorldNode.h"
#include "AIHeroesGameMode.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"
#include "Math/RandomStream.h"

namespace AmberArt
{
UStaticMesh* Mesh(const TCHAR* Name)
{
    return LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/AmberMarch/Meshes/%s.%s"),Name,Name));
}
UStaticMeshComponent* Decor(AActor& Owner,const TCHAR* Name,UStaticMesh* M,USceneComponent* Parent)
{
    if(!M||!Parent)return nullptr;
    auto* C=NewObject<UStaticMeshComponent>(&Owner,FName(Name));
    Owner.AddInstanceComponent(C);C->SetupAttachment(Parent);C->SetMobility(EComponentMobility::Movable);
    C->SetStaticMesh(M);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    C->SetGenerateOverlapEvents(false);C->SetCanEverAffectNavigation(false);C->RegisterComponent();return C;
}
}

bool FAIHeroesArtDirection::ApplyNode(AAIHeroesWorldNode& N)
{
    using namespace AmberArt;
    const TCHAR* Name=nullptr;
    switch(N.NodeType){
        case EAIHeroesNodeType::Headquarters:Name=TEXT("SM_Hearth");break;
        case EAIHeroesNodeType::ForestMine:Name=TEXT("SM_Sawmill");break;
        case EAIHeroesNodeType::OreMine:Name=TEXT("SM_Quarry");break;
        case EAIHeroesNodeType::Outpost:Name=TEXT("SM_Watchtower");break;
        case EAIHeroesNodeType::Ruins:Name=TEXT("SM_Archive");break;
        default:return false;
    }
    TArray<UStaticMeshComponent*> Components;N.GetComponents(Components);
    UStaticMeshComponent* Art=nullptr;
    for(auto* C:Components)if(C->GetFName()==FName(TEXT("AmberSite")))Art=C;
    if(!Art)Art=Decor(N,TEXT("AmberSite"),Mesh(Name),N.GetRootComponent());
    if(!Art)return false;
    Art->SetRelativeLocation(FVector(0,0,-N.GetActorLocation().Z));
    // Keep the old visibility-channel collider, but remove its grey rendering.
    N.Visual->SetVisibility(false,false);
    N.Beacon->SetIntensity(0.f);N.Beacon->SetCastShadows(false);
    if(auto* Stone=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/AmberMarch/Materials/M_Slate.M_Slate")))
        for(auto* C:Components)if(C->GetName().StartsWith(TEXT("HQUpgradePart")))C->SetMaterial(0,Stone);
    return true;
}

void FAIHeroesArtDirection::ApplyWorld(AAIHeroesGameMode& G)
{
    using namespace AmberArt;
    UWorld* W=G.GetWorld();if(!W)return;
    for(TActorIterator<AActor> It(W);It;++It)if(It->GetOwner()==&G&&It->ActorHasTag(TEXT("AmberEnvironment")))return;
    auto* Island=Mesh(TEXT("SM_Island"));if(!Island)return;
    FActorSpawnParameters P;P.Owner=&G;
    auto* Environment=W->SpawnActor<AActor>(FVector::ZeroVector,FRotator::ZeroRotator,P);
    if(!Environment)return;Environment->Tags.Add(TEXT("AmberEnvironment"));
    auto* Root=NewObject<USceneComponent>(Environment,TEXT("AmberRoot"));
    Environment->SetRootComponent(Root);Environment->AddInstanceComponent(Root);Root->RegisterComponent();
    auto* Ground=Decor(*Environment,TEXT("Island"),Island,Root);
    if(Ground)Ground->SetRelativeLocation(FVector(0,0,3));
    auto* Plane=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane"));
    if(auto* Sea=Decor(*Environment,TEXT("Sea"),Plane,Root))
    {
        Sea->SetRelativeLocation(FVector(0,0,-145));Sea->SetRelativeScale3D(FVector(400,400,1));
        Sea->SetCastShadow(false);
        if(auto* M=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/AmberMarch/Materials/M_Water.M_Water")))Sea->SetMaterial(0,M);
    }
    const FVector2D Sites[]={FVector2D(-650,0),FVector2D(-100,-650),FVector2D(150,600),FVector2D(700,-300),FVector2D(750,650)};
    auto* Pine=Mesh(TEXT("SM_Pine"));
    if(Pine)
    {
        auto* Forest=NewObject<UInstancedStaticMeshComponent>(Environment,TEXT("PineGroves"));
        Environment->AddInstanceComponent(Forest);Forest->SetupAttachment(Root);Forest->SetStaticMesh(Pine);
        Forest->SetMobility(EComponentMobility::Movable);Forest->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Forest->SetCanEverAffectNavigation(false);Forest->RegisterComponent();
        FRandomStream R(20261029);
        for(int32 I=0;I<110;++I)
        {
            const FVector2D At(R.FRandRange(-1450,1450),R.FRandRange(-1450,1450));
            bool Clear=true;for(const auto& S:Sites)if(FVector2D::Distance(At,S)<340)Clear=false;
            // Preserve the two starting road corridors and the board centre.
            if(At.X>-750&&At.X<450&&FMath::Abs(At.Y)<360)Clear=false;
            if(!Clear)continue;
            const float S=R.FRandRange(.65f,1.1f);
            Forest->AddInstance(FTransform(FRotator(0,R.FRandRange(0,360),0),FVector(At.X,At.Y,-8),FVector(S)),false);
        }
    }
    FAIHeroesScenicCoast::Attach(*Environment);
    const bool bSceneryAttached=FAIHeroesSceneryV3::Attach(*Environment);
    // Visible crossing is permitted only on the actual discovered Road V2 link.
    if(G.TravelRuleVersion==FAIHeroesTravelRules::RoadVersion &&
       G.IsNodeDiscovered(FName(TEXT("FOREST"))))
    {
        const AAIHeroesWorldNode* Hq=nullptr;
        const AAIHeroesWorldNode* ForestSite=nullptr;
        for(const auto& Node:G.Nodes)
        {
            if(!IsValid(Node.Get()))continue;
            if(Node->GetDisplayName()==TEXT("HQ"))Hq=Node.Get();
            else if(Node->GetDisplayName()==TEXT("FOREST"))ForestSite=Node.Get();
        }
        if(Hq&&ForestSite)
        {
            const bool bCrossing=FAIHeroesScenicCrossing::Attach(
                *Environment,Hq->GetActorLocation(),ForestSite->GetActorLocation());
            UE_LOG(LogTemp,Display,TEXT("[AMBER RIVER V4] bridge_on_known_road=%d"),
                bCrossing?1:0);
        }
    }
    UE_LOG(LogTemp,Display,TEXT("[AMBER SCENERY V3] attached=%d visual_only=1"),bSceneryAttached?1:0);
    if(IsValid(G.HeroMarker.Get()))
    {
        auto* Hero=Decor(*G.HeroMarker,TEXT("AmberHero"),Mesh(TEXT("SM_Seeker")),G.HeroMarker->GetRootComponent());
        if(Hero){Hero->SetRelativeLocation(FVector(0,0,-360));Hero->SetRelativeScale3D(FVector(1.3f));G.HeroMarker->GetStaticMeshComponent()->SetVisibility(false,false);}
    }
    UE_LOG(LogTemp,Display,TEXT("[AMBER ART] original_meshes=9 environment_collision=0 gameplay_mutations=0"));
}
void FAIHeroesArtDirection::RemoveWorld(AAIHeroesGameMode& G)
{
    if(!G.GetWorld())return;TArray<AActor*> Remove;
    for(TActorIterator<AActor> It(G.GetWorld());It;++It)if(It->GetOwner()==&G&&It->ActorHasTag(TEXT("AmberEnvironment")))Remove.Add(*It);
    for(auto* A:Remove)A->Destroy();
}
