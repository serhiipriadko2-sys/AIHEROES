#include "AIHeroesBattleArt.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AIHeroesLiveBattleStage.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/AutomationTest.h"
#include "StaticMeshResources.h"
#include <limits>

namespace AmberBattleTest
{
struct FWorld
{
    UWorld* World=nullptr;
    FWorld()
    {
        const auto Values=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false)
            .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
        World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
        if(World&&GEngine)GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    }
    ~FWorld(){if(World){World->DestroyWorld(false);if(GEngine)GEngine->DestroyWorldContext(World);}}
};
AStaticMeshActor* FindBody(UWorld* W,FName Unit)
{
    for(TActorIterator<AStaticMeshActor> It(W);It;++It)
        if(It->GetStaticMeshComponent()->GetStaticMesh() &&
            It->GetStaticMeshComponent()->GetStaticMesh()->GetPathName()==FAIHeroesBattleArt::MeshPath(Unit))return *It;
    return nullptr;
}
FAIHeroesGridAction Attack(FName Attacker=TEXT("Militia"),FIntPoint Target=FIntPoint(1,2))
{
    FAIHeroesGridAction A;A.Round=1;A.Kind=EAIHeroesGridActionKind::Attack;A.bPlayerAttacker=true;
    A.AttackerStackIndex=0;A.TargetStackIndex=0;A.AttackerId=Attacker;A.TargetId=TEXT("Wolf");
    A.FromCell=FIntPoint(0,2);A.ToCell=Target;A.Damage=5;A.TargetHealthBefore=36;A.TargetHealthAfter=31;
    return A;
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAmberAssetsTest,"AIHeroes.Visual.Battle.SixModelsAndPortraits",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAmberAssetsTest::RunTest(const FString&)
{
    TSet<UStaticMesh*> Unique;
    for(const TCHAR* Id:{TEXT("Militia"),TEXT("Scout"),TEXT("Wolf"),TEXT("Bandit"),TEXT("Raider"),TEXT("Guardian")})
    {
        auto* Mesh=LoadObject<UStaticMesh>(nullptr,*FAIHeroesBattleArt::MeshPath(FName(Id)));
        auto* Portrait=LoadObject<UTexture2D>(nullptr,*FAIHeroesBattleArt::PortraitPath(FName(Id)));
        if(TestNotNull(TEXT("Original model loads"),Mesh))
        {
            Unique.Add(Mesh);TestTrue(TEXT("Centimetre-scale model"),Mesh->GetBounds().BoxExtent.Z>40&&Mesh->GetBounds().BoxExtent.Z<120);
            const auto* Render=Mesh->GetRenderData();
            if(TestNotNull(TEXT("Real mesh render data"),Render))
            {
                TestTrue(TEXT("LOD0 exists"),Render->LODResources.Num()>0);
                if(Render->LODResources.Num()>0)TestTrue(TEXT("Bounded miniature topology"),Render->LODResources[0].GetNumTriangles()>50&&Render->LODResources[0].GetNumTriangles()<=1800);
            }
            for(const auto& Slot:Mesh->GetStaticMaterials())TestNotNull(TEXT("Palette bound"),Slot.MaterialInterface.Get());
        }
        if(TestNotNull(TEXT("Portrait loads"),Portrait))
        {TestEqual(TEXT("Portrait width"),Portrait->GetSizeX(),256);TestEqual(TEXT("Portrait height"),Portrait->GetSizeY(),256);}
        TestFalse(TEXT("Russian title not raw identifier"),FAIHeroesBattleArt::UnitName(FName(Id))==FString(Id));
    }
    TestEqual(TEXT("Six distinct meshes, not six aliases"),Unique.Num(),6);
    TestTrue(TEXT("Unknown identifier cannot select an arbitrary asset"),FAIHeroesBattleArt::MeshPath(TEXT("../private")).IsEmpty());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAmberRolesTest,"AIHeroes.Visual.Battle.RosterLayoutAndGroupIdentity",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAmberRolesTest::RunTest(const FString&)
{
    int Cases=0;
    for(float W:{320.f,360.f,640.f,960.f,1080.f,1280.f,1920.f})
    for(float H:{720.f,900.f})
    {
        const FVector2D V(W,H);TArray<FAIHeroesHUDActionRect> Cards;
        for(bool Side:{true,false})for(int I=0;I<2;++I)
        {
            const auto R=FAIHeroesBattleArt::RosterRect(V,Side,I);
            TestTrue(TEXT("Card in viewport, away from bottom action bar"),R.W>0&&R.X>=0&&R.X+R.W<=W&&R.Y>=62&&R.Y+R.H<H-126);
            for(const auto& Other:Cards)TestFalse(TEXT("Cards do not overlap"),
                R.X<Other.X+Other.W&&R.X+R.W>Other.X&&R.Y<Other.Y+Other.H&&R.Y+R.H>Other.Y);
            Cards.Add(R);++Cases;
        }
    }
    TestEqual(TEXT("56 layout cells checked"),Cases,56);
    TestTrue(TEXT("Invalid size has no rect"),FAIHeroesBattleArt::RosterRect({0,720},true,0).W==0);
    TestTrue(TEXT("Invalid index has no rect"),FAIHeroesBattleArt::RosterRect({1280,720},true,2).W==0);
    TestTrue(TEXT("Nonfinite size rejected"),FAIHeroesBattleArt::RosterRect({std::numeric_limits<double>::quiet_NaN(),720},true,0).W==0);
    TestNotEqual(TEXT("Same-name opposing stacks retain separate IDs"),FAIHeroesBattleArt::GroupCode(true,0),FAIHeroesBattleArt::GroupCode(false,0));
    TestNotEqual(TEXT("Same-name same-side groups retain separate IDs"),FAIHeroesBattleArt::GroupCode(false,0),FAIHeroesBattleArt::GroupCode(false,1));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAmberImpactTest,"AIHeroes.Visual.Battle.HealthAtImpactAndPalettePersistence",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAmberImpactTest::RunTest(const FString&)
{
    using namespace AmberBattleTest;FWorld F;auto* Stage=F.World->SpawnActor<AAIHeroesLiveBattleStage>();
    const TArray<FAIHeroesArmyStack> P={FAIHeroesArmyStack(TEXT("Militia"),8,12,4,2,3)};
    const TArray<FAIHeroesArmyStack> E={FAIHeroesArmyStack(TEXT("Wolf"),4,9,3,1,8)};
    if(!TestTrue(TEXT("Disposable presentation initializes"),Stage->Initialize(P,E,{FIntPoint(0,2)},{FIntPoint(1,2)})))return false;
    auto* Body=FindBody(F.World,TEXT("Wolf"));if(!TestNotNull(TEXT("Wolf is model, not sphere"),Body))return false;
    const auto Scale=Body->GetActorScale3D();auto* OriginalMaterial=Body->GetStaticMeshComponent()->GetMaterial(0);
    if(!TestTrue(TEXT("Actual receipt starts animation"),Stage->PresentAction(Attack())))return false;
    Stage->Tick(.2f);
    TestEqual(TEXT("HUD health not applied before the impact"),Stage->GetPresentedArmy(false)[0].TotalHealth,36);
    Stage->Tick(.2f);
    TestEqual(TEXT("HUD health changes at impact"),Stage->GetPresentedArmy(false)[0].TotalHealth,31);
    TestEqual(TEXT("Bar baseline remains initial stack health"),Stage->GetInitialHealth(false,0),36);
    TestEqual(TEXT("Visual casualty does not shrink model"),Body->GetActorScale3D(),Scale);
    TestTrue(TEXT("Damage flash never replaces body material"),Body->GetStaticMeshComponent()->GetMaterial(0)==OriginalMaterial);
    Stage->SetPresentationPaused(true);Stage->Tick(2.f);
    TestEqual(TEXT("Paused health remains stable"),Stage->GetPresentedArmy(false)[0].TotalHealth,31);
    Stage->FinishPresentationImmediately();
    TestTrue(TEXT("Material survives finish"),Body->GetStaticMeshComponent()->GetMaterial(0)==OriginalMaterial);
    TestEqual(TEXT("Caller army untouched"),E[0].TotalHealth,36);
    TestEqual(TEXT("Invalid baseline index returns zero"),Stage->GetInitialHealth(false,5),0);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAmberPresentationTest,"AIHeroes.Visual.Battle.DecorationIdempotenceAndCleanup",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAmberPresentationTest::RunTest(const FString&)
{
    using namespace AmberBattleTest;FWorld F;auto* Stage=F.World->SpawnActor<AAIHeroesLiveBattleStage>();
    if(!TestTrue(TEXT("Stage starts"),Stage->Initialize({FAIHeroesArmyStack(TEXT("Militia"),8,12,4,2,3)},
        {FAIHeroesArmyStack(TEXT("Wolf"),4,9,3,1,8)})))return false;
    auto* Body=FindBody(F.World,TEXT("Militia"));if(!TestNotNull(TEXT("Original militia body"),Body))return false;
    for(int I=0;I<10;++I){TestTrue(TEXT("Repeated visual application"),FAIHeroesBattleArt::Apply(*Body,TEXT("Militia"),true));FAIHeroesBattleArt::DecorateArena(*Stage);}
    TArray<UStaticMeshComponent*> Cs;Body->GetComponents(Cs);int Badges=0;
    for(auto* C:Cs){if(C->GetFName()==FName(TEXT("AmberTeamBadge")))++Badges;TestEqual(TEXT("Every unit component is non-colliding"),C->GetCollisionEnabled(),ECollisionEnabled::NoCollision);}
    TestEqual(TEXT("One stable badge"),Badges,1);
    TestEqual(TEXT("No health change"),Stage->GetPresentedArmy(true)[0].TotalHealth,96);
    TArray<TWeakObjectPtr<AActor>> Owned;
    for(TActorIterator<AActor> It(F.World);It;++It)if(It->GetOwner()==Stage)Owned.Add(*It);
    TestTrue(TEXT("Stage can be destroyed"),Stage->Destroy());
    for(const auto& A:Owned)TestFalse(TEXT("No leaked miniature, label, or camera"),A.IsValid());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAmberShotTest,"AIHeroes.Visual.Battle.RangedProjectileIsPresentationOnly",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAmberShotTest::RunTest(const FString&)
{
    using namespace AmberBattleTest;FWorld F;auto* Stage=F.World->SpawnActor<AAIHeroesLiveBattleStage>();
    if(!TestTrue(TEXT("Ranged stage starts"),Stage->Initialize({FAIHeroesArmyStack(TEXT("Scout"),3,8,6,1,7)},
        {FAIHeroesArmyStack(TEXT("Wolf"),4,9,3,1,8)},{FIntPoint(0,2)},{FIntPoint(3,2)})))return false;
    TArray<UStaticMeshComponent*> Cs;Stage->GetComponents(Cs);UStaticMeshComponent* Shot=nullptr;
    for(auto* C:Cs)if(C->GetFName()==FName(TEXT("AmberShotProjectile")))Shot=C;
    if(!TestNotNull(TEXT("Owned projectile component exists"),Shot))return false;
    TestTrue(TEXT("Hidden before a supplied attack"),Shot->bHiddenInGame);
    if(!TestTrue(TEXT("Ranged receipt accepted"),Stage->PresentAction(Attack(TEXT("Scout"),FIntPoint(3,2)))))return false;
    Stage->Tick(.2f);TestFalse(TEXT("Flight visible before impact"),Shot->bHiddenInGame);
    TestEqual(TEXT("Flight cannot apply damage itself"),Stage->GetPresentedArmy(false)[0].TotalHealth,36);
    const FVector Position=Shot->GetComponentLocation();Stage->SetPresentationPaused(true);Stage->Tick(5.f);
    TestTrue(TEXT("Pause freezes flight"),Shot->GetComponentLocation().Equals(Position));
    Stage->FinishPresentationImmediately();TestTrue(TEXT("Finish hides flight"),Shot->bHiddenInGame);
    TestEqual(TEXT("Exactly the receipt damage is applied"),Stage->GetPresentedArmy(false)[0].TotalHealth,31);
    return true;
}
#endif
