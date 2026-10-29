#include "AIHeroesGameMode.h"

#if !UE_BUILD_SHIPPING
#include "AIHeroesWorldNode.h"
#include "Containers/Ticker.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#endif

// Development-only explicit launch. Real GameMode ticks/rendering, no save calls.
void AAIHeroesGameMode::BeginTravelSmoke()
{
#if !UE_BUILD_SHIPPING
    struct FRun { int32 Phase=0; float Time=0,Due=2; TArray<FString> Images; TArray<FDateTime> Before; };
    auto Run=MakeShared<FRun>();
    TWeakObjectPtr<AAIHeroesGameMode> Weak(this);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Weak,Run](float Dt)
    {
        auto* G=Weak.Get(); if(!IsValid(G)) return false;
        Run->Time+=Dt;
        auto Fail=[](const TCHAR* Why)
        {
            UE_LOG(LogTemp,Error,TEXT("[TRAVEL V2 SMOKE] FAIL %s"),Why);
            FPlatformMisc::RequestExitWithStatus(false,2); return false;
        };
        auto Capture=[Run](const TCHAR* Name)
        {
            const FString P=FPaths::ConvertRelativePathToFull(FPaths::Combine(
                FPaths::ProjectSavedDir(),TEXT("QA/AdventureTravelV2"),Name));
            IFileManager::Get().MakeDirectory(*FPaths::GetPath(P),true);
            Run->Before.Add(IFileManager::Get().GetTimeStamp(*P)); Run->Images.Add(P);
            FScreenshotRequest::RequestScreenshot(P,false,false);
        };
        auto Site=[G](const TCHAR* Id)->AAIHeroesWorldNode*
        {
            for(AAIHeroesWorldNode* N:G->Nodes) if(IsValid(N) && N->GetDisplayName()==Id) return N;
            return nullptr;
        };
        if(Run->Time>90) return Fail(TEXT("timeout"));
        if(Run->Time<Run->Due || FScreenshotRequest::IsScreenshotRequested()) return true;
        switch(Run->Phase)
        {
            case 0:
            {
                if(G->TravelRuleVersion!=2 || G->AdventureRoadActors.Num()!=2 ||
                    !G->IsNodeDiscovered(FName(TEXT("FOREST"))) ||
                    G->IsNodeDiscovered(FName(TEXT("OUTPOST"))) ||
                    G->IsNodeDiscovered(FName(TEXT("RUINS"))))
                    return Fail(TEXT("new-campaign fog and initial known roads mismatch"));
                auto* Target=Site(TEXT("OUTPOST")); if(!Target) return Fail(TEXT("outpost missing"));
                auto* PC=G->GetWorld()->GetFirstPlayerController();
                if(PC && PC->GetPawn()) PC->GetPawn()->SetActorLocation(FVector(-2350,0,2150));
                G->SelectNode(Target); const auto Before=G->CampaignFingerprint(); G->ConfirmSelectedNode();
                if(G->IsLiveBattleActive() || G->CampaignFingerprint()!=Before ||
                    G->GetSelectedTravelPath().Status!=EAIHeroesAdventurePathStatus::InvalidInput)
                    return Fail(TEXT("hidden target accepted or changed campaign"));
                G->AskAdvisor(); Run->Due=Run->Time+0.8f; ++Run->Phase; break;
            }
            case 1: Capture(TEXT("ROAD_V2_BLOCKED.png")); ++Run->Phase; break;
            case 2:
            {
                auto* Forest=Site(TEXT("FOREST")); if(!Forest) return Fail(TEXT("forest missing"));
                G->SelectNode(Forest);
                if(G->GetSelectedMoveCost()!=2) return Fail(TEXT("forest price mismatch"));
                G->ConfirmSelectedNode();
                if(!G->IsLiveBattleActive() || G->MovesLeft!=4) return Fail(TEXT("early commit"));
                G->FastFinishLiveBattle(); ++Run->Phase; break;
            }
            case 3:
                if(G->GetLiveBattlePhase()!=EAIHeroesLiveBattlePhase::Result) break;
                if(G->MovesLeft!=2 || G->BattleJournal.Num()!=1 || G->DataFragments!=2)
                    return Fail(TEXT("battle result/cost mismatch"));
                G->ReturnFromLiveBattle();
                if(!G->IsNodeDiscovered(FName(TEXT("OUTPOST"))) ||
                    G->IsNodeDiscovered(FName(TEXT("RUINS"))) ||
                    G->AdventureRoadActors.Num()!=3)
                    return Fail(TEXT("scouting after capture did not uncover the proper road"));
                G->SelectNode(Site(TEXT("OUTPOST")));
                if(G->GetSelectedMoveCost()!=1 || !G->GetSelectedTravelPath().IsReachableThisTurn())
                    return Fail(TEXT("capture did not open next road"));
                G->AskAdvisor(); Run->Due=Run->Time+0.8f; ++Run->Phase; break;
            case 4: Capture(TEXT("ROAD_V2_OPEN_AFTER_CAPTURE.png")); ++Run->Phase; break;
            case 5:
                for(int32 I=0;I<Run->Images.Num();++I)
                    if(IFileManager::Get().FileSize(*Run->Images[I])<=0 ||
                        IFileManager::Get().GetTimeStamp(*Run->Images[I])<=Run->Before[I])
                        return Fail(TEXT("fresh screenshot absent"));
                UE_LOG(LogTemp,Display,TEXT("[TRAVEL V2 SMOKE] PASS roads=2_then3 hidden_target_blocked=1 fog_reveal=1 forest_cost=2 next_cost=1 commit=1 save_calls=0"));
                FPlatformMisc::RequestExitWithStatus(false,0); return false;
            default: return Fail(TEXT("invalid phase"));
        }
        return true;
    }));
#endif
}
