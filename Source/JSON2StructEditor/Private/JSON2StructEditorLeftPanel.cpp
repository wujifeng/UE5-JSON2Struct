// Copyright Epic Games, Inc. All Rights Reserved.
//Author WeChat: wujifeng_mr

#include "JSON2StructEditorLeftPanel.h"
#include "JSON2StructEditorActions.h"
#include "JSON2StructEditorPreview.h"
#include "Styling/AppStyle.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SScrollBar.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/STreeView.h"

#define LOCTEXT_NAMESPACE "JSON2Struct"

namespace JSON2StructPanel
{
	TSharedRef<SWidget> BuildLeftPanelContentWidget(const FLeftPanelBuildArgs& Args)
	{
		TSharedPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>> TreeView;
		TSharedRef<TWeakPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>>> TreeViewWeakRef =
			MakeShared<TWeakPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>>>();
		auto BuildContextMenuWidget = [Args, TreeViewWeakRef]() -> TSharedPtr<SWidget>
		{
			const TSharedPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>> PinnedTreeView = TreeViewWeakRef->Pin();
			if (!PinnedTreeView.IsValid())
			{
				return TSharedPtr<SWidget>();
			}
			TArray<TSharedPtr<FGeneratedFileTreeItem>> SelectedItems = PinnedTreeView->GetSelectedItems();
			if (SelectedItems.Num() == 0 || !SelectedItems[0].IsValid())
			{
				return TSharedPtr<SWidget>();
			}
			const TSharedPtr<FGeneratedFileTreeItem> Selected = SelectedItems[0];

			FMenuBuilder MenuBuilder(true, nullptr);
			if (Selected->bIsFolder)
			{
				MenuBuilder.BeginSection("FolderActions", LOCTEXT("FolderActionsHeader", "目录操作"));
				MenuBuilder.AddMenuEntry(
					LOCTEXT("TreeContextDeleteOnly", "删除"),
					LOCTEXT("TreeContextDeleteOnlyTooltip", "删除该接口目录"),
					FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Delete"),
					FUIAction(FExecuteAction::CreateLambda([Selected, Args, TreeViewWeakRef]()
					{
						const TSharedPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>> DeleteTreeView = TreeViewWeakRef->Pin();
						Args.DeleteTreeItem(Selected, DeleteTreeView, Args.FixedTreeRootDir, Args.LastTreeSnapshot);
					})));
				MenuBuilder.EndSection();
			}
			else
			{
				MenuBuilder.BeginSection("FileActions", LOCTEXT("FileActionsHeader", "文件操作"));
				MenuBuilder.AddMenuEntry(
					LOCTEXT("TreeContextPreview", "预览"),
					LOCTEXT("TreeContextPreviewTooltip", "预览文件内容"),
					FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Visible"),
					FUIAction(FExecuteAction::CreateLambda([Selected, Args]() { Args.PreviewTreeItem(Selected); })));
				MenuBuilder.AddMenuEntry(
					LOCTEXT("TreeContextOpen", "打开"),
					LOCTEXT("TreeContextOpenTooltip", "在 IDE 或资源管理器中打开"),
					FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.FolderOpen"),
					FUIAction(FExecuteAction::CreateLambda([Selected, Args]() { Args.OpenTreeItem(Selected); })));
				MenuBuilder.EndSection();
			}
			return MenuBuilder.MakeWidget();
		};

		TSharedRef<SVerticalBox> LeftPanelContent = SNew(SVerticalBox);
		LeftPanelContent->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0, 0, 0, 6))
		[
			SNew(STextBlock).Text(LOCTEXT("TreeHeader", "Generated Directory"))
		];
		LeftPanelContent->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0, 0, 0, 6))
		[
			SNew(STextBlock)
			.AutoWrapText(true)
			.Text(LOCTEXT("TreeInteractionHint", "左键：文件夹展开/收起；右键：显示菜单（删除/打开/预览）。"))
		];
		LeftPanelContent->AddSlot()
		.AutoHeight()
		.Padding(FMargin(0, 0, 0, 6))
		[
			SNew(SBox)
			.MinDesiredWidth(116.0f)
			.MinDesiredHeight(28.0f)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.ToolTipText(LOCTEXT("FixRegistryButtonTip", "扫描 Generated 下接口目录并重建 JSON2StructRegistry.txt 映射。"))
				.OnClicked_Static(&JSON2StructPanel::HandleFixRegistryClicked, Args.AppendLog, TreeView, Args.RefreshTreeViewSafely, Args.FixedTreeRootDir, Args.LastTreeSnapshot)
				[
					SNew(STextBlock)
					.Text(LOCTEXT("FixRegistryButton", "维护 Registry"))
				]
			]
		];

		TSharedPtr<SScrollBar> LeftTreeVerticalScrollBar;
		TSharedRef<SWidget> LeftTreeWidget = SNew(SOverlay)
			+ SOverlay::Slot()
			[
				SAssignNew(TreeView, STreeView<TSharedPtr<FGeneratedFileTreeItem>>)
			.TreeItemsSource(Args.TreeRoots)
			.ExternalScrollbar(LeftTreeVerticalScrollBar)
			.Clipping(EWidgetClipping::ClipToBoundsAlways)
			.OnSelectionChanged_Lambda([Args, TreeViewWeakRef](TSharedPtr<FGeneratedFileTreeItem> Item, ESelectInfo::Type SelectInfo)
			{
				if (!Item.IsValid() || SelectInfo != ESelectInfo::OnMouseClick)
				{
					return;
				}
				if (Args.PendingPreviewTickerHandle->IsValid())
				{
					FTSTicker::GetCoreTicker().RemoveTicker(*Args.PendingPreviewTickerHandle);
					Args.PendingPreviewTickerHandle->Reset();
				}
				if (Item->bIsFolder)
				{
					const TSharedPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>> PinnedTreeView = TreeViewWeakRef->Pin();
					if (PinnedTreeView.IsValid())
					{
						const bool bExpanded = PinnedTreeView->IsItemExpanded(Item);
						PinnedTreeView->SetItemExpansion(Item, !bExpanded);
					}
				}
			})
			.OnContextMenuOpening(FOnContextMenuOpening::CreateLambda([BuildContextMenuWidget]() -> TSharedPtr<SWidget>
			{
				return BuildContextMenuWidget();
			}))
			.OnGetChildren_Lambda([](TSharedPtr<FGeneratedFileTreeItem> Item, TArray<TSharedPtr<FGeneratedFileTreeItem>>& OutChildren)
			{
				if (Item.IsValid())
				{
					OutChildren = Item->Children;
				}
			})
			.OnGenerateRow_Lambda([](TSharedPtr<FGeneratedFileTreeItem> Item, const TSharedRef<STableViewBase>& Owner)
			{
				if (!Item.IsValid())
				{
					return SNew(STableRow<TSharedPtr<FGeneratedFileTreeItem>>, Owner)
						[
							SNew(STextBlock).Text(LOCTEXT("InvalidTreeItem", "<Invalid Item>"))
						];
				}
				TSharedRef<SHorizontalBox> RowContent = SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(FMargin(0, 0, 6, 0))
					[
						SNew(SImage)
						.Image(Item->bIsFolder
							? FAppStyle::Get().GetBrush(TEXT("ContentBrowser.AssetTreeFolderClosed"))
							: FAppStyle::Get().GetBrush(TEXT("ContentBrowser.ColumnViewAssetIcon")))
					];

				RowContent->AddSlot()
				.FillWidth(1.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(Item->DisplayName))
					.ToolTipText(FText::FromString(Item->FullPath))
				];

				return SNew(STableRow<TSharedPtr<FGeneratedFileTreeItem>>, Owner)
					.Content()
					[
						RowContent
					];
			})
			]
			+ SOverlay::Slot()
			.HAlign(HAlign_Right)
			.VAlign(VAlign_Fill)
			[
				SAssignNew(LeftTreeVerticalScrollBar, SScrollBar)
				.Orientation(Orient_Vertical)
			];
		*TreeViewWeakRef = TreeView;

		LeftPanelContent->AddSlot()
		.FillHeight(1.0f)
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::Get().GetBrush("WhiteBrush"))
			.BorderBackgroundColor(FLinearColor(0.04f, 0.04f, 0.04f, 1.0f))
			.Padding(0.0f)
			.Clipping(EWidgetClipping::ClipToBoundsAlways)
			[
				LeftTreeWidget
			]
		];

		if (Args.OutTreeView)
		{
			*Args.OutTreeView = TreeView;
		}
		return LeftPanelContent;
	}

	TSharedRef<SWidget> BuildLeftPanelRootWidget(const TSharedRef<SWidget>& LeftPanelContent, float LeftPanelFixedWidth)
	{
		return SNew(SBox)
			.WidthOverride(LeftPanelFixedWidth)
			.MinDesiredWidth(LeftPanelFixedWidth)
			.MaxDesiredWidth(LeftPanelFixedWidth)
			.Clipping(EWidgetClipping::ClipToBoundsAlways)
			[
				SNew(SBorder)
				.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
				.Padding(FMargin(6))
				.Clipping(EWidgetClipping::ClipToBoundsAlways)
				[
					LeftPanelContent
				]
			];
	}
}

#undef LOCTEXT_NAMESPACE

