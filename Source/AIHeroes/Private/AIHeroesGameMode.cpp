#include "AIHeroesGameMode.h"

#include "AIHeroesAdvisor.h"
#include "AIHeroesBaseRules.h"
#include "AIHeroesDevelopmentRules.h"
#include "AIHeroesRecruitmentAdvisor.h"
#include "AIHeroesBattleEngine.h"
#include "AIHeroesBattleRecord.h"
#include "AIHeroesCampaignRules.h"
#include "AIHeroesCampaignSave.h"
#include "AIHeroesSaveSystem.h"
#include "AIHeroesCameraPawn.h"
#include "AIHeroesHUD.h"
#include "AIHeroesPlayerController.h"
#include "AIHeroesTurnRules.h"
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

    AddStack(FAIHeroesArmyStack(TEXT("Militia"), 8, 12, 4, 2, 3));
    AddStack(FAIHeroesArmyStack(TEXT("Scout"), 3, 8, 6, 1, 7));
    SpawnWorld();
    RefreshHeadquartersVisual();
    SpawnHeroMarker();

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
    if (FParse::Param(FCommandLine::Get(), TEXT("AIHeroesLiveSmoke"))) BeginLiveBattleSmoke();
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
    if (!FAIHeroesTurnRules::SpendIfAffordable(Gold, Wood, Ore, Offer.Gold, Offer.Wood, Offer.Ore))
    {
        SetEvent(TEXT("Need 50 gold and 5 wood for 5 militia."));
        return;
    }
    AddStack(Offer.Units);
    SetEvent(TEXT("Recruited 5 militia at HQ."));
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
    if (!FAIHeroesTurnRules::SpendIfAffordable(Gold, Wood, Ore, Offer.Gold, Offer.Wood, Offer.Ore))
    {
        SetEvent(TEXT("Need 70 gold and 4 ore for 2 scouts."));
        return;
    }
    AddStack(Offer.Units);
    SetEvent(TEXT("Recruited 2 scouts at HQ."));
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

    const int32 Income = static_cast<int32>(NextGold) - Gold;
    InvalidateRoutePreview();
    ++Day;
    MovesLeft = 4;
    Gold = RivalAfter.Gold;
    Wood = RivalAfter.Wood;
    Ore = RivalAfter.Ore;
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
            TEXT(" NEW WEEK: neutral guards +%d units. Capture sites early."),
            WeekPlan.NewNeutralUnits);
        UE_LOG(LogTemp, Display, TEXT("[AI HEROES WEEK] week=%d day=%d recruits=%d"),
            WeekPlan.Week, Day, WeekPlan.NewNeutralUnits);
    }
    if (RivalTurn.IsActive())
    {
        DayReport += TEXT(" ") + RivalTurn.Description;
        UE_LOG(LogTemp, Display,
            TEXT("[AI HEROES RIVAL] day=%d action=%d amount=%d wood=%d ore=%d player_order=0"),
            Day, static_cast<int32>(RivalTurn.Action), RivalTurn.Amount, Wood, Ore);
    }
    SetEvent(DayReport);
}

int32 AAIHeroesGameMode::GetSelectedMoveCost() const
{
    if (!IsValid(SelectedNode.Get()))
    {
        return INDEX_NONE;
    }
    bool bHostile = false;
    if (!SelectedNode->bOwned)
    {
        for (const FAIHeroesArmyStack& Defender : SelectedNode->Defenders)
        {
            bHostile |= Defender.IsAlive();
        }
    }
    return FAIHeroesTurnRules::TravelCost(
        HeroStrategicLocation, SelectedNode->GetActorLocation(), bHostile);
}

FString AAIHeroesGameMode::GetSelectionSummary() const
{
    if (!IsValid(SelectedNode.Get()))
    {
        return TEXT("TARGET: none | Click or tap a node");
    }
    if (SelectedNode->NodeType == EAIHeroesNodeType::Headquarters)
        return GetHeadquartersSummary();
    int32 Guards = 0;
    for (const FAIHeroesArmyStack& Stack : SelectedNode->Defenders)
    {
        Guards += Stack.Count();
    }
    return FString::Printf(TEXT("TARGET: %s | %s | guards %d | move cost %d"),
        *SelectedNode->GetDisplayName(),
        SelectedNode->bOwned ? TEXT("OURS") : TEXT("UNCLAIMED"),
        Guards, GetSelectedMoveCost());
}

void AAIHeroesGameMode::SelectNode(AAIHeroesWorldNode* Node)
{
    if (BlockStrategicAction()) return;
    if (!IsValid(Node))
    {
        return;
    }
    SelectedNode = Node;
    SetEvent(FString::Printf(TEXT("%s selected. Ask H for advice; Enter to move/attack."),
        *Node->GetDisplayName()));
    LastHint = TEXT("Selection is free. Player controls whether the hero attacks.");
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
    if (!IsValid(Node))
    {
        SetEvent(TEXT("Invalid destination. No resources or movement spent."));
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
    const int32 Cost = FAIHeroesTurnRules::TravelCost(
        HeroStrategicLocation, Node->GetActorLocation(), bHostile);
    if (!FAIHeroesTurnRules::CanMove(MovesLeft, Cost))
    {
        SetEvent(FString::Printf(TEXT("Need %d moves, have %d. End turn or choose a nearer node."),
            Cost, MovesLeft));
        return;
    }

    if (Node->bOwned || Node->NodeType == EAIHeroesNodeType::Headquarters)
    {
        MovesLeft -= Cost;
        MoveHeroTo(Node->GetActorLocation());
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
    Snapshot->ProtocolTier = ProtocolTier;
    Snapshot->ModelSlotTier = ModelSlotTier;
    Snapshot->DataFragments = DataFragments;
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

    // Preflight ALL actor references before changing a single game variable.
    if (Nodes.Num() != Snapshot->NodeStates.Num())
    {
        SetEvent(TEXT("LOAD BLOCKED: active map layout differs from the save."));
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
    ProtocolTier = Snapshot->ProtocolTier;
    ModelSlotTier = Snapshot->ModelSlotTier;
    DataFragments = Snapshot->DataFragments;
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
    SelectedNode = NewSelection;
    MoveHeroTo(HeroDestination->GetActorLocation());
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
    if (LastBattleRecord.Trace.IsValidIndex(ReplayCursor))
    {
        return FAIHeroesBattleRecordRules::DescribeStep(LastBattleRecord, ReplayCursor);
    }
    return TEXT("LAST TACTIC: ") + LastCombatStep;
}

FAIHeroesAdvisorSnapshot AAIHeroesGameMode::BuildAdvisorSnapshot() const
{
    // This is the ONLY data passed into the offline strategic adviser.
    // No actor pointer, SaveGame, file path or mutable world authority leaks.
    FAIHeroesAdvisorSnapshot Request;
    Request.ProtocolTier = ProtocolTier;
    Request.MovesLeft = MovesLeft;
    Request.Gold = Gold;
    Request.Wood = Wood;
    Request.Ore = Ore;
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
        Snapshot.Guards = Node->Defenders;
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
