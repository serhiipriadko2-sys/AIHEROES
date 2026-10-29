#include "AIHeroesLocalExchange.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AIHeroesModelRequest.h"
#include "Misc/AutomationTest.h"
#include <limits>

namespace AIHeroesLocalExchangeTests
{
const FString Id(TEXT("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"));
const FString Session(TEXT("bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"));
FAIHeroesProposalSnapshot Fixture()
{
    FAIHeroesProposalSnapshot S; S.Day = 1; S.StateRevision = 7;
    auto& A = S.Strategic; A.TravelRuleVersion = 2; A.bFogEnabled = true;
    A.ExploredSitesMask = 7; A.ProtocolTier = 1; A.MovesLeft = 4;
    A.Gold = 240; A.Wood = 35; A.Ore = 30; A.HeroPosition = FVector(-650,0,155);
    A.SelectedNodeId = TEXT("FOREST");
    A.PlayerArmy = {FAIHeroesArmyStack(TEXT("Militia"),8,12,4,2,3)};
    const TCHAR* Names[] = {TEXT("HQ"),TEXT("FOREST"),TEXT("ORE"),TEXT("OUTPOST"),TEXT("RUINS")};
    const EAIHeroesNodeType Types[] = {EAIHeroesNodeType::Headquarters,EAIHeroesNodeType::ForestMine,
        EAIHeroesNodeType::OreMine,EAIHeroesNodeType::Outpost,EAIHeroesNodeType::Ruins};
    for (int32 I = 0; I < 5; ++I)
    {
        FAIHeroesAdvisorNode N; N.Id = Names[I]; N.Type = Types[I];
        N.Location = A.HeroPosition + FVector(I*200,0,0); N.bOwned = I == 0;
        if (I) N.Guards.Add(FAIHeroesArmyStack(TEXT("Wolf"),4,9,3,1,8));
        A.Nodes.Add(N);
    }
    return S;
}
FString Request()
{
    FString J,E; FAIHeroesModelRequest::Build(Fixture(),Id,Session,EAIHeroesModelGoal::AssessSelected,J,E); return J;
}
TArray<uint8> Bytes(const FString& S)
{
    TArray<uint8> B; for (TCHAR C : S) B.Add(static_cast<uint8>(C)); return B;
}
FString Selection() { return TEXT("{\"request_id\":\"") + Id + TEXT("\",\"candidate_id\":\"c2\"}"); }
FString Envelope()
{
    const FString Escaped = Selection().Replace(TEXT("\""), TEXT("\\\""));
    return TEXT("{\"model\":\"qa-valid\",\"choices\":[{\"index\":0,\"finish_reason\":\"stop\",\"message\":{\"role\":\"assistant\",\"content\":\"") + Escaped + TEXT("\"}}]}");
}
FString Frame(const FString& Body)
{
    return FString::Printf(TEXT("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: %d\r\n\r\n%s"), Body.Len(), *Body);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLocalEndpointTest,"AIHeroes.LocalExchange.EndpointAndOptIn",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLocalEndpointTest::RunTest(const FString& Parameters)
{
    using namespace AIHeroesLocalExchangeTests;
    TestTrue(TEXT("Explicit numeric port and alias"), FAIHeroesLocalExchangeProtocol::ValidEndpoint(8080,TEXT("qwen-3b:Q4_K_M")));
    for (int32 Port : {-1,0,80,1023,65536}) TestFalse(TEXT("Invalid/restricted port"),FAIHeroesLocalExchangeProtocol::ValidEndpoint(Port,TEXT("model")));
    for (const TCHAR* Alias : {TEXT(""),TEXT("http://remote"),TEXT("x/y"),TEXT("x\r\nHost: evil"),TEXT("a b")})
        TestFalse(TEXT("No URL/path/header injection"),FAIHeroesLocalExchangeProtocol::ValidEndpoint(8080,Alias));
    FAIHeroesLocalExchange T; FString E;
    TestFalse(TEXT("No opt-in never opens socket"),T.Start(8080,TEXT("qa-valid"),Request(),false,1.0,4.0,E));
    TestFalse(TEXT("No pending network request"),T.IsPending());
    TestEqual(TEXT("No polling occurred"),T.GetPollCount(),0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLocalRequestTest,"AIHeroes.LocalExchange.RequestMenuAndFogParity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLocalRequestTest::RunTest(const FString& Parameters)
{
    using namespace AIHeroesLocalExchangeTests;
    FString Body,R,E; int32 Count = 0;
    TestTrue(TEXT("Real exporter output accepted"),FAIHeroesLocalExchangeProtocol::BuildRequest(Request(),TEXT("qa-valid"),Body,R,Count,E));
    TestEqual(TEXT("Correlation preserved"),R,Id); TestEqual(TEXT("Two defaults plus two legal attacks"),Count,4);
    TestTrue(TEXT("Bounded chat request has schema and nonstreaming option"),Body.Contains(TEXT("json_schema")) && Body.Contains(TEXT("max_tokens")) && Body.Contains(TEXT("candidate_id")));
    TestFalse(TEXT("Undiscovered outpost not disclosed"),Body.Contains(TEXT("OUTPOST")));
    TestFalse(TEXT("Undiscovered ruins not disclosed"),Body.Contains(TEXT("RUINS")));
    TestFalse(TEXT("No tools schema"),Body.Contains(TEXT("tool_calls")));
    auto Seed = Fixture(); Seed.Strategic.ProtocolTier = 0;
    FString Locked; FAIHeroesModelRequest::Build(Seed,Id,Session,EAIHeroesModelGoal::AssessSelected,Locked,E);
    TestFalse(TEXT("SEED cannot start inference"),FAIHeroesLocalExchangeProtocol::BuildRequest(Locked,TEXT("qa-valid"),Body,R,Count,E));
    TestTrue(TEXT("Rejected request clears every output"),Body.IsEmpty() && R.IsEmpty() && Count == 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLocalFrameTest,"AIHeroes.LocalExchange.FragmentedHTTPFrame",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLocalFrameTest::RunTest(const FString& Parameters)
{
    using namespace AIHeroesLocalExchangeTests;
    const FString Original = Envelope(), Wire = Frame(Original); FString Out,E;
    for (int32 N = 0; N < Wire.Len(); ++N)
    {
        const auto State = FAIHeroesLocalExchangeProtocol::ReadFrame(Bytes(Wire.Left(N)),Out,E);
        if (State != EAIHeroesLocalFrame::Incomplete) { AddError(FString::Printf(TEXT("Prefix %d was not incomplete"),N)); break; }
        if (!Out.IsEmpty()) { AddError(TEXT("Incomplete frame exposed payload")); break; }
    }
    TestTrue(TEXT("Only complete length accepted"),FAIHeroesLocalExchangeProtocol::ReadFrame(Bytes(Wire),Out,E) == EAIHeroesLocalFrame::Complete);
    TestEqual(TEXT("Complete body preserved"),Out,Original);
    TestTrue(TEXT("Extra bytes refused"),FAIHeroesLocalExchangeProtocol::ReadFrame(Bytes(Wire+TEXT("x")),Out,E) == EAIHeroesLocalFrame::Rejected);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLocalHeaderTest,"AIHeroes.LocalExchange.RejectsAmbiguousHTTP",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLocalHeaderTest::RunTest(const FString& Parameters)
{
    using namespace AIHeroesLocalExchangeTests;
    const FString Good = Frame(Envelope());
    const TArray<FString> Bad = {
        Good.Replace(TEXT("200 OK"),TEXT("302 Found")),
        Good.Replace(TEXT("200 OK"),TEXT("500 Error")),
        Good.Replace(TEXT("Content-Length:"),TEXT("Transfer-Encoding: chunked\r\nContent-Length:")),
        Good.Replace(TEXT("Content-Length:"),TEXT("Content-Length: 1\r\nContent-Length:")),
        Good.Replace(TEXT("Content-Length:"),TEXT("Content-Encoding: gzip\r\nContent-Length:")),
        Good.Replace(TEXT("Content-Length:"),TEXT("Content-Length :")),
        Good.Replace(TEXT("application/json"),TEXT("text/html")),
        TEXT("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{}"),
        TEXT("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: 999999\r\n\r\n"),
        TEXT("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: -1\r\n\r\n"),
        FString::ChrN(FAIHeroesLocalExchangeProtocol::MaxHeaderBytes,'X')};
    for (const auto& Input : Bad)
    {
        FString Out=TEXT("old"),E;
        TestTrue(TEXT("Bad framing rejected"),FAIHeroesLocalExchangeProtocol::ReadFrame(Bytes(Input),Out,E) == EAIHeroesLocalFrame::Rejected);
        TestTrue(TEXT("No stale response output"),Out.IsEmpty() && !E.IsEmpty());
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLocalEnvelopeTest,"AIHeroes.LocalExchange.EnvelopeIdentityAndFinish",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLocalEnvelopeTest::RunTest(const FString& Parameters)
{
    using namespace AIHeroesLocalExchangeTests;
    FString Out,E; const FString Good=Envelope();
    TestTrue(TEXT("Synthetic envelope decoded"),FAIHeroesLocalExchangeProtocol::DecodeEnvelope(Good,TEXT("qa-valid"),Id,4,Out,E));
    TestEqual(TEXT("Only bounded inner choice returned"),Out,Selection());
    for (const auto& Bad : {
        Good.Replace(TEXT("qa-valid"),TEXT("other-model")),Good.Replace(TEXT("qa-valid"),TEXT("QA-VALID")),
        Good.Replace(TEXT("stop"),TEXT("length")),
        Good.Replace(TEXT("assistant"),TEXT("user")),Good.Replace(TEXT("\"index\":0"),TEXT("\"index\":1")),
        Good.Replace(TEXT("c2"),TEXT("c9")),Good.Replace(*Id,*Session),
        Good.Replace(TEXT("\"role\":"),TEXT("\"tool_calls\":[],\"role\":")),
        Good.Replace(TEXT("\"role\":"),TEXT("\"function_call\":{},\"role\":")),
        Good.Replace(TEXT("\"role\":"),TEXT("\"refusal\":\"no\",\"role\":"))})
    {
        Out=TEXT("old");
        TestFalse(TEXT("Invalid envelope cannot enter reply gate"),FAIHeroesLocalExchangeProtocol::DecodeEnvelope(Bad,TEXT("qa-valid"),Id,4,Out,E));
        TestTrue(TEXT("Rejected envelope clears selection"),Out.IsEmpty());
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLocalJSONTest,"AIHeroes.LocalExchange.DuplicatesAndJSONBounds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLocalJSONTest::RunTest(const FString& Parameters)
{
    using namespace AIHeroesLocalExchangeTests;
    const FString Good=Envelope(); FString Out,E;
    const TArray<FString> Bad = {Good+Good, Good.LeftChop(1),
        Good.Replace(TEXT("\"model\":"),TEXT("\"model\":\"qa-valid\",\"model\":")),
        Good.Replace(TEXT("\"index\":"),TEXT("\"in\\u0064ex\":0,\"index\":")),
        Good.Replace(TEXT("\"role\":"),TEXT("\"role\":\"assistant\",\"role\":")),
        TEXT("[]"), FString::ChrN(32769,' '),
        TEXT("{\"deep\":")+FString::ChrN(15,'[')+TEXT("0")+FString::ChrN(15,']')+TEXT("}")};
    for (const auto& B : Bad) TestFalse(TEXT("Malformed/deep/ambiguous JSON rejected"),FAIHeroesLocalExchangeProtocol::DecodeEnvelope(B,TEXT("qa-valid"),Id,4,Out,E));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLocalClockTest,"AIHeroes.LocalExchange.CancelAndClockPreflight",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLocalClockTest::RunTest(const FString& Parameters)
{
    using namespace AIHeroesLocalExchangeTests;
    FAIHeroesLocalExchange T; FString E;
    for (double Now : {-1.0, std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()})
        TestFalse(TEXT("Bad clock blocks before network"),T.Start(8080,TEXT("qa-valid"),Request(),true,Now,4.0,E));
    for (double Limit : {0.0,-1.0,4.01,std::numeric_limits<double>::infinity()})
        TestFalse(TEXT("Bad duration blocks before network"),T.Start(8080,TEXT("qa-valid"),Request(),true,1.0,Limit,E));
    T.Cancel(); T.Cancel(); T.Tick(2.0);
    TestFalse(TEXT("Cancellation is idempotent and not pending"),T.IsPending());
    TestTrue(TEXT("Cancelled state exposes no selection"),T.GetSelection().IsEmpty());
    TestEqual(TEXT("Preflight checks did not pump sockets"),T.GetPollCount(),0);
    return true;
}
#endif
