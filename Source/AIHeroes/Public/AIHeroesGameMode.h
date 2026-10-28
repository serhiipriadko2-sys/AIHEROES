#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AIHeroesTypes.h"
#include "AIHeroesGameMode.generated.h"

class AAIHeroesWorldNode;

UCLASS()
class AIHEROES_API AAIHeroesGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AAIHeroesGameMode();
    virtual void BeginPlay() override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 Day = 1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 Gold = 240;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 Wood = 35;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 Ore = 30;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    int32 ProtocolTier = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
    TArray<FAIHeroesArmyStack> PlayerArmy;

    UPROPERTY()
    TArray<TObjectPtr<AAIHeroesWorldNode>> Nodes;

    FString LastEvent;
    FString LastHint;
    TArray<FString> BattleJournal;

    void RecruitMilitia();
    void RecruitScouts();
    void UpgradeProtocol();
    void EndTurn();
    void InteractWithNode(AAIHeroesWorldNode* Node);
    void AskAdvisor();

    FString GetArmySummary() const;
    FString GetCognitionSummary() const;

private:
    void SpawnWorld();
    AAIHeroesWorldNode* SpawnNode(
        EAIHeroesNodeType Type,
        const FString& Name,
        const FVector& At,
        const TArray<FAIHeroesArmyStack>& Guards);
    void SetEvent(const FString& Event);
    void AddStack(const FAIHeroesArmyStack& NewStack);
    bool HasLivingArmy() const;
};
