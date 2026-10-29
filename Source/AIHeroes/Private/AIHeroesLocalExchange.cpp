#include "AIHeroesLocalExchange.h"

#include "IPAddress.h"
#include "SocketSubsystem.h"
#include "Sockets.h"

namespace AIHeroesLocalSocketDetail
{
bool Clock(double T) { return FMath::IsFinite(T) && T >= 0.0; }
bool PendingSocketError(ESocketErrors E)
{
    return E == SE_EWOULDBLOCK || E == SE_EINPROGRESS || E == SE_EALREADY;
}
}

FAIHeroesLocalExchange::~FAIHeroesLocalExchange() { CloseSocket(); }
void FAIHeroesLocalExchange::CloseSocket()
{
    if (Socket)
    {
        Socket->Close();
        if (ISocketSubsystem* S = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)) S->DestroySocket(Socket);
        Socket = nullptr;
    }
}
bool FAIHeroesLocalExchange::IsPending() const
{
    return State == EAIHeroesLocalExchangeState::Connecting ||
        State == EAIHeroesLocalExchangeState::Sending || State == EAIHeroesLocalExchangeState::Receiving;
}
void FAIHeroesLocalExchange::Cancel()
{
    CloseSocket(); SendBytes.Reset(); ReceiveBytes.Reset(); Selection.Empty();
    Alias.Empty(); RequestId.Empty(); Sent = Choices = 0;
    State = EAIHeroesLocalExchangeState::Cancelled; Reason = TEXT("CANCELLED");
}
void FAIHeroesLocalExchange::Fail(const TCHAR* Why)
{
    CloseSocket(); SendBytes.Reset(); ReceiveBytes.Reset(); Selection.Empty();
    State = EAIHeroesLocalExchangeState::Failed; Reason = Why;
}

bool FAIHeroesLocalExchange::Start(int32 Port, const FString& Model, const FString& EngineJson,
    bool bExplicitLocalOptIn, double Now, double Lifetime, FString& Error)
{
    using namespace AIHeroesLocalSocketDetail;
    if (!IsInGameThread()) { Error = TEXT("GAME_THREAD_REQUIRED"); return false; }
    Cancel(); PollCount = 0;
    Error = TEXT("LOCAL_OPT_IN_AND_VALID_CONFIG_REQUIRED");
    if (!bExplicitLocalOptIn || !FAIHeroesLocalExchangeProtocol::ValidEndpoint(Port, Model) ||
        !Clock(Now) || !FMath::IsFinite(Lifetime) || Lifetime <= 0.0 || Lifetime > 4.0 ||
        !FMath::IsFinite(Now + Lifetime) || Now + Lifetime <= Now)
    { Fail(*Error); return false; }
    FString Body;
    if (!FAIHeroesLocalExchangeProtocol::BuildRequest(EngineJson, Model, Body, RequestId, Choices, Error))
    { Fail(*Error); return false; }
    const FString Headers = FString::Printf(
        TEXT("POST /v1/chat/completions HTTP/1.0\r\nHost: 127.0.0.1:%d\r\nContent-Type: application/json\r\nAccept: application/json\r\nAccept-Encoding: identity\r\nConnection: close\r\nContent-Length: %d\r\n\r\n"),
        Port, Body.Len());
    const FString Wire = Headers + Body;
    SendBytes.Reserve(Wire.Len());
    for (TCHAR C : Wire)
    {
        if (C > 127) { Error = TEXT("NON_ASCII_REQUEST"); Fail(*Error); return false; }
        SendBytes.Add(static_cast<uint8>(C));
    }
    ISocketSubsystem* Subsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (!Subsystem) { Error = TEXT("SOCKET_SUBSYSTEM_UNAVAILABLE"); Fail(*Error); return false; }
    Socket = Subsystem->CreateSocket(NAME_Stream, TEXT("AIHeroesExplicitLoopback"), FNetworkProtocolTypes::IPv4);
    if (!Socket || !Socket->SetNonBlocking(true))
    { Error = TEXT("NONBLOCKING_SOCKET_UNAVAILABLE"); Fail(*Error); return false; }
    // Direct numeric IPv4 only: no hostname resolution, proxy configuration,
    // URL interpretation, cookies, authentication, redirects or shell commands.
    auto Address = Subsystem->CreateInternetAddr(FNetworkProtocolTypes::IPv4);
    bool bValid = false; Address->SetIp(TEXT("127.0.0.1"), bValid); Address->SetPort(Port);
    if (!bValid || (!Socket->Connect(*Address) && !PendingSocketError(Subsystem->GetLastErrorCode())))
    { Error = TEXT("CONNECT_FAILED"); Fail(*Error); return false; }
    Alias = Model; Started = LastClock = Now; Deadline = Now + Lifetime;
    State = EAIHeroesLocalExchangeState::Connecting; Reason = TEXT("PENDING_UNATTESTED_LOCAL_SERVER");
    Error.Empty(); return true;
}

void FAIHeroesLocalExchange::Tick(double Now)
{
    using namespace AIHeroesLocalSocketDetail;
    if (!IsInGameThread() || !IsPending()) return;
    ++PollCount;
    if (!Clock(Now) || Now < LastClock || Now < Started || Now >= Deadline)
    { Fail(TEXT("DEADLINE_OR_CLOCK_FAILURE")); return; }
    LastClock = Now;
    ISocketSubsystem* Subsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (!Subsystem || !Socket) { Fail(TEXT("SOCKET_UNAVAILABLE")); return; }
    if (State == EAIHeroesLocalExchangeState::Connecting)
    {
        const auto Connection = Socket->GetConnectionState();
        if (Connection == SCS_ConnectionError) { Fail(TEXT("CONNECT_FAILED")); return; }
        if (Connection != SCS_Connected) return;
        State = EAIHeroesLocalExchangeState::Sending;
    }
    if (State == EAIHeroesLocalExchangeState::Sending)
    {
        int32 Written = 0;
        if (!Socket->Send(SendBytes.GetData() + Sent, FMath::Min(8192, SendBytes.Num() - Sent), Written))
        {
            if (!PendingSocketError(Subsystem->GetLastErrorCode())) Fail(TEXT("SEND_FAILED"));
            return;
        }
        if (Written < 0 || Written > SendBytes.Num() - Sent) { Fail(TEXT("INVALID_SEND_COUNT")); return; }
        Sent += Written;
        if (Sent < SendBytes.Num()) return;
        SendBytes.Reset(); State = EAIHeroesLocalExchangeState::Receiving;
    }
    uint8 Chunk[8192]; int32 Read = 0;
    const bool bRead = Socket->Recv(Chunk, UE_ARRAY_COUNT(Chunk), Read, ESocketReceiveFlags::None);
    if (!bRead)
    {
        if (!PendingSocketError(Subsystem->GetLastErrorCode())) Fail(TEXT("CONNECTION_CLOSED_OR_FAILED"));
        return;
    }
    if (Read < 0 || Read > UE_ARRAY_COUNT(Chunk)) { Fail(TEXT("INVALID_RECEIVE_COUNT")); return; }
    if (Read == 0) return; // Some backends report true/zero when no bytes are ready.
    if (ReceiveBytes.Num() > FAIHeroesLocalExchangeProtocol::MaxHeaderBytes +
        FAIHeroesLocalExchangeProtocol::MaxBodyBytes - Read)
    { Fail(TEXT("RESPONSE_TOO_LARGE")); return; }
    ReceiveBytes.Append(Chunk, Read);
    FString Body, Error;
    const auto Frame = FAIHeroesLocalExchangeProtocol::ReadFrame(ReceiveBytes, Body, Error);
    if (Frame == EAIHeroesLocalFrame::Incomplete) return;
    if (Frame == EAIHeroesLocalFrame::Rejected) { Fail(*Error); return; }
    FString ValidatedSelection;
    if (!FAIHeroesLocalExchangeProtocol::DecodeEnvelope(Body, Alias, RequestId, Choices, ValidatedSelection, Error))
    { Fail(*Error); return; }
    CloseSocket(); ReceiveBytes.Reset(); Selection = MoveTemp(ValidatedSelection);
    State = EAIHeroesLocalExchangeState::Complete;
    Reason = TEXT("WIRE_COMPLETE_CHECKPOINT_UNATTESTED");
}
