#pragma once

#include "CoreMinimal.h"

// Read-only strategic connectivity. Coordinates and rendered geometry are not authority.
struct FAIHeroesAdventureNode
{
    FName Id = NAME_None;
    bool bDiscovered = false;
    bool bPassable = true;
    bool bHostileGuard = false;
};

struct FAIHeroesAdventureLink
{
    FName From = NAME_None;
    FName To = NAME_None;
    int32 MoveCost = 1;
    bool bEnabled = true; // Directed: bidirectional travel needs two links.
};

struct FAIHeroesAdventureGraph
{
    uint32 Revision = 0;
    TArray<FAIHeroesAdventureNode> Nodes;
    TArray<FAIHeroesAdventureLink> Links;
};

enum class EAIHeroesAdventurePathStatus : uint8
{
    InvalidInput, Unreachable, NeedsMoreMoves, Ready
};

struct FAIHeroesAdventurePathResult
{
    EAIHeroesAdventurePathStatus Status = EAIHeroesAdventurePathStatus::InvalidInput;
    TArray<FName> Nodes; // Includes both start and destination; never a partial move.
    int32 TotalCost = 0;
    int32 MissingMoves = 0;
    uint32 SourceRevision = 0;
    bool bBattleAtDestination = false;
    FString Reason;

    bool IsReachableThisTurn() const { return Status == EAIHeroesAdventurePathStatus::Ready; }
};

class AIHEROES_API FAIHeroesAdventurePath
{
public:
    static constexpr int32 MaxNodes = 128;
    static constexpr int32 MaxLinks = 1024;
    static constexpr int32 MaxLinkCost = 1000000;

    // No actors, files, network, random numbers, purchases or battle simulation.
    // Ready means a valid proposal, NOT permission to move or attack.
    static FAIHeroesAdventurePathResult Find(
        const FAIHeroesAdventureGraph& Graph, FName Start, FName Destination,
        int32 AvailableMoves);
};
