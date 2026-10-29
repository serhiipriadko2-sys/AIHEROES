#include "AIHeroesHUDTextLayout.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AIHeroesHUDLayout.h"
#include "Misc/AutomationTest.h"
#include <limits>

namespace HUDTextTestDetail
{
float Measure(const FString& Text)
{
    float Width = 0.f;
    for (TCHAR C : Text) Width += C == TCHAR('W') ? 12.f : C == TCHAR('i') ? 2.f : 5.f;
    return Width;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHUDTextFitTest, "AIHeroes.HUDText.FitAndDisclosure",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHUDTextFitTest::RunTest(const FString& Parameters)
{
    using HUDTextTestDetail::Measure;
    const FString Original(TEXT("WWWWiiii movement needs confirmation"));
    TestEqual(TEXT("Exact fit keeps the complete text"),
        FAIHeroesHUDTextLayout::FitLine(Original, Measure(Original), Measure), Original);
    const FString Short = FAIHeroesHUDTextLayout::FitLine(Original, 52.f, Measure);
    TestTrue(TEXT("Truncation is explicit"), Short.EndsWith(TEXT("...")));
    TestTrue(TEXT("Ellipsis is included in measured budget"), Measure(Short) <= 52.f);
    TestTrue(TEXT("Uses pixel width, not character count"), Short.Len() < Original.Len());
    TestEqual(TEXT("Fits narrower letters without needless truncation"),
        FAIHeroesHUDTextLayout::FitLine(TEXT("iiiiiiii"), 16.f, Measure), FString(TEXT("iiiiiiii")));
    TestTrue(TEXT("Insufficient space even for ellipsis produces no overflow"),
        FAIHeroesHUDTextLayout::FitLine(Original, 10.f, Measure).IsEmpty());
    TestEqual(TEXT("Line breaks cannot draw into another HUD row"),
        FAIHeroesHUDTextLayout::FitLine(TEXT("A\nB\rC\tD\u2028E"), 100.f, Measure), FString(TEXT("A B C D E")));
    TestEqual(TEXT("Input remains unchanged"), Original, FString(TEXT("WWWWiiii movement needs confirmation")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHUDTextInvalidTest, "AIHeroes.HUDText.InvalidInputs",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHUDTextInvalidTest::RunTest(const FString& Parameters)
{
    using HUDTextTestDetail::Measure;
    for (float W : {-1.f, 0.f, 40000.f, std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::infinity()})
        TestTrue(TEXT("Invalid pixel budget rejected"), FAIHeroesHUDTextLayout::FitLine(TEXT("text"), W, Measure).IsEmpty());
    TestTrue(TEXT("Invalid font measurement rejected"), FAIHeroesHUDTextLayout::FitLine(TEXT("text"), 100.f,
        [](const FString&) { return std::numeric_limits<float>::quiet_NaN(); }).IsEmpty());
    TestTrue(TEXT("Negative font measurement rejected"), FAIHeroesHUDTextLayout::FitLine(TEXT("text"), 100.f,
        [](const FString&) { return -1.f; }).IsEmpty());
    const FString Huge = FString::ChrN(FAIHeroesHUDTextLayout::MaxSourceChars + 1, TCHAR('W'));
    int32 Calls = 0;
    TestTrue(TEXT("Oversized text rejected before font work"), FAIHeroesHUDTextLayout::FitLine(Huge, 100.f,
        [&Calls](const FString&) { ++Calls; return 1.f; }).IsEmpty());
    TestEqual(TEXT("No allocation-amplifying measurement on oversized input"), Calls, 0);
    TestEqual(TEXT("Invalid viewport has no header lane"), FAIHeroesHUDTextLayout::HeaderLineWidth(
        FVector2D(std::numeric_limits<double>::infinity(),720), 12.f, 20.f), 0.f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHUDTextLanesTest, "AIHeroes.HUDText.UtilitySeparation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHUDTextLanesTest::RunTest(const FString& Parameters)
{
    int32 Cases = 0;
    for (int32 W : {320,640,699,700,960,1049,1050,1280,1920})
    for (float Y : {12.f,42.f,68.f,94.f,119.f,147.f})
    {
        const FVector2D V(W,720);
        const float Span = FAIHeroesHUDTextLayout::HeaderLineWidth(V,Y,20.f);
        TestTrue(TEXT("Lane stays in viewport"), Span >= 0.f && 18.f + Span <= W - 18.f);
        for (int32 Action = 7; Action <= 11; ++Action)
        {
            const auto R = FAIHeroesHUDLayout::Rect(Action,V);
            if (Y < R.Y+R.H && Y+20.f > R.Y)
                TestTrue(TEXT("All overlapping utility rows reserve their actual rectangles"), 18.f+Span <= R.X-8.f);
        }
        ++Cases;
    }
    TestEqual(TEXT("Fifty-four width/row combinations checked"),Cases,54);
    TestEqual(TEXT("Rows below utility area reclaim the full width"),
        FAIHeroesHUDTextLayout::HeaderLineWidth(FVector2D(640,720),119.f,20.f),604.f);
    TestEqual(TEXT("Save button keeps its established input location"),
        FAIHeroesHUDLayout::Rect(7,FVector2D(1280,720)).Y,7.f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHUDTextSweepTest, "AIHeroes.HUDText.BoundedWidthSweep",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHUDTextSweepTest::RunTest(const FString& Parameters)
{
    using HUDTextTestDetail::Measure;
    const FString Long = FString::ChrN(FAIHeroesHUDTextLayout::MaxSourceChars,TCHAR('W'));
    for (float W : {1.f,15.f,20.f,37.f,100.f,604.f,1200.f})
    {
        int32 Calls = 0;
        const FString Result = FAIHeroesHUDTextLayout::FitLine(Long,W,
            [&Calls](const FString& S) { ++Calls; return Measure(S); });
        TestTrue(TEXT("All displayed output fits including dots"), Measure(Result) <= W);
        TestTrue(TEXT("Measurement work bounded logarithmically"), Calls <= 18);
    }
    FString Pair; Pair.AppendChar(TCHAR(0xd83d)); Pair.AppendChar(TCHAR(0xde80)); Pair += TEXT("WWWW");
    for (float W : {15.f,20.f,25.f,30.f})
    {
        const FString Result = FAIHeroesHUDTextLayout::FitLine(Pair,W,Measure);
        const int32 Prefix = Result.Len() - 3;
        if (Result.EndsWith(TEXT("...")) && Prefix > 0)
            TestFalse(TEXT("Truncation never leaves half a UTF-16 pair"),
                static_cast<uint32>(Result[Prefix-1]) >= 0xd800 && static_cast<uint32>(Result[Prefix-1]) <= 0xdbff);
    }
    return true;
}
#endif
