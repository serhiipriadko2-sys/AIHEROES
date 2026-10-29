#pragma once

#include "CoreMinimal.h"
#include "AIHeroesModelRequest.h"

// A decoded choice is not permission to mutate the campaign.
struct FAIHeroesModelReplyResult
{
    bool bChoiceValid = false;
    bool bConsumed = false;
    FAIHeroesAIProposal Proposal;
    FAIHeroesProposalVerdict Verdict;
    FString Reason;
};

// Engine-owned, transient, single-flight correlation state. No network/files,
// actor access, persistence or execution method. Use only on the game thread.
class AIHEROES_API FAIHeroesModelReplySession
{
public:
    static constexpr int32 MaxReplyChars = 256;
    static constexpr double MaxLifetimeSeconds = 30.0;
    FAIHeroesModelReplySession();
    void Reset(); // New campaign/load: invalidate every earlier session reply.
    void Cancel();
    bool IsPending() const { return bPending; }
    const FString& GetRequestId() const { return RequestId; }
    const FString& GetSessionId() const { return SessionId; }
    bool Begin(const FAIHeroesProposalSnapshot& Snapshot, const FString& HostContext,
        double Now, double Lifetime, FString& OutJson, FString& Error);
    FAIHeroesModelReplyResult Complete(const FAIHeroesProposalSnapshot& Current,
        const FString& CurrentHostContext, const FString& SelectionJson, double Now);
    // Restricted inner response object: two unescaped ASCII string fields only.
    // Transport must separately verify HTTP/model envelope/finish_reason.
    static bool DecodeSelection(const FString& Json, FString& OutRequest, int32& OutIndex);

private:
    bool bPending = false;
    FString SessionId, RequestId, ExpectedJson, ExpectedHostContext;
    TArray<FAIHeroesAIProposal> Menu;
    double StartedAt = 0.0, Deadline = 0.0;
};
