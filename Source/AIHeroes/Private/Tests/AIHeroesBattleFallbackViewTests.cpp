#include "AIHeroesBattleFallbackView.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AIHeroesGridBattle.h"
#include "AIHeroesBattleArt.h"
#include "Misc/AutomationTest.h"

namespace
{
FAIHeroesBattleFallbackView MakeRunningFallback()
{
    return FAIHeroesBattleFallbackView::Build(false, false, false, 7,
        TEXT("Militia #1: 8 units / 90 HP / INIT 3 | Scout #2: 3 units / 21 HP / INIT 7"),
        TEXT("Wolf #1: 4 units / 36 HP / INIT 8"));
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBattleFallbackRunning,
    "AIHeroes.Playability.LiveBattleFallback.RunningKeepsKnownArmies",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBattleFallbackRunning::RunTest(const FString&)
{
    const auto Copy = MakeRunningFallback();
    TestTrue(TEXT("Missing arena activates text fallback"), Copy.bVisible);
    TestTrue(TEXT("Actions count comes from current session"),
        Copy.ActionLabel.Contains(TEXT("7")));
    TestTrue(TEXT("No false loading state"),
        Copy.Status.Contains(TEXT("продолжает бой")));
    if(!TestEqual(TEXT("All two allied stacks are visible"), Copy.PlayerRoster.Num(), 2))
        return false;
    if(!TestEqual(TEXT("Known enemy stack is visible"), Copy.EnemyRoster.Num(), 1))
        return false;
    TestTrue(TEXT("Unit count labels are localised"),
        Copy.PlayerRoster[0].Contains(TEXT("бойцов")));
    TestTrue(TEXT("Health labels are localised"),
        Copy.EnemyRoster[0].Contains(TEXT("ОЗ")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBattleFallbackPaused,
    "AIHeroes.Playability.LiveBattleFallback.PausedDoesNotClaimProgress",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBattleFallbackPaused::RunTest(const FString&)
{
    const auto Copy = FAIHeroesBattleFallbackView::Build(false, false, true,
        5, TEXT("Militia #1: 4 units / 48 HP / INIT 3"), FString());
    TestTrue(TEXT("Paused combat has truthful status"),
        Copy.Status.Contains(TEXT("Пауза")));
    TestFalse(TEXT("Paused combat is not called a result"),
        Copy.Status.Contains(TEXT("Бой завершён")));
    if(!TestEqual(TEXT("Absent enemy state is explicitly unknown"),
        Copy.EnemyRoster.Num(), 1)) return false;
    TestTrue(TEXT("No invented zero-enemy count"),
        Copy.EnemyRoster[0].Contains(TEXT("недоступны")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBattleFallbackResult,
    "AIHeroes.Playability.LiveBattleFallback.ResultOffersReturn",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBattleFallbackResult::RunTest(const FString&)
{
    const auto Copy = FAIHeroesBattleFallbackView::Build(false, true, false,
        13, TEXT("Scout #1: 2 units / 11 HP / INIT 7"), FString());
    TestTrue(TEXT("Completed combat asks player to return"),
        Copy.Status.Contains(TEXT("вернитесь на карту")));
    TestFalse(TEXT("Completed combat does not pretend it is still executing"),
        Copy.Status.Contains(TEXT("продолжает бой")));
    TestFalse(TEXT("No invented reward amount in result hint"),
        Copy.Status.Contains(TEXT("золота")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBattleFallbackAvailable,
    "AIHeroes.Playability.LiveBattleFallback.ArenaAvailableIsSilent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBattleFallbackAvailable::RunTest(const FString&)
{
    const auto Copy = FAIHeroesBattleFallbackView::Build(true, false, false,
        500, TEXT("SecretPlayer"), TEXT("SecretEnemy"));
    TestFalse(TEXT("Normal arena never displays text fallback"), Copy.bVisible);
    TestTrue(TEXT("Normal arena does not reveal duplicated roster"),
        Copy.PlayerRoster.IsEmpty() && Copy.EnemyRoster.IsEmpty());
    TestTrue(TEXT("Normal arena keeps no fallback caption"),
        Copy.Status.IsEmpty() && Copy.ActionLabel.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBattleFallbackInput,
    "AIHeroes.Playability.LiveBattleFallback.EmptyAndNegativeInputSafe",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBattleFallbackInput::RunTest(const FString&)
{
    const auto Copy = FAIHeroesBattleFallbackView::Build(false, false, false,
        -7, TEXT(" | "), TEXT(" "));
    if(!TestTrue(TEXT("Missing roster does not crash or invent fighters"),
        Copy.PlayerRoster.Num() == 1 && Copy.EnemyRoster.Num() == 1)) return false;
    TestEqual(TEXT("Negative action count clamps to zero"),
        Copy.ActionLabel, FString(TEXT("Действий: 0")));
    TestTrue(TEXT("Missing roster stays explicitly unknown"),
        Copy.PlayerRoster[0].Contains(TEXT("недоступны")));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBattleFallbackActualEvent,
    "AIHeroes.Playability.LiveBattleFallback.ActualEventAndDecisionReason",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBattleFallbackActualEvent::RunTest(const FString&)
{
    FAIHeroesGridAction Action;
    Action.Kind=EAIHeroesGridActionKind::Attack;Action.Round=2;
    Action.bPlayerAttacker=true;Action.AttackerStackIndex=0;
    Action.TargetStackIndex=1;Action.AttackerId=TEXT("Militia");
    Action.TargetId=TEXT("Raider");Action.Damage=7;
    Action.TargetHealthBefore=20;Action.TargetHealthAfter=13;
    Action.Reason=TEXT("highest threat");
    const auto Before=Action.ToLogLine();
    const auto View=FAIHeroesBattleFallbackView::Build(
        false,false,true,7,TEXT("Militia #1: 8 units / 90 HP / INIT 3"),
        TEXT("Raider #2: 2 units / 13 HP / INIT 5"),&Action);
    TestTrue(TEXT("Fallback describes the actual attacker and target group"),
        View.Status.Contains(TEXT("A1"))&&View.Status.Contains(TEXT("B2")));
    TestTrue(TEXT("Fallback exposes actual damage, not generic preparation"),
        View.Status.Contains(TEXT("7")));
    TestTrue(TEXT("Target choice is grounded in the event's threat reason"),
        View.DecisionReason.Contains(TEXT("угроз")));
    TestEqual(TEXT("Display never modifies the event"),Action.ToLogLine(),Before);
    const auto Normal=FAIHeroesBattleFallbackView::Build(true,false,false,7,
        TEXT("Unused"),TEXT("Unused"),&Action);
    TestTrue(TEXT("An intact arena never leaks an ahead-of-animation fallback"),
        Normal.Status.IsEmpty()&&Normal.DecisionReason.IsEmpty());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBattleFallbackReasonBoundaries,
    "AIHeroes.Playability.LiveBattleFallback.ReasonVocabularyAndBoundaries",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBattleFallbackReasonBoundaries::RunTest(const FString&)
{
    FAIHeroesGridAction Action;
    Action.Round=1;Action.AttackerId=TEXT("Scout");Action.AttackerStackIndex=1;
    Action.bPlayerAttacker=true;
    const TCHAR* Reasons[]={TEXT("first legal target"),TEXT("highest threat"),
        TEXT("finish damaged stack"),TEXT("lethal priority"),TEXT("no reachable target"),
        TEXT("no target in range after movement")};
    for(const TCHAR* Reason:Reasons)
    {
        Action.Reason=Reason;
        const auto Text=FAIHeroesBattleArt::ActionReason(Action);
        TestTrue(TEXT("Known reason is a translated label, not an inferred new policy"),
            Text.StartsWith(TEXT("Причина: "))&&!Text.Contains(Reason));
    }
    Action.Reason=TEXT("future-policy\nraw-event");
    const auto Unknown=FAIHeroesBattleArt::ActionReason(Action);
    TestTrue(TEXT("Unknown vocabulary remains explicitly quoted event data"),
        Unknown.Contains(TEXT("future-policy raw-event")));
    TestFalse(TEXT("Unknown event reason cannot introduce a second UI line"),Unknown.Contains(TEXT("\n")));
    Action.Reason.Reset();
    TestTrue(TEXT("Missing reason is not invented"),
        FAIHeroesBattleArt::ActionReason(Action).Contains(TEXT("не записана")));
    Action.Round=0;
    TestTrue(TEXT("No action yet means no claimed decision"),
        FAIHeroesBattleArt::ActionReason(Action).IsEmpty()&&
        FAIHeroesBattleArt::ActionCaption(Action).IsEmpty());
    return true;
}
#endif
