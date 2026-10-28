#include "AIHeroesGameMode.h"

#include "AIHeroesProtocolLab.h"

// Entire feature is display-only. Historical armies are copied into pure
// deterministic battle rules. GameMode never applies the alternate outcomes.
void AAIHeroesGameMode::OpenProtocolLab()
{
    if (IsLiveBattleActive() || IsBattleReplayActive() || IsBasePanelVisible())
    {
        return;
    }
    if (IsAdvisorDossierVisible())
    {
        const bool bWasLab = AdvisorDossier.bProtocolLab;
        CloseAdvisorDossier();
        if (bWasLab)
        {
            return;
        }
    }

    FAIHeroesProtocolLabReport Report;
    FString Error;
    TArray<FString> Lines;
    const bool bVerified = FAIHeroesProtocolLabRules::Analyze(
        LastBattleRecord, Report, Error);
    if (bVerified)
    {
        Lines = Report.Describe();
        LastHint = FString::Printf(
            TEXT("PROTOCOL LAB: compared 4 rule tiers for verified %s battle. No orders executed."),
            *Report.NodeId.ToString());
    }
    else
    {
        Lines.Add(TEXT("NO VERIFIED BATTLE AVAILABLE FOR COUNTERFACTUAL ANALYSIS."));
        Lines.Add(TEXT("Finish one guarded encounter, then open this screen again."));
        Lines.Add(TEXT("REASON: ") + Error.Left(240));
        Lines.Add(TEXT("No speculative outcomes will be generated from missing or invalid history."));
        Lines.Add(TEXT("NO ACTION EXECUTED. Hero and campaign remain unchanged."));
        LastHint = TEXT("PROTOCOL LAB: need verified battle history.");
    }

    RecordAdvisorDossier(TEXT("PROTOCOL LAB | 4 RULE POLICIES"),
        Lines, false, false, true);
    bAdvisorDossierOpen = true;
    UE_LOG(LogTemp, Display,
        TEXT("[AI HEROES LAB] OPEN verified=%d variants=%d history_version=%d no_order=1"),
        bVerified ? 1 : 0,
        Report.Scenarios.Num(),
        bVerified ? Report.RuleVersion : 0);
}
