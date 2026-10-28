#pragma once

#include "CoreMinimal.h"
#include "AIHeroesTypes.generated.h"

UENUM(BlueprintType)
enum class EAIHeroesNodeType : uint8
{
    Headquarters,
    ForestMine,
    OreMine,
    Outpost,
    Ruins
};

USTRUCT(BlueprintType)
struct FAIHeroesArmyStack
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName UnitId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 UnitHealth = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TotalHealth = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Attack = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Defense = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Initiative = 1;

    FAIHeroesArmyStack() = default;

    FAIHeroesArmyStack(FName InId, int32 InCount, int32 InHealth, int32 InAttack, int32 InDefense, int32 InInitiative)
        : UnitId(InId),
          UnitHealth(FMath::Max(1, InHealth)),
          TotalHealth(FMath::Max(0, InCount) * UnitHealth),
          Attack(FMath::Max(1, InAttack)),
          Defense(FMath::Max(0, InDefense)),
          Initiative(FMath::Max(0, InInitiative))
    {
    }

    int32 Count() const
    {
        return TotalHealth > 0 ? (TotalHealth + FMath::Max(1, UnitHealth) - 1) / FMath::Max(1, UnitHealth) : 0;
    }

    bool IsAlive() const
    {
        return TotalHealth > 0;
    }
};
