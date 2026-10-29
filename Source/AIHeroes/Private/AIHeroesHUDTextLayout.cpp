#include "AIHeroesHUDTextLayout.h"
#include "AIHeroesHUDLayout.h"

FString FAIHeroesHUDTextLayout::FitLine(const FString& Text, float MaxWidth,
    TFunctionRef<float(const FString&)> Measure)
{
    if (!FMath::IsFinite(MaxWidth) || MaxWidth <= 0.f || MaxWidth > 32768.f ||
        Text.IsEmpty() || Text.Len() > MaxSourceChars) return FString();
    FString Clean = Text;
    for (int32 I = 0; I < Clean.Len(); ++I)
    {
        const uint32 C = static_cast<uint32>(Clean[I]);
        if (C < 0x20 || C == 0x7f || C == 0x2028 || C == 0x2029) Clean[I] = TCHAR(' ');
    }
    const float FullWidth = Measure(Clean);
    if (!FMath::IsFinite(FullWidth) || FullWidth < 0.f) return FString();
    if (FullWidth <= MaxWidth) return Clean;
    const FString Ellipsis(TEXT("..."));
    const float EllipsisWidth = Measure(Ellipsis);
    if (!FMath::IsFinite(EllipsisWidth) || EllipsisWidth < 0.f ||
        EllipsisWidth > MaxWidth) return FString();
    FString Best = Ellipsis;
    int32 Low = 0, High = Clean.Len() - 1;
    // At most 14 binary probes for the bounded source, plus final verification.
    while (Low <= High)
    {
        const int32 Mid = Low + (High - Low) / 2;
        int32 Cut = Mid;
        if (Cut > 0 && Cut < Clean.Len())
        {
            const uint32 Previous = static_cast<uint32>(Clean[Cut - 1]);
            const uint32 Next = static_cast<uint32>(Clean[Cut]);
            if (Previous >= 0xd800 && Previous <= 0xdbff &&
                Next >= 0xdc00 && Next <= 0xdfff) --Cut;
        }
        const FString Candidate = Clean.Left(Cut).TrimEnd() + Ellipsis;
        const float CandidateWidth = Measure(Candidate);
        if (!FMath::IsFinite(CandidateWidth) || CandidateWidth < 0.f) return FString();
        if (CandidateWidth <= MaxWidth)
        {
            Best = Candidate;
            Low = Mid + 1;
        }
        else High = Mid - 1;
    }
    const float Verified = Measure(Best);
    return FMath::IsFinite(Verified) && Verified >= 0.f && Verified <= MaxWidth
        ? Best : FString();
}

float FAIHeroesHUDTextLayout::HeaderLineWidth(
    const FVector2D& Viewport, float Y, float TextHeight)
{
    if (!FMath::IsFinite(Viewport.X) || !FMath::IsFinite(Viewport.Y) ||
        Viewport.X < 320.0 || Viewport.X > 16384.0 ||
        Viewport.Y < 240.0 || Viewport.Y > 16384.0 ||
        !FMath::IsFinite(Y) || !FMath::IsFinite(TextHeight) ||
        Y < 0.f || TextHeight <= 0.f || Y + TextHeight > Viewport.Y) return 0.f;
    float Right = static_cast<float>(Viewport.X) - 18.f;
    for (int32 Action = 7; Action <= 11; ++Action)
    {
        const auto R = FAIHeroesHUDLayout::Rect(Action, Viewport);
        if (R.W > 0.f && R.H > 0.f && Y < R.Y + R.H && Y + TextHeight > R.Y)
            Right = FMath::Min(Right, R.X - 8.f);
    }
    return FMath::Max(0.f, Right - 18.f);
}
