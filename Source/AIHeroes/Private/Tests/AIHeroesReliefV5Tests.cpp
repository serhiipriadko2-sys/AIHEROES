#include "AIHeroesReliefV5.h"
#include <limits>

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReliefV5Math,
    "AIHeroes.Visual.ReliefV5.RiverDepthAndSitePads",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FReliefV5Math::RunTest(const FString&)
{
    const FVector2D Sites[]={{-650,0},{-100,-650},{150,600},{700,-300},{750,650}};
    for(const auto& At:Sites)
        TestTrue(TEXT("All strategic pads remain at original grade"),
            FMath::Abs(FAIHeroesReliefV5::HeightCm(At.X,At.Y)+12.0)<.0001);
    TestTrue(TEXT("River bed is recessed below opaque water"),
        FAIHeroesReliefV5::HeightCm(-375,-325)<-40);
    TestTrue(TEXT("Western heights rise above starting grade"),
        FAIHeroesReliefV5::HeightCm(-1100,800)>100);
    TestTrue(TEXT("Southern mountain has real elevation"),
        FAIHeroesReliefV5::HeightCm(-1000,-1000)>110);
    TestTrue(TEXT("Invalid sample has safe fallback"),
        FMath::IsFinite(FAIHeroesReliefV5::HeightCm(std::numeric_limits<double>::quiet_NaN(),0)));
    TestEqual(TEXT("Deterministic terrain sampling"),
        FAIHeroesReliefV5::HeightCm(-730,1100),
        FAIHeroesReliefV5::HeightCm(-730,1100));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReliefV5Meshes,
    "AIHeroes.Visual.ReliefV5.DesktopMobileAssets",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FReliefV5Meshes::RunTest(const FString&)
{
    for(const TCHAR* Name:{TEXT("SM_IslandReliefV5"),TEXT("SM_IslandReliefV5_Mobile")})
    {
        const FString Path=FString::Printf(TEXT("/Game/AmberMarch/Meshes/%s.%s"),Name,Name);
        auto* Mesh=LoadObject<UStaticMesh>(nullptr,*Path);
        if(TestNotNull(*Path,Mesh))
        {
            TestTrue(TEXT("Island has world-scale geometry"),
                Mesh->GetBounds().BoxExtent.Size()>2000);
            TestTrue(TEXT("Island uses four original palette materials"),
                Mesh->GetStaticMaterials().Num()>=4);
        }
    }
#if PLATFORM_ANDROID
    TestEqual(TEXT("Android asset selection"),FString(FAIHeroesReliefV5::PlatformMeshName()),
              FString(TEXT("SM_IslandReliefV5_Mobile")));
#else
    TestEqual(TEXT("PC asset selection"),FString(FAIHeroesReliefV5::PlatformMeshName()),
              FString(TEXT("SM_IslandReliefV5")));
#endif
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FReliefV5VisualSafe,
    "AIHeroes.Visual.ReliefV5.DecorativeIslandNonColliding",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FReliefV5VisualSafe::RunTest(const FString&)
{
    const UWorld::InitializationValues Settings=UWorld::InitializationValues()
        .AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false)
        .CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,
        nullptr,true,ERHIFeatureLevel::Num,&Settings);
    if(!TestNotNull(TEXT("Disposable world"),World))return false;
    AActor* Actor=World->SpawnActor<AActor>();
    if(TestNotNull(TEXT("Terrain host"),Actor))
    {
        auto* Root=NewObject<USceneComponent>(Actor,TEXT("ReliefTestRoot"));
        Actor->SetRootComponent(Root);
        Actor->AddInstanceComponent(Root);Root->RegisterComponent();
        auto* Visual=NewObject<UStaticMeshComponent>(Actor,TEXT("ReliefTestVisual"));
        Visual->SetupAttachment(Root);
        Actor->AddInstanceComponent(Visual);
        Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Visual->SetGenerateOverlapEvents(false);
        Visual->SetCanEverAffectNavigation(false);
        Visual->RegisterComponent();
        TestEqual(TEXT("Decorative only, no physics"),Visual->GetCollisionEnabled(),
            ECollisionEnabled::NoCollision);
        TestFalse(TEXT("No navigation"),Visual->CanEverAffectNavigation());
        TestFalse(TEXT("No overlaps"),Visual->GetGenerateOverlapEvents());
    }
    World->DestroyWorld(false);
    return true;
}
#endif