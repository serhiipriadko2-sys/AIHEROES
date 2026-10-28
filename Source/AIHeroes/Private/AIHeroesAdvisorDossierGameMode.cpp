#include "AIHeroesGameMode.h"

void AAIHeroesGameMode::RecordAdvisorDossier(
    const FString& Title, const TArray<FString>& Paragraphs, bool bRecruitment)
{
    AdvisorDossier = FAIHeroesAdvisorDossier();
    AdvisorDossier.Title = Title;
    AdvisorDossier.bRecruitment = bRecruitment;
    AdvisorDossier.ObservedDay = Day;
    AdvisorDossier.EvidenceKey = CampaignFingerprint();
    const FAIHeroesAdvisorSnapshot Snapshot = BuildAdvisorSnapshot();
    AdvisorDossier.Paragraphs.Add(FString::Printf(
        TEXT("OBSERVED: day %d | target %s | moves %d/4 | protocol %d | gold %d | wood %d | ore %d"),
        Day, Snapshot.SelectedNodeId.IsNone() ? TEXT("NONE") : *Snapshot.SelectedNodeId.ToString(),
        MovesLeft, ProtocolTier, Gold, Wood, Ore));
    AdvisorDossier.Paragraphs.Append(Paragraphs);
    AdvisorDossier.Paragraphs.Add(TEXT("BOUNDARY: offline rules, not a language model. This report never recruits, moves, attacks, saves or unlocks a protocol."));
    AdvisorDossierPage = 0;
}

bool AAIHeroesGameMode::IsAdvisorDossierVisible() const
{
    return bAdvisorDossierOpen && !IsLiveBattleActive() && !IsBattleReplayActive();
}

bool AAIHeroesGameMode::IsAdvisorDossierFresh() const
{
    if (!AdvisorDossier.HasContent() || AdvisorDossier.EvidenceKey.IsEmpty() ||
        AdvisorDossier.EvidenceKey == TEXT("INVALID MAP")) return false;
    return AdvisorDossier.EvidenceKey == CampaignFingerprint();
}

void AAIHeroesGameMode::ToggleAdvisorDossier()
{
    if (IsLiveBattleActive() || IsBattleReplayActive()) return;
    if (bAdvisorDossierOpen)
    {
        CloseAdvisorDossier();
        return;
    }
    if (!AdvisorDossier.HasContent()) AskAdvisor();
    bAdvisorDossierOpen = true;
    UE_LOG(LogTemp, Display, TEXT("[AI HEROES DOSSIER] OPEN fresh=%d kind=%s"),
        IsAdvisorDossierFresh(), AdvisorDossier.bRecruitment ? TEXT("recruitment") : TEXT("strategy"));
}

void AAIHeroesGameMode::CloseAdvisorDossier()
{
    if (!bAdvisorDossierOpen) return;
    bAdvisorDossierOpen = false;
    UE_LOG(LogTemp, Display, TEXT("[AI HEROES DOSSIER] CLOSE no_order=1"));
}

void AAIHeroesGameMode::RefreshAdvisorDossier()
{
    if (IsLiveBattleActive() || IsBattleReplayActive()) return;
    const bool bWasOpen = bAdvisorDossierOpen;
    const bool bRecruitment = AdvisorDossier.bRecruitment;
    // Temporarily release the UI-only command lock for a read-only query.
    // Neither query owns a campaign mutation path.
    bAdvisorDossierOpen = false;
    if (bRecruitment) AskRecruitmentAdvisor();
    else AskAdvisor();
    bAdvisorDossierOpen = bWasOpen;
    UE_LOG(LogTemp, Display, TEXT("[AI HEROES DOSSIER] REFRESH fresh=%d no_order=1"),
        IsAdvisorDossierFresh());
}

void AAIHeroesGameMode::ChangeAdvisorDossierPage(int32 Direction, int32 PageCount)
{
    if (!IsAdvisorDossierVisible()) return;
    const int32 Count = FMath::Clamp(PageCount, 1, 1024);
    AdvisorDossierPage = FAIHeroesAdvisorDossierRules::ClampPage(
        FAIHeroesAdvisorDossierRules::ClampPage(AdvisorDossierPage, Count) +
        FMath::Clamp(Direction, -1, 1), Count);
    UE_LOG(LogTemp, Display, TEXT("[AI HEROES DOSSIER] PAGE %d/%d"),
        AdvisorDossierPage + 1, Count);
}
