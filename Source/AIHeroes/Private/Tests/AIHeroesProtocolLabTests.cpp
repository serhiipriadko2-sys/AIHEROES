#include "AIHeroesProtocolLab.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AIHeroesBattleEngine.h"
#include "AIHeroesGridBattle.h"
#include "AIHeroesHUDLayout.h"
#include "Misc/AutomationTest.h"

namespace
{
TArray<FAIHeroesArmyStack> LabHeroes()
{
    return {
        FAIHeroesArmyStack(TEXT("Militia"), 8, 12, 4, 2, 3),
        FAIHeroesArmyStack(TEXT("Scout"), 3, 8, 6, 1, 7)
    };
}

TArray<FAIHeroesArmyStack> LabEnemies(bool bGrid)
{
    TArray<FAIHeroesArmyStack> Enemies;
    Enemies.Add(FAIHeroesArmyStack(TEXT("Wolf"), 4, 9, 3, 1, 8));
    if (bGrid)
    {
        Enemies.Add(FAIHeroesArmyStack(TEXT("Raider"), 4, 11, 5, 2, 5));
    }
    return Enemies;
}

FAIHeroesBattleRecord LabRecord(bool bGrid, int32 ActualTier)
{
    const TArray<FAIHeroesArmyStack> Player = LabHeroes();
    const TArray<FAIHeroesArmyStack> Enemies = LabEnemies(bGrid);
    const FAIHeroesBattleOutcome Outcome = bGrid
        ? FAIHeroesGridBattle::RunToEnd(Player, Enemies, ActualTier)
        : FAIHeroesBattleEngine::Resolve(Player, Enemies, ActualTier);
    return FAIHeroesBattleRecordRules::Capture(
        FName(TEXT("FOREST")), 4, ActualTier, Player, Enemies,
        Outcome, bGrid ? 2 : 1);
}

int64 TotalHealth(const TArray<FAIHeroesArmyStack>& Army)
{
    int64 Sum = 0;
    for (const auto& Stack : Army) Sum += Stack.TotalHealth;
    return Sum;
}

int32 FirstChange(const TArray<FString>& Actual, const TArray<FString>& Alternative)
{
    const int32 N = FMath::Min(Actual.Num(), Alternative.Num());
    for (int32 I = 0; I < N; ++I)
    {
        if (Actual[I] != Alternative[I]) return I + 1;
    }
    return Actual.Num() == Alternative.Num() ? 0 : N + 1;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesProtocolLabV2Test,
    "AIHeroes.Lab.GridV2FourTierCounterfactuals",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesProtocolLabV2Test::RunTest(const FString& Parameters)
{
    for (int32 ActualTier = 0; ActualTier <= 3; ++ActualTier)
    {
        const FAIHeroesBattleRecord Original = LabRecord(true, ActualTier);
        FString Error;
        FAIHeroesProtocolLabReport Report;
        if (!TestTrue(TEXT("Every actual V2 battle verifies"),
            FAIHeroesBattleRecordRules::Verify(Original, Error))) return false;
        if (!TestTrue(TEXT("Lab analyzes trusted V2 history"),
            FAIHeroesProtocolLabRules::Analyze(Original, Report, Error))) return false;
        TestTrue(TEXT("Four-tier read-only report is valid"), Report.IsValid());
        TestEqual(TEXT("All four cognitive tiers compared"), Report.Scenarios.Num(), 4);
        if (Report.Scenarios.Num() != 4) return false;
        TestEqual(TEXT("Actual tier named exactly"),
            Report.ActualProtocolTier, ActualTier);
        TestEqual(TEXT("Version bound to historical rules"), Report.RuleVersion, 2);
        for (int32 Tier = 0; Tier < 4; ++Tier)
        {
            const FAIHeroesProtocolLabScenario& Scenario = Report.Scenarios[Tier];
            const FAIHeroesBattleOutcome Alternative =
                FAIHeroesGridBattle::RunToEnd(
                    Original.PlayerBefore, Original.EnemyBefore, Tier);
            TestEqual(TEXT("Tier enumerates in stable order"), Scenario.ProtocolTier, Tier);
            TestEqual(TEXT("One actual run only"), Scenario.bActual, Tier == ActualTier);
            TestEqual(TEXT("Recorded winner is deterministic"),
                Scenario.bVictory, Alternative.bVictory);
            TestEqual(TEXT("Recorded defeat is deterministic"),
                Scenario.bDefeat, Alternative.bDefeat);
            TestEqual(TEXT("Exactly measured rounds"),
                Scenario.Rounds, Alternative.Rounds);
            TestEqual(TEXT("Exactly measured action count"),
                Scenario.TraceActions, Alternative.Trace.Num());
            TestEqual(TEXT("Hero survivors computed from alternative"),
                Scenario.HeroHealth, TotalHealth(Alternative.PlayerAfter));
            TestEqual(TEXT("Enemy survivors computed from alternative"),
                Scenario.EnemyHealth, TotalHealth(Alternative.EnemyAfter));
            TestEqual(TEXT("HP delta relative to genuine actual outcome"),
                Scenario.HeroHpDelta,
                TotalHealth(Alternative.PlayerAfter) - TotalHealth(Original.PlayerAfter));
            TestEqual(TEXT("First trace divergence is 1-based, or zero when identical"),
                Scenario.FirstDifferentAction,
                FirstChange(Original.Trace, Alternative.Trace));
            if (Scenario.bActual)
            {
                TestEqual(TEXT("Actual tier has identical trace"), Scenario.FirstDifferentAction, 0);
                TestEqual(TEXT("Actual tier has exactly zero HP delta"), Scenario.HeroHpDelta, int64(0));
            }
        }
        const TArray<FString> Text = Report.Describe();
        TestEqual(TEXT("Report is finite and concise"), Text.Num(), 10);
        TestTrue(TEXT("Read-only boundary is explicit"),
            Text.IsValidIndex(8) && Text[8].Contains(TEXT("No move, attack")));
        TestTrue(TEXT("Model slots cannot be mistaken for real weights"),
            Text.Last().Contains(TEXT("no actual LLM")));
        TestTrue(TEXT("Original battle is unchanged after four replays"),
            FAIHeroesBattleRecordRules::Verify(Original, Error));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesProtocolLabV1Test,
    "AIHeroes.Lab.LegacyV1Compatible",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesProtocolLabV1Test::RunTest(const FString& Parameters)
{
    const FAIHeroesBattleRecord Original = LabRecord(false, 2);
    FAIHeroesProtocolLabReport Report;
    FString Error;
    if (!TestTrue(TEXT("Valid historical V1 record has exactly one enemy"),
        FAIHeroesBattleRecordRules::Verify(Original, Error))) return false;
    if (!TestTrue(TEXT("Lab uses the original V1 resolver"),
        FAIHeroesProtocolLabRules::Analyze(Original, Report, Error))) return false;
    TestEqual(TEXT("Legacy rule version is preserved"), Report.RuleVersion, 1);
    TestEqual(TEXT("Legacy battle has four comparable tier results"),
        Report.Scenarios.Num(), 4);
    TestEqual(TEXT("Historical actual tier is marked"),
        Report.Scenarios[2].ProtocolTier, 2);
    TestTrue(TEXT("Actual V1 trace unchanged"), Report.Scenarios[2].FirstDifferentAction == 0);
    TestTrue(TEXT("V1 and V2 formats are not conflated"),
        Report.Describe()[0].Contains(TEXT("rules V1")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesProtocolLabTamperTest,
    "AIHeroes.Lab.RejectUnverifiedHistory",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesProtocolLabTamperTest::RunTest(const FString& Parameters)
{
    const FAIHeroesBattleRecord Original = LabRecord(true, 3);
    FString Error;
    FAIHeroesProtocolLabReport Report;
    TestTrue(TEXT("Valid source initializes report"),
        FAIHeroesProtocolLabRules::Analyze(Original, Report, Error));
    TestTrue(TEXT("Initial report populated"), Report.IsValid());

    FAIHeroesBattleRecord Forged = Original;
    Forged.Trace[0] += TEXT(" FAKE ACTION");
    TestFalse(TEXT("Forged trace refused"),
        FAIHeroesProtocolLabRules::Analyze(Forged, Report, Error));
    TestFalse(TEXT("Previous report cleared on rejection"), Report.IsValid());
    TestFalse(TEXT("No partial outcomes exposed"), Report.Scenarios.Num() > 0);
    TestTrue(TEXT("Failure explains rejection"), !Error.IsEmpty());

    Forged = Original;
    Forged.RuleVersion = 99;
    TestFalse(TEXT("Unknown future rules refused"),
        FAIHeroesProtocolLabRules::Analyze(Forged, Report, Error));
    Forged = Original;
    Forged.PlayerAfter[0].TotalHealth += 1;
    TestFalse(TEXT("Forged survivor health refused"),
        FAIHeroesProtocolLabRules::Analyze(Forged, Report, Error));
    Forged = FAIHeroesBattleRecord();
    TestFalse(TEXT("No battle is a safe explicit no-data result"),
        FAIHeroesProtocolLabRules::Analyze(Forged, Report, Error));
    TestTrue(TEXT("No-data output reports a useful next action"),
        Error.Contains(TEXT("Finish a guarded encounter")));
    TestFalse(TEXT("No-data output does not fabricate result"),
        Report.IsValid());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesProtocolLabDeterminismTest,
    "AIHeroes.Lab.RepeatableNonMutatingReport",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesProtocolLabDeterminismTest::RunTest(const FString& Parameters)
{
    FAIHeroesBattleRecord Input = LabRecord(true, 1);
    const FAIHeroesBattleRecord Before = Input;
    FString Error;
    FAIHeroesProtocolLabReport First;
    FAIHeroesProtocolLabReport Second;
    if (!TestTrue(TEXT("First computation succeeds"),
        FAIHeroesProtocolLabRules::Analyze(Input, First, Error))) return false;
    if (!TestTrue(TEXT("Second computation succeeds"),
        FAIHeroesProtocolLabRules::Analyze(Input, Second, Error))) return false;
    TestTrue(TEXT("Same immutable input produces same user-facing explanation"),
        First.Describe() == Second.Describe());
    TestTrue(TEXT("Input remains byte-semantic identical"),
        FAIHeroesBattleRecordRules::Equivalent(Input, Before));
    TestTrue(TEXT("Output contains no order to move or attack"),
        First.Describe().Last().Contains(TEXT("no actual LLM")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesProtocolLabTouchTest,
    "AIHeroes.Lab.KeyboardAndTouchLayout",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesProtocolLabTouchTest::RunTest(const FString& Parameters)
{
    for (const FVector2D Viewport : {
        FVector2D(1280.f, 720.f), FVector2D(960.f, 540.f),
        FVector2D(640.f, 720.f), FVector2D(480.f, 800.f)})
    {
        const auto Lab = FAIHeroesHUDLayout::Rect(203, Viewport);
        const auto Goals = FAIHeroesHUDLayout::Rect(202, Viewport);
        TestTrue(TEXT("Laboratory button is visible on desktop and phone"),
            Lab.W >= 95.f && Lab.H >= 25.f &&
            Lab.X >= 8.f && Lab.X + Lab.W <= Viewport.X &&
            Lab.Y >= 185.f && Lab.Y + Lab.H <= Viewport.Y);
        if (Lab.W <= 0.f) return false;
        const FVector2D Center(Lab.X + Lab.W * 0.5f,
                               Lab.Y + Lab.H * 0.5f);
        TestEqual(TEXT("Touch is dispatched to read-only lab action ID"),
            FAIHeroesHUDLayout::HitTest(Center, Viewport), 203);
        TestFalse(TEXT("Lab and mission goals never overlap"),
            Goals.Contains(Center));
        for (int32 Action = 0; Action < 13; ++Action)
        {
            TestFalse(TEXT("Lab does not steal existing strategic or utility actions"),
                FAIHeroesHUDLayout::Rect(Action, Viewport).Contains(Center));
        }
        TestEqual(TEXT("Lab control is masked in live combat"),
            FAIHeroesHUDLayout::Rect(203, Viewport, true).W, 0.f);
        TestEqual(TEXT("No laboratory input reaches active battle"),
            FAIHeroesHUDLayout::HitTest(Center, Viewport, true), INDEX_NONE);
    }
    return true;
}
#endif
