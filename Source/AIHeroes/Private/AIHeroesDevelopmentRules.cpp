#include "AIHeroesDevelopmentRules.h"
#include "AIHeroesBaseRules.h"

namespace
{
bool ValidResource(int32 Value)
{
    return Value >= 0 && Value <= FAIHeroesBaseRules::MaxResource;
}
}

FAIHeroesDevelopmentOffer FAIHeroesDevelopmentRules::Protocol(int32 Tier)
{
    switch (Tier)
    {
        case 0: return {TEXT("THREAT ANALYSIS"), 80, 20, 15, 0, true};
        case 1: return {TEXT("MEMORY LEDGER"), 150, 35, 30, 0, true};
        case 2: return {TEXT("TACTICAL PLANNER"), 230, 50, 45, 0, true};
        default: return {};
    }
}

FAIHeroesDevelopmentOffer FAIHeroesDevelopmentRules::ModelSlot(int32 Tier)
{
    switch (Tier)
    {
        case 0: return {TEXT("3B MODEL SLOT"), 60, 0, 0, 2, true};
        case 1: return {TEXT("8B MODEL SLOT"), 120, 0, 0, 6, true};
        default: return {};
    }
}

bool FAIHeroesDevelopmentRules::TryProtocol(int32& Tier, int32& Gold,
    int32& Wood, int32& Ore, FString& Error)
{
    Error.Empty();
    if (Tier < 0 || Tier > 3 || !ValidResource(Gold) ||
        !ValidResource(Wood) || !ValidResource(Ore))
    {
        Error = TEXT("PROTOCOL BLOCKED: invalid progress or resources. Nothing spent.");
        return false;
    }
    const auto Cost = Protocol(Tier);
    if (!Cost.bAvailable)
    {
        Error = TEXT("All prototype protocols installed. Real LLM adapter is NOT connected.");
        return false;
    }
    if (Gold < Cost.Gold || Wood < Cost.Wood || Ore < Cost.Ore)
    {
        Error = FString::Printf(TEXT("Next protocol needs %d wood, %d ore, %d gold."),
            Cost.Wood, Cost.Ore, Cost.Gold);
        return false;
    }
    Gold -= Cost.Gold;
    Wood -= Cost.Wood;
    Ore -= Cost.Ore;
    ++Tier;
    return true;
}

bool FAIHeroesDevelopmentRules::TryModelSlot(int32& Tier, int32& Gold,
    int32& Data, FString& Error)
{
    Error.Empty();
    if (Tier < 0 || Tier > 2 || !ValidResource(Gold) || !ValidResource(Data))
    {
        Error = TEXT("MODEL SLOT BLOCKED: invalid progress or resources. Nothing spent.");
        return false;
    }
    const auto Cost = ModelSlot(Tier);
    if (!Cost.bAvailable)
    {
        Error = TEXT("8B is the highest prototype MODEL SLOT. No model weights loaded.");
        return false;
    }
    if (Gold < Cost.Gold || Data < Cost.Data)
    {
        Error = FString::Printf(TEXT("Need %d gold + %d DATA for the next model SLOT; no weights are installed."),
            Cost.Gold, Cost.Data);
        return false;
    }
    Gold -= Cost.Gold;
    Data -= Cost.Data;
    ++Tier;
    return true;
}
