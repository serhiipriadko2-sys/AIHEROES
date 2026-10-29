#include "AIHeroesGameMode.h"

#include "AIHeroesBattleRecord.h"
#include "AIHeroesBattleRewardRules.h"
#include "AIHeroesLiveBattleStage.h"
#include "AIHeroesWorldNode.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

namespace
{
void AppendArmy(FString& Text, const TArray<FAIHeroesArmyStack>& Army)
{
    Text += FString::Printf(TEXT("[%d]"), Army.Num());
    for (const FAIHeroesArmyStack& S : Army)
    {
        Text += FString::Printf(TEXT("%s:%d,%d,%d,%d,%d;"), *S.UnitId.ToString(),
            S.TotalHealth, S.UnitHealth, S.Attack, S.Defense, S.Initiative);
    }
}
}

bool AAIHeroesGameMode::BlockStrategicAction()
{
    // View-only travel never delays or repeats campaign authority.
    FinishStrategicTravelPresentation();
    if (IsBasePanelVisible())
    {
        SetEvent(TEXT("BASE PANEL OPEN: review and confirm a purchase, or close before other orders."));
        return true;
    }
    if (IsAdvisorDossierVisible())
    {
        SetEvent(TEXT("REPORT OPEN: close the read-only dossier before giving orders. Campaign unchanged."));
        return true;
    }
    if (!IsLiveBattleActive() && !IsBattleReplayActive()) return false;
    SetEvent(TEXT("STRATEGY LOCKED: finish the battle and return to the map first. No campaign changes."));
    return true;
}

FString AAIHeroesGameMode::CampaignFingerprint() const
{
    FString Text = FString::Printf(TEXT("%d/%d/%d/%d/%d/%d/%d/%d/%d/%s/%p/"),
        Day, Gold, Wood, Ore, MovesLeft, ProtocolTier, ModelSlotTier,
        DataFragments, ActiveSaveSlot, *HeroStrategicLocation.ToString(), SelectedNode.Get());
    Text += FString::Printf(TEXT("HQ=%d/STOCK=%d:%d/"),
        BaseTier, RecruitmentStock.Militia, RecruitmentStock.Scouts);
    Text += FString::Printf(TEXT("travel=%d/fog=%d:%d/FIELD=%d:%d:%d:%d/"),
        TravelRuleVersion, bFogEnabled ? 1 : 0, FogKnownMask,
        FieldDwellings.Forest.bClaimed,
        FieldDwellings.Forest.Available,
        FieldDwellings.Ore.bClaimed, FieldDwellings.Ore.Available);
    AppendArmy(Text, PlayerArmy);
    for (const AAIHeroesWorldNode* Node : Nodes)
    {
        if (!IsValid(Node)) return TEXT("INVALID MAP");
        Text += FString::Printf(TEXT("%p:%s:%d:%d:%s/"), Node,
            *Node->GetDisplayName(), static_cast<int32>(Node->NodeType),
            Node->bOwned, *Node->GetActorLocation().ToString());
        AppendArmy(Text, Node->Defenders);
    }
    // UI status is deliberately excluded; durable campaign evidence is not.
    Text += LastCombatStep + TEXT("|") + FString::Join(BattleJournal, TEXT("|"));
    Text += FString::Printf(TEXT("/%s/%d/%d/%d/"), *LastBattleRecord.NodeId.ToString(),
        LastBattleRecord.Day, LastBattleRecord.ProtocolTier, LastBattleRecord.RuleVersion);
    Text += LastBattleRecord.Summary + FString::Join(LastBattleRecord.Trace, TEXT("|"));
    AppendArmy(Text, LastBattleRecord.PlayerAfter);
    AppendArmy(Text, LastBattleRecord.EnemyAfter);
    return Text;
}

bool AAIHeroesGameMode::StartLiveBattle(AAIHeroesWorldNode* Node, int32 MoveCost)
{
    if (IsLiveBattleActive() || IsBattleReplayActive() || !IsValid(Node) || !GetWorld()) return false;
    FString Error;
    FAIHeroesGridSession Initial;
    if (!FAIHeroesGridBattle::Begin(PlayerArmy, Node->Defenders, ProtocolTier, Initial, Error))
    {
        SetEvent(TEXT("BATTLE BLOCKED: ") + Error);
        return false;
    }
    AAIHeroesLiveBattleStage* Stage = GetWorld()->SpawnActor<AAIHeroesLiveBattleStage>(
        FVector(0.f, 0.f, 10000.f), FRotator::ZeroRotator);
    if (!IsValid(Stage) || !Stage->Initialize(Initial.PlayerArmy, Initial.EnemyArmy,
        Initial.PlayerCells, Initial.EnemyCells))
    {
        if (IsValid(Stage)) Stage->Destroy();
        SetEvent(TEXT("BATTLE CANCELLED: arena unavailable. Nothing spent."));
        return false;
    }
    LiveBattleStage = Stage;
    LiveSession = MoveTemp(Initial);
    LiveBattleNode = Node;
    LivePlayerBefore = PlayerArmy;
    LiveEnemyBefore = Node->Defenders;
    LiveMoveCost = MoveCost;
    LiveDay = Day;
    LiveProtocolTier = ProtocolTier;
    PreBattleFingerprint = CampaignFingerprint();
    bLiveBattleCommitted = false;
    bFastFinishingLiveBattle = false;
    LiveBattleSpeed = 1.f;
    LiveResultText.Empty();
    LiveBattlePhase = EAIHeroesLiveBattlePhase::Running;
    InvalidateRoutePreview();
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        PreLiveViewTarget = PC->GetViewTarget();
        if (IsValid(Stage->GetViewTarget())) PC->SetViewTargetWithBlend(Stage->GetViewTarget(), 0.2f);
    }
    SetEvent(TEXT("LIVE GRID V2: hero chooses each next action. Campaign is unchanged until the result."));
    UE_LOG(LogTemp, Display, TEXT("[AI HEROES LIVE] BEGIN node=%s actions=0 campaign_commit=0"),
        *Node->GetDisplayName());
    return true;
}

void AAIHeroesGameMode::TickLiveBattle(float DeltaSeconds)
{
    if (IsValid(LiveBattleStage.Get()))
    {
        if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
        {
            const float Yaw = (PC->IsInputKeyDown(EKeys::E) ? 1.f : 0.f) -
                (PC->IsInputKeyDown(EKeys::Q) ? 1.f : 0.f);
            const float Zoom = (PC->IsInputKeyDown(EKeys::R) ? 1.f : 0.f) -
                (PC->IsInputKeyDown(EKeys::F) ? 1.f : 0.f);
            LiveBattleStage->OrbitCamera(Yaw * DeltaSeconds * 65.f, Zoom * DeltaSeconds * 600.f);
        }
    }
    if (LiveBattlePhase != EAIHeroesLiveBattlePhase::Running) return;
    if (IsValid(LiveBattleStage.Get()) && LiveBattleStage->IsPresenting()) return;

    // Even fast completion uses the same current session, bounded per frame.
    const int32 Budget = bFastFinishingLiveBattle ? 16 : 1;
    for (int32 Index = 0; Index < Budget; ++Index)
    {
        FAIHeroesGridAction Action;
        FString Error;
        const EAIHeroesGridStepResult Step = FAIHeroesGridBattle::Step(LiveSession, Action, Error);
        if (Step == EAIHeroesGridStepResult::Error)
        {
            AbortLiveBattle(Error);
            return;
        }
        if (Step == EAIHeroesGridStepResult::Completed)
        {
            CommitLiveBattle();
            return;
        }
        UE_LOG(LogTemp, Display, TEXT("[AI HEROES LIVE] %s"), *Action.ToLogLine());
        if (IsValid(LiveBattleStage.Get()))
        {
            if (!LiveBattleStage->PresentAction(Action))
            {
                // A visual failure never invents an outcome or writes campaign state.
                ClearLiveStage(false);
                SetEvent(TEXT("Arena visual unavailable: continuing validated actions in text."));
            }
            else if (bFastFinishingLiveBattle)
            {
                LiveBattleStage->FinishPresentationImmediately();
            }
        }
    }
}

void AAIHeroesGameMode::CommitLiveBattle()
{
    if (bLiveBattleCommitted || !LiveSession.bCompleted) return;
    AAIHeroesWorldNode* Node = LiveBattleNode.Get();
    if (!IsValid(Node) || CampaignFingerprint() != PreBattleFingerprint ||
        LiveMoveCost < 0 || MovesLeft < LiveMoveCost)
    {
        AbortLiveBattle(TEXT("Campaign changed during battle; no partial result applied."));
        return;
    }
    const FAIHeroesBattleOutcome Outcome = FAIHeroesGridBattle::GetOutcome(LiveSession);
    const FAIHeroesBattleRecord Record = FAIHeroesBattleRecordRules::Capture(
        FName(*Node->GetDisplayName()), LiveDay, LiveProtocolTier,
        LivePlayerBefore, LiveEnemyBefore, Outcome, FAIHeroesGridBattle::RuleVersion);
    FString Error;
    if (!FAIHeroesBattleRecordRules::Verify(Record, Error))
    {
        AbortLiveBattle(TEXT("Final record verification failed: ") + Error);
        return;
    }

    FAIHeroesBattleRewardPlan Rewards;
    if (!FAIHeroesBattleRewardRules::Plan(
        Outcome.bVictory, Node->NodeType, Gold, DataFragments, Rewards, Error))
    {
        AbortLiveBattle(TEXT("Reward preflight failed: ") + Error);
        return;
    }

    // Preflight is complete. No fallible operation below this point.
    bLiveBattleCommitted = true;
    Gold = Rewards.GoldAfter;
    DataFragments = Rewards.DataAfter;
    MovesLeft -= LiveMoveCost;
    PlayerArmy = Outcome.PlayerAfter;
    Node->Defenders = Outcome.EnemyAfter;
    LastBattleRecord = Record;
    ReplayCursor = INDEX_NONE;
    LastCombatStep = Outcome.Trace.IsEmpty() ? TEXT("No tactical actions.") : Outcome.Trace.Last();
    BattleJournal.Add(FString::Printf(TEXT("Day %d | %s | %s"), Day,
        *Node->GetDisplayName(), *Outcome.Summary));
    if (BattleJournal.Num() > 16) BattleJournal.RemoveAt(0);
    if (Outcome.bVictory)
    {
        Node->Defenders.Empty();
        Node->SetOwned(true);
        MoveHeroTo(Node->GetActorLocation());
        RevealAfterArrival(FName(*Node->GetDisplayName()));
    }
    InvalidateRoutePreview();
    LiveBattlePhase = EAIHeroesLiveBattlePhase::Result;
    LiveResultText = Outcome.Summary;
    if (IsValid(LiveBattleStage.Get())) LiveBattleStage->ShowResult(LiveResultText);
    SetEvent(FString::Printf(TEXT("LIVE RESULT: %s. Applied once; RETURN TO MAP to save."), *Outcome.Summary));
    UE_LOG(LogTemp, Display, TEXT("[AI HEROES LIVE] COMMIT node=%s actions=%d moves=%d data=%d"),
        *Node->GetDisplayName(), LiveSession.Actions.Num(), MovesLeft, DataFragments);
}

void AAIHeroesGameMode::ClearLiveStage(bool bReleaseViewTarget)
{
    if (GetWorld())
    {
        if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
        {
            if (IsValid(PreLiveViewTarget.Get())) PC->SetViewTarget(PreLiveViewTarget);
            else if (PC->GetPawn()) PC->SetViewTarget(PC->GetPawn());
        }
    }
    if (bReleaseViewTarget) PreLiveViewTarget = nullptr;
    if (IsValid(LiveBattleStage.Get())) LiveBattleStage->Destroy();
    LiveBattleStage = nullptr;
}

void AAIHeroesGameMode::AbortLiveBattle(const FString& Error)
{
    ClearLiveStage();
    LiveBattlePhase = EAIHeroesLiveBattlePhase::Exploration;
    bFastFinishingLiveBattle = false;
    LiveBattleNode.Reset();
    SetEvent(TEXT("BATTLE ABORTED: ") + Error);
}

void AAIHeroesGameMode::ToggleLiveBattlePause()
{
    if (LiveBattlePhase != EAIHeroesLiveBattlePhase::Running &&
        LiveBattlePhase != EAIHeroesLiveBattlePhase::Paused) return;
    LiveBattlePhase = LiveBattlePhase == EAIHeroesLiveBattlePhase::Running
        ? EAIHeroesLiveBattlePhase::Paused : EAIHeroesLiveBattlePhase::Running;
    if (IsValid(LiveBattleStage.Get())) LiveBattleStage->SetPresentationPaused(
        LiveBattlePhase == EAIHeroesLiveBattlePhase::Paused);
}

void AAIHeroesGameMode::CycleLiveBattleSpeed()
{
    if (!IsLiveBattleActive() || LiveBattlePhase == EAIHeroesLiveBattlePhase::Result) return;
    LiveBattleSpeed = LiveBattleSpeed >= 4.f ? 1.f : LiveBattleSpeed * 2.f;
    if (IsValid(LiveBattleStage.Get())) LiveBattleStage->SetPresentationSpeed(LiveBattleSpeed);
}

void AAIHeroesGameMode::FastFinishLiveBattle()
{
    if (!IsLiveBattleActive() || LiveBattlePhase == EAIHeroesLiveBattlePhase::Result) return;
    bFastFinishingLiveBattle = true;
    LiveBattlePhase = EAIHeroesLiveBattlePhase::Running;
    if (IsValid(LiveBattleStage.Get()))
    {
        LiveBattleStage->SetPresentationPaused(false);
        LiveBattleStage->FinishPresentationImmediately();
    }
}

void AAIHeroesGameMode::ReturnFromLiveBattle()
{
    if (LiveBattlePhase != EAIHeroesLiveBattlePhase::Result) return;
    ClearLiveStage();
    LiveBattlePhase = EAIHeroesLiveBattlePhase::Exploration;
    bFastFinishingLiveBattle = false;
    LiveBattleNode.Reset();
    SetEvent(TEXT("Returned to map. Battle result already applied once; K saves, L loads."));
    AnnounceCampaignVictoryIfEarned();
}

FString AAIHeroesGameMode::GetLiveBattleStatus() const
{
    if (LiveBattlePhase == EAIHeroesLiveBattlePhase::Result) return LiveResultText;
    return FString::Printf(TEXT("%s | ROUND %d/30 | %dx | %d actions | %s"),
        LiveBattlePhase == EAIHeroesLiveBattlePhase::Paused ? TEXT("PAUSED") : TEXT("LIVE GRID V2"),
        LiveSession.Round, FMath::RoundToInt(LiveBattleSpeed), LiveSession.Actions.Num(),
        LiveSession.Actions.IsEmpty() ? TEXT("No future outcome calculated") :
            *LiveSession.Actions.Last().ToLogLine());
}

FString AAIHeroesGameMode::GetLiveArmyStatus(bool bPlayer) const
{
    const TArray<FAIHeroesArmyStack>& Army = bPlayer ? LiveSession.PlayerArmy : LiveSession.EnemyArmy;
    FString Text;
    for (int32 I = 0; I < Army.Num(); ++I)
    {
        const FAIHeroesArmyStack& S = Army[I];
        Text += FString::Printf(TEXT("%s%s #%d: %d units / %d HP / INIT %d"), Text.IsEmpty() ? TEXT("") : TEXT(" | "),
            *S.UnitId.ToString(), I + 1, S.Count(), S.TotalHealth, S.Initiative);
    }
    return Text;
}
