#pragma once
#include "CoreMinimal.h"

// Independent, testable battle policy. No LLM is invoked in this prototype.
struct FAIHArmy
{
    int32 Scouts = 4;
    int32 Guardians = 1;
    int32 Sparks = 0;
    int32 Count() const { return Scouts + Guardians + Sparks; }
    int32 Strength() const { return Scouts * 2 + Guardians * 5 + Sparks * 3; }
};

struct FAIHCombatReport
{
    bool bVictory = false;
    int32 OurPower = 0;
    int32 EnemyPower = 0;
    int32 Casualties = 0;
    FString Tactic;
    FString Explanation;
    FAIHArmy Survivors;
};

class FAIHCombatEngine
{
public:
    static FAIHCombatReport Resolve(
        const FAIHArmy& Army, int32 EnemyPower, int32 ProtocolRank);
};
