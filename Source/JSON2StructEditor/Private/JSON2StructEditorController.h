// Copyright Epic Games, Inc. All Rights Reserved.
//Author WeChat: wujifeng_mr

#pragma once

#include "CoreMinimal.h"
#include "JSON2StructEditor.h"
#include "Containers/Ticker.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Views/STreeView.h"

namespace JSON2StructPanel
{
	TFunction<void(const FString&)> MakeAppendLogHandler(TWeakPtr<SMultiLineEditableTextBox> LogEditableWeak, TSharedRef<FString> LogBuffer);

	TFunction<void(TSharedPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>>, const TArray<TSharedPtr<FGeneratedFileTreeItem>>&)>
	MakeExpandTreeToFirstLevelHandler();

	TFunction<void(TSharedPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>>, const FString&, const TSharedRef<FString>&)>
	MakeRefreshTreeViewSafelyHandler(
		FJSON2StructEditorModule* EditorModule,
		TArray<TSharedPtr<FGeneratedFileTreeItem>>* TreeRoots,
		TFunction<void(TSharedPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>>, const TArray<TSharedPtr<FGeneratedFileTreeItem>>&)> ExpandTreeToFirstLevel);

	TSharedRef<FTSTicker::FDelegateHandle> StartAutoRefreshTicker(
		TWeakPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>> WeakTreeView,
		TFunction<void(TSharedPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>>, const FString&, const TSharedRef<FString>&)> RefreshTreeViewSafely,
		const TSharedRef<FString>& LastTreeSnapshot,
		const FString& FixedTreeRootDir);
}

