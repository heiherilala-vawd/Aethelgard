// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Aethelgard : ModuleRules
{
	public Aethelgard(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput",
			"ProceduralMeshComponent"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		PrivateIncludePaths.Add(ModuleDirectory);
		PrivateIncludePaths.Add(ModuleDirectory + "/Terrain");
		PrivateIncludePaths.Add(ModuleDirectory + "/Game");
		PrivateIncludePaths.Add(ModuleDirectory + "/Interaction");

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
