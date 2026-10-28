#include "AIHeroesHUD.h"
#include "AIHeroesHUDLayout.h"

#include "AIHeroesAdvisorDossier.h"
#include "AIHeroesGameMode.h"
#include "AIHeroesWorldNode.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace
{
const TCHAR* ActionLabels[8] = {
    TEXT("CONFIRM MOVE / FIGHT"),
    TEXT("ASK HERO"),
    TEXT("NEXT DAY"),
    TEXT("RECRUIT MILITIA"),
    TEXT("RECRUIT SCOUTS"),
    TEXT("UNLOCK AI PROTOCOL"),
    TEXT("MODEL SLOT +"),
    TEXT("PLAN ROUTE Z")
};
const TCHAR* UtilityLabels[5] = {
    TEXT("SAVE K"), TEXT("LOAD L"), TEXT("SLOT C"), TEXT("REPLAY J"), TEXT("3D REPLAY V")
};
}

void AAIHeroesHUD::GetActionRect(
    int32 Index, float& X, float& Y, float& W, float& H) const
{
    const AAIHeroesGameMode* Rules =
        Cast<AAIHeroesGameMode>(UGameplayStatics::GetGameMode(this));
    const FAIHeroesHUDActionRect Rect = FAIHeroesHUDLayout::Rect(
        Index, CachedCanvasSize, Rules && Rules->IsLiveBattleActive());
    X = Rect.X;
    Y = Rect.Y;
    W = Rect.W;
    H = Rect.H;
}

int32 AAIHeroesHUD::GetActionAt(const FVector2D& ScreenPoint) const
{
    const AAIHeroesGameMode* Rules =
        Cast<AAIHeroesGameMode>(UGameplayStatics::GetGameMode(this));
    if (Rules && Rules->IsLiveBattleActive())
    {
        return FAIHeroesHUDLayout::HitTest(ScreenPoint, CachedCanvasSize, true);
    }
    if (Rules && Rules->IsAdvisorDossierVisible())
    {
        // Even a click outside the panel is consumed by this read-only modal.
        return FAIHeroesAdvisorDossierRules::HitTest(ScreenPoint, CachedCanvasSize);
    }
    if (Rules && !Rules->IsBattleReplayActive() &&
        FAIHeroesAdvisorDossierRules::OpenButton(CachedCanvasSize).Contains(ScreenPoint))
    {
        return FAIHeroesAdvisorDossierRules::ToggleAction;
    }
    return FAIHeroesHUDLayout::HitTest(ScreenPoint, CachedCanvasSize, false);
}
void AAIHeroesHUD::DrawHUD()
{
    Super::DrawHUD();
    const AAIHeroesGameMode* Rules =
        Cast<AAIHeroesGameMode>(UGameplayStatics::GetGameMode(this));
    if (!Rules || !Canvas || !GEngine)
    {
        return;
    }

    CachedCanvasSize = FVector2D(Canvas->SizeX, Canvas->SizeY);
    const float Width = Canvas->SizeX;
    const float Height = Canvas->SizeY;
    const int32 Columns = Width >= 1150.f ? 4 : (Width >= 800.f ? 3 : 2);
    const float BottomHeight = 120.f + 53.f * ((7 + Columns - 1) / Columns);
    const float Scale = Width < 700.f ? 0.78f : 1.f;
    const int32 TextLimit = FMath::Clamp(FMath::FloorToInt(Width / 8.f), 40, 152);
    UFont* Font = GEngine->GetSmallFont();

    if (Rules->IsLiveBattleActive())
    {
        DrawRect(FLinearColor(0.015f, 0.025f, 0.045f, 0.96f), 0.f, 0.f, Width, 143.f);
        DrawText(FString::Printf(TEXT("AI HEROES | LIVE GRID V2 | PROTOCOL %d | HERO COMMANDS"), Rules->ProtocolTier),
            FLinearColor(0.4f, 0.9f, 1.f), 16.f, 12.f, Font, Scale * 1.15f);
        DrawText(Rules->GetLiveBattleStatus().Left(TextLimit), FLinearColor::White,
            16.f, 42.f, Font, Scale);
        DrawText(TEXT("HERO: ") + Rules->GetLiveArmyStatus(true).Left(TextLimit - 6),
            FLinearColor(0.4f, 0.9f, 1.f), 16.f, 69.f, Font, Scale);
        DrawText(TEXT("ENEMY: ") + Rules->GetLiveArmyStatus(false).Left(TextLimit - 7),
            FLinearColor(1.f, 0.65f, 0.3f), 16.f, 95.f, Font, Scale);
        DrawText(TEXT("Q/E orbit  R/F zoom | SPACE pause | T speed | B fast finish | ENTER return after result"),
            FLinearColor(0.8f, 0.85f, 0.9f), 16.f, 121.f, Font, Scale * 0.85f);
        DrawRect(FLinearColor(0.015f, 0.025f, 0.045f, 0.96f),
            0.f, Height - 126.f, Width, 126.f);
        DrawText(Rules->LastEvent.Left(TextLimit), FLinearColor::White,
            16.f, Height - 115.f, Font, Scale);
        const bool bResult = Rules->GetLiveBattlePhase() == EAIHeroesLiveBattlePhase::Result;
        const FString Labels[] = {
            Rules->GetLiveBattlePhase() == EAIHeroesLiveBattlePhase::Paused ? TEXT("RESUME") : TEXT("PAUSE"),
            FString::Printf(TEXT("SPEED %dx"), FMath::RoundToInt(Rules->GetLiveBattleSpeed())),
            TEXT("FAST FINISH"), bResult ? TEXT("RETURN TO MAP") : TEXT("RESULT PENDING")};
        for (int32 I = 0; I < 4; ++I)
        {
            float X, Y, W, H;
            GetActionRect(100 + I, X, Y, W, H);
            const bool bEnabled = I == 3 ? bResult : !bResult;
            DrawRect(bEnabled ? FLinearColor(0.04f, 0.28f, 0.34f, 1.f) :
                FLinearColor(0.08f, 0.1f, 0.13f, 1.f), X, Y, W, H);
            DrawText(Labels[I], bEnabled ? FLinearColor::White : FLinearColor(0.4f, 0.4f, 0.4f),
                X + 9.f, Y + 14.f, Font, Scale);
        }
        return; // No strategic node labels or hit targets under the arena.
    }

    // World-space nodes receive screen-space labels for readability in 3D.
    if (const APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        for (const AAIHeroesWorldNode* Node : Rules->Nodes)
        {
            if (!IsValid(Node))
            {
                continue;
            }
            FVector2D Screen;
            if (!PC->ProjectWorldLocationToScreen(
                Node->GetActorLocation() + FVector(0.f, 0.f, 260.f), Screen))
            {
                continue;
            }
            if (Screen.X < -150.f || Screen.X > Width + 150.f)
            {
                continue;
            }

            const float LabelX = FMath::Clamp(
                static_cast<float>(Screen.X) - 75.f, 6.f, FMath::Max(6.f, Width - 158.f));
            const float LabelY = FMath::Clamp(
                static_cast<float>(Screen.Y) - 26.f, 182.f,
                FMath::Max(182.f, Height - BottomHeight - 32.f));
            const bool bSelected = Rules->SelectedNode.Get() == Node;
            const FLinearColor Color = bSelected
                ? FLinearColor(0.3f, 1.f, 1.f)
                : Node->bOwned ? FLinearColor(0.3f, 1.f, 0.55f)
                : Node->Defenders.IsEmpty() ? FLinearColor(1.f, 0.92f, 0.33f)
                : FLinearColor(1.f, 0.45f, 0.31f);

            DrawRect(bSelected ? FLinearColor(0.02f, 0.16f, 0.21f, 0.98f) :
                FLinearColor(0.012f, 0.027f, 0.045f, 0.88f),
                LabelX, LabelY, 150.f, 28.f);
            DrawText(FString::Printf(TEXT("%s%s"),
                *Node->GetDisplayName(), Node->bOwned ? TEXT(" [OURS]") : TEXT("")),
                Color, LabelX + 9.f, LabelY + 6.f, Font, Scale);
        }
    }

    // A route is a short-lived, READ-ONLY recommendation. HUD never selects
    // or moves a pawn. Lines are rendered only when both endpoints project
    // onto the unobstructed 3D map (not beneath HUD controls).
    const FAIHeroesRoutePlan& Route = Rules->GetRoutePreview();
    if (Route.IsValid() && !Rules->IsBattleReplayActive())
    {
        if (const APlayerController* PC = GetWorld()->GetFirstPlayerController())
        {
            const float MapTop = 179.f;
            const float MapBottom = Height - BottomHeight - 42.f;
            auto OnMap = [Width, MapTop, MapBottom](const FVector2D& P)
            {
                return P.X >= 12.f && P.X <= Width - 12.f &&
                    P.Y >= MapTop && P.Y <= MapBottom;
            };

            FVector2D Previous;
            bool bPreviousOnMap = PC->ProjectWorldLocationToScreen(
                Rules->GetHeroStrategicLocation() + FVector(0.f, 0.f, 290.f), Previous) &&
                OnMap(Previous);

            for (int32 StopIndex = 0; StopIndex < Route.Stops.Num(); ++StopIndex)
            {
                const AAIHeroesWorldNode* StopNode = nullptr;
                for (const AAIHeroesWorldNode* Candidate : Rules->Nodes)
                {
                    if (IsValid(Candidate) &&
                        FName(*Candidate->GetDisplayName()) == Route.Stops[StopIndex])
                    {
                        StopNode = Candidate;
                        break;
                    }
                }
                if (!IsValid(StopNode))
                {
                    bPreviousOnMap = false;
                    continue;
                }

                FVector2D At;
                const bool bOnMap = PC->ProjectWorldLocationToScreen(
                    StopNode->GetActorLocation() + FVector(0.f, 0.f, 290.f), At) &&
                    OnMap(At);
                const FLinearColor Color = StopIndex == 0
                    ? FLinearColor(0.24f, 0.94f, 0.89f, 1.f)
                    : FLinearColor(1.f, 0.78f, 0.29f, 1.f);
                if (bPreviousOnMap && bOnMap)
                {
                    DrawLine(Previous.X, Previous.Y, At.X, At.Y, Color, 3.f);
                }
                if (bOnMap)
                {
                    const float TagX = FMath::Clamp(At.X - 38.f, 6.f, FMath::Max(6.f, Width - 83.f));
                    const float TagY = FMath::Clamp(At.Y - 63.f, MapTop, FMath::Max(MapTop, MapBottom - 24.f));
                    DrawRect(FLinearColor(0.02f, 0.13f, 0.19f, 0.98f),
                        TagX, TagY, 77.f, 23.f);
                    DrawText(FString::Printf(TEXT("PLAN %d"), StopIndex + 1),
                        Color, TagX + 7.f, TagY + 4.f, Font, Scale);
                }
                Previous = At;
                bPreviousOnMap = bOnMap;
            }
        }
    }

    DrawRect(FLinearColor(0.015f, 0.022f, 0.04f, 0.92f),
        0.f, 0.f, Width, 175.f);
    DrawText(TEXT("AI HEROES  |  PLAYER DECIDES / AI FIGHTS"),
        FLinearColor(0.42f, 0.88f, 1.f), 18.f, 12.f, Font, Scale * 1.3f);
    DrawText(FString::Printf(TEXT("DAY %d  |  MOVES %d/4  |  GOLD %d  |  WOOD %d  |  ORE %d  |  DATA %d"),
        Rules->Day, Rules->MovesLeft, Rules->Gold, Rules->Wood, Rules->Ore, Rules->DataFragments),
        FLinearColor::White, 18.f, 42.f, Font, Scale);
    DrawText(TEXT("ARMY: ") + Rules->GetArmySummary(),
        FLinearColor(0.88f, 0.95f, 0.81f), 18.f, 68.f, Font, Scale);
    DrawText(TEXT("AI: ") + Rules->GetCognitionSummary(),
        FLinearColor(1.f, 0.85f, 0.35f), 18.f, 94.f, Font, Scale);
    DrawText(Rules->GetSelectionSummary().Left(TextLimit),
        FLinearColor(0.4f, 1.f, 0.92f), 18.f, 119.f, Font, Scale);
    DrawText(TEXT("WASD camera  CLICK select  ENTER confirm  H advise  N recruit?  I dossier  Z PLAN  SPACE day  K save  L load  C slot  V 3D"),
        FLinearColor(0.72f, 0.78f, 0.87f), 18.f, 147.f, Font, Scale * 0.92f);

    if (Rules->IsBattleReplayActive())
    {
        // TextRender in a real 3D scene can lose contrast against bright
        // geometry. The deterministic HUD mirrors the live stack state.
        const float ColW = FMath::Max(100.f, (Width - 42.f) * 0.5f);
        const float LeftX = 14.f;
        const float RightX = Width * 0.5f + 7.f;
        DrawRect(FLinearColor(0.015f, 0.12f, 0.21f, 0.94f),
            LeftX, 184.f, ColW, 34.f);
        DrawRect(FLinearColor(0.25f, 0.075f, 0.035f, 0.94f),
            RightX, 184.f, ColW, 34.f);
        const int32 LabelLimit = FMath::Max(18, FMath::FloorToInt(ColW / 7.f));
        DrawText(Rules->GetBattleReplayArmyStatus(true).Left(LabelLimit),
            FLinearColor(0.43f, 0.95f, 1.f), LeftX + 8.f, 193.f, Font, Scale);
        DrawText(Rules->GetBattleReplayArmyStatus(false).Left(LabelLimit),
            FLinearColor(1.f, 0.72f, 0.39f), RightX + 8.f, 193.f, Font, Scale);
    }

    const float PanelY = Height - BottomHeight;
    DrawRect(FLinearColor(0.015f, 0.023f, 0.046f, 0.94f),
        0.f, PanelY, Width, BottomHeight);
    DrawText(TEXT("EVENT: ") + Rules->LastEvent.Left(TextLimit),
        FLinearColor::White, 18.f, PanelY + 11.f, Font, Scale);
    DrawText(TEXT("HERO ADVICE: ") + Rules->LastHint.Left(TextLimit),
        FLinearColor(0.45f, 0.96f, 0.84f), 18.f, PanelY + 39.f, Font, Scale);
    const FString BattleDisplay = Rules->IsBattleReplayActive()
        ? Rules->GetBattleReplayStatus() : Rules->GetReplayDisplayText();
    DrawText(BattleDisplay.Left(TextLimit),
        FLinearColor(0.87f, 0.77f, 0.5f), 18.f, PanelY + 67.f, Font, Scale);
    DrawText(Width < 700.f ? TEXT("RULE AI | NO LLM") :
        TEXT("LOCAL RULE AI  |  1B / 3B / 8B are themed tiers; no actual LLM loaded"),
        FLinearColor(0.78f, 0.75f, 0.79f), 18.f, PanelY + 89.f, Font, Scale * 0.85f);

    for (int32 Index = 0; Index < 8; ++Index)
    {
        // Grid slot 7 uses distinct action ID 12; utility IDs 7..11
        // stay byte-for-byte compatible with existing UI automation.
        const int32 ActionId = Index == 7 ? 12 : Index;
        float X, Y, W, H;
        GetActionRect(ActionId, X, Y, W, H);
        const bool bActiveRoute = Index == 7 &&
            Rules->GetRoutePreview().IsValid() && !Rules->IsBattleReplayActive();
        DrawRect(Index == 0 ? FLinearColor(0.05f, 0.34f, 0.39f, 0.97f) :
            bActiveRoute ? FLinearColor(0.07f, 0.30f, 0.32f, 0.98f) :
            FLinearColor(0.08f, 0.15f, 0.24f, 0.98f), X, Y, W, H);
        DrawText(Index == 0 && Rules->IsBattleReplayActive()
            ? TEXT("SKIP 3D REPLAY") :
            bActiveRoute ? TEXT("NEXT PLAN Z") : ActionLabels[Index],
            FLinearColor::White, X + 10.f, Y + 11.f, Font, Scale);
    }

    {
        float X, Y, W, H;
        GetActionRect(200, X, Y, W, H);
        DrawRect(FLinearColor(0.08f, 0.24f, 0.28f, 0.98f), X, Y, W, H);
        DrawText(TEXT("RECR N"), FLinearColor(0.65f, 1.f, 0.86f),
            X + 6.f, Y + 11.f, Font, Scale * 0.9f);
    }

    for (int32 Index = 7; Index < 12; ++Index)
    {
        float X, Y, W, H;
        GetActionRect(Index, X, Y, W, H);
        DrawRect(FLinearColor(0.07f, 0.23f, 0.3f, 0.98f), X, Y, W, H);
        const FString UtilityText = Index == 9
            ? FString::Printf(TEXT("SLOT %d C"), Rules->ActiveSaveSlot)
            : FString(UtilityLabels[Index - 7]);
        DrawText(UtilityText, FLinearColor(0.84f, 0.95f, 1.f),
            X + 6.f, Y + 7.f, Font, Scale * 0.87f);
    }

    if (!Rules->IsBattleReplayActive())
    {
        // Shared helper keeps rendering and hit tests identical. This lives in
        // the bottom information strip, not over the top SAVE/REPLAY utilities.
        const FAIHeroesHUDActionRect R = FAIHeroesAdvisorDossierRules::OpenButton(CachedCanvasSize);
        if (R.W > 0.f)
        {
            DrawRect(FLinearColor(0.06f, 0.25f, 0.31f, 1.f), R.X, R.Y, R.W, R.H);
            DrawText(TEXT("DOSSIER I"), FLinearColor(0.6f, 0.95f, 1.f),
                R.X + 10.f, R.Y + 6.f, Font, Scale * 0.95f);
        }
    }
    if (Rules->IsAdvisorDossierVisible())
    {
        DrawAdvisorDossier(*Rules); // Modal is always the final rendered layer.
    }
}
