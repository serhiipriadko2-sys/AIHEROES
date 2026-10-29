#include "AIHeroesGameMode.h"

#include "AIHeroesRecruitmentAdvisor.h"
#include "AIHeroesWorldNode.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
bool SiteForType(EAIHeroesNodeType Type, EAIHeroesFieldSite& Out)
{
    if (Type == EAIHeroesNodeType::ForestMine)
    {
        Out = EAIHeroesFieldSite::ForestCamp;
        return true;
    }
    if (Type == EAIHeroesNodeType::OreMine)
    {
        Out = EAIHeroesFieldSite::OreAerie;
        return true;
    }
    return false;
}

const TCHAR* FieldName(EAIHeroesFieldSite Site)
{
    return Site == EAIHeroesFieldSite::ForestCamp
        ? TEXT("FOREST CAMP") : TEXT("ORE AERIE");
}

const TCHAR* FieldUnit(EAIHeroesFieldSite Site)
{
    return Site == EAIHeroesFieldSite::ForestCamp
        ? TEXT("Militia") : TEXT("Scouts");
}
}

FString AAIHeroesGameMode::GetFieldDwellingLabel(EAIHeroesNodeType Type) const
{
    EAIHeroesFieldSite Site = EAIHeroesFieldSite::ForestCamp;
    if (!SiteForType(Type, Site)) return FString();
    const auto& Stock = FAIHeroesFieldDwellingRules::At(FieldDwellings, Site);
    return FString::Printf(
        TEXT("%s %d %s%s"),
        Site == EAIHeroesFieldSite::ForestCamp ? TEXT("CAMP") : TEXT("AERIE"),
        Stock.Available, *FString(FieldUnit(Site)),
        Stock.bClaimed ? TEXT(" [OURS]") : TEXT(" [FREE]"));
}

FString AAIHeroesGameMode::GetFieldDwellingSummary() const
{
    if (!IsValid(SelectedNode.Get()) || !Nodes.Contains(SelectedNode))
        return TEXT("FIELD: select FOREST camp or ORE aerie. E claims/recruits.");
    EAIHeroesFieldSite Site = EAIHeroesFieldSite::ForestCamp;
    if (!SiteForType(SelectedNode->NodeType, Site))
        return TEXT("FIELD: select FOREST or ORE; headquarters stocks are independent.");
    const auto& Stock = FAIHeroesFieldDwellingRules::At(FieldDwellings, Site);
    const bool bHere =
        FVector::DistSquared(
            HeroStrategicLocation, SelectedNode->GetActorLocation()) < 1.0;
    return FString::Printf(
        TEXT("%s: %d %s | %s | %s. E to %s."),
        FieldName(Site), Stock.Available, FieldUnit(Site),
        Stock.bClaimed ? TEXT("CLAIMED") : TEXT("UNCLAIMED"),
        !SelectedNode->bOwned ? TEXT("CLEAR FIRST")
            : bHere ? TEXT("HERO HERE") : TEXT("TRAVEL HERE"),
        Stock.bClaimed ? TEXT("RECRUIT") : TEXT("CLAIM"));
}

void AAIHeroesGameMode::InteractFieldDwelling()
{
    if (BlockStrategicAction()) return;
    if (!IsValid(SelectedNode.Get()) || !Nodes.Contains(SelectedNode.Get()))
    {
        SetEvent(TEXT("FIELD: select an existing FOREST or ORE site first. Nothing spent."));
        return;
    }
    EAIHeroesFieldSite Site = EAIHeroesFieldSite::ForestCamp;
    if (!SiteForType(SelectedNode->NodeType, Site))
    {
        SetEvent(TEXT("FIELD: HQ, OUTPOST and RUINS have no independent dwelling."));
        return;
    }
    const bool bHere =
        FVector::DistSquared(
            HeroStrategicLocation, SelectedNode->GetActorLocation()) < 1.0;
    if (!FAIHeroesFieldDwellingRules::At(FieldDwellings, Site).bClaimed)
    {
        FString Error;
        if (!FAIHeroesFieldDwellingRules::TryClaim(
                Site, SelectedNode->bOwned, bHere, FieldDwellings, Error))
        {
            SetEvent(Error);
            return;
        }
        InvalidateRoutePreview();
        RefreshFieldDwellingVisuals();
        UE_LOG(LogTemp, Display,
            TEXT("[AI HEROES FIELD] CLAIM site=%s stock=%d player_order=1"),
            FieldName(Site),
            FAIHeroesFieldDwellingRules::At(FieldDwellings, Site).Available);
        SetEvent(FString::Printf(
            TEXT("FIELD CLAIMED: %s. Available %d %s. Press E again to buy a pack."),
            FieldName(Site),
            FAIHeroesFieldDwellingRules::At(FieldDwellings, Site).Available,
            FieldUnit(Site)));
        return;
    }

    FString Error;
    if (!FAIHeroesFieldDwellingRules::TryRecruit(
            Site, SelectedNode->bOwned, bHere,
            FieldDwellings, Gold, Wood, Ore, PlayerArmy, Error))
    {
        SetEvent(Error);
        return;
    }
    InvalidateRoutePreview();
    RefreshFieldDwellingVisuals();
    const auto& Remaining = FAIHeroesFieldDwellingRules::At(FieldDwellings, Site);
    UE_LOG(LogTemp, Display,
        TEXT("[AI HEROES FIELD] RECRUIT site=%s remaining=%d gold=%d wood=%d ore=%d player_order=1"),
        FieldName(Site), Remaining.Available, Gold, Wood, Ore);
    SetEvent(FString::Printf(
        TEXT("FIELD RECRUITED: %s pack bought at %s. Remain %d. Player order applied once."),
        FieldUnit(Site), FieldName(Site), Remaining.Available));
}

void AAIHeroesGameMode::SpawnFieldDwellingVisuals()
{
    if (!GetWorld() || FieldDwellingVisuals.Num() > 0) return;
    UStaticMesh* Cube = LoadObject<UStaticMesh>(
        nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    UStaticMesh* Sphere = LoadObject<UStaticMesh>(
        nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    UMaterialInterface* Basic = LoadObject<UMaterialInterface>(
        nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (!Cube || !Sphere) return;

    for (const EAIHeroesFieldSite Site :
        {EAIHeroesFieldSite::ForestCamp, EAIHeroesFieldSite::OreAerie})
    {
        const AAIHeroesWorldNode* Node = nullptr;
        for (const AAIHeroesWorldNode* N : Nodes)
            if (IsValid(N) && FName(*N->GetDisplayName()) ==
                FAIHeroesFieldDwellingRules::ParentNodeId(Site))
                Node = N;
        if (!Node) return;

        FActorSpawnParameters Params;
        Params.Owner = this;
        const FVector Offset = Site == EAIHeroesFieldSite::ForestCamp
            ? FVector(-230.f, -105.f, 90.f)
            : FVector(210.f, 100.f, 105.f);
        AStaticMeshActor* Marker = GetWorld()->SpawnActor<AStaticMeshActor>(
            Node->GetActorLocation() + Offset, FRotator::ZeroRotator, Params);
        if (!Marker) return;
        auto* Mesh = Marker->GetStaticMeshComponent();
        Mesh->SetMobility(EComponentMobility::Movable);
        Mesh->SetStaticMesh(
            Site == EAIHeroesFieldSite::ForestCamp ? Cube : Sphere);
        Mesh->SetWorldScale3D(
            Site == EAIHeroesFieldSite::ForestCamp
            ? FVector(1.55f, 0.8f, 1.4f)
            : FVector(1.15f, 1.15f, 2.1f));
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetCastShadow(true);
        Marker->Tags.Add(Site == EAIHeroesFieldSite::ForestCamp
            ? FName(TEXT("AIH_Field_ForestCamp"))
            : FName(TEXT("AIH_Field_OreAerie")));
        if (Basic)
        {
            auto* Material = UMaterialInstanceDynamic::Create(Basic, Marker);
            Mesh->SetMaterial(0, Material);
        }
        FieldDwellingVisuals.Add(Marker);
    }
    RefreshFieldDwellingVisuals();
}

void AAIHeroesGameMode::RefreshFieldDwellingVisuals()
{
    for (int32 Index = 0; Index < FieldDwellingVisuals.Num() && Index < 2; ++Index)
    {
        AStaticMeshActor* Marker = FieldDwellingVisuals[Index];
        if (!IsValid(Marker)) continue;
        const EAIHeroesFieldSite Site = Index == 0
            ? EAIHeroesFieldSite::ForestCamp : EAIHeroesFieldSite::OreAerie;
        Marker->SetActorHiddenInGame(!IsNodeDiscovered(
            FAIHeroesFieldDwellingRules::ParentNodeId(Site)));
        auto* Mesh = Marker->GetStaticMeshComponent();
        if (!Mesh) continue;
        const bool bClaimed = Index == 0
            ? FieldDwellings.Forest.bClaimed
            : FieldDwellings.Ore.bClaimed;
        if (auto* Material = Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(0)))
        {
            Material->SetVectorParameterValue(TEXT("Color"),
                bClaimed ? FLinearColor(0.15f, 0.83f, 0.45f)
                : Index == 0 ? FLinearColor(0.9f, 0.52f, 0.19f)
                : FLinearColor(0.45f, 0.65f, 0.94f));
        }
    }
}
