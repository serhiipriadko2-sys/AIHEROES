#include "AIHeroesGameMode.h"

#include "AIHeroesBaseRules.h"
#include "AIHeroesWorldNode.h"

bool AAIHeroesGameMode::IsAtSelectedOwnedHeadquarters() const
{
    const AAIHeroesWorldNode* HQ = SelectedNode.Get();
    return IsValid(HQ) && Nodes.Contains(SelectedNode) && HQ->bOwned &&
        HQ->NodeType == EAIHeroesNodeType::Headquarters &&
        HQ->GetDisplayName() == TEXT("HQ") &&
        FVector::DistSquared(HeroStrategicLocation, HQ->GetActorLocation()) < 1.0;
}

void AAIHeroesGameMode::UpgradeHeadquarters()
{
    if (BlockStrategicAction()) return;
    FString Error;
    if (!FAIHeroesBaseRules::TryUpgrade(BaseTier, Gold, Wood, Ore,
        IsAtSelectedOwnedHeadquarters(), Error))
    {
        SetEvent(Error);
        return;
    }
    InvalidateRoutePreview();
    RefreshHeadquartersVisual();
    SetEvent(FString::Printf(
        TEXT("HQ UPGRADED L%d: headquarters pays %d gold per day from NEXT DAY. No immediate income."),
        BaseTier, FAIHeroesBaseRules::DailyGold(BaseTier)));
    LastHint = GetHeadquartersSummary();
}

void AAIHeroesGameMode::RefreshHeadquartersVisual()
{
    for (AAIHeroesWorldNode* Node : Nodes)
    {
        if (IsValid(Node) && Node->NodeType == EAIHeroesNodeType::Headquarters)
            Node->SetHeadquartersTier(BaseTier);
    }
}

FString AAIHeroesGameMode::GetHeadquartersSummary() const
{
    if (!FAIHeroesBaseRules::IsValidTier(BaseTier)) return TEXT("HQ: invalid level; upgrade blocked.");
    const FAIHeroesBaseUpgradeCost Cost = FAIHeroesBaseRules::UpgradeCost(BaseTier);
    if (!Cost.bAvailable)
        return FString::Printf(
            TEXT("HQ L%d MAX | %d G/day | RESERVE M%d S%d | weekly +%d M +%d S"),
            BaseTier, FAIHeroesBaseRules::DailyGold(BaseTier),
            RecruitmentStock.Militia, RecruitmentStock.Scouts,
            FAIHeroesRecruitmentStockRules::MilitiaGrowth(BaseTier),
            FAIHeroesRecruitmentStockRules::ScoutGrowth(BaseTier, false));
    return FString::Printf(
        TEXT("HQ L%d | %d G/day | RESERVE M%d S%d | upgrade %dG %dW %dO | %s"),
        BaseTier, FAIHeroesBaseRules::DailyGold(BaseTier),
        RecruitmentStock.Militia, RecruitmentStock.Scouts,
        Cost.Gold, Cost.Wood, Cost.Ore,
        IsAtSelectedOwnedHeadquarters() ? TEXT("U / HQ + to buy") : TEXT("select HQ and return hero"));
}

int32 AAIHeroesGameMode::GetDailyGoldIncome() const
{
    int32 Income = FAIHeroesBaseRules::DailyGold(BaseTier);
    for (const AAIHeroesWorldNode* Node : Nodes)
        if (IsValid(Node) && Node->bOwned && Node->NodeType == EAIHeroesNodeType::Outpost)
            Income += 30;
    return Income;
}
