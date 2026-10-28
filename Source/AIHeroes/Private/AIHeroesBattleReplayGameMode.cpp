#include "AIHeroesGameMode.h"

#include "AIHeroesBattleEngine.h"
#include "AIHeroesBattleRecord.h"
#include "AIHeroesBattleStage.h"
#include "AIHeroesWorldNode.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

void AAIHeroesGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!IsValid(ActiveBattleStage.Get()) ||
        !ActiveBattleStage->IsFinished())
    {
        return;
    }

    // The scene is disposable; all strategic mutations happened in
    // the original, fully verified battle, not during this playback.
    if (bSavedReplayCamera && IsValid(ReplayCameraPawn.Get()))
    {
        ReplayCameraPawn->SetActorTransform(PreReplayCameraTransform);
    }
    bSavedReplayCamera = false;
    ReplayCameraPawn = nullptr;

    UE_LOG(LogTemp, Display,
        TEXT("[AI HEROES 3D] FINISH; camera restored; strategic state unchanged"));
    ActiveBattleStage->Destroy();
    ActiveBattleStage = nullptr;
    SetEvent(TEXT("3D replay finished. Campaign unchanged; strategic camera restored."));
}

bool AAIHeroesGameMode::IsBattleReplayActive() const
{
    return IsValid(ActiveBattleStage.Get()) &&
        !ActiveBattleStage->IsFinished();
}

FString AAIHeroesGameMode::GetBattleReplayStatus() const
{
    return IsBattleReplayActive()
        ? ActiveBattleStage->GetStatusLine()
        : FString();
}

FString AAIHeroesGameMode::GetBattleReplayArmyStatus(bool bPlayer) const
{
    return IsBattleReplayActive()
        ? ActiveBattleStage->GetArmyStatus(bPlayer)
        : FString();
}

void AAIHeroesGameMode::SkipBattleReplay()
{
    if (IsBattleReplayActive())
    {
        ActiveBattleStage->Skip();
        UE_LOG(LogTemp, Display, TEXT("[AI HEROES 3D] SKIP requested"));
    }
}

void AAIHeroesGameMode::PlayLastBattleIn3D()
{
    if (IsValid(ActiveBattleStage.Get()))
    {
        SetEvent(TEXT("3D replay already running or closing. Press Esc to skip."));
        return;
    }
    if (!GetWorld() || LastBattleRecord.NodeId.IsNone())
    {
        SetEvent(TEXT("No recorded battle yet. Capture a guarded node first."));
        return;
    }

    FString Error;
    if (!FAIHeroesBattleRecordRules::Verify(LastBattleRecord, Error))
    {
        SetEvent(FString::Printf(TEXT("3D replay rejected: %s"), *Error));
        return;
    }

    const FAIHeroesBattleOutcome VerifiedOutcome = FAIHeroesBattleEngine::Resolve(
        LastBattleRecord.PlayerBefore,
        LastBattleRecord.EnemyBefore,
        LastBattleRecord.ProtocolTier);
    if (!FAIHeroesBattleEngine::VerifyReplay(
        LastBattleRecord.PlayerBefore,
        LastBattleRecord.EnemyBefore,
        VerifiedOutcome) || VerifiedOutcome.Actions.IsEmpty())
    {
        SetEvent(TEXT("3D replay unavailable: no valid typed combat actions."));
        return;
    }

    FVector BattleLocation = FVector::ZeroVector;
    bool bFoundNode = false;
    for (AAIHeroesWorldNode* Node : Nodes)
    {
        if (IsValid(Node) &&
            FName(*Node->GetDisplayName()) == LastBattleRecord.NodeId)
        {
            BattleLocation = Node->GetActorLocation();
            bFoundNode = true;
            break;
        }
    }
    if (!bFoundNode)
    {
        SetEvent(TEXT("3D replay blocked: unknown battle location."));
        return;
    }

    const FVector StageCenter = BattleLocation + FVector(0.f, 0.f, 365.f);
    AAIHeroesBattleStage* Stage = GetWorld()->SpawnActor<AAIHeroesBattleStage>(
        StageCenter, FRotator::ZeroRotator);
    if (!IsValid(Stage) ||
        !Stage->Initialize(StageCenter,
            LastBattleRecord.PlayerBefore,
            LastBattleRecord.EnemyBefore, VerifiedOutcome))
    {
        if (IsValid(Stage))
        {
            Stage->Destroy();
        }
        SetEvent(TEXT("3D replay could not create the stage; battle data unchanged."));
        return;
    }

    ActiveBattleStage = Stage;
    if (APlayerController* Controller = GetWorld()->GetFirstPlayerController())
    {
        if (APawn* Pawn = Controller->GetPawn())
        {
            ReplayCameraPawn = Pawn;
            PreReplayCameraTransform = Pawn->GetActorTransform();
            bSavedReplayCamera = true;
            Pawn->SetActorLocation(StageCenter + FVector(-1040.f, 0.f, 930.f));
            Pawn->SetActorRotation(FRotator::ZeroRotator);
        }
    }

    SetEvent(FString::Printf(
        TEXT("3D replay of %s: %d tactical actions. Esc/Enter to skip."),
        *LastBattleRecord.NodeId.ToString(), VerifiedOutcome.Actions.Num()));
    UE_LOG(LogTemp, Display,
        TEXT("[AI HEROES 3D] START node=%s, verified_actions=%d"),
        *LastBattleRecord.NodeId.ToString(), VerifiedOutcome.Actions.Num());
}
