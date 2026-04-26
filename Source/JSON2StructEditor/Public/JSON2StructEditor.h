// Copyright Epic Games, Inc. All Rights Reserved.
//Author WeChat: wujifeng_mr

#pragma once

// 编辑器模块（工具栏、面板）：提供 JSON2Struct 面板、URL 请求、JSON 编辑、生成/合并 Structure；依赖运行时模块 JSON2StructRuntime 的 Rest API 与生成的结构体。

#include "Containers/Ticker.h"
#include "Modules/ModuleManager.h"

class FToolBarBuilder;
class FMenuBuilder;
struct FJSON2StructGeneratedItem;

/** 生成目录下的文件树节点，仅用于面板中“生成代码目录”文件树展示 */
struct FGeneratedFileTreeItem
{
	FString DisplayName;
	FString FullPath;
	bool bIsFolder = false;
	TArray<TSharedPtr<FGeneratedFileTreeItem>> Children;
};

class FJSON2StructEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void PluginButtonClicked();

	/** 刷新文件树（展示项目 Content + 插件目录 + 当前生成目录），在打开面板或生成成功后调用 */
	void RefreshGeneratedCodeTree(const FString& OutputDir);

private:
	void RegisterMenus();
	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);

private:
	TSharedPtr<class FUICommandList> PluginCommands;
	FTSTicker::FDelegateHandle HttpTickerHandle;
	/** 生成目录文件树根节点（一个根 = 当前生成目录，子节点 = 目录层级+生成文件），由模块持有供 STreeView 使用 */
	TArray<TSharedPtr<FGeneratedFileTreeItem>> PluginTreeRoots;
};
