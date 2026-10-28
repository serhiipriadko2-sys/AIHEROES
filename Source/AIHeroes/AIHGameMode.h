#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AIHSimulation.h"
#include "AIHWorldNode.h"
#include "AIHGameMode.generated.h"

class AStaticMeshActor;

// The rules engine owns resources, turns, movement and battle outcomes.
// The advisor never modifies game state directly.
UCLASS()
class AIHEROES_API AAIHGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AAIHGameMode();
    virtual void BeginPlay() override;

    void SelectNode(AAIHWorldNode* Node);
    void PerformAction(int32 ActionIndex);

    FString GetStatusLine() const;
    FString GetArmyLine() const;
    FString GetSelectionLine() const;
    FString GetReportLine() const { return LastReport; }
    FString GetAdviceLine() const { return LastAdvice; }
    FString GetBrainLine() const;

private:
    void BuildWorld();
    void AddNode(EAIHNodeKind Kind, const TCHAR* Name, int32 X, int32 Y, int32 Threat);
    void InteractWithSelected();
    void Recruit(int32 Unit);
    void EndTurn();
    void AskAdvice();
    void UpgradeProtocol();
    void UpgradeModelSlot();
    void MoveHeroMarker();
    FVector CellToWorld(FIntPoint Cell) const;

    UPROPERTY()
    TArray<TObjectPtr<AAIHWorldNode>> Nodes;
    UPROPERTY()
    TObjectPtr<AAIHWorldNode> SelectedNode;
    UPROPERTY()
    TObjectPtr<AStaticMeshActor> HeroMarker;

    FAIHArmy Army;
    FIntPoint HeroCell = FIntPoint(3, 3);
    int32 Turn = 1;
    int32 MovesLeft = 3;
    int32 Ore = 8;
    int32 Energy = 5;
    int32 Data = 0;
    int32 ProtocolRank = 0;
    int32 ModelSlotIndex = 0;
    FString LastReport = TEXT("Choose a node, then MOVE / CONQUER.");
    FString LastAdvice = TEXT("Press ASK HERO for an explicit heuristic recommendation.");
};
