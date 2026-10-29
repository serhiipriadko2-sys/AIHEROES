#include "AIHeroesZoneExpeditionHUD.h"

#include "AIHeroesZoneExpeditionGameMode.h"
#include "AIHeroesZoneExpeditionRules.h"
#include "AIHeroesZoneScreenLayout.h"
#include "AIHeroesReferenceUI.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

namespace
{
// The displayed labels are presentations of already discovered identities.
// This deliberately does not depend on unrevealed map layout or enemy state.
FString SiteName(FName Id)
{
    if (Id == FName(TEXT("HQ"))) return TEXT("Цитадель Искры");
    if (Id == FName(TEXT("SUPPLY"))) return TEXT("Лагерь снабжения");
    if (Id == FName(TEXT("FOREST"))) return TEXT("Лесопилка");
    if (Id == FName(TEXT("ORE"))) return TEXT("Рудник");
    if (Id == FName(TEXT("PASS"))) return TEXT("Горный перевал");
    if (Id == FName(TEXT("OUTPOST"))) return TEXT("Сторожевая застава");
    if (Id == FName(TEXT("RUINS"))) return TEXT("Древний архив");
    return Id.ToString();
}

// Pure overview schematic, NOT a complete generator map. Never draw
// identities unless PlayerView confirms them as discovered.
FVector2D KnownSchematicPoint(FName Id)
{
    if (Id == FName(TEXT("HQ"))) return { .12f, .50f };
    if (Id == FName(TEXT("SUPPLY"))) return { .29f, .50f };
    if (Id == FName(TEXT("FOREST"))) return { .50f, .26f };
    if (Id == FName(TEXT("ORE"))) return { .50f, .73f };
    if (Id == FName(TEXT("PASS"))) return { .70f, .50f };
    if (Id == FName(TEXT("OUTPOST"))) return { .91f, .26f };
    if (Id == FName(TEXT("RUINS"))) return { .91f, .73f };
    if (Id == FName(TEXT("WAY_01"))) return { .38f, .10f };
    if (Id == FName(TEXT("WAY_02"))) return { .38f, .90f };
    if (Id == FName(TEXT("WAY_03"))) return { .77f, .10f };
    if (Id == FName(TEXT("WAY_04"))) return { .77f, .90f };
    return { .50f, .50f };
}
}

void AAIHeroesZoneExpeditionHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas || !GEngine || !GetWorld()) return;
    const auto* Game = Cast<AAIHeroesZoneExpeditionGameMode>(
        GetWorld()->GetAuthGameMode());
    if (!Game || !Game->IsReady()) return;

    UFont* Font = GEngine->GetSmallFont();
    const auto& State = Game->GetState();
    const float W = Canvas->SizeX;
    const float H = Canvas->SizeY;
    const auto Layout = FAIHeroesZoneScreenLayout::Build(
        FVector2D(W, H), Game->IsDossierOpen());
    const bool bNarrow = W < 1200.f;
    const FLinearColor Paper(.93f, .94f, .89f);
    const FLinearColor Amber(.98f, .77f, .36f);
    const FLinearColor Teal(.48f, .90f, .83f);
    const FLinearColor Dim(.67f, .78f, .80f);
    const FLinearColor GoldBorder(.54f, .39f, .18f, .98f);
    const FLinearColor Slate(.025f, .051f, .070f, .95f);

    auto Panel = [this, &GoldBorder, &Slate](const FAIHeroesZoneScreenRect& R)
    {
        DrawRect(Slate, R.X, R.Y, R.Width, R.Height);
        DrawRect(GoldBorder, R.X, R.Y, R.Width, 2.f);
        DrawRect(GoldBorder, R.X, R.Y + R.Height - 2.f, R.Width, 2.f);
        DrawRect(GoldBorder, R.X, R.Y, 2.f, R.Height);
        DrawRect(GoldBorder, R.X + R.Width - 2.f, R.Y, 2.f, R.Height);
    };

    // The readable, compact command header leaves more of the world visible.
    // All data are from the authoritative player snapshot; no sample numbers.
    DrawRect(FLinearColor(.015f, .030f, .047f, .985f), 0.f, 0.f, W, 79.f);
    DrawRect(GoldBorder, 0.f, 77.f, W, 2.f);
    DrawText(TEXT("AIHEROES"), Amber, 18.f, 9.f, Font, bNarrow ? 1.12f : 1.27f);
    DrawText(TEXT("РУБЕЖИ ИСКРЫ"), Paper,
        bNarrow ? 155.f : 180.f, 13.f, Font, bNarrow ? .91f : 1.05f);
    DrawText(FString::Printf(TEXT("День %d / Неделя %d"),
        State.Day, (State.Day - 1) / 7 + 1),
        Teal, W - (bNarrow ? 180.f : 215.f), 13.f, Font,
        bNarrow ? .86f : 1.02f);

    const float ResourceY = 47.f;
    if (bNarrow)
    {
        DrawText(FString::Printf(
            TEXT("ЗОЛОТО %d     ДЕРЕВО %d     РУДА %d     ДАННЫЕ %d     ХОД %d/8"),
            State.Gold, State.Wood, State.Ore, State.DataFragments,
            State.MovesLeft), Amber, 18.f, ResourceY, Font, W < 800.f ? .73f : .86f);
    }
    else
    {
        const FString Values[] = {
            FString::Printf(TEXT("ЗОЛОТО  %d"), State.Gold),
            FString::Printf(TEXT("ДЕРЕВО  %d"), State.Wood),
            FString::Printf(TEXT("РУДА  %d"), State.Ore),
            FString::Printf(TEXT("ДАННЫЕ  %d"), State.DataFragments),
            FString::Printf(TEXT("ХОД  %d/8"), State.MovesLeft)
        };
        const float Slots[] = { 18.f, 262.f, 479.f, 675.f, 923.f };
        const EAIHeroesGlyph Icons[] = {
            EAIHeroesGlyph::Gold, EAIHeroesGlyph::Wood,
            EAIHeroesGlyph::Ore, EAIHeroesGlyph::Data,
            EAIHeroesGlyph::Hourglass
        };
        for (int32 I = 0; I < 5; ++I)
        {
            const FLinearColor Color = I == 4 ? Teal : Amber;
            for (const auto& Stroke : FAIHeroesReferenceUI::Glyph(Icons[I]))
                DrawLine(Slots[I] + Stroke.A.X * 19.f,
                    ResourceY - 2.f + Stroke.A.Y * 19.f,
                    Slots[I] + Stroke.B.X * 19.f,
                    ResourceY - 2.f + Stroke.B.Y * 19.f,
                    Color, 1.6f);
            DrawText(Values[I], Color,
                Slots[I] + 26.f, ResourceY, Font, .98f);
            if (I > 0) DrawRect(GoldBorder,
                Slots[I] - 15.f, 42.f, 1.f, 22.f);
        }
    }
    // This secondary transparent strip stays inside the previously blocked
    // top-137px input region, so no hidden hit targets are introduced.
    DrawRect(FLinearColor(.02f, .047f, .062f, .79f), 0.f, 83.f, W, 39.f);
    DrawText(FString::Printf(TEXT("ГЕРОЙ: %s   |   ВЫБРАНО: %s"),
        *SiteName(State.HeroSite), *SiteName(Game->GetSelection())),
        Paper, 18.f, 91.f, Font, bNarrow ? .83f : .98f);
    DrawText(bNarrow ?
        TEXT("Касание выбирает место. Для действия нажмите «Идти / бой».") :
        TEXT("Клик — выбрать место  •  H — советник  •  D — маршруты  •  Enter — подтвердить  •  Space — день"),
        Dim, 18.f, 121.f, Font, bNarrow ? .79f : .91f);

    // Legible screen-space callouts for the visible expedition only.
    // Never project unrevealed locations or enemy statistics.
    if (Layout.bSupported && W >= 1000.f && H >= 660.f)
    {
        const APlayerController* PC = GetWorld()->GetFirstPlayerController();
        if (PC)
        {
            TArray<FAIHeroesVisibleSiteMarker> Markers;
            Game->GetVisibleSiteMarkers(Markers);
            const float BottomTop = Layout.Buttons.IsEmpty()
                ? H - 195.f : Layout.Buttons[0].Rect.Y - 7.f;
            for (const auto& Marker : Markers)
            {
                FVector2D Screen;
                if (!PC->ProjectWorldLocationToScreen(
                        Marker.Anchor, Screen, false) ||
                    !FMath::IsFinite(Screen.X) || !FMath::IsFinite(Screen.Y))
                    continue;
                const float BoxWidth = 156.f;
                const float BoxHeight = 25.f;
                const float X = Screen.X - BoxWidth * .5f;
                const float Y = Screen.Y - 40.f;
                if (Y < 140.f || Y + BoxHeight > BottomTop - 7.f ||
                    X < 264.f || X + BoxWidth > W - 280.f)
                    continue;
                const FLinearColor Frame = Marker.bSelected ? Amber :
                    Marker.bCleared ? Teal :
                    Marker.bKnownGuard ? FLinearColor(.96f, .61f, .35f) :
                    GoldBorder;
                DrawRect(FLinearColor(.012f, .034f, .047f, .96f),
                    X, Y, BoxWidth, BoxHeight);
                DrawRect(Frame, X, Y, BoxWidth, 2.f);
                DrawRect(Frame, X, Y, 2.f, BoxHeight);
                DrawText(SiteName(Marker.Id), Marker.bSelected ? Amber : Paper,
                    X + 8.f, Y + 5.f, Font, .89f);
            }
        }
    }

    if (Layout.bWideChrome)
    {
        const auto& M = Layout.MissionPanel;
        Panel(M);
        DrawText(TEXT("ЦЕЛЬ ЭКСПЕДИЦИИ"), Amber, M.X + 12.f, M.Y + 10.f, Font, 1.06f);
        DrawText(FString::Printf(TEXT("Разведано: %d из %d зон"),
            Game->KnownZones(), Game->TotalZones()),
            Dim, M.X + 12.f, M.Y + 36.f, Font, .88f);

        const bool bOutpost = FAIHeroesZoneExpeditionRules::IsCleared(
            State, FName(TEXT("OUTPOST")));
        const bool bRuins = FAIHeroesZoneExpeditionRules::IsCleared(
            State, FName(TEXT("RUINS")));
        const FString Goals[] = {
            TEXT("Захватить сторожевую заставу"),
            TEXT("Исследовать древний архив")
        };
        const bool Done[] = { bOutpost, bRuins };
        for (int32 I = 0; I < 2; ++I)
        {
            const float Y = M.Y + 63.f + I * 27.f;
            DrawRect(Done[I] ? Teal : GoldBorder, M.X + 13.f, Y + 2.f, 11.f, 11.f);
            DrawText(Goals[I], Done[I] ? Teal : Paper,
                M.X + 32.f, Y, Font, .87f);
        }
        DrawText(TEXT("Приказы отдаёт только игрок"), Dim,
            M.X + 12.f, M.Y + M.Height - 20.f, Font, .77f);

        const auto& I = Layout.InspectorPanel;
        Panel(I);
        DrawText(TEXT("ВЫБРАННЫЙ ОБЪЕКТ"), Amber,
            I.X + 13.f, I.Y + 10.f, Font, .98f);
        DrawRect(GoldBorder, I.X + 13.f, I.Y + 35.f, I.Width - 26.f, 1.f);
        DrawText(SiteName(Game->GetSelection()), Paper,
            I.X + 13.f, I.Y + 48.f, Font, 1.16f);

        const FName Selected = Game->GetSelection();
        const auto Route = FAIHeroesZoneExpeditionRules::Preview(State, Selected);
        const bool bKnownRoute = Route.Status == EAIHeroesAdventurePathStatus::Ready ||
            Route.Status == EAIHeroesAdventurePathStatus::NeedsMoreMoves;
        DrawText(FAIHeroesZoneExpeditionRules::IsCleared(State, Selected) ?
            TEXT("Контроль: ваша территория") :
            TEXT("Контроль: не захвачено"),
            Teal, I.X + 13.f, I.Y + 85.f, Font, .88f);

        if (Route.Status == EAIHeroesAdventurePathStatus::Ready)
            DrawText(FString::Printf(TEXT("Стоимость пути: %d очков"),
                Route.TotalCost), Paper, I.X + 13.f, I.Y + 116.f, Font, .92f);
        else if (Route.Status == EAIHeroesAdventurePathStatus::NeedsMoreMoves)
            DrawText(FString::Printf(TEXT("Не хватает: %d очков"),
                Route.MissingMoves), Amber, I.X + 13.f, I.Y + 116.f, Font, .92f);
        else
            DrawText(TEXT("Путь сейчас недоступен"), Amber,
                I.X + 13.f, I.Y + 116.f, Font, .92f);

        if (bKnownRoute)
        {
            DrawText(Route.bBattleAtDestination ?
                TEXT("Есть охрана • исход боя неизвестен") :
                TEXT("Охраны на маршруте нет"),
                Route.bBattleAtDestination ? Amber : Teal,
                I.X + 13.f, I.Y + 144.f, Font, .83f);
        }
        int32 LivingHP = 0;
        for (const auto& Unit : State.PlayerArmy)
            LivingHP += FMath::Max(0, Unit.TotalHealth);
        DrawRect(FLinearColor(.07f, .13f, .16f, .95f),
            I.X + 12.f, I.Y + 183.f, I.Width - 24.f, 52.f);
        DrawText(FString::Printf(TEXT("Ваша армия: %d здоровья"), LivingHP),
            Paper, I.X + 22.f, I.Y + 190.f, Font, .92f);
        DrawText(TEXT("Советник: правила • без нейросети"),
            Dim, I.X + 22.f, I.Y + 212.f, Font, .75f);

        const auto& Mini = Layout.MiniMapPanel;
        Panel(Mini);
        DrawText(TEXT("СХЕМА РАЗВЕДКИ"), Amber,
            Mini.X + 12.f, Mini.Y + 8.f, Font, .98f);
        DrawText(TEXT("Только открытые пути"), Dim,
            Mini.X + 12.f, Mini.Y + 29.f, Font, .73f);
        FAIHeroesAdventureGraph Visible;
        FString Reason;
        if (FAIHeroesZoneExpeditionRules::PlayerView(State, Visible, Reason))
        {
            const FVector2D At(Mini.X + 15.f, Mini.Y + 52.f);
            const FVector2D Size(Mini.Width - 30.f, Mini.Height - 65.f);
            const auto Point = [&At, &Size](FName Id)
            {
                const FVector2D UV = KnownSchematicPoint(Id);
                return FVector2D(At.X + UV.X * Size.X,
                                 At.Y + UV.Y * Size.Y);
            };
            for (const auto& Edge : Visible.Links)
            {
                if (Edge.From.ToString().Compare(
                    Edge.To.ToString(), ESearchCase::CaseSensitive) >= 0) continue;
                const FVector2D A = Point(Edge.From);
                const FVector2D B = Point(Edge.To);
                DrawLine(A.X, A.Y, B.X, B.Y,
                    FLinearColor(.35f, .48f, .47f), 1.75f);
            }
            for (const auto& Node : Visible.Nodes)
            {
                const FVector2D P = Point(Node.Id);
                const bool bHero = Node.Id == State.HeroSite;
                const FLinearColor Color = bHero ? Teal :
                    Node.bHostileGuard ? Amber : Paper;
                DrawRect(Color, P.X - (bHero ? 5.f : 3.f),
                    P.Y - (bHero ? 5.f : 3.f),
                    bHero ? 10.f : 6.f, bHero ? 10.f : 6.f);
            }
        }
        else
            DrawText(TEXT("Разведка недоступна"), Amber,
                Mini.X + 12.f, Mini.Y + 72.f, Font, .86f);
    }

    if (Game->IsDossierOpen() && Layout.bSupported)
    {
        // Modal owns all input while open. Cosmetic map frame below is
        // deliberately dimmed, and no action is inferred from ranking.
        DrawRect(FLinearColor(.01f, .025f, .04f, .48f),
            0.f, 137.f, W, H - 225.f);
        const auto& P = Layout.ComparisonPanel;
        Panel(P);
        const auto& Dossier = Game->GetDossier();
        const bool bShortPanel = P.Height < 270.f;
        DrawText(TEXT("СОВЕТНИК ИСКРЫ  /  СРАВНЕНИЕ МАРШРУТОВ"),
            Amber, P.X + 15.f, P.Y + 11.f, Font,
            bNarrow ? .88f : 1.15f);
        DrawText(FString::Printf(
            TEXT("%d варианта из %d известных • исход боя не предсказывается"),
            Dossier.Options.Num(), Dossier.ConsideredKnownTargets),
            Paper, P.X + 15.f, P.Y + 37.f, Font,
            bNarrow ? .76f : .95f);

        const float FirstY = P.Y + (bShortPanel ? 57.f : 76.f);
        const float RowHeight = bShortPanel
            ? (P.Height - 78.f) / 3.f
            : FMath::Min(77.f, (P.Height - 108.f) / 3.f);
        for (int32 Index = 0; Index < Dossier.Options.Num(); ++Index)
        {
            const auto& Option = Dossier.Options[Index];
            const float Y = FirstY + Index * RowHeight;
            DrawRect(FLinearColor(.063f, .11f, .14f, .96f),
                P.X + 11.f, Y - 4.f, P.Width - 22.f,
                FMath::Max(30.f, RowHeight - 6.f));
            const FLinearColor Color = Option.Readiness ==
                EAIHeroesDossierReadiness::ReadyNoGuard ? Teal :
                Option.Readiness == EAIHeroesDossierReadiness::GuardOutcomeUnknown ?
                Amber : FLinearColor(1.f, .61f, .52f);
            const FString Head = FString::Printf(
                TEXT("%d. %s  •  %d очков  •  оценка %d"),
                Index + 1, *SiteName(Option.Site),
                Option.TravelCost, Option.RankScore);
            DrawText(Head, Color, P.X + 21.f, Y + (bShortPanel ? 4.f : 0.f),
                Font, bNarrow ? .81f : 1.f);
            if (!bShortPanel)
            {
                DrawText(Option.MissingMoves > 0 ?
                    FString::Printf(TEXT("Пока не хватает %d очков движения"),
                        Option.MissingMoves) :
                    Option.bGuarded ?
                        TEXT("Охрана известна, исход боя неизвестен") :
                        TEXT("Без охраны на известном пути"),
                    Paper, P.X + 21.f, Y + 29.f, Font, .91f);
            }
        }
        DrawText(TEXT("Только анализ • D/Esc закрыть • действия заблокированы"),
            Teal, P.X + 15.f, P.Y + P.Height - 24.f, Font, .87f);
    }

    if (Layout.bSupported)
    {
        // Identical rectangles drive both Canvas drawing and touch/mouse.
        const float BarTop = Layout.Buttons.IsEmpty() ?
            H - 195.f : Layout.Buttons[0].Rect.Y - 7.f;
        DrawRect(FLinearColor(.018f, .033f, .050f, .965f),
            0.f, BarTop, W, (H - 87.f) - BarTop);
        DrawRect(GoldBorder, 0.f, BarTop, W, 2.f);
        for (const auto& Button : Layout.Buttons)
        {
            const auto& R = Button.Rect;
            const bool bPrimary =
                Button.Action == EAIHeroesZoneScreenAction::ConfirmMove;
            const FLinearColor Background = bPrimary ?
                FLinearColor(.12f, .37f, .32f, .98f) :
                Button.bHighImpact ? FLinearColor(.22f, .15f, .09f, .98f) :
                FLinearColor(.050f, .091f, .120f, .98f);
            DrawRect(Background, R.X, R.Y, R.Width, R.Height);
            const FLinearColor Accent = bPrimary ? Teal :
                Button.bHighImpact ? Amber : GoldBorder;
            DrawRect(Accent, R.X, R.Y, R.Width, 3.f);
            DrawRect(Accent, R.X, R.Y, 2.f, R.Height);
            DrawRect(Accent, R.X + R.Width - 2.f, R.Y, 2.f, R.Height);
            DrawText(Button.Label, bPrimary ? FLinearColor::White : Paper,
                R.X + 10.f, R.Y + (R.Height - 16.f) * .5f,
                Font, W < 850.f ? .77f : bNarrow ? .86f : .99f);
        }
    }
    else
        DrawText(TEXT("Поверните экран • минимум 640 × 520 для кнопок"),
            Amber, 18.f, H - 160.f, Font, .88f);

    DrawRect(FLinearColor(.013f, .025f, .043f, .98f),
        0.f, H - 86.f, W, 86.f);
    DrawRect(GoldBorder, 0.f, H - 86.f, W, 1.f);
    DrawText(TEXT("СОБЫТИЕ"), Amber, 18.f, H - 75.f, Font, .78f);
    DrawText(Game->GetLastMessage().Left(
        FMath::Clamp(static_cast<int32>(W / 7.f), 55, 170)),
        Paper, 18.f, H - 56.f, Font, bNarrow ? .79f : .90f);
    DrawText(bNarrow ?
        TEXT("Пошаговая игра • без активной нейросети • сенсорное устройство не проверено") :
        TEXT("Движение и бой — только после подтверждения игрока • Android ещё не проверен"),
        Dim, 18.f, H - 29.f, Font, bNarrow ? .75f : .86f);
}
