// Copyright Epic Games, Inc. All Rights Reserved.
//Author WeChat: wujifeng_mr

#pragma once

#include "CoreMinimal.h"

/** 生成项类型 */
enum class EJSON2StructGeneratedType : uint8
{
	Structure,
	RestAPI
};

/** 单条生成记录：Structure 或 Rest API，用于列表显示与定位 */
struct FJSON2StructGeneratedItem
{
	EJSON2StructGeneratedType Type = EJSON2StructGeneratedType::Structure;
	FString DisplayName;
	FString Path;

	FString GetTypeLabel() const;
};

/** 插件配置（仅编辑器使用） */
struct FJSON2StructSettings
{
	static const TCHAR* GetConfigSection();
	static const TCHAR* GetLastStructureDirectoryKey();
	static const TCHAR* GetGeneratedItemsKey();
	static const TCHAR* GetUseCustomDirectoryKey();

	static FString GetDefaultGeneratedCodeDir();
	static FString LoadLastStructureDirectory();
	static void SaveLastStructureDirectory(const FString& Path);
	static bool LoadUseCustomDirectory();
	static void SaveUseCustomDirectory(bool bUseCustom);
	static TArray<FJSON2StructGeneratedItem> LoadGeneratedItems();
	static void SaveGeneratedItems(const TArray<FJSON2StructGeneratedItem>& Items);
};
