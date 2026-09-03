using UnrealBuildTool;

public class RandomRoomGenerationRuntime : ModuleRules
{
	public RandomRoomGenerationRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "GameplayTags", "NavigationSystem", "RandomRoomGeneration" });
	}
}
