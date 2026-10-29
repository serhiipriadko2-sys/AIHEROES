using UnrealBuildTool;

public class AIHeroes : ModuleRules
{
    public AIHeroes(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new string[] { "Json", "Sockets" });
        PublicDependencyModuleNames.AddRange(new string[]
        {
            "Core", "CoreUObject", "Engine", "InputCore"
        });
    }
}
