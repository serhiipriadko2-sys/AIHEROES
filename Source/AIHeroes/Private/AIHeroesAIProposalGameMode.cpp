#include "AIHeroesGameMode.h"

#include "AIHeroesAIProposalGate.h"
#include "Misc/Crc.h"

// The real GameMode owns this transient snapshot. No untrusted output is
// allowed to choose the day, movement budget, model readiness or state token.
FString AAIHeroesGameMode::BuildAIProposalGateStatus() const
{
    FAIHeroesProposalSnapshot Snapshot;
    Snapshot.Strategic = BuildAdvisorSnapshot();
    Snapshot.Day = Day;
    Snapshot.StateRevision = FCrc::StrCrc32(*CampaignFingerprint());
    if (Snapshot.StateRevision == 0)
    {
        Snapshot.StateRevision = 1;
    }
    Snapshot.ModelSlotTier = ModelSlotTier;
    Snapshot.bBattleActive = IsLiveBattleActive() || IsBattleReplayActive();

    // Model tiers are fictional gameplay slots until separately evidenced.
    // Future inference adapters must set readiness only after load verification
    // and remote opt-in; this offline build never enables them.
    Snapshot.bLocalModelReady = false;
    Snapshot.bRemoteModelReady = false;
    Snapshot.bRemoteInferenceApproved = false;

    const FAIHeroesAIProposal Proposal =
        FAIHeroesAIProposalGate::SuggestSelected(Snapshot);
    const FAIHeroesProposalVerdict Verdict =
        FAIHeroesAIProposalGate::Validate(Snapshot, Proposal);

    UE_LOG(LogTemp, Display,
        TEXT("[AI HEROES PROPOSAL] accepted=%d requires_player=%d target=%s source=offline"),
        Verdict.bAccepted, Verdict.bPlayerConfirmationRequired,
        *Proposal.TargetNode.ToString());
    return FAIHeroesAIProposalGate::Describe(Proposal, Verdict);
}
