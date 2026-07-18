// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AethelgardTerrain : ModuleRules
{
	public AethelgardTerrain(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine",
			"ProceduralMeshComponent"
		});

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] {
				"AssetTools", "UnrealEd"
			});
		}
	}
}
