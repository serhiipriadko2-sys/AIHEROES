#pragma once

#include "CoreMinimal.h"

class UAIHeroesCampaignSave;

// Pure validation: a damaged/incompatible save cannot partially mutate a running game.
class FAIHeroesCampaignRules
{
public:
    static bool Validate(const UAIHeroesCampaignSave& Save, FString& Error);
};
