#include "AIHeroesGameMode.h"

#include "AIHeroesBattleEngine.h"
#include "AIHeroesBattleRecord.h"
#include "AIHeroesBattleStage.h"
#include "AIHeroesLiveBattleStage.h"
#include "AIHeroesWorldNode.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

void AAIHeroesGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    TickLocalModelExchange();
    TickStrategicTravelPresentation(DeltaSeconds);
    if (IsLiveBattleActive())
    {
        TickLiveBattle(DeltaSeconds);
        return;
    }
    if (IsValid(GridReplayStage.Get()))
    {
        TickGridReplay(DeltaSeconds);
        return;
    }
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

void AAIHeroesGameMode::TickGridReplay(float DeltaSeconds)
{
    if (!IsValid(GridReplayStage.Get()))
    {
        return;
    }
    if (GetWorld())
    {
        if (const APlayerController* PC = GetWorld()->GetFirstPlayerController())
        {
            const float Yaw = (PC->IsInputKeyDown(EKeys::E) ? 1.f : 0.f) -
                (PC->IsInputKeyDown(EKeys::Q) ? 1.f : 0.f);
            const float Zoom = (PC->IsInputKeyDown(EKeys::R) ? 1.f : 0.f) -
                (PC->IsInputKeyDown(EKeys::F) ? 1.f : 0.f);
            GridReplayStage->OrbitCamera(
                Yaw * DeltaSeconds * 65.f, Zoom * DeltaSeconds * 600.f);
        }
    }

    if (bGridReplayPaused || GridReplayStage->IsPresenting())
    {
        return;
    }
    if (GridReplay.Actions.IsValidIndex(GridReplayCursor))
    {
        const FAIHeroesGridAction& Action = GridReplay.Actions[GridReplayCursor];
        if (!GridReplayStage->PresentAction(Action))
        {
            FinishGridReplay(
                TEXT("3D grid replay stopped: presentation rejected a verified action."));
            return;
        }
        ++GridReplayCursor;
        UE_LOG(LogTemp, Display, TEXT("[AI HEROES GRID REPLAY] %d/%d %s"),
            GridReplayCursor, GridReplay.Actions.Num(), *Action.ToLogLine());
        return;
    }
    if (!bGridReplayResultShown)
    {
        GridReplayStage->ShowResult(GridReplay.Summary);
        bGridReplayResultShown = true;
        GridReplayHoldClock = 0.f;
        UE_LOG(LogTemp, Display, TEXT("[AI HEROES GRID REPLAY] RESULT %s"),
            *GridReplay.Summary);
        return;
    }

    GridReplayHoldClock += FMath::Max(0.f, DeltaSeconds);
    if (GridReplayHoldClock >= 4.5f)
    {
        FinishGridReplay(TEXT("3D grid replay complete. Campaign unchanged."));
    }
}

void AAIHeroesGameMode::FinishGridReplay(const FString& Reason)
{
    if (!IsValid(GridReplayStage.Get()))
    {
        return;
    }

    const bool bCampaignUnchanged =
        !PreGridReplayFingerprint.IsEmpty() &&
        PreGridReplayFingerprint == CampaignFingerprint();

    // Restore the player's original view target before destroying the
    // camera owned by the disposable replay stage.
    if (GetWorld())
    {
        if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
        {
            if (IsValid(PreGridReplayViewTarget.Get()))
            {
                PC->SetViewTarget(PreGridReplayViewTarget.Get());
            }
            else if (IsValid(PC->GetPawn()))
            {
                PC->SetViewTarget(PC->GetPawn());
            }
        }
    }
    GridReplayStage->Destroy();
    GridReplayStage = nullptr;
    GridReplay = FAIHeroesGridReplay();
    GridReplayCursor = 0;
    GridReplayHoldClock = 0.f;
    GridReplaySpeed = 1.f;
    bGridReplayPaused = false;
    bGridReplayResultShown = false;
    PreGridReplayViewTarget.Reset();
    PreGridReplayFingerprint.Empty();

    UE_LOG(LogTemp, Display,
        TEXT("[AI HEROES GRID REPLAY] FINISH camera_restored=1 campaign_unchanged=%d"),
        bCampaignUnchanged ? 1 : 0);
    SetEvent(bCampaignUnchanged ? Reason :
        TEXT("GRID REPLAY WARNING: campaign changed unexpectedly; inspect the game state."));
}

void AAIHeroesGameMode::ToggleGridReplayPause()
{
    if (!IsValid(GridReplayStage.Get()) || bGridReplayResultShown)
    {
        return;
    }
    bGridReplayPaused = !bGridReplayPaused;
    GridReplayStage->SetPresentationPaused(bGridReplayPaused);
    UE_LOG(LogTemp, Display, TEXT("[AI HEROES GRID REPLAY] %s"),
        bGridReplayPaused ? TEXT("PAUSED") : TEXT("RESUMED"));
}

void AAIHeroesGameMode::CycleGridReplaySpeed()
{
    if (!IsValid(GridReplayStage.Get()) || bGridReplayResultShown)
    {
        return;
    }
    GridReplaySpeed = GridReplaySpeed >= 4.f ? 1.f : GridReplaySpeed * 2.f;
    GridReplayStage->SetPresentationSpeed(GridReplaySpeed);
    UE_LOG(LogTemp, Display, TEXT("[AI HEROES GRID REPLAY] SPEED %.0fx"),
        GridReplaySpeed);
}

bool AAIHeroesGameMode::IsBattleReplayActive() const
{
    return IsValid(GridReplayStage.Get()) ||
        (IsValid(ActiveBattleStage.Get()) &&
            !ActiveBattleStage->IsFinished());
}

FString AAIHeroesGameMode::GetBattleReplayStatus() const
{
    if (IsValid(GridReplayStage.Get()))
    {
        return FString::Printf(TEXT("ИСТОРИЯ: награда не записана. GRID V2 3D REPLAY %d/%d | %s | SPACE pause T speed ESC skip"),
            GridReplayCursor, GridReplay.Actions.Num(),
            *(bGridReplayPaused ? TEXT("PAUSED | ") + GridReplayStage->GetStatusText()
                : GridReplayStage->GetStatusText()));
    }
    return IsBattleReplayActive()
        ? TEXT("ИСТОРИЯ: награда не записана. ")+ActiveBattleStage->GetStatusLine() : FString();
}

FString AAIHeroesGameMode::GetBattleReplayArmyStatus(bool bPlayer) const
{
    if (IsValid(GridReplayStage.Get()))
    {
        return GridReplayStage->GetArmyStatus(bPlayer);
    }
    return IsBattleReplayActive()
        ? ActiveBattleStage->GetArmyStatus(bPlayer) : FString();
}

void AAIHeroesGameMode::SkipBattleReplay()
{
    if (IsValid(GridReplayStage.Get()))
    {
        UE_LOG(LogTemp, Display, TEXT("[AI HEROES GRID REPLAY] SKIP requested"));
        FinishGridReplay(TEXT("3D grid replay skipped. Campaign unchanged."));
        return;
    }
    if (IsBattleReplayActive())
    {
        ActiveBattleStage->Skip();
        UE_LOG(LogTemp, Display, TEXT("[AI HEROES 3D] SKIP requested"));
    }
}

void AAIHeroesGameMode::PlayLastBattleIn3D()
{
    if (BlockStrategicAction()) return;
    if (IsValid(ActiveBattleStage.Get()) || IsValid(GridReplayStage.Get()))
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

    if (LastBattleRecord.RuleVersion == FAIHeroesGridBattle::RuleVersion)
    {
        FAIHeroesGridReplay Prepared;
        if (!FAIHeroesGridReplayRules::Build(LastBattleRecord, Prepared, Error))
        {
            SetEvent(TEXT("GRID V2 replay blocked: ") + Error);
            return;
        }

        AAIHeroesLiveBattleStage* Stage = GetWorld()->SpawnActor<AAIHeroesLiveBattleStage>(
            FVector(0.f, 0.f, 10000.f), FRotator::ZeroRotator);
        if (!IsValid(Stage) ||
            !Stage->Initialize(Prepared.PlayerBefore, Prepared.EnemyBefore,
                Prepared.PlayerCells, Prepared.EnemyCells) ||
            !IsValid(Stage->GetViewTarget()))
        {
            if (IsValid(Stage)) Stage->Destroy();
            SetEvent(TEXT("GRID V2 replay blocked: 3D presentation unavailable. Campaign unchanged."));
            return;
        }

        // The replay does not call StartLiveBattle/StepLiveBattle/CommitLiveBattle.
        // A complete validated V2 action transcript is already frozen above.
        PreGridReplayFingerprint = CampaignFingerprint();
        GridReplay = MoveTemp(Prepared);
        GridReplayCursor = 0;
        GridReplayHoldClock = 0.f;
        GridReplaySpeed = 1.f;
        bGridReplayPaused = false;
        bGridReplayResultShown = false;
        GridReplayStage = Stage;
        if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
        {
            PreGridReplayViewTarget = PC->GetViewTarget();
            PC->SetViewTargetWithBlend(Stage->GetViewTarget(), 0.2f);
        }

        SetEvent(FString::Printf(
            TEXT("3D GRID V2 REPLAY: %s, %d verified actions. Space pause, T speed, Esc skip."),
            *GridReplay.NodeId.ToString(), GridReplay.Actions.Num()));
        UE_LOG(LogTemp, Display,
            TEXT("[AI HEROES GRID REPLAY] START node=%s verified_actions=%d campaign_commit=0"),
            *GridReplay.NodeId.ToString(), GridReplay.Actions.Num());
        return;
    }
    if (LastBattleRecord.RuleVersion != 1)
    {
        SetEvent(TEXT("3D replay blocked: unsupported battle version."));
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
