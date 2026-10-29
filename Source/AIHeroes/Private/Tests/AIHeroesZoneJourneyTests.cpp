#include "AIHeroesZoneJourney.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AIHeroesZoneExpeditionRules.h"
#include "Misc/AutomationTest.h"
#include "UObject/Class.h"

namespace
{
FAIHeroesZoneExpeditionState Start(uint32 Seed=20261029u,int32 Branches=3)
{
    FAIHeroesZoneExpeditionState S;
    FString Error;
    FAIHeroesZoneExpeditionRules::Create(Seed,Branches,S,Error);
    return S;
}
bool Identical(const FAIHeroesZoneExpeditionState& A,
               const FAIHeroesZoneExpeditionState& B)
{
    return FAIHeroesZoneExpeditionState::StaticStruct()
        ->CompareScriptStruct(&A,&B,0);
}
FAIHeroesAdventureGraph StagedWorld()
{
    FAIHeroesAdventureGraph G;
    G.Revision=42;
    G.Nodes.Add({FName(TEXT("HQ")),true,true,false});
    G.Nodes.Add({FName(TEXT("A")),true,true,false});
    G.Nodes.Add({FName(TEXT("B")),true,true,false});
    G.Nodes.Add({FName(TEXT("GOAL")),true,true,false});
    // The directly cheapest path costs nine and is NEVER a legal
    // single 8-move order. The known longer staged path costs twelve.
    G.Links.Add({FName(TEXT("HQ")),FName(TEXT("GOAL")),9,true});
    G.Links.Add({FName(TEXT("HQ")),FName(TEXT("A")),4,true});
    G.Links.Add({FName(TEXT("A")),FName(TEXT("B")),4,true});
    G.Links.Add({FName(TEXT("B")),FName(TEXT("GOAL")),4,true});
    return G;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FZoneJourneyImmediateTest,
    "AIHeroes.ZoneJourney.ImmediateMoveAndNoSideEffects",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FZoneJourneyImmediateTest::RunTest(const FString&)
{
    auto S=Start();
    const auto Before=S;
    const auto P=FAIHeroesZoneJourney::Plan(S,FName(TEXT("FOREST")));
    if(!TestTrue(TEXT("Known guarded first objective is ready"),
        P.Status==EAIHeroesJourneyStatus::ReadyNow &&
        P.Steps.Num()==1)) return false;
    TestEqual(TEXT("Known goal only"),P.Goal,FName(TEXT("FOREST")));
    TestEqual(TEXT("One human confirmed move"),P.TravelConfirmations,1);
    TestEqual(TEXT("No implicit day advances"),P.DayAdvances,0);
    TestEqual(TEXT("Exact first route cost"),P.TotalMovement,3);
    TestEqual(TEXT("Player's day retained"),P.ArrivalDay,1);
    TestEqual(TEXT("Guard outcome remains unknown"),P.bGuardianOutcomeUnknown,true);
    TestEqual(TEXT("Guarded action is a proposal"),
        P.Steps[0].Kind,EAIHeroesJourneyStepKind::ConfirmTravel);
    TestTrue(TEXT("A forecast never claims a victory"),
        !P.Explanation.Contains(TEXT("victory")) &&
        !P.Explanation.Contains(TEXT("%")));
    TestTrue(TEXT("Whole campaign bytewise semantics unchanged"),
        Identical(S,Before));
    const auto Again=FAIHeroesZoneJourney::Plan(S,FName(TEXT("FOREST")));
    TestEqual(TEXT("Deterministic route cost"),Again.TotalMovement,P.TotalMovement);
    TestEqual(TEXT("Deterministic route state"),Again.Status,P.Status);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FZoneJourneyRestTest,
    "AIHeroes.ZoneJourney.ExplicitNextDayAndDayCap",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FZoneJourneyRestTest::RunTest(const FString&)
{
    auto S=Start();
    S.MovesLeft=0;
    const auto Before=S;
    const auto P=FAIHeroesZoneJourney::Plan(S,FName(TEXT("FOREST")));
    if(!TestTrue(TEXT("No teleport after exhausting moves"),
        P.HasPlan() && P.Steps.Num()==2)) return false;
    TestEqual(TEXT("Rest is explicit first user action"),
        P.Steps[0].Kind,EAIHeroesJourneyStepKind::EndDay);
    TestEqual(TEXT("Rest doesn't claim new territory"),
        P.Steps[0].Destination,NAME_None);
    TestEqual(TEXT("Next day restores exactly eight movement"),
        P.Steps[0].MovesRemainingAfter,8);
    TestEqual(TEXT("Battle still needs player confirmation"),
        P.Steps[1].Kind,EAIHeroesJourneyStepKind::ConfirmTravel);
    TestEqual(TEXT("Known FOREST cost unchanged"),
        P.Steps[1].MoveCost,3);
    TestEqual(TEXT("Arrival is tomorrow not automatically today"),
        P.ArrivalDay,2);
    TestTrue(TEXT("Planner never invokes EndPlayerDay"),
        Identical(S,Before));
    S.Day=FAIHeroesZoneExpeditionRules::MaxDay;
    const auto AtCap=FAIHeroesZoneJourney::Plan(S,FName(TEXT("FOREST")));
    TestEqual(TEXT("Can't plan fictional future day after calendar cap"),
        AtCap.Status,EAIHeroesJourneyStatus::NoLegalItinerary);
    TestTrue(TEXT("No phantom steps after cap"),AtCap.Steps.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FZoneJourneyStagingTest,
    "AIHeroes.ZoneJourney.TrueStagingVersusNineMoveShortcut",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FZoneJourneyStagingTest::RunTest(const FString&)
{
    const auto G=StagedWorld();
    const auto Direct=FAIHeroesAdventurePath::Find(
        G,FName(TEXT("HQ")),FName(TEXT("GOAL")),8);
    TestEqual(TEXT("Original shortest path violates daily budget"),
        Direct.Status,EAIHeroesAdventurePathStatus::NeedsMoreMoves);
    TestEqual(TEXT("Unusable direct path costs nine"),Direct.TotalCost,9);
    const auto P=FAIHeroesZoneJourney::PlanVisibleGraphForTests(
        G,FName(TEXT("HQ")),FName(TEXT("GOAL")),8,2,true);
    if(!TestTrue(TEXT("Legal staged route found"),
        P.HasPlan() && P.Steps.Num()==3))return false;
    TestEqual(TEXT("Equal-cost tie uses lexical first known A"),P.Steps[0].Destination,FName(TEXT("A")));
    TestEqual(TEXT("HQ to A is a legal four-move order"),P.Steps[0].MoveCost,4);
    TestEqual(TEXT("Second action advances human day"),P.Steps[1].Kind,EAIHeroesJourneyStepKind::EndDay);
    TestEqual(TEXT("Final target follows new day"),P.Steps[2].Destination,FName(TEXT("GOAL")));
    TestEqual(TEXT("Final staged A to GOAL route costs eight"),P.Steps[2].MoveCost,8);
    TestEqual(TEXT("No partial tile moves"),P.TotalMovement,12);
    TestEqual(TEXT("Only one day advance"),P.DayAdvances,1);
    TestEqual(TEXT("Target reached on day three"),P.ArrivalDay,3);
    TestEqual(TEXT("Only two movement confirmations"),P.TravelConfirmations,2);
    // Verify each future leg against actual directed known graph, not geometry.
    FName From(TEXT("HQ"));
    for(const auto& Step:P.Steps)
        if(Step.Kind==EAIHeroesJourneyStepKind::ConfirmTravel)
        {
            const auto R=FAIHeroesAdventurePath::Find(
                G,From,Step.Destination,8);
            TestTrue(TEXT("Every stage actually executable in a full day"),
                R.IsReachableThisTurn());
            TestEqual(TEXT("No invented travel prices"),Step.MoveCost,R.TotalCost);
            From=Step.Destination;
        }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FZoneJourneyTieTest,
    "AIHeroes.ZoneJourney.CanonicalEqualCostChoices",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FZoneJourneyTieTest::RunTest(const FString&)
{
    FAIHeroesAdventureGraph G;
    G.Revision=17;
    for(const TCHAR* Name:{TEXT("HQ"),TEXT("B"),TEXT("A"),TEXT("GOAL")})
        G.Nodes.Add({FName(Name),true,true,false});
    G.Links.Add({FName(TEXT("HQ")),FName(TEXT("A")),5,true});
    G.Links.Add({FName(TEXT("HQ")),FName(TEXT("B")),5,true});
    G.Links.Add({FName(TEXT("A")),FName(TEXT("GOAL")),5,true});
    G.Links.Add({FName(TEXT("B")),FName(TEXT("GOAL")),5,true});
    const auto P=FAIHeroesZoneJourney::PlanVisibleGraphForTests(
        G,FName(TEXT("HQ")),FName(TEXT("GOAL")),5,1,true);
    if(!TestTrue(TEXT("Two-stage choice exists"),P.Steps.Num()==3))
        return false;
    TestEqual(TEXT("Lexical site A breaks truly identical options"),
        P.Steps[0].Destination,FName(TEXT("A")));
    TestEqual(TEXT("Two equal-day itineraries preserve earliest arrival"),
        P.ArrivalDay,2);
    const auto Again=FAIHeroesZoneJourney::PlanVisibleGraphForTests(
        G,FName(TEXT("HQ")),FName(TEXT("GOAL")),5,1,true);
    TestEqual(TEXT("Repeated graph produces same stop"),
        Again.Steps[0].Destination,P.Steps[0].Destination);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FZoneJourneyGuardFogTest,
    "AIHeroes.ZoneJourney.GuardWallFogAndTamperingFailClosed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FZoneJourneyGuardFogTest::RunTest(const FString&)
{
    FAIHeroesAdventureGraph G;
    G.Revision=51;
    G.Nodes.Add({FName(TEXT("HQ")),true,true,false});
    G.Nodes.Add({FName(TEXT("GUARD")),true,true,true});
    G.Nodes.Add({FName(TEXT("GOAL")),true,true,false});
    G.Nodes.Add({FName(TEXT("SECRET")),false,true,false});
    G.Links.Add({FName(TEXT("HQ")),FName(TEXT("GUARD")),3,true});
    G.Links.Add({FName(TEXT("GUARD")),FName(TEXT("GOAL")),3,true});
    G.Links.Add({FName(TEXT("HQ")),FName(TEXT("SECRET")),1,true});
    const auto BehindGuard=FAIHeroesZoneJourney::PlanVisibleGraphForTests(
        G,FName(TEXT("HQ")),FName(TEXT("GOAL")),8,1,true);
    TestEqual(TEXT("No bypass across hostile checkpoint"),
        BehindGuard.Status,EAIHeroesJourneyStatus::NoLegalItinerary);
    TestTrue(TEXT("No staged plan through enemy guard"),
        BehindGuard.Steps.IsEmpty());
    const auto Hidden=FAIHeroesZoneJourney::PlanVisibleGraphForTests(
        G,FName(TEXT("HQ")),FName(TEXT("SECRET")),8,1,true);
    TestEqual(TEXT("Unknown node rejected even when caller supplied oracle"),
        Hidden.Status,EAIHeroesJourneyStatus::UnknownDestination);
    TestFalse(TEXT("No hidden name disclosed in explanation"),
        Hidden.Explanation.Contains(TEXT("SECRET")));
    const auto NoArmy=FAIHeroesZoneJourney::PlanVisibleGraphForTests(
        G,FName(TEXT("HQ")),FName(TEXT("GUARD")),8,1,false);
    TestEqual(TEXT("No living army cannot plan a battle"),
        NoArmy.Status,EAIHeroesJourneyStatus::NoLivingTroops);
    const auto Armed=FAIHeroesZoneJourney::PlanVisibleGraphForTests(
        G,FName(TEXT("HQ")),FName(TEXT("GUARD")),8,1,true);
    TestTrue(TEXT("Guard is an end-stage unknown outcome only"),
        Armed.HasPlan() && Armed.bGuardianOutcomeUnknown &&
        Armed.Steps.Num()==1 && Armed.Steps[0].bBattleAtDestination);
    auto S=Start();
    ++S.MapRevision;
    const auto Tampered=FAIHeroesZoneJourney::Plan(
        S,FName(TEXT("FOREST")));
    TestEqual(TEXT("Tampered campaign validation fails closed"),
        Tampered.Status,EAIHeroesJourneyStatus::InvalidCampaign);
    TestEqual(TEXT("No map revision on rejected campaign"),
        Tampered.SourceRevision,0u);
    TestTrue(TEXT("No partial hidden path after invalid state"),
        Tampered.Steps.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FZoneJourneySeedSweepTest,
    "AIHeroes.ZoneJourney.SeedSweep640KnownOnlyNoMutation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FZoneJourneySeedSweepTest::RunTest(const FString&)
{
    int32 Passed=0;
    FString Error;
    for(uint32 Seed=0;Seed<128;++Seed)
        for(int32 Branches=0;Branches<=4;++Branches)
        {
            FAIHeroesZoneExpeditionState S;
            if(!FAIHeroesZoneExpeditionRules::Create(
                Seed,Branches,S,Error))continue;
            const auto Before=S;
            const auto Path=FAIHeroesZoneExpeditionRules::Preview(
                S,FName(TEXT("FOREST")));
            const auto P=FAIHeroesZoneJourney::Plan(
                S,FName(TEXT("FOREST")));
            if(P.HasPlan() && P.TravelConfirmations==1 &&
               P.TotalMovement==Path.TotalCost &&
               P.ArrivalDay==S.Day &&
               P.Steps.Num()==1 &&
               P.Steps[0].Destination==FName(TEXT("FOREST")) &&
               P.SourceRevision!=0 &&
               P.bGuardianOutcomeUnknown &&
               Identical(S,Before))
               ++Passed;
        }
    TestEqual(TEXT("640 generated campaigns: no oracle, no side-effects"),
        Passed,640);
    return true;
}
#endif
