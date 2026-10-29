#include "AIHeroesBasePanelCopy.h"

FString FAIHeroesBasePanelCopy::Title(EAIHeroesBasePurchase Kind, int32 BaseTier,
    int32 ProtocolTier, int32 ModelSlotTier)
{
    switch (Kind)
    {
        case EAIHeroesBasePurchase::Headquarters:
            if (BaseTier >= 3) return TEXT("ЦИТАДЕЛЬ: МАКСИМУМ");
            return FString::Printf(TEXT("ЦИТАДЕЛЬ: УРОВЕНЬ %d → %d"),
                BaseTier, FMath::Min(3, BaseTier + 1));
        case EAIHeroesBasePurchase::Militia:
            return TEXT("АРМИЯ: 5 ОПОЛЧЕНЦЕВ");
        case EAIHeroesBasePurchase::Scouts:
            return TEXT("АРМИЯ: 2 РАЗВЕДЧИКА");
        case EAIHeroesBasePurchase::Protocol:
        {
            const TCHAR* Names[] = {
                TEXT("АНАЛИЗ УГРОЗ"), TEXT("ПАМЯТЬ ГЕРОЯ"),
                TEXT("ТАКТИЧЕСКИЙ ПЛАН")
            };
            if (ProtocolTier >= 0 && ProtocolTier < 3)
                return FString(TEXT("ПРОТОКОЛ: ")) + Names[ProtocolTier];
            return TEXT("ПРОТОКОЛЫ: МАКСИМУМ");
        }
        case EAIHeroesBasePurchase::ModelSlot:
            return ModelSlotTier == 0 ? TEXT("СЛОТ МОДЕЛИ: 3B") :
                ModelSlotTier == 1 ? TEXT("СЛОТ МОДЕЛИ: 8B") :
                TEXT("СЛОТ МОДЕЛИ: МАКСИМУМ");
        default:
            return TEXT("НЕИЗВЕСТНОЕ УЛУЧШЕНИЕ");
    }
}

FString FAIHeroesBasePanelCopy::Detail(EAIHeroesBasePurchase Kind,
    int32 MilitiaReserve, int32 ScoutReserve)
{
    switch (Kind)
    {
        case EAIHeroesBasePurchase::Headquarters:
            return TEXT("Следующий день: +20 золота к доходу. Сила бойцов не меняется.");
        case EAIHeroesBasePurchase::Militia:
            return FString::Printf(TEXT("12 ОЗ, атака 4, защита 2, инициатива 3. Запас: %d."),
                MilitiaReserve);
        case EAIHeroesBasePurchase::Scouts:
            return FString::Printf(TEXT("Стрелки: 8 ОЗ, атака 6, защита 1, инициатива 7. Запас: %d."),
                ScoutReserve);
        case EAIHeroesBasePurchase::Protocol:
            return TEXT("Меняет офлайн-тактику героя. Нейромодель не устанавливается.");
        case EAIHeroesBasePurchase::ModelSlot:
            return TEXT("Игровой слот 3B/8B: без нейровесов и бонуса к урону.");
        default:
            return TEXT("Описание недоступно.");
    }
}

FString FAIHeroesBasePanelCopy::Price(const FAIHeroesBasePanelOffer& Offer)
{
    return FString::Printf(TEXT("%d зол. / %d дер. / %d руд. / %d дан."),
        Offer.Gold, Offer.Wood, Offer.Ore, Offer.Data);
}

FString FAIHeroesBasePanelCopy::Reason(const FAIHeroesBasePanelOffer& Offer)
{
    const FString& R = Offer.BlockReason;
    if (R.IsEmpty()) return FString();
    if (R == TEXT("Maximum headquarters level reached."))
        return TEXT("Цитадель уже максимального уровня.");
    if (R == TEXT("Select owned HQ and return the hero there first."))
        return TEXT("Выберите свою цитадель и верните туда героя.");
    if (R == TEXT("No recruits in stock. Next growth: days 8, 15, 22..."))
        return TEXT("Войск пока нет в запасе. Новый набор: дни 8, 15, 22...");
    if (R == TEXT("Army stack limit reached. Nothing will be spent."))
        return TEXT("Лимит отряда достигнут. Ресурсы не потрачены.");
    if (R == TEXT("All three prototype protocols are unlocked."))
        return TEXT("Все три игровых протокола уже открыты.");
    if (R == TEXT("Highest prototype model slot already unlocked."))
        return TEXT("Максимальный игровой слот модели уже открыт.");
    if (R == TEXT("Invalid campaign data. Purchase blocked."))
        return TEXT("Неверные данные кампании. Покупка запрещена.");
    if (R == TEXT("Not enough resources. No partial purchase."))
        return TEXT("Не хватает ресурсов. Частичная покупка невозможна.");
    return TEXT("Покупка недоступна. Ресурсы не списаны.");
}

FString FAIHeroesBasePanelCopy::Status(const FAIHeroesBasePanelOffer& Offer)
{
    if (Offer.CanBuy()) return TEXT("ДОСТУПНО — ВЫБРАТЬ");
    if (!Offer.bAvailable) return TEXT("НЕДОСТУПНО");
    return TEXT("ПОКА НЕЛЬЗЯ — ВЫБРАТЬ ДЛЯ ПОЯСНЕНИЯ");
}

FString FAIHeroesBasePanelCopy::Message(const FString& Raw,
    const FAIHeroesBasePanelOffer& Selected)
{
    if (Raw == TEXT("Select a card to review it. Nothing is purchased until CONFIRM."))
        return TEXT("Выберите улучшение. Покупка только после подтверждения.");
    if (Raw == TEXT("Unknown purchase rejected. Nothing spent."))
        return TEXT("Неизвестное действие. Ресурсы не списаны.");
    if (Raw == TEXT("Invalid map. Purchase blocked."))
        return TEXT("Ошибка карты. Покупка заблокирована.");
    if (Raw == TEXT("No current quote. Select a card again; nothing spent."))
        return TEXT("Выберите улучшение заново. Ресурсы не списаны.");
    if (Raw == TEXT("PURCHASE APPLIED ONCE. Select another card to continue."))
        return TEXT("Покупка выполнена. Выберите следующее улучшение.");
    if (!Selected.BlockReason.IsEmpty() && Raw == Selected.BlockReason)
        return Reason(Selected);
    if (Raw.StartsWith(TEXT("AFTER PURCHASE: ")) && Selected.CanBuy())
    {
        // Display the authoritative preview amounts, never recompute a
        // second quote from mutable UI state.
        FString Display = Raw;
        Display.ReplaceInline(TEXT("AFTER PURCHASE: "), TEXT("После покупки: "));
        Display.ReplaceInline(TEXT(" G / "), TEXT(" зол. / "));
        Display.ReplaceInline(TEXT(" W / "), TEXT(" дер. / "));
        Display.ReplaceInline(TEXT(" O / "), TEXT(" руд. / "));
        Display.ReplaceInline(TEXT(" DATA"), TEXT(" дан."));
        return Display;
    }
    // Unknown events remain verbatim to avoid hiding actual failure details.
    return Raw;
}
