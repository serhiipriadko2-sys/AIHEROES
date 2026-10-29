#include "AIHeroesHUD.h"

#include "AIHeroesGameMode.h"
#include "AIHeroesHUDTextLayout.h"
#include "AIHeroesVictoryPresentation.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

void AAIHeroesHUD::DrawCampaignVictoryBanner(const AAIHeroesGameMode& Rules)
{
    if (!Canvas || !GEngine || Rules.IsAdvisorDossierVisible() ||
        Rules.IsBasePanelVisible() || Rules.IsLiveBattleActive() ||
        Rules.IsBattleReplayActive())
    {
        return;
    }

    const FAIHeroesHUDActionRect R =
        FAIHeroesVictoryPresentation::Banner(
            Rules.GetCampaignProgress(), CachedCanvasSize,
            bExplorerMenuOpen);
    if (R.W <= 0.f || R.H <= 0.f)
    {
        return;
    }

    // Main campaign only. This overlay cannot unlock, move, pay, save,
    // alter fog or issue any order. Clicks open the existing read-only
    // objectives dossier (action 202); no click reaches the map below.
    const FLinearColor Background(.017f, .052f, .070f, .97f);
    const FLinearColor Gold(.95f, .72f, .34f, 1.f);
    const FLinearColor Teal(.43f, .89f, .77f, 1.f);
    const FLinearColor Paper(.95f, .95f, .89f, 1.f);
    const FLinearColor Muted(.68f, .77f, .75f, 1.f);

    DrawRect(Background, R.X, R.Y, R.W, R.H);
    DrawRect(Gold, R.X, R.Y, R.W, 2.f);
    DrawRect(Gold, R.X, R.Y + R.H - 2.f, R.W, 2.f);
    DrawRect(Gold, R.X, R.Y, 2.f, R.H);
    DrawRect(Gold, R.X + R.W - 2.f, R.Y, 2.f, R.H);
    DrawRect(Teal, R.X + 3.f, R.Y + 3.f, 3.f, R.H - 6.f);

    // A small original gold compass/diamond marker anchors the accolade.
    const float X = R.X + 27.f;
    const float Y = R.Y + 36.f;
    DrawLine(X, Y - 18.f, X + 15.f, Y, Gold, 2.0f);
    DrawLine(X + 15.f, Y, X, Y + 18.f, Gold, 2.0f);
    DrawLine(X, Y + 18.f, X - 15.f, Y, Gold, 2.0f);
    DrawLine(X - 15.f, Y, X, Y - 18.f, Gold, 2.0f);
    DrawRect(Teal, X - 2.f, Y - 2.f, 4.f, 4.f);

    UFont* Font = GEngine->GetSmallFont();
    if (!Font)
    {
        return;
    }
    const float TextX = R.X + 57.f;
    const float TextWidth = R.W - 75.f;
    auto Write = [this, Font, TextX, TextWidth](
        const FString& Text, float Y, float FontScale,
        const FLinearColor& Color)
    {
        const auto Measure = [this, Font, FontScale](const FString& Candidate)
        {
            float W = 0.f, H = 0.f;
            GetTextSize(Candidate, W, H, Font, FontScale);
            return W;
        };
        const FString Line = FAIHeroesHUDTextLayout::FitLine(
            Text, TextWidth, Measure);
        if (!Line.IsEmpty())
        {
            DrawText(Line, Color, TextX, Y, Font, FontScale);
        }
    };

    Write(TEXT("КАМПАНИЯ ЗАВЕРШЕНА"), R.Y + 10.f, 1.20f, Gold);
    Write(TEXT("Все 4 цели выполнены. Рубежи под защитой Искры."),
        R.Y + 33.f, 0.99f, Paper);
    Write(TEXT("Нажмите: итоги [O]   •   Сохранить [K]   •   Играть дальше"),
        R.Y + 58.f, 0.88f, Muted);
}
