#include "AIHeroesFogRules.h"

int32 FAIHeroesFogRules::SiteBit(FName Site)
{
    if (Site == FName(TEXT("HQ"))) return 1;
    if (Site == FName(TEXT("FOREST"))) return 2;
    if (Site == FName(TEXT("ORE"))) return 4;
    if (Site == FName(TEXT("OUTPOST"))) return 8;
    if (Site == FName(TEXT("RUINS"))) return 16;
    return 0;
}

bool FAIHeroesFogRules::IsKnown(bool bEnabled, int32 Mask, FName Site)
{
    const int32 Bit = SiteBit(Site);
    return Bit != 0 && (!bEnabled || (Mask & Bit) != 0);
}

int32 FAIHeroesFogRules::CountKnown(bool bEnabled, int32 Mask)
{
    if (!bEnabled) return 5;
    if (Mask < 0 || (Mask & ~AllMask) != 0) return 0;
    int32 Count = 0;
    for (int32 Bit = 1; Bit <= 16; Bit *= 2)
    {
        if ((Mask & Bit) != 0) ++Count;
    }
    return Count;
}

bool FAIHeroesFogRules::Validate(
    bool bEnabled, int32 Mask, FName HeroSite, FName SelectedSite,
    const TSet<FName>& OwnedSites, FString& Error)
{
    Error.Empty();
    if (!bEnabled)
    {
        // Old snapshots are represented ONLY by false + zero, and stay
        // fully visible in memory. No implicit write/migration.
        if (Mask == 0) return true;
        Error = TEXT("Legacy scouting state must have zero mask.");
        return false;
    }
    if ((Mask & InitialMask) != InitialMask ||
        (Mask & ~AllMask) != 0 || Mask < 0)
    {
        Error = TEXT("Scouting mask has missing starter sites or unknown bits.");
        return false;
    }
    if (!IsKnown(true, Mask, HeroSite) ||
        (!SelectedSite.IsNone() && !IsKnown(true, Mask, SelectedSite)))
    {
        Error = TEXT("Hero or selected site is hidden in scouting snapshot.");
        return false;
    }
    for (FName Id : OwnedSites)
    {
        if (!IsKnown(true, Mask, Id))
        {
            Error = TEXT("Owned territory cannot be hidden by a scouting snapshot.");
            return false;
        }
    }
    return true;
}

bool FAIHeroesFogRules::Reveal(
    bool bEnabled, int32 Mask, FName Visited,
    int32& OutMask, FString& Error)
{
    OutMask = Mask;
    Error.Empty();
    if (!bEnabled)
    {
        if (Mask == 0 && SiteBit(Visited) != 0) return true;
        Error = TEXT("Invalid legacy scouting state or destination.");
        return false;
    }
    if ((Mask & InitialMask) != InitialMask ||
        (Mask & ~AllMask) != 0 || Mask < 0 ||
        !IsKnown(true, Mask, Visited))
    {
        Error = TEXT("Scouting requires a valid already discovered destination.");
        return false;
    }

    int32 NewBits = 0;
    if (Visited == FName(TEXT("HQ"))) NewBits = 2 | 4;
    else if (Visited == FName(TEXT("FOREST"))) NewBits = 8;
    else if (Visited == FName(TEXT("ORE"))) NewBits = 16;
    else if (Visited == FName(TEXT("OUTPOST"))) NewBits = 16;
    else if (Visited == FName(TEXT("RUINS"))) NewBits = 8;
    else
    {
        Error = TEXT("Unknown visit target.");
        return false;
    }
    OutMask = Mask | NewBits;
    return true;
}
