#include "AIHeroesHUD.h"

#include "AIHeroesGameMode.h"
#include "AIHeroesWorldNode.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

void AAIHeroesHUD::DrawHUD()
{
    Super::DrawHUD();

    const AAIHeroesGameMode* Rules = Cast<AAIHeroesGameMode>(UGameplayStatics::GetGameMode(this));
    if (!Rules || !Canvas || !GEngine)
    {
        return;
    }

    const float Width = Canvas->SizeX;
    const float Height = Canvas->SizeY;
    UFont* Font = GEngine->GetSmallFont();

    // Readable 2D labels for genuine 3D world locations, independent of lighting.
    const APlayerController* PC = GetWorld()->GetFirstPlayerController();
    if (PC)
    {
        for (const AAIHeroesWorldNode* Node : Rules->Nodes)
        {
            if (!IsValid(Node))
            {
                continue;
            }
            FVector2D Screen;
            const FVector AboveNode = Node->GetActorLocation() + FVector(0.f, 0.f, 260.f);
            if (!PC->ProjectWorldLocationToScreen(AboveNode, Screen))
            {
                continue;
            }
            if (Screen.X < -130.f || Screen.X > Width + 130.f)
            {
                continue;
            }

            const float X = FMath::Clamp(static_cast<float>(Screen.X) - 75.f, 8.f, Width - 160.f);
            const float Y = FMath::Clamp(static_cast<float>(Screen.Y) - 26.f, 184.f, Height - 196.f);
            const FLinearColor Color = Node->bOwned
                ? FLinearColor(0.35f, 1.f, 0.52f)
                : (Node->Defenders.IsEmpty()
                    ? FLinearColor(1.f, 0.91f, 0.35f)
                    : FLinearColor(1.f, 0.42f, 0.3f));

            DrawRect(FLinearColor(0.012f, 0.027f, 0.045f, 0.88f), X, Y, 150.f, 28.f);
            DrawText(FString::Printf(TEXT("%s%s"), *Node->GetDisplayName(),
                Node->bOwned ? TEXT(" [OURS]") : TEXT("")),
                Color, X + 9.f, Y + 6.f, Font, 1.03f);
        }
    }

    DrawRect(FLinearColor(0.015f, 0.022f, 0.04f, 0.88f), 0.f, 0.f, Width, 178.f);
    DrawText(TEXT("AI HEROES  |  3D TURN-BASED STRATEGY"), FLinearColor(0.4f, 0.85f, 1.f), 18.f, 12.f, Font, 1.35f);
    DrawText(FString::Printf(TEXT("DAY %d   GOLD %d   WOOD %d   ORE %d"), Rules->Day, Rules->Gold, Rules->Wood, Rules->Ore),
        FLinearColor::White, 18.f, 42.f, Font, 1.15f);
    DrawText(FString::Printf(TEXT("ARMY: %s"), *Rules->GetArmySummary()),
        FLinearColor(0.85f, 0.9f, 0.8f), 18.f, 70.f, Font, 1.08f);
    DrawText(FString::Printf(TEXT("HERO AI: %s"), *Rules->GetCognitionSummary()),
        FLinearColor(1.f, 0.85f, 0.35f), 18.f, 95.f, Font, 1.07f);
    DrawText(TEXT("WASD move | Q E turn | R F zoom | Click attack | 1 militia | 2 scouts | P protocol | H advise | Space end turn"),
        FLinearColor(0.7f, 0.76f, 0.85f), 18.f, 127.f, Font, 0.98f);

    DrawRect(FLinearColor(0.015f, 0.022f, 0.04f, 0.88f), 0.f, Height - 148.f, Width, 148.f);
    DrawText(FString::Printf(TEXT("EVENT: %s"), *Rules->LastEvent.Left(155)),
        FLinearColor::White, 18.f, Height - 132.f, Font, 0.98f);
    DrawText(FString::Printf(TEXT("ADVISOR: %s"), *Rules->LastHint.Left(175)),
        FLinearColor(0.48f, 0.94f, 0.82f), 18.f, Height - 99.f, Font, 0.97f);
    DrawText(TEXT("AI BATTLES ARE AUTOMATIC. PROTOCOL / MODEL SIZES ARE PROTOTYPE TIERS, NOT LIVE LLMS."),
        FLinearColor(0.9f, 0.76f, 0.62f), 18.f, Height - 54.f, Font, 0.94f);
}
