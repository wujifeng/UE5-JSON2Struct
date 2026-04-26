// Copyright Epic Games, Inc. All Rights Reserved.
//Author WeChat: wujifeng_mr

#pragma once

#include "Framework/Commands/Commands.h"
#include "JSON2StructStyle.h"

class FJSON2StructCommands : public TCommands<FJSON2StructCommands>
{
public:
	FJSON2StructCommands()
		: TCommands<FJSON2StructCommands>(TEXT("JSON2Struct"), NSLOCTEXT("Contexts", "JSON2Struct", "JSON2Struct Plugin"), NAME_None, FJSON2StructStyle::GetStyleSetName())
	{}
	virtual void RegisterCommands() override;
public:
	TSharedPtr<FUICommandInfo> OpenPluginWindow;
};
