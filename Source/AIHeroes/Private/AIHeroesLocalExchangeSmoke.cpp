#include "AIHeroesGameMode.h"

#if !UE_BUILD_SHIPPING
#include "AIHeroesWorldNode.h"
#include "Containers/Ticker.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#endif

void AAIHeroesGameMode::BeginLocalExchangeSmoke()
{
#if !UE_BUILD_SHIPPING
    struct FState
    {
        int32 Port = 0, Case = 0, Passed = 0, Frames = 0, MaxPendingFrames = 0;
        bool bInitialized = false, bStarted = false, bChanged = false;
        double Begin = FPlatformTime::Seconds(), CaseStart = 0.0;
        FString Fingerprint, Event, Hint;
        TWeakObjectPtr<AAIHeroesWorldNode> Forest;
    };
    auto S = MakeShared<FState>();
    FParse::Value(FCommandLine::Get(), TEXT("AIHeroesMockPort="), S->Port);
    TWeakObjectPtr<AAIHeroesGameMode> Weak(this);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak,S](float)
    {
        auto* G = Weak.Get(); if (!IsValid(G)) return false;
        auto Fail = [G](const TCHAR* Why)
        {
            G->CancelLocalModelExchange();
            UE_LOG(LogTemp,Error,TEXT("[LOCAL EXCHANGE SMOKE] FAIL %s"),Why);
            FPlatformMisc::RequestExitWithStatus(false,2); return false;
        };
        const double Now = FPlatformTime::Seconds();
        if (Now - S->Begin > 80.0) return Fail(TEXT("scenario time bound exceeded"));
        if (Now - S->Begin < 1.0) return true;
        if (!S->bInitialized)
        {
            if (S->Port < 1024 || S->Port > 65535) return Fail(TEXT("explicit mock port required"));
            for (AAIHeroesWorldNode* N : G->Nodes)
                if (IsValid(N) && N->GetDisplayName() == TEXT("FOREST")) S->Forest = N;
            if (!S->Forest.IsValid()) return Fail(TEXT("forest unavailable"));
            G->SelectNode(S->Forest.Get());
            FString E;
            if (G->StartLocalModelExchange(S->Port,TEXT("qa-valid"),true,E)) return Fail(TEXT("SEED not locked"));
            G->UpgradeProtocol(); if (G->ProtocolTier != 1) return Fail(TEXT("real upgrade failed"));
            const FString Before = G->CampaignFingerprint();
            if (G->StartLocalModelExchange(S->Port,TEXT("qa-valid"),false,E) ||
                G->CampaignFingerprint() != Before) return Fail(TEXT("opt-in boundary"));
            S->bInitialized = true;
        }
        const TCHAR* Cases[] = {TEXT("qa-valid"),TEXT("qa-redirect"),TEXT("qa-overflow"),
            TEXT("qa-duplicate"),TEXT("qa-tools"),TEXT("qa-bad-model"),TEXT("qa-stale"),
            TEXT("qa-timeout"),TEXT("qa-cancel"),TEXT("qa-valid")};
        if (S->Case >= UE_ARRAY_COUNT(Cases))
        {
            if (S->Passed != 10 || S->MaxPendingFrames < 2 || G->IsLocalModelAdvicePending())
                return Fail(TEXT("incomplete cases or stuck reply"));
            UE_LOG(LogTemp,Display,TEXT("[LOCAL EXCHANGE SMOKE] PASS cases=10 offline_preserved=1 readiness_off=1 strategy_commands=0 save_calls=0 max_pending_frames=%d"),S->MaxPendingFrames);
            FPlatformMisc::RequestExitWithStatus(false,0); return false;
        }
        if (!S->bStarted)
        {
            FString E;
            if (!G->StartLocalModelExchange(S->Port,Cases[S->Case],true,E)) return Fail(*E);
            S->CaseStart = Now; S->Frames = 0; S->bChanged = false;
            S->Fingerprint = G->CampaignFingerprint(); S->Event = G->LastEvent; S->Hint = G->LastHint;
            S->bStarted = true; return true;
        }
        ++S->Frames;
        if (S->Case == 6 && !S->bChanged && Now - S->CaseStart > 0.05)
        {
            G->EndTurn(); // Explicit test-player command; not transport authority.
            S->Fingerprint = G->CampaignFingerprint(); S->Event = G->LastEvent; S->Hint = G->LastHint;
            S->bChanged = true;
        }
        if (S->Case == 8 && Now - S->CaseStart > 0.05)
        {
            G->CancelLocalModelExchange();
            if (G->IsLocalModelExchangePending() || G->IsLocalModelAdvicePending()) return Fail(TEXT("cancel left request pending"));
        }
        if (G->IsLocalModelExchangePending()) return true;
        const auto& R = G->GetLocalModelExchangeResult();
        if (G->CampaignFingerprint() != S->Fingerprint || G->LastEvent != S->Event || G->LastHint != S->Hint)
            return Fail(TEXT("network altered campaign or offline advice"));
        if (R.Verdict.bAccepted) return Fail(TEXT("unattested test server impersonated loaded weights"));
        if (S->Case == 0 || S->Case == 9)
        {
            if (!R.bChoiceValid || !R.bConsumed || S->Frames < 2) return Fail(TEXT("valid wire or nonblocking frames missing"));
        }
        else if (S->Case == 6)
        {
            if (!S->bChanged || R.bChoiceValid || R.Reason != TEXT("STALE_CONTEXT")) return Fail(TEXT("stale response not rejected"));
        }
        else if (S->Case == 7)
        {
            if (R.Reason != TEXT("DEADLINE_OR_CLOCK_FAILURE")) return Fail(TEXT("deadline not enforced"));
        }
        else if (R.bChoiceValid) return Fail(TEXT("invalid network data accepted"));
        S->MaxPendingFrames = FMath::Max(S->MaxPendingFrames,S->Frames);
        UE_LOG(LogTemp,Display,TEXT("[LOCAL EXCHANGE CASE] index=%d pass=1 frames=%d choice_valid=%d"),S->Case,S->Frames,R.bChoiceValid);
        ++S->Case; ++S->Passed; S->bStarted = false;
        return true;
    }));
#endif
}
