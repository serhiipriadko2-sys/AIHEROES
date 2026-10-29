#include "AIHeroesZoneExpeditionGameMode.h"

#include "AIHeroesCameraPawn.h"
#include "AIHeroesZoneExpeditionHUD.h"
#include "AIHeroesZoneScreenLayout.h"
#include "AIHeroesZoneJourney.h"
#include "AIHeroesZoneExpeditionArt.h"
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

FString VisibleZoneTitle(FName Id)
{
    if (Id == FName(TEXT("HQ"))) return TEXT("Цитадель Искры");
    if (Id == FName(TEXT("SUPPLY"))) return TEXT("Снабжение");
    if (Id == FName(TEXT("FOREST"))) return TEXT("Лесопилка");
    if (Id == FName(TEXT("ORE"))) return TEXT("Рудник");
    if (Id == FName(TEXT("PASS"))) return TEXT("Перевал");
    if (Id == FName(TEXT("OUTPOST"))) return TEXT("Застава");
    if (Id == FName(TEXT("RUINS"))) return TEXT("Древний архив");
    return Id.ToString();
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
        FVector(100.f, 95.f, 1.1f), FLinearColor(0.052f, 0.073f, 0.104f), false))
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
        Text->SetWorldSize(82.f);
        Text->SetTextRenderColor(FColor(14, 30, 42));
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
        FAIHeroesZoneExpeditionArt::StyleRoad(*Actor);
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
    FixedView->SetOrthoWidth(3900.f); // Diagonal diorama view, not a hidden-map zoom.
    FixedView->bConstrainAspectRatio = false;
    if (auto* PC = GetWorld()->GetFirstPlayerController())
    {
        PC->SetViewTarget(ExpeditionCamera);
        PC->bShowMouseCursor = true;
        PC->bEnableClickEvents = true;
        PC->bEnableTouchEvents = true;
    }
    else return false;
    // Only original, non-colliding art. A missing kit falls back to
    // the validated primitive map without changing campaign rules.
    if (!FAIHeroesZoneExpeditionArt::Apply(*this, Generated, ZoneActors, HeroMarker))
        UE_LOG(LogTemp, Warning, TEXT("[AMBER EXPEDITION] original art unavailable; primitive fallback"));
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
                {
                    FString Caption = VisibleZoneTitle(Zone.Id);
                    if (bCleared) Caption += TEXT(" [НАШЕ]");
                    else if (Zone.bGuarded) Caption += TEXT(" [ОХРАНА]");
                    if (bSelected) Caption = TEXT("> ") + Caption;
                    Label->GetTextRender()->SetText(FText::FromString(Caption));
                }
            }
        if (Zone.Id == Campaign.HeroSite && HeroMarker.IsValid())
            HeroMarker->SetActorLocation(WorldAt(Zone) + FVector(0.f, 0.f, 220.f));
    }
    // Framing is a presentation concern. Follow the player's confirmed
    // location, leaving forward space for the next visible destination.
    // Never follow selection: a read-only click must not grant a move.
    if (IsValid(ExpeditionCamera) && HeroMarker.IsValid())
    {
        const float FocusX = FMath::Clamp(
            HeroMarker->GetActorLocation().X + 280.f, 280.f, 2450.f);
        const FVector Focus(FocusX, 0.f, 0.f);
        const FVector CameraPosition(FocusX - 2300.f, 0.f, 3300.f);
        ExpeditionCamera->SetActorLocation(CameraPosition);
        ExpeditionCamera->SetActorRotation((Focus - CameraPosition).Rotation());
    }
    for (const FRoadVisual& Road : RoadVisuals)
        if (Road.Actor.IsValid())
            Road.Actor->SetActorHiddenInGame(
                !FAIHeroesZoneExpeditionRules::IsDiscovered(Campaign, Road.From) ||
                !FAIHeroesZoneExpeditionRules::IsDiscovered(Campaign, Road.To));
}

void AAIHeroesZoneExpeditionGameMode::GetVisibleSiteMarkers(
    TArray<FAIHeroesVisibleSiteMarker>& Out) const
{
    Out.Reset();
    if (!bReady) return;
    // Only provide actors that have passed the exact player-visible fog gate.
    // Their positions are presentation data, not a navigable graph.
    for (const auto& Zone : Generated.Zones)
    {
        if (!FAIHeroesZoneExpeditionRules::IsDiscovered(Campaign, Zone.Id))
            continue;
        const auto* Found = ZoneActors.Find(Zone.Id);
        const AStaticMeshActor* Actor = Found ? Found->Get() : nullptr;
        if (!IsValid(Actor)) continue;
        FAIHeroesVisibleSiteMarker Marker;
        Marker.Id = Zone.Id;
        Marker.Anchor = Actor->GetActorLocation() + FVector(0.f, 0.f, 270.f);
        Marker.bSelected = Zone.Id == SelectedSite;
        Marker.bCleared = FAIHeroesZoneExpeditionRules::IsCleared(
            Campaign, Zone.Id);
        Marker.bKnownGuard = Zone.bGuarded && !Marker.bCleared;
        Out.Add(Marker);
    }
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

void AAIHeroesZoneExpeditionGameMode::ClickSelectAtScreen(const FVector2D& Position)
{
    if (!bReady || !GetWorld()) return;
    auto* PC = GetWorld()->GetFirstPlayerController();
    if (!PC) return;
    FHitResult Hit;
    if (!PC->GetHitResultAtScreenPosition(Position, ECC_Visibility, false, Hit)) return;
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

void AAIHeroesZoneExpeditionGameMode::PreviewJourney()
{
    if (!bReady || SelectedSite.IsNone()) return;
    const auto Plan = FAIHeroesZoneJourney::Plan(Campaign, SelectedSite);
    if (!Plan.HasPlan())
    {
        Status(TEXT("READ-ONLY JOURNEY BLOCKED | ") + Plan.Explanation);
        return;
    }
    FString Next = TEXT("ALREADY HERE");
    if (!Plan.Steps.IsEmpty())
    {
        const auto& First = Plan.Steps[0];
        if (First.Kind == EAIHeroesJourneyStepKind::EndDay)
            Next = TEXT("NEXT: PLAYER END DAY");
        else
            Next = FString::Printf(
                TEXT("NEXT: CONFIRM %s %d MOVES"),
                *First.Destination.ToString(), First.MoveCost);
    }
    // Arrival day is the earliest *conditional* itinerary over today's
    // visible graph, not a victory/casualty forecast or queued command.
    Status(FString::Printf(
        TEXT("READ-ONLY JOURNEY %s | EARLIEST DAY %d | %d MOVES / %d END-DAYS | %s | %s | NO ORDER EXECUTED"),
        *Plan.Goal.ToString(), Plan.ArrivalDay,
        Plan.TravelConfirmations, Plan.DayAdvances,
        *Next,
        Plan.bGuardianOutcomeUnknown ?
            TEXT("GUARD OUTCOME UNKNOWN") : TEXT("NO KNOWN GOAL GUARD")));
    ScheduleVisualQAFrame();
}

void AAIHeroesZoneExpeditionGameMode::ToggleDossier()
{
    if (!bReady) return;
    if (bDossierOpen)
    {
        bDossierOpen = false;
        Dossier = FAIHeroesZoneExpeditionDossier();
        Status(TEXT("READ-ONLY DOSSIER CLOSED | no orders executed."));
        return;
    }
    FAIHeroesZoneExpeditionDossier Candidate =
        FAIHeroesZoneExpeditionAdvisor::CompareKnown(Campaign);
    if (!Candidate.bValid)
    {
        Status(TEXT("READ-ONLY DOSSIER BLOCKED | map preflight failed."));
        return;
    }
    // The comparison is a frozen player-visible snapshot. Block ALL
    // state-changing keys until D/Esc closes the panel; no stale order.
    Dossier = MoveTemp(Candidate);
    bDossierOpen = true;
    const FString Top = Dossier.Options.IsEmpty()
        ? TEXT("NONE") : Dossier.Options[0].Site.ToString();
    Status(FString::Printf(
        TEXT("READ-ONLY DOSSIER OPEN options=%d known_choices=%d top=%s | no orders."),
        Dossier.Options.Num(), Dossier.ConsideredKnownTargets, *Top));
    ScheduleVisualQAFrame();
}

bool AAIHeroesZoneExpeditionGameMode::HandleScreenPress(
    const FVector2D& Position)
{
    if (!bReady || !GetWorld()) return false;
    auto* PC = GetWorld()->GetFirstPlayerController();
    if (!PC) return false;
    int32 Width = 0;
    int32 Height = 0;
    PC->GetViewportSize(Width, Height);
    const auto Layout = FAIHeroesZoneScreenLayout::Build(
        FVector2D(Width, Height), bDossierOpen);
    const auto Action = Layout.HitTest(Position);
    if (bDossierOpen)
    {
        // Modal closes ONLY on the explicit CLOSE rect. Every other
        // screen press is swallowed; it never selects a hidden map node.
        if (Action == EAIHeroesZoneScreenAction::CloseComparison)
        {
            UE_LOG(LogTemp, Display,
                TEXT("[ZONE INPUT] POINTER BUTTON CLOSE ROUTES"));
            ToggleDossier();
        }
        return true;
    }
    if (Action == EAIHeroesZoneScreenAction::None)
    {
        // Header, bottom status and action-bar gutters are never
        // projected into the 3D world as selection attempts.
        const float BarTop = Layout.Buttons.IsEmpty() ?
            Height - 185.f : Layout.Buttons[0].Rect.Y - 9.f;
        return Position.Y < 137.f || Position.Y >= BarTop ||
            Layout.ContainsChrome(Position);
    }

    UE_LOG(LogTemp, Display, TEXT("[ZONE INPUT] POINTER BUTTON %s"),
        FAIHeroesZoneScreenLayout::Name(Action));
    switch (Action)
    {
        case EAIHeroesZoneScreenAction::NextSite:
            CycleSelection(); break;
        case EAIHeroesZoneScreenAction::HeroAdvice:
            AskHero(false); break;
        case EAIHeroesZoneScreenAction::InspectSite:
            AskHero(true); break;
        case EAIHeroesZoneScreenAction::CompareRoutes:
            ToggleDossier(); break;
        case EAIHeroesZoneScreenAction::ConfirmMove:
            ConfirmSelected(); break;
        case EAIHeroesZoneScreenAction::EndDay:
            AdvanceDay(); break;
        case EAIHeroesZoneScreenAction::Recruit:
            Recruit(); break;
        case EAIHeroesZoneScreenAction::UpgradeProtocol:
            Upgrade(); break;
        case EAIHeroesZoneScreenAction::Save:
            Save(); break;
        case EAIHeroesZoneScreenAction::Load:
            Load(); break;
        default:
            break; // Includes malformed or modal-only actions.
    }
    return true;
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
    if (PC->WasInputKeyJustPressed(EKeys::D))
    {
        ToggleDossier();
        return;
    }
    if (bDossierOpen && PC->WasInputKeyJustPressed(EKeys::Escape))
    {
        ToggleDossier();
        return;
    }

    // Screen touch and mouse take the same pure Canvas rectangle path.
    // Touch first prevents a platform-synthesized mouse press from
    // double-applying a movement, purchase, turn or SaveGame action.
    if (PC->WasInputKeyJustPressed(EKeys::TouchKeys[ETouchIndex::Touch1]))
    {
        float TouchX = 0.f;
        float TouchY = 0.f;
        bool bPressed = false;
        PC->GetInputTouchState(
            ETouchIndex::Touch1, TouchX, TouchY, bPressed);
        if (bPressed && FMath::IsFinite(TouchX) &&
            FMath::IsFinite(TouchY))
        {
            const FVector2D Point(TouchX, TouchY);
            if (!HandleScreenPress(Point))
                ClickSelectAtScreen(Point);
        }
        return;
    }
    if (PC->WasInputKeyJustPressed(EKeys::LeftMouseButton))
    {
        float MouseX = 0.f;
        float MouseY = 0.f;
        if (PC->GetMousePosition(MouseX, MouseY) &&
            FMath::IsFinite(MouseX) && FMath::IsFinite(MouseY))
        {
            const FVector2D Point(MouseX, MouseY);
            if (!HandleScreenPress(Point))
                ClickSelectAtScreen(Point);
        }
        return;
    }
    if (bDossierOpen) return; // No key orders or saves in a modal.

    if (PC->WasInputKeyJustPressed(EKeys::Tab)) CycleSelection();
    if (PC->WasInputKeyJustPressed(EKeys::Enter)) ConfirmSelected();
    if (PC->WasInputKeyJustPressed(EKeys::SpaceBar)) AdvanceDay();
    if (PC->WasInputKeyJustPressed(EKeys::R)) Recruit();
    if (PC->WasInputKeyJustPressed(EKeys::P)) Upgrade();
    if (PC->WasInputKeyJustPressed(EKeys::H))
        AskHero(PC->IsInputKeyDown(EKeys::LeftShift) ||
                PC->IsInputKeyDown(EKeys::RightShift));
    if (PC->WasInputKeyJustPressed(EKeys::I)) PreviewJourney();
    if (PC->WasInputKeyJustPressed(EKeys::K)) Save();
    if (PC->WasInputKeyJustPressed(EKeys::L)) Load();
}
