#include "AIHeroesCampaignGuide.h"
#include "AIHeroesCampaignObjectives.h"
#include "AIHeroesExplorerLayout.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

namespace
{
FAIHeroesCampaignObjectiveInput TutorialCampaign()
{
    FAIHeroesCampaignObjectiveInput Input;
    auto Add=[&Input](const TCHAR* Id,EAIHeroesNodeType Type,bool Owned)
    {
        FAIHeroesObjectiveNode S;
        S.Id=FName(Id);S.Type=Type;S.bOwned=Owned;
        Input.Nodes.Add(S);
    };
    Add(TEXT("HQ"),EAIHeroesNodeType::Headquarters,true);
    Add(TEXT("FOREST"),EAIHeroesNodeType::ForestMine,false);
    Add(TEXT("ORE"),EAIHeroesNodeType::OreMine,false);
    Add(TEXT("OUTPOST"),EAIHeroesNodeType::Outpost,false);
    Add(TEXT("RUINS"),EAIHeroesNodeType::Ruins,false);
    return Input;
}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJourneyCampaignSource,
    "AIHeroes.PlayableJourney.CanonicalGoalSequence",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FJourneyCampaignSource::RunTest(const FString&)
{
    auto Input=TutorialCampaign();
    const FName Expected[]={FName(TEXT("FOREST")),FName(TEXT("ORE")),FName(TEXT("OUTPOST")),FName(TEXT("RUINS"))};
    const auto Russian=FAIHeroesCampaignObjectives::Evaluate(Input).ReportRu();
    TestEqual(TEXT("Localized dossier keeps four real goals and safety lines"),
        Russian.Num(),7);
    TestTrue(TEXT("Supply economy goal is player-readable"),
        Russian[1].Contains(TEXT("лесопилку")));
    TestTrue(TEXT("No fake AI weights claim in local report"),
        Russian.Last().Contains(TEXT("не загружены")));
    for(int32 I=0;I<4;++I)
    {
        const auto State=FAIHeroesCampaignObjectives::Evaluate(Input);
        TestEqual(TEXT("Canonical objective ID tracks uncaptured next site"),
            State.NextFocusId,Expected[I]);
        TestFalse(TEXT("Not yet a won campaign"),State.bVictory);
        Input.Nodes[I+1].bOwned=true;
    }
    auto State=FAIHeroesCampaignObjectives::Evaluate(Input);
    TestEqual(TEXT("After all sites, protocols are next"),
        State.NextFocusId,FName(TEXT("PROTOCOL")));
    for(int32 Tier=0;Tier<3;++Tier)
    {
        Input.ProtocolTier=Tier;
        State=FAIHeroesCampaignObjectives::Evaluate(Input);
        TestEqual(TEXT("Keep protocol goal until last upgrade"),
            State.NextFocusId,FName(TEXT("PROTOCOL")));
    }
    Input.ProtocolTier=3;
    State=FAIHeroesCampaignObjectives::Evaluate(Input);
    TestEqual(TEXT("Then upgrade model slot"),
        State.NextFocusId,FName(TEXT("MODEL_SLOT")));
    Input.ModelSlotTier=2;
    State=FAIHeroesCampaignObjectives::Evaluate(Input);
    TestTrue(TEXT("Victory only when every original goal is met"),State.bVictory);
    TestEqual(TEXT("Victory gives a stable focus ID"),
        State.NextFocusId,FName(TEXT("VICTORY")));
    const auto Card=FAIHeroesCampaignGuide::Build(State,{});
    TestTrue(TEXT("Victory prompts a save, not another unneeded attack"),
        Card.NextStep.Contains(TEXT("Сохраните")));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJourneyNextAction,
    "AIHeroes.PlayableJourney.ContextualInstruction",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FJourneyNextAction::RunTest(const FString&)
{
    const auto P=FAIHeroesCampaignObjectives::Evaluate(TutorialCampaign());
    FAIHeroesCampaignGuideContext C;
    C.bTargetDiscovered=true;
    C.SelectedId=FName(TEXT("ORE"));
    auto Card=FAIHeroesCampaignGuide::Build(P,C);
    TestTrue(TEXT("Ask player to select current target"),
        Card.NextStep.Contains(TEXT("Лесопилка")));
    C.SelectedId=FName(TEXT("FOREST"));
    C.bRouteReady=true;
    Card=FAIHeroesCampaignGuide::Build(P,C);
    TestTrue(TEXT("Reachable selected site gets explicit confirmation"),
        Card.NextStep.Contains(TEXT("подтвердите")));
    C.bRouteReady=false;C.bNeedsMoreMoves=true;C.MissingMoves=3;
    Card=FAIHeroesCampaignGuide::Build(P,C);
    TestTrue(TEXT("Movement budget explained"),Card.Detail.Contains(TEXT("3")));
    C.bNeedsMoreMoves=false;
    Card=FAIHeroesCampaignGuide::Build(P,C);
    TestTrue(TEXT("Blocked path never promised"),
        Card.NextStep.Contains(TEXT("закрыт")));
    C.bTravelMoving=true;
    Card=FAIHeroesCampaignGuide::Build(P,C);
    TestTrue(TEXT("Moving hero does not get a second order"),
        Card.NextStep.Contains(TEXT("пути")));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJourneyOverDayRoute,
    "AIHeroes.PlayableJourney.OverDayRouteNeedsWaypoint",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FJourneyOverDayRoute::RunTest(const FString&)
{
    const auto Progress=FAIHeroesCampaignObjectives::Evaluate(TutorialCampaign());
    FAIHeroesCampaignGuideContext C;
    C.SelectedId=FName(TEXT("FOREST"));
    C.bTargetDiscovered=true;
    C.FullDayMoveBudget=4;
    C.bNeedsMoreMoves=true;
    C.MissingMoves=1;

    C.RouteTotalCost=3;
    auto Card=FAIHeroesCampaignGuide::Build(Progress,C);
    TestTrue(TEXT("Short legal route can be reached after End Day"),
        Card.NextStep.Contains(TEXT("закончите день")));
    TestTrue(TEXT("Missing move budget is shown honestly"),
        Card.Detail.Contains(TEXT("1")));

    C.RouteTotalCost=5;C.IntermediateOwnedSite=FName(TEXT("HQ"));
    Card=FAIHeroesCampaignGuide::Build(Progress,C);
    TestTrue(TEXT("Route longer than every full day asks for staging"),
        Card.NextStep.Contains(TEXT("Через Цитадель Искры")));
    TestTrue(TEXT("Only an already-owned stage is named by the caller"),
        Card.NextStep.Contains(TEXT("Цитадель Искры")));
    TestTrue(TEXT("Total cost is surfaced"),
        Card.Detail.Contains(TEXT("5")));
    TestFalse(TEXT("Do not trap the player in endless End Day advice"),
        Card.Detail.Contains(TEXT("Конец хода")));

    C.IntermediateOwnedSite=NAME_None;
    Card=FAIHeroesCampaignGuide::Build(Progress,C);
    TestTrue(TEXT("No verified owned stop: show generic safe fallback"),
        Card.Detail.Contains(TEXT("Ищите свою точку")));
    TestFalse(TEXT("No hidden site is fabricated"),
        Card.Detail.Contains(TEXT("Цитадель")));

    C.IntermediateOwnedSite=FName(TEXT("SECRET"));
    Card=FAIHeroesCampaignGuide::Build(Progress,C);
    TestFalse(TEXT("Unrecognized site identifiers are never printed"),
        Card.Detail.Contains(TEXT("SECRET")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJourneySafety,
    "AIHeroes.PlayableJourney.FogAndArmySafety",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FJourneySafety::RunTest(const FString&)
{
    const auto P=FAIHeroesCampaignObjectives::Evaluate(TutorialCampaign());
    FAIHeroesCampaignGuideContext C;
    C.bTargetDiscovered=false;
    const auto Fog=FAIHeroesCampaignGuide::Build(P,C);
    TestTrue(TEXT("Unknown site prompts exploration rather than a route"),
        Fog.NextStep.Contains(TEXT("Разведайте")));
    TestFalse(TEXT("No hidden guards shown"),Fog.Detail.Contains(TEXT("Охрана")));
    C.bTargetDiscovered=true;C.bHasArmy=false;
    const auto NoArmy=FAIHeroesCampaignGuide::Build(P,C);
    TestTrue(TEXT("Empty army prompts recruitment"),
        NoArmy.NextStep.Contains(TEXT("наймите")));
    TestEqual(TEXT("No campaign mutation from a guide query"),
        FAIHeroesCampaignObjectives::Evaluate(TutorialCampaign()).NextFocusId,
        FName(TEXT("FOREST")));
    FAIHeroesCampaignProgress Invalid;
    Invalid.Total=FAIHeroesCampaignObjectives::GoalCount;
    const auto Fallback=FAIHeroesCampaignGuide::Build(Invalid,C);
    TestTrue(TEXT("Invalid objective has no phantom site"),
        Fallback.FocusId.IsNone());
    TestTrue(TEXT("Invalid objective presents read-only fallback"),
        Fallback.NextStep.Contains(TEXT("список")));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJourneyUpgradeInstructions,
    "AIHeroes.PlayableJourney.UpgradeMilestones",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FJourneyUpgradeInstructions::RunTest(const FString&)
{
    auto Input=TutorialCampaign();
    for(int I=1;I<Input.Nodes.Num();++I)Input.Nodes[I].bOwned=true;
    const auto Proto=FAIHeroesCampaignGuide::Build(
        FAIHeroesCampaignObjectives::Evaluate(Input),{});
    TestTrue(TEXT("Protocol guide names player-controlled progression UI"),
        Proto.NextStep.Contains(TEXT("[G]")));
    Input.ProtocolTier=3;
    const auto Model=FAIHeroesCampaignGuide::Build(
        FAIHeroesCampaignObjectives::Evaluate(Input),{});
    TestTrue(TEXT("Model slot guide names a real base UI button"),
        Model.NextStep.Contains(TEXT("[G]")));
    TestFalse(TEXT("Slot guide does not pretend a neural model is installed"),
        Model.Detail.Contains(TEXT("модель загружена")));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FJourneyClickToGoals,
    "AIHeroes.PlayableJourney.GoalsRibbonTapAllScreens",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FJourneyClickToGoals::RunTest(const FString&)
{
    const FVector2D Viewports[]={{320,640},{480,800},{640,720},
        {960,720},{1280,720},{1920,1080}};
    for(const auto View:Viewports)
    {
        const auto F=FAIHeroesExplorerLayout::Frame(View);
        if(!TestTrue(TEXT("Supported viewport"),F.bSupported))continue;
        const FVector2D P(F.Mission.X+F.Mission.W*.5f,
            F.Mission.Y+F.Mission.H*.5f);
        TestEqual(TEXT("Tapping mission ribbon opens canonical objective dossier"),
            FAIHeroesExplorerLayout::Hit(P,View,false),202);
        TestTrue(TEXT("Open overlay prevents hidden mission click-through"),
            FAIHeroesExplorerLayout::Hit(P,View,true)!=202);
        TestTrue(TEXT("Mission is visually separate from dock"),
            F.Mission.Y+F.Mission.H<F.Dock.Y);
    }
    return true;
}
#endif