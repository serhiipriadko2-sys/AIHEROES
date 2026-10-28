#pragma once

#include "CoreMinimal.h"
#include "AIHeroesAdvisor.h"

// Transient, read-only, engine-owned AI proposal contract.
// This is NOT a SaveGame object, LLM runtime, input command or execution API.
enum class EAIHeroesProposalKind : uint8
{
    Advice,
    ProposeOrder,
    NoAction
};

enum class EAIHeroesProposalOrder : uint8
{
    None,
    MoveToNode,
    AttackNode
};

enum class EAIHeroesProposalSource : uint8
{
    OfflineRules,
    LocalModel,
    RemoteModel
};

struct FAIHeroesProposalSnapshot
{
    FAIHeroesAdvisorSnapshot Strategic;
    int32 Day = 0;
    uint32 StateRevision = 0; // Opaque, per-session game-state token.
    int32 ModelSlotTier = 0;
    bool bBattleActive = false;
    bool bLocalModelReady = false;
    bool bRemoteModelReady = false;
    bool bRemoteInferenceApproved = false;
};

struct FAIHeroesAIProposal
{
    EAIHeroesProposalKind Kind = EAIHeroesProposalKind::NoAction;
    EAIHeroesProposalOrder Order = EAIHeroesProposalOrder::None;
    EAIHeroesProposalSource Source = EAIHeroesProposalSource::OfflineRules;
    int32 ObservedDay = 0;
    uint32 ObservedRevision = 0;
    int32 ClaimedModelSlotTier = INDEX_NONE;
    FName TargetNode = NAME_None;
    FString Explanation;
};

struct FAIHeroesProposalVerdict
{
    bool bAccepted = false;
    bool bPlayerConfirmationRequired = false;
    FString Reason;
};

// Validation gives a read-only verdict. There is intentionally NO method
// to execute proposals, select a node, move an army or start a battle.
class AIHEROES_API FAIHeroesAIProposalGate
{
public:
    static constexpr int32 MaxVisibleNodes = 64;
    static constexpr int32 MaxArmyStacks = 12;
    static constexpr int32 MaxExplanationChars = 512;

    // Small deterministic candidate for the selected node, not an action.
    // No model weights, network access, runtime prompts or generated commands.
    static FAIHeroesAIProposal SuggestSelected(
        const FAIHeroesProposalSnapshot& Snapshot);

    static FAIHeroesProposalVerdict Validate(
        const FAIHeroesProposalSnapshot& Snapshot,
        const FAIHeroesAIProposal& Proposal);

    static FString Describe(
        const FAIHeroesAIProposal& Proposal,
        const FAIHeroesProposalVerdict& Verdict);
};
