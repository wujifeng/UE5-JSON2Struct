// Copyright Epic Games, Inc. All Rights Reserved.
//Author WeChat: wujifeng_mr

#include "JSON2StructEditorRightPanel.h"
#include "JSON2StructEditorActions.h"
#include "JSON2StructSettings.h"
#include "Styling/AppStyle.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "JSON2Struct"

namespace JSON2StructPanel
{
	TSharedRef<SWidget> BuildRightPanelContentWidget(FRightPanelBuildArgs& Args)
	{
		TSharedPtr<SComboBox<TSharedPtr<FString>>> MethodComboBox;
		TSharedPtr<SEditableTextBox> DirEditable;

		TSharedRef<SWidget> MethodRowWidget = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(0, 0, 8, 0))
			[
				SNew(STextBlock).Text(LOCTEXT("MethodLabel", "请求方法"))
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SAssignNew(MethodComboBox, SComboBox<TSharedPtr<FString>>)
				.OptionsSource(Args.HttpMethodOptions)
				.InitiallySelectedItem((*Args.HttpMethodOptions)[0])
				.OnGenerateWidget_Lambda([](TSharedPtr<FString> Item)
				{
					return SNew(STextBlock).Text(FText::FromString(Item.IsValid() ? *Item : TEXT("GET")));
				})
				.OnSelectionChanged_Lambda([SelectedHttpMethodValue = Args.SelectedHttpMethodValue](TSharedPtr<FString> NewValue, ESelectInfo::Type)
				{
					if (NewValue.IsValid())
					{
						*SelectedHttpMethodValue = *NewValue;
					}
				})
				[
					SNew(STextBlock)
					.Text_Lambda([SelectedHttpMethodValue = Args.SelectedHttpMethodValue]()
					{
						return FText::FromString(*SelectedHttpMethodValue);
					})
				]
			];

		TSharedRef<SWidget> UrlRowWidget = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			.Padding(FMargin(0, 0, 8, 0))
			[
				SNew(STextBlock).Text(LOCTEXT("UrlLabel", "请输入Json请求网址"))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			[
				Args.UrlEditable.ToSharedRef()
			];

		TSharedRef<SWidget> RequestButtonWidget = SNew(SBox)
			.MinDesiredWidth(Args.BigButtonMinWidth)
			.MinDesiredHeight(Args.BigButtonMinHeight)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.ToolTipText(LOCTEXT("RequestButtonTip", "按当前请求方法和 URL 发起请求，并将返回 JSON 填入下方文本框。"))
				.ContentPadding(FMargin(10, 6))
				[
					SNew(STextBlock)
					.Font(FCoreStyle::GetDefaultFontStyle("Regular", Args.ButtonFontSize))
					.Justification(ETextJustify::Center)
					.Text(LOCTEXT("Request", "请求"))
				]
				.OnClicked_Static(&JSON2StructPanel::HandleRequestClicked, Args.UrlEditable, Args.JsonEditableWeak, Args.AppendLog, Args.SelectedHttpMethodValue)
			];

		TSharedRef<SWidget> JsonInputHintWidget = SNew(STextBlock)
			.AutoWrapText(true)
			.Text(LOCTEXT("JsonInputHintAfterRequest", "请黏贴Json数据样例到以下文本框，或请求Json获取Json数据到以下文本框。"));
		TSharedRef<SWidget> JsonAreaHeaderWidget = SNew(STextBlock).Text(LOCTEXT("JsonAreaHeader", "JSON 内容"));

		TSharedRef<SWidget> ActionButtonsRowWidget = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth()
			[
				SNew(SBox).MinDesiredWidth(Args.BigButtonMinWidth).MinDesiredHeight(Args.BigButtonMinHeight)
				[
					SNew(SButton)
					.HAlign(HAlign_Center).VAlign(VAlign_Center)
					.ToolTipText(LOCTEXT("GenStructButtonTip", "根据当前 JSON 内容生成/更新接口结构体与接口专属蓝图 API 文件。"))
					.ContentPadding(FMargin(10, 6))
					[
						SNew(STextBlock)
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", Args.ButtonFontSize))
						.Justification(ETextJustify::Center)
						.Text(LOCTEXT("GenStruct", "生成 Structure"))
					]
					.OnClicked_Static(&JSON2StructPanel::HandleGenerateStructureClicked, Args.UrlEditable, Args.JsonEditable, Args.TreeView, Args.AppendLog, Args.RefreshTreeViewSafely, Args.LastTreeSnapshot, Args.FixedTreeRootDir, Args.SelectedHttpMethodValue)
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(6, 0, 0, 0))
			[
				SNew(SBox).MinDesiredWidth(Args.BigButtonMinWidth).MinDesiredHeight(Args.BigButtonMinHeight)
				[
					SNew(SButton)
					.HAlign(HAlign_Center).VAlign(VAlign_Center)
					.ToolTipText(LOCTEXT("HotCompileButtonTip", "仅触发 UE Live Coding 热编译。"))
					.ContentPadding(FMargin(10, 6))
					[
						SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", Args.ButtonFontSize)).Justification(ETextJustify::Center).Text(LOCTEXT("HotCompile", "热编译"))
					]
					.OnClicked_Static(&JSON2StructPanel::HandleHotCompileClicked, Args.AppendLog)
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(6, 0, 0, 0))
			[
				SNew(SBox).MinDesiredWidth(Args.BigButtonMinWidth).MinDesiredHeight(Args.BigButtonMinHeight)
				[
					SNew(SButton)
					.HAlign(HAlign_Center).VAlign(VAlign_Center)
					.ToolTipText(LOCTEXT("RefreshAndHotCompileButtonTip", "先刷新工程文件（-projectfiles），再触发 Live Coding 热编译。"))
					.ContentPadding(FMargin(10, 6))
					[
						SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", Args.ButtonFontSize)).Justification(ETextJustify::Center).Text(LOCTEXT("RefreshAndHotCompile", "刷新工程文件+热编译"))
					]
					.OnClicked_Static(&JSON2StructPanel::HandleRefreshAndHotCompileClicked, Args.AppendLog)
				]
			];

		TSharedRef<SWidget> LogSectionWidget = SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0, 0, 0, 2))
			[
				SNew(STextBlock).Text(LOCTEXT("LogHeader", "日志"))
			]
			+ SVerticalBox::Slot().FillHeight(1.0f)
			[
				Args.LogEditable.ToSharedRef()
			];

		TSharedRef<SWidget> MigrateControlsRowWidget = SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().FillWidth(1.0f)
			[
				SAssignNew(DirEditable, SEditableTextBox)
				.Text(FText::FromString(FJSON2StructSettings::LoadLastStructureDirectory()))
				.HintText(LOCTEXT("DirHint", "选择生成目录"))
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(4, 0, 0, 0))
			[
				SNew(SBox).MinDesiredWidth(Args.BigButtonMinWidth).MinDesiredHeight(Args.BigButtonMinHeight)
				[
					SNew(SButton)
					.HAlign(HAlign_Center).VAlign(VAlign_Center)
					.ToolTipText(LOCTEXT("BrowseButtonTip", "选择迁移目标目录。"))
					.ContentPadding(FMargin(10, 6))
					[
						SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", Args.ButtonFontSize)).Justification(ETextJustify::Center).Text(LOCTEXT("Browse", "浏览"))
					]
					.OnClicked_Static(&JSON2StructPanel::HandleBrowseDirectoryClicked, DirEditable, Args.TreeView, Args.AppendLog, Args.RefreshTreeViewSafely, Args.LastTreeSnapshot, Args.FixedTreeRootDir)
				]
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(FMargin(4, 0, 0, 0))
			[
				SNew(SBox).MinDesiredWidth(Args.BigButtonMinWidth).MinDesiredHeight(Args.BigButtonMinHeight)
				[
					SNew(SButton)
					.HAlign(HAlign_Center).VAlign(VAlign_Center)
					.ToolTipText(LOCTEXT("CopyGeneratedButtonTip", "将 Generated 下全部文件递归复制到目标目录。"))
					.ContentPadding(FMargin(10, 6))
					[
						SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular", Args.ButtonFontSize)).Justification(ETextJustify::Center).Text(LOCTEXT("CopyGenerated", "复制"))
					]
					.OnClicked_Static(&JSON2StructPanel::HandleCopyGeneratedClicked, DirEditable, Args.AppendLog, Args.FixedTreeRootDir, Args.RefreshTreeViewSafely, Args.TreeView, Args.LastTreeSnapshot)
				]
			];

		if (Args.OutDirEditable)
		{
			*Args.OutDirEditable = DirEditable;
		}
		if (Args.OutMethodComboBox)
		{
			*Args.OutMethodComboBox = MethodComboBox;
		}

		return SNew(SVerticalBox)
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)[MethodRowWidget]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)[UrlRowWidget]
			+ SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Left).Padding(FMargin(0, 4, 0, 0))[RequestButtonWidget]
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0, 4, 0, 0))[JsonInputHintWidget]
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0, 8, 0, 2))[JsonAreaHeaderWidget]
			+ SVerticalBox::Slot().FillHeight(0.56f).Padding(0, 4)[Args.JsonEditable.ToSharedRef()]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 4)[ActionButtonsRowWidget]
			+ SVerticalBox::Slot().FillHeight(0.44f).Padding(FMargin(0, 8, 0, 0))[LogSectionWidget]
			+ SVerticalBox::Slot().AutoHeight().Padding(FMargin(0, 8, 0, 0))
			[
				SNew(STextBlock).Text(LOCTEXT("MigrateHint", "迁移到指定目录：点击“复制”可将 Generated 下全部文件递归复制到目标目录"))
			]
			+ SVerticalBox::Slot().AutoHeight().Padding(0, 2)[MigrateControlsRowWidget];
	}

	TSharedRef<SWidget> BuildRightPanelRootWidget(const TSharedRef<SWidget>& RightPanelContent)
	{
		return SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("ToolPanel.GroupBorder"))
			.Padding(FMargin(8))
			[
				RightPanelContent
			];
	}
}

#undef LOCTEXT_NAMESPACE

