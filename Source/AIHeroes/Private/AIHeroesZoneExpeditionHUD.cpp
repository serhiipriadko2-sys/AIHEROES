#include "AIHeroesZoneExpeditionHUD.h"

#include "AIHeroesZoneExpeditionGameMode.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

void AAIHeroesZoneExpeditionHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas || !GEngine || !GetWorld()) return;
    const auto* G = Cast<AAIHeroesZoneExpeditionGameMode>(GetWorld()->GetAuthGameMode());
    if (!G || !G->IsReady()) return;
    const auto& S = G->GetState();
    UFont* Font = GEngine->GetSmallFont();
    const float W = Canvas->SizeX;
    const float H = Canvas->SizeY;

    DrawRect(FLinearColor(0.01f, 0.025f, 0.05f, 0.95f), 0, 0, W, 137);
    DrawText(TEXT("AI HEROES | ZONE EXPEDITION V2 | HUMAN ORDERS / OFFLINE HERO BATTLES"),
        FLinearColor(0.45f, 0.9f, 1.f), 18, 10, Font, 1.12f);
    DrawText(FString::Printf(TEXT("MAP seed=%u  zones %d  known %d/%d  cleared zones & rewards persist"),
        S.Seed, G->TotalZones(), G->KnownZones(), G->TotalZones()),
        FLinearColor::White, 18, 37, Font, 1.f);
    DrawText(FString::Printf(TEXT("DAY %d | MOVES %d/8 | GOLD %d | WOOD %d | ORE %d | DATA %d | PROTOCOL %d"),
        S.Day, S.MovesLeft, S.Gold, S.Wood, S.Ore, S.DataFragments, S.ProtocolTier),
        FLinearColor(0.89f, 0.91f, 0.75f), 18, 62, Font);
    DrawText(FString::Printf(TEXT("HERO %s  |  TARGET %s  |  %s"),
        *S.HeroSite.ToString(), *G->GetSelection().ToString(),
        *G->GetSelectedPathInfo().Left(87)),
        FLinearColor(0.35f, 0.97f, 0.81f), 18, 90, Font);
    DrawText(TEXT("TAB/CLICK select | H advise | SHIFT+H inspect | ENTER confirm | SPACE day | R recruit | P protocol | K save | L load"),
        FLinearColor(0.83f, 0.89f, 0.98f), 18, 117, Font, 0.92f);

    DrawRect(FLinearColor(0.018f, 0.033f, 0.06f, 0.95f), 0, H - 86.f, W, 86.f);
    DrawText(G->GetLastMessage().Left(FMath::Clamp(static_cast<int32>(W / 8.f), 35, 170)),
        FLinearColor::White, 20, H - 74.f, Font, 1.02f);
    DrawText(TEXT("Separate, opt-in prototype | Dedicated schema-2 A/B saves | Grid V2 is instant (no 3D arena here) | No real LLM | Android unverified"),
        FLinearColor(0.7f, 0.83f, 0.95f), 20, H - 45.f, Font, 0.88f);
}
