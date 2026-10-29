#include "AIHeroesVictoryPresentation.h"

#include "AIHeroesExplorerLayout.h"

bool FAIHeroesVictoryPresentation::IsAuthoritativeVictory(
    const FAIHeroesCampaignProgress& Progress)
{
    if (!Progress.bVictory ||
        Progress.Completed != FAIHeroesCampaignObjectives::GoalCount ||
        Progress.Total != FAIHeroesCampaignObjectives::GoalCount ||
        Progress.Goals.Num() != FAIHeroesCampaignObjectives::GoalCount)
    {
        return false;
    }

    // A stale or forged bVictory alone must never draw the final accolade.
    for (const FAIHeroesCampaignGoal& Goal : Progress.Goals)
    {
        if (!Goal.IsComplete())
        {
            return false;
        }
    }
    return true;
}

FAIHeroesHUDActionRect FAIHeroesVictoryPresentation::Banner(
    const FAIHeroesCampaignProgress& Progress,
    const FVector2D& Viewport,
    bool bUtilityMenuOpen)
{
    if (!IsAuthoritativeVictory(Progress) || bUtilityMenuOpen ||
        !FMath::IsFinite(Viewport.X) || !FMath::IsFinite(Viewport.Y) ||
        Viewport.X < 1150.f || Viewport.Y < 640.f)
    {
        return {};
    }

    const FAIHeroesExplorerFrame Layout =
        FAIHeroesExplorerLayout::Frame(Viewport);
    if (!Layout.bSupported || !Layout.bWide ||
        Layout.Map.W <= 0.f || Layout.Map.H <= 0.f)
    {
        return {};
    }

    // Adjacent to the mission strip rather than over a navigation button.
    const float X = Layout.Mission.X + Layout.Mission.W + 14.f;
    // Keep the plaque inside the playable map even if the mission strip
    // moves independently in a future responsive layout.
    const float Y = FMath::Max(Layout.Mission.Y, Layout.Map.Y);
    const float MaxWidth = Layout.Map.X + Layout.Map.W - X - 12.f;
    constexpr float Height = 86.f;
    if (MaxWidth < 450.f ||
        Y + Height > Layout.Map.Y + Layout.Map.H)
    {
        return {};
    }

    return {X, Y, FMath::Min(570.f, MaxWidth), Height};
}

int32 FAIHeroesVictoryPresentation::HitTest(
    const FAIHeroesCampaignProgress& Progress,
    const FVector2D& Viewport,
    bool bUtilityMenuOpen,
    const FVector2D& ScreenPoint)
{
    const FAIHeroesHUDActionRect Rect = Banner(
        Progress, Viewport, bUtilityMenuOpen);
    return Rect.Contains(ScreenPoint) ? OpenGoalsAction : INDEX_NONE;
}
