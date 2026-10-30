#include "AIHeroesBattleArt.h"
#include "AIHeroesGridBattle.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

FString FAIHeroesBattleArt::ActionCaption(const FAIHeroesGridAction& Action)
{
    if(Action.Round<=0)return FString();
    const FString Actor=GroupCode(Action.bPlayerAttacker,Action.AttackerStackIndex)
        +TEXT(" ")+UnitName(Action.AttackerId);
    switch(Action.Kind)
    {
        case EAIHeroesGridActionKind::Move:
            return FString::Printf(TEXT("%s: (%d,%d) → (%d,%d)."),*Actor,
                Action.FromCell.X,Action.FromCell.Y,Action.ToCell.X,Action.ToCell.Y);
        case EAIHeroesGridActionKind::Attack:
            return FString::Printf(TEXT("%s атакует %s: %d урона."),*Actor,
                *GroupCode(!Action.bPlayerAttacker,Action.TargetStackIndex),Action.Damage);
        case EAIHeroesGridActionKind::Wait:
            return Actor+TEXT(" ожидает: сейчас нет допустимой атаки.");
        default:return TEXT("Событие боя не распознано.");
    }
}

FString FAIHeroesBattleArt::ActionReason(const FAIHeroesGridAction& Action)
{
    if(Action.Round<=0)return FString();
    const FString& Reason=Action.Reason;
    if(Reason==TEXT("first legal target"))return TEXT("Причина: первая допустимая цель.");
    if(Reason==TEXT("highest threat"))return TEXT("Причина: цель с наибольшей угрозой.");
    if(Reason==TEXT("finish damaged stack"))return TEXT("Причина: добить повреждённый отряд.");
    if(Reason==TEXT("lethal priority"))return TEXT("Причина: приоритет уничтожения отряда.");
    if(Reason==TEXT("no reachable target"))return TEXT("Причина: нет достижимой цели.");
    if(Reason==TEXT("no target in range after movement"))
        return TEXT("Причина: после движения нет цели в радиусе атаки.");
    if(Reason.IsEmpty())return TEXT("Причина в событии не записана.");
    // Unknown future vocabulary is quoted as event data, never inferred policy.
    FString Safe=Reason.Left(160);
    Safe.ReplaceInline(TEXT("\r"),TEXT(" "));Safe.ReplaceInline(TEXT("\n"),TEXT(" "));
    return TEXT("Причина из события: ")+Safe;
}

namespace AIHeroesBattleArtDetail
{
bool Known(FName Unit)
{
    for (const TCHAR* Id : {TEXT("Militia"),TEXT("Scout"),TEXT("Wolf"),TEXT("Bandit"),TEXT("Raider"),TEXT("Guardian")})
        if (Unit == FName(Id)) return true;
    return false;
}
UStaticMeshComponent* Badge(AStaticMeshActor& Body)
{
    TArray<UStaticMeshComponent*> Components;
    Body.GetComponents(Components);
    for (auto* C : Components) if (C->GetFName() == FName(TEXT("AmberTeamBadge"))) return C;
    return nullptr;
}
}

FString FAIHeroesBattleArt::MeshPath(FName Unit)
{
    return AIHeroesBattleArtDetail::Known(Unit)
        ? TEXT("/Game/AmberMarch/Units/SM_") + Unit.ToString() + TEXT(".SM_") + Unit.ToString() : FString();
}
FString FAIHeroesBattleArt::PortraitPath(FName Unit)
{
    return AIHeroesBattleArtDetail::Known(Unit)
        ? TEXT("/Game/AmberMarch/Units/T_") + Unit.ToString() + TEXT(".T_") + Unit.ToString() : FString();
}
FString FAIHeroesBattleArt::UnitName(FName Unit)
{
    if (Unit == FName(TEXT("Militia"))) return TEXT("Ополчение");
    if (Unit == FName(TEXT("Scout"))) return TEXT("Разведчики");
    if (Unit == FName(TEXT("Wolf"))) return TEXT("Волки");
    if (Unit == FName(TEXT("Bandit"))) return TEXT("Разбойники");
    if (Unit == FName(TEXT("Raider"))) return TEXT("Налётчики");
    if (Unit == FName(TEXT("Guardian"))) return TEXT("Стражи архива");
    return TEXT("Неизвестный отряд");
}
FString FAIHeroesBattleArt::Role(FName Unit)
{
    if (Unit == FName(TEXT("Scout"))) return TEXT("Стрелки");
    if (Unit == FName(TEXT("Wolf"))) return TEXT("Быстрый ближний бой");
    if (Unit == FName(TEXT("Guardian"))) return TEXT("Тяжёлый ближний бой");
    return TEXT("Ближний бой");
}
FString FAIHeroesBattleArt::GroupCode(bool bPlayer, int32 Index)
{
    return FString::Printf(TEXT("%s%d"), bPlayer ? TEXT("A") : TEXT("B"), FMath::Max(0,Index)+1);
}
bool FAIHeroesBattleArt::HasUnitMesh(const AStaticMeshActor& Body)
{
    const UStaticMesh* Mesh=Body.GetStaticMeshComponent()->GetStaticMesh();
    return Mesh && Mesh->GetPathName().StartsWith(TEXT("/Game/AmberMarch/Units/SM_"));
}
bool FAIHeroesBattleArt::Apply(AStaticMeshActor& Body, FName Unit, bool bPlayer)
{
    const FString Path=MeshPath(Unit);
    if (Path.IsEmpty()) return false;
    UStaticMesh* Model=LoadObject<UStaticMesh>(nullptr,*Path);
    if (!Model) return false;
    auto* Mesh=Body.GetStaticMeshComponent();
    Mesh->EmptyOverrideMaterials();
    Mesh->SetStaticMesh(Model);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Mesh->SetCastShadow(true);
    auto* Mark=AIHeroesBattleArtDetail::Badge(Body);
    if (!Mark)
    {
        Mark=NewObject<UStaticMeshComponent>(&Body,TEXT("AmberTeamBadge"),RF_Transient);
        Body.AddInstanceComponent(Mark);
        Mark->SetupAttachment(Body.GetRootComponent());
        Mark->SetMobility(EComponentMobility::Movable);
        Mark->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mark->SetCastShadow(false);
        Mark->RegisterComponent();
    }
    // Shapes as well as letters distinguish sides: allied discs, hostile squares.
    Mark->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,bPlayer
        ? TEXT("/Engine/BasicShapes/Cylinder.Cylinder") : TEXT("/Engine/BasicShapes/Cube.Cube")));
    Mark->SetRelativeLocation(FVector(0,0,-1));
    Mark->SetRelativeScale3D(FVector(1.23f,1.23f,.035f));
    if (!Cast<UMaterialInstanceDynamic>(Mark->GetMaterial(0)))
    {
        if (auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
            Mark->SetMaterial(0,UMaterialInstanceDynamic::Create(Base,&Body));
    }
    TeamMark(Body,bPlayer);
    Body.SetActorRotation(FRotator(0,bPlayer?0.f:180.f,0));
    return true;
}
void FAIHeroesBattleArt::TeamMark(AStaticMeshActor& Body, bool bPlayer, bool bHit)
{
    if (auto* Mark=AIHeroesBattleArtDetail::Badge(Body))
        if (auto* Mat=Cast<UMaterialInstanceDynamic>(Mark->GetMaterial(0)))
            Mat->SetVectorParameterValue(TEXT("Color"),bHit ? FLinearColor(1.f,.68f,.15f)
                : bPlayer ? FLinearColor(.06f,.42f,.42f) : FLinearColor(.62f,.19f,.09f));
}
void FAIHeroesBattleArt::DecorateArena(AActor& Stage)
{
    TArray<UInstancedStaticMeshComponent*> Existing;
    Stage.GetComponents(Existing);
    for (const auto* C:Existing) if(C->GetFName()==FName(TEXT("AmberArenaBorder"))) return;
    auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!Cube || !Stage.GetRootComponent()) return;
    auto* Border=NewObject<UInstancedStaticMeshComponent>(&Stage,TEXT("AmberArenaBorder"),RF_Transient);
    Stage.AddInstanceComponent(Border);Border->SetupAttachment(Stage.GetRootComponent());
    Border->SetStaticMesh(Cube);Border->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Border->SetCastShadow(false);Border->RegisterComponent();
    if (auto* Base=LoadObject<UMaterialInterface>(nullptr,TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")))
    {
        auto* Mat=UMaterialInstanceDynamic::Create(Base,&Stage);
        Mat->SetVectorParameterValue(TEXT("Color"),FLinearColor(.28f,.32f,.27f));
        Border->SetMaterial(0,Mat);
    }
    for (float Y:{-574.f,574.f})
        Border->AddInstance(FTransform(FQuat::Identity,FVector(0,Y,-4),FVector(15.3f,.27f,.28f)));
    for (float X:{-754.f,754.f})
        Border->AddInstance(FTransform(FQuat::Identity,FVector(X,0,-4),FVector(.27f,11.7f,.28f)));
    // Decorative boundary only. It never occupies or changes a simulation cell.
}
FAIHeroesHUDActionRect FAIHeroesBattleArt::RosterRect(const FVector2D& V,bool bPlayer,int32 Index)
{
    FAIHeroesHUDActionRect R;
    if(!FMath::IsFinite(V.X)||!FMath::IsFinite(V.Y)||V.X<320||V.Y<400||V.X>16384||V.Y>16384||Index<0||Index>1) return R;
    const float Half=(V.X-36.f)*.5f;
    const bool Wide=V.X>=1080;
    R.X=12.f+(bPlayer?0:Half+12.f);
    R.Y=92.f;
    R.W=Wide?(Half-8.f)*.5f:Half;
    R.H=66.f;
    if(Wide) R.X+=Index*(R.W+8.f);else R.Y+=Index*72.f;
    return R;
}
