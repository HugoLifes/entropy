// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class entropy : ModuleRules
{
	public entropy(ReadOnlyTargetRules Target) : base(Target)
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
			"Niagara"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		// Solo el editor: TemporalFX crea y guarda el material M_TemporalGhost la primera vez.
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.Add("AssetRegistry");
		}

		PublicIncludePaths.AddRange(new string[] {
			"entropy",
			"entropy/Variant_Platforming",
			"entropy/Variant_Platforming/Animation",
			"entropy/Variant_Combat",
			"entropy/Variant_Combat/AI",
			"entropy/Variant_Combat/Animation",
			"entropy/Variant_Combat/Gameplay",
			"entropy/Variant_Combat/Interfaces",
			"entropy/Variant_Combat/UI",
			"entropy/Variant_SideScrolling",
			"entropy/Variant_SideScrolling/AI",
			"entropy/Variant_SideScrolling/Gameplay",
			"entropy/Variant_SideScrolling/Interfaces",
			"entropy/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
