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
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#endif

// Explicit development-only launch harness, not a gameplay capability.
// Uses the real live GameMode/actor ticks, no Windows input injection, no saves.
void AAIHeroesGameMode::BeginLiveBattleSmoke()
{
#if !UE_BUILD_SHIPPING
    FString Scenario = TEXT("Victory");
    FParse::Value(FCommandLine::Get(), TEXT("AIHeroesLiveSmokeScenario="), Scenario);
    if (Scenario != TEXT("Victory") && Scenario != TEXT("Defeat") && Scenario != TEXT("TwoGroups"))
    {
        // Reject the development fixture selector before touching any army or
        // starting a session. No unsupported value silently falls back to victory.
        UE_LOG(LogTemp, Error, TEXT("[LIVE GRID SMOKE] FAIL unknown scenario; use Victory, Defeat or TwoGroups"));
        FPlatformMisc::RequestExitWithStatus(false, 2);
        return;
    }
    struct FSmokeState
    {
        int32 Phase = 0;
        float Elapsed = 0.f;
        float Due = 2.f;
        int32 InitialMoves = 0;
        int32 InitialData = 0;
        int32 InitialGold = 0;
        int32 ExpectedMoveCost = 0;
        FString Scenario;
        FString ScreenshotPrefix;
        FString BeforeBattleFingerprint;
        FVector InitialHeroLocation;
        TWeakObjectPtr<AAIHeroesWorldNode> Target;
        TArray<FString> Screenshots;
        TArray<FDateTime> PreviousScreenshotTimes;
        TWeakObjectPtr<AActor> InitialViewTarget;
        FTransform InitialViewTransform;
        FString BeforeDossierFingerprint;
    };
    TSharedRef<FSmokeState> State = MakeShared<FSmokeState>();
    State->Scenario = Scenario;
    State->ScreenshotPrefix = Scenario == TEXT("Victory") ? TEXT("LIVE_GRID_V2") :
        Scenario == TEXT("Defeat") ? TEXT("LIVE_GRID_V2_DEFEAT") : TEXT("LIVE_GRID_V2_TWO_GROUPS");
    TWeakObjectPtr<AAIHeroesGameMode> WeakThis(this);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
        [WeakThis, State](float DeltaSeconds)
    {
        AAIHeroesGameMode* G = WeakThis.Get();
        if (!IsValid(G)) return false;
        State->Elapsed += DeltaSeconds;
        auto Fail = [State](const TCHAR* Why)
        {
            UE_LOG(LogTemp, Error, TEXT("[LIVE GRID SMOKE] FAIL scenario=%s %s"), *State->Scenario, Why);
            FPlatformMisc::RequestExitWithStatus(false, 2);
            return false;
        };
        auto Capture = [State](const TCHAR* Suffix)
        {
            const FString Name = State->ScreenshotPrefix + TEXT("_") + Suffix + TEXT(".png");
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
                // Explicit synthetic acceptance fixtures, only in this opt-in
                // non-shipping harness. No future action/outcome is calculated.
                // Apply them BEFORE recording invariants and calling Begin.
                if (State->Scenario == TEXT("Defeat"))
                {
                    G->PlayerArmy = {FAIHeroesArmyStack(TEXT("Militia"), 1, 12, 4, 2, 3)};
                }
                else if (State->Scenario == TEXT("TwoGroups"))
                {
                    G->PlayerArmy = {
                        FAIHeroesArmyStack(TEXT("Militia"), 20, 12, 4, 2, 3),
                        FAIHeroesArmyStack(TEXT("Scout"), 12, 8, 6, 1, 7)};
                    Target->Configure(EAIHeroesNodeType::ForestMine, TEXT("FOREST"), {
                        FAIHeroesArmyStack(TEXT("Raider"), 6, 11, 5, 2, 5),
                        FAIHeroesArmyStack(TEXT("Raider"), 4, 11, 5, 2, 5)});
                }
                G->SelectNode(Target);
                State->InitialViewTarget = PC->GetViewTarget();
                State->InitialViewTransform = PC->GetViewTarget()->GetActorTransform();
                const FAIHeroesAdventurePathResult Path = G->GetSelectedTravelPath();
                if (!Path.IsReachableThisTurn() || Path.TotalCost < 0)
                    return Fail(TEXT("fixture target is not reachable"));
                State->Target = Target;
                State->InitialMoves = G->MovesLeft;
                State->InitialData = G->DataFragments;
                State->InitialGold = G->Gold;
                State->ExpectedMoveCost = Path.TotalCost;
                State->InitialHeroLocation = G->GetHeroStrategicLocation();
                State->BeforeBattleFingerprint = G->CampaignFingerprint();
                UE_LOG(LogTemp, Display, TEXT("[LIVE GRID SMOKE] FIXTURE scenario=%s player_groups=%d enemy_groups=%d duplicate_enemy_ids=%d synthetic=%d"),
                    *State->Scenario, G->PlayerArmy.Num(), Target->Defenders.Num(),
                    State->Scenario == TEXT("TwoGroups") ? 1 : 0,
                    State->Scenario == TEXT("Victory") ? 0 : 1);
                G->ConfirmSelectedNode();
                if (!G->IsLiveBattleActive() || G->GetLiveBattleActionCount() != 0)
                    return Fail(TEXT("Begin precomputed actions or failed"));
                G->ToggleLiveBattlePause();
                State->Due = State->Elapsed + 1.5f;
                ++State->Phase;
                break;
            }
            case 1:
                Capture(TEXT("START"));
                if (G->CampaignFingerprint() != State->BeforeBattleFingerprint)
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
                    G->CampaignFingerprint() != State->BeforeBattleFingerprint)
                    return Fail(TEXT("pause or campaign isolation"));
                Capture(TEXT("PAUSED"));
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
                Capture(TEXT("2X"));
                State->Phase = 12;
                break;
            case 12:
                G->CycleLiveBattleSpeed();
                G->FastFinishLiveBattle();
                State->Phase = 5;
                break;
            case 5:
            {
                if (G->GetLiveBattlePhase() != EAIHeroesLiveBattlePhase::Result) break;
                const bool bExpectVictory = State->Scenario != TEXT("Defeat");
                AAIHeroesWorldNode* Target = State->Target.Get();
                if (G->BattleJournal.Num() != 1 || G->LastBattleRecord.RuleVersion != 2 ||
                    G->LastBattleRecord.bVictory != bExpectVictory ||
                    G->LastBattleRecord.bDefeat == bExpectVictory ||
                    G->DataFragments != State->InitialData + (bExpectVictory ? 2 : 0) ||
                    G->Gold != State->InitialGold ||
                    G->MovesLeft != State->InitialMoves - State->ExpectedMoveCost ||
                    !IsValid(Target) || Target->bOwned != bExpectVictory)
                    return Fail(TEXT("missing or repeated commit"));
                if (!bExpectVictory && !G->GetHeroStrategicLocation().Equals(State->InitialHeroLocation))
                    return Fail(TEXT("defeat moved the strategic hero"));
                if (State->Scenario == TEXT("TwoGroups") &&
                    (G->LastBattleRecord.EnemyBefore.Num() != 2 ||
                     G->LastBattleRecord.EnemyBefore[0].UnitId != FName(TEXT("Raider")) ||
                     G->LastBattleRecord.EnemyBefore[1].UnitId != FName(TEXT("Raider"))))
                    return Fail(TEXT("duplicate enemy formations were collapsed"));
                const FString Committed = G->CampaignFingerprint();
                G->CommitLiveBattle(); // A repeated completion callback is inert.
                G->FastFinishLiveBattle();
                if (G->CampaignFingerprint() != Committed ||
                    G->GetLiveBattlePhase() != EAIHeroesLiveBattlePhase::Result)
                    return Fail(TEXT("repeated completion changed campaign"));
                State->Due = State->Elapsed + 1.f;
                ++State->Phase;
                break;
            }
            case 6:
                Capture(TEXT("RESULT"));
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
                Capture(TEXT("RETURN"));
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
                Capture(TEXT("DOSSIER"));
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
                UE_LOG(LogTemp, Display, TEXT("[LIVE GRID SMOKE] PASS scenario=%s real actor ticks, pause, normal2x, fastfinish at4x, one commit, exact camera return, read-only dossier; no save writes"), *State->Scenario);
                FPlatformMisc::RequestExitWithStatus(false, 0);
                return false;
            default: return Fail(TEXT("unknown harness phase"));
        }
        return true;
    }));
#endif
}
