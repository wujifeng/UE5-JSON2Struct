// Copyright Epic Games, Inc. All Rights Reserved.
//Author WeChat: wujifeng_mr

#pragma once

#include "CoreMinimal.h"
#include "JSON2StructEditor.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Containers/Ticker.h"

namespace JSON2StructPanel
{
	struct FLeftPanelBuildArgs
	{
		TArray<TSharedPtr<FGeneratedFileTreeItem>>* TreeRoots = nullptr;
		TSharedPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>>* OutTreeView = nullptr;
		TFunction<void(const FString&)> AppendLog;
		TFunction<void(TSharedPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>>, const FString&, const TSharedRef<FString>&)> RefreshTreeViewSafely;
		FString FixedTreeRootDir;
		TSharedRef<FString> LastTreeSnapshot;
		TSharedRef<FString> PendingPreviewPath;
		TSharedRef<bool> bSuppressNextSinglePreview;
		TSharedRef<FTSTicker::FDelegateHandle> PendingPreviewTickerHandle;
		TFunction<void(const TSharedPtr<FGeneratedFileTreeItem>&)> OpenTreeItem;
		TFunction<void(const TSharedPtr<FGeneratedFileTreeItem>&)> PreviewTreeItem;
		TFunction<void(const TSharedPtr<FGeneratedFileTreeItem>&, TSharedPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>>, const FString&, const TSharedRef<FString>&)> DeleteTreeItem;
	};

	TSharedRef<SWidget> BuildLeftPanelContentWidget(const FLeftPanelBuildArgs& Args);
	TSharedRef<SWidget> BuildLeftPanelRootWidget(const TSharedRef<SWidget>& LeftPanelContent, float LeftPanelFixedWidth);
}

