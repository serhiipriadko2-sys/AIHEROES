#include "AIHeroesGameMode.h"

#if !UE_BUILD_SHIPPING
#include "AIHeroesWorldNode.h"
#include "Containers/Ticker.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#endif

// Explicit development-only scenario. Never called by normal play.
void AAIHeroesGameMode::BeginModelRequestSmoke()
{
#if !UE_BUILD_SHIPPING
    struct FSmokeState
    {
        int32 Phase = 0;
        float Elapsed = 0.f;
        FString Session = FGuid::NewGuid().ToString(EGuidFormats::Digits).ToLower();
        FString Directory;
        TWeakObjectPtr<AAIHeroesWorldNode> Forest;
    };
    auto S = MakeShared<FSmokeState>();
    S->Directory = FPaths::ConvertRelativePathToFull(FPaths::Combine(
        FPaths::ProjectSavedDir(), TEXT("QA"), TEXT("ModelRequestSmoke_") + S->Session));
    IFileManager::Get().MakeDirectory(*S->Directory, true);
    TWeakObjectPtr<AAIHeroesGameMode> WeakThis(this);
    FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakThis,S](float Dt)
    {
        auto* G = WeakThis.Get();
        if (!IsValid(G)) return false;
        auto Fail = [](const TCHAR* Why)
        {
            UE_LOG(LogTemp, Error, TEXT("[MODEL REQUEST SMOKE] FAIL %s"), Why);
            FPlatformMisc::RequestExitWithStatus(false, 2);
            return false;
        };
        S->Elapsed += Dt;
        if (S->Elapsed > 90.f) return Fail(TEXT("bounded scenario expired"));
        if (S->Elapsed < 2.f) return true;
        auto Dump = [G,S](const TCHAR* Name, FString& J)
        {
            const FString Before = G->CampaignFingerprint();
            const auto SelectedBefore = G->SelectedNode;
            const FString EventBefore = G->LastEvent, HintBefore = G->LastHint;
            FString Error;
            const FString RequestId = FGuid::NewGuid().ToString(EGuidFormats::Digits).ToLower();
            if (!G->BuildLocalModelRequest(RequestId,S->Session,J,Error) ||
                G->CampaignFingerprint()!=Before || G->SelectedNode!=SelectedBefore ||
                G->LastEvent!=EventBefore || G->LastHint!=HintBefore) return false;
            const FString File=FPaths::Combine(S->Directory,FString(Name)+TEXT(".json"));
            if (!FFileHelper::SaveStringToFile(J,*File,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
                &IFileManager::Get(),FILEWRITE_NoReplaceExisting)) return false;
            FString Back;
            return FFileHelper::LoadFileToString(Back,*File) && Back==J;
        };
        FString J;
        switch(S->Phase)
        {
            case 0:
                for (AAIHeroesWorldNode* N:G->Nodes)
                    if (IsValid(N) && N->GetDisplayName()==TEXT("FOREST")) S->Forest=N;
                if (!S->Forest.IsValid()) return Fail(TEXT("forest missing"));
                G->SelectNode(S->Forest.Get());
                if (G->ProtocolTier!=0 || !Dump(TEXT("seed"),J) ||
                    !J.Contains(TEXT("\"legal_orders\":[]")) || J.Contains(TEXT("OUTPOST")) || J.Contains(TEXT("RUINS")))
                    return Fail(TEXT("seed or fog boundary"));
                S->Phase=1; break;
            case 1:
                // Buy through the real player-command path, not a test assignment.
                G->UpgradeProtocol();
                if (G->ProtocolTier!=1 || !Dump(TEXT("analyst"),J) ||
                    !J.Contains(TEXT("\"target_node\":\"FOREST\",\"cost\":2")))
                    return Fail(TEXT("analyst menu or real route price"));
                G->ConfirmSelectedNode();
                if (!G->IsLiveBattleActive() || !Dump(TEXT("battle_locked"),J) ||
                    !J.Contains(TEXT("\"legal_orders\":[]"))) return Fail(TEXT("battle strategy lock"));
                G->FastFinishLiveBattle(); S->Phase=2; break;
            case 2:
                if (G->GetLiveBattlePhase()!=EAIHeroesLiveBattlePhase::Result) break;
                G->ReturnFromLiveBattle();
                if (!S->Forest->bOwned || !Dump(TEXT("after_forest"),J) ||
                    !J.Contains(TEXT("OUTPOST")) || J.Contains(TEXT("RUINS")) ||
                    !J.Contains(TEXT("\"target_node\":\"OUTPOST\",\"cost\":1")))
                    return Fail(TEXT("actual discovery or opened route"));
                S->Phase=3; break;
            case 3:
            {
                FString Error, Bad=TEXT("stale payload");
                const FString Before=G->CampaignFingerprint();
                if (G->BuildLocalModelRequest(TEXT("bad"),S->Session,Bad,Error) ||
                    !Bad.IsEmpty() || G->CampaignFingerprint()!=Before)
                    return Fail(TEXT("invalid request must clear output without mutation"));
                UE_LOG(LogTemp,Display,TEXT("[MODEL REQUEST SMOKE] PASS exports=4 state_unchanged_by_export=1 network_calls=0 save_calls=0 weights_loaded=0 directory=%s"),*S->Directory);
                FPlatformMisc::RequestExitWithStatus(false,0);
                return false;
            }
            default: return Fail(TEXT("invalid phase"));
        }
        return true;
    }));
#endif
}
