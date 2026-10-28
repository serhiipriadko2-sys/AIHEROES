#include "AIHeroesGameMode.h"

#include "AIHeroesAdvisor.h"
#include "AIHeroesBattleEngine.h"
#include "AIHeroesCampaignRules.h"
#include "AIHeroesCampaignSave.h"
#include "AIHeroesCameraPawn.h"
#include "AIHeroesHUD.h"
#include "AIHeroesPlayerController.h"
#include "AIHeroesTurnRules.h"
#include "AIHeroesWorldNode.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UObjectGlobals.h"

AAIHeroesGameMode::AAIHeroesGameMode()
{
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
        {FAIHeroesArmyStack(TEXT("Raider"), 10, 11, 5, 2, 5)});
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
    if (!FAIHeroesTurnRules::SpendIfAffordable(Gold, Wood, Ore, 50, 5, 0))
    {
        SetEvent(TEXT("Need 50 gold and 5 wood for 5 militia."));
        return;
    }
    AddStack(FAIHeroesArmyStack(TEXT("Militia"), 5, 12, 4, 2, 3));
    SetEvent(TEXT("Recruited 5 militia at HQ."));
}

void AAIHeroesGameMode::RecruitScouts()
{
    if (!FAIHeroesTurnRules::SpendIfAffordable(Gold, Wood, Ore, 70, 0, 4))
    {
        SetEvent(TEXT("Need 70 gold and 4 ore for 2 scouts."));
        return;
    }
    AddStack(FAIHeroesArmyStack(TEXT("Scout"), 2, 8, 6, 1, 7));
    SetEvent(TEXT("Recruited 2 scouts at HQ."));
}

void AAIHeroesGameMode::UpgradeProtocol()
{
    static const int32 WoodCosts[] = {20, 35, 50};
    static const int32 OreCosts[] = {15, 30, 45};
    static const int32 GoldCosts[] = {80, 150, 230};
    static const TCHAR* Protocols[] = {TEXT("THREAT ANALYSIS"), TEXT("MEMORY LEDGER"), TEXT("TACTICAL PLANNER")};

    if (ProtocolTier >= 3)
    {
        SetEvent(TEXT("All prototype protocols installed. Real LLM adapter is NOT connected."));
        return;
    }

    const int32 Index = ProtocolTier;
    if (!FAIHeroesTurnRules::SpendIfAffordable(
        Gold, Wood, Ore, GoldCosts[Index], WoodCosts[Index], OreCosts[Index]))
    {
        SetEvent(FString::Printf(TEXT("Next protocol needs %d wood, %d ore, %d gold."),
            WoodCosts[Index], OreCosts[Index], GoldCosts[Index]));
        return;
    }
    ++ProtocolTier;
    SetEvent(FString::Printf(TEXT("Unlocked %s. Strategy policy improved (offline)."), Protocols[Index]));
}

void AAIHeroesGameMode::UpgradeModelSlot()
{
    if (ModelSlotTier >= 2)
    {
        SetEvent(TEXT("8B is the highest prototype MODEL SLOT. No model weights loaded."));
        return;
    }
    static const int32 DataCosts[2] = {2, 6};
    static const int32 GoldCosts[2] = {60, 120};
    const int32 Index = ModelSlotTier;
    if (!FAIHeroesTurnRules::SpendModelSlotTokens(
        Gold, DataFragments, GoldCosts[Index], DataCosts[Index]))
    {
        SetEvent(FString::Printf(
            TEXT("Need %d gold + %d DATA for the next model SLOT; no weights are installed."),
            GoldCosts[Index], DataCosts[Index]));
        return;
    }
    ++ModelSlotTier;
    SetEvent(FString::Printf(
        TEXT("MODEL SLOT %s unlocked. NO actual LLM checkpoint loaded; combat is still rule-based."),
        ModelSlotTier == 1 ? TEXT("3B") : TEXT("8B")));
}

void AAIHeroesGameMode::EndTurn()
{
    ++Day;
    MovesLeft = 4;
    Gold += 40;
    for (const AAIHeroesWorldNode* Node : Nodes)
    {
        if (!Node || !Node->bOwned)
        {
            continue;
        }
        if (Node->NodeType == EAIHeroesNodeType::ForestMine)
        {
            Wood += 15;
        }
        else if (Node->NodeType == EAIHeroesNodeType::OreMine)
        {
            Ore += 12;
        }
        else if (Node->NodeType == EAIHeroesNodeType::Outpost)
        {
            Gold += 30;
        }
    }
    SetEvent(FString::Printf(TEXT("Day %d: income collected; movement restored to %d."), Day, MovesLeft));
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
    if (!IsValid(SelectedNode.Get()))
    {
        SetEvent(TEXT("Select a node before confirming your move."));
        return;
    }
    InteractWithNode(SelectedNode.Get());
}

void AAIHeroesGameMode::InteractWithNode(AAIHeroesWorldNode* Node)
{
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
        return;
    }

    if (!HasLivingArmy())
    {
        SetEvent(TEXT("No living army. Recruit units before confirming an attack."));
        return;
    }

    // The player authorizes battle. The hero AI chooses attacks inside rounds.
    MovesLeft -= Cost;
    const FAIHeroesBattleOutcome Outcome =
        FAIHeroesBattleEngine::Resolve(PlayerArmy, Node->Defenders, ProtocolTier);
    PlayerArmy = Outcome.PlayerAfter;
    Node->Defenders = Outcome.EnemyAfter;
    BattleJournal.Add(FString::Printf(TEXT("Day %d | %s | %s"),
        Day, *Node->GetDisplayName(), *Outcome.Summary));
    if (BattleJournal.Num() > 16)
    {
        BattleJournal.RemoveAt(0);
    }
    LastCombatStep = Outcome.Trace.IsEmpty()
        ? TEXT("No tactical actions occurred.")
        : Outcome.Trace.Last();

    if (Outcome.bVictory)
    {
        DataFragments += Node->NodeType == EAIHeroesNodeType::Ruins ? 4 : 2;
        Node->Defenders.Empty();
        Node->SetOwned(true);
        MoveHeroTo(Node->GetActorLocation());
        if (Node->NodeType == EAIHeroesNodeType::Ruins)
        {
            Gold += 100;
        }
        SetEvent(FString::Printf(TEXT("%s CAPTURED. AI chose its own targets. %s"),
            *Node->GetDisplayName(), *Outcome.Summary));
    }
    else
    {
        SetEvent(FString::Printf(TEXT("%s resisted. Hero stayed put. %s"),
            *Node->GetDisplayName(), *Outcome.Summary));
    }

    for (const FString& Line : Outcome.Trace)
    {
        UE_LOG(LogTemp, Display, TEXT("[AI HEROES BATTLE] %s"), *Line);
    }
}

void AAIHeroesGameMode::SaveCampaign()
{
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
    Snapshot->MovesLeft = MovesLeft;
    Snapshot->ProtocolTier = ProtocolTier;
    Snapshot->ModelSlotTier = ModelSlotTier;
    Snapshot->DataFragments = DataFragments;
    Snapshot->PlayerArmy = PlayerArmy;
    Snapshot->BattleJournal = BattleJournal;
    Snapshot->LastCombatStep = LastCombatStep;

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
    if (!UGameplayStatics::SaveGameToSlot(Snapshot, UAIHeroesCampaignSave::SlotName(), 0))
    {
        SetEvent(TEXT("SAVE FAILED: could not write the local save slot."));
        return;
    }

    SetEvent(FString::Printf(TEXT("GAME SAVED: day %d, %d nodes, protocol tier %d."),
        Day, Snapshot->NodeStates.Num(), ProtocolTier));
}

void AAIHeroesGameMode::LoadCampaign()
{
    const FString& Slot = UAIHeroesCampaignSave::SlotName();
    if (!UGameplayStatics::DoesSaveGameExist(Slot, 0))
    {
        SetEvent(TEXT("LOAD BLOCKED: no saved campaign. Save with F5 first."));
        return;
    }

    UAIHeroesCampaignSave* Snapshot =
        Cast<UAIHeroesCampaignSave>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
    if (!Snapshot)
    {
        SetEvent(TEXT("LOAD FAILED: unreadable or incompatible save class."));
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
    Day = Snapshot->Day;
    Gold = Snapshot->Gold;
    Wood = Snapshot->Wood;
    Ore = Snapshot->Ore;
    MovesLeft = Snapshot->MovesLeft;
    ProtocolTier = Snapshot->ProtocolTier;
    ModelSlotTier = Snapshot->ModelSlotTier;
    DataFragments = Snapshot->DataFragments;
    PlayerArmy = Snapshot->PlayerArmy;
    BattleJournal = Snapshot->BattleJournal;
    LastCombatStep = Snapshot->LastCombatStep;

    for (AAIHeroesWorldNode* Node : Nodes)
    {
        const FAIHeroesSavedNode* Saved =
            SavedById.FindChecked(FName(*Node->GetDisplayName()));
        Node->Defenders = Saved->Defenders;
        Node->SetOwned(Saved->bOwned);
    }

    SelectedNode = NewSelection;
    MoveHeroTo(HeroDestination->GetActorLocation());
    LastHint = TEXT("Loaded local campaign; ask H for a fresh tactical assessment.");
    SetEvent(FString::Printf(TEXT("GAME LOADED: day %d, %d nodes, protocol tier %d."),
        Day, Snapshot->NodeStates.Num(), ProtocolTier));
}

void AAIHeroesGameMode::AskAdvisor()
{
    // The advisor sees only a typed snapshot, never actor mutation functions.
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
    LastHint = FAIHeroesAdvisor::Advise(Request);
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
