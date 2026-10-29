#include "AIHeroesRecruitmentAdvisor.h"

#include "AIHeroesGridBattle.h"
#include "AIHeroesTurnRules.h"
#include "AIHeroesTravelRules.h"
#include "AIHeroesRecruitmentStock.h"

namespace
{
int32 HealthOf(const TArray<FAIHeroesArmyStack>& Army)
{
    int32 Total = 0;
    for (const FAIHeroesArmyStack& Stack : Army) Total += Stack.TotalHealth;
    return Total;
}

int32 UnitsOf(const TArray<FAIHeroesArmyStack>& Army)
{
    int32 Total = 0;
    for (const FAIHeroesArmyStack& Stack : Army) Total += Stack.Count();
    return Total;
}

bool BoundedArmy(const TArray<FAIHeroesArmyStack>& Army, bool bFriendly)
{
    if (Army.Num() > 2) return false;
    TSet<FName> Seen;
    for (const FAIHeroesArmyStack& S : Army)
    {
        // Grid V2 identifies enemy formations by side/index; Raider6+4
        // intentionally has two equal UnitIds. Friendly recruitment still merges.
        if (S.UnitId.IsNone() || (bFriendly && Seen.Contains(S.UnitId)) ||
            S.UnitHealth < 1 || S.UnitHealth > 1000 ||
            S.TotalHealth < 0 || S.TotalHealth > 500000 ||
            S.Attack < 1 || S.Attack > 1000 ||
            S.Defense < 0 || S.Defense > 1000 ||
            S.Initiative < 0 || S.Initiative > 1000) return false;
        Seen.Add(S.UnitId);
        if (bFriendly)
        {
            const bool bMilitia = S.UnitId == FName(TEXT("Militia"));
            const bool bScout = S.UnitId == FName(TEXT("Scout"));
            if ((!bMilitia && !bScout) ||
                S.UnitHealth != (bMilitia ? 12 : 8) ||
                S.Attack != (bMilitia ? 4 : 6) ||
                S.Defense != (bMilitia ? 2 : 1) ||
                S.Initiative != (bMilitia ? 3 : 7)) return false;
        }
    }
    return true;
}

void AddOffer(TArray<FAIHeroesArmyStack>& Army, const FAIHeroesRecruitOffer& Offer)
{
    if (Offer.Choice == EAIHeroesRecruitChoice::KeepArmy) return;
    for (FAIHeroesArmyStack& Stack : Army)
    {
        if (Stack.UnitId == Offer.Units.UnitId)
        {
            Stack.TotalHealth += Offer.Units.TotalHealth;
            return;
        }
    }
    Army.Add(Offer.Units);
}
}

FAIHeroesRecruitOffer FAIHeroesRecruitmentAdvisor::GetOffer(EAIHeroesRecruitChoice Choice)
{
    FAIHeroesRecruitOffer Offer;
    Offer.Choice = Choice;
    switch (Choice)
    {
        case EAIHeroesRecruitChoice::Militia:
            Offer.Label = TEXT("5 Militia");
            Offer.Gold = 50;
            Offer.Wood = 5;
            Offer.Units = FAIHeroesArmyStack(TEXT("Militia"), 5, 12, 4, 2, 3);
            break;
        case EAIHeroesRecruitChoice::Scouts:
            Offer.Label = TEXT("2 Scouts");
            Offer.Gold = 70;
            Offer.Ore = 4;
            Offer.Units = FAIHeroesArmyStack(TEXT("Scout"), 2, 8, 6, 1, 7);
            break;
        default:
            Offer.Choice = EAIHeroesRecruitChoice::KeepArmy;
            Offer.Label = TEXT("KEEP ARMY");
            break;
    }
    return Offer;
}

FAIHeroesRecruitAdvice FAIHeroesRecruitmentAdvisor::Evaluate(const FAIHeroesAdvisorSnapshot& S)
{
    FAIHeroesRecruitAdvice Result;
    if (!FAIHeroesTravelRules::IsSupported(S.TravelRuleVersion) ||
        S.ProtocolTier < 0 || S.ProtocolTier > 3 ||
        S.MovesLeft < 0 || S.MovesLeft > 4 ||
        S.Gold < 0 || S.Gold > 500000000 ||
        S.Wood < 0 || S.Wood > 500000000 ||
        S.Ore < 0 || S.Ore > 500000000 ||
        !FAIHeroesRecruitmentStockRules::IsValid(
            FAIHeroesRecruitmentStock{S.MilitiaReserve, S.ScoutReserve}) ||
        S.HeroPosition.ContainsNaN() || S.HeroPosition.SizeSquared() > 1.e12 ||
        !BoundedArmy(S.PlayerArmy, true))
    {
        Result.Summary = TEXT("RECRUIT: invalid snapshot. No advice or purchase issued.");
        return Result;
    }
    Result.bValidInput = true;
    for (EAIHeroesRecruitChoice Choice : {
        EAIHeroesRecruitChoice::KeepArmy,
        EAIHeroesRecruitChoice::Militia,
        EAIHeroesRecruitChoice::Scouts})
    {
        FAIHeroesRecruitOption Option;
        Option.Offer = GetOffer(Choice);
        Option.bAffordable = S.Gold >= Option.Offer.Gold &&
            S.Wood >= Option.Offer.Wood && S.Ore >= Option.Offer.Ore;
        if (Choice != EAIHeroesRecruitChoice::KeepArmy)
        {
            const FAIHeroesRecruitmentStock Inventory{
                S.MilitiaReserve, S.ScoutReserve};
            FString ReserveError;
            Option.bAffordable &= FAIHeroesRecruitmentStockRules::CanBuy(
                Inventory, Choice, Option.Offer.Units.Count(), ReserveError);
        }
        // Respect the campaign's per-stack health cap before suggesting a purchase.
        for (const FAIHeroesArmyStack& Stack : S.PlayerArmy)
        {
            if (Stack.UnitId == Option.Offer.Units.UnitId &&
                Stack.TotalHealth > 500000 - Option.Offer.Units.TotalHealth)
                Option.bAffordable = false;
        }
        Result.Options.Add(Option);
    }
    const FString Catalog = FString::Printf(
        TEXT("5 Militia: 50G+5W (%s); 2 Scouts: 70G+4O (%s). Stock: %d M/%d S. Player buys separately."),
        Result.Options[1].bAffordable ? TEXT("available") : TEXT("unavailable"),
        Result.Options[2].bAffordable ? TEXT("available") : TEXT("unavailable"),
        S.MilitiaReserve, S.ScoutReserve);
    Result.Detail = Catalog;
    if (S.ProtocolTier == 0)
    {
        Result.Summary = TEXT("SEED: recruitment prices only; battle forecast LOCKED. Unlock ANALYST with P.");
        return Result;
    }
    const FAIHeroesAdvisorNode* Target = S.Nodes.FindByPredicate(
        [&S](const FAIHeroesAdvisorNode& Node) { return Node.Id == S.SelectedNodeId; });
    if (!Target || S.SelectedNodeId.IsNone())
    {
        Result.Summary = TEXT("RECRUIT: select an enemy node to compare armies. No purchase needed just to ask.");
        return Result;
    }
    if (Target->Location.ContainsNaN() || Target->Location.SizeSquared() > 1.e12 ||
        !BoundedArmy(Target->Guards, false))
    {
        Result.bValidInput = false;
        Result.Summary = TEXT("RECRUIT: invalid target data; prediction blocked.");
        return Result;
    }
    if (Target->bOwned || UnitsOf(Target->Guards) == 0)
    {
        Result.RecommendedIndex = 0;
        Result.Summary = TEXT("KEEP ARMY: this node needs no combat. Preserve resources; player decides.");
        return Result;
    }
    const auto Path = FAIHeroesTravelRules::Find(S, S.HeroPosition, Target->Id, S.MovesLeft);
    if (Path.Status == EAIHeroesAdventurePathStatus::InvalidInput ||
        Path.Status == EAIHeroesAdventurePathStatus::Unreachable)
    {
        Result.Summary = TEXT("RECRUIT: ") + Path.Reason;
        return Result;
    }
    const int32 MoveCost = Path.TotalCost;
    Result.bTargetReachable = Path.IsReachableThisTurn();
    if (!Result.bTargetReachable)
    {
        Result.Summary = FString::Printf(
            TEXT("RECRUIT: target needs %d moves, you have %d. End day or choose nearer; no attack advised."),
            MoveCost, S.MovesLeft);
        return Result;
    }
    FString Comparison;
    for (int32 Index = 0; Index < Result.Options.Num(); ++Index)
    {
        FAIHeroesRecruitOption& Option = Result.Options[Index];
        if (!Option.bAffordable) continue;
        TArray<FAIHeroesArmyStack> Army = S.PlayerArmy;
        AddOffer(Army, Option.Offer);
        FAIHeroesGridSession Session;
        FString Error;
        bool bComplete = false;
        if (FAIHeroesGridBattle::Begin(Army, Target->Guards, S.ProtocolTier, Session, Error))
        {
            for (int32 Step = 0; Step <= FAIHeroesGridBattle::MaxActions; ++Step)
            {
                FAIHeroesGridAction Action;
                const auto Status = FAIHeroesGridBattle::Step(Session, Action, Error);
                if (Status == EAIHeroesGridStepResult::Completed) { bComplete = true; break; }
                if (Status == EAIHeroesGridStepResult::Error) break;
            }
        }
        if (!bComplete)
        {
            Result.RecommendedIndex = INDEX_NONE;
            Result.bValidInput = false;
            Result.Summary = TEXT("RECRUIT: Grid V2 forecast unavailable. No purchase or attack advised.");
            Result.Detail = Error.IsEmpty() ? TEXT("Prediction exceeded its action budget.") : Error;
            return Result;
        }
        const FAIHeroesBattleOutcome Forecast = FAIHeroesGridBattle::GetOutcome(Session);
        Option.bForecastAvailable = true;
        Option.bVictory = Forecast.bVictory;
        Option.LostHP = FMath::Max(0, HealthOf(Army) - HealthOf(Forecast.PlayerAfter));
        Option.Survivors = UnitsOf(Forecast.PlayerAfter);
        Option.Rounds = Forecast.Rounds;
        // Design heuristic, not a probability: 1 gold, 2 per material, 3 per lost HP.
        Option.BurdenScore = int64(Option.Offer.Gold) + 2LL * Option.Offer.Wood +
            2LL * Option.Offer.Ore + 3LL * Option.LostHP;
        if (!Comparison.IsEmpty()) Comparison += TEXT(" | ");
        Comparison += FString::Printf(TEXT("%s: %s -%dHP"), *Option.Offer.Label,
            Option.bVictory ? TEXT("WIN") : TEXT("NOT WIN"), Option.LostHP);
        if (Option.bVictory && (Result.RecommendedIndex == INDEX_NONE ||
            Option.BurdenScore < Result.Options[Result.RecommendedIndex].BurdenScore))
            Result.RecommendedIndex = Index;
    }
    Result.Detail = Comparison + TEXT(". Score = G+2W+2O+3 lost HP (lower wins). Grid V2.");
    if (Result.RecommendedIndex == INDEX_NONE)
    {
        Result.Summary = TEXT("RECRUIT: no affordable single purchase predicts victory. Avoid this fight or build a larger army.");
        return Result;
    }
    const FAIHeroesRecruitOption& Best = Result.Options[Result.RecommendedIndex];
    Result.Summary = FString::Printf(
        TEXT("%s: %dG %dW %dO; WIN forecast, -%dHP, %d survivors. No order issued; player decides."),
        *Best.Offer.Label, Best.Offer.Gold, Best.Offer.Wood, Best.Offer.Ore,
        Best.LostHP, Best.Survivors);
    return Result;
}
