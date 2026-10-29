#include "AIHeroesZonePreviewGameMode.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AIHeroesCameraPawn.h"
#include "AIHeroesCampaignSave.h"
#include "AIHeroesGameMode.h"
#include "AIHeroesZoneTemplate.h"
#include "GameFramework/PlayerController.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesZonePreviewIsolationTest,
    "AIHeroes.ZonePreview.SeparateGameModeAndNoCampaignController",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesZonePreviewIsolationTest::RunTest(const FString&)
{
    UClass* Preview = AAIHeroesZonePreviewGameMode::StaticClass();
    const AAIHeroesZonePreviewGameMode* Default =
        GetDefault<AAIHeroesZonePreviewGameMode>();
    TestTrue(TEXT("Preview is an Unreal GameMode"),
        Preview->IsChildOf(AGameModeBase::StaticClass()));
    TestFalse(TEXT("Preview is not the active campaign GameMode"),
        Preview->IsChildOf(AAIHeroesGameMode::StaticClass()));
    TestTrue(TEXT("Preview has a plain controller without campaign actions"),
        Default->PlayerControllerClass == APlayerController::StaticClass());
    TestTrue(TEXT("Preview uses free camera, not a strategic actor"),
        Default->DefaultPawnClass == AAIHeroesCameraPawn::StaticClass());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesZonePreviewGeneratedDraftTest,
    "AIHeroes.ZonePreview.GeneratedDraftCannotChangeCampaignTopology",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesZonePreviewGeneratedDraftTest::RunTest(const FString&)
{
    FAIHeroesGeneratedWorld World;
    FString Error;
    if (!TestTrue(TEXT("Isolated 3D preview seed generates"),
        FAIHeroesZoneTemplate::Generate(20261028u, 3, World, Error)))
    {
        return false;
    }
    TestTrue(TEXT("Preview draft validates"),
        FAIHeroesZoneTemplate::Validate(World, Error));
    TestEqual(TEXT("Preview has 10 zones"), World.Zones.Num(), 10);
    TestEqual(TEXT("Existing campaign save schema remains unchanged"),
        UAIHeroesCampaignSave::CurrentSchema, 1);
    TestTrue(TEXT("Future-world descriptor warns that it is not active gameplay"),
        FAIHeroesZoneTemplate::Describe(World).Contains(TEXT("NOT ACTIVE CAMPAIGN")));

    int32 NumberOfRenderedRoads = 0;
    for (const FAIHeroesAdventureLink& Link : World.Adventure.Links)
    {
        if (Link.From.ToString() < Link.To.ToString())
        {
            ++NumberOfRenderedRoads;
        }
    }
    TestEqual(TEXT("Each bidirectional road will render exactly once"),
        NumberOfRenderedRoads * 2, World.Adventure.Links.Num());
    const auto FirstGuard = FAIHeroesAdventurePath::Find(
        World.Adventure, TEXT("HQ"), TEXT("FOREST"), 1000);
    const auto FinalGoal = FAIHeroesAdventurePath::Find(
        World.Adventure, TEXT("HQ"), TEXT("RUINS"), 1000);
    TestTrue(TEXT("First guard is a valid encounter endpoint"),
        FirstGuard.IsReachableThisTurn() && FirstGuard.bBattleAtDestination);
    TestTrue(TEXT("Late mission cannot bypass the unbroken frontier"),
        FinalGoal.Status == EAIHeroesAdventurePathStatus::Unreachable);
    return true;
}
#endif
