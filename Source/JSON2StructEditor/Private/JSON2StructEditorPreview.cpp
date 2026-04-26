// Copyright Epic Games, Inc. All Rights Reserved.
//Author WeChat: wujifeng_mr

#include "JSON2StructEditorPreview.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SScrollBar.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SWindow.h"
#include "Widgets/Text/STextBlock.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#define LOCTEXT_NAMESPACE "JSON2StructPreview"

namespace JSON2StructPanel
{
	static FReply HandleClosePreviewWindowClicked(TSharedRef<SWindow> PreviewWindow)
	{
		PreviewWindow->RequestDestroyWindow();
		return FReply::Handled();
	}

	void ShowFilePreviewWindow(const FString& FilePath, TFunction<void(const FString&)> AppendLog)
	{
		if (!FPaths::FileExists(FilePath))
		{
			AppendLog(FString::Printf(TEXT("预览失败，文件不存在：%s"), *FilePath));
			return;
		}

		FString FileContent;
		if (!FFileHelper::LoadFileToString(FileContent, *FilePath))
		{
			AppendLog(FString::Printf(TEXT("预览失败，读取文件失败：%s"), *FilePath));
			return;
		}

		TSharedRef<SWindow> PreviewWindow = SNew(SWindow)
			.Title(FText::FromString(FString::Printf(TEXT("代码预览 - %s"), *FPaths::GetCleanFilename(FilePath))))
			.ClientSize(FVector2D(960.0f, 620.0f))
			.SizingRule(ESizingRule::FixedSize)
			.SupportsMaximize(false)
			.SupportsMinimize(false);

		TSharedPtr<SScrollBar> PreviewVerticalScrollBar;
		TSharedPtr<SScrollBar> PreviewHorizontalScrollBar;
		PreviewWindow->SetContent(
			SNew(SBorder)
			.Padding(8.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding(0, 0, 0, 6)
				[
					SNew(STextBlock)
					.Text(FText::FromString(FilePath))
				]
				+ SVerticalBox::Slot()
				.FillHeight(1.0f)
				[
					SNew(SBox)
					.WidthOverride(1180.0f)
					.HeightOverride(500.0f)
					[
						SNew(SOverlay)
						+ SOverlay::Slot()
						[
							SNew(SMultiLineEditableTextBox)
							.IsReadOnly(true)
							.AutoWrapText(false)
							.VScrollBar(PreviewVerticalScrollBar)
							.HScrollBar(PreviewHorizontalScrollBar)
							.Text(FText::FromString(FileContent))
						]
						+ SOverlay::Slot()
						.HAlign(HAlign_Right)
						.VAlign(VAlign_Fill)
						.Padding(FMargin(0, 0, 0, 14))
						[
							SAssignNew(PreviewVerticalScrollBar, SScrollBar)
							.Orientation(Orient_Vertical)
						]
						+ SOverlay::Slot()
						.HAlign(HAlign_Fill)
						.VAlign(VAlign_Bottom)
						.Padding(FMargin(0, 0, 14, 0))
						[
							SAssignNew(PreviewHorizontalScrollBar, SScrollBar)
							.Orientation(Orient_Horizontal)
						]
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Right)
				.Padding(0, 8, 0, 0)
				[
					SNew(SButton)
					.ToolTipText(LOCTEXT("ClosePreviewWindowTip", "关闭当前代码预览窗口。"))
					.Text(LOCTEXT("ClosePreviewWindow", "关闭预览"))
					.OnClicked_Static(&JSON2StructPanel::HandleClosePreviewWindowClicked, PreviewWindow)
				]
			]);

		FSlateApplication::Get().AddWindow(PreviewWindow);
	}
}

#undef LOCTEXT_NAMESPACE

