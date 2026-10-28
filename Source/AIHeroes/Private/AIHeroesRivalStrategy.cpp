#include "AIHeroesRivalStrategy.h"

namespace
{
constexpr int32 MaxDay = 1000000;
constexpr int32 MaxResources = 500000000;
constexpr int32 RaiderHealth = 11;
constexpr int32 RaiderAttack = 5;
constexpr int32 RaiderDefense = 2;
constexpr int32 RaiderInitiative = 5;

bool IsValidRaider(const FAIHeroesArmyStack& Stack)
{
    return Stack.UnitId == FName(TEXT("Raider")) &&
        Stack.UnitHealth == RaiderHealth &&
        Stack.Attack == RaiderAttack &&
        Stack.Defense == RaiderDefense &&
        Stack.Initiative == RaiderInitiative &&
        Stack.TotalHealth >= 0 && Stack.TotalHealth <= 500000;
}

bool InputValid(const FAIHeroesRivalSnapshot& Input, FString& Error)
{
    if (Input.Day < 2 || Input.Day > MaxDay ||
        Input.Gold < 0 || Input.Gold > MaxResources ||
        Input.Wood < 0 || Input.Wood > MaxResources ||
        Input.Ore < 0 || Input.Ore > MaxResources ||
        !Input.bOutpostPresent || Input.OutpostDefenders.Num() > 2)
    {
        Error = TEXT("NPC turn rejected: invalid day, resources, or map topology.");
        return false;
    }
    for (const FAIHeroesArmyStack& Stack : Input.OutpostDefenders)
    {
        if (!IsValidRaider(Stack))
        {
            Error = TEXT("NPC turn rejected: unrecognized outpost garrison.");
            return false;
        }
        if (Input.bOutpostOwned && Stack.IsAlive())
        {
            Error = TEXT("NPC turn rejected: player-owned outpost has defenders.");
            return false;
        }
    }
    return true;
}

FAIHeroesRivalTurn RaidChoice(const FAIHeroesRivalSnapshot& Input)
{
    FAIHeroesRivalTurn Turn;
    // Alternating objective, with a deterministic fallback when one mine
    // is uncaptured or has no stored resources.
    const bool bPreferForest = (Input.Day / 3) % 2 == 0;
    const bool bCanForest = Input.bForestOwned && Input.Wood > 0;
    const bool bCanOre = Input.bOreOwned && Input.Ore > 0;
    if ((bPreferForest && bCanForest) || (!bCanOre && bCanForest))
    {
        Turn.Action = EAIHeroesRivalAction::RaidForest;
        Turn.Amount = FMath::Min(FAIHeroesRivalStrategy::ForestRaidLimit, Input.Wood);
        Turn.Description = FString::Printf(
            TEXT("RIVAL RAID: Day %d. Outpost raiders took %d WOOD from FOREST. Secure OUTPOST to end raids."),
            Input.Day, Turn.Amount);
    }
    else if (bCanOre)
    {
        Turn.Action = EAIHeroesRivalAction::RaidOre;
        Turn.Amount = FMath::Min(FAIHeroesRivalStrategy::OreRaidLimit, Input.Ore);
        Turn.Description = FString::Printf(
            TEXT("RIVAL RAID: Day %d. Outpost raiders took %d ORE from ORE mine. Secure OUTPOST to end raids."),
            Input.Day, Turn.Amount);
    }
    return Turn;
}
}

bool FAIHeroesRivalStrategy::Plan(
    const FAIHeroesRivalSnapshot& Input,
    FAIHeroesRivalTurn& Turn, FString& Error)
{
    Turn = FAIHeroesRivalTurn();
    Error.Empty();
    if (!InputValid(Input, Error)) return false;

    // Enemy faction is disabled permanently by the player's successful
    // capture of its outpost. No hidden attacks against player units occur.
    if (Input.bOutpostOwned) return true;

    // The new day 8,15,22,... begins a week of defensive recruiting. Use
    // existing persistent guard stacks; never invent new SaveGame fields.
    if ((Input.Day - 1) % 7 == 0)
    {
        const int32 MaximumHealth = MaxRaidersPerStack * RaiderHealth;
        for (int32 Index = 0; Index < Input.OutpostDefenders.Num(); ++Index)
        {
            const FAIHeroesArmyStack& Defender = Input.OutpostDefenders[Index];
            if (Defender.TotalHealth < MaximumHealth)
            {
                Turn.Action = EAIHeroesRivalAction::RecruitAtOutpost;
                Turn.DefenderIndex = Index;
                Turn.Amount = FMath::Min(WeeklyRecruits * RaiderHealth,
                    MaximumHealth - Defender.TotalHealth);
                break;
            }
        }
        if (!Turn.IsActive() && Input.OutpostDefenders.IsEmpty())
        {
            Turn.Action = EAIHeroesRivalAction::RecruitAtOutpost;
            Turn.DefenderIndex = 0;
            Turn.Amount = WeeklyRecruits * RaiderHealth;
        }
        if (Turn.IsActive())
        {
            Turn.Description = FString::Printf(
                TEXT("RIVAL RECRUIT: Day %d. OUTPOST reinforced (+%d guard HP). Player army unchanged."),
                Input.Day, Turn.Amount);
            return true;
        }
    }

    // Small resource pressure every third day, only after mine income and
    // only if the mine is actually player-owned. No automatic battle.
    if (Input.Day % 3 == 0)
    {
        Turn = RaidChoice(Input);
    }
    return true;
}

bool FAIHeroesRivalStrategy::Resolve(
    const FAIHeroesRivalSnapshot& Input,
    FAIHeroesRivalSnapshot& After, FAIHeroesRivalTurn& Turn, FString& Error)
{
    After = Input;
    if (!Plan(Input, Turn, Error))
    {
        After = Input;
        return false;
    }

    if (Turn.Action == EAIHeroesRivalAction::RaidForest)
    {
        After.Wood -= Turn.Amount;
    }
    else if (Turn.Action == EAIHeroesRivalAction::RaidOre)
    {
        After.Ore -= Turn.Amount;
    }
    else if (Turn.Action == EAIHeroesRivalAction::RecruitAtOutpost)
    {
        if (Turn.DefenderIndex == After.OutpostDefenders.Num() &&
            After.OutpostDefenders.IsEmpty())
        {
            After.OutpostDefenders.Add(FAIHeroesArmyStack(
                FName(TEXT("Raider")), 0, RaiderHealth,
                RaiderAttack, RaiderDefense, RaiderInitiative));
        }
        if (!After.OutpostDefenders.IsValidIndex(Turn.DefenderIndex))
        {
            After = Input;
            Turn = FAIHeroesRivalTurn();
            Error = TEXT("NPC turn rejected: inconsistent defender target.");
            return false;
        }
        After.OutpostDefenders[Turn.DefenderIndex].TotalHealth += Turn.Amount;
    }

    // Explicit postcondition: no resource theft below zero and no changed
    // gold, day, ownership, human army or unrelated map node.
    if (After.Wood < 0 || After.Ore < 0 ||
        After.Gold != Input.Gold || After.Day != Input.Day ||
        After.bOutpostOwned != Input.bOutpostOwned ||
        After.bForestOwned != Input.bForestOwned ||
        After.bOreOwned != Input.bOreOwned)
    {
        After = Input;
        Turn = FAIHeroesRivalTurn();
        Error = TEXT("NPC turn failed its postconditions; input preserved.");
        return false;
    }
    return true;
}
