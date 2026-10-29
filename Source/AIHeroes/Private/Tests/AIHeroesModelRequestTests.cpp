#include "AIHeroesModelRequest.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AIHeroesFogRules.h"
#include "Misc/AutomationTest.h"
#include <limits>

namespace AIHeroesModelRequestTest
{
const FString Request(TEXT("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
const FString Session(TEXT("bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"));
FAIHeroesProposalSnapshot Fixture()
{
    FAIHeroesProposalSnapshot S;
    S.Day = 1; S.StateRevision = 7;
    auto& A = S.Strategic;
    A.TravelRuleVersion = 2; A.bFogEnabled = true; A.ExploredSitesMask = 7;
    A.ProtocolTier = 1; A.MovesLeft = 4; A.Gold = 240; A.Wood = 35; A.Ore = 30;
    A.HeroPosition = FVector(-650,0,155); A.SelectedNodeId = TEXT("FOREST");
    A.LastBattle = TEXT("PRIVATE_HISTORY_NOT_FOR_EXPORT");
    A.PlayerArmy = {FAIHeroesArmyStack(TEXT("Militia"),8,12,4,2,3),
        FAIHeroesArmyStack(TEXT("Scout"),3,8,6,1,7)};
    const TCHAR* Names[] = {TEXT("HQ"),TEXT("FOREST"),TEXT("ORE"),TEXT("OUTPOST"),TEXT("RUINS")};
    const EAIHeroesNodeType Types[] = {EAIHeroesNodeType::Headquarters,EAIHeroesNodeType::ForestMine,
        EAIHeroesNodeType::OreMine,EAIHeroesNodeType::Outpost,EAIHeroesNodeType::Ruins};
    const FVector At[] = {FVector(-650,0,155),FVector(-100,-650,110),FVector(150,600,120),
        FVector(700,-300,140),FVector(750,650,165)};
    for (int32 I=0; I<5; ++I)
    {
        FAIHeroesAdvisorNode N;
        N.Id=Names[I]; N.Type=Types[I]; N.Location=At[I]; N.bOwned=I==0;
        if (I) N.Guards.Add(FAIHeroesArmyStack(I>2 ? TEXT("SecretGuard") : TEXT("Wolf"),4,9,3,1,8));
        A.Nodes.Add(N);
    }
    return S;
}
bool Export(const FAIHeroesProposalSnapshot& S, FString& J)
{
    FString Error;
    return FAIHeroesModelRequest::Build(S,Request,Session,EAIHeroesModelGoal::AssessSelected,J,Error);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FModelRequestVisibilityTest,"AIHeroes.ModelRequest.VisibleAllowlist",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FModelRequestVisibilityTest::RunTest(const FString& Parameters)
{
    using namespace AIHeroesModelRequestTest;
    const auto S=Fixture(); FString J;
    TestTrue(TEXT("Engine snapshot exported"),Export(S,J));
    TestTrue(TEXT("Version pinned"),J.Contains(TEXT("\"schema\":\"aiheroes.hero-request.v1\"")));
    TestTrue(TEXT("Visible target and guard included"),J.Contains(TEXT("FOREST")) && J.Contains(TEXT("Wolf")));
    for (const TCHAR* Forbidden : {TEXT("OUTPOST"),TEXT("RUINS"),TEXT("SecretGuard"),
        TEXT("PRIVATE_HISTORY_NOT_FOR_EXPORT"),TEXT("HeroPosition"),TEXT("Location"),TEXT("bLocalModelReady")})
        TestFalse(TEXT("Hidden metadata and internal fields never exported"),J.Contains(Forbidden));
    TestTrue(TEXT("Bounded ASCII wire request"),J.Len()>0 && J.Len()<=FAIHeroesModelRequest::MaxBytes);
    for (TCHAR C:J) if (C>127) { AddError(TEXT("Non-ASCII output")); break; }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FModelRequestLegalMenuTest,"AIHeroes.ModelRequest.RoadGuardAndBudgetParity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FModelRequestLegalMenuTest::RunTest(const FString& Parameters)
{
    using namespace AIHeroesModelRequestTest;
    auto S=Fixture(); S.Strategic.ExploredSitesMask=31; FString J;
    TestTrue(TEXT("Known map exported"),Export(S,J));
    TestTrue(TEXT("Forest quoted by real solver"),J.Contains(TEXT("\"order\":\"AttackNode\",\"target_node\":\"FOREST\",\"cost\":2")));
    TestFalse(TEXT("No free transit through forest guards"),J.Contains(TEXT("\"target_node\":\"OUTPOST\"")));
    TestFalse(TEXT("No no-op suggestion at own origin"),J.Contains(TEXT("\"target_node\":\"HQ\"")));
    S.Strategic.Nodes[1].Guards.Empty(); S.Strategic.Nodes[1].bOwned=true;
    TestTrue(TEXT("Open route exported"),Export(S,J));
    TestTrue(TEXT("Opened outpost is reachable for three"),J.Contains(TEXT("\"target_node\":\"OUTPOST\",\"cost\":3")));
    S.Strategic.MovesLeft=2;
    TestTrue(TEXT("Low budget is still a valid state"),Export(S,J));
    TestFalse(TEXT("Unaffordable target not in legal menu"),J.Contains(TEXT("\"target_node\":\"OUTPOST\"")));
    S.Strategic.Nodes.RemoveAt(4);
    J=TEXT("old payload");
    TestFalse(TEXT("Broken V2 topology fails closed"),Export(S,J));
    TestTrue(TEXT("Failure removes stale output"),J.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FModelRequestPhaseTest,"AIHeroes.ModelRequest.SeedBattleAndArmyLocks",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FModelRequestPhaseTest::RunTest(const FString& Parameters)
{
    using namespace AIHeroesModelRequestTest;
    auto S=Fixture(); FString J;
    S.Strategic.ProtocolTier=0;
    TestTrue(TEXT("SEED status may be represented"),Export(S,J));
    TestTrue(TEXT("SEED has no formal orders"),J.Contains(TEXT("\"legal_orders\":[]")));
    S.Strategic.ProtocolTier=1; S.bBattleActive=true;
    TestTrue(TEXT("Battle phase exported explicitly"),Export(S,J));
    TestTrue(TEXT("No strategy orders during battle"),J.Contains(TEXT("\"battle_active\":true")) && J.Contains(TEXT("\"legal_orders\":[]")));
    S.bBattleActive=false; S.Strategic.PlayerArmy.Empty();
    TestTrue(TEXT("Unarmed state can be inspected"),Export(S,J));
    TestTrue(TEXT("No attacks without a living army"),J.Contains(TEXT("\"legal_orders\":[]")));
    S=Fixture(); S.Strategic.MovesLeft=0;
    TestTrue(TEXT("No movement is still a valid state"),Export(S,J));
    TestTrue(TEXT("No affordable orders"),J.Contains(TEXT("\"legal_orders\":[]")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FModelRequestBoundsTest,"AIHeroes.ModelRequest.MalformedInputsClearOutput",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FModelRequestBoundsTest::RunTest(const FString& Parameters)
{
    using namespace AIHeroesModelRequestTest;
    FString J,Error;
    for (int32 Case=0; Case<12; ++Case)
    {
        auto S=Fixture(); FString R=Request, T=Session;
        auto Goal=EAIHeroesModelGoal::AssessSelected;
        switch(Case)
        {
            case 0: R=TEXT("bad-token"); break;
            case 1: T=FString::ChrN(32,'G'); break;
            case 2: S.Day=1000001; break;
            case 3: S.StateRevision=0; break;
            case 4: S.Strategic.PlayerArmy[0].UnitId=TEXT("inject\"command"); break;
            case 5: S.Strategic.PlayerArmy[0].UnitHealth=0; break;
            case 6: S.Strategic.SelectedNodeId=TEXT("RUINS"); break;
            case 7: S.Strategic.Nodes[1].bOwned=true; break;
            case 8: S.Strategic.HeroPosition.X=std::numeric_limits<double>::quiet_NaN(); break;
            case 9: { const auto N=S.Strategic.Nodes[0]; S.Strategic.Nodes.Add(N); break; }
            case 10: Goal=static_cast<EAIHeroesModelGoal>(255); break;
            case 11: S.Strategic.Gold=-1; break;
        }
        J=TEXT("old payload"); Error.Empty();
        TestFalse(FString::Printf(TEXT("Invalid case %d rejected"),Case),
            FAIHeroesModelRequest::Build(S,R,T,Goal,J,Error));
        TestTrue(TEXT("No stale output and bounded diagnostic"),J.IsEmpty() && !Error.IsEmpty());
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FModelRequestStabilityTest,"AIHeroes.ModelRequest.StableOrderAndReadOnly",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FModelRequestStabilityTest::RunTest(const FString& Parameters)
{
    using namespace AIHeroesModelRequestTest;
    auto S=Fixture(); FString Before,After;
    TestTrue(TEXT("Initial export"),Export(S,Before));
    S.Strategic.Nodes.Swap(0,4); S.Strategic.Nodes.Swap(1,2);
    TestTrue(TEXT("Reordered engine list valid"),Export(S,After));
    TestEqual(TEXT("Wire menu stable across list order"),After,Before);
    for(int32 I=0; I<20; ++I) TestTrue(TEXT("Repeated read works"),Export(S,After));
    TestEqual(TEXT("No spending"),S.Strategic.Gold,240);
    TestEqual(TEXT("No movement"),S.Strategic.MovesLeft,4);
    TestEqual(TEXT("No health mutation"),S.Strategic.PlayerArmy[0].TotalHealth,96);
    TestFalse(TEXT("No fake model readiness"),S.bLocalModelReady);
    S.Strategic.Gold=239;
    TestTrue(TEXT("Updated state exported"),Export(S,After));
    TestNotEqual(TEXT("Body detects change even if revision unchanged"),After,Before);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FModelRequestLegacyTest,"AIHeroes.ModelRequest.LegacyGoalsAndSlots",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FModelRequestLegacyTest::RunTest(const FString& Parameters)
{
    using namespace AIHeroesModelRequestTest;
    auto S=Fixture(); S.Strategic.TravelRuleVersion=1;
    S.Strategic.bFogEnabled=false; S.Strategic.ExploredSitesMask=0;
    S.Strategic.SelectedNodeId=NAME_None;
    for(int32 Slot=0; Slot<3; ++Slot)
    for(auto Goal:{EAIHeroesModelGoal::AssessSelected,EAIHeroesModelGoal::Explore,EAIHeroesModelGoal::PreserveArmy})
    {
        S.ModelSlotTier=Slot; FString J,E;
        TestTrue(TEXT("Legacy goal/slot exports without migration"),FAIHeroesModelRequest::Build(S,Request,Session,Goal,J,E));
        TestTrue(TEXT("No selection is explicit null"),J.Contains(TEXT("\"selected_node\":null")));
        TestTrue(TEXT("Legacy map fully known"),J.Contains(TEXT("RUINS")));
        TestFalse(TEXT("Export never claims verified checkpoint"),J.Contains(TEXT("weights_verified")));
    }
    return true;
}
#endif
