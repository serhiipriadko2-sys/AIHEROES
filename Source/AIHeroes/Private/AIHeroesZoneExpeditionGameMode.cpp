#include "AIHeroesZoneExpeditionGameMode.h"

#include "AIHeroesCameraPawn.h"
#include "AIHeroesZoneExpeditionHUD.h"
#include "AIHeroesZoneExpeditionAdvisor.h"
#include "AIHeroesZoneExpeditionStore.h"
#include "Camera/CameraComponent.h"
#include "HAL/FileManager.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"
#include "Misc/Guid.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Camera/CameraActor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextRenderActor.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
constexpr float GridScale = 160.f;
constexpr float MarkerZ = 8.f;
constexpr float ZoneExpeditionRoadZ = -45.f;

FVector WorldAt(const FAIHeroesGeneratedZone& Z)
{
    return FVector(Z.Grid.X * GridScale, Z.Grid.Y * GridScale, MarkerZ);
}

FLinearColor SiteColor(EAIHeroesZoneRole Role)
{
    switch (Role)
    {
        case EAIHeroesZoneRole::Headquarters: return FLinearColor(0.10f, 0.68f, 0.37f);
        case EAIHeroesZoneRole::Hub: return FLinearColor(0.16f, 0.76f, 0.89f);
        case EAIHeroesZoneRole::Resource: return FLinearColor(0.9f, 0.65f, 0.20f);
        case EAIHeroesZoneRole::Checkpoint: return FLinearColor(0.92f, 0.24f, 0.17f);
        case EAIHeroesZoneRole::Objective: return FLinearColor(0.63f, 0.27f, 0.9f);
        case EAIHeroesZoneRole::SideRoute: return FLinearColor(0.36f, 0.67f, 0.68f);
        default: return FLinearColor::White;
    }
}
}

AAIHeroesZoneExpeditionGameMode::AAIHeroesZoneExpeditionGameMode()
{
    PrimaryActorTick.bCanEverTick = true;
    DefaultPawnClass = AAIHeroesCameraPawn::StaticClass();
    PlayerControllerClass = APlayerController::StaticClass();
    HUDClass = AAIHeroesZoneExpeditionHUD::StaticClass();
}

void AAIHeroesZoneExpeditionGameMode::Status(const FString& Message)
{
    LastMessage = Message.Left(300);
    UE_LOG(LogTemp, Display, TEXT("[ZONE EXPEDITION] %s"), *LastMessage);
}

void AAIHeroesZoneExpeditionGameMode::BeginPlay()
{
    Super::BeginPlay();

    int32 Seed = 20261029;
    int32 Branches = 3;
    FParse::Value(FCommandLine::Get(), TEXT("ZoneSeed="), Seed);
    FParse::Value(FCommandLine::Get(), TEXT("ZoneBranches="), Branches);
    if (Seed < 0 || Branches < 0 || Branches > FAIHeroesZoneTemplate::MaxExtraRoutes)
    {
        Status(TEXT("BLOCKED invalid expedition launch seed or route count."));
        return;
    }
#if !UE_BUILD_SHIPPING
    bVisualQAEnabled = FParse::Param(FCommandLine::Get(), TEXT("ZoneExpeditionCapture"));
#endif
    FString Error;
    if (!FAIHeroesZoneExpeditionRules::Create(
            static_cast<uint32>(Seed), Branches, Campaign, Error) ||
        !FAIHeroesZoneTemplate::Generate(Campaign.Seed, Campaign.BranchCount, Generated, Error))
    {
        Status(TEXT("BLOCKED cannot generate or validate zone expedition: ") + Error);
        return;
    }

    SelectedSite = FName(TEXT("FOREST"));
    if (!CreateScene())
    {
        Status(TEXT("BLOCKED 3D scene unavailable; game never began."));
        return;
    }
    bReady = true;
    RefreshScene();
    ScheduleVisualQAFrame();
    Status(FString::Printf(
        TEXT("READY seed=%u zones=%d known=%d/ %d | CLICK or TAB select, ENTER confirm. No automatic orders or saves."),
        Campaign.Seed, Generated.Zones.Num(),
        FAIHeroesZoneExpeditionRules::DiscoveredCount(Campaign), Generated.Zones.Num()));
}

bool AAIHeroesZoneExpeditionGameMode::CreateScene()
{
    if (!GetWorld()) return false;
    UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    if (!Cube || !Sphere || !Base) return false;
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Params.Owner = this;

    auto SpawnShape = [this, &Params, Base](UStaticMesh* Shape, FVector Position,
        FRotator Rotation, FVector Scale, const FLinearColor& Color,
        bool bClickTarget) -> AStaticMeshActor*
    {
        auto* Actor = GetWorld()->SpawnActor<AStaticMeshActor>(Position, Rotation, Params);
        if (!Actor) return nullptr;
        auto* Mesh = Actor->GetStaticMeshComponent();
        Mesh->SetMobility(EComponentMobility::Movable);
        Mesh->SetStaticMesh(Shape);
        Mesh->SetWorldScale3D(Scale);
        Mesh->SetCastShadow(false);
        Mesh->SetCollisionEnabled(bClickTarget
            ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
        if (bClickTarget)
        {
            Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
            Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
        }
        UMaterialInstanceDynamic* Material =
            UMaterialInstanceDynamic::Create(Base, Actor);
        if (Material)
        {
            Material->SetVectorParameterValue(TEXT("Color"), Color);
            Mesh->SetMaterial(0, Material);
        }
        return Actor;
    };

    if (!SpawnShape(Cube, FVector(1250.f, 0.f, -118.f), FRotator::ZeroRotator,
        FVector(38.f, 36.f, 1.1f), FLinearColor(0.052f, 0.073f, 0.104f), false))
        return false;
    if (auto* Sun = GetWorld()->SpawnActor<ADirectionalLight>(
        FVector::ZeroVector, FRotator(-65.f, -34.f, 0.f), Params))
        Sun->GetLightComponent()->SetIntensity(3.f);
    if (auto* Sky = GetWorld()->SpawnActor<ASkyLight>())
    {
        Sky->GetLightComponent()->SetIntensity(0.7f);
        Sky->GetLightComponent()->RecaptureSky();
    }

    for (const auto& Zone : Generated.Zones)
    {
        const bool bBuilding =
            Zone.Role == EAIHeroesZoneRole::Headquarters ||
            Zone.Role == EAIHeroesZoneRole::Checkpoint ||
            Zone.Role == EAIHeroesZoneRole::Objective;
        const float Size = bBuilding ? 1.55f : 1.18f;
        AStaticMeshActor* Actor = SpawnShape(
            bBuilding ? Cube : Sphere, WorldAt(Zone), FRotator::ZeroRotator,
            FVector(Size, Size, bBuilding ? Size * 0.85f : Size),
            SiteColor(Zone.Role), true);
        if (!Actor) return false;
        ZoneActors.Add(Zone.Id, Actor);
        auto* Label = GetWorld()->SpawnActor<ATextRenderActor>(
            WorldAt(Zone) + FVector(-160.f, 0.f, 165.f),
            FRotator(90.f, 180.f, 0.f), Params);
        if (!Label || !Label->GetTextRender()) return false;
        auto* Text = Label->GetTextRender();
        Text->SetWorldSize(58.f);
        Text->SetTextRenderColor(FColor(15, 25, 39));
        Text->SetHorizontalAlignment(EHTA_Center);
        Text->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Text->SetCastShadow(false);
        ZoneLabels.Add(Zone.Id, Label);
    }

    for (const auto& Road : Generated.Adventure.Links)
    {
        if (Road.From.ToString() >= Road.To.ToString()) continue;
        const auto* From = Generated.Zones.FindByPredicate(
            [&Road](const auto& Z) { return Z.Id == Road.From; });
        const auto* To = Generated.Zones.FindByPredicate(
            [&Road](const auto& Z) { return Z.Id == Road.To; });
        if (!From || !To) return false;
        FVector A = WorldAt(*From); A.Z = ZoneExpeditionRoadZ;
        FVector B = WorldAt(*To); B.Z = ZoneExpeditionRoadZ;
        const FVector Delta = B - A;
        auto* Actor = SpawnShape(Cube, (A + B) * .5f, Delta.Rotation(),
            FVector(Delta.Size() / 100.f, .20f, .10f),
            FLinearColor(.21f, .37f, .46f), false);
        if (!Actor) return false;
        RoadVisuals.Add({Actor, Road.From, Road.To});
    }
    const auto* HQ = Generated.Zones.FindByPredicate(
        [](const auto& Z) { return Z.Id == FName(TEXT("HQ")); });
    if (!HQ) return false;
    AStaticMeshActor* Marker = SpawnShape(
        Sphere, WorldAt(*HQ) + FVector(0.f, 0.f, 200.f),
        FRotator::ZeroRotator, FVector(.75f, .75f, .75f),
        FLinearColor(.20f, 1.f, .80f), false);
    if (!Marker) return false;
    HeroMarker = Marker;

    ExpeditionCamera = GetWorld()->SpawnActor<ACameraActor>(
        FVector(1250.f, 0.f, 4500.f), FRotator(-90.f, 0.f, 0.f), Params);
    if (!IsValid(ExpeditionCamera) || !ExpeditionCamera->GetCameraComponent())
        return false;
    auto* FixedView = ExpeditionCamera->GetCameraComponent();
    FixedView->SetProjectionMode(ECameraProjectionMode::Orthographic);
    FixedView->SetOrthoWidth(4500.f);
    FixedView->bConstrainAspectRatio = false;
    if (auto* PC = GetWorld()->GetFirstPlayerController())
    {
        PC->SetViewTarget(ExpeditionCamera);
        PC->bShowMouseCursor = true;
        PC->bEnableClickEvents = true;
    }
    else return false;
    return ZoneActors.Num() == Generated.Zones.Num() &&
        ZoneLabels.Num() == Generated.Zones.Num();
}

void AAIHeroesZoneExpeditionGameMode::RefreshScene()
{
    if (!bReady) return;
    for (const auto& Zone : Generated.Zones)
    {
        const bool bKnown = FAIHeroesZoneExpeditionRules::IsDiscovered(Campaign, Zone.Id);
        const bool bCleared = FAIHeroesZoneExpeditionRules::IsCleared(Campaign, Zone.Id);
        const bool bSelected = Zone.Id == SelectedSite;
        if (auto* Found = ZoneActors.Find(Zone.Id))
            if (auto* Actor = Found->Get())
            {
                Actor->SetActorHiddenInGame(!bKnown);
                Actor->SetActorEnableCollision(bKnown);
                // Preserve the authored marker dimensions instead of shrinking
                // every unselected world object to the unit cube.
                const bool bBuilding = Zone.Role == EAIHeroesZoneRole::Headquarters ||
                    Zone.Role == EAIHeroesZoneRole::Checkpoint ||
                    Zone.Role == EAIHeroesZoneRole::Objective;
                const float BaseSize = bBuilding ? 1.55f : 1.18f;
                const float Highlight = bSelected && bKnown ? 1.18f : 1.f;
                Actor->SetActorScale3D(FVector(
                    BaseSize * Highlight, BaseSize * Highlight,
                    BaseSize * (bBuilding ? .85f : 1.f) * Highlight));
            }
        if (auto* Found = ZoneLabels.Find(Zone.Id))
            if (auto* Label = Found->Get())
            {
                Label->SetActorHiddenInGame(!bKnown);
                if (Label->GetTextRender())
                    Label->GetTextRender()->SetText(FText::FromString(
                        Zone.Id.ToString() +
                        (bCleared ? TEXT(" [OURS]") :
                            Zone.bGuarded ? TEXT(" [GUARD]") : TEXT("")) +
                        (bSelected ? TEXT(" [SELECT]") : TEXT(""))));
            }
        if (Zone.Id == Campaign.HeroSite && HeroMarker.IsValid())
            HeroMarker->SetActorLocation(WorldAt(Zone) + FVector(0.f, 0.f, 220.f));
    }
    for (const FRoadVisual& Road : RoadVisuals)
        if (Road.Actor.IsValid())
            Road.Actor->SetActorHiddenInGame(
                !FAIHeroesZoneExpeditionRules::IsDiscovered(Campaign, Road.From) ||
                !FAIHeroesZoneExpeditionRules::IsDiscovered(Campaign, Road.To));
}

FString AAIHeroesZoneExpeditionGameMode::GetSelectedPathInfo() const
{
    if (!bReady || SelectedSite.IsNone()) return TEXT("No site selected.");
    const auto Path = FAIHeroesZoneExpeditionRules::Preview(Campaign, SelectedSite);
    if (Path.Status == EAIHeroesAdventurePathStatus::Ready)
        return FString::Printf(TEXT("READY %d MOVES%s | PLAYER DECIDES"),
            Path.TotalCost, Path.bBattleAtDestination ? TEXT(" | GUARDED BATTLE") : TEXT(""));
    if (Path.Status == EAIHeroesAdventurePathStatus::NeedsMoreMoves)
        return FString::Printf(TEXT("NEED %d MORE MOVES"), Path.MissingMoves);
    return TEXT("BLOCKED: ") + Path.Reason;
}

void AAIHeroesZoneExpeditionGameMode::CycleSelection()
{
    if (!bReady) return;
    const int32 Current = Generated.Zones.IndexOfByPredicate(
        [this](const auto& Z) { return Z.Id == SelectedSite; });
    for (int32 Step = 1; Step <= Generated.Zones.Num(); ++Step)
    {
        const int32 Index = (Current + Step + Generated.Zones.Num()) % Generated.Zones.Num();
        const FName Id = Generated.Zones[Index].Id;
        if (FAIHeroesZoneExpeditionRules::IsDiscovered(Campaign, Id))
        {
            SelectedSite = Id;
            RefreshScene();
            Status(TEXT("Selected known zone ") + SelectedSite.ToString() +
                TEXT(". ENTER is the only movement/attack confirmation."));
            return;
        }
    }
}

void AAIHeroesZoneExpeditionGameMode::ClickSelect()
{
    if (!bReady || !GetWorld()) return;
    auto* PC = GetWorld()->GetFirstPlayerController();
    if (!PC) return;
    FHitResult Hit;
    if (!PC->GetHitResultUnderCursor(ECC_Visibility, false, Hit)) return;
    for (const auto& Entry : ZoneActors)
    {
        if (Entry.Value.Get() != Hit.GetActor() ||
            !FAIHeroesZoneExpeditionRules::IsDiscovered(Campaign, Entry.Key))
            continue;
        SelectedSite = Entry.Key;
        RefreshScene();
        Status(TEXT("Selected known zone ") + SelectedSite.ToString() +
            TEXT(". No order executed until ENTER."));
        break;
    }
}

void AAIHeroesZoneExpeditionGameMode::ConfirmSelected()
{
    if (!bReady) return;
    FAIHeroesZoneExpeditionResult Result;
    if (!FAIHeroesZoneExpeditionRules::ConfirmPlayerDestination(
        Campaign, SelectedSite, Result))
    {
        Status(TEXT("MOVE REJECTED: ") + Result.Message);
        return;
    }
    RefreshScene();
    ScheduleVisualQAFrame();
    Status(FString::Printf(
        TEXT("PLAYER CONFIRMED %s | battle=%d won=%d known=%d/%d | %s"),
        *SelectedSite.ToString(), Result.bBattle ? 1 : 0,
        Result.bVictory ? 1 : 0, Result.DiscoveredCount,
        Generated.Zones.Num(), *Result.Message));
    if (FAIHeroesZoneExpeditionRules::HasWon(Campaign))
        Status(TEXT("EXPEDITION VICTORY: OUTPOST and RUINS secured. Only K saves."));
}

void AAIHeroesZoneExpeditionGameMode::AdvanceDay()
{
    FString Error;
    if (FAIHeroesZoneExpeditionRules::EndPlayerDay(Campaign, Error))
    {
        ScheduleVisualQAFrame();
        Status(Campaign.LastEvent);
    }
    else Status(TEXT("DAY BLOCKED: ") + Error);
}

void AAIHeroesZoneExpeditionGameMode::Recruit()
{
    FString Error;
    if (FAIHeroesZoneExpeditionRules::RecruitAtHeadquarters(Campaign, Error))
        Status(Campaign.LastEvent);
    else Status(TEXT("RECRUIT BLOCKED: ") + Error);
}

void AAIHeroesZoneExpeditionGameMode::Upgrade()
{
    FString Error;
    if (FAIHeroesZoneExpeditionRules::UpgradeProtocol(Campaign, Error))
        Status(Campaign.LastEvent);
    else Status(TEXT("PROTOCOL BLOCKED: ") + Error);
}

void AAIHeroesZoneExpeditionGameMode::AskHero(bool bInspectSelection)
{
    if (!bReady) return;
    // H recommends from the player's discovered graph; Shift+H inspects
    // only the selected, known target. Neither path grants an action.
    const auto Advice = FAIHeroesZoneExpeditionAdvisor::Advise(
        Campaign, bInspectSelection ? SelectedSite : NAME_None);
    Status(TEXT("READ-ONLY HERO ADVICE | ") + Advice.Message);
}

void AAIHeroesZoneExpeditionGameMode::Save()
{
    FString Message;
    if (FAIHeroesZoneExpeditionStore::Save(
        Campaign, FAIHeroesZoneExpeditionStore::ProductionPrefix(), Message))
        Status(Message);
    else Status(TEXT("SAVE BLOCKED: ") + Message);
}

void AAIHeroesZoneExpeditionGameMode::Load()
{
    const auto Result = FAIHeroesZoneExpeditionStore::Load(
        FAIHeroesZoneExpeditionStore::ProductionPrefix());
    if (!Result.bOk)
    {
        Status(TEXT("LOAD BLOCKED: ") + Result.Message);
        return;
    }
    // No state/visual half-commit: this window has one fixed generated map.
    // Different map seed is recoverable by reopening the matching URL.
    if (Result.State.MapRevision != Campaign.MapRevision ||
        Result.State.Seed != Campaign.Seed ||
        Result.State.BranchCount != Campaign.BranchCount)
    {
        Status(TEXT("LOAD BLOCKED: saved seed differs; relaunch matching map. No changes."));
        return;
    }
    Campaign = Result.State;
    SelectedSite = Campaign.HeroSite;
    RefreshScene();
    Status(FString::Printf(TEXT("LOADED V2 sequence=%d day=%d known=%d/%d %s"),
        Result.Sequence, Campaign.Day,
        FAIHeroesZoneExpeditionRules::DiscoveredCount(Campaign),
        Generated.Zones.Num(), *Result.Message));
}

void AAIHeroesZoneExpeditionGameMode::ScheduleVisualQAFrame()
{
#if !UE_BUILD_SHIPPING
    if (!bVisualQAEnabled) return;
    ++VisualQASerial;
    VisualQACountdown = VisualQASerial == 0 ? 2.5f : 1.0f;
#endif
}

void AAIHeroesZoneExpeditionGameMode::CaptureVisualQAFrameTick(float DeltaSeconds)
{
#if !UE_BUILD_SHIPPING
    if (!bVisualQAEnabled || VisualQACountdown < 0.f || !GetWorld() ||
        !GetWorld()->GetGameViewport()) return;
    VisualQACountdown -= FMath::Max(0.f, DeltaSeconds);
    if (VisualQACountdown > 0.f) return;
    VisualQACountdown = -1.f;

    // PrintWindow frequently produces a black Unreal D3D swapchain.
    // Request an engine-authored frame only in explicitly opted-in QA.
    const FString Folder = FPaths::Combine(
        FPaths::ProjectSavedDir(), TEXT("QA"), TEXT("ZoneExpeditionFrames"));
    if (!IFileManager::Get().MakeDirectory(*Folder, true))
    {
        UE_LOG(LogTemp, Error, TEXT("[ZONE EXPEDITION] QA_CAPTURE_DIRECTORY_FAILED"));
        return;
    }
    const FString File = FPaths::Combine(Folder, FString::Printf(
        TEXT("Expedition_%02d_%s.png"), VisualQASerial,
        *FGuid::NewGuid().ToString(EGuidFormats::Digits)));
    FScreenshotRequest::RequestScreenshot(File, false, false);
    UE_LOG(LogTemp, Display,
        TEXT("[ZONE EXPEDITION] SCREENSHOT_REQUEST serial=%d path=%s"),
        VisualQASerial, *File);
#endif
}

void AAIHeroesZoneExpeditionGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bReady || !GetWorld()) return;
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (!PC) return;
    // Pawn possession can override the initial camera target during setup.
    if (IsValid(ExpeditionCamera) && PC->GetViewTarget() != ExpeditionCamera.Get())
        PC->SetViewTarget(ExpeditionCamera.Get());
    CaptureVisualQAFrameTick(DeltaSeconds);
    if (PC->WasInputKeyJustPressed(EKeys::Tab)) CycleSelection();
    if (PC->WasInputKeyJustPressed(EKeys::LeftMouseButton)) ClickSelect();
    if (PC->WasInputKeyJustPressed(EKeys::Enter)) ConfirmSelected();
    if (PC->WasInputKeyJustPressed(EKeys::SpaceBar)) AdvanceDay();
    if (PC->WasInputKeyJustPressed(EKeys::R)) Recruit();
    if (PC->WasInputKeyJustPressed(EKeys::P)) Upgrade();
    if (PC->WasInputKeyJustPressed(EKeys::H))
        AskHero(PC->IsInputKeyDown(EKeys::LeftShift) ||
                PC->IsInputKeyDown(EKeys::RightShift));
    if (PC->WasInputKeyJustPressed(EKeys::K)) Save();
    if (PC->WasInputKeyJustPressed(EKeys::L)) Load();
}
