#include "AIHeroesGameMode.h"

#include "AIHeroesAdvisor.h"
#include "AIHeroesArtDirection.h"
#include "AIHeroesBaseRules.h"
#include "AIHeroesDevelopmentRules.h"
#include "AIHeroesRecruitmentAdvisor.h"
#include "AIHeroesBattleEngine.h"
#include "AIHeroesBattleRecord.h"
#include "AIHeroesCampaignRules.h"
#include "AIHeroesCampaignSave.h"
#include "AIHeroesLoadPreflight.h"
#include "AIHeroesSaveSystem.h"
#include "AIHeroesCameraPawn.h"
#include "AIHeroesHUD.h"
#include "AIHeroesPlayerController.h"
#include "AIHeroesTurnRules.h"
#include "AIHeroesTravelRules.h"
#include "AIHeroesWorldNode.h"
#include "AIHeroesWorldWeek.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UObject/UObjectGlobals.h"

namespace
{
// Human campaign slots, each with two internal rotating recovery snapshots.
// Slot 1 retains the old resilient prefix for backward compatibility.
FString SaveNamespaceForPlayer(int32 Slot)
{
    const FString& Root = FAIHeroesSaveSystem::CampaignPrefix();
    return Slot == 1 ? Root : FString::Printf(TEXT("%s_Player%d"), *Root, Slot);
}

FString LegacySlotForPlayer(int32 Slot)
{
    return Slot == 1 ? UAIHeroesCampaignSave::SlotName() : FString();
}
}

AAIHeroesGameMode::AAIHeroesGameMode()
{
    PrimaryActorTick.bCanEverTick = true;
    DefaultPawnClass = AAIHeroesCameraPawn::StaticClass();
    PlayerControllerClass = AAIHeroesPlayerController::StaticClass();
    HUDClass = AAIHeroesHUD::StaticClass();
}

void AAIHeroesGameMode::BeginPlay()
{
    Super::BeginPlay();
    CancelLocalModelExchange();
    LocalModelReplies.Reset();
    TravelRuleVersion = FAIHeroesTravelRules::RoadVersion;
    bFogEnabled = true;
    FogKnownMask = FAIHeroesFogRules::InitialMask;

    AddStack(FAIHeroesArmyStack(TEXT("Militia"), 8, 12, 4, 2, 3));
    AddStack(FAIHeroesArmyStack(TEXT("Scout"), 3, 8, 6, 1, 7));
    SpawnWorld();
    SpawnFieldDwellingVisuals();
    RefreshHeadquartersVisual();
    SpawnHeroMarker();
    FAIHeroesArtDirection::ApplyWorld(*this);
    RefreshFogVisibility();
    RefreshAdventureRoads();

    // The generated map intentionally has no authored PlayerStart asset.
    APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC && !PC->GetPawn())
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        APawn* Pawn = GetWorld()->SpawnActor<AAIHeroesCameraPawn>(
            AAIHeroesCameraPawn::StaticClass(),
            FVector(-1050.f, 0.f, 1200.f),
            FRotator::ZeroRotator,
            Params);
        if (Pawn)
        {
            PC->Possess(Pawn);
        }
    }

    LastHint = TEXT("SEED AI: choose a node and press H. The hero has no threat analysis yet.");
    LastCombatStep = TEXT("No combat yet. The hero will choose targets during battles.");
    SetEvent(TEXT("Select a node, ASK HERO (H), then CONFIRM (Enter). Battles are AI-controlled."));
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(), TEXT("AIHeroesLocalExchangeSmoke"))) BeginLocalExchangeSmoke();
    else if (FParse::Param(FCommandLine::Get(), TEXT("AIHeroesModelRequestSmoke"))) BeginModelRequestSmoke();
    else if (FParse::Param(FCommandLine::Get(), TEXT("AIHeroesSupplySmoke"))) BeginSupplySmoke();
    else if (FParse::Param(FCommandLine::Get(), TEXT("AIHeroesTravelViewSmoke"))) BeginTravelViewSmoke();
    else if (FParse::Param(FCommandLine::Get(), TEXT("AIHeroesTravelSmoke"))) BeginTravelSmoke();
    else if (FParse::Param(FCommandLine::Get(), TEXT("AIHeroesLiveSmoke"))) BeginLiveBattleSmoke();
#endif
}

void AAIHeroesGameMode::SpawnWorld()
{
    UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh)
    {
        AStaticMeshActor* Ground = GetWorld()->SpawnActor<AStaticMeshActor>(
            FVector(0.f, 0.f, -85.f), FRotator::ZeroRotator);
        if (Ground)
        {
            UStaticMeshComponent* Mesh = Ground->GetStaticMeshComponent();
            Mesh->SetMobility(EComponentMobility::Movable);
            Mesh->SetStaticMesh(CubeMesh);
            Mesh->SetWorldScale3D(FVector(26.f, 26.f, 1.5f));
            // Keep the legacy fallback, but do not let it intersect the authored island.
            if (LoadObject<UStaticMesh>(nullptr, TEXT("/Game/AmberMarch/Meshes/SM_Island.SM_Island")))
                Mesh->SetVisibility(false);
            Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
        }
    }

    ADirectionalLight* Sun = GetWorld()->SpawnActor<ADirectionalLight>(
        FVector::ZeroVector, FRotator(-55.f, -35.f, 0.f));
    if (Sun)
    {
        Sun->GetLightComponent()->SetIntensity(7.f);
    }
    ASkyLight* Sky = GetWorld()->SpawnActor<ASkyLight>();
    if (Sky)
    {
        Sky->GetLightComponent()->SetIntensity(1.2f);
        Sky->GetLightComponent()->RecaptureSky();
    }

    SpawnNode(EAIHeroesNodeType::Headquarters, TEXT("HQ"), FVector(-650.f, 0.f, 155.f), {});
    SpawnNode(EAIHeroesNodeType::ForestMine, TEXT("FOREST"), FVector(-100.f, -650.f, 110.f),
        {FAIHeroesArmyStack(TEXT("Wolf"), 4, 9, 3, 1, 8)});
    SpawnNode(EAIHeroesNodeType::OreMine, TEXT("ORE"), FVector(150.f, 600.f, 120.f),
        {FAIHeroesArmyStack(TEXT("Bandit"), 6, 10, 4, 2, 4)});
    SpawnNode(EAIHeroesNodeType::Outpost, TEXT("OUTPOST"), FVector(700.f, -300.f, 140.f),
        {FAIHeroesArmyStack(TEXT("Raider"), 6, 11, 5, 2, 5),
         FAIHeroesArmyStack(TEXT("Raider"), 4, 11, 5, 2, 5)});
    SpawnNode(EAIHeroesNodeType::Ruins, TEXT("RUINS"), FVector(750.f, 650.f, 165.f),
        {FAIHeroesArmyStack(TEXT("Guardian"), 7, 24, 7, 4, 2)});
}

void AAIHeroesGameMode::SpawnHeroMarker()
{
    UStaticMesh* Sphere = LoadObject<UStaticMesh>(
        nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (!Sphere || !GetWorld())
    {
        return;
    }

    HeroMarker = GetWorld()->SpawnActor<AStaticMeshActor>(
        HeroStrategicLocation + FVector(0.f, 0.f, 290.f),
        FRotator::ZeroRotator);
    if (HeroMarker)
    {
        UStaticMeshComponent* Mesh = HeroMarker->GetStaticMeshComponent();
        Mesh->SetMobility(EComponentMobility::Movable);
        Mesh->SetStaticMesh(Sphere);
        Mesh->SetWorldScale3D(FVector(0.9f, 0.9f, 0.9f));
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
}

void AAIHeroesGameMode::MoveHeroTo(const FVector& Location)
{
    FinishStrategicTravelPresentation();
    InvalidateRoutePreview();
    HeroStrategicLocation = Location;
    if (HeroMarker)
    {
        HeroMarker->SetActorLocation(Location + FVector(0.f, 0.f, 290.f));
    }
}

AAIHeroesWorldNode* AAIHeroesGameMode::SpawnNode(
    EAIHeroesNodeType Type,
    const FString& Name,
    const FVector& At,
    const TArray<FAIHeroesArmyStack>& Guards)
{
    AAIHeroesWorldNode* Node = GetWorld()->SpawnActor<AAIHeroesWorldNode>(At, FRotator::ZeroRotator);
    if (Node)
    {
        Node->Configure(Type, Name, Guards);
        Nodes.Add(Node);
    }
    return Node;
}

void AAIHeroesGameMode::SetEvent(const FString& Event)
{
    LastEvent = Event;
    UE_LOG(LogTemp, Display, TEXT("[AI HEROES] %s"), *Event);
}

void AAIHeroesGameMode::AddStack(const FAIHeroesArmyStack& NewStack)
{
    InvalidateRoutePreview();
    for (FAIHeroesArmyStack& Existing : PlayerArmy)
    {
        if (Existing.UnitId == NewStack.UnitId)
        {
            Existing.TotalHealth += NewStack.TotalHealth;
            return;
        }
    }
    PlayerArmy.Add(NewStack);
}

bool AAIHeroesGameMode::HasLivingArmy() const
{
    for (const FAIHeroesArmyStack& Stack : PlayerArmy)
    {
        if (Stack.IsAlive())
        {
            return true;
        }
    }
    return false;
}

void AAIHeroesGameMode::RecruitMilitia()
{
    if (BlockStrategicAction()) return;
    const FAIHeroesRecruitOffer Offer =
        FAIHeroesRecruitmentAdvisor::GetOffer(EAIHeroesRecruitChoice::Militia);
    for (const FAIHeroesArmyStack& Stack : PlayerArmy)
    {
        if (Stack.UnitId == Offer.Units.UnitId &&
            Stack.TotalHealth > 500000 - Offer.Units.TotalHealth)
        {
            SetEvent(TEXT("Recruitment blocked: army stack limit reached. No resources spent."));
            return;
        }
    }
    FAIHeroesRecruitmentStock NextStock = RecruitmentStock;
    FString StockError;
    if (!FAIHeroesRecruitmentStockRules::Consume(
            NextStock, Offer.Choice, Offer.Units.Count(), StockError))
    {
        SetEvent(StockError);
        return;
    }
    if (!FAIHeroesTurnRules::SpendIfAffordable(Gold, Wood, Ore, Offer.Gold, Offer.Wood, Offer.Ore))
    {
        SetEvent(TEXT("Need 50 gold and 5 wood for 5 militia."));
        return;
    }
    RecruitmentStock = NextStock;
    AddStack(Offer.Units);
    SetEvent(FString::Printf(
        TEXT("Recruited 5 militia at HQ. Unpurchased militia: %d."),
        RecruitmentStock.Militia));
}

void AAIHeroesGameMode::RecruitScouts()
{
    if (BlockStrategicAction()) return;
    const FAIHeroesRecruitOffer Offer =
        FAIHeroesRecruitmentAdvisor::GetOffer(EAIHeroesRecruitChoice::Scouts);
    for (const FAIHeroesArmyStack& Stack : PlayerArmy)
    {
        if (Stack.UnitId == Offer.Units.UnitId &&
            Stack.TotalHealth > 500000 - Offer.Units.TotalHealth)
        {
            SetEvent(TEXT("Recruitment blocked: army stack limit reached. No resources spent."));
            return;
        }
    }
    FAIHeroesRecruitmentStock NextStock = RecruitmentStock;
    FString StockError;
    if (!FAIHeroesRecruitmentStockRules::Consume(
            NextStock, Offer.Choice, Offer.Units.Count(), StockError))
    {
        SetEvent(StockError);
        return;
    }
    if (!FAIHeroesTurnRules::SpendIfAffordable(Gold, Wood, Ore, Offer.Gold, Offer.Wood, Offer.Ore))
    {
        SetEvent(TEXT("Need 70 gold and 4 ore for 2 scouts."));
        return;
    }
    RecruitmentStock = NextStock;
    AddStack(Offer.Units);
    SetEvent(FString::Printf(
        TEXT("Recruited 2 scouts at HQ. Unpurchased scouts: %d."),
        RecruitmentStock.Scouts));
}

void AAIHeroesGameMode::UpgradeProtocol()
{
    if (BlockStrategicAction()) return;
    const auto Offer = FAIHeroesDevelopmentRules::Protocol(ProtocolTier);
    FString Error;
    if (!FAIHeroesDevelopmentRules::TryProtocol(ProtocolTier, Gold, Wood, Ore, Error))
    {
        SetEvent(Error);
        return;
    }
    InvalidateRoutePreview();
    SetEvent(FString::Printf(TEXT("Unlocked %s. Strategy policy improved (offline)."), *Offer.Name));
    AnnounceCampaignVictoryIfEarned();
}

void AAIHeroesGameMode::UpgradeModelSlot()
{
    if (BlockStrategicAction()) return;
    FString Error;
    if (!FAIHeroesDevelopmentRules::TryModelSlot(ModelSlotTier, Gold, DataFragments, Error))
    {
        SetEvent(Error);
        return;
    }
    InvalidateRoutePreview();
    SetEvent(FString::Printf(
        TEXT("MODEL SLOT %s unlocked. NO actual LLM checkpoint loaded; combat is still rule-based."),
        ModelSlotTier == 1 ? TEXT("3B") : TEXT("8B")));
    AnnounceCampaignVictoryIfEarned();
}

void AAIHeroesGameMode::EndTurn()
{
    if (BlockStrategicAction()) return;
    if (!FAIHeroesBaseRules::IsValidTier(BaseTier) || Day < 1 ||
        Day >= FAIHeroesBaseRules::MaxDay || Gold < 0 || Wood < 0 || Ore < 0)
    {
        SetEvent(TEXT("DAY BLOCKED: invalid progress or campaign day limit. Nothing changed."));
        return;
    }
    int64 NextGold = int64(Gold) + FAIHeroesBaseRules::DailyGold(BaseTier);
    int64 NextWood = Wood, NextOre = Ore;
    for (const AAIHeroesWorldNode* Node : Nodes)
    {
        if (!IsValid(Node))
        {
            SetEvent(TEXT("DAY BLOCKED: missing map node. Nothing changed."));
            return;
        }
        if (!Node->bOwned) continue;
        if (Node->NodeType == EAIHeroesNodeType::ForestMine) NextWood += 15;
        else if (Node->NodeType == EAIHeroesNodeType::OreMine) NextOre += 12;
        else if (Node->NodeType == EAIHeroesNodeType::Outpost) NextGold += 30;
    }
    if (NextGold > FAIHeroesBaseRules::MaxResource ||
        NextWood > FAIHeroesBaseRules::MaxResource || NextOre > FAIHeroesBaseRules::MaxResource)
    {
        SetEvent(TEXT("DAY BLOCKED: resource limit reached. Nothing changed."));
        return;
    }
    // Prepare the entire next day before touching the authoritative campaign.
    // This preserves the existing save schema: defender HP already lives in
    // NodeStates. A malformed map or overflowing reinforcement blocks the
    // ENTIRE day, including income, rather than partially advancing it.
    TArray<FAIHeroesWeekNode> WorldBefore;
    WorldBefore.Reserve(Nodes.Num());
    for (const AAIHeroesWorldNode* Node : Nodes)
    {
        FAIHeroesWeekNode Snapshot;
        Snapshot.Id = FName(*Node->GetDisplayName());
        Snapshot.Type = Node->NodeType;
        Snapshot.bOwned = Node->bOwned;
        Snapshot.Guards = Node->Defenders;
        WorldBefore.Add(MoveTemp(Snapshot));
    }

    FAIHeroesWeekPlan WeekPlan;
    FString WeekError;
    if (!FAIHeroesWorldWeekRules::PlanNextDay(
            Day, WorldBefore, WeekPlan, WeekError))
    {
        SetEvent(TEXT("DAY BLOCKED: ") + WeekError + TEXT(" No changes applied."));
        return;
    }

    // Preflight the original rival's action against the next-day world.
    // The world-week rules already applied neutral growth to the proposal;
    // do not replace or lose it by reading the previous OUTPOST army.
    FAIHeroesRivalSnapshot RivalBefore;
    FString RivalError;
    if (!BuildRivalSnapshot(Day + 1,
        static_cast<int32>(NextGold), static_cast<int32>(NextWood),
        static_cast<int32>(NextOre), RivalBefore, RivalError))
    {
        SetEvent(TEXT("DAY BLOCKED: ") + RivalError + TEXT(" Nothing changed."));
        return;
    }
    for (const FAIHeroesWeekNode& Proposed : WeekPlan.Nodes)
    {
        if (Proposed.Id == FName(TEXT("OUTPOST")))
        {
            RivalBefore.OutpostDefenders = Proposed.Guards;
        }
    }
    FAIHeroesRivalSnapshot RivalAfter;
    FAIHeroesRivalTurn RivalTurn;
    if (!FAIHeroesRivalStrategy::Resolve(
            RivalBefore, RivalAfter, RivalTurn, RivalError))
    {
        SetEvent(TEXT("DAY BLOCKED: ") + RivalError + TEXT(" Nothing changed."));
        return;
    }

    // An owned outpost increases Scouts' *recruitable stock*; an unowned
    // outpost instead reinforces its neutral guards in WorldWeekRules.
    // Preflight the production snapshot before any day/income commits.
    FAIHeroesRecruitmentStock NextStock;
    FString ProductionError;
    if (!FAIHeroesRecruitmentStockRules::PlanNextDay(
            Day, BaseTier, RivalBefore.bOutpostOwned,
            RecruitmentStock, NextStock, ProductionError))
    {
        SetEvent(TEXT("DAY BLOCKED: ") + ProductionError +
            TEXT(" No income or growth applied."));
        return;
    }
    // Field dwellings are independent from both HQ stock and neutral guards.
    // A site accrues only when the player explicitly claimed it after visiting.
    bool bForestOwned = false, bOreOwned = false;
    for (const auto& Site : WeekPlan.Nodes)
    {
        if (Site.Id == FName(TEXT("FOREST"))) bForestOwned = Site.bOwned;
        if (Site.Id == FName(TEXT("ORE"))) bOreOwned = Site.bOwned;
    }
    FAIHeroesFieldDwellings NextField;
    FString FieldError;
    if (!FAIHeroesFieldDwellingRules::PlanNextDay(
            Day, bForestOwned, bOreOwned, FieldDwellings, NextField, FieldError))
    {
        SetEvent(TEXT("DAY BLOCKED: ") + FieldError + TEXT(" No changes applied."));
        return;
    }
    const int32 FieldForestGrowth = NextField.Forest.Available - FieldDwellings.Forest.Available;
    const int32 FieldOreGrowth = NextField.Ore.Available - FieldDwellings.Ore.Available;
    const int32 AddedMilitia = NextStock.Militia - RecruitmentStock.Militia;
    const int32 AddedScouts = NextStock.Scouts - RecruitmentStock.Scouts;

    const int32 Income = static_cast<int32>(NextGold) - Gold;
    InvalidateRoutePreview();
    ++Day;
    MovesLeft = 4;
    Gold = RivalAfter.Gold;
    Wood = RivalAfter.Wood;
    Ore = RivalAfter.Ore;
    RecruitmentStock = NextStock;
    FieldDwellings = NextField;
    RefreshFieldDwellingVisuals();
    if (WeekPlan.bNewWeek)
    {
        // Commit only after the full resource and world-transition checks.
        for (int32 Index = 0; Index < Nodes.Num(); ++Index)
        {
            if (!Nodes[Index]->bOwned)
            {
                Nodes[Index]->Defenders = MoveTemp(WeekPlan.Nodes[Index].Guards);
            }
        }
    }
    if (RivalTurn.Action == EAIHeroesRivalAction::RecruitAtOutpost)
    {
        for (AAIHeroesWorldNode* Node : Nodes)
        {
            if (Node->NodeType == EAIHeroesNodeType::Outpost &&
                Node->GetDisplayName() == TEXT("OUTPOST"))
            {
                Node->Defenders = RivalAfter.OutpostDefenders;
                Node->SetOwned(false); // Preserve the neutral-growth result as well.
                break;
            }
        }
    }
    FString DayReport = FString::Printf(
        TEXT("Day %d: income collected (+%d gold); movement restored to %d. [W%d D%d/7]"),
        Day, Income, MovesLeft, WeekPlan.Week, WeekPlan.DayInWeek);
    if (WeekPlan.bNewWeek)
    {
        DayReport += FString::Printf(
            TEXT(" NEW WEEK: neutral guards +%d; own reserves +%d militia +%d scouts (now %d/%d)."),
            WeekPlan.NewNeutralUnits, AddedMilitia, AddedScouts,
            RecruitmentStock.Militia, RecruitmentStock.Scouts);
        UE_LOG(LogTemp, Display,
            TEXT("[AI HEROES WEEK] week=%d day=%d neutral=%d militia_stock_plus=%d scout_stock_plus=%d stock=%d/%d"),
            WeekPlan.Week, Day, WeekPlan.NewNeutralUnits,
            AddedMilitia, AddedScouts, RecruitmentStock.Militia, RecruitmentStock.Scouts);
        DayReport += FString::Printf(
            TEXT(" FIELD: FOREST %+d (%d), ORE %+d (%d)."),
            FieldForestGrowth, FieldDwellings.Forest.Available,
            FieldOreGrowth, FieldDwellings.Ore.Available);
        UE_LOG(LogTemp, Display,
            TEXT("[AI HEROES FIELD] WEEK day=%d forest_plus=%d forest_stock=%d forest_claimed=%d ore_plus=%d ore_stock=%d ore_claimed=%d"),
            Day, FieldForestGrowth, FieldDwellings.Forest.Available,
            FieldDwellings.Forest.bClaimed, FieldOreGrowth,
            FieldDwellings.Ore.Available, FieldDwellings.Ore.bClaimed);
    }
    if (RivalTurn.IsActive())
    {
        DayReport += IsNodeDiscovered(FName(TEXT("OUTPOST")))
            ? TEXT(" ") + RivalTurn.Description
            : TEXT(" SCOUTING: distant hostile activity; location and strength unknown.");
        UE_LOG(LogTemp, Display,
            TEXT("[AI HEROES RIVAL] day=%d action=%d amount=%d wood=%d ore=%d player_order=0"),
            Day, static_cast<int32>(RivalTurn.Action), RivalTurn.Amount, Wood, Ore);
    }
    SetEvent(DayReport);
}

int32 AAIHeroesGameMode::GetSelectedMoveCost() const
{
    const auto Path = GetSelectedTravelPath();
    return Path.Status == EAIHeroesAdventurePathStatus::Ready ||
        Path.Status == EAIHeroesAdventurePathStatus::NeedsMoreMoves ? Path.TotalCost : INDEX_NONE;
}

FString AAIHeroesGameMode::GetSelectionSummary() const
{
    if (IsStrategicTravelMoving())
        return FString::Printf(TEXT("TRAVEL ANIMATION | move applied once | %d moves left | next order skips animation"), MovesLeft);
    if (!IsValid(SelectedNode.Get()))
    {
        return TEXT("TARGET: none | Click or tap a node");
    }
    const FString Travel = FAIHeroesTravelRules::Describe(GetSelectedTravelPath());
    if (SelectedNode->NodeType == EAIHeroesNodeType::Headquarters)
        return GetHeadquartersSummary() + TEXT(" | ") + Travel;
    int32 Guards = 0;
    for (const FAIHeroesArmyStack& Stack : SelectedNode->Defenders)
    {
        Guards += Stack.Count();
    }
    return FString::Printf(TEXT("TARGET: %s | %s | guards %d | %s"),
        *SelectedNode->GetDisplayName(),
        SelectedNode->bOwned ? TEXT("OURS") : TEXT("UNCLAIMED"),
        Guards, *Travel) +
        (SelectedNode->NodeType == EAIHeroesNodeType::ForestMine ||
         SelectedNode->NodeType == EAIHeroesNodeType::OreMine
            ? TEXT(" | ") + GetFieldDwellingLabel(SelectedNode->NodeType)
            : FString());
}

void AAIHeroesGameMode::SelectNode(AAIHeroesWorldNode* Node)
{
    if (BlockStrategicAction()) return;
    if (!IsValid(Node) || !Nodes.Contains(Node))
    {
        return;
    }
    if (!IsNodeDiscovered(FName(*Node->GetDisplayName())))
    {
        SetEvent(TEXT("SCOUTING: destination unknown. Visit a visible site to explore."));
        return;
    }
    SelectedNode = Node;
    SetEvent(FString::Printf(TEXT("%s selected. Ask H for advice; Enter to move/attack."),
        *Node->GetDisplayName()));
    LastHint = FAIHeroesTravelRules::Describe(GetSelectedTravelPath());
}

void AAIHeroesGameMode::ConfirmSelectedNode()
{
    if (BlockStrategicAction()) return;
    if (!IsValid(SelectedNode.Get()))
    {
        SetEvent(TEXT("Select a node before confirming your move."));
        return;
    }
    InteractWithNode(SelectedNode.Get());
}

void AAIHeroesGameMode::InteractWithNode(AAIHeroesWorldNode* Node)
{
    if (BlockStrategicAction()) return;
    if (!IsValid(Node) || !Nodes.Contains(Node))
    {
        SetEvent(TEXT("Invalid destination. No resources or movement spent."));
        return;
    }
    if (!IsNodeDiscovered(FName(*Node->GetDisplayName())))
    {
        SetEvent(TEXT("SCOUTING BLOCKED: unknown site. No movement or battle issued."));
        return;
    }

    bool bHostile = false;
    if (!Node->bOwned)
    {
        for (const FAIHeroesArmyStack& Defender : Node->Defenders)
        {
            bHostile |= Defender.IsAlive();
        }
    }
    // Recompute from current authoritative state; never execute a cached advisory path.
    const auto Path = FAIHeroesTravelRules::Find(BuildAdvisorSnapshot(),
        HeroStrategicLocation, FName(*Node->GetDisplayName()), MovesLeft);
    if (!Path.IsReachableThisTurn())
    {
        SetEvent(FAIHeroesTravelRules::Describe(Path));
        return;
    }
    const int32 Cost = Path.TotalCost;
    const FVector TravelOrigin = HeroStrategicLocation;
    UE_LOG(LogTemp, Display, TEXT("[AI HEROES TRAVEL] v%d %s"),
        TravelRuleVersion, *FAIHeroesTravelRules::Describe(Path));

    if (Node->bOwned || Node->NodeType == EAIHeroesNodeType::Headquarters)
    {
        MovesLeft -= Cost;
        MoveHeroTo(Node->GetActorLocation());
        PresentStrategicTravel(Path, TravelOrigin);
        RevealAfterArrival(FName(*Node->GetDisplayName()));
        SetEvent(FString::Printf(TEXT("Hero moved to %s. Remaining moves: %d."),
            *Node->GetDisplayName(), MovesLeft));
        return;
    }

    if (!bHostile)
    {
        MovesLeft -= Cost;
        Node->Defenders.Empty();
        Node->SetOwned(true);
        MoveHeroTo(Node->GetActorLocation());
        PresentStrategicTravel(Path, TravelOrigin);
        RevealAfterArrival(FName(*Node->GetDisplayName()));
        SetEvent(FString::Printf(TEXT("%s secured without a fight. Income begins next turn."),
            *Node->GetDisplayName()));
        AnnounceCampaignVictoryIfEarned();
        return;
    }

    if (!HasLivingArmy())
    {
        SetEvent(TEXT("No living army. Recruit units before confirming an attack."));
        return;
    }

    // No future outcome exists: the live session chooses one action at a time.
    StartLiveBattle(Node, Cost);
}

void AAIHeroesGameMode::SaveCampaign()
{
    if (BlockStrategicAction()) return;
    UAIHeroesCampaignSave* Snapshot = Cast<UAIHeroesCampaignSave>(
        UGameplayStatics::CreateSaveGameObject(UAIHeroesCampaignSave::StaticClass()));
    if (!Snapshot)
    {
        SetEvent(TEXT("SAVE FAILED: unable to allocate campaign snapshot."));
        return;
    }

    Snapshot->Day = Day;
    Snapshot->Gold = Gold;
    Snapshot->Wood = Wood;
    Snapshot->Ore = Ore;
    Snapshot->BaseTier = BaseTier;
    Snapshot->MovesLeft = MovesLeft;
    Snapshot->TravelRuleVersion = TravelRuleVersion;
    Snapshot->ProtocolTier = ProtocolTier;
    Snapshot->ModelSlotTier = ModelSlotTier;
    Snapshot->DataFragments = DataFragments;
    Snapshot->bRecruitmentStockInitialized = true;
    Snapshot->MilitiaReserve = RecruitmentStock.Militia;
    Snapshot->ScoutReserve = RecruitmentStock.Scouts;
    Snapshot->bFieldDwellingsInitialized = true;
    Snapshot->bForestCampClaimed = FieldDwellings.Forest.bClaimed;
    Snapshot->ForestCampUnits = FieldDwellings.Forest.Available;
    Snapshot->bOreAerieClaimed = FieldDwellings.Ore.bClaimed;
    Snapshot->OreAerieUnits = FieldDwellings.Ore.Available;
    Snapshot->bFogInitialized = bFogEnabled;
    Snapshot->ExploredSitesMask = bFogEnabled ? FogKnownMask : 0;
    Snapshot->PlayerArmy = PlayerArmy;
    Snapshot->BattleJournal = BattleJournal;
    Snapshot->LastCombatStep = LastCombatStep;
    Snapshot->LastBattleRecord = LastBattleRecord;

    for (const AAIHeroesWorldNode* Node : Nodes)
    {
        if (!IsValid(Node))
        {
            SetEvent(TEXT("SAVE FAILED: a map node is missing."));
            return;
        }

        FAIHeroesSavedNode State;
        State.NodeId = FName(*Node->GetDisplayName());
        State.Type = Node->NodeType;
        State.bOwned = Node->bOwned;
        State.Defenders = Node->Defenders;
        Snapshot->NodeStates.Add(MoveTemp(State));

        if (FVector::DistSquared(Node->GetActorLocation(), HeroStrategicLocation) < 1.0)
        {
            Snapshot->HeroNodeId = FName(*Node->GetDisplayName());
        }
        if (SelectedNode.Get() == Node)
        {
            Snapshot->SelectedNodeId = FName(*Node->GetDisplayName());
        }
    }

    FString ValidationError;
    if (!FAIHeroesCampaignRules::Validate(*Snapshot, ValidationError))
    {
        SetEvent(FString::Printf(TEXT("SAVE BLOCKED: %s"), *ValidationError));
        return;
    }
    FString SaveMessage;
    if (!FAIHeroesSaveSystem::Save(*Snapshot,
        SaveNamespaceForPlayer(ActiveSaveSlot),
        LegacySlotForPlayer(ActiveSaveSlot), SaveMessage))
    {
        SetEvent(SaveMessage);
        return;
    }

    SetEvent(FString::Printf(TEXT("SLOT %d: %s"), ActiveSaveSlot, *SaveMessage));
}

void AAIHeroesGameMode::LoadCampaign()
{
    if (BlockStrategicAction()) return;
    // Session-only reward evidence must never outlive a load attempt.
    LiveBattleCommitReceipt = FAIHeroesBattleCommitReceipt();
    LiveBattleAftermathView = FAIHeroesBattleAftermathView();
    CancelLocalModelExchange();
    LocalModelReplies.Reset(); // Pending inference cannot cross a load attempt.
    PendingSupplyFingerprint.Empty(); // Quote authorization never survives load.
    const FAIHeroesLoadedCampaign Loaded = FAIHeroesSaveSystem::Load(
        SaveNamespaceForPlayer(ActiveSaveSlot),
        LegacySlotForPlayer(ActiveSaveSlot));
    UAIHeroesCampaignSave* Snapshot = Loaded.Snapshot;
    if (!Snapshot)
    {
        SetEvent(TEXT("LOAD BLOCKED: ") + Loaded.Message);
        return;
    }

    FString ValidationError;
    if (!FAIHeroesCampaignRules::Validate(*Snapshot, ValidationError))
    {
        SetEvent(FString::Printf(TEXT("LOAD BLOCKED: %s State unchanged."), *ValidationError));
        return;
    }

    // Validate actors, unique identities and exact map coverage before commit.
    TArray<FAIHeroesActiveNodeIdentity> ActiveNodeIdentities;
    ActiveNodeIdentities.Reserve(Nodes.Num());
    for (AAIHeroesWorldNode* Node : Nodes)
    {
        if (!IsValid(Node))
        {
            SetEvent(TEXT("LOAD BLOCKED: active map contains a missing node."));
            return;
        }
        ActiveNodeIdentities.Add({FName(*Node->GetDisplayName()), Node->NodeType});
    }
    if (!FAIHeroesLoadPreflight::Validate(ActiveNodeIdentities, Snapshot->NodeStates,
            Snapshot->HeroNodeId, Snapshot->SelectedNodeId, ValidationError))
    {
        SetEvent(TEXT("LOAD BLOCKED: ") + ValidationError + TEXT(" State unchanged."));
        return;
    }

    TMap<FName, const FAIHeroesSavedNode*> SavedById;
    for (const FAIHeroesSavedNode& State : Snapshot->NodeStates)
    {
        SavedById.Add(State.NodeId, &State);
    }

    AAIHeroesWorldNode* HeroDestination = nullptr;
    AAIHeroesWorldNode* NewSelection = nullptr;
    for (AAIHeroesWorldNode* Node : Nodes)
    {
        if (!IsValid(Node))
        {
            SetEvent(TEXT("LOAD BLOCKED: active map contains a missing node."));
            return;
        }

        const FName Id(*Node->GetDisplayName());
        const FAIHeroesSavedNode* const* Saved = SavedById.Find(Id);
        if (!Saved || !(*Saved) || (*Saved)->Type != Node->NodeType)
        {
            SetEvent(TEXT("LOAD BLOCKED: node identifiers do not match this map."));
            return;
        }

        if (Id == Snapshot->HeroNodeId)
        {
            HeroDestination = Node;
        }
        if (Id == Snapshot->SelectedNodeId)
        {
            NewSelection = Node;
        }
    }

    if (!HeroDestination || !SavedById.Contains(Snapshot->HeroNodeId))
    {
        SetEvent(TEXT("LOAD BLOCKED: hero position cannot be restored."));
        return;
    }

    // Commit validated state atomically, after successful read and preflight.
    InvalidateRoutePreview();
    Day = Snapshot->Day;
    Gold = Snapshot->Gold;
    Wood = Snapshot->Wood;
    Ore = Snapshot->Ore;
    BaseTier = Snapshot->BaseTier;
    MovesLeft = Snapshot->MovesLeft;
    TravelRuleVersion = Snapshot->TravelRuleVersion;
    bFogEnabled = Snapshot->bFogInitialized;
    FogKnownMask = bFogEnabled ? Snapshot->ExploredSitesMask : 0;
    ProtocolTier = Snapshot->ProtocolTier;
    ModelSlotTier = Snapshot->ModelSlotTier;
    DataFragments = Snapshot->DataFragments;
    // Old v1 saves do not contain optional stock fields. Do not invent
    // retroactive past-week growth: give one starter reserve in memory.
    RecruitmentStock = Snapshot->bRecruitmentStockInitialized
        ? FAIHeroesRecruitmentStock{
            Snapshot->MilitiaReserve, Snapshot->ScoutReserve}
        : FAIHeroesRecruitmentStockRules::Initial();
    FieldDwellings = Snapshot->bFieldDwellingsInitialized
        ? FAIHeroesFieldDwellings{
            {Snapshot->bForestCampClaimed, Snapshot->ForestCampUnits},
            {Snapshot->bOreAerieClaimed, Snapshot->OreAerieUnits}}
        : FAIHeroesFieldDwellingRules::Initial();
    PlayerArmy = Snapshot->PlayerArmy;
    BattleJournal = Snapshot->BattleJournal;
    LastCombatStep = Snapshot->LastCombatStep;
    LastBattleRecord = Snapshot->LastBattleRecord;
    ReplayCursor = INDEX_NONE;

    for (AAIHeroesWorldNode* Node : Nodes)
    {
        const FAIHeroesSavedNode* Saved =
            SavedById.FindChecked(FName(*Node->GetDisplayName()));
        Node->Defenders = Saved->Defenders;
        Node->SetOwned(Saved->bOwned);
    }

    RefreshHeadquartersVisual();
    RefreshFieldDwellingVisuals();
    SelectedNode = NewSelection;
    MoveHeroTo(HeroDestination->GetActorLocation());
    RefreshFogVisibility();
    RefreshAdventureRoads();
    LastHint = TEXT("Loaded local campaign; ask H for a fresh tactical assessment.");
    SetEvent(FString::Printf(TEXT("GAME LOADED SLOT %d: day %d, %d nodes, protocol tier %d. %s"),
        ActiveSaveSlot, Day, Snapshot->NodeStates.Num(), ProtocolTier, *Loaded.Message));
    AnnounceCampaignVictoryIfEarned();
}

void AAIHeroesGameMode::CycleSaveSlot()
{
    if (BlockStrategicAction()) return;
    ActiveSaveSlot = (ActiveSaveSlot % 3) + 1;
    SetEvent(FString::Printf(TEXT("SAVE SLOT %d/3 selected. K writes, L loads; other slots are untouched."),
        ActiveSaveSlot));
}

void AAIHeroesGameMode::AdvanceBattleReplay()
{
    if (BlockStrategicAction()) return;
    FString Error;
    if (LastBattleRecord.NodeId.IsNone())
    {
        SetEvent(TEXT("REPLAY: no recorded battle yet."));
        return;
    }
    if (!FAIHeroesBattleRecordRules::Verify(LastBattleRecord, Error))
    {
        SetEvent(TEXT("REPLAY BLOCKED: tactical record failed verification."));
        return;
    }
    if (LastBattleRecord.Trace.IsEmpty())
    {
        SetEvent(TEXT("REPLAY: this battle has no recorded actions."));
        return;
    }

    ReplayCursor = (ReplayCursor + 1) % LastBattleRecord.Trace.Num();
    SetEvent(FAIHeroesBattleRecordRules::DescribeStep(LastBattleRecord, ReplayCursor));
}

FString AAIHeroesGameMode::GetReplayDisplayText() const
{
    // BattleRecord deliberately has no persisted award fields. This is an
    // absence-of-evidence label, never a current-rules reward calculation.
    const FString HistoryNote=LastBattleRecord.NodeId.IsNone()
        ? FString():TEXT("ИСТОРИЯ: награда не записана. ");
    if (LastBattleRecord.Trace.IsValidIndex(ReplayCursor))
    {
        return HistoryNote+FAIHeroesBattleRecordRules::DescribeStep(LastBattleRecord, ReplayCursor);
    }
    return HistoryNote+TEXT("LAST TACTIC: ")+LastCombatStep;
}

FAIHeroesAdvisorSnapshot AAIHeroesGameMode::BuildAdvisorSnapshot() const
{
    // This is the ONLY data passed into the offline strategic adviser.
    // No actor pointer, SaveGame, file path or mutable world authority leaks.
    FAIHeroesAdvisorSnapshot Request;
    Request.ProtocolTier = ProtocolTier;
    Request.TravelRuleVersion = TravelRuleVersion;
    Request.bFogEnabled = bFogEnabled;
    Request.ExploredSitesMask = FogKnownMask;
    Request.MovesLeft = MovesLeft;
    Request.Gold = Gold;
    Request.Wood = Wood;
    Request.Ore = Ore;
    Request.MilitiaReserve = RecruitmentStock.Militia;
    Request.ScoutReserve = RecruitmentStock.Scouts;
    Request.HeroPosition = HeroStrategicLocation;
    Request.PlayerArmy = PlayerArmy;
    Request.SelectedNodeId = IsValid(SelectedNode.Get())
        ? FName(*SelectedNode->GetDisplayName()) : NAME_None;
    if (!BattleJournal.IsEmpty())
    {
        Request.LastBattle = BattleJournal.Last();
    }

    for (const AAIHeroesWorldNode* Node : Nodes)
    {
        if (!IsValid(Node))
        {
            continue;
        }
        FAIHeroesAdvisorNode Snapshot;
        Snapshot.Id = FName(*Node->GetDisplayName());
        Snapshot.Type = Node->NodeType;
        Snapshot.Location = Node->GetActorLocation();
        Snapshot.bOwned = Node->bOwned;
        // The advice snapshot cannot inspect an unrevealed garrison.
        // Topology stays typed for validation, but its hostile strength
        // is not made available to the player's AI policy.
        if (IsNodeDiscovered(Snapshot.Id))
        {
            Snapshot.Guards = Node->Defenders;
        }
        Request.Nodes.Add(MoveTemp(Snapshot));
    }
    return Request;
}

void AAIHeroesGameMode::AskAdvisor()
{
    if (BlockStrategicAction()) return;
    LastHint = IsValid(SelectedNode.Get()) &&
        SelectedNode->NodeType == EAIHeroesNodeType::Headquarters
        ? GetHeadquartersSummary() : FAIHeroesAdvisor::Advise(BuildAdvisorSnapshot());
    if (IsValid(SelectedNode.Get()))
        LastHint += TEXT(" | ") + FAIHeroesTravelRules::Describe(GetSelectedTravelPath());
    RecordAdvisorDossier(TEXT("HERO STRATEGY REPORT"), {LastHint}, false);
}

void AAIHeroesGameMode::InvalidateRoutePreview()
{
    RoutePreview = FAIHeroesRoutePlan();
    RouteAlternatives.Reset();
    ActiveRouteIndex = 0;
}

void AAIHeroesGameMode::PlanRoute()
{
    if (BlockStrategicAction()) return;
    if (IsBattleReplayActive())
    {
        SetEvent(TEXT("ROUTE: finish 3D combat replay before planning."));
        return;
    }

    if (ProtocolTier < 3)
    {
        InvalidateRoutePreview();
        LastHint = TEXT("ROUTE LOCKED: unlock TACTICAL PLANNER (P, protocol tier 3).");
        SetEvent(LastHint);
        return;
    }

    // Z cycles alternatives without repeating grid combat simulations.
    // Every meaningful campaign mutation calls InvalidateRoutePreview().
    if (RouteAlternatives.IsEmpty())
    {
        RouteAlternatives = FAIHeroesRoutePlanner::Rank(BuildAdvisorSnapshot());
        ActiveRouteIndex = 0;
    }
    else
    {
        ActiveRouteIndex = (ActiveRouteIndex + 1) % RouteAlternatives.Num();
    }
    if (RouteAlternatives.IsEmpty())
    {
        InvalidateRoutePreview();
        LastHint = TEXT("ROUTE: no safe reachable capture this turn. Recruit, reposition or end day.");
        SetEvent(LastHint);
        return;
    }

    RoutePreview = RouteAlternatives[ActiveRouteIndex];
    LastHint = FString::Printf(
        TEXT("PLAN %d/%d (heuristic %d): %s. Confirm each step manually."),
        ActiveRouteIndex + 1, RouteAlternatives.Num(),
        RoutePreview.Score, *RoutePreview.Describe());
    UE_LOG(LogTemp, Display, TEXT("[AI HEROES ROUTE] %s"), *LastHint);
    SetEvent(TEXT("ROUTE PREVIEW: press Z for alternatives; click a node, Enter to confirm."));
}

FString AAIHeroesGameMode::GetArmySummary() const
{
    FString Out;
    for (const FAIHeroesArmyStack& Stack : PlayerArmy)
    {
        if (Stack.IsAlive())
        {
            if (!Out.IsEmpty())
            {
                Out += TEXT("  ");
            }
            Out += FString::Printf(TEXT("%s:%d"), *Stack.UnitId.ToString(), Stack.Count());
        }
    }
    return Out.IsEmpty() ? TEXT("NO UNITS") : Out;
}

FString AAIHeroesGameMode::GetCognitionSummary() const
{
    static const TCHAR* ProtocolNames[4] = {
        TEXT("SEED"), TEXT("ANALYST"), TEXT("ARCHIVIST"), TEXT("STRATEGIST")
    };
    static const TCHAR* SlotNames[3] = { TEXT("1B"), TEXT("3B"), TEXT("8B") };
    return FString::Printf(TEXT("%s | MODEL SLOT %s (NO CHECKPOINT)"),
        ProtocolNames[FMath::Clamp(ProtocolTier, 0, 3)],
        SlotNames[FMath::Clamp(ModelSlotTier, 0, 2)]);
}
