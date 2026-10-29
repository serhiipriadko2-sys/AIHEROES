#include "AIHeroesZoneScreenLayout.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

namespace
{
bool Overlap(const FAIHeroesZoneScreenRect& A,
             const FAIHeroesZoneScreenRect& B)
{
    return A.X < B.X + B.Width && A.X + A.Width > B.X &&
        A.Y < B.Y + B.Height && A.Y + A.Height > B.Y;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesExpeditionWideChromeTest,
    "AIHeroes.Visual.ExpeditionChrome.WidePanelsDoNotStealOrders",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesExpeditionWideChromeTest::RunTest(const FString&)
{
    const auto Layout = FAIHeroesZoneScreenLayout::Build(
        FVector2D(1280.f, 720.f), false);
    if (!TestTrue(TEXT("Wide reference HUD is supported"),
        Layout.bSupported && Layout.bWideChrome)) return false;
    const FAIHeroesZoneScreenRect Panels[] = {
        Layout.MissionPanel, Layout.InspectorPanel, Layout.MiniMapPanel
    };
    for (int32 I = 0; I < 3; ++I)
    {
        const auto& P = Panels[I];
        const FVector2D Center(P.X + .5f * P.Width,
                               P.Y + .5f * P.Height);
        TestTrue(TEXT("Decorative chrome consumes map clicks"),
            Layout.ContainsChrome(Center));
        TestEqual(TEXT("No hidden authority inside decorative panel"),
            Layout.HitTest(Center), EAIHeroesZoneScreenAction::None);
        TestTrue(TEXT("Chrome stays inside map-side safe bounds"),
            P.X >= 18.f && P.Y >= 137.f &&
            P.X + P.Width <= 1262.f && P.Y + P.Height <= 500.f);
        for (const auto& Action : Layout.Buttons)
            TestFalse(TEXT("Panel never overlaps action hit box"),
                Overlap(P, Action.Rect));
        for (int32 J = I + 1; J < 3; ++J)
            TestFalse(TEXT("Decorative panels do not intersect"),
                Overlap(P, Panels[J]));
    }
    TestFalse(TEXT("Free map center is not consumed"),
        Layout.ContainsChrome(FVector2D(640.f, 350.f)));
    TestFalse(TEXT("Invalid coordinates never dispatch"),
        Layout.ContainsChrome(FVector2D(-1.f, 350.f)));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesExpeditionCompactChromeTest,
    "AIHeroes.Visual.ExpeditionChrome.CompactMapRemainsOpen",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesExpeditionCompactChromeTest::RunTest(const FString&)
{
    const FVector2D Screens[] = {
        FVector2D(960.f, 540.f),
        FVector2D(640.f, 520.f),
        FVector2D(720.f, 1280.f)
    };
    for (const auto& Screen : Screens)
    {
        const auto Layout = FAIHeroesZoneScreenLayout::Build(Screen, false);
        TestTrue(TEXT("Compact action layout stays playable"), Layout.bSupported);
        TestFalse(TEXT("Landscape chrome hides on compact screens"),
            Layout.bWideChrome);
        TestFalse(TEXT("Compact screen keeps map taps available"),
            Layout.ContainsChrome(FVector2D(Screen.X * .5f, Screen.Y * .5f)));
        TestEqual(TEXT("Ten human controls remain present"),
            Layout.Buttons.Num(), 10);
    }
    const auto Modal = FAIHeroesZoneScreenLayout::Build(
        FVector2D(1280.f, 720.f), true);
    TestEqual(TEXT("Read-only modal still has only its Close action"),
        Modal.Buttons.Num(), 1);
    TestEqual(TEXT("Modal action remains Close"),
        Modal.Buttons[0].Action,
        EAIHeroesZoneScreenAction::CloseComparison);
    return true;
}
#endif
