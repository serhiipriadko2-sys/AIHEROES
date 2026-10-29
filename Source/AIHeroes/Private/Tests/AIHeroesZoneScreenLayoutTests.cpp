#include "AIHeroesZoneScreenLayout.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

namespace
{
bool Overlaps(const FAIHeroesZoneScreenRect& A,
              const FAIHeroesZoneScreenRect& B)
{
    return A.X < B.X + B.Width && A.X + A.Width > B.X &&
           A.Y < B.Y + B.Height && A.Y + A.Height > B.Y;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesZoneTouchGeometryTest,
    "AIHeroes.ZoneTouch.LayoutFourViewportsNoOverlaps",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesZoneTouchGeometryTest::RunTest(const FString&)
{
    const FVector2D Sizes[] = {
        {1280.f, 720.f},
        {960.f, 540.f},
        {720.f, 1280.f},
        {640.f, 520.f},
        {1920.f, 1080.f}
    };
    for (const auto& Size : Sizes)
    {
        const auto Layout = FAIHeroesZoneScreenLayout::Build(Size, false);
        TestTrue(TEXT("Playable screen layout fits"), Layout.bSupported);
        if (!TestEqual(TEXT("Ten manual actions available"),
            Layout.Buttons.Num(), 10)) return false;
        TSet<uint8> Seen;
        for (int32 I = 0; I < Layout.Buttons.Num(); ++I)
        {
            const auto& Button = Layout.Buttons[I];
            const auto& R = Button.Rect;
            TestTrue(TEXT("All buttons minimum finger-height"),
                R.Height >= 42.f && R.Width >= 100.f);
            TestTrue(TEXT("Buttons stay within viewport and footer safe margin"),
                R.X >= 18.f && R.Y >= 137.f &&
                R.X + R.Width <= Size.X - 18.f &&
                R.Y + R.Height <= Size.Y - 97.f);
            const FVector2D Center(R.X + R.Width * .5f,
                                   R.Y + R.Height * .5f);
            TestEqual(TEXT("Touch and mouse use one hit-test geometry"),
                Layout.HitTest(Center), Button.Action);
            const uint8 Id = static_cast<uint8>(Button.Action);
            TestFalse(TEXT("No repeated authority-bearing controls"),
                Seen.Contains(Id));
            Seen.Add(Id);
            TestTrue(TEXT("Every button has an explicit accessible label"),
                FCString::Strlen(Button.Label) > 2);
            for (int32 J = I + 1; J < Layout.Buttons.Num(); ++J)
                TestFalse(TEXT("No click-target overlap"),
                    Overlaps(R, Layout.Buttons[J].Rect));
        }
        TestFalse(TEXT("Status footer cannot be a hidden action"),
            Layout.HitTest(FVector2D(Size.X * .5f, Size.Y - 10.f)) !=
                EAIHeroesZoneScreenAction::None);
        TestEqual(TEXT("Outside the viewport never dispatches"),
            Layout.HitTest(FVector2D(-1.f, -1.f)),
            EAIHeroesZoneScreenAction::None);
        const auto Panel = FAIHeroesZoneScreenLayout::Build(Size, true);
        TestTrue(TEXT("Comparison ends before touch toolbar"),
            Panel.ComparisonPanel.Y + Panel.ComparisonPanel.Height <=
            Layout.Buttons[0].Rect.Y - 8.f);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesZoneTouchModalTest,
    "AIHeroes.ZoneTouch.ModalAllowsOnlyClose",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesZoneTouchModalTest::RunTest(const FString&)
{
    const FVector2D Size(1280.f, 720.f);
    const auto Normal = FAIHeroesZoneScreenLayout::Build(Size, false);
    const auto Modal = FAIHeroesZoneScreenLayout::Build(Size, true);
    if (!TestTrue(TEXT("Modal supported"), Modal.bSupported &&
        Modal.Buttons.Num() == 1)) return false;
    const auto& Close = Modal.Buttons[0].Rect;
    TestEqual(TEXT("The ONLY modal action closes the read-only dossier"),
        Modal.Buttons[0].Action,
        EAIHeroesZoneScreenAction::CloseComparison);
    TestEqual(TEXT("Close rect dispatches close"),
        Modal.HitTest(FVector2D(
            Close.X + Close.Width*.5f, Close.Y + Close.Height*.5f)),
        EAIHeroesZoneScreenAction::CloseComparison);
    for (const auto& Item : Normal.Buttons)
    {
        const auto Center = FVector2D(
            Item.Rect.X + Item.Rect.Width*.5f,
            Item.Rect.Y + Item.Rect.Height*.5f);
        if (Close.Contains(Center)) continue;
        TestEqual(TEXT("All authoritative buttons absent during comparison"),
            Modal.HitTest(Center), EAIHeroesZoneScreenAction::None);
    }
    TestEqual(TEXT("Tapping comparison does not automatically start a fight"),
        Modal.HitTest(FVector2D(640.f, 300.f)),
        EAIHeroesZoneScreenAction::None);
    TestFalse(TEXT("Opening a dossier is not an executable movement"),
        Normal.Buttons[3].Action == EAIHeroesZoneScreenAction::ConfirmMove);
    TestEqual(TEXT("MOVE requires its separate explicit player press"),
        Normal.Buttons[4].Action, EAIHeroesZoneScreenAction::ConfirmMove);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesZoneTouchFailClosedTest,
    "AIHeroes.ZoneTouch.TinyScreenAndBoundaryFailClosed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesZoneTouchFailClosedTest::RunTest(const FString&)
{
    const FVector2D Invalid[] = {
        {0.f, 0.f}, {639.f, 520.f}, {640.f, 519.f},
        {-1.f, 720.f}, {20000.f, 720.f}
    };
    for (const auto& Size : Invalid)
    {
        const auto InvalidLayout =
            FAIHeroesZoneScreenLayout::Build(Size, false);
        TestFalse(TEXT("Too-small or invalid screen cannot have hit targets"),
            InvalidLayout.bSupported);
        TestTrue(TEXT("Invalid screens have zero executable buttons"),
            InvalidLayout.Buttons.IsEmpty());
    }
    const auto Layout =
        FAIHeroesZoneScreenLayout::Build(FVector2D(1280.f, 720.f), false);
    const auto& Confirm = Layout.Buttons[4].Rect;
    TestEqual(TEXT("Left edge of confirm is inclusive"),
        Layout.HitTest(FVector2D(Confirm.X, Confirm.Y)),
        EAIHeroesZoneScreenAction::ConfirmMove);
    TestEqual(TEXT("Confirmation right border is exclusive"),
        Layout.HitTest(FVector2D(Confirm.X + Confirm.Width, Confirm.Y)),
        EAIHeroesZoneScreenAction::None);
    TestTrue(TEXT("SAVE is explicit and separately bounded"),
        Layout.Buttons[8].bHighImpact &&
        Layout.Buttons[8].Action == EAIHeroesZoneScreenAction::Save);
    TestTrue(TEXT("LOAD is explicit and separately bounded"),
        Layout.Buttons[9].bHighImpact &&
        Layout.Buttons[9].Action == EAIHeroesZoneScreenAction::Load);
    TestEqual(TEXT("Nothing outside the buttons can confirm battle"),
        Layout.HitTest(FVector2D(640.f, 400.f)),
        EAIHeroesZoneScreenAction::None);
    return true;
}
#endif
