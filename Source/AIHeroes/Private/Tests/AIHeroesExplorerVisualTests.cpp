#include "AIHeroesExplorerLayout.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AIHeroesMapLabelLayout.h"
#include "AIHeroesWorldNode.h"
#include "AIHeroesArtDirection.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include <limits>

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExplorerLayoutTest,"AIHeroes.Visual.Explorer.LayoutAndMapSpace",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FExplorerLayoutTest::RunTest(const FString&)
{
    for(auto V:{FVector2D(320,640),FVector2D(360,800),FVector2D(640,720),FVector2D(960,720),FVector2D(1280,720),FVector2D(1920,1080),FVector2D(2560,1440)})
    {
        const auto F=FAIHeroesExplorerLayout::Frame(V);TestTrue(TEXT("Supported screen"),F.bSupported);
        const FAIHeroesHUDActionRect Screen{0,0,static_cast<float>(V.X),static_cast<float>(V.Y)};
        for(auto R:{F.Header,F.Dock,F.Inspector,F.Menu,F.Map,F.Mission})
            TestTrue(TEXT("Panel inside screen"),FAIHeroesMapLabelLayout::ContainsRect(Screen,R));
        TestFalse(TEXT("Inspector does not cover map rectangle"),FAIHeroesMapLabelLayout::Overlaps(F.Inspector,F.Map,0));
        TestFalse(TEXT("Dock does not cover map rectangle"),FAIHeroesMapLabelLayout::Overlaps(F.Dock,F.Map,0));
        if(F.bWide)TestTrue(TEXT("Wide-screen map remains over half total area"),F.Map.W*F.Map.H>V.X*V.Y*.5);
    }
    for(auto V:{FVector2D(0,0),FVector2D(100,100),FVector2D(std::numeric_limits<double>::quiet_NaN(),720)})
        TestFalse(TEXT("Invalid small sizes fail closed"),FAIHeroesExplorerLayout::Frame(V).bSupported);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExplorerCompactArmyTest,
    "AIHeroes.Visual.Explorer.CompactArmyMapPanel",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FExplorerCompactArmyTest::RunTest(const FString&)
{
    for(const FVector2D V:{
        FVector2D(480,800),FVector2D(640,720),FVector2D(960,720),
        FVector2D(1024,640)})
    {
        const auto F=FAIHeroesExplorerLayout::Frame(V);
        TestTrue(TEXT("Compact supported map has visible army card"),
            F.CompactArmy.W>=250.f &&
            F.CompactArmy.H>=(V.X>=640.f?54.f:70.f));
        TestTrue(TEXT("Army card stays inside map"),
            FAIHeroesMapLabelLayout::ContainsRect(F.Map,F.CompactArmy));
        if(V.X==640.f || V.X==960.f)
            TestTrue(TEXT("First row of visible site labels stays below army card"),
                F.CompactArmy.Y+F.CompactArmy.H+4.f < F.Mission.Y+62.f);
        for(const auto R:{F.Header,F.Dock,F.Mission,F.Inspector,F.Overview,F.Hero})
            if(R.W>0.f)
                TestFalse(TEXT("Army never obscures another HUD panel"),
                    FAIHeroesMapLabelLayout::Overlaps(F.CompactArmy,R,0.f));
        const FVector2D Centre(F.CompactArmy.X+F.CompactArmy.W*.5f,
            F.CompactArmy.Y+F.CompactArmy.H*.5f);
        TestEqual(TEXT("Tapping army card cannot order movement or combat"),
            FAIHeroesExplorerLayout::Hit(Centre,V,false),
            FAIHeroesExplorerLayout::ConsumeAction);
        const FVector2D Mission(F.Mission.X+F.Mission.W*.5f,
            F.Mission.Y+F.Mission.H*.5f);
        TestEqual(TEXT("Goals ribbon retains its existing action"),
            FAIHeroesExplorerLayout::Hit(Mission,V,false),202);
    }
    for(const FVector2D V:{
        FVector2D(320,640),FVector2D(640,480),FVector2D(1280,720)})
    {
        const auto F=FAIHeroesExplorerLayout::Frame(V);
        TestEqual(TEXT("No armory overlay in too-small or existing hero-card layout"),
            F.CompactArmy.W,0.f);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExplorerHitTest,"AIHeroes.Visual.Explorer.ButtonsMatchHitAreas",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FExplorerHitTest::RunTest(const FString&)
{
    for(auto V:{FVector2D(360,800),FVector2D(640,720),FVector2D(1280,720)})
    for(bool Open:{false,true})
    {
        TArray<FAIHeroesHUDActionRect> Seen;
        for(int32 A:FAIHeroesExplorerLayout::Actions(Open))
        {
            const auto R=FAIHeroesExplorerLayout::Button(A,V,Open);
            TestTrue(TEXT("Every visible button has area"),R.W>=40&&R.H>=30);
            TestEqual(TEXT("Visual centre maps to same action"),FAIHeroesExplorerLayout::Hit({R.X+R.W*.5,R.Y+R.H*.5},V,Open),A);
            for(const auto& B:Seen)TestFalse(TEXT("No button overlaps"),FAIHeroesMapLabelLayout::Overlaps(R,B,0));
            Seen.Add(R);
        }
        const auto F=FAIHeroesExplorerLayout::Frame(V);
        TestEqual(TEXT("Header blocks click-through"),FAIHeroesExplorerLayout::Hit({2,2},V,Open),219);
        if(!Open)TestEqual(TEXT("Hidden save target has no area"),FAIHeroesExplorerLayout::Button(7,V,false).W,0.f);
    }
    return true;
}
// Independently test click protection at the REAL overlay edges, not an
// assumed fixed position. The compact card may move as the layout evolves.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExplorerCompactArmyHitTest,
    "AIHeroes.Visual.Explorer.CompactArmyOverlayStopsClickThrough",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FExplorerCompactArmyHitTest::RunTest(const FString&)
{
    for(const FVector2D V:{FVector2D(640,720),FVector2D(960,720)})
    {
        const auto F=FAIHeroesExplorerLayout::Frame(V);
        TestTrue(TEXT("Narrow PC screen has map and no wide Hero card"),
            F.bSupported && F.Map.W>0.f && F.Hero.W==0.f);
        if(!TestTrue(TEXT("Compact roster has a visible map rectangle"),
            F.CompactArmy.W>200.f && F.CompactArmy.H>=50.f))return false;
        for(const FVector2D Inside:{
            FVector2D(F.CompactArmy.X+2.f,F.CompactArmy.Y+2.f),
            FVector2D(F.CompactArmy.X+F.CompactArmy.W-2.f,
                      F.CompactArmy.Y+F.CompactArmy.H-2.f)})
        {
            TestEqual(TEXT("Overlay border pixels do not issue map orders"),
                FAIHeroesExplorerLayout::Hit(Inside,V,false),
                FAIHeroesExplorerLayout::ConsumeAction);
        }
        const FVector2D ClearMap(F.CompactArmy.X+F.CompactArmy.W*.5f,
            F.CompactArmy.Y-6.f);
        TestEqual(TEXT("Nearby map remains interactive"),
            FAIHeroesExplorerLayout::Hit(ClearMap,V,false),INDEX_NONE);
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExplorerArtTest,"AIHeroes.Visual.Explorer.OriginalAssetsLoaded",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FExplorerArtTest::RunTest(const FString&)
{
    for(const TCHAR* N:{TEXT("SM_Hearth"),TEXT("SM_Sawmill"),TEXT("SM_Quarry"),TEXT("SM_Watchtower"),TEXT("SM_Archive"),TEXT("SM_Seeker"),TEXT("SM_Pine"),TEXT("SM_Boulder"),TEXT("SM_Island")})
    {
        const auto P=FString::Printf(TEXT("/Game/AmberMarch/Meshes/%s.%s"),N,N);
        auto* M=LoadObject<UStaticMesh>(nullptr,*P);
        if(TestNotNull(*P,M))
        {
            TestTrue(TEXT("Real geometry and sane centimetre units"),M->GetBounds().BoxExtent.Size()>20&&M->GetBounds().BoxExtent.Size()<5000);
            TestTrue(TEXT("Palette material assigned"),M->GetStaticMaterials().Num()>0);
        }
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExplorerNodeTest,"AIHeroes.Visual.Explorer.NodeArtReadOnlyAndIdempotent",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FExplorerNodeTest::RunTest(const FString&)
{
    const UWorld::InitializationValues Values=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* W=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
    if(!TestNotNull(TEXT("World"),W))return false;
    auto* N=W->SpawnActor<AAIHeroesWorldNode>(FVector(100,200,110),FRotator::ZeroRotator);
    if(TestNotNull(TEXT("Site"),N))
    {
        N->Configure(EAIHeroesNodeType::ForestMine,TEXT("FOREST"),{FAIHeroesArmyStack(TEXT("Wolf"),4,9,3,1,8)});
        const auto Position=N->GetActorLocation();const int32 HP=N->Defenders[0].TotalHealth;
        for(int I=0;I<10;++I)TestTrue(TEXT("Visual refresh works"),FAIHeroesArtDirection::ApplyNode(*N));
        TArray<UStaticMeshComponent*> Cs;N->GetComponents(Cs);int Art=0;
        for(auto* C:Cs)if(C->GetFName()==FName(TEXT("AmberSite"))){++Art;TestEqual(TEXT("Decor does not intercept clicks"),C->GetCollisionEnabled(),ECollisionEnabled::NoCollision);}
        TestEqual(TEXT("No repeated component leak"),Art,1);TestEqual(TEXT("Position unchanged"),N->GetActorLocation(),Position);
        TestEqual(TEXT("Health unchanged"),N->Defenders[0].TotalHealth,HP);TestFalse(TEXT("No ownership granted"),N->bOwned);
        TestEqual(TEXT("Original pick collider preserved"),N->Visual->GetCollisionResponseToChannel(ECC_Visibility),ECR_Block);
        N->SetActorHiddenInGame(true);FAIHeroesArtDirection::ApplyNode(*N);TestTrue(TEXT("Art never uncovers hidden site"),N->IsHidden());
    }
    W->DestroyWorld(false);return true;
}
#endif
