// Copyright Epic Games, Inc. All Rights Reserved.
//Author WeChat: wujifeng_mr

#pragma once

#include "Templates/Function.h"

namespace JSON2StructPanel
{
	void ShowFilePreviewWindow(const FString& FilePath, TFunction<void(const FString&)> AppendLog);
}

