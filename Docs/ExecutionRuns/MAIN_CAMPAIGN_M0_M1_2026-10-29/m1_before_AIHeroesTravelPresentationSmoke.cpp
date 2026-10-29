#include "AIHeroesGameMode.h"

#if !UE_BUILD_SHIPPING
#include "AIHeroesTravelPresentation.h"
#include "AIHeroesWorldNode.h"
#include "Containers/Ticker.h"
#include "Engine/StaticMeshActor.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#endif

// Opt-in disposable game launch; no save/load commands and no human inputs.
void AAIHeroesGameMode::BeginTravelViewSmoke()
{
#if !UE_BUILD_SHIPPING
    struct FViewSmoke
    {
        int32 Phase = 0;
        float Elapsed = 0.f, Due = 2.f;
        TWeakObjectPtr<AAIHeroesWorldNode> Forest, HQ;
        FVector Origin;
        FString CommittedFingerprint;
        int32 ExpectedMoves = 0;
        TArray<FString> Screens;
        TArray<FDateTime> PreviousTimes;
    };
    auto S = MakeShared<FViewSmoke>();
    TWeakObjectPtr<AAIHeroesGameMode> WeakThis(this);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakThis,S](float Dt)
    {
        auto* G = WeakThis.Get();
        if (!IsValid(G)) return false;
        S->Elapsed += Dt;
        auto Fail = [](const TCHAR* Why)
        {
            UE_LOG(LogTemp, Error, TEXT("[TRAVEL VIEW SMOKE] FAIL %s"), Why);
            FPlatformMisc::RequestExitWithStatus(false, 2);
            return false;
        };
        auto Capture = [S](const TCHAR* Name)
        {
            const FString Dir = FPaths::ConvertRelativePathToFull(FPaths::Combine(
                FPaths::ProjectSavedDir(), TEXT("QA/TravelPresentation")));
            IFileManager::Get().MakeDirectory(*Dir, true);
            const FString Path = FPaths::Combine(Dir, Name);
            S->PreviousTimes.Add(IFileManager::Get().GetTimeStamp(*Path));
            S->Screens.Add(Path);
            FScreenshotRequest::RequestScreenshot(Path, false, false);
        };
        if (S->Elapsed > 120.f) return Fail(TEXT("bounded run expired"));
        if (S->Elapsed < S->Due || FScreenshotRequest::IsScreenshotRequested()) return true;
        switch (S->Phase)
        {
            case 0:
                for (AAIHeroesWorldNode* Node : G->Nodes)
                {
                    if (!IsValid(Node)) continue;
                    if (Node->GetDisplayName() == TEXT("FOREST")) S->Forest = Node;
                    if (Node->GetDisplayName() == TEXT("HQ")) S->HQ = Node;
                }
                if (!S->Forest.IsValid() || !S->HQ.IsValid()) return Fail(TEXT("fixture sites missing"));
                G->SelectNode(S->Forest.Get());
                if (!G->GetSelectedTravelPath().IsReachableThisTurn()) return Fail(TEXT("initial forest not reachable"));
                S->Due = S->Elapsed + 0.4f; S->Phase = 1; break;
            case 1:
                Capture(TEXT("VIEW_READY.png")); S->Phase = 2; break;
            case 2:
                G->ConfirmSelectedNode();
                if (!G->IsLiveBattleActive()) return Fail(TEXT("normal forest battle did not start"));
                G->FastFinishLiveBattle(); S->Phase = 3; break;
            case 3:
            {
                if (G->GetLiveBattlePhase() != EAIHeroesLiveBattlePhase::Result) break;
                G->ReturnFromLiveBattle();
                if (!S->Forest->bOwned) return Fail(TEXT("forest victory missing"));
                S->Origin = G->GetHeroStrategicLocation();
                G->SelectNode(S->HQ.Get());
                const int32 Cost = G->GetSelectedMoveCost();
                if (Cost != 2) return Fail(TEXT("return price changed"));
                S->ExpectedMoves = G->MovesLeft - Cost;
                G->ConfirmSelectedNode();
                if (!G->IsStrategicTravelMoving()) return Fail(TEXT("return is not animated"));
                S->CommittedFingerprint = G->CampaignFingerprint();
                S->Due = S->Elapsed + 0.3f; S->Phase = 4; break;
            }
            case 4:
                if (!G->IsStrategicTravelMoving() || !IsValid(G->HeroMarker.Get()) ||
                    G->MovesLeft != S->ExpectedMoves ||
                    !G->GetHeroStrategicLocation().Equals(S->HQ->GetActorLocation()) ||
                    G->HeroMarker->GetActorLocation().Equals(S->Origin + AAIHeroesTravelPresentation::MarkerOffset()))
                    return Fail(TEXT("mid-route view or atomic authority mismatch"));
                Capture(TEXT("VIEW_MOVING.png"));
                S->Due = S->Elapsed + 2.f; S->Phase = 5; break;
            case 5:
                if (G->IsStrategicTravelMoving() || G->CampaignFingerprint() != S->CommittedFingerprint ||
                    !G->HeroMarker->GetActorLocation().Equals(S->HQ->GetActorLocation() + AAIHeroesTravelPresentation::MarkerOffset()))
                    return Fail(TEXT("finished animation changed campaign or marker"));
                G->SelectNode(S->Forest.Get());
                if (G->GetSelectedTravelPath().Status != EAIHeroesAdventurePathStatus::NeedsMoreMoves)
                    return Fail(TEXT("expected over-budget preview"));
                S->Due = S->Elapsed + 0.4f; S->Phase = 6; break;
            case 6:
                Capture(TEXT("VIEW_OVER_BUDGET.png")); S->Phase = 7; break;
            case 7:
                for (int32 I = 0; I < S->Screens.Num(); ++I)
                    if (IFileManager::Get().FileSize(*S->Screens[I]) <= 0 ||
                        IFileManager::Get().GetTimeStamp(*S->Screens[I]) <= S->PreviousTimes[I])
                        return Fail(TEXT("fresh screenshot missing"));
                UE_LOG(LogTemp, Display, TEXT("[TRAVEL VIEW SMOKE] PASS ready_preview=1 moving_marker=1 over_budget=1 one_debit=1 no_campaign_write_by_animation=1 save_calls=0"));
                FPlatformMisc::RequestExitWithStatus(false, 0);
                return false;
            default: return Fail(TEXT("unknown state"));
        }
        return true;
    }));
#endif
}
