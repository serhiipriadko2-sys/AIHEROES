#include "AIHeroesMapLabelLayout.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AIHeroesTravelPresentation.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include <limits>

namespace MapReadabilityTest
{
FAIHeroesMapLabelRequest Label(const TCHAR* Id, FVector2D At, bool Selected = false, bool Detail = true)
{
    FAIHeroesMapLabelRequest R;
    R.Id = FName(Id); R.Anchor = At; R.bKnown = true;
    R.bSelected = Selected; R.bHasDetail = Detail;
    return R;
}
bool Clean(const FVector2D& V, const TArray<FAIHeroesMapLabelPlacement>& P)
{
    const auto Safe = FAIHeroesMapLabelLayout::MapBounds(V);
    for (int32 I = 0; I < P.Num(); ++I)
    {
        if (!FAIHeroesMapLabelLayout::ContainsRect(Safe, P[I].Rect)) return false;
        for (int32 J = I + 1; J < P.Num(); ++J)
            if (FAIHeroesMapLabelLayout::Overlaps(P[I].Rect, P[J].Rect)) return false;
        for (int32 Action : {0,1,2,3,4,5,6,7,8,9,10,11,12,200,201,202,203,204})
            if (FAIHeroesMapLabelLayout::Overlaps(P[I].Rect, FAIHeroesHUDLayout::Rect(Action,V),0.f)) return false;
    }
    return true;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMapSafeBoundsTest,"AIHeroes.Readability.SafeMapBounds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMapSafeBoundsTest::RunTest(const FString& Parameters)
{
    for (int32 Width : {640,960,1280,1920,2560})
    {
        const FVector2D V(Width,720);
        const auto Safe = FAIHeroesMapLabelLayout::MapBounds(V);
        TestTrue(TEXT("Threat strip is outside label region"),Safe.Y >= 212.f && Safe.H > 0.f);
        const auto Goals = FAIHeroesHUDLayout::Rect(202,V);
        TestTrue(TEXT("Full label region ends above mission controls"),Safe.Y+Safe.H <= Goals.Y-8.f);
        TestTrue(TEXT("Full label region within viewport"),Safe.X >= 0 && Safe.X+Safe.W <= Width);
    }
    TestEqual(TEXT("No drawable map in too-short viewport"),
        FAIHeroesMapLabelLayout::MapBounds(FVector2D(640,360)).W,0.f);
    TestEqual(TEXT("NaN viewport rejected"),
        FAIHeroesMapLabelLayout::MapBounds(FVector2D(std::numeric_limits<double>::quiet_NaN(),720)).W,0.f);
    TestEqual(TEXT("Huge viewport bounded"),
        FAIHeroesMapLabelLayout::MapBounds(FVector2D(1.e12,720)).W,0.f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMapLabelPackingTest,"AIHeroes.Readability.LabelsDoNotOverlapPanelsOrEachOther",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMapLabelPackingTest::RunTest(const FString& Parameters)
{
    using namespace MapReadabilityTest;
    for (int32 Width : {640,960,1280,1920})
    {
        const FVector2D V(Width,720), Center(Width/2,195);
        const TArray<FAIHeroesMapLabelRequest> R = {
            Label(TEXT("FOREST"),Center), Label(TEXT("ORE"),Center),
            Label(TEXT("OUTPOST"),Center,false,false), Label(TEXT("RUINS"),Center,false,false),
            Label(TEXT("HQ"),FVector2D(Width/2,650),true,false)};
        const auto P = FAIHeroesMapLabelLayout::Place(V,R);
        TestEqual(TEXT("All five current sites fit"),P.Num(),5);
        TestTrue(TEXT("No control or full two-line label overlap"),Clean(V,P));
        if (!P.IsEmpty()) TestEqual(TEXT("Selected target takes first place"),P[0].Id,FName(TEXT("HQ")));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMapLabelVisibilityTest,"AIHeroes.Readability.HiddenAndInvalidInputs",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMapLabelVisibilityTest::RunTest(const FString& Parameters)
{
    using namespace MapReadabilityTest;
    const FVector2D V(1280,720);
    TArray<FAIHeroesMapLabelRequest> R = {Label(TEXT("HQ"),FVector2D(640,300))};
    auto Hidden = Label(TEXT("SECRET"),FVector2D(640,300),true);
    Hidden.bKnown = false; R.Add(Hidden);
    R.Add(Label(TEXT("OFFSCREEN"),FVector2D(-1,300)));
    R.Add(Label(TEXT("NAN"),FVector2D(std::numeric_limits<double>::quiet_NaN(),300)));
    R.Add(Label(TEXT("INFINITY"),FVector2D(100,std::numeric_limits<double>::infinity())));
    auto P = FAIHeroesMapLabelLayout::Place(V,R);
    TestEqual(TEXT("Unknown or invalid projection never leaks/reserves a label"),P.Num(),1);
    if (!P.IsEmpty()) TestEqual(TEXT("Only visible site rendered"),P[0].Id,FName(TEXT("HQ")));
    R.Add(Label(TEXT("hq"),FVector2D(800,300)));
    TestTrue(TEXT("Ambiguous visible IDs fail closed"),FAIHeroesMapLabelLayout::Place(V,R).IsEmpty());
    R.Empty(); R.Init(Label(TEXT("HQ"),FVector2D(640,300)),65);
    TestTrue(TEXT("Oversized input rejected before packing"),FAIHeroesMapLabelLayout::Place(V,R).IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMapLabelStableTest,"AIHeroes.Readability.StableSelectedPriority",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMapLabelStableTest::RunTest(const FString& Parameters)
{
    using namespace MapReadabilityTest;
    const FVector2D V(640,720), Center(320,300);
    const TArray<FAIHeroesMapLabelRequest> A = {Label(TEXT("C"),Center),Label(TEXT("A"),Center),Label(TEXT("B"),Center,true)};
    const TArray<FAIHeroesMapLabelRequest> B = {A[2],A[1],A[0]};
    const auto P = FAIHeroesMapLabelLayout::Place(V,A), Q = FAIHeroesMapLabelLayout::Place(V,B);
    TestEqual(TEXT("Reorder retains count"),P.Num(),Q.Num());
    TestEqual(TEXT("Input order not modified"),A[0].Id,FName(TEXT("C")));
    for (int32 I=0; I<P.Num() && I<Q.Num(); ++I)
    {
        TestEqual(TEXT("Stable placement IDs"),P[I].Id,Q[I].Id);
        TestEqual(TEXT("Stable placement X"),P[I].Rect.X,Q[I].Rect.X);
        TestEqual(TEXT("Stable placement Y"),P[I].Rect.Y,Q[I].Rect.Y);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMapLabelSweepTest,"AIHeroes.Readability.ResolutionAndCrowdingSweep",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMapLabelSweepTest::RunTest(const FString& Parameters)
{
    using namespace MapReadabilityTest;
    int32 Cases=0;
    for (int32 Width : {320,360,640,799,800,960,1049,1050,1149,1150,1280,1920})
    for (int32 Height : {240,360,540,720,1080})
    {
        TArray<FAIHeroesMapLabelRequest> R;
        for (int32 I=0; I<20; ++I)
        {
            const FString Id=FString::Printf(TEXT("SITE_%02d"),I);
            R.Add(Label(*Id,FVector2D(Width*0.5,Height*0.5),I==19,I%2==0));
        }
        const FVector2D V(Width,Height);
        const auto P=FAIHeroesMapLabelLayout::Place(V,R);
        TestTrue(TEXT("Every emitted rectangle is bounded and separated"),Clean(V,P));
        TestTrue(TEXT("Packing cannot invent additional sites"),P.Num() <= R.Num());
        if (!P.IsEmpty()) TestEqual(TEXT("Crowding retains selected site first"),P[0].Id,FName(TEXT("SITE_19")));
        ++Cases;
    }
    TestEqual(TEXT("Exactly sixty viewport cases executed"),Cases,60);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMapRepeatedChevronTest,"AIHeroes.Readability.RepeatedChevronGeometry",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMapRepeatedChevronTest::RunTest(const FString& Parameters)
{
    const auto Values=UWorld::InitializationValues().AllowAudioPlayback(false)
        .CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
    if (!TestNotNull(TEXT("Disposable world exists"),World)) return false;
    auto* View=World->SpawnActor<AAIHeroesTravelPresentation>();
    if (!TestNotNull(TEXT("Presenter exists"),View)) { World->DestroyWorld(false); return false; }
    const TArray<FVector> Path={FVector(0,0,0),FVector(500,0,0),FVector(500,500,0)};
    View->ShowPath(Path,EAIHeroesAdventurePathStatus::Ready);
    TestEqual(TEXT("Two chevrons plus one line per edge"),View->GetPathInstanceCount(),10);
    FTransform First,Second;
    TestTrue(TEXT("First cue has readable transform"),View->GetPathMesh()->GetInstanceTransform(1,First,true));
    TestTrue(TEXT("Second cue has readable transform"),View->GetPathMesh()->GetInstanceTransform(3,Second,true));
    TestTrue(TEXT("Repeated cues occupy separate positions"),Second.GetLocation().X-First.GetLocation().X>100.0);
    TestTrue(TEXT("Cue widened from previous 15 cm"),First.GetScale3D().Y>=0.25);
    const int32 Revision=View->GetGeometryRevision();
    View->ShowPath(Path,EAIHeroesAdventurePathStatus::Ready);
    TestEqual(TEXT("Unchanged route still cached"),View->GetGeometryRevision(),Revision);
    View->ShowPath(Path,EAIHeroesAdventurePathStatus::NeedsMoreMoves);
    TestEqual(TEXT("Over-budget has chevrons but no connecting line"),View->GetPathInstanceCount(),8);
    TestTrue(TEXT("Route visual still cannot catch map clicks"),View->GetPathMesh()->GetCollisionEnabled()==ECollisionEnabled::NoCollision);
    View->ShowPath(Path,EAIHeroesAdventurePathStatus::Unreachable);
    TestEqual(TEXT("Blocked route has zero geometry"),View->GetPathInstanceCount(),0);
    World->DestroyWorld(false);
    return true;
}
#endif
