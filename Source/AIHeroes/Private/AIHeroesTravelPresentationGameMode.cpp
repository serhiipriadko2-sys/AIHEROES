#include "AIHeroesGameMode.h"

#include "AIHeroesTravelPresentation.h"
#include "AIHeroesArtDirection.h"
#include "AIHeroesWorldNode.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"

bool AAIHeroesGameMode::BuildTravelViewPoints(const FAIHeroesAdventurePathResult& Path,
    const FVector& Origin, TArray<FVector>& Out) const
{
    Out.Reset();
    if ((Path.Status != EAIHeroesAdventurePathStatus::Ready &&
         Path.Status != EAIHeroesAdventurePathStatus::NeedsMoreMoves) ||
        Path.Nodes.IsEmpty() || Path.Nodes.Num() > 128 || Origin.ContainsNaN()) return false;
    Out.Add(Origin);
    for (FName Id : Path.Nodes)
    {
        const AAIHeroesWorldNode* Found = nullptr;
        for (const AAIHeroesWorldNode* Node : Nodes)
        {
            if (!IsValid(Node) || FName(*Node->GetDisplayName()) != Id) continue;
            if (Found) { Out.Reset(); return false; }
            Found = Node;
        }
        if (!Found || Found->IsHidden() || !IsNodeDiscovered(Id)) { Out.Reset(); return false; }
        const FVector At = Found->GetActorLocation();
        if (!Out.Last().Equals(At, 0.01)) Out.Add(At);
    }
    FAIHeroesTravelMotion Validation;
    if (!Validation.Begin(Out)) { Out.Reset(); return false; }
    return true;
}

AAIHeroesTravelPresentation* AAIHeroesGameMode::EnsureTravelPresentation()
{
    if (IsValid(TravelPresentation.Get())) return TravelPresentation;
    if (!GetWorld()) return nullptr;
    FActorSpawnParameters Params;
    Params.Owner = this;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    TravelPresentation = GetWorld()->SpawnActor<AAIHeroesTravelPresentation>(
        FVector::ZeroVector, FRotator::ZeroRotator, Params);
    return TravelPresentation;
}

void AAIHeroesGameMode::PresentStrategicTravel(
    const FAIHeroesAdventurePathResult& Path, const FVector& Origin)
{
    // Called ONLY after the existing non-combat transaction commits.
    // A visual allocation/validation failure cannot undo or duplicate gameplay.
    if (!IsValid(HeroMarker.Get()) || Path.Status != EAIHeroesAdventurePathStatus::Ready) return;
    TArray<FVector> Points;
    if (!BuildTravelViewPoints(Path, Origin, Points) ||
        !Points.Last().Equals(HeroStrategicLocation, 0.01)) return;
    if (AAIHeroesTravelPresentation* View = EnsureTravelPresentation())
        View->BeginMovement(HeroMarker, Points);
}

void AAIHeroesGameMode::FinishStrategicTravelPresentation()
{
    if (IsValid(TravelPresentation.Get())) TravelPresentation->FinishAt(HeroStrategicLocation);
}

bool AAIHeroesGameMode::IsStrategicTravelMoving() const
{
    return IsValid(TravelPresentation.Get()) && TravelPresentation->IsMoving();
}

void AAIHeroesGameMode::TickStrategicTravelPresentation(float DeltaSeconds)
{
    if (IsLiveBattleActive() || IsBattleReplayActive() || IsBasePanelVisible() || IsAdvisorDossierVisible())
    {
        FinishStrategicTravelPresentation();
        return;
    }
    if (IsValid(TravelPresentation.Get()))
    {
        TravelPresentation->Advance(DeltaSeconds, HeroStrategicLocation);
        if (TravelPresentation->IsMoving()) return;
    }
    // Restore the marker even when a missing presenter is replaced for a new preview.
    if (IsValid(HeroMarker.Get()))
        HeroMarker->SetActorLocation(HeroStrategicLocation + AAIHeroesTravelPresentation::MarkerOffset());
    const FAIHeroesAdventurePathResult Path = GetSelectedTravelPath();
    TArray<FVector> Points;
    if (!BuildTravelViewPoints(Path, HeroStrategicLocation, Points))
    {
        if (IsValid(TravelPresentation.Get())) TravelPresentation->ClearPath();
        // Lost disposable presenter must never leave a marker stranded mid-road.
        if (IsValid(HeroMarker.Get()))
            HeroMarker->SetActorLocation(HeroStrategicLocation + AAIHeroesTravelPresentation::MarkerOffset());
        return;
    }
    if (AAIHeroesTravelPresentation* View = EnsureTravelPresentation()) View->ShowPath(Points, Path.Status);
}

void AAIHeroesGameMode::DestroyTravelPresentation()
{
    FinishStrategicTravelPresentation();
    if (IsValid(TravelPresentation.Get())) TravelPresentation->Destroy();
    TravelPresentation = nullptr;
}

void AAIHeroesGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    FAIHeroesArtDirection::RemoveWorld(*this);
    CancelLocalModelExchange();
    DestroyTravelPresentation();
    Super::EndPlay(EndPlayReason);
}

void AAIHeroesGameMode::Destroyed()
{
    FAIHeroesArtDirection::RemoveWorld(*this);
    CancelLocalModelExchange();
    DestroyTravelPresentation();
    Super::Destroyed();
}
