#include "AIHeroesGameMode.h"

#if !UE_BUILD_SHIPPING
#include "AIHeroesWorldNode.h"
#endif

// Development-only, command-line opt-in, disposable live-window fixture.
// NOT campaign startup, NOT a reward, NOT SaveGame or player authorization.
// The real game requires clearing and claiming both sites through gameplay.
void AAIHeroesGameMode::BeginSupplySmoke()
{
#if !UE_BUILD_SHIPPING
    AAIHeroesWorldNode* Forest = nullptr;
    AAIHeroesWorldNode* Outpost = nullptr;
    for (AAIHeroesWorldNode* N : Nodes)
    {
        if (!IsValid(N)) continue;
        if (N->GetDisplayName() == TEXT("FOREST") &&
            N->NodeType == EAIHeroesNodeType::ForestMine)
            Forest = N;
        else if (N->GetDisplayName() == TEXT("OUTPOST") &&
            N->NodeType == EAIHeroesNodeType::Outpost)
            Outpost = N;
    }
    if (!Forest || !Outpost)
    {
        UE_LOG(LogTemp, Error, TEXT("[AI HEROES SUPPLY SMOKE] FAIL: missing nodes"));
        return;
    }
    Forest->Defenders.Reset();
    Outpost->Defenders.Reset();
    Forest->SetOwned(true);
    Outpost->SetOwned(true);
    FieldDwellings.Forest.bClaimed = true;
    FieldDwellings.Forest.Available = 10;
    SelectedNode = Forest; // Hero deliberately stays at HQ.
    RevealAfterArrival(FName(TEXT("FOREST")));
    RefreshFogVisibility();
    RefreshFieldDwellingVisuals();
    RefreshAdventureRoads();
    PendingSupplyFingerprint.Empty();
    const FAIHeroesSupplyAccess Access = BuildSupplyAccess();
    const auto Quote = FAIHeroesSupplyLines::Quote(
        Access, FieldDwellings, Gold, Wood, Ore, PlayerArmy);
    if (!Quote.bReady)
    {
        UE_LOG(LogTemp, Error, TEXT("[AI HEROES SUPPLY SMOKE] FAIL: %s"),
            *Quote.Reason);
        return;
    }
    LastHint = TEXT("DEVELOPMENT QA FIXTURE: cleared FOREST and OUTPOST; hero at HQ.");
    UE_LOG(LogTemp, Display,
        TEXT("[AI HEROES SUPPLY SMOKE] READY source=FOREST stock=%d gold=%d moves=%d no_save=1"),
        FieldDwellings.Forest.Available, Gold, MovesLeft);
    SetEvent(TEXT("QA-ONLY SUPPLY: hero at HQ, source FOREST selected. "
                  "Click CONVOY X once to quote, again to confirm."));
#endif
}
