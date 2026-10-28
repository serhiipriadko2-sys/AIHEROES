#include "AIHeroesGameMode.h"

#include "AIHeroesBattleEngine.h"
#include "AIHeroesCameraPawn.h"
#include "AIHeroesHUD.h"
#include "AIHeroesPlayerController.h"
#include "AIHeroesWorldNode.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
#include "Engine/SkyLight.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/PlayerController.h"
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

    LastHint = TEXT("Seed AI: no reasoning protocols. Press H for its limited status report.");
    SetEvent(TEXT("Welcome to AI HEROES. Recruit with 1 / 2, then click a red node to attack."));
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
    if (Gold < 50 || Wood < 5)
    {
        SetEvent(TEXT("Need 50 gold and 5 wood for 5 militia."));
        return;
    }
    Gold -= 50;
    Wood -= 5;
    AddStack(FAIHeroesArmyStack(TEXT("Militia"), 5, 12, 4, 2, 3));
    SetEvent(TEXT("Recruited 5 militia at HQ."));
}

void AAIHeroesGameMode::RecruitScouts()
{
    if (Gold < 70 || Ore < 4)
    {
        SetEvent(TEXT("Need 70 gold and 4 ore for 2 scouts."));
        return;
    }
    Gold -= 70;
    Ore -= 4;
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
    if (Wood < WoodCosts[Index] || Ore < OreCosts[Index] || Gold < GoldCosts[Index])
    {
        SetEvent(FString::Printf(TEXT("Next protocol needs %d wood, %d ore, %d gold."),
            WoodCosts[Index], OreCosts[Index], GoldCosts[Index]));
        return;
    }
    Wood -= WoodCosts[Index];
    Ore -= OreCosts[Index];
    Gold -= GoldCosts[Index];
    ++ProtocolTier;
    SetEvent(FString::Printf(TEXT("Unlocked %s. Strategy policy improved (offline)."), Protocols[Index]));
}

void AAIHeroesGameMode::EndTurn()
{
    ++Day;
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
    SetEvent(FString::Printf(TEXT("Turn ended. Day %d: income gathered."), Day));
}

void AAIHeroesGameMode::InteractWithNode(AAIHeroesWorldNode* Node)
{
    if (!IsValid(Node))
    {
        return;
    }
    if (Node->NodeType == EAIHeroesNodeType::Headquarters)
    {
        SetEvent(TEXT("HQ selected. Press 1 / 2 to recruit; P to upgrade the hero AI."));
        return;
    }
    if (Node->bOwned)
    {
        SetEvent(FString::Printf(TEXT("%s belongs to you. End turn to collect income."), *Node->GetDisplayName()));
        return;
    }
    if (!HasLivingArmy())
    {
        SetEvent(TEXT("No army. Recruit troops before attacking."));
        return;
    }

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

    if (Outcome.bVictory)
    {
        Node->Defenders.Empty();
        Node->SetOwned(true);
        if (Node->NodeType == EAIHeroesNodeType::Ruins)
        {
            Gold += 100;
        }
        SetEvent(FString::Printf(TEXT("%s CAPTURED. AI fought automatically. %s"),
            *Node->GetDisplayName(), *Outcome.Summary));
    }
    else
    {
        SetEvent(FString::Printf(TEXT("Attack on %s failed. %s"),
            *Node->GetDisplayName(), *Outcome.Summary));
    }

    for (const FString& Line : Outcome.Trace)
    {
        UE_LOG(LogTemp, Display, TEXT("[AI HEROES BATTLE] %s"), *Line);
    }
}

void AAIHeroesGameMode::AskAdvisor()
{
    AAIHeroesWorldNode* Target = nullptr;
    for (AAIHeroesWorldNode* Node : Nodes)
    {
        if (IsValid(Node) && !Node->bOwned)
        {
            Target = Node;
            break;
        }
    }

    if (!HasLivingArmy())
    {
        LastHint = TEXT("Army is empty. Recruit with 1 and 2. I cannot start combat.");
        return;
    }
    if (ProtocolTier == 0)
    {
        LastHint = TEXT("SEED AI: I can report your resources, but have no threat analysis protocol.");
        return;
    }
    if (!Target)
    {
        LastHint = TEXT("All strategic nodes captured. Develop the base and build a stronger army.");
        return;
    }

    const FAIHeroesBattleOutcome Forecast =
        FAIHeroesBattleEngine::Resolve(PlayerArmy, Target->Defenders, ProtocolTier);
    LastHint = FString::Printf(TEXT("Threat scan: %s. Forecast: %s. You decide whether to attack."),
        *Target->GetDisplayName(), Forecast.bVictory ? TEXT("likely win") : TEXT("danger: recruit first"));
    if (ProtocolTier >= 2 && !BattleJournal.IsEmpty())
    {
        LastHint += FString::Printf(TEXT(" Last record: %s."), *BattleJournal.Last());
    }
    if (ProtocolTier >= 3)
    {
        LastHint += TEXT(" Planner focuses vulnerable enemy stacks.");
    }
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
    switch (ProtocolTier)
    {
        case 0: return TEXT("SEED / 1B-themed (NO MODEL LOADED)");
        case 1: return TEXT("ANALYST / 3B-themed (RULE AI)");
        case 2: return TEXT("ARCHIVIST / 3B-themed (RULE AI)");
        default: return TEXT("STRATEGIST / 8B-themed (RULE AI)");
    }
}
