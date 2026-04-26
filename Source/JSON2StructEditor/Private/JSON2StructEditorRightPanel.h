// Copyright Epic Games, Inc. All Rights Reserved.
//Author WeChat: wujifeng_mr

#pragma once

#include "CoreMinimal.h"
#include "JSON2StructEditor.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Views/STreeView.h"

namespace JSON2StructPanel
{
	struct FRightPanelBuildArgs
	{
		TSharedPtr<SEditableTextBox> UrlEditable;
		TWeakPtr<SMultiLineEditableTextBox> JsonEditableWeak;
		TSharedPtr<SMultiLineEditableTextBox> JsonEditable;
		TSharedPtr<SMultiLineEditableTextBox> LogEditable;
		TSharedPtr<SEditableTextBox>* OutDirEditable = nullptr;
		TSharedPtr<SComboBox<TSharedPtr<FString>>>* OutMethodComboBox = nullptr;
		TSharedPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>> TreeView;
		const TArray<TSharedPtr<FString>>* HttpMethodOptions = nullptr;
		TSharedRef<FString> SelectedHttpMethodValue;
		TFunction<void(const FString&)> AppendLog;
		TFunction<void(TSharedPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>>, const FString&, const TSharedRef<FString>&)> RefreshTreeViewSafely;
		TSharedRef<FString> LastTreeSnapshot;
		FString FixedTreeRootDir;
		float BigButtonMinWidth = 116.0f;
		float BigButtonMinHeight = 30.0f;
		int32 ButtonFontSize = 12;
	};

	TSharedRef<SWidget> BuildRightPanelContentWidget(FRightPanelBuildArgs& Args);
	TSharedRef<SWidget> BuildRightPanelRootWidget(const TSharedRef<SWidget>& RightPanelContent);
}

