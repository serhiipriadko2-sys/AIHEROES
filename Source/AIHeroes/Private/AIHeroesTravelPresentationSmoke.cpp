#include "AIHeroesGameMode.h"

#if !UE_BUILD_SHIPPING
#include "AIHeroesCampaignGuide.h"
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
        TWeakObjectPtr<AAIHeroesWorldNode> Forest, HQ, Ore;
        TArray<FAIHeroesArmyStack> SavedArmy;
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
                    if (Node->GetDisplayName() == TEXT("ORE")) S->Ore = Node;
                }
                if (!S->Forest.IsValid() || !S->HQ.IsValid() || !S->Ore.IsValid())
                    return Fail(TEXT("fixture sites missing"));
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
                G->SelectNode(S->HQ.Get());
                S->CommittedFingerprint = G->CampaignFingerprint();
                G->ToggleBasePanel();
                if (!G->IsBasePanelVisible() || G->CampaignFingerprint() != S->CommittedFingerprint)
                    return Fail(TEXT("opening base changed campaign or base not visible"));
                S->Due = S->Elapsed + 0.4f; S->Phase = 8; break;
            case 8:
                Capture(TEXT("VIEW_BASE.png")); S->Phase = 9; break;
            case 9:
            {
                G->CloseBasePanel();
                // Deliberate disposable presentation fixtures, not evidence
                // of campaign balance, recruitment or a human playthrough.
                S->SavedArmy = G->PlayerArmy;
                G->PlayerArmy.Reset(); G->MovesLeft = 4;
                G->SelectNode(S->Ore.Get());
                const auto Path = G->GetSelectedTravelPath();
                if (!Path.IsReachableThisTurn() || !Path.bBattleAtDestination ||
                    FAIHeroesCampaignGuide::CanOfferOrder(true, false, true))
                    return Fail(TEXT("empty-army guarded display verdict is not blocked"));
                S->CommittedFingerprint = G->CampaignFingerprint();
                G->ConfirmSelectedNode(); // Keyboard/click bypass still hits authority.
                if (G->IsLiveBattleActive() || G->CampaignFingerprint() != S->CommittedFingerprint)
                    return Fail(TEXT("empty-army guarded confirmation mutated campaign"));
                S->Due = S->Elapsed + 0.4f; S->Phase = 10; break;
            }
            case 10:
                Capture(TEXT("VIEW_EMPTY_ARMY_GUARDED.png")); S->Phase = 11; break;
            case 11:
            {
                S->Ore->Defenders.Reset(); // Safe-visit fixture only, never a user save.
                const auto Path = G->GetSelectedTravelPath();
                if (!Path.IsReachableThisTurn() || Path.bBattleAtDestination ||
                    !FAIHeroesCampaignGuide::CanOfferOrder(true, false, false))
                    return Fail(TEXT("empty army cannot display a legal safe visit"));
                S->CommittedFingerprint = G->CampaignFingerprint();
                S->Due = S->Elapsed + 0.4f; S->Phase = 12; break;
            }
            case 12:
                if (G->CampaignFingerprint() != S->CommittedFingerprint)
                    return Fail(TEXT("safe-visit preview changed campaign before confirmation"));
                Capture(TEXT("VIEW_EMPTY_ARMY_SAFE.png")); S->Phase = 13; break;
            case 13:
                S->ExpectedMoves = G->MovesLeft - G->GetSelectedMoveCost();
                G->ConfirmSelectedNode();
                if (!S->Ore->bOwned || G->MovesLeft != S->ExpectedMoves || G->IsLiveBattleActive())
                    return Fail(TEXT("authority rejected safe visit or spent the wrong cost"));
                S->Phase = 14; break;
            case 14:
            {
                if (G->IsStrategicTravelMoving()) break;
                G->PlayerArmy = S->SavedArmy; // Restore fixture army for route guidance.
                G->EndTurn();
                AAIHeroesWorldNode* Outpost = nullptr;
                for (AAIHeroesWorldNode* Node : G->Nodes)
                    if (IsValid(Node) && Node->GetDisplayName() == TEXT("OUTPOST")) Outpost = Node;
                if (!Outpost) return Fail(TEXT("staged route fixture missing outpost"));
                G->SelectNode(Outpost);
                const auto Path = G->GetSelectedTravelPath();
                if (G->MovesLeft != 4 || Path.TotalCost != 5 ||
                    Path.Status != EAIHeroesAdventurePathStatus::NeedsMoreMoves)
                    return Fail(TEXT("expected five-move route from ore through headquarters"));
                S->CommittedFingerprint = G->CampaignFingerprint();
                const auto Stop = G->GetTravelPathTo(FName(TEXT("HQ")));
                if (!Stop.IsReachableThisTurn() || Stop.TotalCost != 2 ||
                    G->CampaignFingerprint() != S->CommittedFingerprint)
                    return Fail(TEXT("owned waypoint query changed campaign or is not reachable"));
                S->Due = S->Elapsed + 0.4f; S->Phase = 15; break;
            }
            case 15:
                Capture(TEXT("VIEW_STAGED_ROUTE.png")); S->Phase = 16; break;
            case 16:
                if (G->CampaignFingerprint() != S->CommittedFingerprint)
                    return Fail(TEXT("route guidance changed campaign without a player order"));
                for (int32 I = 0; I < S->Screens.Num(); ++I)
                    if (IFileManager::Get().FileSize(*S->Screens[I]) <= 0 ||
                        IFileManager::Get().GetTimeStamp(*S->Screens[I]) <= S->PreviousTimes[I])
                        return Fail(TEXT("fresh screenshot missing"));
                UE_LOG(LogTemp, Display, TEXT("[TRAVEL VIEW SMOKE] PASS ready_preview=1 moving_marker=1 over_budget=1 one_debit=1 no_campaign_write_by_animation=1 base_read_only=1 empty_army_guarded_blocked=1 empty_army_safe_visit=1 staged_route=1 fixture_not_balance_or_manual_qa=1 save_calls=0"));
                FPlatformMisc::RequestExitWithStatus(false, 0);
                return false;
            default: return Fail(TEXT("unknown state"));
        }
        return true;
    }));
#endif
}
