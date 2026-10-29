#pragma once

#include "CoreMinimal.h"
#include "AIHeroesAIProposalGate.h"

// Engine-owned serialization only. No network, files, model readiness or actions.
enum class EAIHeroesModelGoal : uint8 { AssessSelected, Explore, PreserveArmy };

class AIHEROES_API FAIHeroesModelRequest
{
public:
    static constexpr int32 MaxBytes = 65536;
    // IDs are correlation tokens supplied by the trusted host, NOT authorization.
    // Failure clears output; callers must never send an older request by accident.
    static bool Build(const FAIHeroesProposalSnapshot& Snapshot,
        const FString& RequestId, const FString& SessionId,
        EAIHeroesModelGoal Goal, FString& OutJson, FString& Error,
        TArray<FAIHeroesAIProposal>* OutMenu = nullptr);
};
