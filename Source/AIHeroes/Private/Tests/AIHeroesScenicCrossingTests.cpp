#include "AIHeroesScenicCrossing.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FScenicCrossingTopology,
    "AIHeroes.Visual.RiverV4.AlignedWithVerifiedRoad",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FScenicCrossingTopology::RunTest(const FString&)
{
    const FVector HQ(-650,0,155);
    const FVector Forest(-100,-650,110);
    FTransform T;
    TestTrue(TEXT("Current V2 HQ-FOREST segment accepted"),
        FAIHeroesScenicCrossing::CalculateTransform(HQ,Forest,T));
    TestTrue(TEXT("River-crossing anchored to actual route midpoint"),
        FVector2D::Distance(FVector2D(T.GetLocation().X,T.GetLocation().Y),
            FVector2D((HQ.X+Forest.X)*.5,(HQ.Y+Forest.Y)*.5))<.01);
    const FVector CrossAxis=T.GetRotation().RotateVector(FVector::YAxisVector);
    const FVector Travel=(Forest-HQ).GetSafeNormal2D();
    TestTrue(TEXT("Bridge span aligned to existing road"),
        FMath::Abs(FVector::DotProduct(CrossAxis,Travel))>.999);
    FTransform Out;
    TestFalse(TEXT("Coincident nodes refused"),
        FAIHeroesScenicCrossing::CalculateTransform(HQ,HQ,Out));
    TestFalse(TEXT("Missing/invalid coordinates refused"),
        FAIHeroesScenicCrossing::CalculateTransform(
            FVector(std::numeric_limits<float>::quiet_NaN(),0,0),Forest,Out));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FScenicCrossingAssets,
    "AIHeroes.Visual.RiverV4.ImportedMaterialsAndGeometry",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FScenicCrossingAssets::RunTest(const FString&)
{
    for(const TCHAR* Name:{TEXT("SM_Silverbrook"),TEXT("SM_BrookBridge")})
    {
        const FString Path=FString::Printf(TEXT("/Game/AmberMarch/Meshes/%s.%s"),Name,Name);
        auto* Mesh=LoadObject<UStaticMesh>(nullptr,*Path);
        if(TestNotNull(*Path,Mesh))
        {
            TestTrue(TEXT("Mesh has measurable extent"),
                Mesh->GetBounds().BoxExtent.Size()>100.0);
            TestTrue(TEXT("Multiple hand-authored palette materials"),
                Mesh->GetStaticMaterials().Num()>=2);
        }
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FScenicCrossingNoAuthority,
    "AIHeroes.Visual.RiverV4.VisualOnlyIdempotent",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FScenicCrossingNoAuthority::RunTest(const FString&)
{
    const UWorld::InitializationValues Settings=UWorld::InitializationValues()
        .AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false)
        .CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,
        nullptr,true,ERHIFeatureLevel::Num,&Settings);
    if(!TestNotNull(TEXT("Disposable world"),World))return false;
    auto* Host=World->SpawnActor<AActor>();
    if(TestNotNull(TEXT("Decorative host"),Host))
    {
        auto* Root=NewObject<USceneComponent>(Host,TEXT("RiverTestRoot"));
        Host->SetRootComponent(Root);
        Host->AddInstanceComponent(Root);
        Root->RegisterComponent();
        const FVector HQ(-650,0,155),Forest(-100,-650,110);
        TestTrue(TEXT("First attachment"),FAIHeroesScenicCrossing::Attach(*Host,HQ,Forest));
        TestTrue(TEXT("Repeated attachment"),FAIHeroesScenicCrossing::Attach(*Host,HQ,Forest));
        FAIHeroesScenicCrossing::SetShown(*Host,false);
        TArray<UStaticMeshComponent*> HideCheck;
        Host->GetComponents(HideCheck);
        for(auto* Mesh:HideCheck)TestFalse(TEXT("Legacy road hides crossing"),Mesh->IsVisible());
        FAIHeroesScenicCrossing::SetShown(*Host,true);
        for(auto* Mesh:HideCheck)TestTrue(TEXT("V2 road restores crossing"),Mesh->IsVisible());
        TArray<UStaticMeshComponent*> Items;
        Host->GetComponents(Items);
        TestEqual(TEXT("Exactly two visual meshes"),Items.Num(),2);
        for(auto* Mesh:Items)
        {
            TestEqual(TEXT("No physics collision"),Mesh->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
            TestFalse(TEXT("No overlap events"),Mesh->GetGenerateOverlapEvents());
            TestFalse(TEXT("No navigation authority"),Mesh->CanEverAffectNavigation());
        }
    }
    World->DestroyWorld(false);
    return true;
}
#endif