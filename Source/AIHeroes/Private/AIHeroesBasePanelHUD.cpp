#include "AIHeroesHUD.h"
#include "AIHeroesBasePanel.h"
#include "AIHeroesBasePanelCopy.h"
#include "AIHeroesGameMode.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

void AAIHeroesHUD::DrawBasePanel(const AAIHeroesGameMode& Rules)
{
    if (!Canvas || !GEngine) return;
    UFont* Font = GEngine->GetSmallFont();
    const auto P = FAIHeroesBasePanelRules::Panel(CachedCanvasSize);
    DrawRect(FLinearColor(0.f, 0.f, 0.f, 0.86f), 0, 0, CachedCanvasSize.X, CachedCanvasSize.Y);
    if (P.W <= 0)
    {
        DrawText(TEXT("Окно слишком маленькое. Нажмите G или Esc, чтобы закрыть."),
            FLinearColor::White, 8, 8, Font, 0.8f);
        return;
    }
    const float Scale = P.W < 600.f ? 0.82f : 1.f;
    // Measure real glyph widths and elide with an explicit marker. Prices
    // are short; long explanations never overlap adjacent controls.
    auto Text = [this, Font](FString Value, const FLinearColor& Color,
        float X, float Y, float MaxWidth, float S)
    {
        float W = 0.f, H = 0.f;
        GetTextSize(Value, W, H, Font, S);
        if (W > MaxWidth)
        {
            while (Value.Len() > 1)
            {
                Value.LeftChopInline(1);
                GetTextSize(Value + TEXT("..."), W, H, Font, S);
                if (W <= MaxWidth) break;
            }
            Value += TEXT("...");
        }
        DrawText(Value, Color, X, Y, Font, S);
    };
    DrawRect(FLinearColor(0.025f, 0.045f, 0.075f, 1.f), P.X, P.Y, P.W, P.H);
    Text(TEXT("ЦИТАДЕЛЬ | РЕСУРСЫ · АРМИЯ · ГЕРОЙ"), FLinearColor(0.5f, 0.92f, 1.f),
        P.X + 16, P.Y + 13, P.W - 32, Scale * 1.15f);
    Text(FString::Printf(TEXT("ЗОЛОТО %d   ДЕРЕВО %d   РУДА %d   ДАННЫЕ %d"),
        Rules.Gold, Rules.Wood, Rules.Ore, Rules.DataFragments), FLinearColor::White,
        P.X + 16, P.Y + 39, P.W - 32, Scale);
    const bool bCompact = P.W < 760.f;
    const FString BaseStatus = bCompact
        ? FString::Printf(TEXT("БАЗА %d | +%d ЗОЛ./ДЕНЬ | ТАКТИКА %d/3"),
            Rules.BaseTier, Rules.GetDailyGoldIncome(), Rules.ProtocolTier)
        : FString::Printf(TEXT("БАЗА %d | +%d ЗОЛ./ДЕНЬ | ЗАПАС: %d ОПОЛЧ., %d РАЗВ. | ТАКТИКА %d/3 | ИИ: ПРАВИЛА"),
            Rules.BaseTier, Rules.GetDailyGoldIncome(),
            Rules.GetAvailableMilitia(), Rules.GetAvailableScouts(), Rules.ProtocolTier);
    Text(BaseStatus, FLinearColor(0.93f, 0.81f, 0.45f),
        P.X + 16, P.Y + 62, P.W - 32, Scale * 0.85f);
    Text(bCompact ? TEXT("Выберите улучшение | Enter — подтвердить | Esc/G — закрыть")
        : TEXT("Мышь/касание или U/1/2/P/M | Enter — подтвердить | Esc/G — закрыть"),
        FLinearColor(0.66f, 0.75f, 0.85f),
        P.X + 16, P.Y + 82, P.W - 32, Scale * 0.8f);

    const auto Selected = Rules.GetSelectedBasePurchase();
    for (int32 I = 0; I < FAIHeroesBasePanelRules::OfferCount; ++I)
    {
        const auto Kind = static_cast<EAIHeroesBasePurchase>(I);
        const auto Offer = Rules.GetBasePanelOffer(Kind);
        const auto R = FAIHeroesBasePanelRules::Button(FAIHeroesBasePanelRules::FirstOfferAction + I, CachedCanvasSize);
        const bool bSelected = Selected == Kind;
        DrawRect(bSelected ? FLinearColor(0.06f, 0.26f, 0.31f, 1.f) :
            FLinearColor(0.045f, 0.09f, 0.14f, 1.f), R.X, R.Y, R.W, R.H);
        DrawRect(bSelected ? FLinearColor(0.38f, 0.95f, 0.88f) :
            FLinearColor(0.2f, 0.34f, 0.4f), R.X, R.Y, 3, R.H);
        const float LineScale = R.H < 42 ? Scale * 0.85f : Scale;
        Text(FAIHeroesBasePanelCopy::Title(Kind, Rules.BaseTier,
                Rules.ProtocolTier, Rules.ModelSlotTier),
            Offer.CanBuy() ? FLinearColor::White : FLinearColor(0.64f, 0.7f, 0.76f),
            R.X + 12, R.Y + 5, R.W - 24, LineScale);
        Text(FAIHeroesBasePanelCopy::Price(Offer) + TEXT(" | ") +
                FAIHeroesBasePanelCopy::Status(Offer),
            Offer.CanBuy() ? FLinearColor(0.55f, 0.91f, 0.72f) : FLinearColor(0.95f, 0.69f, 0.4f),
            R.X + 12, R.Y + 23, R.W - 24, LineScale * 0.85f);
        if (R.H > 61)
            Text(FAIHeroesBasePanelCopy::Detail(Kind,
                    Rules.GetAvailableMilitia(), Rules.GetAvailableScouts()),
                FLinearColor(0.7f, 0.79f, 0.87f),
                R.X + 12, R.Y + 46, R.W - 24, Scale * 0.85f);
    }
    const bool bConfirm = Rules.CanConfirmBasePurchase();
    const auto SelectedOffer = Rules.GetBasePanelOffer(Selected);
    FString SelectedHint;
    if (Selected == EAIHeroesBasePurchase::None)
        SelectedHint = TEXT("Выберите улучшение. Покупка потребует подтверждения.");
    else if (SelectedOffer.CanBuy())
        SelectedHint = FAIHeroesBasePanelCopy::Detail(Selected,
            Rules.GetAvailableMilitia(), Rules.GetAvailableScouts());
    else
        SelectedHint = FAIHeroesBasePanelCopy::Reason(SelectedOffer);
    Text(SelectedHint, FLinearColor(0.73f, 0.84f, 0.94f),
        P.X + 16, P.Y + P.H - 113, P.W - 32, Scale * 0.85f);
    Text(FAIHeroesBasePanelCopy::Message(
            Rules.GetBasePanelMessage(), SelectedOffer),
        FLinearColor(0.98f, 0.84f, 0.47f),
        P.X + 16, P.Y + P.H - 90, P.W - 32, Scale * 0.9f);
    Text(bConfirm ? TEXT("ПОДТВЕРЖДЕНИЕ | Эта покупка выполнится только один раз.") :
        TEXT("БЕЗ ПОКУПКИ | Выберите доступное улучшение."),
        FLinearColor(0.62f, 0.78f, 0.87f), P.X + 16, P.Y + P.H - 68, P.W - 32, Scale * 0.8f);
    for (const int32 Action : {FAIHeroesBasePanelRules::ConfirmAction, FAIHeroesBasePanelRules::CloseAction})
    {
        const auto R = FAIHeroesBasePanelRules::Button(Action, CachedCanvasSize);
        const bool bEnabled = Action == FAIHeroesBasePanelRules::CloseAction || bConfirm;
        DrawRect(bEnabled ? FLinearColor(0.07f, 0.3f, 0.33f) : FLinearColor(0.09f, 0.12f, 0.15f),
            R.X, R.Y, R.W, R.H);
        Text(Action == FAIHeroesBasePanelRules::CloseAction ? TEXT("ЗАКРЫТЬ / ESC") : TEXT("КУПИТЬ / ENTER"),
            bEnabled ? FLinearColor::White : FLinearColor(0.4f, 0.45f, 0.5f),
            R.X + 10, R.Y + 10, R.W - 20, Scale);
    }
}
