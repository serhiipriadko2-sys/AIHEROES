#include "AIHeroesZonePreviewGameMode.h"

#include "AIHeroesCameraPawn.h"
#include "AIHeroesZoneTemplate.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextRenderActor.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
constexpr float ZoneGridScale = 160.f;
constexpr float ZoneMarkerZ = 8.f;
constexpr float RoadZ = -50.f;

FLinearColor ColorForRole(EAIHeroesZoneRole Role)
{
    switch (Role)
    {
        case EAIHeroesZoneRole::Headquarters: return FLinearColor(0.13f, 0.77f, 0.37f);
        case EAIHeroesZoneRole::Hub:          return FLinearColor(0.14f, 0.70f, 0.92f);
        case EAIHeroesZoneRole::Resource:     return FLinearColor(0.93f, 0.68f, 0.17f);
        case EAIHeroesZoneRole::Checkpoint:   return FLinearColor(0.97f, 0.25f, 0.20f);
        case EAIHeroesZoneRole::Objective:    return FLinearColor(0.64f, 0.29f, 0.98f);
        case EAIHeroesZoneRole::SideRoute:    return FLinearColor(0.24f, 0.73f, 0.74f);
        default:                              return FLinearColor::White;
    }
}

FVector ZoneToWorld(const FAIHeroesGeneratedZone& Zone)
{
    return FVector(Zone.Grid.X * ZoneGridScale, Zone.Grid.Y * ZoneGridScale, ZoneMarkerZ);
}
}

AAIHeroesZonePreviewGameMode::AAIHeroesZonePreviewGameMode()
{
    // Default PlayerController has no AI HEROES game-action input bindings.
    // No SaveGame UI or campaign execution API is exposed to this preview.
    DefaultPawnClass = AAIHeroesCameraPawn::StaticClass();
    PlayerControllerClass = APlayerController::StaticClass();
    PrimaryActorTick.bCanEverTick = true;
}

void AAIHeroesZonePreviewGameMode::BeginPlay()
{
    Super::BeginPlay();

#if UE_BUILD_SHIPPING
    UE_LOG(LogTemp, Error, TEXT("[AI HEROES ZONE PREVIEW] BLOCKED shipping build."));
    return;
#else
    bCaptureEnabled = FParse::Param(FCommandLine::Get(), TEXT("ZoneCapture"));
    if (!GetWorld())
    {
        UE_LOG(LogTemp, Error, TEXT("[AI HEROES ZONE PREVIEW] FAIL missing world."));
        return;
    }
    int32 Seed = 20261028;
    int32 ExtraRoutes = 3;
    FParse::Value(FCommandLine::Get(), TEXT("ZoneSeed="), Seed);
    FParse::Value(FCommandLine::Get(), TEXT("ZoneBranches="), ExtraRoutes);
    if (Seed < 0 || ExtraRoutes < 0 || ExtraRoutes > FAIHeroesZoneTemplate::MaxExtraRoutes)
    {
        UE_LOG(LogTemp, Error, TEXT("[AI HEROES ZONE PREVIEW] FAIL invalid launch options."));
        return;
    }

    FAIHeroesGeneratedWorld Draft;
    FString Error;
    if (!FAIHeroesZoneTemplate::Generate(
            static_cast<uint32>(Seed), ExtraRoutes, Draft, Error) ||
        !FAIHeroesZoneTemplate::Validate(Draft, Error))
    {
        UE_LOG(LogTemp, Error, TEXT("[AI HEROES ZONE PREVIEW] FAIL graph: %s"), *Error);
        return;
    }

    UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (!Cube || !Sphere || !Base)
    {
        UE_LOG(LogTemp, Error, TEXT("[AI HEROES ZONE PREVIEW] FAIL engine preview assets missing."));
        return;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Params.Owner = this;

    const auto SpawnShape = [this, &Params, Base](
        UStaticMesh* Shape, const FVector& Position, const FRotator& Rotation,
        const FVector& Scale, const FLinearColor& Color) -> bool
    {
        AStaticMeshActor* Actor = GetWorld()->SpawnActor<AStaticMeshActor>(
            Position, Rotation, Params);
        if (!Actor) return false;
        UStaticMeshComponent* Mesh = Actor->GetStaticMeshComponent();
        if (!Mesh) return false;
        Mesh->SetMobility(EComponentMobility::Movable);
        Mesh->SetStaticMesh(Shape);
        Mesh->SetWorldScale3D(Scale);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetCastShadow(false);
        UMaterialInstanceDynamic* Material = UMaterialInstanceDynamic::Create(Base, Actor);
        if (Material)
        {
            Material->SetVectorParameterValue(TEXT("Color"), Color);
            Mesh->SetMaterial(0, Material);
        }
        return true;
    };

    // Isolated preview world: only inexpensive original colored primitives.
    // No combat, armies, resources, 3D asset downloads or player save slots.
    if (!SpawnShape(Cube, FVector(1250.f, 0.f, -118.f), FRotator::ZeroRotator,
        FVector(38.f, 36.f, 1.1f), FLinearColor(0.055f, 0.075f, 0.11f)))
    {
        UE_LOG(LogTemp, Error, TEXT("[AI HEROES ZONE PREVIEW] FAIL preview board."));
        return;
    }
    ADirectionalLight* Sun = GetWorld()->SpawnActor<ADirectionalLight>(
        FVector::ZeroVector, FRotator(-65.f, -34.f, 0.f), Params);
    if (Sun) Sun->GetLightComponent()->SetIntensity(3.f);
    ASkyLight* Sky = GetWorld()->SpawnActor<ASkyLight>(
        FVector::ZeroVector, FRotator::ZeroRotator, Params);
    if (Sky)
    {
        Sky->GetLightComponent()->SetIntensity(0.7f);
        Sky->GetLightComponent()->RecaptureSky();
    }

    TMap<FName, FVector> ZonePositions;
    for (const FAIHeroesGeneratedZone& Zone : Draft.Zones)
        ZonePositions.Add(Zone.Id, ZoneToWorld(Zone));

    int32 RoadsRendered = 0;
    for (const FAIHeroesAdventureLink& Road : Draft.Adventure.Links)
    {
        if (Road.From.ToString() >= Road.To.ToString()) continue; // directed pair
        const FVector* From = ZonePositions.Find(Road.From);
        const FVector* To = ZonePositions.Find(Road.To);
        if (!From || !To) continue; // candidate already validated
        FVector Start(From->X, From->Y, RoadZ);
        FVector End(To->X, To->Y, RoadZ);
        const FVector Direction = End - Start;
        if (SpawnShape(Cube, (Start + End) * 0.5f,
                Direction.Rotation(),
                FVector(Direction.Size() / 100.f, 0.25f, 0.11f),
                FLinearColor(0.19f, 0.39f, 0.48f)))
        {
            ++RoadsRendered;
        }
    }

    int32 NodesRendered = 0;
    int32 LabelsRendered = 0;
    for (const FAIHeroesGeneratedZone& Zone : Draft.Zones)
    {
        const FVector Position = ZoneToWorld(Zone);
        const bool bBuilding = Zone.Role == EAIHeroesZoneRole::Headquarters ||
            Zone.Role == EAIHeroesZoneRole::Checkpoint ||
            Zone.Role == EAIHeroesZoneRole::Objective;
        const float Size = Zone.Role == EAIHeroesZoneRole::Headquarters ? 2.05f :
            Zone.Role == EAIHeroesZoneRole::Objective ? 1.65f :
            Zone.Role == EAIHeroesZoneRole::Checkpoint ? 1.5f :
            Zone.Role == EAIHeroesZoneRole::SideRoute ? 0.88f : 1.33f;
        if (SpawnShape(bBuilding ? Cube : Sphere, Position, FRotator::ZeroRotator,
                FVector(Size, Size, bBuilding ? Size * 0.75f : Size),
                ColorForRole(Zone.Role)))
        {
            ++NodesRendered;
        }

        ATextRenderActor* Label = GetWorld()->SpawnActor<ATextRenderActor>(
            Position + FVector(-185.f, 0.f, 160.f), FRotator(90.f, 180.f, 0.f), Params);
        if (Label && Label->GetTextRender())
        {
            UTextRenderComponent* Text = Label->GetTextRender();
            Text->SetText(FText::FromString(
                Zone.Id.ToString() + (Zone.bGuarded ? TEXT(" [GUARD]") : TEXT(""))));
            Text->SetHorizontalAlignment(EHTA_Center);
            Text->SetWorldSize(70.f);
            Text->SetTextRenderColor(FColor(17, 27, 39));
            Text->SetCastShadow(false);
            Text->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            ++LabelsRendered;
        }
    }

    // A dedicated orthographic camera gives repeatable full-map framing.
    // The old Pawn relative pitch could be overridden by controller rotation
    // during possession, leaving a completely black forward-looking view.
    PreviewCamera = GetWorld()->SpawnActor<ACameraActor>(
        FVector(1250.f, 0.f, 4500.f), FRotator(-90.f, 0.f, 0.f), Params);
    if (!IsValid(PreviewCamera) || !PreviewCamera->GetCameraComponent())
    {
        UE_LOG(LogTemp, Error, TEXT("[AI HEROES ZONE PREVIEW] FAIL camera spawn."));
        return;
    }
    UCameraComponent* FixedView = PreviewCamera->GetCameraComponent();
    FixedView->SetProjectionMode(ECameraProjectionMode::Orthographic);
    FixedView->SetOrthoWidth(4500.f);
    FixedView->bConstrainAspectRatio = false;

    // This is a full-world developer view, NOT a fog-limited player view.
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        APawn* Pawn = PC->GetPawn();
        if (!Pawn)
        {
            AAIHeroesCameraPawn* Created = GetWorld()->SpawnActor<AAIHeroesCameraPawn>(
                FVector(1250.f, 0.f, 3050.f), FRotator::ZeroRotator, Params);
            if (Created)
            {
                PC->Possess(Created);
                Pawn = Created;
            }
        }
        if (Pawn)
        {
            Pawn->SetActorLocation(FVector(1250.f, 0.f, 3050.f));
            Pawn->SetActorRotation(FRotator(-44.f, 0.f, 0.f));
            PC->SetViewTarget(PreviewCamera);
        }
    }

    const auto Early = FAIHeroesAdventurePath::Find(
        Draft.Adventure, TEXT("HQ"), TEXT("FOREST"), 1000);
    const auto Locked = FAIHeroesAdventurePath::Find(
        Draft.Adventure, TEXT("HQ"), TEXT("RUINS"), 1000);

    if (NodesRendered != Draft.Zones.Num() ||
        RoadsRendered * 2 != Draft.Adventure.Links.Num() ||
        LabelsRendered != Draft.Zones.Num() || !PC ||
        Early.Status != EAIHeroesAdventurePathStatus::Ready ||
        Locked.Status != EAIHeroesAdventurePathStatus::Unreachable)
    {
        UE_LOG(LogTemp, Error,
            TEXT("[AI HEROES ZONE PREVIEW] FAIL world render nodes=%d roads=%d labels=%d PC=%d early=%d late=%d"),
            NodesRendered, RoadsRendered, LabelsRendered,
            PC ? 1 : 0, static_cast<int32>(Early.Status),
            static_cast<int32>(Locked.Status));
        return;
    }
    UE_LOG(LogTemp, Display,
        TEXT("[AI HEROES ZONE PREVIEW] PASS seed=%u zones=%d roads=%d labels=%d revision=%u early_guard=ready late_guard=locked isolated=1 save_writes=0 campaign_writes=0"),
        Draft.Seed, NodesRendered, RoadsRendered, LabelsRendered, Draft.Revision);
#endif
}

void AAIHeroesZonePreviewGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

#if !UE_BUILD_SHIPPING
    // Re-assert the dedicated preview camera after the PlayerController has
    // completed possession; no gameplay CameraPawn can override this view.
    APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    if (PC && IsValid(PreviewCamera) && PC->GetViewTarget() != PreviewCamera.Get())
    {
        PC->SetViewTarget(PreviewCamera.Get());
    }
    if (!bCaptureEnabled || bCaptureRequested || !GetWorld() ||
        !GetWorld()->GetGameViewport())
    {
        return;
    }
    PreviewElapsed += FMath::Max(0.f, DeltaSeconds);
    if (PreviewElapsed < 2.5f)
    {
        return;
    }
    // The Win32 PrintWindow API often captures a blank D3D swapchain.
    // Read the rendered game viewport through Unreal itself instead.
    const FString Folder = FPaths::Combine(
        FPaths::ProjectSavedDir(), TEXT("QA"), TEXT("ZonePreviewFrames"));
    if (!IFileManager::Get().MakeDirectory(*Folder, true))
    {
        UE_LOG(LogTemp, Error,
            TEXT("[AI HEROES ZONE PREVIEW] FAIL screenshot QA directory."));
        bCaptureRequested = true;
        return;
    }
    const FString File = FPaths::Combine(Folder, FString::Printf(
        TEXT("ZonePreview_%s.png"),
        *FGuid::NewGuid().ToString(EGuidFormats::Digits)));
    bCaptureRequested = true;
    if (PC && PC->PlayerCameraManager)
    {
        UE_LOG(LogTemp, Display,
            TEXT("[AI HEROES ZONE PREVIEW] CAMERA world=%s rotation=%s target=%s"),
            *PC->PlayerCameraManager->GetCameraLocation().ToString(),
            *PC->PlayerCameraManager->GetCameraRotation().ToString(),
            *GetNameSafe(PC->GetViewTarget()));
    }
    FScreenshotRequest::RequestScreenshot(File, false, false);
    UE_LOG(LogTemp, Display,
        TEXT("[AI HEROES ZONE PREVIEW] SCREENSHOT_REQUEST path=%s"),
        *File);
#endif
}
