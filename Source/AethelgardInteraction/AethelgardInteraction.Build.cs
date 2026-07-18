// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AethelgardInteraction : ModuleRules
{
	public AethelgardInteraction(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine",
			"AethelgardTerrain"
		});
	}
}
