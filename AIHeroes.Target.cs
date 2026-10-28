using UnrealBuildTool;
using System.Collections.Generic;

public class AIHeroesTarget : TargetRules
{
    public AIHeroesTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.Add("AIHeroes");
    }
}
