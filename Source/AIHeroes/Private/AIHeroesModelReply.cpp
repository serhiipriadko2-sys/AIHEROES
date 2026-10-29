#include "AIHeroesModelReply.h"

#include "Misc/Guid.h"

namespace AIHeroesModelReplyDetail
{
FString NewToken() { return FGuid::NewGuid().ToString(EGuidFormats::Digits).ToLower(); }
bool SafeClock(double Value) { return FMath::IsFinite(Value) && Value >= 0.0; }
FAIHeroesModelReplyResult Rejected(const TCHAR* Why, bool Consumed = false)
{
    FAIHeroesModelReplyResult R;
    R.Reason = Why; R.bConsumed = Consumed; R.Verdict.Reason = Why;
    return R;
}
}

FAIHeroesModelReplySession::FAIHeroesModelReplySession() { Reset(); }
void FAIHeroesModelReplySession::Cancel()
{
    bPending = false;
    RequestId.Empty(); ExpectedJson.Empty(); ExpectedHostContext.Empty(); Menu.Reset();
    StartedAt = 0.0; Deadline = 0.0;
}
void FAIHeroesModelReplySession::Reset()
{
    Cancel();
    SessionId = AIHeroesModelReplyDetail::NewToken();
}

bool FAIHeroesModelReplySession::Begin(const FAIHeroesProposalSnapshot& S,
    const FString& HostContext, double Now, double Lifetime, FString& OutJson, FString& Error)
{
    using namespace AIHeroesModelReplyDetail;
    Cancel(); // A newer attempt supersedes the earlier one, even on failure.
    OutJson.Empty();
    Error = TEXT("MODEL REPLY: invalid clock, context or locked strategy phase.");
    if (!SafeClock(Now) || !FMath::IsFinite(Lifetime) || Lifetime <= 0.0 ||
        Lifetime > MaxLifetimeSeconds || !FMath::IsFinite(Now + Lifetime) ||
        Now + Lifetime <= Now || HostContext.IsEmpty() || HostContext.Len() > 65536 ||
        S.bBattleActive || S.Strategic.ProtocolTier < 1) return false;
    const FString NewRequest = NewToken();
    FString Json;
    TArray<FAIHeroesAIProposal> NewMenu;
    if (!FAIHeroesModelRequest::Build(S, NewRequest, SessionId,
        EAIHeroesModelGoal::AssessSelected, Json, Error, &NewMenu)) return false;
    if (NewMenu.Num() < 2 || NewMenu.Num() > 66)
    { Error = TEXT("MODEL REPLY: invalid engine-owned menu."); return false; }
    RequestId = NewRequest; ExpectedJson = Json; ExpectedHostContext = HostContext;
    Menu = MoveTemp(NewMenu); StartedAt = Now; Deadline = Now + Lifetime;
    bPending = true; OutJson = MoveTemp(Json); Error.Empty();
    return true;
}

bool FAIHeroesModelReplySession::DecodeSelection(
    const FString& Json, FString& OutRequest, int32& OutIndex)
{
    OutRequest.Empty(); OutIndex = INDEX_NONE;
    if (Json.IsEmpty() || Json.Len() > MaxReplyChars) return false;
    // This intentionally is NOT a general JSON parser. The wire grammar is a
    // flat two-string object; escapes, nested values and duplicate keys fail.
    int32 At = 0;
    auto Space = [&]()
    {
        while (At < Json.Len() && (Json[At] == ' ' || Json[At] == '\t' ||
            Json[At] == '\r' || Json[At] == '\n')) ++At;
    };
    auto Take = [&](TCHAR C)
    {
        Space();
        if (At >= Json.Len() || Json[At] != C) return false;
        ++At; return true;
    };
    auto String = [&](FString& Out)
    {
        Out.Empty();
        if (!Take('"')) return false;
        while (At < Json.Len() && Json[At] != '"')
        {
            const TCHAR C = Json[At++];
            if (!((C >= 'a' && C <= 'z') || (C >= '0' && C <= '9') || C == '_')) return false;
            Out.AppendChar(C);
        }
        if (At >= Json.Len()) return false;
        ++At; return true;
    };
    FString Request, Candidate;
    bool SeenRequest = false, SeenCandidate = false;
    if (!Take('{')) return false;
    for (int32 I = 0; I < 2; ++I)
    {
        if (I && !Take(',')) return false;
        FString Key, Value;
        if (!String(Key) || !Take(':') || !String(Value)) return false;
        if (Key == TEXT("request_id") && !SeenRequest)
        { SeenRequest = true; Request = Value; }
        else if (Key == TEXT("candidate_id") && !SeenCandidate)
        { SeenCandidate = true; Candidate = Value; }
        else return false;
    }
    if (!Take('}')) return false;
    Space();
    if (At != Json.Len() || !SeenRequest || !SeenCandidate || Request.Len() != 32) return false;
    for (TCHAR C : Request)
        if (!((C >= 'a' && C <= 'f') || (C >= '0' && C <= '9'))) return false;
    if (Candidate.Len() < 2 || Candidate.Len() > 3 || Candidate[0] != 'c' ||
        (Candidate.Len() == 3 && Candidate[1] == '0')) return false;
    int32 Index = 0;
    for (int32 I = 1; I < Candidate.Len(); ++I)
    {
        if (Candidate[I] < '0' || Candidate[I] > '9') return false;
        Index = Index * 10 + Candidate[I] - '0';
    }
    if (Index > 65) return false;
    OutRequest = MoveTemp(Request); OutIndex = Index;
    return true;
}

FAIHeroesModelReplyResult FAIHeroesModelReplySession::Complete(
    const FAIHeroesProposalSnapshot& Current, const FString& CurrentHostContext,
    const FString& SelectionJson, double Now)
{
    using namespace AIHeroesModelReplyDetail;
    if (!bPending) return Rejected(TEXT("NO_PENDING_REQUEST"));
    if (!SafeClock(Now) || Now < StartedAt || Now >= Deadline)
    { Cancel(); return Rejected(TEXT("EXPIRED_OR_INVALID_CLOCK"), true); }
    FString ReplyRequest;
    int32 Choice = INDEX_NONE;
    if (!DecodeSelection(SelectionJson, ReplyRequest, Choice))
        return Rejected(TEXT("MALFORMED_SELECTION"));
    if (!ReplyRequest.Equals(RequestId, ESearchCase::CaseSensitive))
        return Rejected(TEXT("REQUEST_MISMATCH")); // A late old reply cannot cancel a newer request.

    // From this point the matching request is single-use, including failures.
    FString CurrentJson, Error;
    if (Current.bBattleActive || Current.Strategic.ProtocolTier < 1 ||
        !CurrentHostContext.Equals(ExpectedHostContext, ESearchCase::CaseSensitive) ||
        !FAIHeroesModelRequest::Build(Current, RequestId, SessionId,
            EAIHeroesModelGoal::AssessSelected, CurrentJson, Error) ||
        !CurrentJson.Equals(ExpectedJson, ESearchCase::CaseSensitive))
    { Cancel(); return Rejected(TEXT("STALE_CONTEXT"), true); }
    if (!Menu.IsValidIndex(Choice))
    { Cancel(); return Rejected(TEXT("UNKNOWN_CANDIDATE"), true); }
    FAIHeroesAIProposal P = Menu[Choice];
    P.Source = EAIHeroesProposalSource::LocalModel;
    P.ClaimedModelSlotTier = Current.ModelSlotTier;
    Cancel();
    FAIHeroesModelReplyResult R;
    R.bChoiceValid = true; R.bConsumed = true;
    R.Verdict = FAIHeroesAIProposalGate::Validate(Current, P);
    R.Reason = R.Verdict.Reason;
    // No rejected payload can be accidentally displayed as an accepted proposal.
    if (R.Verdict.bAccepted) R.Proposal = MoveTemp(P);
    return R;
}
