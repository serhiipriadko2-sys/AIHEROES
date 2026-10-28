#pragma once

#include "CoreMinimal.h"

// Shared deterministic rules: no LLM and no UObject/world dependencies.
// A failed validation never mutates state.
struct FAIHeroesTurnRules
{
    static int32 TravelCost(const FVector& From, const FVector& To, bool bHostile)
    {
        const float Distance = FVector::Dist2D(From, To);
        const int32 Steps = FMath::CeilToInt(Distance / 700.f);
        return FMath::Max(bHostile ? 1 : 0, Steps);
    }

    static bool CanMove(int32 AvailableMoves, int32 RequiredMoves)
    {
        return AvailableMoves >= 0 && RequiredMoves >= 0 &&
            RequiredMoves <= AvailableMoves;
    }

    static bool SpendModelSlotTokens(
        int32& Gold, int32& Data, int32 GoldCost, int32 DataCost)
    {
        if (GoldCost < 0 || DataCost < 0 || Gold < GoldCost || Data < DataCost)
        {
            return false;
        }
        Gold -= GoldCost;
        Data -= DataCost;
        return true;
    }

    static bool SpendIfAffordable(
        int32& Gold, int32& Wood, int32& Ore,
        int32 GoldCost, int32 WoodCost, int32 OreCost)
    {
        if (GoldCost < 0 || WoodCost < 0 || OreCost < 0 ||
            Gold < GoldCost || Wood < WoodCost || Ore < OreCost)
        {
            return false;
        }

        Gold -= GoldCost;
        Wood -= WoodCost;
        Ore -= OreCost;
        return true;
    }
};
