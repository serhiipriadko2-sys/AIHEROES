#include "AIHeroesZoneExpeditionArt.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/GameModeBase.h"
#include "Materials/MaterialInterface.h"
#include "Math/RandomStream.h"
#include "Misc/PackageName.h"

namespace
{
UStaticMesh* AmberMesh(const TCHAR* Name)
{
    return LoadObject<UStaticMesh>(nullptr,
        *FString::Printf(TEXT("/Game/AmberMarch/Meshes/%s.%s"), Name, Name));
}

// A missing optional kit must be a normal fallback, not a load warning.
UStaticMesh* OptionalArchitectureMesh(const TCHAR* Name)
{
#if WITH_EDITOR
    const FString Package = FString::Printf(
        TEXT("/Game/AmberMarch/Meshes/%s"), Name);
    if (!FPackageName::DoesPackageExist(Package)) return nullptr;
#endif
    return AmberMesh(Name);
}

UMaterialInterface* AmberMaterial(const TCHAR* Name)
{
    return LoadObject<UMaterialInterface>(nullptr,
        *FString::Printf(TEXT("/Game/AmberMarch/Materials/M_%s.M_%s"), Name, Name));
}

UStaticMeshComponent* Decor(AActor& Owner, USceneComponent* Root,
                           FName Id, UStaticMesh* Mesh)
{
    if (!Root || !Mesh) return nullptr;
    auto* C = NewObject<UStaticMeshComponent>(&Owner, Id);
    if (!C) return nullptr;
    Owner.AddInstanceComponent(C);
    C->SetupAttachment(Root);
    C->SetMobility(EComponentMobility::Movable);
    C->SetStaticMesh(Mesh);
    C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    C->SetGenerateOverlapEvents(false);
    C->SetCanEverAffectNavigation(false);
    C->RegisterComponent();
    return C;
}

void AddGrove(AActor& Owner, USceneComponent* Root, UStaticMesh* Mesh,
              FName Name, const TArray<FVector2D>& Coordinates,
              float ScaleMin, float ScaleMax, uint32 Seed)
{
    if (!Mesh || !Root || Coordinates.IsEmpty()) return;
    auto* Instances = NewObject<UInstancedStaticMeshComponent>(&Owner, Name);
    Owner.AddInstanceComponent(Instances);
    Instances->SetupAttachment(Root);
    Instances->SetMobility(EComponentMobility::Movable);
    Instances->SetStaticMesh(Mesh);
    Instances->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Instances->SetGenerateOverlapEvents(false);
    Instances->SetCanEverAffectNavigation(false);
    Instances->SetCastShadow(false); // Bounded visual prototype; mobile profiling pending.
    Instances->RegisterComponent();
    FRandomStream R(Seed);
    for (const FVector2D& Point : Coordinates)
    {
        const float S = R.FRandRange(ScaleMin, ScaleMax);
        const FRotator Rotation(0.f, R.FRandRange(0.f, 360.f), 0.f);
        // Ground is original island mesh. Scenery never becomes navigation.
        Instances->AddInstance(FTransform(Rotation,
            FVector(Point.X, Point.Y, 8.f), FVector(S)), false);
    }
}

const TCHAR* NodeMesh(FName Id)
{
    // Additive V6 kit: when optional imported assets are absent the game
    // keeps using the exact pre-existing AmberMarch meshes.
    if (Id == FName(TEXT("HQ")))
        return OptionalArchitectureMesh(TEXT("SM_CitadelCrownV6")) ?
            TEXT("SM_CitadelCrownV6") : TEXT("SM_Hearth");
    if (Id == FName(TEXT("FOREST"))) return TEXT("SM_Sawmill");
    if (Id == FName(TEXT("ORE"))) return TEXT("SM_Quarry");
    if (Id == FName(TEXT("PASS"))) return TEXT("SM_Watchtower");
    if (Id == FName(TEXT("OUTPOST")))
        return OptionalArchitectureMesh(TEXT("SM_FrontierGateV6")) ?
            TEXT("SM_FrontierGateV6") : TEXT("SM_Watchtower");
    if (Id == FName(TEXT("RUINS")))
        return OptionalArchitectureMesh(TEXT("SM_ArchiveSanctumV6")) ?
            TEXT("SM_ArchiveSanctumV6") : TEXT("SM_Archive");
    if (Id == FName(TEXT("SUPPLY"))) return TEXT("SM_Hearth");
    // The optional WAY markers keep their distinct prototype shape.
    return nullptr;
}
}

TArray<FVector2D> FAIHeroesZoneExpeditionArt::Scatter(
    uint32 Seed, int32 MaxItems, uint32 Salt)
{
    TArray<FVector2D> Out;
    if (MaxItems < 0 || MaxItems > 180) return Out;
    Out.Reserve(MaxItems);
    FRandomStream R(static_cast<int32>(Seed ^ Salt));
    // A broad forest framing a readable central route corridor.
    // No hidden node, route or guard is inspected to make these clearings.
    for (int32 Attempt = 0; Attempt < 720 && Out.Num() < MaxItems; ++Attempt)
    {
        const float X = R.FRandRange(-1470.f, 1470.f);
        const float Y = R.FRandRange(-1440.f, 1440.f);
        if (FMath::Abs(X) < 1320.f && FMath::Abs(Y) < 680.f &&
            R.FRand() > .12f) continue;
        // Avoid isolated nearly coincident scenery without graph knowledge.
        const FVector2D Candidate(X, Y);
        if (Out.ContainsByPredicate([&Candidate](const FVector2D& P)
        {
            return FVector2D::DistSquared(P, Candidate) < 125.f * 125.f;
        })) continue;
        Out.Add(Candidate);
    }
    return Out;
}

void FAIHeroesZoneExpeditionArt::StyleRoad(AStaticMeshActor& Road)
{
    auto* Mesh = Road.GetStaticMeshComponent();
    if (!Mesh) return;
    if (auto* Path = AmberMaterial(TEXT("Path")))
        Mesh->SetMaterial(0, Path);
    // Road remains a non-colliding visual over a bounded logical link.
}

bool FAIHeroesZoneExpeditionArt::Apply(
    AGameModeBase& Owner, const FAIHeroesGeneratedWorld& Generated,
    const TMap<FName, TWeakObjectPtr<AStaticMeshActor>>& Zones,
    TWeakObjectPtr<AStaticMeshActor> HeroMarker)
{
    UWorld* World = Owner.GetWorld();
    if (!World || Generated.Zones.Num() < 7 ||
        Generated.Zones.Num() > 7 + FAIHeroesZoneTemplate::MaxExtraRoutes ||
        Zones.Num() != Generated.Zones.Num()) return false;

    // Fail closed to existing cubes if assets were not imported/cooked.
    auto* Island = AmberMesh(TEXT("SM_Island"));
    auto* Pine = AmberMesh(TEXT("SM_Pine"));
    auto* Boulder = AmberMesh(TEXT("SM_Boulder"));
    auto* Seeker = AmberMesh(TEXT("SM_Seeker"));
    if (!Island || !Pine || !Boulder || !Seeker) return false;
    for (const auto& Z : Generated.Zones)
        if (const TCHAR* Name = NodeMesh(Z.Id))
            if (!AmberMesh(Name)) return false;

    for (TActorIterator<AActor> It(World); It; ++It)
        if (It->GetOwner() == &Owner && It->ActorHasTag(TEXT("AmberZoneEnvironment")))
            return true; // Idempotent refresh; do not duplicate meshes.

    FActorSpawnParameters Spawn;
    Spawn.Owner = &Owner;
    Spawn.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    const FVector WorldCentre(1250.f, 0.f, -55.f);
    AActor* Env = World->SpawnActor<AActor>(
        WorldCentre, FRotator::ZeroRotator, Spawn);
    if (!Env) return false;
    Env->Tags.Add(TEXT("AmberZoneEnvironment"));

    auto* Root = NewObject<USceneComponent>(Env, TEXT("AmberExpeditionRoot"));
    if (!Root) { Env->Destroy(); return false; }
    Root->SetMobility(EComponentMobility::Movable);
    Env->SetRootComponent(Root);
    Env->AddInstanceComponent(Root);
    Root->RegisterComponent();
    // A spawned bare AActor has no root when SpawnActor receives its
    // location; attaching a new root must explicitly restore that location.
    // Without this, the island and forest sit at world origin while the
    // camera and generated graph are centred around X=1250.
    Root->SetWorldLocation(WorldCentre);

    if (auto* Land = Decor(*Env, Root, TEXT("Island"), Island))
    {
        Land->SetRelativeLocation(FVector(0.f, 0.f, 15.f));
        // Widen only the non-colliding board so the forested coast frames
        // the playable routes in an angled orthographic camera.
        Land->SetRelativeScale3D(FVector(1.03f, 1.15f, 1.f));
        Land->SetCastShadow(false);
    }
    if (auto* SeaMesh = LoadObject<UStaticMesh>(nullptr,
            TEXT("/Engine/BasicShapes/Plane.Plane")))
    {
        if (auto* Sea = Decor(*Env, Root, TEXT("Sea"), SeaMesh))
        {
            Sea->SetRelativeLocation(FVector(0.f, 0.f, -135.f));
            Sea->SetRelativeScale3D(FVector(44.f, 43.f, 1.f));
            Sea->SetCastShadow(false);
            if (auto* Blue = AmberMaterial(TEXT("Water")))
                Sea->SetMaterial(0, Blue);
        }
    }
    AddGrove(*Env, Root, Pine, TEXT("ExpeditionPines"),
        Scatter(Generated.Seed, 94, 0x3a1915u), .50f, .85f,
        Generated.Seed ^ 0x14f45u);
    AddGrove(*Env, Root, Boulder, TEXT("ExpeditionStones"),
        Scatter(Generated.Seed, 30, 0x778102u), .35f, .72f,
        Generated.Seed ^ 0x19246u);

    for (const auto& Zone : Generated.Zones)
    {
        const TCHAR* Name = NodeMesh(Zone.Id);
        if (!Name) continue;
        const auto* Found = Zones.Find(Zone.Id);
        AStaticMeshActor* Actor = Found ? Found->Get() : nullptr;
        if (!Actor || !Actor->GetStaticMeshComponent()) continue;
        auto* Art = Decor(*Actor, Actor->GetRootComponent(),
            TEXT("AmberExpeditionSite"), AmberMesh(Name));
        if (!Art) continue;
        const bool bLarge = Zone.Role == EAIHeroesZoneRole::Headquarters ||
            Zone.Role == EAIHeroesZoneRole::Checkpoint ||
            Zone.Role == EAIHeroesZoneRole::Objective;
        const float ActorScale = bLarge ? 1.55f : 1.18f;
        Art->SetRelativeLocation(FVector(0.f, 0.f, -58.f / ActorScale));
        // Keep the new, more detailed landmark silhouettes within the
        // existing click-target footprint and camera framing.
        const bool bV6 = FString(Name).EndsWith(TEXT("V6"));
        const float MeshScale = Zone.Id == FName(TEXT("SUPPLY")) ?
            .32f : (bV6 ? .43f : .54f);
        Art->SetRelativeScale3D(FVector(MeshScale));
        Art->SetCastShadow(bV6 && bLarge);
        Actor->GetStaticMeshComponent()->SetVisibility(false, false);
        // The invisible original click collider remains the authority.
    }
    if (AStaticMeshActor* Hero = HeroMarker.Get())
    {
        if (auto* Art = Decor(*Hero, Hero->GetRootComponent(),
                TEXT("AmberExpeditionHero"), Seeker))
        {
            Art->SetRelativeLocation(FVector(0.f, 0.f, -365.f));
            Art->SetRelativeScale3D(FVector(1.5f));
            Hero->GetStaticMeshComponent()->SetVisibility(false, false);
        }
    }
    UE_LOG(LogTemp, Display,
        TEXT("[AMBER EXPEDITION] original_art=1 seed=%u scenery_seed_only=1 gameplay_writes=0 tree_instances=%d stone_instances=%d"),
        Generated.Seed,
        Scatter(Generated.Seed, 94, 0x3a1915u).Num(),
        Scatter(Generated.Seed, 30, 0x778102u).Num());
    return true;
}
