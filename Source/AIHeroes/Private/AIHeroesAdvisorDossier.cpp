#include "AIHeroesAdvisorDossier.h"

TArray<FString> FAIHeroesAdvisorDossierRules::Wrap(
    const TArray<FString>& Paragraphs, int32 Columns)
{
    TArray<FString> Lines;
    const int32 Width = FMath::Clamp(Columns, 16, 160);
    int32 Remaining = MaxCharacters;
    bool bLimited = Paragraphs.Num() > 64;
    const int32 ParagraphCount = FMath::Min(64, Paragraphs.Num());
    for (int32 P = 0; P < ParagraphCount; ++P)
    {
        if (Remaining <= 0) { bLimited = true; break; }
        FString Text = Paragraphs[P];
        if (Text.Len() > Remaining)
        {
            Text.LeftInline(Remaining);
            bLimited = true;
        }
        Remaining -= Text.Len();
        for (int32 I = 0; I < Text.Len(); ++I)
        {
            if (FChar::IsWhitespace(Text[I]) || Text[I] < 32) Text[I] = TEXT(' ');
        }
        Text.TrimStartAndEndInline();
        if (Text.IsEmpty()) continue;
        if (!Lines.IsEmpty()) Lines.Add(FString());
        int32 Start = 0;
        while (Start < Text.Len())
        {
            const int32 Available = Text.Len() - Start;
            int32 Take = FMath::Min(Width, Available);
            if (Available > Width)
            {
                int32 Boundary = Take;
                while (Boundary > 0 && !FChar::IsWhitespace(Text[Start + Boundary])) --Boundary;
                if (Boundary > 0) Take = Boundary;
            }
            FString Line = Text.Mid(Start, Take);
            Line.TrimEndInline();
            Lines.Add(MoveTemp(Line));
            Start += Take;
            while (Start < Text.Len() && FChar::IsWhitespace(Text[Start])) ++Start;
        }
    }
    if (bLimited)
    {
        Lines.Add(TEXT("[REPORT LIMIT]"));
    }
    return Lines;
}

FString FAIHeroesAdvisorDossierRules::DisplayTitle(
    const FAIHeroesAdvisorDossier& Dossier)
{
    if (Dossier.bObjectives) return TEXT("ЦЕЛИ КАМПАНИИ");
    if (Dossier.bRecruitment) return TEXT("СОВЕТ ПО АРМИИ");
    if (Dossier.bProtocolLab) return TEXT("СРАВНЕНИЕ ТАКТИК");
    return Dossier.Title;
}

FString FAIHeroesAdvisorDossierRules::FreshnessLabel(bool bFresh)
{
    return bFresh ? TEXT("ДАННЫЕ АКТУАЛЬНЫ") : TEXT("ДАННЫЕ УСТАРЕЛИ — ОБНОВИТЕ");
}

FString FAIHeroesAdvisorDossierRules::ActionLabel(int32 ActionId)
{
    switch (ActionId)
    {
        case PreviousAction: return TEXT("НАЗАД");
        case NextAction: return TEXT("ДАЛЕЕ");
        case RefreshAction: return TEXT("ОБНОВИТЬ");
        case CloseAction: return TEXT("ЗАКРЫТЬ");
        default: return FString();
    }
}

int32 FAIHeroesAdvisorDossierRules::PageCount(int32 LineCount, int32 RowsPerPage)
{
    const int64 Rows = FMath::Max(1, RowsPerPage);
    return static_cast<int32>(FMath::Max<int64>(1,
        (int64(FMath::Max(0, LineCount)) + Rows - 1) / Rows));
}

int32 FAIHeroesAdvisorDossierRules::ClampPage(int32 Page, int32 Count)
{
    return FMath::Clamp(Page, 0, FMath::Max(0, Count - 1));
}

FAIHeroesDossierLayout FAIHeroesAdvisorDossierRules::Layout(const FVector2D& Viewport)
{
    FAIHeroesDossierLayout Out;
    if (!FMath::IsFinite(Viewport.X) || !FMath::IsFinite(Viewport.Y) ||
        Viewport.X < 320.f || Viewport.Y < 240.f ||
        Viewport.X > 100000.f || Viewport.Y > 100000.f) return Out;
    Out.Panel.W = FMath::Min(1000.f, static_cast<float>(Viewport.X) - 24.f);
    Out.Panel.H = FMath::Min(600.f, static_cast<float>(Viewport.Y) - 24.f);
    Out.Panel.X = (Viewport.X - Out.Panel.W) * 0.5f;
    Out.Panel.Y = (Viewport.Y - Out.Panel.H) * 0.5f;
    Out.TextX = Out.Panel.X + 20.f;
    Out.TextY = Out.Panel.Y + 102.f;
    Out.TextWidth = Out.Panel.W - 40.f;
    Out.RowsPerPage = FMath::Max(1, FMath::FloorToInt((Out.Panel.H - 204.f) / Out.RowHeight));
    return Out;
}

FAIHeroesHUDActionRect FAIHeroesAdvisorDossierRules::Button(
    int32 ActionId, const FVector2D& Viewport)
{
    FAIHeroesHUDActionRect Out;
    const FAIHeroesDossierLayout L = Layout(Viewport);
    if (ActionId < PreviousAction || ActionId > CloseAction || L.Panel.W <= 0.f) return Out;
    Out.W = (L.Panel.W - 64.f) / 4.f;
    Out.H = 36.f;
    Out.X = L.Panel.X + 20.f + (ActionId - PreviousAction) * (Out.W + 8.f);
    Out.Y = L.Panel.Y + L.Panel.H - 52.f;
    return Out;
}

FAIHeroesHUDActionRect FAIHeroesAdvisorDossierRules::OpenButton(const FVector2D& Viewport)
{
    FAIHeroesHUDActionRect Out;
    if (Layout(Viewport).Panel.W <= 0.f) return Out;
    Out.W = 124.f;
    Out.H = 27.f;
    Out.X = Viewport.X - Out.W - 14.f;
    Out.Y = Viewport.Y - FAIHeroesHUDLayout::BottomPanelHeight(Viewport.X) + 80.f;
    return Out;
}

int32 FAIHeroesAdvisorDossierRules::HitTest(
    const FVector2D& Point, const FVector2D& Viewport)
{
    for (int32 Action = PreviousAction; Action <= CloseAction; ++Action)
    {
        if (Button(Action, Viewport).Contains(Point)) return Action;
    }
    return ConsumeAction;
}
