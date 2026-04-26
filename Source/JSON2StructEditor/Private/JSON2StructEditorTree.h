// Copyright Epic Games, Inc. All Rights Reserved.
//Author WeChat: wujifeng_mr

#pragma once

#include "CoreMinimal.h"
#include "JSON2StructEditor.h"

namespace JSON2StructPanel
{
	TSharedPtr<FGeneratedFileTreeItem> BuildFileTreeFromDirectory(const FString& OutputDir);
	FString BuildDirectoryTreeSnapshot(const FString& OutputDir);
	void OpenFolderAndSelectFile(const FString& FilePath);
	bool OpenFileInIDE(const FString& FilePath);
}

