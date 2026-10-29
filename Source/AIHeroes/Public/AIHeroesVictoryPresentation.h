#pragma once

#include "CoreMinimal.h"
#include "AIHeroesCampaignObjectives.h"
#include "AIHeroesHUDLayout.h"

// Post-victory plaque for the PRIMARY five-site campaign.
// Clicking opens the already-existing, read-only Goals dossier (action 202).
// No reward, move, save, unlock, fog reveal or inference occurs here.
// Layout is shared by drawing and hit-testing to prevent map click-through.
class AIHEROES_API FAIHeroesVictoryPresentation
{
public:
    static bool IsAuthoritativeVictory(
        const FAIHeroesCampaignProgress& Progress);

    // Zero rectangle = no overlay. Disabled for small viewports, modals and
    // the open utility menu, leaving the original campaign controls intact.
    static constexpr int32 OpenGoalsAction = 202; // Existing read-only O/Goals action.
    static FAIHeroesHUDActionRect Banner(
        const FAIHeroesCampaignProgress& Progress,
        const FVector2D& Viewport,
        bool bUtilityMenuOpen);

    // The entire visible banner opens the existing read-only goals dossier.
    // Never dispatch an action when victory is false, a menu is open,
    // or the pointer is outside the painted hit area.
    static int32 HitTest(
        const FAIHeroesCampaignProgress& Progress,
        const FVector2D& Viewport,
        bool bUtilityMenuOpen,
        const FVector2D& ScreenPoint);
};
