#include "AIHeroesAdvisorDossier.h"

// Pixel-exact, read-only line breaking for the existing dossier.
// No advice text, price, state or action is changed. The font metric is
// supplied by the real HUD, so Cyrillic and proportional glyphs fit.
TArray<FString> FAIHeroesAdvisorDossierRules::WrapMeasured(
    const TArray<FString>& Paragraphs, float MaxPixelWidth,
    TFunctionRef<float(const FString&)> Measure)
{
    if (!FMath::IsFinite(MaxPixelWidth) || MaxPixelWidth <= 0.f ||
        MaxPixelWidth > 32768.f)
        return {};

    TArray<FString> Output;
    int32 RemainingCharacters = MaxCharacters;
    bool bLimited = Paragraphs.Num() > 64;

    // A max-12k-character source bounds both the loop and all font queries.
    // Never split a UTF-16 surrogate pair between two drawn lines.
    auto SafeLength = [](const FString& S, int32 Start, int32 Desired) -> int32
    {
        if (Desired > 0 && Start + Desired < S.Len())
        {
            const uint32 A = static_cast<uint32>(S[Start + Desired - 1]);
            const uint32 B = static_cast<uint32>(S[Start + Desired]);
            if (A >= 0xd800 && A <= 0xdbff &&
                B >= 0xdc00 && B <= 0xdfff)
                return Desired - 1;
        }
        return Desired;
    };
    auto ValidMeasure = [&Measure](const FString& Text, float& Out) -> bool
    {
        Out = Measure(Text);
        // A zero/negative font metric for visible letters cannot be used to
        // establish that a line is inside the viewport.
        return FMath::IsFinite(Out) && Out > 0.f;
    };
    auto Reflow = [&](const FString& Line) -> bool
    {
        int32 Start = 0;
        while (Start < Line.Len())
        {
            const int32 Remaining = Line.Len() - Start;
            const FString All = Line.Mid(Start);
            float AllWidth = 0.f;
            if (!ValidMeasure(All, AllWidth)) return false;
            if (AllWidth <= MaxPixelWidth)
            {
                Output.Add(All);
                return true;
            }

            int32 Low = 1, High = Remaining, Best = 0;
            while (Low <= High)
            {
                const int32 Mid = Low + (High - Low) / 2;
                const int32 Safe = SafeLength(Line, Start, Mid);
                if (Safe <= 0)
                {
                    Low = Mid + 1;
                    continue;
                }
                float Width = 0.f;
                if (!ValidMeasure(Line.Mid(Start, Safe), Width)) return false;
                if (Width <= MaxPixelWidth)
                {
                    Best = Safe;
                    Low = Mid + 1;
                }
                else High = Mid - 1;
            }
            if (Best <= 0) return false;

            int32 Take = Best;
            // Prefer a complete word if a whitespace boundary occurs inside
            // the measured prefix. Long Russian or English tokens still split.
            for (int32 I = Start + Best - 1; I > Start; --I)
            {
                if (FChar::IsWhitespace(Line[I]))
                {
                    Take = I - Start;
                    break;
                }
            }
            if (Take <= 0) Take = Best;
            FString Piece = Line.Mid(Start, Take);
            Piece.TrimEndInline();
            if (Piece.IsEmpty()) return false;
            float PieceWidth = 0.f;
            if (!ValidMeasure(Piece, PieceWidth) ||
                PieceWidth > MaxPixelWidth) return false;
            Output.Add(MoveTemp(Piece));
            Start += Take;
            while (Start < Line.Len() && FChar::IsWhitespace(Line[Start]))
                ++Start;
        }
        return true;
    };

    const int32 Count = FMath::Min(Paragraphs.Num(), 64);
    for (int32 P = 0; P < Count; ++P)
    {
        if (RemainingCharacters <= 0) { bLimited = true; break; }
        FString Line = Paragraphs[P];
        if (Line.Len() > RemainingCharacters)
        {
            Line.LeftInline(RemainingCharacters);
            bLimited = true;
        }
        RemainingCharacters -= Line.Len();
        for (int32 I = 0; I < Line.Len(); ++I)
        {
            const uint32 C = static_cast<uint32>(Line[I]);
            if (FChar::IsWhitespace(Line[I]) || C < 0x20 ||
                C == 0x7f || C == 0x2028 || C == 0x2029)
                Line[I] = TCHAR(' ');
        }
        Line.TrimStartAndEndInline();
        if (Line.IsEmpty()) continue;
        if (!Output.IsEmpty()) Output.Add(FString());
        if (!Reflow(Line)) return {};
    }
    if (bLimited)
    {
        if (!Output.IsEmpty()) Output.Add(FString());
        if (!Reflow(TEXT("[REPORT LIMIT]"))) return {};
    }
    return Output;
}
