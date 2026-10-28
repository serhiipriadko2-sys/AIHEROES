#include "AIHeroesCampaignObjectives.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AIHeroesCampaignSave.h"
#include "AIHeroesHUDLayout.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/Guid.h"

namespace
{
FAIHeroesCampaignObjectiveInput SampleMission()
{
    FAIHeroesCampaignObjectiveInput Input;
    const auto Add = [&Input](const TCHAR* Id, EAIHeroesNodeType Type, bool bOwned)
    {
        FAIHeroesObjectiveNode Node;
        Node.Id = FName(Id);
        Node.Type = Type;
        Node.bOwned = bOwned;
        Input.Nodes.Add(Node);
    };
    Add(TEXT("HQ"), EAIHeroesNodeType::Headquarters, true);
    Add(TEXT("FOREST"), EAIHeroesNodeType::ForestMine, false);
    Add(TEXT("ORE"), EAIHeroesNodeType::OreMine, false);
    Add(TEXT("OUTPOST"), EAIHeroesNodeType::Outpost, false);
    Add(TEXT("RUINS"), EAIHeroesNodeType::Ruins, false);
    return Input;
}

FAIHeroesCampaignObjectiveInput FromSave(const UAIHeroesCampaignSave& Save)
{
    FAIHeroesCampaignObjectiveInput Input;
    Input.ProtocolTier = Save.ProtocolTier;
    Input.ModelSlotTier = Save.ModelSlotTier;
    for (const FAIHeroesSavedNode& Node : Save.NodeStates)
    {
        FAIHeroesObjectiveNode Source;
        Source.Id = Node.NodeId;
        Source.Type = Node.Type;
        Source.bOwned = Node.bOwned;
        Input.Nodes.Add(Source);
    }
    return Input;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesCampaignMilestonesTest,
    "AIHeroes.Objectives.CampaignProgression",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesCampaignMilestonesTest::RunTest(const FString& Parameters)
{
    FAIHeroesCampaignObjectiveInput Input = SampleMission();
    const FAIHeroesCampaignProgress Start = FAIHeroesCampaignObjectives::Evaluate(Input);
    TestEqual(TEXT("Four independent mission goals"), Start.Total, 4);
    TestEqual(TEXT("Unstarted mission progress"), Start.Completed, 0);
    TestFalse(TEXT("Fresh campaign must not be won"), Start.bVictory);
    TestTrue(TEXT("First objective is visible"), Start.NextAction.Contains(TEXT("FOREST")));
    TestEqual(TEXT("Mission dossier contains all goals and context"),
        Start.Report().Num(), 7);

    Input.Nodes[1].bOwned = true;
    const FAIHeroesCampaignProgress ForestOnly = FAIHeroesCampaignObjectives::Evaluate(Input);
    TestEqual(TEXT("One economy node is partial progress"), ForestOnly.Goals[0].Current, 1);
    TestTrue(TEXT("Next economic goal is ore"), ForestOnly.NextAction.Contains(TEXT("ORE")));

    Input.Nodes[2].bOwned = true;
    const FAIHeroesCampaignProgress Economy = FAIHeroesCampaignObjectives::Evaluate(Input);
    TestTrue(TEXT("Both mines complete economy milestone"), Economy.Goals[0].IsComplete());
    TestEqual(TEXT("One goal complete"), Economy.Completed, 1);

    Input.Nodes[3].bOwned = true;
    Input.Nodes[4].bOwned = true;
    const FAIHeroesCampaignProgress FreedMap = FAIHeroesCampaignObjectives::Evaluate(Input);
    TestEqual(TEXT("Map liberation alone cannot win the AI evolution campaign"),
        FreedMap.Completed, 2);
    TestFalse(TEXT("Remaining protocol gates are real goals"), FreedMap.bVictory);

    Input.ProtocolTier = 3;
    TestEqual(TEXT("All actual offline protocols give third milestone"),
        FAIHeroesCampaignObjectives::Evaluate(Input).Completed, 3);

    Input.ModelSlotTier = 2;
    const FAIHeroesCampaignProgress Won = FAIHeroesCampaignObjectives::Evaluate(Input);
    TestTrue(TEXT("Victory needs liberation AND all progression"), Won.bVictory);
    TestEqual(TEXT("All four goals complete"), Won.Completed, 4);
    TestTrue(TEXT("Victory is clearly reported"), Won.Headline().Contains(TEXT("VICTORY")));
    TestTrue(TEXT("Dossier does not pretend an LLM exists"),
        Won.Report().Last().Contains(TEXT("not a loaded neural checkpoint")));
    TestEqual(TEXT("Read-only evaluation is deterministic"),
        Won.Headline(), FAIHeroesCampaignObjectives::Evaluate(Input).Headline());
    TestEqual(TEXT("UI query never mutates progression"),
        Input.ProtocolTier, 3);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesCampaignIdentityTest,
    "AIHeroes.Objectives.DuplicateAndSpoofedNodesRejected",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesCampaignIdentityTest::RunTest(const FString& Parameters)
{
    FAIHeroesCampaignObjectiveInput Input = SampleMission();
    for (FAIHeroesObjectiveNode& Node : Input.Nodes)
    {
        Node.bOwned = true;
    }
    Input.ProtocolTier = 3;
    Input.ModelSlotTier = 2;

    TestTrue(TEXT("Valid complete map grants victory"),
        FAIHeroesCampaignObjectives::Evaluate(Input).bVictory);

    Input.Nodes[1].Type = EAIHeroesNodeType::Outpost;
    TestFalse(TEXT("Spoofed FOREST type does not count"),
        FAIHeroesCampaignObjectives::Evaluate(Input).bVictory);
    Input.Nodes[1].Type = EAIHeroesNodeType::ForestMine;

    // Never pass a reference into the same TArray while growing it.
    // Reallocation otherwise triggers Unreal's container-alias assertion.
    const FAIHeroesObjectiveNode DuplicateForest = Input.Nodes[1];
    Input.Nodes.Add(DuplicateForest);
    TestFalse(TEXT("Duplicate FOREST identifier does not count twice"),
        FAIHeroesCampaignObjectives::Evaluate(Input).bVictory);
    Input.Nodes.Pop();

    Input.Nodes[4].bOwned = false;
    TestFalse(TEXT("Unowned RUINS does not count"),
        FAIHeroesCampaignObjectives::Evaluate(Input).bVictory);

    Input.ProtocolTier = -7;
    TestEqual(TEXT("Invalid negative tiers cannot create progress"),
        FAIHeroesCampaignObjectives::Evaluate(Input).Goals[2].Current, 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesCampaignSaveReconstructionTest,
    "AIHeroes.Objectives.DerivedFromSaveGame",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesCampaignSaveReconstructionTest::RunTest(const FString& Parameters)
{
    UAIHeroesCampaignSave* Before = NewObject<UAIHeroesCampaignSave>(GetTransientPackage());
    Before->Day = 25;
    Before->ProtocolTier = 3;
    Before->ModelSlotTier = 2;
    const FAIHeroesCampaignObjectiveInput Mission = SampleMission();
    for (const FAIHeroesObjectiveNode& Source : Mission.Nodes)
    {
        FAIHeroesSavedNode State;
        State.NodeId = Source.Id;
        State.Type = Source.Type;
        State.bOwned = true;
        Before->NodeStates.Add(State);
    }
    const FString Slot = FString::Printf(TEXT("AIHEROES_GOALS_TEST_%s"),
        *FGuid::NewGuid().ToString(EGuidFormats::Digits));
    const bool bSaved = UGameplayStatics::SaveGameToSlot(Before, Slot, 0);
    TestTrue(TEXT("Existing SaveGame schema stores mission inputs"), bSaved);
    UAIHeroesCampaignSave* After = bSaved
        ? Cast<UAIHeroesCampaignSave>(UGameplayStatics::LoadGameFromSlot(Slot, 0))
        : nullptr;
    if (TestTrue(TEXT("Saved inputs load without new goal fields"), IsValid(After)))
    {
        TestTrue(TEXT("Victory correctly reconstructed from restored campaign"),
            FAIHeroesCampaignObjectives::Evaluate(FromSave(*After)).bVictory);
        After->ModelSlotTier = 0;
        TestFalse(TEXT("Restored campaign loses victory when tier is lower"),
            FAIHeroesCampaignObjectives::Evaluate(FromSave(*After)).bVictory);
    }
    if (bSaved)
    {
        TestTrue(TEXT("Unique test-only save cleaned up"),
            UGameplayStatics::DeleteGameInSlot(Slot, 0));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAIHeroesMissionTouchButtonTest,
    "AIHeroes.Objectives.TouchMissionButton",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAIHeroesMissionTouchButtonTest::RunTest(const FString& Parameters)
{
    for (const FVector2D Viewport : {FVector2D(1280.f, 720.f), FVector2D(480.f, 800.f)})
    {
        const FAIHeroesHUDActionRect Button =
            FAIHeroesHUDLayout::Rect(202, Viewport, false);
        TestTrue(TEXT("Goals button visible on PC and portrait layout"), Button.W > 0.f);
        const FVector2D Center(Button.X + Button.W * 0.5f,
                               Button.Y + Button.H * 0.5f);
        TestEqual(TEXT("Hit-test selects read-only mission action"),
            FAIHeroesHUDLayout::HitTest(Center, Viewport, false), 202);
        TestEqual(TEXT("Strategic mission button is absent during live battle"),
            FAIHeroesHUDLayout::HitTest(Center, Viewport, true), INDEX_NONE);
    }
    return true;
}
#endif
