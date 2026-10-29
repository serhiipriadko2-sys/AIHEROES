#include "AIHeroesGameMode.h"

#include "AIHeroesModelRequest.h"
#include "AIHeroesWorldNode.h"
#include "Misc/Crc.h"

bool AAIHeroesGameMode::BuildLocalModelRequest(
    const FString& RequestId, const FString& SessionId,
    FString& OutJson, FString& Error) const
{
    OutJson.Empty();
    Error = TEXT("MODEL REQUEST: export requires a stable game-thread map.");
    if (!IsInGameThread() || Nodes.Num() != 5 || IsBasePanelVisible() ||
        IsAdvisorDossierVisible() || IsStrategicTravelMoving()) return false;
    for (const AAIHeroesWorldNode* Node : Nodes) if (!IsValid(Node)) return false;
    FAIHeroesProposalSnapshot Snapshot;
    Snapshot.Strategic = BuildAdvisorSnapshot();
    Snapshot.Day = Day;
    Snapshot.StateRevision = FCrc::StrCrc32(*CampaignFingerprint());
    if (Snapshot.StateRevision == 0) Snapshot.StateRevision = 1;
    Snapshot.ModelSlotTier = ModelSlotTier;
    Snapshot.bBattleActive = IsLiveBattleActive() || IsBattleReplayActive();
    // Readiness remains false. Building a request is not loading a checkpoint.
    return FAIHeroesModelRequest::Build(Snapshot, RequestId, SessionId,
        EAIHeroesModelGoal::AssessSelected, OutJson, Error);
}
