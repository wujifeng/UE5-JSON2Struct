// Copyright Epic Games, Inc. All Rights Reserved.
//Author WeChat: wujifeng_mr

#pragma once

#include "CoreMinimal.h"
#include "JSON2StructEditor.h"
#include "JSON2StructSettings.h"

namespace JSON2StructPanel
{
	bool GenerateStructureToFolder(
		const FString& JsonText,
		const FString& OutputDir,
		bool bMergeWithExisting,
		const FString& RequestUrl,
		const FString& RequestMethod,
		TArray<FJSON2StructGeneratedItem>& OutGeneratedItems,
		TFunction<void(const FString&)> AppendLog);
}

