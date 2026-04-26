// Copyright Epic Games, Inc. All Rights Reserved.
//Author WeChat: wujifeng_mr

#pragma once

#include "CoreMinimal.h"
#include "JSON2StructEditor.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Views/STreeView.h"

namespace JSON2StructPanel
{
	FReply HandleFixRegistryClicked(
		TFunction<void(const FString&)> AppendLog,
		TSharedPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>> TreeView,
		TFunction<void(TSharedPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>>, const FString&, const TSharedRef<FString>&)> RefreshTreeViewSafely,
		FString FixedTreeRootDir,
		TSharedRef<FString> LastTreeSnapshot);

	FReply HandleRequestClicked(
		TSharedPtr<SEditableTextBox> UrlEditable,
		TWeakPtr<SMultiLineEditableTextBox> JsonEditableWeak,
		TFunction<void(const FString&)> AppendLog,
		TSharedRef<FString> SelectedHttpMethodValue);
	FReply HandleGenerateStructureClicked(
		TSharedPtr<SEditableTextBox> UrlEditable,
		TSharedPtr<SMultiLineEditableTextBox> JsonEditable,
		TSharedPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>> TreeView,
		TFunction<void(const FString&)> AppendLog,
		TFunction<void(TSharedPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>>, const FString&, const TSharedRef<FString>&)> RefreshTreeViewSafely,
		TSharedRef<FString> LastTreeSnapshot,
		FString FixedTreeRootDir,
		TSharedRef<FString> SelectedHttpMethodValue);

	FReply HandleHotCompileClicked(TFunction<void(const FString&)> AppendLog);
	FReply HandleRefreshAndHotCompileClicked(TFunction<void(const FString&)> AppendLog);

	FReply HandleBrowseDirectoryClicked(
		TSharedPtr<SEditableTextBox> DirEditable,
		TSharedPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>> TreeView,
		TFunction<void(const FString&)> AppendLog,
		TFunction<void(TSharedPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>>, const FString&, const TSharedRef<FString>&)> RefreshTreeViewSafely,
		TSharedRef<FString> LastTreeSnapshot,
		FString FixedTreeRootDir);

	FReply HandleCopyGeneratedClicked(
		TSharedPtr<SEditableTextBox> DirEditable,
		TFunction<void(const FString&)> AppendLog,
		FString FixedTreeRootDir,
		TFunction<void(TSharedPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>>, const FString&, const TSharedRef<FString>&)> RefreshTreeViewSafely,
		TSharedPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>> TreeView,
		TSharedRef<FString> LastTreeSnapshot);

	FReply HandleToggleTreeFolderClicked(
		TSharedRef<STreeView<TSharedPtr<FGeneratedFileTreeItem>>> TreeOwner,
		TSharedPtr<FGeneratedFileTreeItem> Item);
}

