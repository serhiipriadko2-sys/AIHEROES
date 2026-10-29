#include "AIHeroesGameMode.h"

#include "AIHeroesTravelRules.h"
#include "AIHeroesScenicCrossing.h"
#include "EngineUtils.h"
#include "AIHeroesWorldNode.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

FAIHeroesAdventurePathResult AAIHeroesGameMode::GetSelectedTravelPath() const
{
    if (!IsValid(SelectedNode.Get()) || !Nodes.Contains(SelectedNode) ||
        !IsNodeDiscovered(FName(*SelectedNode->GetDisplayName())))
    {
        FAIHeroesAdventurePathResult Invalid;
        Invalid.Reason = TEXT("Select a discovered current map site first.");
        return Invalid;
    }
    return GetTravelPathTo(FName(*SelectedNode->GetDisplayName()));
}

FAIHeroesAdventurePathResult AAIHeroesGameMode::GetTravelPathTo(FName Destination) const
{
    bool bCurrentSite = false;
    for (const AAIHeroesWorldNode* Node : Nodes)
        if (IsValid(Node) && FName(*Node->GetDisplayName()) == Destination)
            bCurrentSite = true;
    if (!bCurrentSite || Destination.IsNone() || !IsNodeDiscovered(Destination))
    {
        FAIHeroesAdventurePathResult Invalid;
        Invalid.Reason = TEXT("Choose a discovered current map site first.");
        return Invalid;
    }
    return FAIHeroesTravelRules::Find(BuildAdvisorSnapshot(), HeroStrategicLocation,
        Destination, MovesLeft);
}

void AAIHeroesGameMode::RefreshAdventureRoads()
{
    // A legacy V1 load has no authoritative Road V2 link: hide the bridge.
    // Exploration/fog changes also hide the bridge when FOREST is undiscovered.
    if(GetWorld())
    {
        const bool bCrossingKnown=TravelRuleVersion==FAIHeroesTravelRules::RoadVersion &&
            IsNodeDiscovered(FName(TEXT("FOREST")));
        for(TActorIterator<AActor> It(GetWorld());It;++It)
            if(It->GetOwner()==this && It->ActorHasTag(TEXT("AmberEnvironment")))
                FAIHeroesScenicCrossing::SetShown(**It,bCrossingKnown);
    }
    for (AStaticMeshActor* Road : AdventureRoadActors) if (IsValid(Road)) Road->Destroy();
    AdventureRoadActors.Reset();
    if (!GetWorld() || TravelRuleVersion != FAIHeroesTravelRules::RoadVersion) return;
    const FAIHeroesAdvisorSnapshot Snapshot = BuildAdvisorSnapshot();
    FAIHeroesAdventureGraph Graph;
    FString Error;
    if (!FAIHeroesTravelRules::BuildGraph(Snapshot, Graph, Error)) return;
    UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/AmberMarch/Materials/M_Path.M_Path"));
    if (!Cube) return;
    // Same link list as movement authority; geometry never defines legality.
    for (const auto& Link : Graph.Links)
    {
        if (Link.From.ToString() >= Link.To.ToString() ||
            !IsNodeDiscovered(Link.From) || !IsNodeDiscovered(Link.To)) continue;
        const auto* A = Snapshot.Nodes.FindByPredicate([&Link](const auto& N) { return N.Id == Link.From; });
        const auto* B = Snapshot.Nodes.FindByPredicate([&Link](const auto& N) { return N.Id == Link.To; });
        if (!A || !B) continue;
        FVector From(A->Location.X, A->Location.Y, -5.f);
        FVector To(B->Location.X, B->Location.Y, -5.f);
        FActorSpawnParameters Params;
        Params.Owner = this;
        auto* Road = GetWorld()->SpawnActor<AStaticMeshActor>((From + To) * 0.5,
            (To - From).Rotation(), Params);
        if (!Road) continue;
        auto* Mesh = Road->GetStaticMeshComponent();
        Mesh->SetMobility(EComponentMobility::Movable);
        Mesh->SetStaticMesh(Cube);
        Mesh->SetWorldScale3D(FVector(FVector::Dist(From, To) / 100.f, 0.65f, 0.06f));
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetCastShadow(false);
        if (Base)
        {
            auto* Material = UMaterialInstanceDynamic::Create(Base, Road);
            Material->SetVectorParameterValue(TEXT("Tint"), FLinearColor(0.52f, 0.43f, 0.28f));
            Mesh->SetMaterial(0, Material);
        }
        AdventureRoadActors.Add(Road);
    }
}
