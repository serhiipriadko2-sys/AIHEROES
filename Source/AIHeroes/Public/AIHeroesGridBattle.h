#pragma once

#include "CoreMinimal.h"
#include "AIHeroesBattleEngine.h"

enum class EAIHeroesGridActionKind : uint8
{
    Move,
    Attack,
    Wait
};

// A receipt of ONE action already calculated by the pure GridRulesV2 engine.
// Path excludes FromCell and includes each traversed cell through ToCell.
struct FAIHeroesGridAction
{
    EAIHeroesGridActionKind Kind = EAIHeroesGridActionKind::Wait;
    int32 Round = 0;
    bool bPlayerAttacker = false;
    int32 AttackerStackIndex = INDEX_NONE;
    int32 TargetStackIndex = INDEX_NONE;
    FName AttackerId = NAME_None;
    FName TargetId = NAME_None;
    FIntPoint FromCell = FIntPoint::ZeroValue;
    FIntPoint ToCell = FIntPoint::ZeroValue;
    TArray<FIntPoint> Path;
    int32 Damage = 0;
    int32 TargetHealthBefore = 0;
    int32 TargetHealthAfter = 0;
    FString Reason;

    FString ToLogLine() const;
};

enum class EAIHeroesGridStepResult : uint8
{
    ActionProduced,
    Completed,
    Error
};

struct FAIHeroesGridSession
{
    TArray<FAIHeroesArmyStack> PlayerArmy;
    TArray<FAIHeroesArmyStack> EnemyArmy;
    TArray<FIntPoint> PlayerCells;
    TArray<FIntPoint> EnemyCells;
    int32 Round = 0;
    int32 ProtocolTier = 0;
    TArray<FAIHeroesGridAction> Actions;
    // Public read-back only; engine rejects caller changes to this mirror.
    bool bCompleted = false;

private:
    friend class FAIHeroesGridBattle;

    struct FTurn
    {
        bool bPlayer = false;
        int32 StackIndex = INDEX_NONE;
        int32 Initiative = 0;
    };

    TArray<FTurn> TurnOrder;
    int32 TurnCursor = 0;
    int32 FrozenProtocolTier = 0;
    bool bAfterMove = false;
    bool bStarted = false;
    bool bInvalid = false;
    bool bTerminal = false;
};

// No actors, clocks, randomness, input commands, filesystem or network access.
// Begin stores initial state only. Step calculates at most one new action.
class AIHEROES_API FAIHeroesGridBattle
{
public:
    static constexpr int32 RuleVersion = 2;
    static constexpr int32 Width = 8;
    static constexpr int32 Height = 6;
    static constexpr int32 MaxRounds = 30;
    static constexpr int32 MaxActions = 240;

    static bool Begin(
        const TArray<FAIHeroesArmyStack>& PlayerArmy,
        const TArray<FAIHeroesArmyStack>& EnemyArmy,
        int32 ProtocolTier,
        FAIHeroesGridSession& OutSession,
        FString& Error);

    static EAIHeroesGridStepResult Step(
        FAIHeroesGridSession& Session,
        FAIHeroesGridAction& OutAction,
        FString& Error);

    // An incomplete/invalid session never reports a victory or final receipt.
    static FAIHeroesBattleOutcome GetOutcome(const FAIHeroesGridSession& Session);

    static FAIHeroesBattleOutcome RunToEnd(
        const TArray<FAIHeroesArmyStack>& PlayerArmy,
        const TArray<FAIHeroesArmyStack>& EnemyArmy,
        int32 ProtocolTier);

private:
    static bool HasReachedTerminalState(const FAIHeroesGridSession& Session);
};
