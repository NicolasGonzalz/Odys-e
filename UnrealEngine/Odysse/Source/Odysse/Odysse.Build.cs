// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Odysse : ModuleRules
{
	public Odysse(ReadOnlyTargetRules Target) : base(Target)
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
			"Slate",
			"Landscape" // pinceau qui perce les entrées de grotte (World/OdysseeHoleBrush.h)
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"Odysse",
			"Odysse/Variant_Horror",
			"Odysse/Variant_Horror/UI",
			"Odysse/Variant_Shooter",
			"Odysse/Variant_Shooter/AI",
			"Odysse/Variant_Shooter/UI",
			"Odysse/Variant_Shooter/Weapons"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
