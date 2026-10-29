#include "AIHeroesModelReply.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AIHeroesGameMode.h"
#include "AIHeroesWorldNode.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include <limits>

namespace AIHeroesModelReplyTest
{
FAIHeroesProposalSnapshot Fixture(bool Ready = true)
{
    FAIHeroesProposalSnapshot S;
    S.Day = 1; S.StateRevision = 7; S.bLocalModelReady = Ready; // TEST FIXTURE ONLY.
    auto& A = S.Strategic;
    A.TravelRuleVersion = 2; A.bFogEnabled = true; A.ExploredSitesMask = 7;
    A.ProtocolTier = 1; A.MovesLeft = 4; A.Gold = 240; A.Wood = 35; A.Ore = 30;
    A.HeroPosition = FVector(-650,0,155); A.SelectedNodeId = TEXT("FOREST");
    A.PlayerArmy = {FAIHeroesArmyStack(TEXT("Militia"),8,12,4,2,3)};
    const TCHAR* Ids[] = {TEXT("HQ"),TEXT("FOREST"),TEXT("ORE"),TEXT("OUTPOST"),TEXT("RUINS")};
    const EAIHeroesNodeType Types[] = {EAIHeroesNodeType::Headquarters,EAIHeroesNodeType::ForestMine,
        EAIHeroesNodeType::OreMine,EAIHeroesNodeType::Outpost,EAIHeroesNodeType::Ruins};
    const FVector Positions[] = {FVector(-650,0,155),FVector(-100,-650,110),FVector(150,600,120),
        FVector(700,-300,140),FVector(750,650,165)};
    for (int32 I=0; I<5; ++I)
    {
        FAIHeroesAdvisorNode N;
        N.Id=Ids[I]; N.Type=Types[I]; N.Location=Positions[I]; N.bOwned=I==0;
        if (I) N.Guards.Add(FAIHeroesArmyStack(TEXT("Wolf"),4,9,3,1,8));
        A.Nodes.Add(N);
    }
    return S;
}
FString Reply(const FString& Id, const TCHAR* Choice = TEXT("c2"))
{
    return FString::Printf(TEXT("{\"request_id\":\"%s\",\"candidate_id\":\"%s\"}"), *Id, Choice);
}
bool Begin(FAIHeroesModelReplySession& R, const FAIHeroesProposalSnapshot& S)
{
    FString J,E; return R.Begin(S,TEXT("engine-context"),10.0,5.0,J,E);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FModelReplyMenuTest,"AIHeroes.ModelReply.MenuParityAndReadOnly",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FModelReplyMenuTest::RunTest(const FString& Parameters)
{
    using namespace AIHeroesModelReplyTest;
    auto S=Fixture(); FString J,E;
    TArray<FAIHeroesAIProposal> Menu;
    TestTrue(TEXT("One engine pass produces both JSON and menu"),FAIHeroesModelRequest::Build(S,
        FString::ChrN(32,'a'),FString::ChrN(32,'b'),EAIHeroesModelGoal::AssessSelected,J,E,&Menu));
    TestEqual(TEXT("Python-compatible c0,c1,FOREST,ORE order"),Menu.Num(),4);
    if(Menu.Num()!=4) return false;
    TestEqual(TEXT("c0 is no action"),Menu[0].Kind,EAIHeroesProposalKind::NoAction);
    TestEqual(TEXT("c1 is advice"),Menu[1].Kind,EAIHeroesProposalKind::Advice);
    TestEqual(TEXT("c2 is forest"),Menu[2].TargetNode,FName(TEXT("FOREST")));
    TestTrue(TEXT("Cost text is engine-owned"),Menu[2].Explanation.Contains(TEXT("cost 2 moves")));
    for (const TCHAR* Choice:{TEXT("c0"),TEXT("c1"),TEXT("c2"),TEXT("c3")})
    {
        FAIHeroesModelReplySession R;
        TestTrue(TEXT("Begin fixture request"),Begin(R,S));
        const auto V=R.Complete(S,TEXT("engine-context"),Reply(R.GetRequestId(),Choice),11.0);
        TestTrue(TEXT("Fixture readiness allows read-only candidate"),V.bChoiceValid && V.bConsumed && V.Verdict.bAccepted);
        TestFalse(TEXT("Request consumed"),R.IsPending());
        TestEqual(TEXT("Source remains LocalModel, not impersonated offline policy"),V.Proposal.Source,EAIHeroesProposalSource::LocalModel);
        TestEqual(TEXT("Only orders require player confirmation"),V.Verdict.bPlayerConfirmationRequired,V.Proposal.Kind==EAIHeroesProposalKind::ProposeOrder);
    }
    TestEqual(TEXT("No resources spent"),S.Strategic.Gold,240);
    TestEqual(TEXT("No movement spent"),S.Strategic.MovesLeft,4);
    TestFalse(TEXT("No target captured"),S.Strategic.Nodes[1].bOwned);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FModelReplyAuthorityTest,"AIHeroes.ModelReply.ReadinessCannotBeForged",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FModelReplyAuthorityTest::RunTest(const FString& Parameters)
{
    using namespace AIHeroesModelReplyTest;
    auto S=Fixture(false); S.bRemoteModelReady=true; S.bRemoteInferenceApproved=true;
    FAIHeroesModelReplySession R; Begin(R,S);
    FString Fake=Reply(R.GetRequestId()); Fake.RemoveAt(Fake.Len()-1);
    Fake+=TEXT(",\"bLocalModelReady\":true}");
    TestFalse(TEXT("Payload cannot enable readiness"),R.Complete(S,TEXT("engine-context"),Fake,11.0).Verdict.bAccepted);
    TestTrue(TEXT("Malformed packet does not cancel unrelated in-flight request"),R.IsPending());
    const auto V=R.Complete(S,TEXT("engine-context"),Reply(R.GetRequestId()),11.0);
    TestTrue(TEXT("Choice syntax valid"),V.bChoiceValid);
    TestFalse(TEXT("No verified local checkpoint means blocked"),V.Verdict.bAccepted);
    TestTrue(TEXT("No rejected order escapes"),V.Proposal.TargetNode.IsNone());
    TestFalse(TEXT("No fabricated readiness"),S.bLocalModelReady);
    TestFalse(TEXT("Matching blocked reply is consumed"),R.IsPending());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FModelReplyDriftTest,"AIHeroes.ModelReply.FullStateAndHostDrift",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FModelReplyDriftTest::RunTest(const FString& Parameters)
{
    using namespace AIHeroesModelReplyTest;
    for(int32 Case=0;Case<10;++Case)
    {
        auto S=Fixture(); FAIHeroesModelReplySession R; Begin(R,S);
        const FString Response=Reply(R.GetRequestId()); FString Context=TEXT("engine-context");
        switch(Case)
        {
            case 0: --S.Strategic.Gold; break; // Revision deliberately unchanged.
            case 1: --S.Strategic.MovesLeft; break;
            case 2: ++S.Day; break;
            case 3: S.Strategic.SelectedNodeId=TEXT("HQ"); break;
            case 4: --S.Strategic.Nodes[1].Guards[0].TotalHealth; break;
            case 5: S.Strategic.ProtocolTier=0; break;
            case 6: ++S.ModelSlotTier; break;
            case 7: S.bBattleActive=true; break;
            case 8: Context=TEXT("engine-context-base-or-stock-changed"); break;
            case 9: S.Strategic.ExploredSitesMask=31; break;
        }
        const auto V=R.Complete(S,Context,Response,11.0);
        TestFalse(FString::Printf(TEXT("Drift %d rejected"),Case),V.Verdict.bAccepted);
        TestEqual(TEXT("Drift reason"),V.Reason,FString(TEXT("STALE_CONTEXT")));
        TestFalse(TEXT("Stale matching request consumed"),R.IsPending());
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FModelReplyCorrelationTest,"AIHeroes.ModelReply.SingleUseCancelAndSessionReset",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FModelReplyCorrelationTest::RunTest(const FString& Parameters)
{
    using namespace AIHeroesModelReplyTest;
    const auto S=Fixture(); FAIHeroesModelReplySession R;
    Begin(R,S); const FString Old=Reply(R.GetRequestId());
    Begin(R,S); const FString Fresh=Reply(R.GetRequestId());
    TestNotEqual(TEXT("New request has a new token"),Old,Fresh);
    TestFalse(TEXT("Old reply rejected"),R.Complete(S,TEXT("engine-context"),Old,11.0).Verdict.bAccepted);
    TestTrue(TEXT("Old reply cannot cancel newer pending request"),R.IsPending());
    TestTrue(TEXT("Current reply accepted as suggestion"),R.Complete(S,TEXT("engine-context"),Fresh,11.0).Verdict.bAccepted);
    TestFalse(TEXT("Duplicate cannot be reused"),R.Complete(S,TEXT("engine-context"),Fresh,11.0).Verdict.bAccepted);
    Begin(R,S); const FString Session=R.GetSessionId(), BeforeReset=Reply(R.GetRequestId());
    R.Reset();
    TestNotEqual(TEXT("Load/new-session token rotates"),R.GetSessionId(),Session);
    TestFalse(TEXT("Reset rejects previous response"),R.Complete(S,TEXT("engine-context"),BeforeReset,11.0).Verdict.bAccepted);
    Begin(R,S); R.Cancel(); TestFalse(TEXT("Explicit cancel clears pending"),R.IsPending());
    Begin(R,S); FString J=TEXT("stale"),E;
    TestFalse(TEXT("Invalid replacement rejected"),R.Begin(S,TEXT(""),11.0,5.0,J,E));
    TestTrue(TEXT("Invalid replacement clears old request/output"),!R.IsPending() && J.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FModelReplyClockTest,"AIHeroes.ModelReply.DeadlineAndClockBounds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FModelReplyClockTest::RunTest(const FString& Parameters)
{
    using namespace AIHeroesModelReplyTest;
    const auto S=Fixture(); FAIHeroesModelReplySession R; FString J,E;
    for(double Time:{-1.0,9.0,15.0,1000.0,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()})
    {
        Begin(R,S); const auto V=R.Complete(S,TEXT("engine-context"),Reply(R.GetRequestId()),Time);
        TestFalse(TEXT("Expired/nonmonotonic/invalid clock rejected"),V.Verdict.bAccepted);
        TestFalse(TEXT("Expired request cleared"),R.IsPending());
    }
    for(double Lifetime:{0.0,-1.0,30.1,std::numeric_limits<double>::quiet_NaN()})
        TestFalse(TEXT("Lifetime bounds"),R.Begin(S,TEXT("ctx"),10.0,Lifetime,J,E));
    TestFalse(TEXT("Clock overflow/precision loss rejected"),R.Begin(S,TEXT("ctx"),1.e300,5.0,J,E));
    Begin(R,S);
    TestTrue(TEXT("Just before deadline remains eligible"),R.Complete(S,TEXT("engine-context"),Reply(R.GetRequestId()),14.999).Verdict.bAccepted);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FModelReplySyntaxTest,"AIHeroes.ModelReply.StrictSelectionGrammar",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FModelReplySyntaxTest::RunTest(const FString& Parameters)
{
    using namespace AIHeroesModelReplyTest;
    const FString Id=FString::ChrN(32,'a'); FString Out; int32 Index;
    TestTrue(TEXT("Normal response parsed"),FAIHeroesModelReplySession::DecodeSelection(Reply(Id),Out,Index));
    TestEqual(TEXT("c2 mapped to index"),Index,2);
    TestTrue(TEXT("Whitespace/reversed keys accepted"),FAIHeroesModelReplySession::DecodeSelection(
        TEXT(" { \"candidate_id\" : \"c0\", \"request_id\" : \"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\" } \n"),Out,Index));
    const TArray<FString> Bad={TEXT(""),TEXT("{}"),TEXT("[]"),Reply(Id)+TEXT(" trailing"),
        TEXT("{\"request_id\":\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\",\"request_id\":\"c2\"}"),
        TEXT("{\"request_id\":\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\",\"candidate_id\":2}"),
        TEXT("{\"request_id\":\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\",\"candidate_id\":{}}"),
        TEXT("{\"request_id\":\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\",\"candidate_id\":\"c2\",\"extra\":\"yes\"}"),
        Reply(Id,TEXT("c02")),Reply(Id,TEXT("c66")),Reply(Id,TEXT("c-1")),Reply(Id,TEXT("c+2")),
        Reply(Id,TEXT("c2\\n")),Reply(FString::ChrN(32,'A')),FString::ChrN(257,' ')};
    for(const auto& Raw:Bad)
    {
        Out=TEXT("old");Index=7;
        TestFalse(TEXT("Malformed/extended grammar rejected"),FAIHeroesModelReplySession::DecodeSelection(Raw,Out,Index));
        TestTrue(TEXT("Failed decoding clears outputs"),Out.IsEmpty() && Index==INDEX_NONE);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FModelReplySweepTest,"AIHeroes.ModelReply.ForeignAndUnknownChoiceSweep",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FModelReplySweepTest::RunTest(const FString& Parameters)
{
    using namespace AIHeroesModelReplyTest;
    const auto S=Fixture(); FAIHeroesModelReplySession R; Begin(R,S);
    int32 Rejected=0;
    for(int32 I=0;I<512;++I)
    {
        const FString Foreign=FString::Printf(TEXT("%032x"),I);
        if(!R.Complete(S,TEXT("engine-context"),Reply(Foreign),11.0).Verdict.bAccepted) ++Rejected;
    }
    TestEqual(TEXT("512 foreign replies rejected"),Rejected,512);
    TestTrue(TEXT("Foreign packets cannot consume current request"),R.IsPending());
    const auto Bad=R.Complete(S,TEXT("engine-context"),Reply(R.GetRequestId(),TEXT("c65")),11.0);
    TestEqual(TEXT("Well-formed unknown menu index rejected"),Bad.Reason,FString(TEXT("UNKNOWN_CANDIDATE")));
    TestFalse(TEXT("Unknown matching choice consumes request"),R.IsPending());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FModelReplyMenuFailureTest,"AIHeroes.ModelReply.MenuFailureClearsOutputs",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FModelReplyMenuFailureTest::RunTest(const FString& Parameters)
{
    using namespace AIHeroesModelReplyTest;
    auto S=Fixture(); S.StateRevision=0;
    FString J=TEXT("old"),E; TArray<FAIHeroesAIProposal> Menu; Menu.AddDefaulted();
    TestFalse(TEXT("Invalid snapshot rejected"),FAIHeroesModelRequest::Build(S,
        FString::ChrN(32,'a'),FString::ChrN(32,'b'),EAIHeroesModelGoal::AssessSelected,J,E,&Menu));
    TestTrue(TEXT("Both outputs clear atomically"),J.IsEmpty() && Menu.IsEmpty());
    S=Fixture(); FAIHeroesModelReplySession R;
    S.Strategic.ProtocolTier=0;TestFalse(TEXT("SEED cannot begin inference request"),Begin(R,S));
    S.Strategic.ProtocolTier=1;S.bBattleActive=true;TestFalse(TEXT("Active battle cannot begin"),Begin(R,S));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FModelReplyGameModeTest,"AIHeroes.ModelReply.RealGameModeRemainsOffline",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FModelReplyGameModeTest::RunTest(const FString& Parameters)
{
    using namespace AIHeroesModelReplyTest;
    const auto Values=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false)
        .CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
    UWorld* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Values);
    if(!TestNotNull(TEXT("Disposable world exists"),World)) return false;
    if(GEngine) GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    auto* Game=World->SpawnActor<AAIHeroesGameMode>();
    const auto S=Fixture(false);
    if(Game)
    {
        for(const auto& N:S.Strategic.Nodes)
        {
            auto* Node=World->SpawnActor<AAIHeroesWorldNode>(N.Location,FRotator::ZeroRotator);
            if(!Node) continue;
            Node->Configure(N.Type,N.Id.ToString(),N.Guards);Node->SetOwned(N.bOwned);
            Game->Nodes.Add(Node);
            if(N.Id==TEXT("FOREST")) Game->SelectedNode=Node;
        }
        Game->TravelRuleVersion=2;Game->ProtocolTier=1;Game->PlayerArmy=S.Strategic.PlayerArmy;
        Game->LastHint=TEXT("offline hint");Game->LastEvent=TEXT("existing event");
        FString J,E;
        if(TestTrue(TEXT("Live GameMode prepares read-only request"),Game->BeginLocalModelAdvice(J,E)))
        {
            const FString Prefix=TEXT("\"request_id\":\""); const int32 Start=J.Find(Prefix);
            TestTrue(TEXT("Engine correlation token exists"),Start!=INDEX_NONE);
            const FString Id=J.Mid(Start+Prefix.Len(),32);
            const auto V=Game->CompleteLocalModelAdvice(Reply(Id));
            TestTrue(TEXT("Valid choice reaches engine gate"),V.bChoiceValid);
            TestFalse(TEXT("Game never pretends local weights loaded"),V.Verdict.bAccepted);
            TestFalse(TEXT("Response cannot start combat"),Game->IsLiveBattleActive());
            TestEqual(TEXT("No spending"),Game->Gold,240);
            TestEqual(TEXT("No movement"),Game->MovesLeft,4);
            TestEqual(TEXT("Offline hint unchanged"),Game->LastHint,FString(TEXT("offline hint")));
            TestEqual(TEXT("Event unchanged"),Game->LastEvent,FString(TEXT("existing event")));
            Game->BeginLocalModelAdvice(J,E);Game->CancelLocalModelAdvice();
            TestFalse(TEXT("Game cancellation clears transient request"),Game->IsLocalModelAdvicePending());
        }
    }
    else AddError(TEXT("GameMode allocation failed"));
    World->DestroyWorld(false);if(GEngine) GEngine->DestroyWorldContext(World);
    return true;
}
#endif
