#include "AIHeroesReferenceUI.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "AIHeroesExplorerLayout.h"
#include "AIHeroesMapLabelLayout.h"
#include "AIHeroesScenicCoast.h"
#include "Misc/AutomationTest.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include <limits>

namespace ReferenceUITest
{
FAIHeroesAdvisorSnapshot Fixture()
{
    FAIHeroesAdvisorSnapshot S;S.TravelRuleVersion=2;S.bFogEnabled=true;S.ExploredSitesMask=7;
    const TCHAR* Ids[]={TEXT("HQ"),TEXT("FOREST"),TEXT("ORE"),TEXT("OUTPOST"),TEXT("RUINS")};
    const EAIHeroesNodeType Types[]={EAIHeroesNodeType::Headquarters,EAIHeroesNodeType::ForestMine,
        EAIHeroesNodeType::OreMine,EAIHeroesNodeType::Outpost,EAIHeroesNodeType::Ruins};
    const FVector Positions[]={FVector(-650,0,155),FVector(-100,-650,110),FVector(150,600,120),FVector(700,-300,140),FVector(750,650,165)};
    S.HeroPosition=Positions[0];S.Gold=240;S.MovesLeft=4;
    for(int32 I=0;I<5;++I)
    {FAIHeroesAdvisorNode N;N.Id=Ids[I];N.Type=Types[I];N.Location=Positions[I];N.bOwned=I==0;
        if(I)N.Guards={FAIHeroesArmyStack(TEXT("Wolf"),4,9,3,1,8)};S.Nodes.Add(N);}
    return S;
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRefGlyphTest,"AIHeroes.Visual.Reference.GlyphGeometryBounds",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRefGlyphTest::RunTest(const FString&)
{
    for(uint8 K=0;K<=static_cast<uint8>(EAIHeroesGlyph::Hourglass);++K)
    {
        const auto Lines=FAIHeroesReferenceUI::Glyph(static_cast<EAIHeroesGlyph>(K));
        TestTrue(TEXT("Original icon has bounded strokes"),Lines.Num()>=4&&Lines.Num()<=64);
        for(const auto& L:Lines)for(const auto& P:{L.A,L.B})
            TestTrue(TEXT("Every icon stroke stays in its drawing box"),FMath::IsFinite(P.X)&&FMath::IsFinite(P.Y)&&P.X>=0&&P.Y>=0&&P.X<=1&&P.Y<=1);
    }
    TestTrue(TEXT("Unknown glyph yields no fabricated symbol"),FAIHeroesReferenceUI::Glyph(static_cast<EAIHeroesGlyph>(255)).IsEmpty());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRefOverviewTest,"AIHeroes.Visual.Reference.FogAndAuthoritativeRoads",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRefOverviewTest::RunTest(const FString&)
{
    auto S=ReferenceUITest::Fixture();auto O=FAIHeroesReferenceUI::Overview(S);
    TestTrue(TEXT("Known starting state valid"),O.bValid);TestEqual(TEXT("Only three explored sites"),O.Marks.Num(),3);
    TestEqual(TEXT("Two starting roads, no distance-generated shortcuts"),O.Links.Num(),2);
    for(const auto& M:O.Marks)TestTrue(TEXT("No secret identities"),M.Id==FName(TEXT("HQ"))||M.Id==FName(TEXT("FOREST"))||M.Id==FName(TEXT("ORE")));
    S.Nodes[3].Guards[0].TotalHealth=200;
    const auto Hidden=FAIHeroesReferenceUI::Overview(S);
    TestEqual(TEXT("Hidden strength cannot alter visible mark count"),Hidden.Marks.Num(),O.Marks.Num());
    TestEqual(TEXT("Hidden strength cannot expose roads"),Hidden.Links.Num(),O.Links.Num());
    S.Nodes[1].Guards.Empty();S.Nodes[1].bOwned=true;S.HeroPosition=S.Nodes[1].Location;S.ExploredSitesMask=15;
    O=FAIHeroesReferenceUI::Overview(S);TestTrue(TEXT("Actual discovery accepted"),O.bValid);
    TestEqual(TEXT("Four known sites after exploration"),O.Marks.Num(),4);TestEqual(TEXT("One road newly visible"),O.Links.Num(),3);
    for(const auto& L:O.Links)TestTrue(TEXT("Link endpoints are visible, distinct and bounded"),O.Marks.IsValidIndex(L.X)&&O.Marks.IsValidIndex(L.Y)&&L.X<L.Y);
    TestEqual(TEXT("Rendering never spends movement"),S.MovesLeft,4);TestEqual(TEXT("Rendering never spends gold"),S.Gold,240);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRefOverviewInvalidTest,"AIHeroes.Visual.Reference.LegacyAndInvalidSnapshots",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRefOverviewInvalidTest::RunTest(const FString&)
{
    auto S=ReferenceUITest::Fixture();S.TravelRuleVersion=1;S.bFogEnabled=false;S.ExploredSitesMask=0;
    const auto Old=FAIHeroesReferenceUI::Overview(S);TestTrue(TEXT("Legacy state can be shown"),Old.bValid);
    TestEqual(TEXT("Legacy discovers all five"),Old.Marks.Num(),5);TestTrue(TEXT("No invented legacy roads"),Old.Links.IsEmpty());
    for(int I=0;I<5;++I)
    {
        S=ReferenceUITest::Fixture();
        if(I==0)S.Nodes.RemoveAt(4);if(I==1)S.HeroPosition.X=std::numeric_limits<double>::quiet_NaN();
        if(I==2)S.ExploredSitesMask=64;if(I==3)S.TravelRuleVersion=99;
        if(I==4)S.SelectedNodeId=TEXT("RUINS");
        const auto Bad=FAIHeroesReferenceUI::Overview(S);
        TestTrue(TEXT("Malformed view rejected without partial data"),!Bad.bValid&&Bad.Marks.IsEmpty()&&Bad.Links.IsEmpty());
    }
    const auto P=FAIHeroesReferenceUI::Project(FVector(-1700,1700,0));
    TestTrue(TEXT("Fixed public projection, no hidden bounds inference"),P.Equals(FVector2D(1,1)));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRefPanelsTest,"AIHeroes.Visual.Reference.PanelsAndInputIsolation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRefPanelsTest::RunTest(const FString&)
{
    for(auto V:{FVector2D(320,640),FVector2D(640,720),FVector2D(960,720),FVector2D(1280,720),FVector2D(1920,1080),FVector2D(2560,1440)})
    {
        const auto F=FAIHeroesExplorerLayout::Frame(V);
        FAIHeroesHUDActionRect Screen{0,0,static_cast<float>(V.X),static_cast<float>(V.Y)};
        for(const auto& R:{F.Hero,F.Overview})if(R.W>0)
        {
            TestTrue(TEXT("Supplemental panel stays inside screen"),FAIHeroesMapLabelLayout::ContainsRect(Screen,R));
            TestFalse(TEXT("No dock overlap"),FAIHeroesMapLabelLayout::Overlaps(R,F.Dock,0));
            TestFalse(TEXT("No inspector overlap"),FAIHeroesMapLabelLayout::Overlaps(R,F.Inspector,0));
            TestFalse(TEXT("No objective overlap"),FAIHeroesMapLabelLayout::Overlaps(R,F.Mission,0));
            TestEqual(TEXT("Read-only panels never click through to map"),FAIHeroesExplorerLayout::Hit({R.X+R.W/2,R.Y+R.H/2},V,false),219);
        }
        if(!F.bWide)TestTrue(TEXT("Small views prioritize map over permanent extra panels"),F.Hero.W==0&&F.Overview.W==0);
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRefCoastTest,"AIHeroes.Visual.Reference.CoastDeterministicAndNonBlocking",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRefCoastTest::RunTest(const FString&)
{
    const auto A=FAIHeroesScenicCoast::Transforms(),B=FAIHeroesScenicCoast::Transforms();
    TestEqual(TEXT("Coast budget is fixed"),A.Num(),64);TestEqual(TEXT("Stable repeat count"),B.Num(),64);
    for(int I=0;I<A.Num();++I)
    {
        TestTrue(TEXT("Deterministic transforms"),A[I].Equals(B[I]));
        const auto P=A[I].GetLocation();TestTrue(TEXT("Rocks stay outside gameplay centre"),FMath::Max(FMath::Abs(P.X),FMath::Abs(P.Y))>=1570.f);
        TestTrue(TEXT("Finite rocks"),!A[I].ContainsNaN());
    }
    const UWorld::InitializationValues Values=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* W=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
    if(!TestNotNull(TEXT("Disposable world"),W))return false;
    AActor* E=W->SpawnActor<AActor>();
    if(TestNotNull(TEXT("Disposable environment"),E))
    {
        auto* Root=NewObject<USceneComponent>(E);E->AddInstanceComponent(Root);E->SetRootComponent(Root);Root->RegisterComponent();
        TestTrue(TEXT("Coast created"),FAIHeroesScenicCoast::Attach(*E));TestTrue(TEXT("Repeat is idempotent"),FAIHeroesScenicCoast::Attach(*E));
        TArray<UInstancedStaticMeshComponent*> Cs;E->GetComponents(Cs);TestEqual(TEXT("No duplicate scene components"),Cs.Num(),1);
        if(Cs.Num()==1){TestEqual(TEXT("64 grouped instances"),Cs[0]->GetInstanceCount(),64);TestEqual(TEXT("No collision changes routes or picks"),Cs[0]->GetCollisionEnabled(),ECollisionEnabled::NoCollision);}
    }
    W->DestroyWorld(false);return true;
}
#endif
