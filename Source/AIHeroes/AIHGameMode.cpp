#include "AIHGameMode.h"

#include "AIHCameraPawn.h"
#include "AIHGameHUD.h"
#include "AIHPlayerController.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"

AAIHGameMode::AAIHGameMode()
{
    DefaultPawnClass = AAIHCameraPawn::StaticClass();
    PlayerControllerClass = AAIHPlayerController::StaticClass();
    HUDClass = AAIHGameHUD::StaticClass();
}

FVector AAIHGameMode::CellToWorld(FIntPoint Cell) const
{
    return FVector((Cell.X - 3) * 320.f, (Cell.Y - 3) * 320.f, 0.f);
}

void AAIHGameMode::BeginPlay()
{
    Super::BeginPlay();
    BuildWorld();
}

void AAIHGameMode::BuildWorld()
{
    UWorld* World = GetWorld();
    UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr,
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr,
        TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (!World || !Cube || !Sphere)
    {
        LastReport = TEXT("ERROR: Unreal basic shapes not found. World cannot be built.");
        return;
    }

    for (int32 X = 0; X < 7; ++X)
    {
        for (int32 Y = 0; Y < 7; ++Y)
        {
            const FVector Location = CellToWorld(FIntPoint(X, Y)) + FVector(0, 0, -38.f);
            AStaticMeshActor* Tile = World->SpawnActor<AStaticMeshActor>(
                Location, FRotator::ZeroRotator);
            if (Tile)
            {
                UStaticMeshComponent* Mesh = Tile->GetStaticMeshComponent();
                Mesh->SetStaticMesh(Cube);
                Mesh->SetWorldScale3D(FVector(3.1f, 3.1f, 0.15f));
                Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
            }
        }
    }

    if (ADirectionalLight* Sun = World->SpawnActor<ADirectionalLight>(
        FVector(0, 0, 1300), FRotator(-55.f, -32.f, 0.f)))
    {
        // Light source for the runtime-generated board.
    }

    HeroMarker = World->SpawnActor<AStaticMeshActor>(
        CellToWorld(HeroCell) + FVector(0, 0, 105.f), FRotator::ZeroRotator);
    if (HeroMarker)
    {
        UStaticMeshComponent* Mesh = HeroMarker->GetStaticMeshComponent();
        Mesh->SetMobility(EComponentMobility::Movable);
        Mesh->SetStaticMesh(Sphere);
        Mesh->SetWorldScale3D(FVector(0.48f, 0.48f, 0.48f));
        Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);
        // This sphere shows the hero's strategic position.
    }

    AddNode(EAIHNodeKind::Core, TEXT("HOME CORE"), 3, 3, 0);
    AddNode(EAIHNodeKind::Ore, TEXT("IRON RIDGE"), 4, 3, 0);
    AddNode(EAIHNodeKind::Energy, TEXT("SUN WELL"), 3, 4, 0);
    AddNode(EAIHNodeKind::Data, TEXT("DATA RUINS"), 2, 3, 0);
    AddNode(EAIHNodeKind::Neutral, TEXT("ROGUE SCOUTS"), 5, 3, 13);
    AddNode(EAIHNodeKind::Ore, TEXT("OLD QUARRY"), 5, 4, 0);
    AddNode(EAIHNodeKind::Enemy, TEXT("RED BASTION"), 5, 5, 25);
    AddNode(EAIHNodeKind::Neutral, TEXT("STONE WATCH"), 1, 4, 17);
    AddNode(EAIHNodeKind::Data, TEXT("LOST ARCHIVE"), 1, 5, 0);
    AddNode(EAIHNodeKind::Energy, TEXT("ARC SPRING"), 2, 1, 0);
    AddNode(EAIHNodeKind::Enemy, TEXT("ASH CITADEL"), 0, 1, 35);
    LastReport = TEXT("World ready. Select a nearby node, then MOVE / CONQUER.");
}

void AAIHGameMode::AddNode(
    EAIHNodeKind Kind, const TCHAR* Name, int32 X, int32 Y, int32 Threat)
{
    if (!GetWorld())
    {
        return;
    }
    const FIntPoint Cell(X, Y);
    AAIHWorldNode* Node = GetWorld()->SpawnActor<AAIHWorldNode>(
        CellToWorld(Cell), FRotator::ZeroRotator);
    if (Node)
    {
        Node->SetCell(Cell);
        Node->Configure(Kind, FString(Name), Threat);
        Nodes.Add(Node);
    }
}

void AAIHGameMode::MoveHeroMarker()
{
    if (HeroMarker)
    {
        HeroMarker->SetActorLocation(CellToWorld(HeroCell) + FVector(0, 0, 105.f));
    }
}

void AAIHGameMode::SelectNode(AAIHWorldNode* Node)
{
    SelectedNode = Node;
    LastReport = SelectedNode
        ? FString::Printf(TEXT("Selected %s. MOVE / CONQUER to visit it."),
            *SelectedNode->GetNodeName())
        : TEXT("No node selected.");
    LastAdvice = TEXT("ASK HERO to assess this node.");
}

void AAIHGameMode::PerformAction(int32 ActionIndex)
{
    switch (ActionIndex)
    {
    case 0: Recruit(0); break;
    case 1: Recruit(1); break;
    case 2: Recruit(2); break;
    case 3: InteractWithSelected(); break;
    case 4: AskAdvice(); break;
    case 5: EndTurn(); break;
    case 6: UpgradeProtocol(); break;
    case 7: UpgradeModelSlot(); break;
    default: break;
    }
}

void AAIHGameMode::Recruit(int32 Unit)
{
    if (Unit == 0 && Ore >= 2)
    {
        Ore -= 2;
        ++Army.Scouts;
        LastReport = TEXT("Recruited one Scout for 2 ore.");
    }
    else if (Unit == 1 && Ore >= 5 && Energy >= 1)
    {
        Ore -= 5;
        --Energy;
        ++Army.Guardians;
        LastReport = TEXT("Recruited one Guardian for 5 ore + 1 energy.");
    }
    else if (Unit == 2 && Ore >= 1 && Energy >= 3 && Data >= 1)
    {
        --Ore;
        Energy -= 3;
        --Data;
        ++Army.Sparks;
        LastReport = TEXT("Recruited one Spark for 1 ore + 3 energy + 1 data.");
    }
    else
    {
        LastReport = TEXT("Insufficient resources: Scout 2 ore | Guardian 5 ore+1 energy | Spark 1 ore+3 energy+1 data.");
    }
}

void AAIHGameMode::InteractWithSelected()
{
    if (!SelectedNode)
    {
        LastReport = TEXT("Select a world node first.");
        return;
    }

    const FIntPoint Target = SelectedNode->GetCell();
    const int32 Distance = FMath::Abs(Target.X - HeroCell.X) +
        FMath::Abs(Target.Y - HeroCell.Y);
    const bool bBattle = !SelectedNode->IsCaptured() &&
        (SelectedNode->GetKind() == EAIHNodeKind::Neutral ||
         SelectedNode->GetKind() == EAIHNodeKind::Enemy);
    const int32 Cost = FMath::Max(Distance, bBattle ? 1 : 0);
    if (Cost > MovesLeft)
    {
        LastReport = FString::Printf(TEXT("Out of range: need %d move points, have %d."),
            Cost, MovesLeft);
        return;
    }

    if (bBattle)
    {
        if (Army.Count() == 0)
        {
            LastReport = TEXT("Cannot battle without units. Recruit at the base.");
            return;
        }

        MovesLeft -= Cost;
        const FAIHCombatReport Battle = FAIHCombatEngine::Resolve(
            Army, SelectedNode->GetThreat(), ProtocolRank);
        Army = Battle.Survivors;
        LastReport = FString::Printf(TEXT("HERO AI: %s"), *Battle.Explanation);
        if (!Battle.bVictory)
        {
            LastAdvice = TEXT("Defeat. The AI withdrew; recruit and reassess before retrying.");
            return;
        }
        Data += 2;
        SelectedNode->Capture();
    }
    else
    {
        MovesLeft -= Cost;
        if (!SelectedNode->IsCaptured())
        {
            SelectedNode->Capture();
            if (SelectedNode->GetKind() == EAIHNodeKind::Data)
            {
                Data += 3;
            }
            LastReport = FString::Printf(TEXT("Claimed %s. Resource production begins next turn."),
                *SelectedNode->GetNodeName());
        }
        else
        {
            LastReport = FString::Printf(TEXT("Arrived at %s."),
                *SelectedNode->GetNodeName());
        }
    }

    HeroCell = Target;
    MoveHeroMarker();

    int32 EnemyNodesLeft = 0;
    for (const AAIHWorldNode* Node : Nodes)
    {
        if (IsValid(Node) && Node->GetKind() == EAIHNodeKind::Enemy && !Node->IsCaptured())
        {
            ++EnemyNodesLeft;
        }
    }
    if (EnemyNodesLeft == 0)
    {
        LastReport = TEXT("CAMPAIGN WON! Both enemy citadels captured by your evolving hero AI.");
    }
}

void AAIHGameMode::EndTurn()
{
    ++Turn;
    MovesLeft = 3;
    Ore += 2;
    Energy += 1;

    for (const AAIHWorldNode* Node : Nodes)
    {
        if (!IsValid(Node) || !Node->IsCaptured())
        {
            continue;
        }
        switch (Node->GetKind())
        {
        case EAIHNodeKind::Ore: Ore += 3; break;
        case EAIHNodeKind::Energy: Energy += 3; break;
        case EAIHNodeKind::Data: Data += 2; break;
        default: break;
        }
    }
    LastReport = FString::Printf(TEXT("Turn %d: income paid; 3 movement points restored."), Turn);
}

void AAIHGameMode::AskAdvice()
{
    if (!SelectedNode)
    {
        LastAdvice = TEXT("Select a resource site or opponent before asking for advice.");
        return;
    }

    const int32 Distance = FMath::Abs(SelectedNode->GetCell().X - HeroCell.X) +
        FMath::Abs(SelectedNode->GetCell().Y - HeroCell.Y);
    const int32 Threat = SelectedNode->GetThreat();
    const int32 EstimatedPower = Army.Strength() +
        (Army.Scouts >= 3 ? 2 : 0) + (Army.Guardians >= 2 ? 3 : 0) +
        ProtocolRank * 2;

    if (Distance > MovesLeft)
    {
        LastAdvice = FString::Printf(TEXT("Distance %d > moves %d. End turn or choose a nearer site."),
            Distance, MovesLeft);
    }
    else if (SelectedNode->IsCaptured())
    {
        LastAdvice = TEXT("Already controlled. Securing more production sites is usually more useful.");
    }
    else if (Threat == 0)
    {
        LastAdvice = TEXT("Free node. Capturing it improves resource income next turn.");
    }
    else if (ProtocolRank == 0)
    {
        LastAdvice = TEXT("BASIC heuristic: enemy appears dangerous. No trace or risk audit unlocked.");
    }
    else if (ProtocolRank == 1)
    {
        LastAdvice = FString::Printf(TEXT("TRACE: visible threat %d; our estimated power %d. No safety margin assessment."),
            Threat, EstimatedPower);
    }
    else if (EstimatedPower < Threat)
    {
        LastAdvice = FString::Printf(TEXT("SIFT: power %d vs threat %d. Avoid attack; recruit or raise protocol rank."),
            EstimatedPower, Threat);
    }
    else if (ProtocolRank >= 4 && EstimatedPower < Threat + 5)
    {
        LastAdvice = TEXT("GUARD: narrow winning margin. Recruit more units to reduce likely casualties.");
    }
    else
    {
        LastAdvice = FString::Printf(TEXT("PLAN: estimated advantage +%d. Attack possible; casualties still expected."),
            EstimatedPower - Threat);
    }
}

void AAIHGameMode::UpgradeProtocol()
{
    if (ProtocolRank >= 5)
    {
        LastReport = TEXT("All six prototype cognitive stages unlocked (NOT full SoT30).");
        return;
    }
    const int32 Cost = 2 + ProtocolRank * 2;
    if (Data < Cost)
    {
        LastReport = FString::Printf(TEXT("Protocol upgrade needs %d data; you have %d."), Cost, Data);
        return;
    }
    Data -= Cost;
    ++ProtocolRank;
    LastReport = FString::Printf(TEXT("Unlocked cognitive stage %d. Battle policy and advice improved."),
        ProtocolRank);
}

void AAIHGameMode::UpgradeModelSlot()
{
    if (ModelSlotIndex >= 2)
    {
        LastReport = TEXT("8B SLOT is max prototype tier. No weights are loaded.");
        return;
    }
    const int32 DataCost = 5 + ModelSlotIndex * 6;
    const int32 EnergyCost = 2 + ModelSlotIndex * 3;
    if (Data < DataCost || Energy < EnergyCost)
    {
        LastReport = FString::Printf(TEXT("Next model SLOT needs %d data and %d energy."),
            DataCost, EnergyCost);
        return;
    }

    Data -= DataCost;
    Energy -= EnergyCost;
    ++ModelSlotIndex;
    LastReport = TEXT("Model slot unlocked; NO actual LLM checkpoint is installed.");
}

FString AAIHGameMode::GetStatusLine() const
{
    return FString::Printf(TEXT("TURN %d | MOVES %d/3 | ORE %d | ENERGY %d | DATA %d"),
        Turn, MovesLeft, Ore, Energy, Data);
}

FString AAIHGameMode::GetArmyLine() const
{
    return FString::Printf(TEXT("ARMY: Scout %d | Guardian %d | Spark %d | strength %d"),
        Army.Scouts, Army.Guardians, Army.Sparks, Army.Strength());
}

FString AAIHGameMode::GetSelectionLine() const
{
    if (!SelectedNode)
    {
        return TEXT("NODE: none selected");
    }
    return TEXT("NODE: ") + SelectedNode->Describe();
}

FString AAIHGameMode::GetBrainLine() const
{
    static const TCHAR* Stages[6] = {
        TEXT("BASIC"), TEXT("TRACE"), TEXT("SIFT"), TEXT("PLANNING"),
        TEXT("GUARD"), TEXT("COUNCIL")
    };
    static const TCHAR* Slots[3] = { TEXT("1B"), TEXT("3B"), TEXT("8B") };
    return FString::Printf(
        TEXT("HERO AI: %s | MODEL %s SLOT (NO LLM) | LOCAL RULE-BASED BRAIN"),
        Stages[ProtocolRank], Slots[ModelSlotIndex]);
}
