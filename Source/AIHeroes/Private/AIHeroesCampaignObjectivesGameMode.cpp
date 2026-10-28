#include "AIHeroesGameMode.h"

#include "AIHeroesCampaignObjectives.h"
#include "AIHeroesWorldNode.h"

FAIHeroesCampaignProgress AAIHeroesGameMode::GetCampaignProgress() const
{
    FAIHeroesCampaignObjectiveInput Snapshot;
    Snapshot.ProtocolTier = ProtocolTier;
    Snapshot.ModelSlotTier = ModelSlotTier;
    for (const AAIHeroesWorldNode* Node : Nodes)
    {
        if (!IsValid(Node))
        {
            continue;
        }

        FAIHeroesObjectiveNode Entry;
        Entry.Id = FName(*Node->GetDisplayName());
        Entry.Type = Node->NodeType;
        Entry.bOwned = Node->bOwned;
        Snapshot.Nodes.Add(Entry);
    }
    return FAIHeroesCampaignObjectives::Evaluate(Snapshot);
}

void AAIHeroesGameMode::ShowCampaignObjectives()
{
    if (IsLiveBattleActive() || IsBattleReplayActive() || IsBasePanelVisible()) return;

    if (IsAdvisorDossierVisible())
    {
        const bool bAlreadyShowingGoals = AdvisorDossier.bObjectives;
        CloseAdvisorDossier();
        if (bAlreadyShowingGoals) return;
    }
    // This is a read-only report, not a command to change campaign state.
    const FAIHeroesCampaignProgress Progress = GetCampaignProgress();
    LastHint = Progress.Headline();
    RecordAdvisorDossier(TEXT("CAMPAIGN OBJECTIVES"),
        Progress.Report(), false, true);
    bAdvisorDossierOpen = true;
    UE_LOG(LogTemp, Display, TEXT("[AI HEROES OBJECTIVES] OPEN %d/%d victory=%d no_order=1"),
        Progress.Completed, Progress.Total, Progress.bVictory);
}

void AAIHeroesGameMode::AnnounceCampaignVictoryIfEarned()
{
    const FAIHeroesCampaignProgress Progress = GetCampaignProgress();
    if (Progress.bVictory)
    {
        SetEvent(TEXT("CAMPAIGN VICTORY: frontier liberated, protocols maxed, 8B-themed slot unlocked. Press K to save."));
        UE_LOG(LogTemp, Display, TEXT("[AI HEROES OBJECTIVES] VICTORY %d/%d no_auto_order=1"),
            Progress.Completed, Progress.Total);
    }
}
