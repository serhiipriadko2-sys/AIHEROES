#include "AIHeroesAdvisorDossier.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include <limits>

namespace
{
float MixedWidth(const FString& Text)
{
    if (Text.IsEmpty()) return 0.f;
    float Width = 0.f;
    for (int32 I = 0; I < Text.Len(); ++I)
    {
        const TCHAR C = Text[I];
        Width += C == TCHAR('Ш') ? 20.f :
            C == TCHAR('i') ? 3.f :
            C == TCHAR(' ') ? 4.f : 9.f;
    }
    return Width;
}
FString WithoutSpaces(const TArray<FString>& Lines)
{
    FString Joined;
    for (const FString& Line : Lines)
        for (const TCHAR C : Line)
            if (!FChar::IsWhitespace(C))
                Joined.AppendChar(C);
    return Joined;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesDossierMeasuredRussianTest,
    "AIHeroes.Dossier.MeasuredRussianAndMixedGlyphs",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAIHeroesDossierMeasuredRussianTest::RunTest(const FString&)
{
    const TArray<FString> Paragraphs = {
        TEXT("ШШШШШШШШШШШШ"),
        TEXT("Сверхдлинноеназваниесохранения «Широкий шрифт» iiii 123")
    };
    const auto Lines = FAIHeroesAdvisorDossierRules::WrapMeasured(
        Paragraphs, 92.f, MixedWidth);
    if (!TestTrue(TEXT("All source paragraphs produce visible lines"),
        Lines.Num() > 4 && Lines.Contains(FString())))
        return false;
    for (const FString& Line : Lines)
        if (!Line.IsEmpty())
        {
            TestTrue(TEXT("Actual variable glyph widths fit 92px"),
                MixedWidth(Line) <= 92.f);
        }
    const FString Before = WithoutSpaces(Paragraphs);
    TestEqual(TEXT("Measured reflow preserves Cyrillic and punctuation content"),
        WithoutSpaces(Lines), Before);
    TestTrue(TEXT("PageCount includes the full report, not only first page"),
        FAIHeroesAdvisorDossierRules::PageCount(Lines.Num(), 4)>1);
    const auto Wide = FAIHeroesAdvisorDossierRules::WrapMeasured(
        Paragraphs, 1600.f, MixedWidth);
    TestTrue(TEXT("A wide report keeps its separate paragraphs"),
        Wide.Num()==3 && Wide[1].IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesDossierMeasuredLongTokens,
    "AIHeroes.Dossier.MeasuredBoundedLongWordsAndSurrogates",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAIHeroesDossierMeasuredLongTokens::RunTest(const FString&)
{
    const FString Token = FString::ChrN(245, TCHAR('Ж'));
    const auto Lines = FAIHeroesAdvisorDossierRules::WrapMeasured(
        {Token}, 47.f, MixedWidth);
    TestTrue(TEXT("Long Cyrillic tokens wrap into many actual lines"),
        Lines.Num() > 40);
    TestEqual(TEXT("No letter is lost at a measured line break"),
        FString::Join(Lines, TEXT("")), Token);
    for (const FString& Line : Lines)
        TestTrue(TEXT("Every long token segment fits measured width"),
            MixedWidth(Line) <= 47.f);

    // UTF-16 surrogate pairs represent one displayed code point.
    FString Pair;
    Pair.AppendChar(static_cast<TCHAR>(0xd83e));
    Pair.AppendChar(static_cast<TCHAR>(0xdded));
    const auto Emoji = FAIHeroesAdvisorDossierRules::WrapMeasured(
        {Pair + Pair}, 18.f, MixedWidth);
    TestEqual(TEXT("An emoji surrogate pair is not split"), Emoji.Num(), 2);
    for (const FString& Line : Emoji)
        TestEqual(TEXT("Each whole surrogate pair remains intact"), Line, Pair);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesDossierMeasuredFailure,
    "AIHeroes.Dossier.MeasuredInvalidInputsFailClosed",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAIHeroesDossierMeasuredFailure::RunTest(const FString&)
{
    const TArray<FString> Input = {TEXT("Широкий текст")};
    const float NaN = std::numeric_limits<float>::quiet_NaN();
    for (const float Width : {-1.f, 0.f, 50000.f, NaN})
    {
        TestTrue(TEXT("Invalid viewport width yields no fabricated lines"),
            FAIHeroesAdvisorDossierRules::WrapMeasured(
                Input, Width, MixedWidth).IsEmpty());
    }
    auto ZeroMeasure = [](const FString&) { return 0.f; };
    auto NegativeMeasure = [](const FString&) { return -2.f; };
    auto NanMeasure = [NaN](const FString&) { return NaN; };
    TestTrue(TEXT("Zero glyph metric stops instead of infinite wrapping"),
        FAIHeroesAdvisorDossierRules::WrapMeasured(
            Input, 120.f, ZeroMeasure).IsEmpty());
    TestTrue(TEXT("Negative glyph metric stops safely"),
        FAIHeroesAdvisorDossierRules::WrapMeasured(
            Input, 120.f, NegativeMeasure).IsEmpty());
    TestTrue(TEXT("NaN glyph metric stops safely"),
        FAIHeroesAdvisorDossierRules::WrapMeasured(
            Input, 120.f, NanMeasure).IsEmpty());
    TestTrue(TEXT("Text narrower than one actual glyph fails closed"),
        FAIHeroesAdvisorDossierRules::WrapMeasured(
            {TEXT("Ш")}, 2.f, MixedWidth).IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FAIHeroesDossierMeasuredLimit,
    "AIHeroes.Dossier.MeasuredReportsRetainLimitsAndPages",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FAIHeroesDossierMeasuredLimit::RunTest(const FString&)
{
    const FString TooLong=FString::ChrN(
        FAIHeroesAdvisorDossierRules::MaxCharacters+37,TCHAR('X'));
    const auto Limited=FAIHeroesAdvisorDossierRules::WrapMeasured(
        {TooLong}, 200.f, MixedWidth);
    TestTrue(TEXT("Long input explicitly displays report cap"),
        Limited.Contains(FString(TEXT("[REPORT LIMIT]"))));
    for (const FString& Line:Limited)
        if (!Line.IsEmpty())
            TestTrue(TEXT("Limit marker and report are measured too"),
                MixedWidth(Line)<=200.f);
    TArray<FString> Many;
    Many.Init(TEXT("paragraph"),65);
    const auto Extra=FAIHeroesAdvisorDossierRules::WrapMeasured(
        Many, 200.f, MixedWidth);
    TestTrue(TEXT("Paragraph limit stays visible on the last page"),
        Extra.Contains(FString(TEXT("[REPORT LIMIT]"))));
    TestTrue(TEXT("Reflow never drops the previous page's report lines"),
        FAIHeroesAdvisorDossierRules::PageCount(Extra.Num(),5)>1);
    return true;
}
#endif
