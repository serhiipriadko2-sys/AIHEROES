#include "AIHeroesCampaignObjectives.h"

namespace
{
bool IsCaptured(const FAIHeroesCampaignObjectiveInput& Input,
                const TCHAR* Id, EAIHeroesNodeType ExpectedType)
{
    int32 Seen = 0;
    bool bOwned = false;
    for (const FAIHeroesObjectiveNode& Node : Input.Nodes)
    {
        if (Node.Id == FName(Id))
        {
            ++Seen;
            bOwned = Node.Type == ExpectedType && Node.bOwned;
        }
    }
    // Duplicate IDs must never count as a captured objective.
    return Seen == 1 && bOwned;
}

FAIHeroesCampaignGoal MakeGoal(
    const TCHAR* Id, const TCHAR* Title, const TCHAR* Description,
    int32 Current, int32 Required)
{
    FAIHeroesCampaignGoal Goal;
    Goal.Id = FName(Id);
    Goal.Title = Title;
    Goal.Description = Description;
    Goal.Current = FMath::Clamp(Current, 0, Required);
    Goal.Required = Required;
    return Goal;
}
}

FString FAIHeroesCampaignGoal::ToReportLine() const
{
    return FString::Printf(TEXT("%s %s (%d/%d). %s"),
        IsComplete() ? TEXT("[DONE]") : TEXT("[OPEN]"),
        *Title, Current, Required, *Description);
}

FString FAIHeroesCampaignProgress::Headline() const
{
    return bVictory
        ? TEXT("CAMPAIGN VICTORY: all objectives achieved. Hero AI remains offline/rule-based.")
        : FString::Printf(TEXT("CAMPAIGN %d/%d | NEXT: %s"),
            Completed, Total, *NextAction);
}

TArray<FString> FAIHeroesCampaignProgress::Report() const
{
    TArray<FString> Lines;
    Lines.Add(Headline());
    for (const FAIHeroesCampaignGoal& Goal : Goals)
    {
        Lines.Add(Goal.ToReportLine());
    }
    Lines.Add(TEXT("Player chooses recruitment, upgrades, movement, turns and attacks; the hero AI commands battles."));
    Lines.Add(TEXT("MODEL SLOT 1B/3B/8B is a game upgrade, not a loaded neural checkpoint."));
    return Lines;
}

TArray<FString> FAIHeroesCampaignProgress::ReportRu() const
{
    TArray<FString> Lines;
    Lines.Add(bVictory
        ? TEXT("ПОБЕДА: все четыре условия кампании выполнены.")
        : FString::Printf(TEXT("ЦЕЛИ КАМПАНИИ: %d из %d выполнено."),
            FMath::Clamp(Completed,0,Total), Total));
    for(const FAIHeroesCampaignGoal& Goal:Goals)
    {
        const TCHAR* Title=TEXT("Неизвестная цель");
        const TCHAR* Meaning=TEXT("Нет подтверждённого описания.");
        if(Goal.Id==FName(TEXT("ECONOMY")))
        {
            Title=TEXT("Сеть снабжения");
            Meaning=TEXT("Захватите лесопилку и рудник.");
        }
        else if(Goal.Id==FName(TEXT("FRONTIER")))
        {
            Title=TEXT("Безопасные рубежи");
            Meaning=TEXT("Освободите сторожевую башню и древний архив.");
        }
        else if(Goal.Id==FName(TEXT("PROTOCOL")))
        {
            Title=TEXT("Развитие Искры");
            Meaning=TEXT("Купите три игровых протокола через Постройки.");
        }
        else if(Goal.Id==FName(TEXT("MODEL_SLOT")))
        {
            Title=TEXT("Слот модели");
            Meaning=TEXT("Откройте тематический слот 8B. Это не загрузка весов.");
        }
        Lines.Add(FString::Printf(TEXT("%s  %s  (%d/%d). %s"),
            Goal.IsComplete()?TEXT("[ГОТОВО]"):TEXT("[В ПРОЦЕССЕ]"),
            Title,Goal.Current,Goal.Required,Meaning));
    }
    Lines.Add(TEXT("Вы выбираете путь, набор армии и покупки. ИИ ведёт бой по правилам."));
    Lines.Add(TEXT("Слоты 1B/3B/8B обозначают игровые улучшения; настоящие веса модели не загружены."));
    return Lines;
}
FAIHeroesCampaignProgress FAIHeroesCampaignObjectives::Evaluate(
    const FAIHeroesCampaignObjectiveInput& Input)
{
    const bool bForest = IsCaptured(Input, TEXT("FOREST"), EAIHeroesNodeType::ForestMine);
    const bool bOre = IsCaptured(Input, TEXT("ORE"), EAIHeroesNodeType::OreMine);
    const bool bOutpost = IsCaptured(Input, TEXT("OUTPOST"), EAIHeroesNodeType::Outpost);
    const bool bRuins = IsCaptured(Input, TEXT("RUINS"), EAIHeroesNodeType::Ruins);

    FAIHeroesCampaignProgress Result;
    Result.Goals.Reserve(GoalCount);
    Result.Goals.Add(MakeGoal(TEXT("ECONOMY"), TEXT("Secure the supply network"),
        TEXT("Capture FOREST and ORE to sustain upgrades and recruitment."),
        static_cast<int32>(bForest) + static_cast<int32>(bOre), 2));
    Result.Goals.Add(MakeGoal(TEXT("FRONTIER"), TEXT("Liberate the frontier"),
        TEXT("Capture OUTPOST and RUINS using player-authorized battles."),
        static_cast<int32>(bOutpost) + static_cast<int32>(bRuins), 2));
    Result.Goals.Add(MakeGoal(TEXT("PROTOCOL"), TEXT("Evolve the hero's protocols"),
        TEXT("Unlock all three actual offline strategy protocols (P)."),
        FMath::Clamp(Input.ProtocolTier, 0, 3), 3));
    Result.Goals.Add(MakeGoal(TEXT("MODEL_SLOT"), TEXT("Develop the model-capacity slot"),
        TEXT("Unlock the 8B-themed slot (M); no neural weights are installed."),
        FMath::Clamp(Input.ModelSlotTier, 0, 2), 2));

    Result.Total = Result.Goals.Num();
    for (const FAIHeroesCampaignGoal& Goal : Result.Goals)
    {
        Result.Completed += Goal.IsComplete() ? 1 : 0;
    }
    Result.bVictory = Result.Total == GoalCount && Result.Completed == Result.Total;

    // Objective IDs are independent of the display language and backed by
    // the same conditions as the existing campaign victory calculation.
    if (Result.bVictory)
    {
        Result.NextFocusId = FName(TEXT("VICTORY"));
        Result.NextAction = TEXT("Victory achieved. You may keep exploring or save the campaign (K).");
    }
    else if (!bForest)
    {
        Result.NextFocusId = FName(TEXT("FOREST"));
        Result.NextAction = TEXT("Capture FOREST");
    }
    else if (!bOre)
    {
        Result.NextFocusId = FName(TEXT("ORE"));
        Result.NextAction = TEXT("Capture ORE");
    }
    else if (!bOutpost)
    {
        Result.NextFocusId = FName(TEXT("OUTPOST"));
        Result.NextAction = TEXT("Capture OUTPOST");
    }
    else if (!bRuins)
    {
        Result.NextFocusId = FName(TEXT("RUINS"));
        Result.NextAction = TEXT("Capture RUINS");
    }
    else if (Input.ProtocolTier < 3)
    {
        Result.NextFocusId = FName(TEXT("PROTOCOL"));
        Result.NextAction = TEXT("Unlock the next protocol (P)");
    }
    else
    {
        Result.NextFocusId = FName(TEXT("MODEL_SLOT"));
        Result.NextAction = TEXT("Upgrade your themed model slot (M)");
    }

    return Result;
}
