#include "AIHeroesZoneExpeditionHUD.h"

#include "AIHeroesZoneExpeditionGameMode.h"
#include "AIHeroesZoneScreenLayout.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

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
    const bool bCompact = W < 1024.f;
    const auto Layout = FAIHeroesZoneScreenLayout::Build(
        FVector2D(W, H), Game->IsDossierOpen());
    const FLinearColor Paper(.92f, .95f, .98f);
    const FLinearColor Amber(1.f, .83f, .46f);
    const FLinearColor Teal(.46f, .96f, .85f);

    DrawRect(FLinearColor(.01f, .025f, .05f, .97f), 0, 0, W, 137.f);
    DrawText(bCompact ?
        TEXT("AI HEROES | ZONE EXPEDITION") :
        TEXT("AI HEROES | ZONE EXPEDITION V2 | HUMAN ORDERS / OFFLINE HERO BATTLES"),
        Teal, 18.f, 10.f, Font, bCompact ? 1.f : 1.12f);
    DrawText(bCompact
        ? FString::Printf(TEXT("SEED %u  |  ZONES %d  |  DISCOVERED %d"),
            State.Seed, Game->TotalZones(), Game->KnownZones())
        : FString::Printf(TEXT("MAP seed=%u | zones %d | discovered %d"),
            State.Seed, Game->TotalZones(), Game->KnownZones()),
        Paper, 18.f, 37.f, Font, 1.f);
    DrawText(bCompact
        ? FString::Printf(TEXT("DAY %d  MOVES %d  GOLD %d  WOOD %d  ORE %d  DATA %d"),
            State.Day, State.MovesLeft, State.Gold, State.Wood, State.Ore, State.DataFragments)
        : FString::Printf(TEXT("DAY %d | MOVES %d/8 | GOLD %d | WOOD %d | ORE %d | DATA %d"),
            State.Day, State.MovesLeft, State.Gold, State.Wood, State.Ore, State.DataFragments),
        Amber, 18.f, 62.f, Font, bCompact ? .86f : 1.f);
    DrawText(FString::Printf(
        TEXT("HERO %s | SELECTED %s | %s"),
        *State.HeroSite.ToString(), *Game->GetSelection().ToString(),
        *Game->GetSelectedPathInfo().Left(bCompact ? 40 : 87)),
        Teal, 18.f, 90.f, Font, bCompact ? .84f : 1.f);
    DrawText(bCompact ?
        TEXT("TAP ACTIONS BELOW | MAP TAP selects only | confirm MOVE/FIGHT") :
        TEXT("MAP CLICK/TAP selects | H hero | SHIFT+H inspect | D compare | ENTER confirms | SPACE day | K/L saves"),
        FLinearColor(.77f, .84f, .94f), 18.f, 117.f,
        Font, bCompact ? .87f : .92f);

    if (Game->IsDossierOpen() && Layout.bSupported)
    {
        const auto& P = Layout.ComparisonPanel;
        const auto& Dossier = Game->GetDossier();
        const bool bShortPanel = P.Height < 270.f;
        DrawRect(FLinearColor(.014f, .032f, .061f, .98f),
            P.X, P.Y, P.Width, P.Height);
        DrawText(bCompact ?
            TEXT("HERO | COMPARE KNOWN ROUTES") :
            TEXT("OFFLINE HERO | READ-ONLY KNOWN FRONTIER COMPARISON"),
            Teal, P.X + 15.f, P.Y + 10.f, Font,
            bCompact ? 1.f : 1.22f);
        DrawText(FString::Printf(
            TEXT("%d options | %d known alternatives | outcome NOT predicted"),
            Dossier.Options.Num(), Dossier.ConsideredKnownTargets),
            Paper, P.X + 15.f, P.Y + 37.f, Font,
            bCompact ? .78f : .97f);

        const float FirstY = P.Y + (bShortPanel ? 57.f : 76.f);
        const float RowHeight = bShortPanel
            ? (P.Height - 78.f) / 3.f
            : FMath::Min(77.f, (P.Height - 108.f) / 3.f);
        for (int32 I = 0; I < Dossier.Options.Num(); ++I)
        {
            const auto& Choice = Dossier.Options[I];
            const float Y = FirstY + I * RowHeight;
            const float BodyHeight = FMath::Max(30.f, RowHeight - 6.f);
            DrawRect(FLinearColor(.038f, .082f, .126f, .94f),
                P.X + 11.f, Y - 4.f, P.Width - 22.f, BodyHeight);
            const FLinearColor StateColor = Choice.Readiness ==
                EAIHeroesDossierReadiness::ReadyNoGuard ? Teal :
                Choice.Readiness == EAIHeroesDossierReadiness::GuardOutcomeUnknown ?
                Amber : FLinearColor(1.f, .58f, .47f);
            if (bShortPanel)
            {
                DrawText(FString::Printf(TEXT("%d. %s | %d moves | %s"),
                    I + 1, *Choice.Site.ToString(), Choice.TravelCost,
                    Choice.bGuarded ? TEXT("GUARD UNKNOWN") :
                                      TEXT("NO GUARD")),
                    StateColor, P.X + 21.f, Y + 4.f, Font, .90f);
            }
            else
            {
                DrawText(FString::Printf(
                    TEXT("%d. %s | %d MOVES | SCORE %d | %s"),
                    I + 1, *Choice.Site.ToString(), Choice.TravelCost,
                    Choice.RankScore, *Choice.Assessment),
                    StateColor, P.X + 21.f, Y, Font, 1.05f);
                DrawText(Choice.Rationale.Left(92), Paper,
                    P.X + 21.f, Y + 29.f, Font, .96f);
            }
        }
        DrawText(TEXT("READ-ONLY | D / ESC / CLOSE | Enter blocked"),
            Teal, P.X + 15.f, P.Y + P.Height - 24.f, Font, .89f);
    }

    if (Layout.bSupported)
    {
        // Canvas and click/touch hit-testing consume the SAME rectangle list.
        const float BarTop = Layout.Buttons.IsEmpty()
            ? H - 195.f : Layout.Buttons[0].Rect.Y - 7.f;
        DrawRect(FLinearColor(.009f, .027f, .051f, .94f),
            0, BarTop, W, (H - 87.f) - BarTop);
        for (const auto& Button : Layout.Buttons)
        {
            const auto& R = Button.Rect;
            const bool bPrimary =
                Button.Action == EAIHeroesZoneScreenAction::ConfirmMove;
            const FLinearColor Background = bPrimary ?
                FLinearColor(.08f, .36f, .31f, .98f) :
                Button.bHighImpact ? FLinearColor(.23f, .16f, .09f, .98f) :
                FLinearColor(.066f, .115f, .164f, .97f);
            DrawRect(Background, R.X, R.Y, R.Width, R.Height);
            DrawRect(bPrimary ? Teal : Button.bHighImpact ? Amber :
                FLinearColor(.25f, .44f, .57f), R.X, R.Y, R.Width, 3.f);
            DrawText(Button.Label, bPrimary ? FLinearColor::White : Paper,
                R.X + 10.f, R.Y + (R.Height - 16.f) * .5f,
                Font, bCompact ? .89f : 1.03f);
        }
    }
    else
    {
        DrawText(TEXT("RESIZE OR ROTATE: minimum 640 x 520 for touch buttons"),
            Amber, 20.f, H - 170.f, Font, 1.f);
    }

    DrawRect(FLinearColor(.018f, .033f, .06f, .97f),
        0, H - 86.f, W, 86.f);
    DrawText(Game->GetLastMessage().Left(FMath::Clamp(
        static_cast<int32>(W / (bCompact ? 8.f : 7.f)), 40, 170)),
        Paper, 18.f, H - 74.f, Font, bCompact ? .88f : 1.02f);
    DrawText(bCompact ?
        TEXT("Offline strategy | no real LLM | device safe-zone QA pending") :
        TEXT("Opt-in campaign | schema-2 A/B saves | offline battles | Android physical-device QA pending"),
        FLinearColor(.7f, .83f, .95f), 18.f, H - 45.f,
        Font, bCompact ? .81f : .88f);
}
