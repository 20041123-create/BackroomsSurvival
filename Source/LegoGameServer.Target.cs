// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class LegoGameServerTarget : TargetRules
{
	public LegoGameServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
		DefaultBuildSettings = BuildSettingsVersion.V5;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_4;
		ExtraModuleNames.Add("LegoGame");
	}
}
