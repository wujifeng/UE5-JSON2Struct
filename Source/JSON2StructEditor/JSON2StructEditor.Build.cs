// Copyright Epic Games, Inc. All Rights Reserved.
//Author WeChat: wujifeng_mr

using UnrealBuildTool;

public class JSON2StructEditor : ModuleRules
{
	public JSON2StructEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"JSON2StructRuntime",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Projects",
				"InputCore",
				"EditorFramework",
				"UnrealEd",
				"LevelEditor",
				"ToolMenus",
				"Slate",
				"SlateCore",
				"DesktopPlatform",
				"ContentBrowser",
				"SourceCodeAccess",
				"AssetRegistry",
				"HTTP",
				"Json",
				"JsonUtilities",
				"UMG",
				"UMGEditor",
				"BlueprintGraph",
			}
		);
	}
}
