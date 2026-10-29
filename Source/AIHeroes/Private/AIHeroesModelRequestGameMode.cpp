#include "AIHeroesGameMode.h"

#include "AIHeroesModelRequest.h"
#include "AIHeroesWorldNode.h"
#include "HAL/PlatformTime.h"
#include "Misc/Crc.h"

bool AAIHeroesGameMode::BuildLocalModelSnapshot(FAIHeroesProposalSnapshot& Out) const
{
    Out = FAIHeroesProposalSnapshot();
    if (!IsInGameThread() || Nodes.Num() != 5 || IsBasePanelVisible() ||
        IsAdvisorDossierVisible() || IsStrategicTravelMoving()) return false;
    for (const AAIHeroesWorldNode* Node : Nodes) if (!IsValid(Node)) return false;
    Out.Strategic = BuildAdvisorSnapshot();
    Out.Day = Day;
    Out.StateRevision = FCrc::StrCrc32(*CampaignFingerprint());
    if (Out.StateRevision == 0) Out.StateRevision = 1;
    Out.ModelSlotTier = ModelSlotTier;
    Out.bBattleActive = IsLiveBattleActive() || IsBattleReplayActive();
    // No transport/checkpoint attestation exists yet. NEVER trust model payload
    // fields to turn these flags on. A request or valid choice is not readiness.
    Out.bLocalModelReady = false;
    Out.bRemoteModelReady = false;
    Out.bRemoteInferenceApproved = false;
    return true;
}

bool AAIHeroesGameMode::BuildLocalModelRequest(
    const FString& RequestId, const FString& SessionId,
    FString& OutJson, FString& Error) const
{
    OutJson.Empty();
    Error = TEXT("MODEL REQUEST: export requires a stable game-thread map.");
    FAIHeroesProposalSnapshot Snapshot;
    if (!BuildLocalModelSnapshot(Snapshot)) return false;
    return FAIHeroesModelRequest::Build(Snapshot, RequestId, SessionId,
        EAIHeroesModelGoal::AssessSelected, OutJson, Error);
}

bool AAIHeroesGameMode::BeginLocalModelAdvice(FString& OutJson, FString& Error)
{
    OutJson.Empty();
    Error = TEXT("MODEL REPLY: request requires a stable game-thread map.");
    if (!IsInGameThread()) return false;
    FAIHeroesProposalSnapshot Snapshot;
    if (!BuildLocalModelSnapshot(Snapshot)) { LocalModelReplies.Cancel(); return false; }
    return LocalModelReplies.Begin(Snapshot, CampaignFingerprint(),
        FPlatformTime::Seconds(), 5.0, OutJson, Error);
}

FAIHeroesModelReplyResult AAIHeroesGameMode::CompleteLocalModelAdvice(const FString& SelectionJson)
{
    FAIHeroesModelReplyResult Blocked;
    Blocked.Reason = TEXT("MODEL REPLY: current engine state unavailable.");
    Blocked.Verdict.Reason = Blocked.Reason;
    if (!IsInGameThread()) return Blocked;
    FAIHeroesProposalSnapshot Snapshot;
    if (!BuildLocalModelSnapshot(Snapshot))
    {
        Blocked.bConsumed = LocalModelReplies.IsPending();
        LocalModelReplies.Cancel(); return Blocked;
    }
    // No LastHint/UI/campaign changes here: return a verdict to a future host.
    // The normal advisor remains offline. No HTTP or response adapter is wired.
    return LocalModelReplies.Complete(Snapshot, CampaignFingerprint(),
        SelectionJson, FPlatformTime::Seconds());
}

void AAIHeroesGameMode::CancelLocalModelAdvice()
{
    if (IsInGameThread()) LocalModelReplies.Cancel();
}
