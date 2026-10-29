#include "AIHeroesTravelRules.h"

#include "AIHeroesAdvisor.h"
#include "AIHeroesTurnRules.h"
#include "AIHeroesFogRules.h"

namespace
{
bool TravelSafePosition(const FVector& P)
{
    return !P.ContainsNaN() && FMath::Abs(P.X) <= 1000000.0 &&
        FMath::Abs(P.Y) <= 1000000.0 && FMath::Abs(P.Z) <= 1000000.0;
}
bool TravelGuarded(const FAIHeroesAdvisorNode& Node)
{
    for (const auto& Guard : Node.Guards) if (Guard.IsAlive()) return true;
    return false;
}
const TCHAR* TravelMapIds[] = {TEXT("HQ"), TEXT("FOREST"), TEXT("ORE"), TEXT("OUTPOST"), TEXT("RUINS")};
const EAIHeroesNodeType TravelMapTypes[] = {EAIHeroesNodeType::Headquarters,
    EAIHeroesNodeType::ForestMine, EAIHeroesNodeType::OreMine,
    EAIHeroesNodeType::Outpost, EAIHeroesNodeType::Ruins};
}

bool FAIHeroesTravelRules::BuildGraph(const FAIHeroesAdvisorSnapshot& S,
    FAIHeroesAdventureGraph& Out, FString& Error, const TSet<FName>& Cleared)
{
    Out = FAIHeroesAdventureGraph();
    Error = TEXT("ROAD V2: invalid or incomplete five-site map.");
    if (S.TravelRuleVersion != RoadVersion || S.Nodes.Num() != 5 || Cleared.Num() > 5) return false;
    FAIHeroesAdventureGraph Candidate;
    Candidate.Revision = RoadVersion;
    for (int32 I = 0; I < 5; ++I)
    {
        const FName Id(TravelMapIds[I]);
        const FAIHeroesAdvisorNode* Found = nullptr;
        for (const auto& Node : S.Nodes)
        {
            if (Node.Id != Id) continue;
            if (Found) return false;
            Found = &Node;
        }
        if (!Found || Found->Type != TravelMapTypes[I] || !TravelSafePosition(Found->Location) ||
            Found->Guards.Num() > 2 || (I == 0 && !Found->bOwned)) return false;
        for (const auto& Guard : Found->Guards)
            if (Guard.TotalHealth < 0 || Guard.TotalHealth > 500000) return false;
        if ((Found->bOwned && TravelGuarded(*Found)) || (I == 0 && !Found->Guards.IsEmpty())) return false;
        FAIHeroesAdventureNode Node;
        Node.Id = Id;
        Node.bDiscovered = FAIHeroesFogRules::IsKnown(
            S.bFogEnabled, S.ExploredSitesMask, Id);
        Node.bHostileGuard = !Found->bOwned && !Cleared.Contains(Id) && TravelGuarded(*Found);
        Candidate.Nodes.Add(Node);
        if (Node.bHostileGuard) Candidate.Revision |= 1u << (8 + I);
    }
    if (S.bFogEnabled)
    {
        // The same discovered-site set is used by the native path core.
        // Route revisions change whenever reconnaissance adds knowledge.
        Candidate.Revision |= static_cast<uint32>(S.ExploredSitesMask) << 16;
    }
    for (FName Id : Cleared)
        if (!Candidate.Nodes.ContainsByPredicate([Id](const auto& N) { return N.Id == Id; })) return false;
    // Coincident sites make a position->identity mapping ambiguous; reject all.
    for (int32 I = 0; I < S.Nodes.Num(); ++I)
        for (int32 J = I + 1; J < S.Nodes.Num(); ++J)
            if (FVector::DistSquared(S.Nodes[I].Location, S.Nodes[J].Location) < 1.0) return false;
    auto Road = [&Candidate](const TCHAR* A, const TCHAR* B, int32 Cost)
    {
        Candidate.Links.Add({FName(A), FName(B), Cost, true});
        Candidate.Links.Add({FName(B), FName(A), Cost, true});
    };
    Road(TEXT("HQ"), TEXT("FOREST"), 2);
    Road(TEXT("HQ"), TEXT("ORE"), 2);
    Road(TEXT("FOREST"), TEXT("OUTPOST"), 1);
    Road(TEXT("ORE"), TEXT("RUINS"), 1);
    Road(TEXT("OUTPOST"), TEXT("RUINS"), 2);
    Out = MoveTemp(Candidate);
    Error.Empty();
    return true;
}

FAIHeroesAdventurePathResult FAIHeroesTravelRules::Find(
    const FAIHeroesAdvisorSnapshot& S, const FVector& From, FName Destination,
    int32 AvailableMoves, const TSet<FName>& Cleared)
{
    FAIHeroesAdventurePathResult Out;
    Out.Reason = TEXT("TRAVEL BLOCKED: invalid version, position, target or budget.");
    if (!IsSupported(S.TravelRuleVersion) || !TravelSafePosition(From) ||
        AvailableMoves < 0 || AvailableMoves > 4 || S.Nodes.Num() > 64 || Destination.IsNone()) return Out;
    const FAIHeroesAdvisorNode* Target = nullptr;
    for (const auto& Node : S.Nodes)
    {
        if (Node.Id != Destination) continue;
        if (Target) return Out;
        Target = &Node;
    }
    if (!Target || !TravelSafePosition(Target->Location)) return Out;
    if (S.TravelRuleVersion == LegacyVersion)
    {
        Out.bBattleAtDestination = !Target->bOwned && !Cleared.Contains(Destination) && TravelGuarded(*Target);
        Out.TotalCost = FAIHeroesTurnRules::TravelCost(From, Target->Location, Out.bBattleAtDestination);
        Out.MissingMoves = FMath::Max(0, Out.TotalCost - AvailableMoves);
        Out.Status = Out.MissingMoves > 0 ? EAIHeroesAdventurePathStatus::NeedsMoreMoves : EAIHeroesAdventurePathStatus::Ready;
        Out.Nodes.Add(Destination);
        Out.SourceRevision = LegacyVersion;
        Out.Reason = TEXT("Legacy distance V1, preserved for this campaign.");
        return Out;
    }
    FAIHeroesAdventureGraph Graph;
    if (!BuildGraph(S, Graph, Out.Reason, Cleared)) return Out;
    FName Origin = NAME_None;
    for (const auto& Node : S.Nodes)
    {
        if (FVector::DistSquared(From, Node.Location) >= 1.0) continue;
        if (!Origin.IsNone() || (!Node.bOwned && !Cleared.Contains(Node.Id)))
        {
            Out.Reason = TEXT("ROAD V2: invalid hero origin ownership.");
            return Out;
        }
        Origin = Node.Id;
    }
    if (Origin.IsNone())
    {
        Out.Reason = TEXT("ROAD V2: hero must be at a known owned site.");
        return Out;
    }
    Out = FAIHeroesAdventurePath::Find(Graph, Origin, Destination, AvailableMoves);
    if (Out.Status == EAIHeroesAdventurePathStatus::Unreachable)
        Out.Reason = TEXT("ROAD BLOCKED: capture a guarding site before passing through it. No move issued.");
    return Out;
}

FString FAIHeroesTravelRules::Describe(const FAIHeroesAdventurePathResult& Path)
{
    if (Path.Status == EAIHeroesAdventurePathStatus::InvalidInput ||
        Path.Status == EAIHeroesAdventurePathStatus::Unreachable) return Path.Reason;
    TArray<FString> Names;
    for (FName Id : Path.Nodes) Names.Add(Id.ToString());
    FString Text = FString::Printf(TEXT("PATH %s | %d moves"), *FString::Join(Names, TEXT(" > ")), Path.TotalCost);
    if (Path.MissingMoves > 0) Text += FString::Printf(TEXT(" | need %d more"), Path.MissingMoves);
    if (Path.bBattleAtDestination) Text += TEXT(" | battle needs player confirmation");
    return Text;
}
