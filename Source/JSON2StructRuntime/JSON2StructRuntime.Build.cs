// Copyright Epic Games, Inc. All Rights Reserved.
//Author WeChat: wujifeng_mr

using UnrealBuildTool;

public class JSON2StructRuntime : ModuleRules
{
	public JSON2StructRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"HTTP",
				"Json",
				"JsonUtilities",
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
			}
		);
	}
}
