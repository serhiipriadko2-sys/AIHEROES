#include "AIHeroesHUD.h"

#include "AIHeroesAdvisorDossier.h"
#include "AIHeroesGameMode.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"

void AAIHeroesHUD::PageAdvisorDossier(int32 Direction)
{
    if (AAIHeroesGameMode* Rules = Cast<AAIHeroesGameMode>(UGameplayStatics::GetGameMode(this)))
        Rules->ChangeAdvisorDossierPage(Direction, CachedDossierPageCount);
}

void AAIHeroesHUD::DrawAdvisorDossier(const AAIHeroesGameMode& Rules)
{
    if (!Canvas || !GEngine) return;
    const auto L = FAIHeroesAdvisorDossierRules::Layout(CachedCanvasSize);
    if (L.Panel.W <= 0.f) return;
    UFont* Font = GEngine->GetSmallFont();
    const float Scale = CachedCanvasSize.X < 700.f ? 0.9f : 1.f;
    if (!FMath::IsNearlyEqual(DossierFontScale, Scale))
    {
        // Generated report text is ASCII. Measure the widest actual glyph
        // rather than assuming all fonts/screens use eight pixels per letter.
        DossierGlyphWidth = 1.f;
        for (int32 Character = 32; Character < 127; ++Character)
        {
            float W = 0.f, H = 0.f;
            GetTextSize(FString::Chr(static_cast<TCHAR>(Character)), W, H, Font, Scale);
            DossierGlyphWidth = FMath::Max(DossierGlyphWidth, W);
        }
        DossierFontScale = Scale;
    }
    const int32 Columns = FMath::Clamp(
        FMath::FloorToInt(L.TextWidth / (DossierGlyphWidth + 1.f)), 16, 160);
    const auto& Dossier = Rules.GetAdvisorDossier();
    const TArray<FString> Lines = FAIHeroesAdvisorDossierRules::Wrap(Dossier.Paragraphs, Columns);
    CachedDossierPageCount = FAIHeroesAdvisorDossierRules::PageCount(Lines.Num(), L.RowsPerPage);
    const int32 Page = FAIHeroesAdvisorDossierRules::ClampPage(
        Rules.GetAdvisorDossierPage(), CachedDossierPageCount);
    const bool bFresh = Rules.IsAdvisorDossierFresh();

    DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.8f), 0.f, 0.f, CachedCanvasSize.X, CachedCanvasSize.Y);
    DrawRect(FLinearColor(0.025f, 0.045f, 0.075f, 1.f),
        L.Panel.X, L.Panel.Y, L.Panel.W, L.Panel.H);
    DrawText(Dossier.Title, FLinearColor(0.48f, 0.92f, 1.f),
        L.TextX, L.Panel.Y + 16.f, Font, Scale * 1.15f);
    DrawText(bFresh ? TEXT("CURRENT SNAPSHOT") : TEXT("STALE - REFRESH REQUIRED"),
        bFresh ? FLinearColor(0.48f, 1.f, 0.73f) : FLinearColor(1.f, 0.63f, 0.32f),
        L.TextX, L.Panel.Y + 47.f, Font, Scale);
    DrawText(TEXT("Read-only. Close before giving orders."),
        FLinearColor(0.75f, 0.82f, 0.9f), L.TextX, L.Panel.Y + 72.f, Font, Scale * 0.85f);

    for (int32 Row = 0; Row < L.RowsPerPage; ++Row)
    {
        const int32 Index = Page * L.RowsPerPage + Row;
        if (!Lines.IsValidIndex(Index)) break;
        DrawText(Lines[Index], bFresh ? FLinearColor(0.91f, 0.95f, 1.f) :
            FLinearColor(0.68f, 0.73f, 0.8f),
            L.TextX, L.TextY + Row * L.RowHeight, Font, Scale);
    }
    DrawText(FString::Printf(TEXT("PAGE %d/%d | PgUp / PgDn"), Page + 1, CachedDossierPageCount),
        FLinearColor(0.75f, 0.85f, 0.94f), L.TextX,
        L.Panel.Y + L.Panel.H - 82.f, Font, Scale * 0.85f);
    const TCHAR* Labels[] = {TEXT("PREV"), TEXT("NEXT"), TEXT("REFRESH"), TEXT("CLOSE")};
    for (int32 I = 0; I < 4; ++I)
    {
        const auto R = FAIHeroesAdvisorDossierRules::Button(
            FAIHeroesAdvisorDossierRules::PreviousAction + I, CachedCanvasSize);
        const bool bEnabled = I == 0 ? Page > 0 : I == 1 ? Page + 1 < CachedDossierPageCount : true;
        DrawRect(bEnabled ? FLinearColor(0.07f, 0.29f, 0.35f, 1.f) :
            FLinearColor(0.09f, 0.12f, 0.16f, 1.f), R.X, R.Y, R.W, R.H);
        DrawText(Labels[I], bEnabled ? FLinearColor::White : FLinearColor(0.45f, 0.5f, 0.55f),
            R.X + 8.f, R.Y + 10.f, Font, Scale * 0.9f);
    }
}
