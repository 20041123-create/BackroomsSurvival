using UnrealBuildTool;

public class RandomRoomGeneration : ModuleRules
{
	public RandomRoomGeneration(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "GameplayTags" });
	}
}
