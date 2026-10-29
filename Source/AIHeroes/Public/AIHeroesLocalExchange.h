#pragma once

#include "CoreMinimal.h"

class FSocket;

enum class EAIHeroesLocalExchangeState : uint8
{
    Idle, Connecting, Sending, Receiving, Complete, Failed, Cancelled
};
enum class EAIHeroesLocalFrame : uint8 { Incomplete, Complete, Rejected };

// Restricted non-streaming loopback HTTP profile, NOT a general-purpose client.
// No URLs, DNS, redirects, proxy, credentials, model loading or gameplay authority.
class AIHEROES_API FAIHeroesLocalExchangeProtocol
{
public:
    static constexpr int32 MaxHeaderBytes = 8192;
    static constexpr int32 MaxBodyBytes = 32768;
    static constexpr int32 MaxRequestBytes = 131072;
    static bool ValidEndpoint(int32 Port, const FString& Model);
    static bool BuildRequest(const FString& EngineJson, const FString& Model,
        FString& OutBody, FString& OutRequestId, int32& OutChoices, FString& Error);
    static EAIHeroesLocalFrame ReadFrame(const TArray<uint8>& Bytes,
        FString& OutBody, FString& Error);
    static bool DecodeEnvelope(const FString& Body, const FString& Model,
        const FString& RequestId, int32 Choices, FString& OutSelection, FString& Error);
};

// Single-flight, game-thread pump. Every socket is non-blocking and each tick
// sends/receives at most 8192 bytes. Never wait/sleep or touch UObjects here.
class AIHEROES_API FAIHeroesLocalExchange
{
public:
    FAIHeroesLocalExchange() = default;
    ~FAIHeroesLocalExchange();
    FAIHeroesLocalExchange(const FAIHeroesLocalExchange&) = delete;
    FAIHeroesLocalExchange& operator=(const FAIHeroesLocalExchange&) = delete;
    bool Start(int32 Port, const FString& Model, const FString& EngineJson,
        bool bExplicitLocalOptIn, double Now, double Lifetime, FString& Error);
    void Tick(double Now);
    void Cancel();
    bool IsPending() const;
    EAIHeroesLocalExchangeState GetState() const { return State; }
    const FString& GetSelection() const { return Selection; }
    const FString& GetReason() const { return Reason; }
    int32 GetPollCount() const { return PollCount; }
private:
    FSocket* Socket = nullptr;
    EAIHeroesLocalExchangeState State = EAIHeroesLocalExchangeState::Idle;
    FString Alias, RequestId, Selection, Reason;
    TArray<uint8> SendBytes, ReceiveBytes;
    int32 Sent = 0, Choices = 0, PollCount = 0;
    double Started = 0.0, Deadline = 0.0, LastClock = 0.0;
    void CloseSocket();
    void Fail(const TCHAR* Why);
};
