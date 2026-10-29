#include "AIHeroesGameMode.h"

#include "HAL/PlatformTime.h"

bool AAIHeroesGameMode::StartLocalModelExchange(int32 Port, const FString& Model,
    bool bExplicitLocalOptIn, FString& Error)
{
    if (!IsInGameThread()) { Error = TEXT("GAME_THREAD_REQUIRED"); return false; }
    CancelLocalModelExchange();
    if (!bExplicitLocalOptIn)
    { Error = TEXT("LOCAL_NETWORK_OPT_IN_REQUIRED"); return false; }
    FString Json;
    if (!BeginLocalModelAdvice(Json, Error)) return false;
    if (!LocalModelExchange.Start(Port, Model, Json, true, FPlatformTime::Seconds(), 4.0, Error))
    { LocalModelReplies.Cancel(); return false; }
    bLocalExchangeHandled = false;
    return true;
}

void AAIHeroesGameMode::CancelLocalModelExchange()
{
    if (!IsInGameThread()) return;
    LocalModelExchange.Cancel(); LocalModelReplies.Cancel();
    LocalExchangeResult = FAIHeroesModelReplyResult();
    bLocalExchangeHandled = true;
}

void AAIHeroesGameMode::TickLocalModelExchange()
{
    if (bLocalExchangeHandled) return;
    LocalModelExchange.Tick(FPlatformTime::Seconds());
    if (LocalModelExchange.IsPending()) return;
    bLocalExchangeHandled = true;
    if (LocalModelExchange.GetState() == EAIHeroesLocalExchangeState::Complete)
        LocalExchangeResult = CompleteLocalModelAdvice(LocalModelExchange.GetSelection());
    else
    {
        LocalModelReplies.Cancel();
        LocalExchangeResult = FAIHeroesModelReplyResult();
        LocalExchangeResult.Reason = LocalModelExchange.GetReason();
        LocalExchangeResult.Verdict.Reason = LocalExchangeResult.Reason;
    }
    // Intentionally no LastHint/LastEvent/dossier change. A working wire does
    // not prove checkpoint identity, and production readiness remains false.
    // Existing offline advice remains available throughout this optional call.
}
