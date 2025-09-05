// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class AutoPropertyEditorUIEditor : ModuleRules
{
	public AutoPropertyEditorUIEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
                "AutoPropertyEditorUI",
				"Core",
				"CoreUObject",
				"Engine",
				"UnrealEd",
				"SlateCore",
                "AssetRegistry",
                "ContentBrowser"
            });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
