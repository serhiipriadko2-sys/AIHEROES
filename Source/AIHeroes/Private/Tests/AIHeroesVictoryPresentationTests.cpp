#include "AIHeroesVictoryPresentation.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AIHeroesCampaignObjectives.h"
#include "AIHeroesExplorerLayout.h"
#include "Misc/AutomationTest.h"

namespace
{
FAIHeroesCampaignProgress CompleteCampaign()
{
    FAIHeroesCampaignObjectiveInput Input;
    Input.ProtocolTier = 3;
    Input.ModelSlotTier = 2;
    auto Add = [&Input](const TCHAR* Id, EAIHeroesNodeType Kind)
    {
        FAIHeroesObjectiveNode Node;
        Node.Id = FName(Id);
        Node.Type = Kind;
        Node.bOwned = true;
        Input.Nodes.Add(Node);
    };
    Add(TEXT("HQ"), EAIHeroesNodeType::Headquarters);
    Add(TEXT("FOREST"), EAIHeroesNodeType::ForestMine);
    Add(TEXT("ORE"), EAIHeroesNodeType::OreMine);
    Add(TEXT("OUTPOST"), EAIHeroesNodeType::Outpost);
    Add(TEXT("RUINS"), EAIHeroesNodeType::Ruins);
    return FAIHeroesCampaignObjectives::Evaluate(Input);
}

bool Intersect(const FAIHeroesHUDActionRect& A,
               const FAIHeroesHUDActionRect& B)
{
    return A.W > 0.f && A.H > 0.f && B.W > 0.f && B.H > 0.f &&
        A.X < B.X + B.W && A.X + A.W > B.X &&
        A.Y < B.Y + B.H && A.Y + A.H > B.Y;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesVictoryOverlayHonestyTest,
    "AIHeroes.Playability.VictoryOverlay.OnlyFromCompleteObjectives",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesVictoryOverlayHonestyTest::RunTest(const FString&)
{
    FAIHeroesCampaignProgress Won = CompleteCampaign();
    if (!TestTrue(TEXT("Four authoritative goals complete"),
            FAIHeroesVictoryPresentation::IsAuthoritativeVictory(Won)))
        return false;
    TestTrue(TEXT("Victory state comes from real campaign rules"), Won.bVictory);

    FAIHeroesCampaignProgress Forged = Won;
    Forged.Completed = 3;
    TestFalse(TEXT("Forged bVictory without 4 completed goals rejected"),
        FAIHeroesVictoryPresentation::IsAuthoritativeVictory(Forged));
    Forged = Won;
    Forged.Goals[2].Current = 0;
    TestFalse(TEXT("Stale protocol state cannot show victory"),
        FAIHeroesVictoryPresentation::IsAuthoritativeVictory(Forged));
    Forged = Won;
    Forged.Total = 3;
    TestFalse(TEXT("Invalid goal count cannot show victory"),
        FAIHeroesVictoryPresentation::IsAuthoritativeVictory(Forged));
    Forged = Won;
    Forged.bVictory = false;
    TestFalse(TEXT("A completed but not authorized state remains hidden"),
        FAIHeroesVictoryPresentation::IsAuthoritativeVictory(Forged));

    const FAIHeroesCampaignObjectiveInput None;
    const FAIHeroesCampaignProgress Start =
        FAIHeroesCampaignObjectives::Evaluate(None);
    TestFalse(TEXT("Fresh game never displays victory"),
        FAIHeroesVictoryPresentation::IsAuthoritativeVictory(Start));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesVictoryOverlayLayoutTest,
    "AIHeroes.Playability.VictoryOverlay.MapBoundsAndInputs",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesVictoryOverlayLayoutTest::RunTest(const FString&)
{
    const FAIHeroesCampaignProgress Won = CompleteCampaign();
    const FVector2D WideViews[] = {
        FVector2D(1150.f, 640.f),
        FVector2D(1280.f, 720.f),
        FVector2D(1600.f, 900.f)
    };
    for (const FVector2D& View : WideViews)
    {
        const auto Layout = FAIHeroesExplorerLayout::Frame(View);
        const auto Banner = FAIHeroesVictoryPresentation::Banner(Won, View, false);
        if (!TestTrue(TEXT("Victory banner appears for wide playable screen"),
                Banner.W >= 450.f && Banner.H >= 80.f))
            return false;
        TestTrue(TEXT("Banner starts inside the visible map"),
            Banner.X >= Layout.Map.X && Banner.Y >= Layout.Map.Y &&
            Banner.X + Banner.W <= Layout.Map.X + Layout.Map.W &&
            Banner.Y + Banner.H <= Layout.Map.Y + Layout.Map.H);
        TestFalse(TEXT("Mission ribbon remains clickable"),
            Intersect(Banner, Layout.Mission));
        TestFalse(TEXT("Inspector is not covered"),
            Intersect(Banner, Layout.Inspector));
        TestFalse(TEXT("Dock controls remain accessible"),
            Intersect(Banner, Layout.Dock));
        TestFalse(TEXT("Fog-safe overview remains visible"),
            Intersect(Banner, Layout.Overview));
        const FVector2D Centre(Banner.X + Banner.W * 0.5f,
                               Banner.Y + Banner.H * 0.5f);
        TestTrue(TEXT("Matching HUD click guard consumes the painted plaque"),
            Banner.Contains(Centre));
    }

    for (const FVector2D& View : {
        FVector2D(960.f, 540.f),
        FVector2D(640.f, 520.f),
        FVector2D(480.f, 800.f),
        FVector2D(320.f, 480.f)
    })
    {
        const auto Banner=FAIHeroesVictoryPresentation::Banner(Won,View,false);
        TestTrue(TEXT("Narrow-screen mission card stays the only victory UI"),
            Banner.W <= 0.f && Banner.H <= 0.f);
    }
    TestTrue(TEXT("Utility menu hides plaque to avoid overlapping input"),
        FAIHeroesVictoryPresentation::Banner(
            Won, FVector2D(1280.f, 720.f), true).W <= 0.f);

    const FAIHeroesCampaignProgress Invalid;
    TestTrue(TEXT("Invalid state cannot allocate banner"),
        FAIHeroesVictoryPresentation::Banner(
            Invalid,FVector2D(1280.f,720.f),false).W <= 0.f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesVictoryOverlayReadOnlyClickTest,
    "AIHeroes.Playability.VictoryOverlay.ReadOnlyGoalsHitTest",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FAIHeroesVictoryOverlayReadOnlyClickTest::RunTest(const FString&)
{
    const FAIHeroesCampaignProgress Won = CompleteCampaign();
    const FVector2D Viewport(1280.f, 720.f);
    const auto Rect = FAIHeroesVictoryPresentation::Banner(
        Won, Viewport, false);
    if (!TestTrue(TEXT("Clickable plaque has a real screen rectangle"),
            Rect.W > 0.f && Rect.H > 0.f))
        return false;

    const FVector2D Center(Rect.X + Rect.W * 0.5f,
                           Rect.Y + Rect.H * 0.5f);
    TestEqual(TEXT("Victory plaque opens only the existing goals action"),
        FAIHeroesVictoryPresentation::HitTest(
            Won, Viewport, false, Center),
        FAIHeroesVictoryPresentation::OpenGoalsAction);
    TestEqual(TEXT("Click outside plaque cannot issue an action"),
        FAIHeroesVictoryPresentation::HitTest(
            Won, Viewport, false, FVector2D(Rect.X - 4.f, Rect.Y)),
        INDEX_NONE);
    TestEqual(TEXT("Open utility menu hides clickable plaque"),
        FAIHeroesVictoryPresentation::HitTest(
            Won, Viewport, true, Center),
        INDEX_NONE);
    TestEqual(TEXT("Compact viewport has no invisible victory hit target"),
        FAIHeroesVictoryPresentation::HitTest(
            Won, FVector2D(640.f, 520.f), false, Center),
        INDEX_NONE);

    FAIHeroesCampaignProgress Forged = Won;
    Forged.Goals[0].Current = 0;
    TestEqual(TEXT("Incomplete objectives cannot create an action"),
        FAIHeroesVictoryPresentation::HitTest(
            Forged, Viewport, false, Center),
        INDEX_NONE);
    return true;
}
#endif
