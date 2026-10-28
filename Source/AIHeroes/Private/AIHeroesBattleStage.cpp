#include "AIHeroesBattleStage.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextRenderActor.h"
#include "Engine/World.h"
#include "UObject/UObjectGlobals.h"

AAIHeroesBattleStage::AAIHeroesBattleStage()
{
    PrimaryActorTick.bCanEverTick = true;
    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("ReplayRoot"));
    RootComponent = SceneRoot;

    PlayerGlow = CreateDefaultSubobject<UPointLightComponent>(TEXT("PlayerGlow"));
    PlayerGlow->SetupAttachment(SceneRoot);
    PlayerGlow->SetRelativeLocation(FVector(-180.f, -170.f, 170.f));
    PlayerGlow->SetIntensity(2700.f);
    PlayerGlow->SetAttenuationRadius(570.f);
    PlayerGlow->SetLightColor(FLinearColor(0.2f, 0.65f, 1.f));

    EnemyGlow = CreateDefaultSubobject<UPointLightComponent>(TEXT("EnemyGlow"));
    EnemyGlow->SetupAttachment(SceneRoot);
    EnemyGlow->SetRelativeLocation(FVector(180.f, 170.f, 170.f));
    EnemyGlow->SetIntensity(2700.f);
    EnemyGlow->SetAttenuationRadius(570.f);
    EnemyGlow->SetLightColor(FLinearColor(1.f, 0.35f, 0.13f));
}

bool AAIHeroesBattleStage::Initialize(
    const FVector& InCenter,
    const TArray<FAIHeroesArmyStack>& PlayerInitial,
    const TArray<FAIHeroesArmyStack>& EnemyInitial,
    const FAIHeroesBattleOutcome& Outcome)
{
    if (bStarted || !GetWorld() ||
        !FAIHeroesBattleEngine::VerifyReplay(PlayerInitial, EnemyInitial, Outcome))
    {
        return false;
    }

    CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    SphereMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (!CubeMesh || !SphereMesh)
    {
        return false;
    }

    Center = InCenter;
    SetActorLocation(Center);
    PlayerState = PlayerInitial;
    EnemyState = EnemyInitial;
    Battle = Outcome;

    Arena = GetWorld()->SpawnActor<AStaticMeshActor>(
        Center + FVector(0.f, 0.f, -38.f), FRotator::ZeroRotator);
    if (!IsValid(Arena.Get()))
    {
        return false;
    }
    UStaticMeshComponent* ArenaMesh = Arena->GetStaticMeshComponent();
    ArenaMesh->SetMobility(EComponentMobility::Movable);
    ArenaMesh->SetStaticMesh(CubeMesh);
    Arena->SetActorScale3D(FVector(9.f, 7.2f, 0.17f));
    ArenaMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    for (int32 Index = 0; Index < PlayerState.Num(); ++Index)
    {
        SpawnFigure(true, Index);
    }
    for (int32 Index = 0; Index < EnemyState.Num(); ++Index)
    {
        SpawnFigure(false, Index);
    }

    bStarted = true;
    bFinished = false;
    UE_LOG(LogTemp, Display,
        TEXT("[AI HEROES REPLAY] START steps=%d player_stacks=%d enemy_stacks=%d"),
        Battle.Actions.Num(), PlayerState.Num(), EnemyState.Num());
    return true;
}

void AAIHeroesBattleStage::SpawnFigure(bool bPlayer, int32 Index)
{
    if (!GetWorld())
    {
        return;
    }
    TArray<TObjectPtr<AStaticMeshActor>>& Figures = bPlayer
        ? PlayerFigures : EnemyFigures;
    TArray<TObjectPtr<ATextRenderActor>>& Labels = bPlayer
        ? PlayerLabels : EnemyLabels;

    // Two small armies face each other across a physical 3D platform.
    const float X = bPlayer ? -185.f : 185.f;
    const float Y = (Index - (bPlayer ? (PlayerState.Num() - 1) :
        (EnemyState.Num() - 1)) * 0.5f) * 235.f;
    const FVector FigurePosition = Center + FVector(X, Y, 75.f);

    AStaticMeshActor* Figure = GetWorld()->SpawnActor<AStaticMeshActor>(
        FigurePosition, FRotator::ZeroRotator);
    Figures.Add(Figure);
    if (Figure)
    {
        UStaticMeshComponent* Mesh = Figure->GetStaticMeshComponent();
        Mesh->SetMobility(EComponentMobility::Movable);
        Mesh->SetStaticMesh(bPlayer ? SphereMesh : CubeMesh);
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    ATextRenderActor* Caption = GetWorld()->SpawnActor<ATextRenderActor>(
        FigurePosition + FVector(0.f, 0.f, 135.f),
        FRotator(0.f, 180.f, 0.f));
    Labels.Add(Caption);
    if (Caption)
    {
        UTextRenderComponent* Text = Caption->GetTextRender();
        Text->SetHorizontalAlignment(EHTA_Center);
        Text->SetWorldSize(32.f);
        Text->SetCastShadow(false);
        Text->SetTextRenderColor(bPlayer ? FColor(80, 235, 255) :
            FColor(255, 154, 75));
    }
    UpdateFigure(bPlayer, Index);
}

void AAIHeroesBattleStage::UpdateFigure(bool bPlayer, int32 Index)
{
    const TArray<FAIHeroesArmyStack>& Army = bPlayer ? PlayerState : EnemyState;
    const TArray<TObjectPtr<AStaticMeshActor>>& Figures =
        bPlayer ? PlayerFigures : EnemyFigures;
    const TArray<TObjectPtr<ATextRenderActor>>& Labels =
        bPlayer ? PlayerLabels : EnemyLabels;

    if (!Army.IsValidIndex(Index) || !Figures.IsValidIndex(Index))
    {
        return;
    }

    const FAIHeroesArmyStack& Stack = Army[Index];
    if (IsValid(Figures[Index].Get()))
    {
        AStaticMeshActor* Figure = Figures[Index].Get();
        Figure->SetActorHiddenInGame(!Stack.IsAlive());
        const float Height = FMath::Clamp(0.5f + Stack.Count() * 0.045f,
            0.5f, 1.55f);
        Figure->SetActorScale3D(FVector(0.78f, 0.78f, Height));
    }

    if (Labels.IsValidIndex(Index) && IsValid(Labels[Index].Get()))
    {
        Labels[Index]->GetTextRender()->SetText(FText::FromString(
            FString::Printf(TEXT("%s %s: %d [HP %d]"),
                bPlayer ? TEXT("HERO") : TEXT("NPC"),
                *Stack.UnitId.ToString(), Stack.Count(), Stack.TotalHealth)));
    }
}

void AAIHeroesBattleStage::ResetPulse()
{
    if (IsValid(PulsingFigure.Get()))
    {
        PulsingFigure->SetActorScale3D(PreviousPulseScale);
    }
    PulsingFigure = nullptr;
    PulseClock = 0.f;
}

void AAIHeroesBattleStage::ApplyNextAction()
{
    if (!Battle.Actions.IsValidIndex(DisplayedSteps))
    {
        return;
    }
    ResetPulse();
    const FAIHeroesBattleAction& Action = Battle.Actions[DisplayedSteps];

    TArray<FAIHeroesArmyStack>& Targets =
        Action.bPlayerAttacker ? EnemyState : PlayerState;
    const TArray<TObjectPtr<AStaticMeshActor>>& Attackers =
        Action.bPlayerAttacker ? PlayerFigures : EnemyFigures;
    if (!Targets.IsValidIndex(Action.TargetStackIndex))
    {
        Skip();
        return;
    }

    Targets[Action.TargetStackIndex].TotalHealth = Action.TargetHealthAfter;
    UpdateFigure(!Action.bPlayerAttacker, Action.TargetStackIndex);
    if (Attackers.IsValidIndex(Action.AttackerStackIndex) &&
        IsValid(Attackers[Action.AttackerStackIndex].Get()))
    {
        PulsingFigure = Attackers[Action.AttackerStackIndex];
        PreviousPulseScale = PulsingFigure->GetActorScale3D();
        PulsingFigure->SetActorScale3D(PreviousPulseScale * 1.24f);
    }

    ++DisplayedSteps;
    UE_LOG(LogTemp, Display, TEXT("[AI HEROES REPLAY] %d/%d %s"),
        DisplayedSteps, Battle.Actions.Num(), *Action.ToLogLine());

    if (DisplayedSteps >= MaxVisualSteps &&
        Battle.Actions.Num() > MaxVisualSteps)
    {
        // Long battles retain their FULL authoritative trace; playback is
        // bounded for mobile performance and player attention.
        ApplyFinalSnapshot();
    }
}

void AAIHeroesBattleStage::ApplyFinalSnapshot()
{
    PlayerState = Battle.PlayerAfter;
    EnemyState = Battle.EnemyAfter;
    for (int32 Index = 0; Index < PlayerState.Num(); ++Index)
    {
        UpdateFigure(true, Index);
    }
    for (int32 Index = 0; Index < EnemyState.Num(); ++Index)
    {
        UpdateFigure(false, Index);
    }
}

void AAIHeroesBattleStage::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bStarted || bFinished)
    {
        return;
    }

    if (PulsingFigure)
    {
        PulseClock += DeltaSeconds;
        if (PulseClock >= 0.24f)
        {
            ResetPulse();
        }
    }

    if (DisplayedSteps < FMath::Min(Battle.Actions.Num(), MaxVisualSteps))
    {
        StepClock += DeltaSeconds;
        if (StepClock >= 0.9f)
        {
            StepClock = 0.f;
            ApplyNextAction();
        }
        return;
    }

    HoldClock += DeltaSeconds;
    if (HoldClock >= 4.8f)
    {
        bFinished = true;
    }
}

FString AAIHeroesBattleStage::GetStatusLine() const
{
    if (!bStarted)
    {
        return TEXT("Battle replay unavailable.");
    }
    if (DisplayedSteps == 0)
    {
        return FString::Printf(
            TEXT("3D BATTLE REPLAY | %d tactical actions | press Esc/Enter to skip"),
            Battle.Actions.Num());
    }

    const FAIHeroesBattleAction& Last =
        Battle.Actions[FMath::Min(DisplayedSteps - 1, Battle.Actions.Num() - 1)];
    if (DisplayedSteps >= MaxVisualSteps && Battle.Actions.Num() > MaxVisualSteps)
    {
        return FString::Printf(
            TEXT("3D REPLAY %d/%d (abridged); FULL trace in log | %s"),
            DisplayedSteps, Battle.Actions.Num(), *Battle.Summary);
    }
    return FString::Printf(TEXT("3D REPLAY %d/%d | %s"),
        DisplayedSteps, Battle.Actions.Num(), *Last.ToLogLine());
}

FString AAIHeroesBattleStage::GetArmyStatus(bool bPlayer) const
{
    const TArray<FAIHeroesArmyStack>& Army = bPlayer ? PlayerState : EnemyState;
    FString Text = bPlayer ? TEXT("HERO | ") : TEXT("NPC | ");
    for (const FAIHeroesArmyStack& Stack : Army)
    {
        Text += FString::Printf(TEXT("%s: %d units / %d HP  "),
            *Stack.UnitId.ToString(), Stack.Count(), Stack.TotalHealth);
    }
    return Text;
}

void AAIHeroesBattleStage::Skip()
{
    if (!bStarted)
    {
        bFinished = true;
        return;
    }
    ResetPulse();
    ApplyFinalSnapshot();
    bFinished = true;
    UE_LOG(LogTemp, Display, TEXT("[AI HEROES REPLAY] SKIP (game outcome unchanged)"));
}

void AAIHeroesBattleStage::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ResetPulse();
    for (AStaticMeshActor* Actor : PlayerFigures)
    {
        if (IsValid(Actor)) Actor->Destroy();
    }
    for (AStaticMeshActor* Actor : EnemyFigures)
    {
        if (IsValid(Actor)) Actor->Destroy();
    }
    for (ATextRenderActor* Actor : PlayerLabels)
    {
        if (IsValid(Actor)) Actor->Destroy();
    }
    for (ATextRenderActor* Actor : EnemyLabels)
    {
        if (IsValid(Actor)) Actor->Destroy();
    }
    if (IsValid(Arena.Get()))
    {
        Arena->Destroy();
    }
    Super::EndPlay(EndPlayReason);
}
