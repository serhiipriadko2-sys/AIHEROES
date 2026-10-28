#include "AIHeroesGameMode.h"

#include "AIHeroesRecruitmentAdvisor.h"

void AAIHeroesGameMode::AskRecruitmentAdvisor()
{
    if (BlockStrategicAction() || IsBattleReplayActive()) return;
    const FAIHeroesRecruitAdvice Advice =
        FAIHeroesRecruitmentAdvisor::Evaluate(BuildAdvisorSnapshot());
    LastHint = Advice.Summary;
    RecordAdvisorDossier(TEXT("HERO RECRUITMENT REPORT"),
        {Advice.Summary, Advice.Detail}, true);
    SetEvent(Advice.Detail.IsEmpty() ? Advice.Summary : Advice.Detail);
    UE_LOG(LogTemp, Display, TEXT("[AI HEROES RECRUIT ADVICE] %s | %s"),
        *Advice.Summary, *Advice.Detail);
}
