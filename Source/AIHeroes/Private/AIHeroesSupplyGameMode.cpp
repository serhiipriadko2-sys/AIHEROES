#include "AIHeroesGameMode.h"

#include "AIHeroesSupplyLines.h"
#include "AIHeroesTravelRules.h"
#include "AIHeroesWorldNode.h"

FAIHeroesSupplyAccess AAIHeroesGameMode::BuildSupplyAccess() const
{
    FAIHeroesSupplyAccess Access;
    Access.Site = static_cast<EAIHeroesFieldSite>(255); // No valid selection.
    Access.MovesLeft = MovesLeft;

    if (!IsValid(SelectedNode.Get()) || !Nodes.Contains(SelectedNode.Get()))
        return Access;
    if (SelectedNode->NodeType == EAIHeroesNodeType::ForestMine &&
        SelectedNode->GetDisplayName() == TEXT("FOREST"))
        Access.Site = EAIHeroesFieldSite::ForestCamp;
    else if (SelectedNode->NodeType == EAIHeroesNodeType::OreMine &&
        SelectedNode->GetDisplayName() == TEXT("ORE"))
        Access.Site = EAIHeroesFieldSite::OreAerie;
    else return Access;

    const FName ParentId =
        FAIHeroesFieldDwellingRules::ParentNodeId(Access.Site);
    Access.bSourceDiscovered = IsNodeDiscovered(ParentId);
    // All subsequent checks describe current campaign authority; do not
    // infer ownership or hidden map locations from the 3D decorations.
    if (!Access.bSourceDiscovered) return Access;
    Access.bSourceOwned = SelectedNode->bOwned;

    const AAIHeroesWorldNode* HQ = nullptr;
    const AAIHeroesWorldNode* Outpost = nullptr;
    int32 HQCount = 0, OutpostCount = 0;
    for (const AAIHeroesWorldNode* Node : Nodes)
    {
        if (!IsValid(Node)) return FAIHeroesSupplyAccess();
        const FString Name = Node->GetDisplayName();
        if (Node->NodeType == EAIHeroesNodeType::Headquarters && Name == TEXT("HQ"))
        {
            ++HQCount;
            HQ = Node;
        }
        if (Node->NodeType == EAIHeroesNodeType::Outpost && Name == TEXT("OUTPOST"))
        {
            ++OutpostCount;
            Outpost = Node;
        }
    }
    if (HQCount != 1 || OutpostCount != 1) return FAIHeroesSupplyAccess();
    Access.bHeadquartersOwned = HQ->bOwned;
    Access.bOutpostOwned = Outpost->bOwned;
    Access.bHeroAtHeadquarters =
        FVector::DistSquared(
            GetHeroStrategicLocation(), HQ->GetActorLocation()) < 1.0;

    // RoadVersion 2 uses the ACTUAL authoritative HQ->field link, not a
    // Euclidean shortcut or the decorative meshes. V1 stays backward-
    // compatible with its direct-distance travel rules.
    if (TravelRuleVersion == FAIHeroesTravelRules::LegacyVersion)
    {
        Access.bRoadOpen = true;
    }
    else if (TravelRuleVersion == FAIHeroesTravelRules::RoadVersion)
    {
        FAIHeroesAdventureGraph Graph;
        FString RoadError;
        if (FAIHeroesTravelRules::BuildGraph(
                BuildAdvisorSnapshot(), Graph, RoadError))
        {
            for (const FAIHeroesAdventureLink& Link : Graph.Links)
            {
                if (Link.From == FName(TEXT("HQ")) &&
                    Link.To == ParentId && Link.bEnabled)
                {
                    Access.bRoadOpen = true;
                    break;
                }
            }
        }
    }
    return Access;
}

FString AAIHeroesGameMode::GetSupplyConvoyStatus() const
{
    if (!IsValid(SelectedNode.Get()) || !Nodes.Contains(SelectedNode.Get()))
        return TEXT("CONVOY: select known FOREST/ORE while hero is at HQ.");
    const FAIHeroesSupplyAccess Access = BuildSupplyAccess();
    return FAIHeroesSupplyLines::Quote(
        Access, FieldDwellings, Gold, Wood, Ore, PlayerArmy).Summary();
}

bool AAIHeroesGameMode::IsSupplyConvoyPending() const
{
    return !PendingSupplyFingerprint.IsEmpty() &&
        PendingSupplyFingerprint == CampaignFingerprint();
}

bool AAIHeroesGameMode::CancelSupplyConvoyQuote()
{
    // Cancellation is permitted even if a quote became stale after a world
    // change. Only a fresh second X may dispatch; Esc always disarms it.
    if (PendingSupplyFingerprint.IsEmpty()) return false;
    PendingSupplyFingerprint.Empty();
    LastHint = TEXT("CONVOY CANCELED: no gold, materials, moves or troops changed.");
    SetEvent(LastHint);
    UE_LOG(LogTemp, Display,
        TEXT("[AI HEROES SUPPLY] CANCEL player_order=1 commit=0"));
    return true;
}

void AAIHeroesGameMode::RequestSupplyConvoy()
{
    if (BlockStrategicAction()) return;
    const FAIHeroesSupplyAccess Access = BuildSupplyAccess();
    const FAIHeroesSupplyQuote Quote = FAIHeroesSupplyLines::Quote(
        Access, FieldDwellings, Gold, Wood, Ore, PlayerArmy);
    if (!Quote.bReady)
    {
        PendingSupplyFingerprint.Empty();
        // A revoked OUTPOST or depleted site must also clear stale HUD advice.
        LastHint = Quote.Summary();
        SetEvent(LastHint);
        return;
    }

    const FString Fingerprint = CampaignFingerprint();
    const TCHAR* SourceName =
        Access.Site == EAIHeroesFieldSite::ForestCamp
            ? TEXT("FOREST CAMP") : TEXT("ORE AERIE");
    if (PendingSupplyFingerprint != Fingerprint)
    {
        PendingSupplyFingerprint = Fingerprint;
        LastHint = Quote.Summary();
        UE_LOG(LogTemp, Display,
            TEXT("[AI HEROES SUPPLY] QUOTE source=%s units=%d cost=%dG/%dW/%dO moves=%d player_order=0 commit=0"),
            SourceName, Quote.Units, Quote.Gold, Quote.Wood, Quote.Ore,
            Quote.MovementCost);
        SetEvent(FString::Printf(
            TEXT("CONVOY QUOTED: %s %d units for %dG %dW %dO +%d move. "
                 "Press X / CONVOY again to CONFIRM. No spend yet."),
            SourceName, Quote.Units, Quote.Gold, Quote.Wood, Quote.Ore,
            Quote.MovementCost));
        return;
    }

    // A fresh typed access snapshot is recomputed above for every action.
    // An old quote is not a recurring order and never survives a save/load.
    PendingSupplyFingerprint.Empty();
    FString Error;
    if (!FAIHeroesSupplyLines::Dispatch(
            Access, FieldDwellings, MovesLeft,
            Gold, Wood, Ore, PlayerArmy, Error))
    {
        SetEvent(Error);
        return;
    }
    InvalidateRoutePreview();
    RefreshFieldDwellingVisuals();
    LastHint = TEXT("A human-confirmed convoy delivered one pack. HQ reserve unchanged.");
    UE_LOG(LogTemp, Display,
        TEXT("[AI HEROES SUPPLY] DISPATCH source=%s units=%d cost=%dG/%dW/%dO moves=%d remaining=%d player_order=1 commit=1"),
        SourceName, Quote.Units, Quote.Gold, Quote.Wood, Quote.Ore,
        Quote.MovementCost,
        FAIHeroesFieldDwellingRules::At(FieldDwellings, Access.Site).Available);
    SetEvent(FString::Printf(
        TEXT("CONVOY ARRIVED: %d units from %s, paid %dG %dW %dO and 1 move. "
             "HQ reserves untouched."),
        Quote.Units, SourceName, Quote.Gold, Quote.Wood, Quote.Ore));
}
