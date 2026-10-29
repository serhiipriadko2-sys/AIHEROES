#include "AIHeroesZoneJourney.h"

#include "AIHeroesZoneExpeditionRules.h"
#include "Algo/Reverse.h"

namespace
{
constexpr int32 MaxPlanningDays = 14;
constexpr int32 MaxVisibleSites = 20;
constexpr int32 DailyMoves = FAIHeroesZoneExpeditionRules::MovesPerDay;

struct FMoveProposal
{
    int32 Cost = 0; // Zero = not traversable by a single player order.
    bool bGuarded = false;
};

struct FSearchRecord
{
    bool bKnown = false;
    bool bVisited = false;
    int32 Confirmations = MAX_int32;
    int32 TotalMoves = MAX_int32;
    int32 Previous = INDEX_NONE;
    int32 ViaCost = 0;
    EAIHeroesJourneyStepKind Via = EAIHeroesJourneyStepKind::EndDay;
    FString StableSteps;
};

struct FSearchIndex
{
    int32 Nodes = 0;
    int32 At(int32 DayOffset, int32 Left, int32 Site) const
    {
        return ((DayOffset * (DailyMoves + 1) + Left) * Nodes) + Site;
    }
    int32 Site(int32 Index) const { return Index % Nodes; }
    int32 Left(int32 Index) const
    {
        return Index / Nodes % (DailyMoves + 1);
    }
    int32 Day(int32 Index) const
    {
        return Index / (Nodes * (DailyMoves + 1));
    }
};

// Single planner entry; the shipping caller has already obtained the
// exact validated PlayerView graph, not the generated-world oracle.
FAIHeroesJourneyPlan BuildKnownOnly(
    const FAIHeroesAdventureGraph& GivenView,
    const FName From, const FName Target,
    const int32 MovesLeft, const int32 Day, const bool bHasLivingArmy)
{
    FAIHeroesJourneyPlan Result;
    Result.Explanation = TEXT("Known-only itinerary not available.");
    Result.Goal = NAME_None;
    if (Day < 1 || Day > FAIHeroesZoneExpeditionRules::MaxDay ||
        MovesLeft < 0 || MovesLeft > DailyMoves ||
        GivenView.Revision == 0 ||
        GivenView.Nodes.IsEmpty() ||
        GivenView.Nodes.Num() > MaxVisibleSites ||
        GivenView.Links.Num() > FAIHeroesAdventurePath::MaxLinks)
        return Result;

    // Defence-in-depth for the development-only raw graph overload:
    // silently discard all undiscovered nodes and their incident links.
    FAIHeroesAdventureGraph Visible;
    Visible.Revision = GivenView.Revision;
    TSet<FName> Known;
    for (const FAIHeroesAdventureNode& Node : GivenView.Nodes)
    {
        if (!Node.bDiscovered) continue;
        if (Node.Id.IsNone() || Known.Contains(Node.Id))
            return Result; // Invalid or ambiguous known topology.
        Known.Add(Node.Id);
        Visible.Nodes.Add(Node);
    }
    for (const FAIHeroesAdventureLink& Link : GivenView.Links)
    {
        if (Known.Contains(Link.From) && Known.Contains(Link.To))
            Visible.Links.Add(Link);
    }
    const int32 Start = Visible.Nodes.IndexOfByPredicate(
        [From](const auto& Node) { return Node.Id == From; });
    if (Start == INDEX_NONE || !Visible.Nodes[Start].bPassable ||
        Visible.Nodes[Start].bHostileGuard)
        return Result;
    Result.SourceRevision = Visible.Revision;
    const int32 Goal = Visible.Nodes.IndexOfByPredicate(
        [Target](const auto& Node) { return Node.Id == Target; });
    if (Goal == INDEX_NONE || !Visible.Nodes[Goal].bPassable)
    {
        Result.Status = EAIHeroesJourneyStatus::UnknownDestination;
        Result.Explanation = TEXT(
            "Destination is not known or passable on the player's revealed map.");
        return Result;
    }
    Result.Goal = Target;
    Result.ArrivalDay = Day;
    if (Goal == Start)
    {
        Result.Status = EAIHeroesJourneyStatus::AlreadyThere;
        Result.Explanation = TEXT("Hero is already at the selected known site.");
        return Result;
    }
    if (Visible.Nodes[Goal].bHostileGuard && !bHasLivingArmy)
    {
        Result.Status = EAIHeroesJourneyStatus::NoLivingTroops;
        Result.Explanation = TEXT(
            "Known guarded destination requires living troops; battle not simulated.");
        return Result;
    }

    const int32 Count = Visible.Nodes.Num();
    TArray<FMoveProposal> Proposals;
    Proposals.SetNumZeroed(Count * Count);
    for (int32 A = 0; A < Count; ++A)
    {
        const auto& Node = Visible.Nodes[A];
        // A hostile checkpoint must first be won by the human in a real
        // battle. Planning cannot assume victory at an intermediate stop.
        if (!Node.bPassable || Node.bHostileGuard) continue;
        for (int32 B = 0; B < Count; ++B)
        {
            if (B == A || !Visible.Nodes[B].bPassable ||
                (Visible.Nodes[B].bHostileGuard && B != Goal))
                continue;
            const auto Path = FAIHeroesAdventurePath::Find(
                Visible, Node.Id, Visible.Nodes[B].Id, DailyMoves);
            if (Path.IsReachableThisTurn() &&
                Path.TotalCost > 0 && Path.TotalCost <= DailyMoves &&
                Path.Nodes.Num() >= 2)
            {
                auto& P = Proposals[A * Count + B];
                P.Cost = Path.TotalCost;
                P.bGuarded = Path.bBattleAtDestination;
            }
        }
    }

    const int32 Ahead = FMath::Min(
        MaxPlanningDays, FAIHeroesZoneExpeditionRules::MaxDay - Day);
    const FSearchIndex Key{Count};
    TArray<FSearchRecord> Records;
    Records.SetNum((Ahead + 1) * (DailyMoves + 1) * Count);
    const int32 Origin = Key.At(0, MovesLeft, Start);
    Records[Origin].bKnown = true;
    Records[Origin].Confirmations = 0;
    Records[Origin].TotalMoves = 0;

    // Deterministic lexicographic Dijkstra:
    // fewest days -> fewest separate player travel confirmations ->
    // least total movement -> canonical known-site sequence.
    auto IsBetter = [&](int32 Left, int32 Right)
    {
        if (Right == INDEX_NONE) return true;
        const auto& A = Records[Left];
        const auto& B = Records[Right];
        if (Key.Day(Left) != Key.Day(Right))
            return Key.Day(Left) < Key.Day(Right);
        if (A.Confirmations != B.Confirmations)
            return A.Confirmations < B.Confirmations;
        if (A.TotalMoves != B.TotalMoves)
            return A.TotalMoves < B.TotalMoves;
        return A.StableSteps.Compare(B.StableSteps,
            ESearchCase::CaseSensitive) < 0;
    };
    auto Relax = [&](int32 Prev, int32 Next,
                     EAIHeroesJourneyStepKind Kind, int32 Cost,
                     const FString& Token)
    {
        const auto& Old = Records[Prev];
        auto& New = Records[Next];
        const int32 Confirmations = Old.Confirmations +
            (Kind == EAIHeroesJourneyStepKind::ConfirmTravel ? 1 : 0);
        const int32 TotalMoves = Old.TotalMoves + Cost;
        const FString Stable = Old.StableSteps + Token;
        if (New.bKnown &&
            (Confirmations > New.Confirmations ||
             (Confirmations == New.Confirmations &&
              TotalMoves > New.TotalMoves) ||
             (Confirmations == New.Confirmations &&
              TotalMoves == New.TotalMoves &&
              Stable.Compare(New.StableSteps,
                  ESearchCase::CaseSensitive) >= 0)))
            return;
        New.bKnown = true;
        New.Confirmations = Confirmations;
        New.TotalMoves = TotalMoves;
        New.Previous = Prev;
        New.Via = Kind;
        New.ViaCost = Cost;
        New.StableSteps = Stable;
    };

    int32 Found = INDEX_NONE;
    for (int32 Iteration = 0; Iteration < Records.Num(); ++Iteration)
    {
        int32 Current = INDEX_NONE;
        for (int32 I = 0; I < Records.Num(); ++I)
        {
            if (Records[I].bKnown && !Records[I].bVisited &&
                IsBetter(I, Current))
                Current = I;
        }
        if (Current == INDEX_NONE) break;
        Records[Current].bVisited = true;
        const int32 Node = Key.Site(Current);
        const int32 Left = Key.Left(Current);
        const int32 DayOffset = Key.Day(Current);
        if (Node == Goal)
        {
            Found = Current;
            break;
        }

        // Each End Day is only a proposed separate HUMAN command.
        if (DayOffset < Ahead && Left != DailyMoves)
        {
            Relax(Current, Key.At(DayOffset + 1, DailyMoves, Node),
                EAIHeroesJourneyStepKind::EndDay, 0, TEXT("|DAY"));
        }
        for (int32 Dest = 0; Dest < Count; ++Dest)
        {
            const FMoveProposal& P = Proposals[Node * Count + Dest];
            if (P.Cost == 0 || P.Cost > Left) continue;
            Relax(Current, Key.At(DayOffset, Left - P.Cost, Dest),
                EAIHeroesJourneyStepKind::ConfirmTravel, P.Cost,
                TEXT("|") + Visible.Nodes[Dest].Id.ToString());
        }
    }

    if (Found == INDEX_NONE)
    {
        Result.Status = EAIHeroesJourneyStatus::NoLegalItinerary;
        Result.Explanation = TEXT(
            "No known all-or-nothing journey fits daily movement and the bounded horizon. No order issued.");
        return Result;
    }

    TArray<FAIHeroesJourneyStep> Backward;
    int32 Cursor = Found;
    for (int32 CountSteps = 0; Cursor != Origin; ++CountSteps)
    {
        if (CountSteps > 40 || Cursor < 0 ||
            !Records.IsValidIndex(Cursor) ||
            Records[Cursor].Previous == INDEX_NONE)
        {
            Result.Status = EAIHeroesJourneyStatus::NoLegalItinerary;
            Result.Explanation = TEXT(
                "Internal itinerary reconstruction failed closed.");
            return Result;
        }
        const auto& R = Records[Cursor];
        const int32 ToNode = Key.Site(Cursor);
        const bool bTravel =
            R.Via == EAIHeroesJourneyStepKind::ConfirmTravel;
        FAIHeroesJourneyStep Step;
        Step.Kind = R.Via;
        Step.DayOffset = Key.Day(Cursor);
        Step.MoveCost = R.ViaCost;
        Step.MovesRemainingAfter = Key.Left(Cursor);
        if (bTravel)
        {
            Step.Destination = Visible.Nodes[ToNode].Id;
            Step.bBattleAtDestination = Proposals[
                Key.Site(R.Previous) * Count + ToNode].bGuarded;
        }
        Backward.Add(Step);
        Cursor = R.Previous;
    }
    if (Backward.IsEmpty())
    {
        Result.Status = EAIHeroesJourneyStatus::NoLegalItinerary;
        return Result;
    }
    Algo::Reverse(Backward);
    Result.Steps = MoveTemp(Backward);
    Result.DayAdvances = Key.Day(Found);
    Result.ArrivalDay = Day + Result.DayAdvances;
    Result.TravelConfirmations = Records[Found].Confirmations;
    Result.TotalMovement = Records[Found].TotalMoves;
    Result.bGuardianOutcomeUnknown =
        Visible.Nodes[Goal].bHostileGuard;
    Result.Status = Result.Steps.Num() == 1 &&
        Result.Steps[0].Kind == EAIHeroesJourneyStepKind::ConfirmTravel
        ? EAIHeroesJourneyStatus::ReadyNow
        : EAIHeroesJourneyStatus::PlannedWithRestOrStages;
    Result.Explanation = FString::Printf(
        TEXT("READ ONLY: %d player-confirmed site moves, %d explicit end-days; earliest known arrival day %d. %s Replan after each action."),
        Result.TravelConfirmations,
        Result.DayAdvances,
        Result.ArrivalDay,
        Result.bGuardianOutcomeUnknown
            ? TEXT("Guard outcome UNKNOWN.") : TEXT("No known guard at goal."));
    return Result;
}
} // namespace

FAIHeroesJourneyPlan FAIHeroesZoneJourney::Plan(
    const FAIHeroesZoneExpeditionState& Campaign,
    const FName KnownDestination)
{
    FString Error;
    FAIHeroesAdventureGraph View;
    if (!FAIHeroesZoneExpeditionRules::PlayerView(Campaign, View, Error))
    {
        FAIHeroesJourneyPlan Rejected;
        Rejected.Explanation = TEXT(
            "Campaign view invalid. No journey or hidden route disclosed.");
        return Rejected;
    }
    bool bLivingArmy = false;
    for (const auto& Stack : Campaign.PlayerArmy)
        if (Stack.IsAlive()) bLivingArmy = true;
    return BuildKnownOnly(View, Campaign.HeroSite, KnownDestination,
        Campaign.MovesLeft, Campaign.Day, bLivingArmy);
}

#if WITH_DEV_AUTOMATION_TESTS
FAIHeroesJourneyPlan FAIHeroesZoneJourney::PlanVisibleGraphForTests(
    const FAIHeroesAdventureGraph& PlayerVisibleGraph,
    const FName From, const FName To,
    const int32 CurrentMoves, const int32 CurrentDay,
    const bool bHasLivingArmy)
{
    return BuildKnownOnly(PlayerVisibleGraph, From, To,
        CurrentMoves, CurrentDay, bHasLivingArmy);
}
#endif
