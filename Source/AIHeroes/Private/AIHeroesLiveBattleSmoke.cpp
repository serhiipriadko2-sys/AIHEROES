#include "AIHeroesGameMode.h"

#if !UE_BUILD_SHIPPING
#include "AIHeroesLiveBattleStage.h"
#include "AIHeroesWorldNode.h"
#include "Containers/Ticker.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "GameFramework/PlayerController.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#endif

// Explicit development-only launch harness, not a gameplay capability.
// Uses the real live GameMode/actor ticks, no Windows input injection, no saves.
void AAIHeroesGameMode::BeginLiveBattleSmoke()
{
#if !UE_BUILD_SHIPPING
    struct FSmokeState
    {
        int32 Phase = 0;
        float Elapsed = 0.f;
        float Due = 2.f;
        int32 InitialMoves = 0;
        int32 InitialData = 0;
        int32 InitialHP = 0;
        TArray<FString> Screenshots;
        TArray<FDateTime> PreviousScreenshotTimes;
        TWeakObjectPtr<AActor> InitialViewTarget;
        FTransform InitialViewTransform;
        FString BeforeDossierFingerprint;
    };
    TSharedRef<FSmokeState> State = MakeShared<FSmokeState>();
    State->InitialMoves = MovesLeft;
    State->InitialData = DataFragments;
    State->InitialHP = PlayerArmy.IsEmpty() ? 0 : PlayerArmy[0].TotalHealth;
    TWeakObjectPtr<AAIHeroesGameMode> WeakThis(this);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
        [WeakThis, State](float DeltaSeconds)
    {
        AAIHeroesGameMode* G = WeakThis.Get();
        if (!IsValid(G)) return false;
        State->Elapsed += DeltaSeconds;
        auto Fail = [](const TCHAR* Why)
        {
            UE_LOG(LogTemp, Error, TEXT("[LIVE GRID SMOKE] FAIL %s"), Why);
            FPlatformMisc::RequestExitWithStatus(false, 2);
            return false;
        };
        auto Capture = [State](const TCHAR* Name)
        {
            const FString Path = FPaths::ConvertRelativePathToFull(
                FPaths::Combine(FPaths::ProjectDir(), TEXT("Docs"), Name));
            State->PreviousScreenshotTimes.Add(IFileManager::Get().GetTimeStamp(*Path));
            FScreenshotRequest::RequestScreenshot(Path, false, false);
            State->Screenshots.Add(Path);
            UE_LOG(LogTemp, Display, TEXT("[LIVE GRID SMOKE] SCREENSHOT %s"), *Path);
        };
        if (State->Elapsed > 120.f) return Fail(TEXT("timed out"));
        // Screenshot requests are deferred to viewport rendering. Never change
        // the state being attested until that request has actually been processed.
        if (State->Elapsed < State->Due || FScreenshotRequest::IsScreenshotRequested()) return true;
        switch (State->Phase)
        {
            case 0:
            {
                AAIHeroesWorldNode* Target = nullptr;
                for (AAIHeroesWorldNode* N : G->Nodes)
                    if (IsValid(N) && N->NodeType == EAIHeroesNodeType::ForestMine) Target = N;
                if (!Target) return Fail(TEXT("missing forest"));
                APlayerController* PC = G->GetWorld()->GetFirstPlayerController();
                if (!PC || !IsValid(PC->GetViewTarget())) return Fail(TEXT("missing initial camera"));
                State->InitialViewTarget = PC->GetViewTarget();
                State->InitialViewTransform = PC->GetViewTarget()->GetActorTransform();
                G->SelectNode(Target);
                G->ConfirmSelectedNode();
                if (!G->IsLiveBattleActive() || G->GetLiveBattleActionCount() != 0)
                    return Fail(TEXT("Begin precomputed actions or failed"));
                G->ToggleLiveBattlePause();
                State->Due = State->Elapsed + 1.5f;
                ++State->Phase;
                break;
            }
            case 1:
                Capture(TEXT("LIVE_GRID_V2_START.png"));
                if (G->MovesLeft != State->InitialMoves || G->DataFragments != State->InitialData)
                    return Fail(TEXT("early campaign mutation"));
                State->Phase = 10;
                break;
            case 10:
                G->ToggleLiveBattlePause();
                State->Phase = 2;
                break;
            case 2:
                if (G->GetLiveBattleActionCount() < 1) break;
                G->ToggleLiveBattlePause();
                State->Due = State->Elapsed + 0.8f;
                ++State->Phase;
                break;
            case 3:
                if (G->GetLiveBattlePhase() != EAIHeroesLiveBattlePhase::Paused ||
                    G->MovesLeft != State->InitialMoves || G->PlayerArmy[0].TotalHealth != State->InitialHP)
                    return Fail(TEXT("pause or campaign isolation"));
                Capture(TEXT("LIVE_GRID_V2_PAUSED.png"));
                State->Phase = 11;
                break;
            case 11:
                G->CycleLiveBattleSpeed();
                for (TActorIterator<AAIHeroesLiveBattleStage> It(G->GetWorld()); It; ++It)
                    It->OrbitCamera(28.f, -100.f);
                G->ToggleLiveBattlePause();
                State->Due = State->Elapsed + 1.f;
                State->Phase = 4;
                break;
            case 4:
                Capture(TEXT("LIVE_GRID_V2_2X.png"));
                State->Phase = 12;
                break;
            case 12:
                G->CycleLiveBattleSpeed();
                G->FastFinishLiveBattle();
                State->Phase = 5;
                break;
            case 5:
                if (G->GetLiveBattlePhase() != EAIHeroesLiveBattlePhase::Result) break;
                if (G->BattleJournal.Num() != 1 || G->LastBattleRecord.RuleVersion != 2 ||
                    G->DataFragments != State->InitialData + 2)
                    return Fail(TEXT("missing or repeated commit"));
                State->Due = State->Elapsed + 1.f;
                ++State->Phase;
                break;
            case 6:
                Capture(TEXT("LIVE_GRID_V2_RESULT.png"));
                State->Phase = 13;
                break;
            case 13:
                G->ReturnFromLiveBattle();
                State->Due = State->Elapsed + 1.f;
                State->Phase = 7;
                break;
            case 7:
                if (G->IsLiveBattleActive()) return Fail(TEXT("return failed"));
                if (APlayerController* PC = G->GetWorld()->GetFirstPlayerController();
                    !PC || PC->GetViewTarget() != State->InitialViewTarget.Get() ||
                    !IsValid(State->InitialViewTarget.Get()) ||
                    !State->InitialViewTarget->GetActorTransform().Equals(State->InitialViewTransform))
                    return Fail(TEXT("original strategic view was not restored exactly"));
                Capture(TEXT("LIVE_GRID_V2_RETURN.png"));
                State->Due = State->Elapsed + 1.f;
                ++State->Phase;
                break;
            case 8:
                State->BeforeDossierFingerprint = G->CampaignFingerprint();
                G->AskAdvisor();
                G->ToggleAdvisorDossier();
                if (!G->IsAdvisorDossierVisible()) return Fail(TEXT("dossier did not open"));
                G->EndTurn(); // The read-only modal must not authorize an order.
                if (G->CampaignFingerprint() != State->BeforeDossierFingerprint)
                    return Fail(TEXT("dossier changed campaign state"));
                State->Due = State->Elapsed + 0.3f;
                State->Phase = 20;
                break;
            case 20:
                Capture(TEXT("LIVE_GRID_V2_DOSSIER.png"));
                State->Phase = 21;
                break;
            case 21:
                G->CloseAdvisorDossier();
                for (int32 I = 0; I < State->Screenshots.Num(); ++I)
                {
                    const FString& Path = State->Screenshots[I];
                    if (IFileManager::Get().FileSize(*Path) <= 0 ||
                        IFileManager::Get().GetTimeStamp(*Path) <= State->PreviousScreenshotTimes[I])
                        return Fail(TEXT("fresh screenshot write failed"));
                }
                UE_LOG(LogTemp, Display, TEXT("[LIVE GRID SMOKE] PASS real actor ticks, pause, normal2x, fastfinish at4x, one commit, exact camera return, read-only dossier; no save writes"));
                FPlatformMisc::RequestExitWithStatus(false, 0);
                return false;
            default: return Fail(TEXT("unknown harness phase"));
        }
        return true;
    }));
#endif
}
