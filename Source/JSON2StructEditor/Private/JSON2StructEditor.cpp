// Copyright Epic Games, Inc. All Rights Reserved.
//Author WeChat: wujifeng_mr

#include "JSON2StructEditor.h"
#include "JSON2StructStyle.h"
#include "JSON2StructCommands.h"
#include "JSON2StructEditorLeftPanel.h"
#include "JSON2StructEditorRightPanel.h"
#include "JSON2StructEditorActions.h"
#include "JSON2StructEditorTree.h"
#include "JSON2StructEditorController.h"
#include "JSON2StructEditorPreview.h"
#include "LevelEditor.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Framework/Docking/TabManager.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SWindow.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Views/STreeView.h"
#include "ToolMenus.h"
#include "ToolMenuEntry.h"
#include "Misc/Paths.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "HAL/PlatformProcess.h"
#include "Interfaces/IPluginManager.h"
#include "Modules/ModuleManager.h"
#include "HttpModule.h"
#include "HttpManager.h"
#include "Containers/Ticker.h"
#include "HAL/PlatformFileManager.h"
#include "Framework/Text/TextLayout.h"
#include "Engine/Engine.h"
#include "Async/Async.h"
#include "HAL/FileManager.h"

IMPLEMENT_MODULE(FJSON2StructEditorModule, JSON2StructEditor)

static const FName JSON2StructTabName("JSON2Struct");
static const FString DefaultJsonUrl(TEXT("https://api.open-meteo.com/v1/forecast?latitude=22.54&longitude=114.07&current_weather=true"));

#define LOCTEXT_NAMESPACE "JSON2Struct"

// ---------- 模块实现 ----------
void FJSON2StructEditorModule::StartupModule()
{
	FJSON2StructStyle::Initialize();
	FJSON2StructStyle::ReloadTextures();
	FJSON2StructCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);
	PluginCommands->MapAction(
		FJSON2StructCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateLambda([]()
		{
			if (FJSON2StructEditorModule* Module = FModuleManager::GetModulePtr<FJSON2StructEditorModule>(TEXT("JSON2StructEditor")))
			{
				Module->PluginButtonClicked();
			}
		}),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FJSON2StructEditorModule::RegisterMenus));

	FGlobalTabmanager::Get()->RegisterNomadTabSpawner(JSON2StructTabName, FOnSpawnTab::CreateRaw(this, &FJSON2StructEditorModule::OnSpawnPluginTab))
		.SetDisplayName(LOCTEXT("JSON2StructTabTitle", "JSON2Struct"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	HttpTickerHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda(
		[](float DeltaTime) -> bool
		{
			if (FModuleManager::Get().IsModuleLoaded("HTTP"))
				FHttpModule::Get().GetHttpManager().Tick(DeltaTime);
			return true;
		}));
}

void FJSON2StructEditorModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(this);
	UToolMenus::UnregisterOwner(this);
	FJSON2StructStyle::Shutdown();
	if (HttpTickerHandle.IsValid())
	{
		FTSTicker::RemoveTicker(HttpTickerHandle);
		HttpTickerHandle.Reset();
	}
	FJSON2StructCommands::Unregister();
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(JSON2StructTabName);
}

void FJSON2StructEditorModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(FTabId(JSON2StructTabName));
}

void FJSON2StructEditorModule::RefreshGeneratedCodeTree(const FString& OutputDir)
{
	PluginTreeRoots.Reset();
	if (OutputDir.IsEmpty())
	{
		return;
	}

	const FString FullOutDir = FPaths::ConvertRelativePathToFull(OutputDir);
	if (!IFileManager::Get().DirectoryExists(*FullOutDir))
	{
		return;
	}

	TSharedPtr<FGeneratedFileTreeItem> OutputRoot = JSON2StructPanel::BuildFileTreeFromDirectory(FullOutDir);
	if (OutputRoot.IsValid())
	{
		OutputRoot->DisplayName = FString::Printf(TEXT("Generated (%s)"), *FPaths::GetCleanFilename(FullOutDir));
		PluginTreeRoots.Add(OutputRoot);
	}
}

void FJSON2StructEditorModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	// Window 菜单：打开 JSON2Struct 面板
	UToolMenu* WindowMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
	if (WindowMenu)
	{
		FToolMenuSection& WindowSection = WindowMenu->FindOrAddSection("WindowLayout");
		WindowSection.AddMenuEntryWithCommandList(FJSON2StructCommands::Get().OpenPluginWindow, PluginCommands);
	}

	// 工具栏按钮：在关卡编辑器主工具栏显示 JSON2Struct 按钮
	UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.LevelEditorToolBar.AssetsToolBar");
	if (ToolbarMenu)
	{
		FToolMenuSection& ToolbarSection = ToolbarMenu->FindOrAddSection("Content");
		ToolbarSection.AddEntry(FToolMenuEntry::InitToolBarButton(
			FName("JSON2Struct"),
			FToolUIActionChoice(FUIAction(FExecuteAction::CreateLambda([]()
			{
				if (FJSON2StructEditorModule* Module = FModuleManager::GetModulePtr<FJSON2StructEditorModule>(TEXT("JSON2StructEditor")))
				{
					Module->PluginButtonClicked();
				}
			}))),
			LOCTEXT("JSON2StructToolbarLabel", "JSON2Struct"),
			LOCTEXT("JSON2StructToolbarTooltip", "打开 JSON2Struct 面板"),
			TAttribute<FSlateIcon>(FSlateIcon(FJSON2StructStyle::GetStyleSetName(), FName("JSON2Struct.OpenPluginWindow")))
		));
	}
}

TSharedRef<SDockTab> FJSON2StructEditorModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	// 先创建 URL/JSON/Log 控件，再构建布局，这样“请求”按钮回调里捕获的指针才有效（之前版本的做法）
	TSharedPtr<SEditableTextBox> UrlEditable = SNew(SEditableTextBox)
		.Text(FText::FromString(DefaultJsonUrl))
		.HintText(LOCTEXT("UrlHint", "输入 JSON API 地址"));
	TSharedPtr<SMultiLineEditableTextBox> JsonEditable = SNew(SMultiLineEditableTextBox)
		.HintText(LOCTEXT("JsonHint", "请求后 JSON 将显示在此，或直接粘贴"))
		.IsReadOnly(false);
	TSharedPtr<SMultiLineEditableTextBox> LogEditable = SNew(SMultiLineEditableTextBox)
		.IsReadOnly(true)
		.AutoWrapText(true)
		.HintText(LOCTEXT("LogHint", "日志"));
	static TArray<TSharedPtr<FString>> HttpMethodOptions;
	if (HttpMethodOptions.Num() == 0)
	{
		HttpMethodOptions.Add(MakeShared<FString>(TEXT("GET")));
		HttpMethodOptions.Add(MakeShared<FString>(TEXT("POST")));
		HttpMethodOptions.Add(MakeShared<FString>(TEXT("PUT")));
		HttpMethodOptions.Add(MakeShared<FString>(TEXT("DELETE")));
		HttpMethodOptions.Add(MakeShared<FString>(TEXT("PATCH")));
	}
	TSharedRef<FString> SelectedHttpMethodValue = MakeShared<FString>(TEXT("GET"));
	TSharedPtr<SComboBox<TSharedPtr<FString>>> MethodComboBox;
	const float BigButtonMinWidth = 116.0f;
	const float BigButtonMinHeight = 30.0f;
	const int32 ButtonFontSize = 12;

	TSharedPtr<SEditableTextBox> DirEditable;
	TSharedPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>> TreeView;
	auto ExpandTreeToFirstLevel = JSON2StructPanel::MakeExpandTreeToFirstLevelHandler();
	auto RefreshTreeViewSafely = JSON2StructPanel::MakeRefreshTreeViewSafelyHandler(this, &PluginTreeRoots, ExpandTreeToFirstLevel);

	// 左侧树固定显示 Runtime/Public/Generated，避免出现无关目录
	const FString FixedTreeRootDir = FPaths::ConvertRelativePathToFull(
		FPaths::Combine(FPaths::ProjectDir(), TEXT("Plugins/JSON2Struct/Source/JSON2StructRuntime/Public/Generated")));
	RefreshGeneratedCodeTree(FixedTreeRootDir);
	TSharedRef<FString> LastTreeSnapshot = MakeShared<FString>(JSON2StructPanel::BuildDirectoryTreeSnapshot(FixedTreeRootDir));

	// 异步回调中用 TWeakPtr，避免标签页关闭后访问已销毁的 Slate 控件导致 0xC0000005
	TWeakPtr<SMultiLineEditableTextBox> LogEditableWeak = LogEditable;
	TWeakPtr<SMultiLineEditableTextBox> JsonEditableWeak = JsonEditable;
	TSharedRef<FString> LogBuffer = MakeShared<FString>();
	auto AppendLog = JSON2StructPanel::MakeAppendLogHandler(LogEditableWeak, LogBuffer);
	TSharedRef<FString> PendingPreviewPath = MakeShared<FString>();
	TSharedRef<bool> bSuppressNextSinglePreview = MakeShared<bool>(false);
	TSharedRef<FTSTicker::FDelegateHandle> PendingPreviewTickerHandle = MakeShared<FTSTicker::FDelegateHandle>();
	auto OpenTreeItem = [AppendLog](const TSharedPtr<FGeneratedFileTreeItem>& Item)
	{
		if (!Item.IsValid())
		{
			return;
		}
		if (Item->bIsFolder)
		{
			JSON2StructPanel::OpenFolderAndSelectFile(Item->FullPath);
			return;
		}
		if (!JSON2StructPanel::OpenFileInIDE(Item->FullPath))
		{
			AppendLog(FString::Printf(TEXT("无法在IDE打开：%s"), *Item->FullPath));
		}
	};
	auto PreviewTreeItem = [AppendLog](const TSharedPtr<FGeneratedFileTreeItem>& Item)
	{
		if (!Item.IsValid() || Item->bIsFolder)
		{
			return;
		}
		JSON2StructPanel::ShowFilePreviewWindow(Item->FullPath, AppendLog);
	};
	auto DeleteTreeItem = [AppendLog, RefreshTreeViewSafely](const TSharedPtr<FGeneratedFileTreeItem>& Item, TSharedPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>> InTreeView, const FString& OutputDir, const TSharedRef<FString>& InLastTreeSnapshot)
	{
		if (!Item.IsValid())
		{
			return;
		}
		const FString NormalizedRoot = FPaths::ConvertRelativePathToFull(OutputDir);
		const FString NormalizedTarget = FPaths::ConvertRelativePathToFull(Item->FullPath);
		if (!NormalizedTarget.StartsWith(NormalizedRoot))
		{
			AppendLog(FString::Printf(TEXT("拒绝删除：目标不在 Generated 根目录内 (%s)"), *NormalizedTarget));
			return;
		}
		const FString RuntimeModulePrivateGeneratedRoot = FPaths::ConvertRelativePathToFull(
			FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("JSON2Struct/Source/JSON2StructRuntime/Private/Generated")));

		IPlatformFile& PF = FPlatformFileManager::Get().GetPlatformFile();
		auto DeletePath = [&PF](const FString& TargetPath, bool bIsFolder) -> bool
		{
			if (bIsFolder)
			{
				bool bRemoved = IFileManager::Get().DeleteDirectory(*TargetPath, false, true);
				if (!bRemoved)
				{
					bRemoved = PF.DeleteDirectoryRecursively(*TargetPath);
				}
				return bRemoved;
			}

			bool bRemoved = IFileManager::Get().Delete(*TargetPath, false, true, true);
			if (!bRemoved)
			{
				bRemoved = PF.DeleteFile(*TargetPath);
			}
			return bRemoved;
		};

		bool bDeleted = false;
		bDeleted = DeletePath(NormalizedTarget, Item->bIsFolder);
		if (bDeleted)
		{
			AppendLog(FString::Printf(TEXT("已删除：%s"), *NormalizedTarget));

			FString RelativeTarget = NormalizedTarget;
			if (FPaths::MakePathRelativeTo(RelativeTarget, *NormalizedRoot) && !RelativeTarget.StartsWith(TEXT("..")))
			{
				const FString PrivateMirrorTarget = FPaths::Combine(RuntimeModulePrivateGeneratedRoot, RelativeTarget);
				const bool bPrivateExists = Item->bIsFolder
					? PF.DirectoryExists(*PrivateMirrorTarget)
					: PF.FileExists(*PrivateMirrorTarget);
				if (bPrivateExists)
				{
					if (DeletePath(PrivateMirrorTarget, Item->bIsFolder))
					{
						AppendLog(FString::Printf(TEXT("已同步删除 Private 对应项：%s"), *PrivateMirrorTarget));
					}
					else
					{
						AppendLog(FString::Printf(TEXT("Private 对应项删除失败（可能被占用或只读）：%s"), *PrivateMirrorTarget));
					}
				}
			}

			RefreshTreeViewSafely(InTreeView, OutputDir, InLastTreeSnapshot);
		}
		else
		{
			AppendLog(FString::Printf(TEXT("删除失败（可能被占用或只读）：%s"), *NormalizedTarget));
		}
	};
	constexpr float LeftPanelFixedWidth = 420.0f;
	JSON2StructPanel::FLeftPanelBuildArgs LeftPanelArgs{
		&PluginTreeRoots,
		&TreeView,
		AppendLog,
		RefreshTreeViewSafely,
		FixedTreeRootDir,
		LastTreeSnapshot,
		PendingPreviewPath,
		bSuppressNextSinglePreview,
		PendingPreviewTickerHandle,
		OpenTreeItem,
		PreviewTreeItem,
		DeleteTreeItem
	};
	TSharedRef<SWidget> LeftPanelContentWidget = JSON2StructPanel::BuildLeftPanelContentWidget(LeftPanelArgs);
	TSharedRef<SWidget> LeftPanelWidget = JSON2StructPanel::BuildLeftPanelRootWidget(LeftPanelContentWidget, LeftPanelFixedWidth);

	JSON2StructPanel::FRightPanelBuildArgs RightPanelArgs{
		UrlEditable,
		JsonEditableWeak,
		JsonEditable,
		LogEditable,
		&DirEditable,
		&MethodComboBox,
		TreeView,
		&HttpMethodOptions,
		SelectedHttpMethodValue,
		AppendLog,
		RefreshTreeViewSafely,
		LastTreeSnapshot,
		FixedTreeRootDir,
		BigButtonMinWidth,
		BigButtonMinHeight,
		ButtonFontSize
	};
	TSharedRef<SWidget> RightPanelContentWidget = JSON2StructPanel::BuildRightPanelContentWidget(RightPanelArgs);
	TSharedRef<SWidget> RightPanelWidget = JSON2StructPanel::BuildRightPanelRootWidget(RightPanelContentWidget);

	// Root composition
	TSharedRef<SWidget> MainContentWidget = SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.MinWidth(LeftPanelFixedWidth)
		.MaxWidth(LeftPanelFixedWidth)
		.Padding(FMargin(0, 0, 6, 0))
		[
			LeftPanelWidget
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			RightPanelWidget
		];

	TSharedRef<SWidget> MainRootWidget = SNew(SBorder)
		.Padding(4.0f)
		[
			MainContentWidget
		];

	TSharedRef<SDockTab> Tab = SNew(SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			MainRootWidget
		];

	if (TreeView.IsValid())
	{
		TreeView->RequestTreeRefresh();
		ExpandTreeToFirstLevel(TreeView, PluginTreeRoots);
	}

	TSharedRef<FTSTicker::FDelegateHandle> AutoRefreshTickerHandle =
		JSON2StructPanel::StartAutoRefreshTicker(TreeView, RefreshTreeViewSafely, LastTreeSnapshot, FixedTreeRootDir);

	Tab->SetOnTabClosed(SDockTab::FOnTabClosedCallback::CreateLambda([AutoRefreshTickerHandle, PendingPreviewTickerHandle](TSharedRef<SDockTab>)
	{
		if (AutoRefreshTickerHandle->IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(*AutoRefreshTickerHandle);
			AutoRefreshTickerHandle->Reset();
		}
		if (PendingPreviewTickerHandle->IsValid())
		{
			FTSTicker::GetCoreTicker().RemoveTicker(*PendingPreviewTickerHandle);
			PendingPreviewTickerHandle->Reset();
		}
	}));

	return Tab;
}

#undef LOCTEXT_NAMESPACE
