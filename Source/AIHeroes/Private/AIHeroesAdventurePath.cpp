#include "AIHeroesAdventurePath.h"
#include "AIHeroesAdventurePathCore.h"

FAIHeroesAdventurePathResult FAIHeroesAdventurePath::Find(
    const FAIHeroesAdventureGraph& Graph, FName Start, FName Destination,
    int32 AvailableMoves)
{
    FAIHeroesAdventurePathResult Out;
    Out.SourceRevision = Graph.Revision;
    Out.Reason = TEXT("Invalid adventure graph or movement budget.");
    // Bound allocations before conversion. IDs are stable ASCII identifiers,
    // not translated display labels or strings returned by a model.
    if (Graph.Nodes.Num() > MaxNodes || Graph.Links.Num() > MaxLinks) return Out;
    AIHeroesAdventureCore::Graph Input;
    Input.Revision = Graph.Revision;
    for (const auto& N : Graph.Nodes)
        Input.Nodes.push_back({TCHAR_TO_UTF8(*N.Id.ToString()), N.bDiscovered, N.bPassable, N.bHostileGuard});
    for (const auto& L : Graph.Links)
        Input.Links.push_back({TCHAR_TO_UTF8(*L.From.ToString()), TCHAR_TO_UTF8(*L.To.ToString()), L.MoveCost, L.bEnabled});
    const auto Result = AIHeroesAdventureCore::Find(Input,
        TCHAR_TO_UTF8(*Start.ToString()), TCHAR_TO_UTF8(*Destination.ToString()), AvailableMoves);
    switch (Result.State)
    {
        case AIHeroesAdventureCore::Status::InvalidInput: return Out;
        case AIHeroesAdventureCore::Status::Unreachable:
            Out.Status = EAIHeroesAdventurePathStatus::Unreachable;
            Out.Reason = TEXT("No known passable route to this destination.");
            return Out;
        case AIHeroesAdventureCore::Status::NeedsMoreMoves:
            Out.Status = EAIHeroesAdventurePathStatus::NeedsMoreMoves;
            Out.Reason = TEXT("Known route exceeds this turn's movement. Nothing has moved.");
            break;
        case AIHeroesAdventureCore::Status::Ready:
            Out.Status = EAIHeroesAdventurePathStatus::Ready;
            Out.Reason = Result.Battle
                ? TEXT("Reachable guard. Player must authorize battle; outcome not predicted.")
                : TEXT("Reachable known route. Player confirmation is still required.");
            break;
    }
    for (const auto I : Result.NodeIndices) Out.Nodes.Add(Graph.Nodes[I].Id);
    Out.TotalCost = Result.Cost;
    Out.MissingMoves = Result.Missing;
    Out.bBattleAtDestination = Result.Battle;
    return Out;
}
