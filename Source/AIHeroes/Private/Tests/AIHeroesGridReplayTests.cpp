#include "AIHeroesGridReplay.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AIHeroesBattleEngine.h"
#include "AIHeroesLiveBattleStage.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

namespace
{
TArray<FAIHeroesArmyStack> ReplayHeroes()
{
    return {
        FAIHeroesArmyStack(TEXT("Militia"), 8, 12, 4, 2, 3),
        FAIHeroesArmyStack(TEXT("Scout"), 3, 8, 6, 1, 7)
    };
}

TArray<FAIHeroesArmyStack> ReplayEnemies()
{
    return {
        FAIHeroesArmyStack(TEXT("Wolf"), 4, 9, 3, 1, 8),
        FAIHeroesArmyStack(TEXT("Raider"), 4, 11, 5, 2, 5)
    };
}

FAIHeroesBattleRecord MakeVerifiedGridRecord()
{
    const TArray<FAIHeroesArmyStack> Player = ReplayHeroes();
    const TArray<FAIHeroesArmyStack> Enemy = ReplayEnemies();
    const FAIHeroesBattleOutcome Outcome = FAIHeroesGridBattle::RunToEnd(Player, Enemy, 3);
    return FAIHeroesBattleRecordRules::Capture(
        FName(TEXT("FOREST")), 5, 3, Player, Enemy, Outcome,
        FAIHeroesGridBattle::RuleVersion);
}

struct FGridReplayTestWorld
{
    UWorld* World = nullptr;

    FGridReplayTestWorld()
    {
        const UWorld::InitializationValues Values = UWorld::InitializationValues()
            .AllowAudioPlayback(false).CreatePhysicsScene(false)
            .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
        World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr,
            true, ERHIFeatureLevel::Num, &Values);
        if (World && GEngine)
        {
            GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        }
    }

    ~FGridReplayTestWorld()
    {
        if (World)
        {
            World->DestroyWorld(false);
            if (GEngine) GEngine->DestroyWorldContext(World);
        }
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesGridReplayVerifyTest,
    "AIHeroes.Replay.GridV2VerifiedTypedParity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesGridReplayVerifyTest::RunTest(const FString& Parameters)
{
    const FAIHeroesBattleRecord Record = MakeVerifiedGridRecord();
    FString Error;
    if (!TestTrue(TEXT("Record is signed by deterministic V2 rules"),
        FAIHeroesBattleRecordRules::Verify(Record, Error))) return false;

    FAIHeroesGridReplay Replay;
    if (!TestTrue(TEXT("Reconstruct exactly the verified actions"),
        FAIHeroesGridReplayRules::Build(Record, Replay, Error))) return false;
    TestTrue(TEXT("Replay metadata is nonempty"), Replay.IsValid());
    TestEqual(TEXT("Replay source node is retained"), Replay.NodeId, Record.NodeId);
    TestEqual(TEXT("Every textual receipt has a typed grid action"),
        Replay.Actions.Num(), Record.Trace.Num());
    TestEqual(TEXT("Initial player cell count matches army"),
        Replay.PlayerCells.Num(), Record.PlayerBefore.Num());
    TestEqual(TEXT("Initial enemy cell count matches army"),
        Replay.EnemyCells.Num(), Record.EnemyBefore.Num());
    TestEqual(TEXT("Summary retains the original battle decision"),
        Replay.Summary, Record.Summary);

    for (int32 I = 0; I < Replay.Actions.Num(); ++I)
    {
        TestEqual(TEXT("Action-by-action transcript parity"),
            Replay.Actions[I].ToLogLine(), Record.Trace[I]);
    }

    FAIHeroesGridReplay Rebuilt;
    TestTrue(TEXT("Rebuild succeeds reproducibly"),
        FAIHeroesGridReplayRules::Build(Record, Rebuilt, Error));
    TestEqual(TEXT("Deterministic action count"),
        Replay.Actions.Num(), Rebuilt.Actions.Num());
    for (int32 I = 0; I < FMath::Min(Replay.Actions.Num(), Rebuilt.Actions.Num()); ++I)
    {
        TestEqual(TEXT("No drift across reconstructions"),
            Replay.Actions[I].ToLogLine(), Rebuilt.Actions[I].ToLogLine());
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesGridReplayFailClosedTest,
    "AIHeroes.Replay.GridV2RejectTamperAndLegacy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesGridReplayFailClosedTest::RunTest(const FString& Parameters)
{
    const FAIHeroesBattleRecord Baseline = MakeVerifiedGridRecord();
    FString Error;
    FAIHeroesGridReplay Output;
    if (!TestTrue(TEXT("Valid initial replay"),
        FAIHeroesGridReplayRules::Build(Baseline, Output, Error))) return false;

    FAIHeroesBattleRecord Tampered = Baseline;
    Tampered.Trace[0] += TEXT(" FORGED");
    TestFalse(TEXT("Forged action is refused"),
        FAIHeroesGridReplayRules::Build(Tampered, Output, Error));
    TestFalse(TEXT("Failed build clears previous good output"), Output.IsValid());
    TestTrue(TEXT("Failure includes an explicit reason"), !Error.IsEmpty());

    Tampered = Baseline;
    Tampered.PlayerAfter[0].TotalHealth += 1;
    TestFalse(TEXT("Forged survivor HP is refused"),
        FAIHeroesGridReplayRules::Build(Tampered, Output, Error));
    TestTrue(TEXT("No partial typed actions leak on a failed build"),
        Output.Actions.IsEmpty());

    // The historical V1 format supports ONE enemy group, whereas V2
    // supports two. Build a truly valid legacy fixture before asserting
    // that the new renderer refuses to reinterpret it as grid combat.
    TArray<FAIHeroesArmyStack> LegacyEnemy;
    LegacyEnemy.Add(Baseline.EnemyBefore[0]);
    const FAIHeroesBattleOutcome Legacy = FAIHeroesBattleEngine::Resolve(
        Baseline.PlayerBefore, LegacyEnemy, 3);
    const FAIHeroesBattleRecord V1 = FAIHeroesBattleRecordRules::Capture(
        FName(TEXT("FOREST")), 5, 3, Baseline.PlayerBefore, LegacyEnemy,
        Legacy, 1);
    TestTrue(TEXT("Old V1 record still verifies with its own rules"),
        FAIHeroesBattleRecordRules::Verify(V1, Error));
    TestFalse(TEXT("V1 belongs to the existing legacy 3D stage, not grid stage"),
        FAIHeroesGridReplayRules::Build(V1, Output, Error));
    TestFalse(TEXT("Legacy rejection cannot retain stale typed actions"),
        Output.IsValid());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesGridReplayStageTest,
    "AIHeroes.Replay.GridV2ReadOnly3DStage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesGridReplayStageTest::RunTest(const FString& Parameters)
{
    const FAIHeroesBattleRecord Record = MakeVerifiedGridRecord();
    FString Error;
    FAIHeroesGridReplay Replay;
    if (!TestTrue(TEXT("Stage test starts from verified complete transcript"),
        FAIHeroesGridReplayRules::Build(Record, Replay, Error))) return false;

    FGridReplayTestWorld Fixture;
    AAIHeroesLiveBattleStage* Stage = Fixture.World
        ? Fixture.World->SpawnActor<AAIHeroesLiveBattleStage>() : nullptr;
    if (!TestNotNull(TEXT("A disposable visual stage spawns"), Stage)) return false;
    if (!TestTrue(TEXT("Stage can initialize from immutable original armies and cells"),
        Stage->Initialize(Replay.PlayerBefore, Replay.EnemyBefore,
            Replay.PlayerCells, Replay.EnemyCells))) return false;
    TestNotNull(TEXT("Standalone replay has its own camera"),
        Stage->GetViewTarget());

    // Present exactly the same events, one by one. There is no GameMode
    // in this isolated world: the stage cannot purchase, capture or save.
    for (int32 I = 0; I < Replay.Actions.Num(); ++I)
    {
        if (!TestTrue(TEXT("Every verified event can be presented in 3D"),
            Stage->PresentAction(Replay.Actions[I]))) return false;
        Stage->FinishPresentationImmediately();
        TestFalse(TEXT("No event remains pending after completion"),
            Stage->IsPresenting());
    }
    Stage->ShowResult(Replay.Summary);
    TestTrue(TEXT("Stage finishes with the verified decision"),
        Stage->GetStatusText().Contains(Replay.Summary));
    TestFalse(TEXT("Final result cannot accept new actions"),
        Stage->PresentAction(Replay.Actions[0]));

    TestEqual(TEXT("Player initial save record not mutated by visualization"),
        Record.PlayerBefore[0].TotalHealth,
        Replay.PlayerBefore[0].TotalHealth);
    return true;
}
#endif
