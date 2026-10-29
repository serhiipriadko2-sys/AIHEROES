#pragma once

#include "CoreMinimal.h"

// Deterministic visual-only sampling of the terrain profile authored in
// Tools/build_amber_relief_v5.py. Gameplay nodes, roads and save-state
// remain under their existing authoritative systems.
class AIHEROES_API FAIHeroesReliefV5
{
public:
    static double HeightCm(double Xcm,double Ycm);
    static const TCHAR* PlatformMeshName();
};