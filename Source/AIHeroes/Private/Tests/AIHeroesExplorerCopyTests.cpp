#include "AIHeroesExplorerCopy.h"
#include "AIHeroesExplorerLayout.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompactExplorerButtons,
    "AIHeroes.Visual.ExplorerCopy.CompactDockActions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCompactExplorerButtons::RunTest(const FString&)
{
    const int32 Actions[] = {400, 203, 1, 2};
    const TCHAR* Full[] = {
        TEXT("Постройки [G]"), TEXT("Тактики [Y]"),
        TEXT("Спросить [H]"), TEXT("Конец хода")
    };
    const TCHAR* Compact[] = {
        TEXT("База"), TEXT("Тактики"), TEXT("Совет"), TEXT("Ход")
    };
    for (const float Width : {320.f, 360.f, 480.f, 640.f, 960.f, 1280.f})
    {
        const FVector2D V(Width, 720.f);
        for (int32 I = 0; I < 4; ++I)
        {
            const auto R = FAIHeroesExplorerLayout::Button(Actions[I], V, false);
            const FString Label = FAIHeroesExplorerCopy::DockLabel(
                Actions[I], R.W, Full[I]);
            TestEqual(TEXT("Narrow primary actions have complete short labels"),
                Label, FString(R.W < 168.f ? Compact[I] : Full[I]));
            TestEqual(TEXT("The displayed label uses the old action rectangle"),
                FAIHeroesExplorerLayout::Hit(
                    FVector2D(R.X + R.W * .5f, R.Y + R.H * .5f), V, false),
                Actions[I]);
            TestTrue(TEXT("Player-facing button is never empty"), !Label.IsEmpty());
        }
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompactExplorerEvents,
    "AIHeroes.Visual.ExplorerCopy.KnownEventsLocalized",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCompactExplorerEvents::RunTest(const FString&)
{
    const auto Intro = FAIHeroesExplorerCopy::EventText(TEXT(
        "Select a node, ASK HERO (H), then CONFIRM (Enter). Battles are AI-controlled."), 640.f);
    TestTrue(TEXT("Opening order is Russian and touch readable"),
        Intro.Contains(TEXT("Выберите цель")) && !Intro.Contains(TEXT("Enter")));
    for (const TCHAR* Id : {
            TEXT("HQ"), TEXT("FOREST"), TEXT("ORE"),
            TEXT("OUTPOST"), TEXT("RUINS")})
    {
        const FString Raw = FString::Printf(
            TEXT("%s selected. Ask H for advice; Enter to move/attack."), Id);
        const auto Small = FAIHeroesExplorerCopy::EventText(Raw, 640.f);
        const auto Wide = FAIHeroesExplorerCopy::EventText(Raw, 1280.f);
        TestTrue(TEXT("Known sites translate to player-readable instructions"),
            Small.Contains(TEXT("Совет")) && Wide.Contains(TEXT("подтвердите")));
        TestFalse(TEXT("Translated compact message never reveals a guard count"),
            Small.Contains(TEXT("охрана")));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCompactExplorerUnknown,
    "AIHeroes.Visual.ExplorerCopy.UnknownMessagesUntouched",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCompactExplorerUnknown::RunTest(const FString&)
{
    for (const FString Raw : {
        FString(TEXT("SAVE BLOCKED: invalid campaign state.")),
        FString(TEXT("SECRET_HIDDEN_NODE selected. Ask H for advice; Enter to move/attack.")),
        FString(TEXT("Unknown opponent has 99 guards")),
        FString(TEXT("")),
        FString(TEXT("PURCHASE APPLIED ONCE. Select another card to continue."))})
    {
        TestEqual(TEXT("No invented meanings for unrecognized events"),
            FAIHeroesExplorerCopy::EventText(Raw, 640.f), Raw);
    }
    TestEqual(TEXT("Unknown action does not get a fake button label"),
        FAIHeroesExplorerCopy::DockLabel(777, 144.f, TEXT("Unknown command")),
        FString(TEXT("Unknown command")));
    TestEqual(TEXT("Invalid geometry cannot change button text"),
        FAIHeroesExplorerCopy::DockLabel(400, -1.f, TEXT("Постройки [G]")),
        FString(TEXT("Постройки [G]")));
    return true;
}
#endif
