#include "AIHeroesCampaignGuide.h"

namespace
{
FString LocalizedTarget(FName Id)
{
    if(Id==FName(TEXT("HQ")))return TEXT("Цитадель Искры");
    if(Id==FName(TEXT("FOREST")))return TEXT("Лесопилка");
    if(Id==FName(TEXT("ORE")))return TEXT("Рудник");
    if(Id==FName(TEXT("OUTPOST")))return TEXT("Сторожевая башня");
    if(Id==FName(TEXT("RUINS")))return TEXT("Древний архив");
    return FString();
}

bool IsMapTarget(FName Id)
{
    return Id==FName(TEXT("FOREST")) || Id==FName(TEXT("ORE")) ||
        Id==FName(TEXT("OUTPOST")) || Id==FName(TEXT("RUINS"));
}
}

bool FAIHeroesCampaignGuide::CanOfferOrder(
    bool bRouteReady, bool bHasArmy, bool bRequiresBattle)
{
    return bRouteReady && (bHasArmy || !bRequiresBattle);
}

FAIHeroesCampaignGuideCard FAIHeroesCampaignGuide::Build(
    const FAIHeroesCampaignProgress& Progress,
    const FAIHeroesCampaignGuideContext& Context)
{
    FAIHeroesCampaignGuideCard Result;
    Result.FocusId=Progress.NextFocusId;
    Result.bVictory=Progress.bVictory;
    const int32 Total=FMath::Clamp(Progress.Total,0,100);
    const int32 Completed=FMath::Clamp(Progress.Completed,0,Total);
    Result.Heading=FString::Printf(TEXT("ЦЕЛИ %d/%d  ·  ОТКРЫТЬ"),Completed,Total);
    if(Progress.bVictory)
    {
        Result.Heading=TEXT("ПОБЕДА  ·  ОТКРЫТЬ ЦЕЛИ");
        Result.NextStep=TEXT("Кампания завершена. Сохраните игру [K]");
        Result.Detail=TEXT("Территории освобождены, улучшения открыты.");
        return Result;
    }
    if(Total!=FAIHeroesCampaignObjectives::GoalCount ||
       Progress.NextFocusId.IsNone())
    {
        Result.NextStep=TEXT("Откройте полный список целей");
        Result.Detail=TEXT("Следующий шаг пока не определён.");
        Result.FocusId=NAME_None;
        return Result;
    }
    const FName Id=Progress.NextFocusId;
    if(Id==FName(TEXT("PROTOCOL")))
    {
        Result.NextStep=TEXT("Купите протокол в «Постройках» [G]");
        Result.Detail=TEXT("Покупка в «Постройках». «Тактики [Y]» — только анализ.");
        return Result;
    }
    if(Id==FName(TEXT("MODEL_SLOT")))
    {
        Result.NextStep=TEXT("Развивайте слот модели [G]");
        Result.Detail=TEXT("Откройте «Постройки» и улучшите слот модели.");
        return Result;
    }
    if(!IsMapTarget(Id))
    {
        Result.FocusId=NAME_None;
        Result.NextStep=TEXT("Откройте полный список целей");
        Result.Detail=TEXT("Неизвестный этап не запускает игровой приказ.");
        return Result;
    }
    const FString Target=LocalizedTarget(Id);
    if(!Context.bHasArmy && Context.bTargetGuarded)
    {
        Result.NextStep=TEXT("Армия пуста: наймите бойцов [G]");
        Result.Detail=TEXT("Сначала подготовьте армию на базе.");
    }
    else if(Context.bTravelMoving)
    {
        Result.NextStep=TEXT("Герой в пути к цели");
        Result.Detail=TEXT("Дождитесь завершения движения.");
    }
    else if(!Context.bTargetDiscovered)
    {
        Result.NextStep=TEXT("Разведайте путь к следующей цели");
        Result.Detail=TEXT("Ищите разведанные дороги; скрытую карту ИИ не раскрывает.");
    }
    else if(Context.SelectedId!=Id)
    {
        Result.NextStep=Target+TEXT(": выберите на карте");
        Result.Detail=TEXT("Затем «Спросить» и подтвердите поход.");
    }
    else if(CanOfferOrder(Context.bRouteReady, Context.bHasArmy, Context.bTargetGuarded))
    {
        Result.NextStep=Target+TEXT(": подтвердите поход");
        Result.Detail=TEXT("Вы решаете, куда идти. ИИ проводит бой.");
    }
    else if(Context.bNeedsMoreMoves &&
            Context.FullDayMoveBudget>0 &&
            Context.RouteTotalCost>Context.FullDayMoveBudget)
    {
        // End Day cannot help when the full route is longer than every
        // future day's allowance. The player must choose a legal staging
        // point manually. Never imply that a selected long path will auto-move.
        const FString Stop=LocalizedTarget(Context.IntermediateOwnedSite);
        if(!Stop.IsEmpty())
        {
            // The mission ribbon has a single 270px text line at 1280x720:
            // put the actual waypoint first, not after a clipped sentence.
            Result.NextStep=FString(TEXT("Через "))+Stop;
            Result.Detail=FString::Printf(
                TEXT("Путь %d > %d/день. Выберите эту точку."),
                Context.RouteTotalCost,Context.FullDayMoveBudget);
        }
        else if(Context.bOwnedStopAfterRest)
        {
            Result.NextStep=TEXT("Восстановите ходы для остановки");
            Result.Detail=TEXT("Конец хода, затем своя промежуточная точка. Не весь путь сразу.");
        }
        else
        {
            Result.NextStep=TEXT("Нужна остановка по пути");
            Result.Detail=FString::Printf(
                TEXT("Путь %d > %d/день. Ищите свою точку."),
                Context.RouteTotalCost,Context.FullDayMoveBudget);
        }
    }
    else if(Context.bNeedsMoreMoves)
    {
        Result.NextStep=TEXT("Не хватает ходов: закончите день");
        Result.Detail=FString::Printf(TEXT("Нужно ещё %d хода. Нажмите «Конец хода»."),
            FMath::Clamp(Context.MissingMoves,1,99));
    }
    else
    {
        Result.NextStep=TEXT("Маршрут закрыт: найдите обход");
        Result.Detail=TEXT("Проверьте путь, охрану и подготовку армии.");
    }
    return Result;
}
