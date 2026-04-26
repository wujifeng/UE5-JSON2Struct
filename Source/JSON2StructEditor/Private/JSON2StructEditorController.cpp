// Copyright Epic Games, Inc. All Rights Reserved.
//Author WeChat: wujifeng_mr

#include "JSON2StructEditorController.h"
#include "JSON2StructEditorTree.h"
#include "Containers/Ticker.h"
#include "Misc/DateTime.h"

namespace JSON2StructPanel
{
	TFunction<void(const FString&)> MakeAppendLogHandler(TWeakPtr<SMultiLineEditableTextBox> LogEditableWeak, TSharedRef<FString> LogBuffer)
	{
		return [LogEditableWeak, LogBuffer](const FString& Line)
		{
			if (TSharedPtr<SMultiLineEditableTextBox> P = LogEditableWeak.Pin())
			{
				const FDateTime Now = FDateTime::Now();
				const FString Timestamp = FString::Printf(
					TEXT("%04d-%02d-%02d %02d:%02d:%02d.%03d"),
					Now.GetYear(), Now.GetMonth(), Now.GetDay(),
					Now.GetHour(), Now.GetMinute(), Now.GetSecond(), Now.GetMillisecond());
				const FString NewLine = FString::Printf(TEXT("[%s] %s\n"), *Timestamp, *Line);
				(*LogBuffer) += NewLine;
				P->SetText(FText::FromString(*LogBuffer));

				TArray<FString> LogLines;
				LogBuffer->ParseIntoArrayLines(LogLines, true);
				const int32 LastLineIndex = FMath::Max(0, LogLines.Num() - 1);
				P->GoTo(FTextLocation(LastLineIndex, 0));
			}
		};
	}

	TFunction<void(TSharedPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>>, const TArray<TSharedPtr<FGeneratedFileTreeItem>>&)>
	MakeExpandTreeToFirstLevelHandler()
	{
		return [](TSharedPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>> InTreeView, const TArray<TSharedPtr<FGeneratedFileTreeItem>>& Roots)
		{
			if (!InTreeView.IsValid())
			{
				return;
			}
			for (const TSharedPtr<FGeneratedFileTreeItem>& RootNode : Roots)
			{
				if (!RootNode.IsValid())
				{
					continue;
				}
				InTreeView->SetItemExpansion(RootNode, true);
				for (const TSharedPtr<FGeneratedFileTreeItem>& Child : RootNode->Children)
				{
					if (Child.IsValid() && Child->bIsFolder)
					{
						InTreeView->SetItemExpansion(Child, false);
					}
				}
			}
		};
	}

	TFunction<void(TSharedPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>>, const FString&, const TSharedRef<FString>&)>
	MakeRefreshTreeViewSafelyHandler(
		FJSON2StructEditorModule* EditorModule,
		TArray<TSharedPtr<FGeneratedFileTreeItem>>* TreeRoots,
		TFunction<void(TSharedPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>>, const TArray<TSharedPtr<FGeneratedFileTreeItem>>&)> ExpandTreeToFirstLevel)
	{
		return [EditorModule, TreeRoots, ExpandTreeToFirstLevel](TSharedPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>> InTreeView, const FString& OutputDir, const TSharedRef<FString>& LastTreeSnapshot)
		{
			const FString NewSnapshot = JSON2StructPanel::BuildDirectoryTreeSnapshot(OutputDir);
			if (NewSnapshot == *LastTreeSnapshot)
			{
				return;
			}
			*LastTreeSnapshot = NewSnapshot;

			if (!EditorModule || !TreeRoots)
			{
				return;
			}

			EditorModule->RefreshGeneratedCodeTree(OutputDir);
			TWeakPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>> WeakTreeView = InTreeView;
			FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([WeakTreeView, TreeRoots, ExpandTreeToFirstLevel](float)
			{
				if (TSharedPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>> TV = WeakTreeView.Pin())
				{
					TV->RequestTreeRefresh();
					ExpandTreeToFirstLevel(TV, *TreeRoots);
				}
				return false;
			}));
		};
	}

	TSharedRef<FTSTicker::FDelegateHandle> StartAutoRefreshTicker(
		TWeakPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>> WeakTreeView,
		TFunction<void(TSharedPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>>, const FString&, const TSharedRef<FString>&)> RefreshTreeViewSafely,
		const TSharedRef<FString>& LastTreeSnapshot,
		const FString& FixedTreeRootDir)
	{
		TSharedRef<FTSTicker::FDelegateHandle> AutoRefreshTickerHandle = MakeShared<FTSTicker::FDelegateHandle>();
		*AutoRefreshTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateLambda([WeakTreeView, RefreshTreeViewSafely, LastTreeSnapshot, FixedTreeRootDir](float)
			{
				if (TSharedPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>> TV = WeakTreeView.Pin())
				{
					RefreshTreeViewSafely(TV, FixedTreeRootDir, LastTreeSnapshot);
					return true;
				}
				return false;
			}),
			1.0f);
		return AutoRefreshTickerHandle;
	}
}

