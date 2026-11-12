// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Reel : ModuleRules
{
	public Reel(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"Reel",
			"Reel/Variant_Platforming",
			"Reel/Variant_Platforming/Animation",
			"Reel/Variant_Combat",
			"Reel/Variant_Combat/AI",
			"Reel/Variant_Combat/Animation",
			"Reel/Variant_Combat/Gameplay",
			"Reel/Variant_Combat/Interfaces",
			"Reel/Variant_Combat/UI",
			"Reel/Variant_SideScrolling",
			"Reel/Variant_SideScrolling/AI",
			"Reel/Variant_SideScrolling/Gameplay",
			"Reel/Variant_SideScrolling/Interfaces",
			"Reel/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
