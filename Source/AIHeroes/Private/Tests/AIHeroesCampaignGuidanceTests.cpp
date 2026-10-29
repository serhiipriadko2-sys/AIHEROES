#include "AIHeroesCampaignGuide.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AIHeroesGameMode.h"
#include "AIHeroesWorldNode.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMainGuidanceEmptyArmy,
    "AIHeroes.PlayableJourney.EmptyArmyGuidanceRespectsBattleRequirement",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMainGuidanceEmptyArmy::RunTest(const FString&)
{
    FAIHeroesCampaignProgress Progress;
    Progress.Total = FAIHeroesCampaignObjectives::GoalCount;
    Progress.NextFocusId = FName(TEXT("FOREST"));
    FAIHeroesCampaignGuideContext Context;
    Context.SelectedId = Progress.NextFocusId;
    Context.bTargetDiscovered = true;
    Context.bRouteReady = true;
    Context.bHasArmy = false;
    Context.bTargetGuarded = true;
    const auto Guarded = FAIHeroesCampaignGuide::Build(Progress, Context);
    TestTrue(TEXT("A guarded objective requires recruitment before an attack"),
        Guarded.NextStep.Contains(TEXT("наймите")));

    Context.bTargetGuarded = false;
    const auto Unguarded = FAIHeroesCampaignGuide::Build(Progress, Context);
    TestTrue(TEXT("A safe visit remains available with an empty army"),
        Unguarded.NextStep.Contains(TEXT("подтвердите")));
    TestFalse(TEXT("Do not make recruitment a fictitious movement requirement"),
        Unguarded.NextStep.Contains(TEXT("наймите")));
    TestFalse(TEXT("A read-only guide did not create an army"), Context.bHasArmy);
    TestEqual(TEXT("A read-only guide did not change the selected destination"),
        Context.SelectedId, Progress.NextFocusId);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMainGuidanceOrderEligibility,
    "AIHeroes.PlayableJourney.OrderEligibilityTruthTable",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMainGuidanceOrderEligibility::RunTest(const FString&)
{
    for (bool Ready : {false, true})
        for (bool Army : {false, true})
            for (bool Battle : {false, true})
                TestEqual(TEXT("Display order needs a route, and an army only for battle"),
                    FAIHeroesCampaignGuide::CanOfferOrder(Ready, Army, Battle),
                    Ready && (!Battle || Army));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMainGuidanceStagingAfterRest,
    "AIHeroes.PlayableJourney.ExhaustedMovesRestOnlyForStagingLeg",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMainGuidanceStagingAfterRest::RunTest(const FString&)
{
    FAIHeroesCampaignProgress Progress;
    Progress.Total = FAIHeroesCampaignObjectives::GoalCount;
    Progress.NextFocusId = FName(TEXT("OUTPOST"));
    FAIHeroesCampaignGuideContext Context;
    Context.SelectedId = Progress.NextFocusId;
    Context.bTargetDiscovered = true;
    Context.bNeedsMoreMoves = true;
    Context.RouteTotalCost = 5;
    Context.MissingMoves = 5;
    Context.bOwnedStopAfterRest = true;
    const auto Rest = FAIHeroesCampaignGuide::Build(Progress, Context);
    TestTrue(TEXT("No moves: recover before choosing a real staging leg"),
        Rest.NextStep.Contains(TEXT("Восстановите")));
    TestTrue(TEXT("Rest does not promise the complete five-move journey"),
        Rest.Detail.Contains(TEXT("Не весь путь сразу")));
    Context.IntermediateOwnedSite = FName(TEXT("HQ"));
    const auto Today = FAIHeroesCampaignGuide::Build(Progress, Context);
    TestTrue(TEXT("A verified reachable stop takes precedence over resting"),
        Today.NextStep.Contains(TEXT("Через Цитадель")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMainGuidanceReadOnlyPath,
    "AIHeroes.PlayableJourney.WaypointQueryDoesNotSelectOrReveal",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FMainGuidanceReadOnlyPath::RunTest(const FString&)
{
    const auto Values = UWorld::InitializationValues().AllowAudioPlayback(false)
        .CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false)
        .ShouldSimulatePhysics(false);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None,
        nullptr, true, ERHIFeatureLevel::Num, &Values);
    if (!TestNotNull(TEXT("Disposable query world"), World)) return false;
    if (GEngine) GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    auto* Game = World->SpawnActor<AAIHeroesGameMode>();
    if (TestNotNull(TEXT("Disposable game"), Game))
    {
        // Initialize the real new-campaign fog through its public lifecycle,
        // rather than exposing or modifying private scouting fields for QA.
        Game->DispatchBeginPlay();
        if (TestEqual(TEXT("Actual new campaign has five sites"), Game->Nodes.Num(), 5))
        {
            Game->SelectedNode = Game->Nodes[1];
            Game->MovesLeft = 1;
            const auto HeroBefore = Game->GetHeroStrategicLocation();
            const int32 GoldBefore = Game->Gold;
            const auto ScoutingBefore = Game->GetScoutingStatus();
            const auto EventBefore = Game->LastEvent;
            const auto HintBefore = Game->LastHint;
            const auto Ore = Game->GetTravelPathTo(FName(TEXT("ORE")));
            TestEqual(TEXT("Known stage has its real two-move cost"), Ore.TotalCost, 2);
            TestEqual(TEXT("A stage is not promised reachable on one remaining move"),
                Ore.Status, EAIHeroesAdventurePathStatus::NeedsMoreMoves);
            // Dedicated Fog/TravelV2 tests cover hidden destination denial.
            // This disposable GameMode leaves its private fog state alone.
            TestEqual(TEXT("Unknown identifier is rejected"),
                Game->GetTravelPathTo(FName(TEXT("SECRET"))).Status,
                EAIHeroesAdventurePathStatus::InvalidInput);
            TestTrue(TEXT("Current owned stop is reachable without spending"),
                Game->GetTravelPathTo(FName(TEXT("HQ"))).IsReachableThisTurn());
            TestEqual(TEXT("Queries do not change selection"), Game->SelectedNode.Get(), Game->Nodes[1].Get());
            TestEqual(TEXT("Queries do not spend moves"), Game->MovesLeft, 1);
            TestEqual(TEXT("Queries do not move the hero"), Game->GetHeroStrategicLocation(), HeroBefore);
            TestEqual(TEXT("Queries do not reveal fog"), Game->GetScoutingStatus(), ScoutingBefore);
            TestFalse(TEXT("The rejected hidden outpost remains unknown"),
                Game->IsNodeDiscovered(FName(TEXT("OUTPOST"))));
            TestEqual(TEXT("Queries do not spend gold"), Game->Gold, GoldBefore);
            TestEqual(TEXT("Queries do not publish events"), Game->LastEvent, EventBefore);
            TestEqual(TEXT("Queries do not rewrite advice"), Game->LastHint, HintBefore);
            TestFalse(TEXT("Queries do not begin combat"), Game->IsLiveBattleActive());
        }
    }
    World->DestroyWorld(false);
    if (GEngine) GEngine->DestroyWorldContext(World);
    return true;
}
#endif
