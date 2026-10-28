#include "AIHeroesHUD.h"

#include "AIHeroesGameMode.h"
#include "AIHeroesWorldNode.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

namespace
{
const TCHAR* ActionLabels[7] = {
    TEXT("CONFIRM MOVE / FIGHT"),
    TEXT("ASK HERO"),
    TEXT("NEXT DAY"),
    TEXT("RECRUIT MILITIA"),
    TEXT("RECRUIT SCOUTS"),
    TEXT("UNLOCK AI PROTOCOL"),
    TEXT("MODEL SLOT +")
};
const TCHAR* UtilityLabels[2] = {TEXT("SAVE F5"), TEXT("LOAD L")};
}

void AAIHeroesHUD::GetActionRect(
    int32 Index, float& X, float& Y, float& W, float& H) const
{
    X = Y = W = H = 0.f;
    // HUD Canvas can be unavailable between DrawHUD frames during input.
    // Use the last observed viewport size for identical rendering and hit tests.
    if (Index < 0 || Index >= 9 || CachedCanvasSize.X <= 0.f || CachedCanvasSize.Y <= 0.f)
    {
        return;
    }

    if (Index >= 7)
    {
        W = CachedCanvasSize.X >= 700.f ? 118.f : 96.f;
        H = 30.f;
        if (CachedCanvasSize.X >= 850.f)
        {
            X = CachedCanvasSize.X - (9 - Index) * (W + 8.f) - 8.f;
            Y = 7.f;
        }
        else
        {
            X = CachedCanvasSize.X - W - 8.f;
            Y = 5.f + (Index - 7) * 34.f;
        }
        return;
    }

    const int32 Columns = CachedCanvasSize.X >= 1150.f ? 4 :
        (CachedCanvasSize.X >= 800.f ? 3 : 2);
    const float Gap = 10.f;
    W = FMath::Min(260.f, (CachedCanvasSize.X - 28.f - (Columns - 1) * Gap) / Columns);
    H = 43.f;
    const float PanelHeight = 120.f + 53.f * ((7 + Columns - 1) / Columns);
    const float GridWidth = Columns * W + (Columns - 1) * Gap;
    const float Left = FMath::Max(14.f, (CachedCanvasSize.X - GridWidth) * 0.5f);
    X = Left + (Index % Columns) * (W + Gap);
    Y = CachedCanvasSize.Y - PanelHeight + 113.f + (Index / Columns) * (H + Gap);
}

int32 AAIHeroesHUD::GetActionAt(const FVector2D& ScreenPoint) const
{
    for (int32 Index = 0; Index < 9; ++Index)
    {
        float X, Y, W, H;
        GetActionRect(Index, X, Y, W, H);
        if (W > 0.f &&
            ScreenPoint.X >= X && ScreenPoint.X <= X + W &&
            ScreenPoint.Y >= Y && ScreenPoint.Y <= Y + H)
        {
            return Index;
        }
    }
    return INDEX_NONE;
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
    DrawText(TEXT("WASD pan  Q/E rotate  R/F zoom  CLICK select  H ask  ENTER confirm  SPACE day  P protocol  M slot  F5 save  L load"),
        FLinearColor(0.72f, 0.78f, 0.87f), 18.f, 147.f, Font, Scale * 0.92f);

    const float PanelY = Height - BottomHeight;
    DrawRect(FLinearColor(0.015f, 0.023f, 0.046f, 0.94f),
        0.f, PanelY, Width, BottomHeight);
    DrawText(TEXT("EVENT: ") + Rules->LastEvent.Left(TextLimit),
        FLinearColor::White, 18.f, PanelY + 11.f, Font, Scale);
    DrawText(TEXT("HERO ADVICE: ") + Rules->LastHint.Left(TextLimit),
        FLinearColor(0.45f, 0.96f, 0.84f), 18.f, PanelY + 39.f, Font, Scale);
    DrawText(TEXT("LAST TACTIC: ") + Rules->LastCombatStep.Left(TextLimit),
        FLinearColor(0.87f, 0.77f, 0.5f), 18.f, PanelY + 67.f, Font, Scale);
    DrawText(TEXT("LOCAL RULE AI  |  1B / 3B / 8B are themed tiers; no actual LLM loaded"),
        FLinearColor(0.78f, 0.75f, 0.79f), 18.f, PanelY + 89.f, Font, Scale * 0.85f);

    for (int32 Index = 0; Index < 7; ++Index)
    {
        float X, Y, W, H;
        GetActionRect(Index, X, Y, W, H);
        DrawRect(Index == 0 ? FLinearColor(0.05f, 0.34f, 0.39f, 0.97f) :
            FLinearColor(0.08f, 0.15f, 0.24f, 0.98f), X, Y, W, H);
        DrawText(ActionLabels[Index], FLinearColor::White,
            X + 10.f, Y + 11.f, Font, Scale);
    }

    for (int32 Index = 7; Index < 9; ++Index)
    {
        float X, Y, W, H;
        GetActionRect(Index, X, Y, W, H);
        DrawRect(FLinearColor(0.07f, 0.23f, 0.3f, 0.98f), X, Y, W, H);
        DrawText(UtilityLabels[Index - 7], FLinearColor(0.84f, 0.95f, 1.f),
            X + 6.f, Y + 7.f, Font, Scale * 0.87f);
    }
}
