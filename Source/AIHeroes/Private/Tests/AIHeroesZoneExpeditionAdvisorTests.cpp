#include "AIHeroesZoneExpeditionAdvisor.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AIHeroesZoneExpeditionRules.h"
#include "Misc/AutomationTest.h"
#include "UObject/Class.h" // UScriptStruct::CompareScriptStruct
#include "UObject/Class.h"

namespace
{
FAIHeroesZoneExpeditionState NewExpedition(uint32 Seed = 20261029u,
                                           int32 Branches = 3)
{
    FAIHeroesZoneExpeditionState S;
    FString Error;
    FAIHeroesZoneExpeditionRules::Create(Seed, Branches, S, Error);
    return S;
}

bool IsKnown(const FAIHeroesAdventureGraph& G, FName Site)
{
    return G.Nodes.ContainsByPredicate(
        [Site](const FAIHeroesAdventureNode& N)
        { return N.Id == Site && N.bDiscovered; });
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FZoneExpeditionAdvisorKnownWorldTest,
    "AIHeroes.ZoneExpeditionAdvisor.OnlyKnownRoutesAndNoOrders",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FZoneExpeditionAdvisorKnownWorldTest::RunTest(const FString&)
{
    auto S = NewExpedition();
    FString Error;
    FAIHeroesAdventureGraph G;
    if (!TestTrue(TEXT("Visible graph exists"),
        FAIHeroesZoneExpeditionRules::PlayerView(S, G, Error))) return false;
    const auto Old = S;
    const auto Advice = FAIHeroesZoneExpeditionAdvisor::Advise(S);
    TestTrue(TEXT("Deterministic offline advisor returns a suggestion"),
        Advice.HasSuggestion());
    TestTrue(TEXT("Proposed site must actually be discovered"),
        IsKnown(G, Advice.SuggestedSite));
    TestFalse(TEXT("Never recommend an unknown checkpoint"),
        Advice.SuggestedSite == FName(TEXT("PASS")));
    TestFalse(TEXT("Never recommend an unknown objective"),
        Advice.SuggestedSite == FName(TEXT("RUINS")));
    TestTrue(TEXT("Adviser recommends a fully traversable path"),
        FAIHeroesAdventurePath::Find(G, S.HeroSite, Advice.SuggestedSite,
            S.MovesLeft).IsReachableThisTurn());
    TestEqual(TEXT("Projection revision preserved"), Advice.SourceRevision,
        G.Revision);
    const auto Repeat = FAIHeroesZoneExpeditionAdvisor::Advise(S);
    TestEqual(TEXT("Same state gives same destination"), Repeat.SuggestedSite,
        Advice.SuggestedSite);
    TestEqual(TEXT("Same state gives same explanation"), Repeat.Message,
        Advice.Message);
    TestEqual(TEXT("No fog expansion from advice"), S.DiscoveredMask,
        Old.DiscoveredMask);
    TestEqual(TEXT("No captures from advice"), S.ClearedMask, Old.ClearedMask);
    TestEqual(TEXT("No movement spent from advice"), S.MovesLeft,
        Old.MovesLeft);
    TestEqual(TEXT("No hero teleport from advice"), S.HeroSite,
        Old.HeroSite);
    TestEqual(TEXT("No gold purchase from advice"), S.Gold, Old.Gold);
    TestEqual(TEXT("No data purchase from advice"), S.DataFragments,
        Old.DataFragments);
    TestEqual(TEXT("No semantic journal changes"), S.LastEvent,
        Old.LastEvent);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FZoneExpeditionAdvisorTargetTest,
    "AIHeroes.ZoneExpeditionAdvisor.InspectKnownGuardAndHideUnknown",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FZoneExpeditionAdvisorTargetTest::RunTest(const FString&)
{
    auto S = NewExpedition();
    const auto Guard = FAIHeroesZoneExpeditionAdvisor::Advise(
        S, FName(TEXT("FOREST")));
    const auto Route = FAIHeroesZoneExpeditionRules::Preview(
        S, FName(TEXT("FOREST")));
    TestTrue(TEXT("Known forest is safe to inspect but has no auto-attack"),
        Guard.HasSuggestion() && Guard.bGuarded);
    TestEqual(TEXT("Advisor uses authoritative visible path cost"),
        Guard.TravelCost, Route.TotalCost);
    TestTrue(TEXT("Baseline protocol explicitly refuses fabricated forecast"),
        Guard.Message.Contains(TEXT("outcome UNKNOWN")));
    TestTrue(TEXT("Human confirmation remains required"),
        Guard.Message.Contains(TEXT("Human ENTER required")));

    const auto Invisible = FAIHeroesZoneExpeditionAdvisor::Advise(
        S, FName(TEXT("PASS")));
    const auto Far = FAIHeroesZoneExpeditionAdvisor::Advise(
        S, FName(TEXT("RUINS")));
    const auto Fake = FAIHeroesZoneExpeditionAdvisor::Advise(
        S, FName(TEXT("DOES_NOT_EXIST")));
    TestTrue(TEXT("Hidden destination is not an advice candidate"),
        Invisible.Status == EAIHeroesZoneAdviceStatus::UnknownTarget &&
        !Invisible.HasSuggestion() && Invisible.SuggestedSite.IsNone());
    TestEqual(TEXT("All hidden/unknown targets get same neutral message"),
        Invisible.Message, Far.Message);
    TestEqual(TEXT("Unknown forged node gets no more info"),
        Invisible.Message, Fake.Message);
    TestFalse(TEXT("Hidden PASS details are not leaked"),
        Invisible.Message.Contains(TEXT("PASS")));
    TestFalse(TEXT("No hidden RUINS details"),
        Invisible.Message.Contains(TEXT("RUINS")));
    TestEqual(TEXT("Unknown guard cost is zero"), Invisible.TravelCost, 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FZoneExpeditionAdvisorConstraintsTest,
    "AIHeroes.ZoneExpeditionAdvisor.NoArmyNoMovesAndCognitiveTiers",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FZoneExpeditionAdvisorConstraintsTest::RunTest(const FString&)
{
    auto S = NewExpedition();
    S.PlayerArmy.Empty();
    const auto NoArmy = FAIHeroesZoneExpeditionAdvisor::Advise(
        S, FName(TEXT("FOREST")));
    TestTrue(TEXT("Known guarded target warns about zero troops"),
        NoArmy.Status == EAIHeroesZoneAdviceStatus::NoArmy);
    TestFalse(TEXT("No troop exception grants a playable attack"),
        NoArmy.HasSuggestion());
    S = NewExpedition();
    S.MovesLeft = 0;
    const auto NoMoves = FAIHeroesZoneExpeditionAdvisor::Advise(
        S, FName(TEXT("FOREST")));
    TestTrue(TEXT("No movement produces a needs-more-moves advisory"),
        NoMoves.Status == EAIHeroesZoneAdviceStatus::NeedsMovement &&
        NoMoves.MissingMoves > 0);
    TestFalse(TEXT("No movement is never ready to authorize"),
        NoMoves.HasSuggestion());
    const auto General = FAIHeroesZoneExpeditionAdvisor::Advise(S);
    TestTrue(TEXT("General advice knows when player must end day"),
        General.Status == EAIHeroesZoneAdviceStatus::NeedsMovement &&
        General.MissingMoves > 0);
    S = NewExpedition();
    S.ProtocolTier = 3;
    const auto Planner = FAIHeroesZoneExpeditionAdvisor::Advise(
        S, FName(TEXT("FOREST")));
    TestTrue(TEXT("Protocol changes explanation, not route authority"),
        Planner.Message.Contains(TEXT("PLANNER")));
    TestEqual(TEXT("Protocol does not change actual route price"),
        Planner.TravelCost,
        FAIHeroesZoneExpeditionRules::Preview(
            S, FName(TEXT("FOREST"))).TotalCost);
    S.MapRevision ^= 1u;
    const auto Invalid = FAIHeroesZoneExpeditionAdvisor::Advise(S);
    TestTrue(TEXT("Tampered map fails closed"),
        Invalid.Status == EAIHeroesZoneAdviceStatus::InvalidState &&
        !Invalid.HasSuggestion() && Invalid.SourceRevision == 0);
    TestFalse(TEXT("Invalid map cannot leak topology"),
        Invalid.Message.Contains(TEXT("RUINS")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FZoneExpeditionAdvisorSeedSweepTest,
    "AIHeroes.ZoneExpeditionAdvisor.SeedSweepAndVisibleAfterCombat",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FZoneExpeditionAdvisorSeedSweepTest::RunTest(const FString&)
{
    int32 Valid = 0;
    FString Error;
    for (uint32 Seed = 0; Seed < 128; ++Seed)
    {
        for (int32 Branches = 0; Branches <= 4; ++Branches)
        {
            auto S = NewExpedition(Seed, Branches);
            FAIHeroesAdventureGraph G;
            if (!FAIHeroesZoneExpeditionRules::PlayerView(S, G, Error))
                continue;
            const auto Advice = FAIHeroesZoneExpeditionAdvisor::Advise(S);
            if (Advice.HasSuggestion() &&
                IsKnown(G, Advice.SuggestedSite) &&
                FAIHeroesAdventurePath::Find(G, S.HeroSite,
                    Advice.SuggestedSite, S.MovesLeft).IsReachableThisTurn())
                ++Valid;
        }
    }
    TestEqual(TEXT("640 deterministic seeds never leak hidden target"),
        Valid, 640);

    auto S = NewExpedition();
    FAIHeroesZoneExpeditionResult Confirmed;
    if (!TestTrue(TEXT("Separate HUMAN-confirmed forest action runs"),
        FAIHeroesZoneExpeditionRules::ConfirmPlayerDestination(
            S, FName(TEXT("FOREST")), Confirmed))) return false;
    TestTrue(TEXT("Confirmed first battle allows checkpoint discovery"),
        FAIHeroesZoneExpeditionRules::IsDiscovered(S, FName(TEXT("PASS"))));
    const auto BeforeAdvice = S;
    const uint32 ClearedBeforeAdvice = S.ClearedMask;
    const int32 MovesBeforeAdvice = S.MovesLeft;
    const auto Advice = FAIHeroesZoneExpeditionAdvisor::Advise(
        S, FName(TEXT("PASS")));
    TestTrue(TEXT("Newly discovered checkpoint can be inspected"),
        Advice.Status != EAIHeroesZoneAdviceStatus::UnknownTarget);
    TestFalse(TEXT("Inspection does not conquer guarded checkpoint"),
        FAIHeroesZoneExpeditionRules::IsCleared(S, FName(TEXT("PASS"))));
    TestEqual(TEXT("Read-only advice cannot clear another site"),
        S.ClearedMask, ClearedBeforeAdvice);
    TestEqual(TEXT("Read-only advice cannot spend movement"),
        S.MovesLeft, MovesBeforeAdvice);
    // The discovered count depends on map topology, not on whether advice
    // executed a second battle. Compare the full reflected state instead.
    const UScriptStruct* StateType = FAIHeroesZoneExpeditionState::StaticStruct();
    TestTrue(TEXT("Advice leaves the whole post-combat state identical"),
        StateType->CompareScriptStruct(&S, &BeforeAdvice, 0));
    TestEqual(TEXT("Advice leaves discovered count at the confirmed action result"),
        FAIHeroesZoneExpeditionRules::DiscoveredCount(S), Confirmed.DiscoveredCount);
    auto NegativeControl = BeforeAdvice;
    --NegativeControl.MovesLeft;
    TestFalse(TEXT("Comparison detects an unauthorized movement debit"),
        StateType->CompareScriptStruct(&NegativeControl, &BeforeAdvice, 0));
    NegativeControl = BeforeAdvice;
    if (TestTrue(TEXT("Post-combat army available for HP negative control"),
        !NegativeControl.PlayerArmy.IsEmpty()))
    {
        --NegativeControl.PlayerArmy[0].TotalHealth;
        TestFalse(TEXT("Comparison detects unauthorized battle damage"),
            StateType->CompareScriptStruct(&NegativeControl, &BeforeAdvice, 0));
    }
    return true;
}
#endif
