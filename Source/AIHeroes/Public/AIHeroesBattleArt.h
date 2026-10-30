#pragma once

#include "CoreMinimal.h"
#include "AIHeroesHUDLayout.h"

class AActor;
class AStaticMeshActor;
struct FAIHeroesGridAction;

// Visual-only mapping. No combat rules, damage, save data or campaign authority.
class AIHEROES_API FAIHeroesBattleArt
{
public:
    static FString MeshPath(FName Unit);
    static FString PortraitPath(FName Unit);
    static FString UnitName(FName Unit);
    static FString Role(FName Unit);
    static FString GroupCode(bool bPlayer, int32 Index);
    static FString ActionCaption(const FAIHeroesGridAction& Action);
    static FString ActionReason(const FAIHeroesGridAction& Action);
    static bool Apply(AStaticMeshActor& Body, FName Unit, bool bPlayer);
    static bool HasUnitMesh(const AStaticMeshActor& Body);
    static void TeamMark(AStaticMeshActor& Body, bool bPlayer, bool bHit = false);
    static void DecorateArena(AActor& Stage);
    static FAIHeroesHUDActionRect RosterRect(const FVector2D& Viewport, bool bPlayer, int32 Index);
};
