#include "AIHeroesSceneryV3.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSceneryV3Transforms,
    "AIHeroes.Visual.SceneryV3.DeterministicSafeLocations",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSceneryV3Transforms::RunTest(const FString&)
{
    const auto A=FAIHeroesSceneryV3::RidgeTransforms();
    const auto B=FAIHeroesSceneryV3::RidgeTransforms();
    TestEqual(TEXT("Eight ridge features"),A.Num(),8);
    for(int32 I=0;I<A.Num()&&I<B.Num();++I)
        TestTrue(TEXT("Deterministic transforms"),A[I].Equals(B[I]));
    const FVector2D Sites[]={{-650,0},{-100,-650},{150,600},{700,-300},{750,650}};
    for(const auto& T:A)
    {
        TestFalse(TEXT("Finite world coordinates"),T.ContainsNaN());
        const FVector P=T.GetLocation();
        for(const auto& S:Sites)
            TestTrue(TEXT("Ridges away from strategic nodes"),
                FVector2D::Distance(FVector2D(P.X,P.Y),S)>670.);
        TestTrue(TEXT("Limited detail scale"),T.GetScale3D().GetMax()<=.9);
    }
    TestEqual(TEXT("Two civic markers"),
        FAIHeroesSceneryV3::WaystoneTransforms().Num(),2);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSceneryV3AssetTest,
    "AIHeroes.Visual.SceneryV3.OriginalAssetsLoaded",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSceneryV3AssetTest::RunTest(const FString&)
{
    for(const TCHAR* Name:{TEXT("SM_RidgeOutcrop"),TEXT("SM_Waystone"),TEXT("SM_Waystation")})
    {
        const FString Asset=FString::Printf(TEXT("/Game/AmberMarch/Meshes/%s.%s"),Name,Name);
        auto* Mesh=LoadObject<UStaticMesh>(nullptr,*Asset);
        if(TestNotNull(*Asset,Mesh))
        {
            TestTrue(TEXT("Real-sized 3D geometry"),Mesh->GetBounds().BoxExtent.Size()>50.);
            TestTrue(TEXT("Materials assigned"),Mesh->GetStaticMaterials().Num()>=2);
        }
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSceneryV3AttachTest,
    "AIHeroes.Visual.SceneryV3.NonInteractiveAndIdempotent",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FSceneryV3AttachTest::RunTest(const FString&)
{
    const UWorld::InitializationValues Values=UWorld::InitializationValues()
        .AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false)
        .CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,
                                     nullptr,true,ERHIFeatureLevel::Num,&Values);
    if(!TestNotNull(TEXT("Disposable world"),World))return false;
    auto* Owner=World->SpawnActor<AActor>();
    if(TestNotNull(TEXT("Scenic host"),Owner))
    {
        auto* Root=NewObject<USceneComponent>(Owner,TEXT("ScenicTestRoot"));
        Owner->SetRootComponent(Root);
        Owner->AddInstanceComponent(Root);Root->RegisterComponent();
        TestTrue(TEXT("First attach"),FAIHeroesSceneryV3::Attach(*Owner));
        TestTrue(TEXT("Repeated attach"),FAIHeroesSceneryV3::Attach(*Owner));
        TArray<UStaticMeshComponent*> Components;
        Owner->GetComponents(Components);
        TestEqual(TEXT("Exactly eleven decorative components"),Components.Num(),11);
        for(auto* Component:Components)
        {
            TestEqual(TEXT("No collision"),Component->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
            TestFalse(TEXT("No overlaps"),Component->GetGenerateOverlapEvents());
            TestFalse(TEXT("No nav obstacle"),Component->CanEverAffectNavigation());
        }
    }
    World->DestroyWorld(false);
    return true;
}
#endif