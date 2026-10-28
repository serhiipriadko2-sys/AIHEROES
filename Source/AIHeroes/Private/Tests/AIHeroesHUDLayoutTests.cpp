#include "AIHeroesHUDLayout.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesHUDLayoutTest,
    "AIHeroes.UI.RouteButtonLayoutRegression",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesHUDLayoutTest::RunTest(const FString& Parameters)
{
    // Test the three responsive layouts without requiring a graphics device.
    for (const int32 Width : {1280, 960, 640})
    {
        const FVector2D Viewport(Width, 720.f);
        const FAIHeroesHUDActionRect Plan = FAIHeroesHUDLayout::Rect(12, Viewport);
        TestTrue(TEXT("Plan action has a non-empty hit rect"), Plan.W > 0.f && Plan.H > 0.f);
        TestTrue(TEXT("Plan cell is visible inside the viewport"),
            Plan.X >= 0.f && Plan.X + Plan.W <= Width &&
            Plan.Y >= 0.f && Plan.Y + Plan.H <= Viewport.Y);

        const FVector2D Center(Plan.X + Plan.W / 2.f, Plan.Y + Plan.H / 2.f);
        TestEqual(TEXT("Click/touch the plan cell resolves action 12"),
            FAIHeroesHUDLayout::HitTest(Center, Viewport), 12);

        // The original seven strategic actions and five utility actions
        // remain uniquely addressable with the new eighth strategic action.
        for (int32 Action = 0; Action < 12; ++Action)
        {
            const FAIHeroesHUDActionRect Rect = FAIHeroesHUDLayout::Rect(Action, Viewport);
            TestTrue(TEXT("Legacy action rectangle inside the viewport"),
                Rect.W > 0.f && Rect.X >= 0.f && Rect.X + Rect.W <= Width);
            const FVector2D LegacyCenter(Rect.X + Rect.W / 2.f, Rect.Y + Rect.H / 2.f);
            TestEqual(TEXT("Legacy action still receives its original ID"),
                FAIHeroesHUDLayout::HitTest(LegacyCenter, Viewport), Action);
            TestFalse(TEXT("Plan and legacy action must not overlap"),
                Rect.Contains(Center));
        }
    }

    const FVector2D Desktop(1280.f, 720.f);
    const FAIHeroesHUDActionRect Plan = FAIHeroesHUDLayout::Rect(12, Desktop);
    TestEqual(TEXT("Desktop plan button X remains in the old blank cell"), Plan.X, 915.f);
    TestEqual(TEXT("Desktop plan button Y is second row"), Plan.Y, 660.f);
    TestEqual(TEXT("Previous confirm button X is unchanged"),
        FAIHeroesHUDLayout::Rect(0, Desktop).X, 105.f);
    TestEqual(TEXT("Previous save utility Y stays at the top"),
        FAIHeroesHUDLayout::Rect(7, Desktop).Y, 7.f);
    TestEqual(TEXT("Outside all rectangles has no action"),
        FAIHeroesHUDLayout::HitTest(FVector2D(0.f, 320.f), Desktop), INDEX_NONE);
    TestEqual(TEXT("Invalid action ID is not drawable"),
        FAIHeroesHUDLayout::Rect(13, Desktop).W, 0.f);

    for (const int32 Width : {1280, 960, 640})
    {
        const FVector2D Viewport(Width, 720.f);
        const FAIHeroesHUDActionRect Recruit = FAIHeroesHUDLayout::Rect(200, Viewport);
        const FAIHeroesHUDActionRect Ask = FAIHeroesHUDLayout::Rect(1, Viewport);
        const FVector2D RecruitCenter(Recruit.X + Recruit.W * 0.5f,
            Recruit.Y + Recruit.H * 0.5f);
        TestTrue(TEXT("Recruit advisor has a visible on-screen hitbox"),
            Recruit.W >= 40.f && Recruit.X >= 0.f && Recruit.X + Recruit.W <= Width);
        TestEqual(TEXT("Recruit advisor resolves its independent ID 200"),
            FAIHeroesHUDLayout::HitTest(RecruitCenter, Viewport), 200);
        TestFalse(TEXT("Recruit advisor cannot swallow the ASK HERO center"),
            Recruit.Contains(FVector2D(Ask.X + Ask.W * 0.5f, Ask.Y + Ask.H * 0.5f)));

        for (int32 Action = 100; Action <= 103; ++Action)
        {
            const FAIHeroesHUDActionRect Combat = FAIHeroesHUDLayout::Rect(
                Action, Viewport, true);
            const FVector2D CombatCenter(Combat.X + Combat.W / 2.f,
                Combat.Y + Combat.H / 2.f);
            TestEqual(TEXT("Live combat control is independently clickable"),
                FAIHeroesHUDLayout::HitTest(CombatCenter, Viewport, true), Action);
        }
        TestEqual(TEXT("Live combat masks strategic ASK HERO"),
            FAIHeroesHUDLayout::Rect(1, Viewport, true).W, 0.f);
        TestEqual(TEXT("Live combat masks recruitment advisor"),
            FAIHeroesHUDLayout::Rect(200, Viewport, true).W, 0.f);
        TestEqual(TEXT("Outside the live arena controls has no action"),
            FAIHeroesHUDLayout::HitTest(FVector2D(20.f, 300.f), Viewport, true),
            INDEX_NONE);
    }
    TestEqual(TEXT("Legacy 1280x720 ASK HERO click remains functional"),
        FAIHeroesHUDLayout::HitTest(FVector2D(505.f, 630.f), Desktop), 1);
    TestEqual(TEXT("New 1280x720 PLAN ROUTE click remains functional"),
        FAIHeroesHUDLayout::HitTest(FVector2D(1010.f, 681.f), Desktop), 12);
    return true;
}
#endif
