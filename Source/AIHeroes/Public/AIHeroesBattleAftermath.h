#pragma once

#include "CoreMinimal.h"

struct FAIHeroesBattleRecord;

// Player-facing explanation of an already committed battle. No combat,
// reward, save, movement or campaign mutation is performed here.
struct FAIHeroesBattleAftermathView
{
    FString PlayerLine;
    bool bCountsAvailable = false;
};

class AIHEROES_API FAIHeroesBattleAftermath
{
public:
    static FAIHeroesBattleAftermathView Build(
        const FAIHeroesBattleRecord& CommittedRecord);
};
