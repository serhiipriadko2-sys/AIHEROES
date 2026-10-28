#include "AIHeroesCampaignRules.h"

#include "AIHeroesCampaignSave.h"
#include "AIHeroesBattleRecord.h"

namespace
{
struct FNodeDefinition
{
    const TCHAR* Id;
    EAIHeroesNodeType Type;
    const TCHAR* DefenderId;
};

struct FUnitDefinition
{
    const TCHAR* Id;
    int32 Health;
    int32 Attack;
    int32 Defense;
    int32 Initiative;
};

const FNodeDefinition KnownNodes[] = {
    {TEXT("HQ"), EAIHeroesNodeType::Headquarters, nullptr},
    {TEXT("FOREST"), EAIHeroesNodeType::ForestMine, TEXT("Wolf")},
    {TEXT("ORE"), EAIHeroesNodeType::OreMine, TEXT("Bandit")},
    {TEXT("OUTPOST"), EAIHeroesNodeType::Outpost, TEXT("Raider")},
    {TEXT("RUINS"), EAIHeroesNodeType::Ruins, TEXT("Guardian")}
};

const FUnitDefinition KnownUnits[] = {
    {TEXT("Militia"), 12, 4, 2, 3},
    {TEXT("Scout"), 8, 6, 1, 7},
    {TEXT("Wolf"), 9, 3, 1, 8},
    {TEXT("Bandit"), 10, 4, 2, 4},
    {TEXT("Raider"), 11, 5, 2, 5},
    {TEXT("Guardian"), 24, 7, 4, 2}
};

bool Fail(FString& Error, const TCHAR* Reason)
{
    Error = Reason;
    return false;
}

bool ValidateStack(const FAIHeroesArmyStack& Stack, const FName ExpectedId)
{
    if (Stack.UnitId != ExpectedId ||
        Stack.TotalHealth < 0 || Stack.TotalHealth > 500000)
    {
        return false;
    }

    for (const FUnitDefinition& Unit : KnownUnits)
    {
        if (Stack.UnitId == FName(Unit.Id))
        {
            return Stack.UnitHealth == Unit.Health &&
                Stack.Attack == Unit.Attack &&
                Stack.Defense == Unit.Defense &&
                Stack.Initiative == Unit.Initiative;
        }
    }
    return false;
}
}

bool FAIHeroesCampaignRules::Validate(const UAIHeroesCampaignSave& Save, FString& Error)
{
    Error.Empty();

    if (Save.SchemaVersion != UAIHeroesCampaignSave::CurrentSchema)
    {
        return Fail(Error, TEXT("Unsupported save version."));
    }
    if (Save.Day < 1 || Save.Day > 1000000 ||
        Save.SaveSequence < 0 || Save.SaveSequence > 1000000 ||
        Save.MovesLeft < 0 || Save.MovesLeft > 4 ||
        Save.ProtocolTier < 0 || Save.ProtocolTier > 3 ||
        Save.ModelSlotTier < 0 || Save.ModelSlotTier > 2 ||
        Save.DataFragments < 0 || Save.DataFragments > 500000000)
    {
        return Fail(Error, TEXT("Invalid turn or protocol progress."));
    }
    if (Save.Gold < 0 || Save.Gold > 500000000 ||
        Save.Wood < 0 || Save.Wood > 500000000 ||
        Save.Ore < 0 || Save.Ore > 500000000)
    {
        return Fail(Error, TEXT("Invalid resource totals."));
    }
    if (Save.PlayerArmy.Num() > 2)
    {
        return Fail(Error, TEXT("Army stack limit exceeded."));
    }

    TSet<FName> SeenUnits;
    for (const FAIHeroesArmyStack& Stack : Save.PlayerArmy)
    {
        if (Stack.UnitId != FName(TEXT("Militia")) &&
            Stack.UnitId != FName(TEXT("Scout")))
        {
            return Fail(Error, TEXT("Unknown friendly unit type."));
        }
        if (SeenUnits.Contains(Stack.UnitId) ||
            !ValidateStack(Stack, Stack.UnitId))
        {
            return Fail(Error, TEXT("Invalid or repeated army stack."));
        }
        SeenUnits.Add(Stack.UnitId);
    }

    if (Save.NodeStates.Num() != UE_ARRAY_COUNT(KnownNodes))
    {
        return Fail(Error, TEXT("Wrong map layout for this save version."));
    }

    TSet<FName> SeenNodes;
    bool bHeroAtOwnedNode = false;
    for (const FAIHeroesSavedNode& Node : Save.NodeStates)
    {
        if (Node.NodeId.IsNone() || SeenNodes.Contains(Node.NodeId))
        {
            return Fail(Error, TEXT("Duplicate or missing map node."));
        }
        SeenNodes.Add(Node.NodeId);

        const FNodeDefinition* Def = nullptr;
        for (const FNodeDefinition& Candidate : KnownNodes)
        {
            if (Node.NodeId == FName(Candidate.Id))
            {
                Def = &Candidate;
                break;
            }
        }
        if (!Def || Node.Type != Def->Type || Node.Defenders.Num() > 2)
        {
            return Fail(Error, TEXT("Unknown or incompatible map node."));
        }
        if (Node.NodeId == FName(TEXT("HQ")) &&
            (!Node.bOwned || !Node.Defenders.IsEmpty()))
        {
            return Fail(Error, TEXT("Headquarters must remain owned."));
        }

        for (const FAIHeroesArmyStack& Defender : Node.Defenders)
        {
            if (!Def->DefenderId ||
                !ValidateStack(Defender, FName(Def->DefenderId)) ||
                (Node.bOwned && Defender.IsAlive()))
            {
                return Fail(Error, TEXT("Invalid garrison or captured node."));
            }
        }

        if (Node.NodeId == Save.HeroNodeId && Node.bOwned)
        {
            bHeroAtOwnedNode = true;
        }
    }

    if (!bHeroAtOwnedNode)
    {
        return Fail(Error, TEXT("Hero position is not an owned map node."));
    }
    if (!Save.SelectedNodeId.IsNone() && !SeenNodes.Contains(Save.SelectedNodeId))
    {
        return Fail(Error, TEXT("Saved selection does not exist."));
    }

    if (Save.BattleJournal.Num() > 16 || Save.LastCombatStep.Len() > 300)
    {
        return Fail(Error, TEXT("Saved journal is too large."));
    }
    for (const FString& Line : Save.BattleJournal)
    {
        if (Line.Len() > 300)
        {
            return Fail(Error, TEXT("Saved journal entry is too large."));
        }
    }

    if (!FAIHeroesBattleRecordRules::Verify(Save.LastBattleRecord, Error))
    {
        return false;
    }
    if (!Save.LastBattleRecord.NodeId.IsNone())
    {
        if (Save.LastBattleRecord.Day > Save.Day ||
            !SeenNodes.Contains(Save.LastBattleRecord.NodeId))
        {
            return Fail(Error, TEXT("Replay references an unknown or future battle."));
        }
        const FAIHeroesBattleRecord& Replay = Save.LastBattleRecord;
        for (const FAIHeroesArmyStack& Unit : Replay.PlayerBefore)
        {
            if ((Unit.UnitId != FName(TEXT("Militia")) &&
                 Unit.UnitId != FName(TEXT("Scout"))) ||
                !ValidateStack(Unit, Unit.UnitId))
            {
                return Fail(Error, TEXT("Replay contains an impossible friendly unit."));
            }
        }
        const FAIHeroesSavedNode* BattleNode = Save.NodeStates.FindByPredicate(
            [&Replay](const FAIHeroesSavedNode& Node)
            {
                return Node.NodeId == Replay.NodeId;
            });
        if (!BattleNode || BattleNode->Type == EAIHeroesNodeType::Headquarters)
        {
            return Fail(Error, TEXT("Replay references a non-combat node."));
        }
        const TCHAR* ExpectedGuard = nullptr;
        switch (BattleNode->Type)
        {
            case EAIHeroesNodeType::ForestMine: ExpectedGuard = TEXT("Wolf"); break;
            case EAIHeroesNodeType::OreMine: ExpectedGuard = TEXT("Bandit"); break;
            case EAIHeroesNodeType::Outpost: ExpectedGuard = TEXT("Raider"); break;
            case EAIHeroesNodeType::Ruins: ExpectedGuard = TEXT("Guardian"); break;
            default: break;
        }
        for (const FAIHeroesArmyStack& Guard : Replay.EnemyBefore)
        {
            if (!ExpectedGuard || !ValidateStack(Guard, FName(ExpectedGuard)))
            {
                return Fail(Error, TEXT("Replay contains an impossible neutral defender."));
            }
        }
    }

    return true;
}
