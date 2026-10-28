#include "AIHGameHUD.h"

#include "AIHGameMode.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

namespace
{
    const TCHAR* ButtonText[8] = {
        TEXT("SCOUT +1"), TEXT("GUARDIAN +1"),
        TEXT("SPARK +1"), TEXT("MOVE / CONQUER"),
        TEXT("ASK HERO"), TEXT("END TURN"),
        TEXT("PROTOCOL +"), TEXT("MODEL SLOT +")
    };
}

void AAIHGameHUD::GetButtonRect(int32 Index, float& X, float& Y, float& W, float& H) const
{
    X = Y = W = H = 0.f;
    if (!Canvas || Index < 0 || Index >= 8)
    {
        return;
    }

    const int32 Columns = Canvas->SizeX < 680.f ? 2 : 4;
    const int32 Rows = 8 / Columns;
    const float Gap = 8.f;
    W = FMath::Min(178.f, (Canvas->SizeX - 32.f - (Columns - 1) * Gap) / Columns);
    H = 42.f;
    X = 16.f + (Index % Columns) * (W + Gap);
    Y = Canvas->SizeY - 20.f - Rows * (H + Gap) + (Index / Columns) * (H + Gap);
}

int32 AAIHGameHUD::GetActionAt(const FVector2D& ScreenPoint) const
{
    for (int32 Index = 0; Index < 8; ++Index)
    {
        float X, Y, W, H;
        GetButtonRect(Index, X, Y, W, H);
        if (W > 0.f && ScreenPoint.X >= X && ScreenPoint.X <= X + W &&
            ScreenPoint.Y >= Y && ScreenPoint.Y <= Y + H)
        {
            return Index;
        }
    }
    return INDEX_NONE;
}

void AAIHGameHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas || !GEngine)
    {
        return;
    }
    const AAIHGameMode* Game = GetWorld() ? GetWorld()->GetAuthGameMode<AAIHGameMode>() : nullptr;
    if (!Game)
    {
        return;
    }

    const float PanelWidth = FMath::Min(Canvas->SizeX - 32.f, 900.f);
    const float Scale = Canvas->SizeX < 700.f ? 0.8f : 1.0f;
    DrawRect(FLinearColor(0.018f, 0.028f, 0.06f, 0.87f), 12.f, 10.f, PanelWidth, 205.f);
    DrawText(TEXT("AI HEROES | 3D STRATEGY + EVOLVING AI"), FLinearColor(0.35f, 0.9f, 1.f),
        24.f, 17.f, GEngine->GetSmallFont(), 1.15f);
    DrawText(Game->GetStatusLine(), FLinearColor::White, 24.f, 49.f, GEngine->GetSmallFont(), Scale);
    DrawText(Game->GetArmyLine(), FLinearColor::White, 24.f, 72.f, GEngine->GetSmallFont(), Scale);
    DrawText(Game->GetSelectionLine(), FLinearColor(1.f, 0.83f, 0.35f),
        24.f, 95.f, GEngine->GetSmallFont(), Scale);
    DrawText(Game->GetBrainLine(), FLinearColor(0.55f, 1.f, 0.7f),
        24.f, 118.f, GEngine->GetSmallFont(), Scale);
    DrawText(Game->GetReportLine().Left(110), FLinearColor::White,
        24.f, 146.f, GEngine->GetSmallFont(), Scale);
    DrawText(Game->GetAdviceLine().Left(110), FLinearColor(0.6f, 0.85f, 1.f),
        24.f, 171.f, GEngine->GetSmallFont(), Scale);

    for (int32 Index = 0; Index < 8; ++Index)
    {
        float X, Y, W, H;
        GetButtonRect(Index, X, Y, W, H);
        DrawRect(Index == 4 ? FLinearColor(0.08f, 0.34f, 0.42f, 0.95f) :
            FLinearColor(0.07f, 0.12f, 0.2f, 0.94f), X, Y, W, H);
        DrawText(ButtonText[Index], FLinearColor::White,
            X + 8.f, Y + 10.f, GEngine->GetSmallFont(), Scale);
    }

    DrawText(TEXT("WASD move camera | Q/E rotate | wheel zoom | click or tap nodes"),
        FLinearColor(0.6f, 0.7f, 0.78f), 20.f,
        FMath::Max(216.f, Canvas->SizeY - (Canvas->SizeX < 680.f ? 235.f : 135.f)),
        GEngine->GetSmallFont(), Scale);
}
