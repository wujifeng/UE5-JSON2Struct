// Copyright Epic Games, Inc. All Rights Reserved.
//Author WeChat: wujifeng_mr

#include "JSON2StructCommands.h"

#define LOCTEXT_NAMESPACE "JSON2StructCommands"

void FJSON2StructCommands::RegisterCommands()
{
	UI_COMMAND(OpenPluginWindow, "JSON2Struct", "Bring up JSON2Struct window", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
