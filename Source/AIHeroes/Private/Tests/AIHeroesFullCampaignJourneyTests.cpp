// Native end-to-end regression for the ONE five-site campaign.
// Starts from the real GameMode initial state. Never edits guards, resources,
// fog, movement, combat results, campaign tiers or user-owned save slots.
#include "AIHeroesGameMode.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "AIHeroesCampaignRules.h"
#include "AIHeroesCampaignSave.h"
#include "AIHeroesFogRules.h"
#include "AIHeroesSaveSystem.h"
#include "AIHeroesWorldNode.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/Guid.h"

namespace
{
struct FFullJourneyFixture
{
    UWorld* World = nullptr;
    AAIHeroesGameMode* Game = nullptr;

    FFullJourneyFixture()
    {
        const auto Values = UWorld::InitializationValues()
            .AllowAudioPlayback(false).CreatePhysicsScene(false)
            .CreateNavigation(false).CreateAISystem(false)
            .ShouldSimulatePhysics(false);
        World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None,
            nullptr, true, ERHIFeatureLevel::Num, &Values);
        if (!World) return;
        if (GEngine)
            GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
        Game = World->SpawnActor<AAIHeroesGameMode>();
        if (Game) Game->DispatchBeginPlay();
    }
    ~FFullJourneyFixture()
    {
        if (!World) return;
        World->DestroyWorld(false);
        if (GEngine) GEngine->DestroyWorldContext(World);
    }
    AAIHeroesWorldNode* Site(FName Id) const
    {
        if (!Game) return nullptr;
        for (AAIHeroesWorldNode* Node : Game->Nodes)
            if (IsValid(Node) && FName(*Node->GetDisplayName()) == Id) return Node;
        return nullptr;
    }
    bool Ready() const { return Game && Game->Nodes.Num() == 5; }
};

// The game only writes rotating campaign A/B slots in the active player
// namespace. This test never uses those. A unique, test-owned prefix is used
// for save/load and always cleaned up, including on an assertion failure.
struct FTestSlotCleanup
{
    FString Prefix = FString(TEXT("AIHEROES_E2E_QA_")) +
        FGuid::NewGuid().ToString(EGuidFormats::Digits);
    ~FTestSlotCleanup()
    {
        for (const FString& Slot : {
            FAIHeroesSaveSystem::SlotA(Prefix), FAIHeroesSaveSystem::SlotB(Prefix)})
            if (UGameplayStatics::DoesSaveGameExist(Slot, 0))
                UGameplayStatics::DeleteGameInSlot(Slot, 0);
    }
};

bool BuyThroughPlayerPanel(AAIHeroesGameMode& G, EAIHeroesBasePurchase Kind)
{
    const auto Quote = G.GetBasePanelOffer(Kind);
    if (!Quote.CanBuy() || G.IsLiveBattleActive() || G.IsBasePanelVisible())
        return false;
    const int32 Gold = G.Gold, Wood = G.Wood, Ore = G.Ore, Data = G.DataFragments;
    G.ToggleBasePanel();
    if (!G.IsBasePanelVisible()) return false;
    G.PreviewBasePurchase(Kind);
    if (!G.CanConfirmBasePurchase()) { G.CloseBasePanel(); return false; }
    G.ConfirmBasePurchase();
    G.CloseBasePanel();
    return G.Gold == Gold - Quote.Gold &&
        G.Wood == Wood - Quote.Wood &&
        G.Ore == Ore - Quote.Ore &&
        G.DataFragments == Data - Quote.Data;
}

bool EndPlayerDay(AAIHeroesGameMode& G, FString& Failure)
{
    const int32 Before = G.Day;
    G.EndTurn();
    if (G.Day != Before + 1 || G.MovesLeft != 4)
    {
        Failure = FString::Printf(TEXT("Day %d could not advance: %s"),
            Before, *G.LastEvent);
        return false;
    }
    UE_LOG(LogTemp, Display, TEXT("[AIHEROES E2E] day=%d gold=%d wood=%d ore=%d"),
        G.Day, G.Gold, G.Wood, G.Ore);
    return true;
}

bool PlayerVisits(FFullJourneyFixture& F, FName Id, FString& Failure)
{
    AAIHeroesGameMode& G = *F.Game;
    AAIHeroesWorldNode* Node = F.Site(Id);
    if (!IsValid(Node) || !G.IsNodeDiscovered(Id))
    {
        Failure = TEXT("Player cannot select unknown or absent site: ") + Id.ToString();
        return false;
    }
    const FAIHeroesAdventurePathResult Before = G.GetTravelPathTo(Id);
    if (!Before.IsReachableThisTurn())
    {
        Failure = FString::Printf(TEXT("No authorized path to %s: %s"),
            *Id.ToString(), *Before.Reason);
        return false;
    }
    const int32 Moves = G.MovesLeft, Data = G.DataFragments;
    const bool bAlreadyOwned = Node->bOwned;
    G.SelectNode(Node);
    if (G.SelectedNode.Get() != Node)
    {
        Failure = TEXT("Selection rejected for ") + Id.ToString();
        return false;
    }
    G.ConfirmSelectedNode();
    if (G.IsLiveBattleActive())
    {
        if (bAlreadyOwned || !Before.bBattleAtDestination)
        {
            Failure = TEXT("Battle started against wrong target state: ") + Id.ToString();
            return false;
        }
        G.FastFinishLiveBattle();
        for (int32 Frame = 0; Frame < 320 &&
            G.GetLiveBattlePhase() == EAIHeroesLiveBattlePhase::Running; ++Frame)
            G.Tick(1.f / 60.f);
        if (G.GetLiveBattlePhase() != EAIHeroesLiveBattlePhase::Result)
        {
            Failure = TEXT("Live battle did not complete: ") + Id.ToString() +
                TEXT(" event=") + G.LastEvent;
            return false;
        }
        if (!G.LastBattleRecord.bVictory)
        {
            Failure = FString::Printf(
                TEXT("Player army lost at %s day=%d: %s"),
                *Id.ToString(), G.Day, *G.LastBattleRecord.Summary);
            G.ReturnFromLiveBattle();
            return false;
        }
        if (G.DataFragments <= Data)
        {
            Failure = TEXT("Battle was won but data reward did not arrive: ") + Id.ToString();
            return false;
        }
        G.ReturnFromLiveBattle();
    }
    if (!Node->bOwned || G.MovesLeft != Moves - Before.TotalCost ||
        !G.GetHeroStrategicLocation().Equals(Node->GetActorLocation(), 1.0))
    {
        Failure = FString::Printf(TEXT("Visit failed for %s: %s"),
            *Id.ToString(), *G.LastEvent);
        return false;
    }
    UE_LOG(LogTemp, Display,
        TEXT("[AIHEROES E2E] visited=%s day=%d moves=%d armyHP=%d/%d data=%d owned=%d"),
        *Id.ToString(), G.Day, G.MovesLeft,
        G.PlayerArmy.IsValidIndex(0) ? G.PlayerArmy[0].TotalHealth : -1,
        G.PlayerArmy.IsValidIndex(1) ? G.PlayerArmy[1].TotalHealth : -1,
        G.DataFragments, Node->bOwned);
    return true;
}

// A five-move itinerary is not one executable game order. The simulated
// player must choose a discovered, owned intermediate site explicitly.
bool PlayerCaptures(FFullJourneyFixture& F, FName Id, FString& Failure)
{
    AAIHeroesGameMode& G = *F.Game;
    AAIHeroesWorldNode* Goal = F.Site(Id);
    if (!Goal) { Failure = TEXT("Missing objective ") + Id.ToString(); return false; }
    for (int32 Choice = 0; Choice < 20 && !Goal->bOwned; ++Choice)
    {
        const auto Plan = G.GetTravelPathTo(Id);
        if (Plan.IsReachableThisTurn())
            return PlayerVisits(F, Id, Failure);
        if (Plan.Status != EAIHeroesAdventurePathStatus::NeedsMoreMoves)
        {
            Failure = TEXT("Player's route blocked to ") + Id.ToString() +
                TEXT(": ") + Plan.Reason;
            return false;
        }
        if (Plan.TotalCost <= 4)
        {
            if (!EndPlayerDay(G, Failure)) return false;
            continue;
        }
        // Prefer the furthest owned waypoint that is genuinely affordable.
        FName Staging = NAME_None;
        for (int32 Index = Plan.Nodes.Num() - 2; Index > 0; --Index)
        {
            const FName Candidate = Plan.Nodes[Index];
            const AAIHeroesWorldNode* Stop = F.Site(Candidate);
            if (!IsValid(Stop) || !Stop->bOwned ||
                !G.IsNodeDiscovered(Candidate)) continue;
            const auto Leg = G.GetTravelPathTo(Candidate);
            if (Leg.IsReachableThisTurn() && Leg.TotalCost > 0)
            {
                Staging = Candidate;
                break;
            }
        }
        if (!Staging.IsNone())
        {
            if (!PlayerVisits(F, Staging, Failure)) return false;
        }
        else if (G.MovesLeft < 4)
        {
            if (!EndPlayerDay(G, Failure)) return false;
        }
        else
        {
            Failure = TEXT("Route longer than full day lacks reachable owned waypoint: ") +
                Id.ToString();
            return false;
        }
    }
    if (!Goal->bOwned)
    {
        Failure = TEXT("Bounded player travel could not capture ") + Id.ToString();
        return false;
    }
    return true;
}

UAIHeroesCampaignSave* CaptureRealVictory(const AAIHeroesGameMode& G)
{
    auto* Save = NewObject<UAIHeroesCampaignSave>(GetTransientPackage());
    Save->Day = G.Day;
    Save->Gold = G.Gold;
    Save->Wood = G.Wood;
    Save->Ore = G.Ore;
    Save->BaseTier = G.BaseTier;
    Save->MovesLeft = G.MovesLeft;
    Save->TravelRuleVersion = G.TravelRuleVersion;
    Save->ProtocolTier = G.ProtocolTier;
    Save->ModelSlotTier = G.ModelSlotTier;
    Save->DataFragments = G.DataFragments;
    Save->bRecruitmentStockInitialized = true;
    Save->MilitiaReserve = G.RecruitmentStock.Militia;
    Save->ScoutReserve = G.RecruitmentStock.Scouts;
    Save->bFieldDwellingsInitialized = true;
    Save->bForestCampClaimed = G.FieldDwellings.Forest.bClaimed;
    Save->ForestCampUnits = G.FieldDwellings.Forest.Available;
    Save->bOreAerieClaimed = G.FieldDwellings.Ore.bClaimed;
    Save->OreAerieUnits = G.FieldDwellings.Ore.Available;
    Save->bFogInitialized = true;
    Save->PlayerArmy = G.PlayerArmy;
    Save->BattleJournal = G.BattleJournal;
    Save->LastCombatStep = G.LastCombatStep;
    Save->LastBattleRecord = G.LastBattleRecord;
    for (const AAIHeroesWorldNode* Node : G.Nodes)
    {
        if (!IsValid(Node)) return nullptr;
        const FName Id(*Node->GetDisplayName());
        FAIHeroesSavedNode Entry;
        Entry.NodeId = Id;
        Entry.Type = Node->NodeType;
        Entry.bOwned = Node->bOwned;
        Entry.Defenders = Node->Defenders;
        Save->NodeStates.Add(MoveTemp(Entry));
        if (G.IsNodeDiscovered(Id))
            Save->ExploredSitesMask |= FAIHeroesFogRules::SiteBit(Id);
        if (G.GetHeroStrategicLocation().Equals(Node->GetActorLocation(), 1.0))
            Save->HeroNodeId = Id;
        if (G.SelectedNode.Get() == Node) Save->SelectedNodeId = Id;
    }
    return Save;
}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFullRealCampaignJourney,
    "AIHeroes.Playability.RealCampaign.EndToEndVictoryAndSaveReadback",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFullRealCampaignJourney::RunTest(const FString&)
{
    FFullJourneyFixture F;
    if (!TestTrue(TEXT("Actual fresh campaign spawns five map sites"), F.Ready()))
        return false;
    AAIHeroesGameMode& G = *F.Game;
    if (!TestEqual(TEXT("Fresh campaign starts at day one"), G.Day, 1) ||
        !TestEqual(TEXT("Road V2 is active"), G.TravelRuleVersion, 2) ||
        !TestFalse(TEXT("Fresh campaign not already victorious"),
            G.GetCampaignProgress().bVictory)) return false;

    // Four offers are affordable from the genuine starting stock/currency.
    // Confirm each via the same preview/confirm gate as the clickable panel.
    for (int32 I = 0; I < 2; ++I)
    {
        if (!TestTrue(TEXT("Player confirms a five-militia recruitment offer"),
            BuyThroughPlayerPanel(G, EAIHeroesBasePurchase::Militia))) return false;
        if (!TestTrue(TEXT("Player confirms a two-scout recruitment offer"),
            BuyThroughPlayerPanel(G, EAIHeroesBasePurchase::Scouts))) return false;
    }
    TestEqual(TEXT("Starting recruitment used real treasury"), G.Gold, 0);
    TestEqual(TEXT("All initial militia stock purchased"), G.RecruitmentStock.Militia, 0);
    TestEqual(TEXT("All initial scout stock purchased"), G.RecruitmentStock.Scouts, 0);

    FString Failure;
    for (const FName Id : {
        FName(TEXT("FOREST")), FName(TEXT("ORE")),
        FName(TEXT("OUTPOST")), FName(TEXT("RUINS"))})
    {
        if (!PlayerCaptures(F, Id, Failure))
        {
            AddError(TEXT("REAL CAMPAIGN BLOCKED: ") + Failure);
            return false;
        }
    }
    if (!TestEqual(TEXT("All four sites genuinely captured"),
        G.GetCampaignProgress().Completed, 2)) return false;
    if (!TestEqual(TEXT("Four real fights added four battle records"),
        G.BattleJournal.Num(), 4)) return false;
    if (!TestTrue(TEXT("Earned fragments are enough for both model slots"),
        G.DataFragments >= 8)) return false;

    // Economic development is purchased from actually accumulated income.
    // No direct mutation of resources or tiers; EndTurn runs rival/world rules.
    for (int32 Step = 0; Step < 55 && !G.GetCampaignProgress().bVictory; ++Step)
    {
        bool bBought = false;
        if (G.ProtocolTier < 3 &&
            G.GetBasePanelOffer(EAIHeroesBasePurchase::Protocol).CanBuy())
            bBought = BuyThroughPlayerPanel(G, EAIHeroesBasePurchase::Protocol);
        else if (G.ModelSlotTier < 2 &&
            G.GetBasePanelOffer(EAIHeroesBasePurchase::ModelSlot).CanBuy())
            bBought = BuyThroughPlayerPanel(G, EAIHeroesBasePurchase::ModelSlot);
        if (!bBought && !EndPlayerDay(G, Failure))
        {
            AddError(TEXT("REAL ECONOMY BLOCKED: ") + Failure);
            return false;
        }
        if (G.Day > 50)
        {
            AddError(TEXT("Campaign requires over 50 days to complete legal upgrades."));
            return false;
        }
    }
    const auto Won = G.GetCampaignProgress();
    if (!TestTrue(TEXT("No synthetic state: real GameMode reported victory"),
        Won.bVictory && Won.Completed == 4 &&
        Won.NextFocusId == FName(TEXT("VICTORY")))) return false;
    if (!TestTrue(TEXT("Both protocol and model upgrades actually purchased"),
        G.ProtocolTier == 3 && G.ModelSlotTier == 2)) return false;
    UE_LOG(LogTemp, Display,
        TEXT("[AIHEROES E2E] REAL VICTORY day=%d gold=%d wood=%d ore=%d data=%d journal=%d"),
        G.Day, G.Gold, G.Wood, G.Ore, G.DataFragments, G.BattleJournal.Num());

    // Use the same production SaveSystem and validator, but ONLY GUID-owned
    // temporary slots. The four player's existing *.sav files are untouched.
    FTestSlotCleanup Slots;
    UAIHeroesCampaignSave* Snapshot = CaptureRealVictory(G);
    if (!TestNotNull(TEXT("Constructed snapshot from actual won game"), Snapshot))
        return false;
    FString Error;
    if (!TestTrue(TEXT("Real victory passes production save preflight"),
        FAIHeroesCampaignRules::Validate(*Snapshot, Error)))
    {
        AddError(TEXT("SAVE VALIDATION: ") + Error);
        return false;
    }
    if (!TestTrue(TEXT("Real victory saved to unique isolated A/B namespace"),
        FAIHeroesSaveSystem::Save(*Snapshot, Slots.Prefix, FString(), Error)))
    {
        AddError(TEXT("SAVE FAILURE: ") + Error);
        return false;
    }
    const FAIHeroesLoadedCampaign Restored =
        FAIHeroesSaveSystem::Load(Slots.Prefix, FString());
    if (!TestNotNull(TEXT("Round-trip returned validated snapshot"),
        Restored.Snapshot)) return false;
    const UAIHeroesCampaignSave& Loaded = *Restored.Snapshot;
    TestEqual(TEXT("Day survived disk read-back"), Loaded.Day, G.Day);
    TestEqual(TEXT("Gold survived disk read-back"), Loaded.Gold, G.Gold);
    TestEqual(TEXT("Movement survived disk read-back"), Loaded.MovesLeft, G.MovesLeft);
    TestEqual(TEXT("Protocol tier survived disk read-back"), Loaded.ProtocolTier, G.ProtocolTier);
    TestEqual(TEXT("Model tier survived disk read-back"), Loaded.ModelSlotTier, G.ModelSlotTier);
    TestEqual(TEXT("Battle journal survived disk read-back"),
        Loaded.BattleJournal, G.BattleJournal);
    FAIHeroesCampaignObjectiveInput Reconstructed;
    Reconstructed.ProtocolTier = Loaded.ProtocolTier;
    Reconstructed.ModelSlotTier = Loaded.ModelSlotTier;
    for (const auto& Node : Loaded.NodeStates)
        Reconstructed.Nodes.Add({Node.NodeId, Node.Type, Node.bOwned});
    TestTrue(TEXT("Victory is reconstructed from physical read-back, not stored flag"),
        FAIHeroesCampaignObjectives::Evaluate(Reconstructed).bVictory);
    UE_LOG(LogTemp, Display,
        TEXT("[AIHEROES E2E] SAVE READBACK VERIFIED day=%d seq=%d nodes=%d; GUID test slots cleaned"),
        Loaded.Day, Loaded.SaveSequence, Loaded.NodeStates.Num());
    return true;
}
#endif
