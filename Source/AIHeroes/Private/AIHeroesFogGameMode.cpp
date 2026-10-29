#include "AIHeroesGameMode.h"

#include "AIHeroesWorldNode.h"

FString AAIHeroesGameMode::GetScoutingStatus() const
{
    const int32 Known = FAIHeroesFogRules::CountKnown(bFogEnabled, FogKnownMask);
    return bFogEnabled
        ? FString::Printf(TEXT("SCOUTING %d/5 | Visit a known site to reveal adjacent territory."), Known)
        : TEXT("SCOUTING 5/5 | Legacy map knowledge preserved.");
}

void AAIHeroesGameMode::RefreshFogVisibility()
{
    // Actors and their colliders cannot reveal or select hidden sites.
    // This affects presentation only; visibility logic is enforced again
    // by selection, travel and save validation.
    for (AAIHeroesWorldNode* Node : Nodes)
    {
        if (!IsValid(Node)) continue;
        const bool bKnown = IsNodeDiscovered(FName(*Node->GetDisplayName()));
        Node->SetActorHiddenInGame(!bKnown);
        Node->SetActorEnableCollision(bKnown);
    }
    RefreshFieldDwellingVisuals(); // Never reveal a hidden sub-site.
}

void AAIHeroesGameMode::RevealAfterArrival(FName ArrivedSite)
{
    if (!bFogEnabled) return;
    int32 ProposedMask = FogKnownMask;
    FString Error;
    if (!FAIHeroesFogRules::Reveal(
            true, FogKnownMask, ArrivedSite, ProposedMask, Error))
    {
        UE_LOG(LogTemp, Error, TEXT("[AI HEROES SCOUT] invalid reveal: %s"), *Error);
        return;
    }
    if (ProposedMask == FogKnownMask) return;

    const int32 PreviousMask = FogKnownMask;
    FogKnownMask = ProposedMask;
    InvalidateRoutePreview();
    RefreshFogVisibility();
    RefreshAdventureRoads();

    FString NewNames;
    for (const TCHAR* Name : {
        TEXT("HQ"), TEXT("FOREST"), TEXT("ORE"), TEXT("OUTPOST"), TEXT("RUINS")})
    {
        const FName Id(Name);
        const int32 Bit = FAIHeroesFogRules::SiteBit(Id);
        if ((FogKnownMask & Bit) != 0 && (PreviousMask & Bit) == 0)
        {
            if (!NewNames.IsEmpty()) NewNames += TEXT(", ");
            NewNames += Name;
        }
    }
    LastHint = FString::Printf(
        TEXT("SCOUTING: discovered %s. %d/5 map sites known."),
        *NewNames, FAIHeroesFogRules::CountKnown(true, FogKnownMask));
    UE_LOG(LogTemp, Display,
        TEXT("[AI HEROES SCOUT] discovered=%s known=%d/5 no_order=1"),
        *NewNames, FAIHeroesFogRules::CountKnown(true, FogKnownMask));
}
