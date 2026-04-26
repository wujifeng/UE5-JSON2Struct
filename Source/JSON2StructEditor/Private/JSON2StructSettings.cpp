// Copyright Epic Games, Inc. All Rights Reserved.
//Author WeChat: wujifeng_mr

#include "JSON2StructSettings.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "JSON2StructSettings"

const TCHAR* FJSON2StructSettings::GetConfigSection() { return TEXT("JSON2Struct"); }
const TCHAR* FJSON2StructSettings::GetLastStructureDirectoryKey() { return TEXT("LastStructureDirectory"); }
const TCHAR* FJSON2StructSettings::GetGeneratedItemsKey() { return TEXT("GeneratedItems"); }
const TCHAR* FJSON2StructSettings::GetUseCustomDirectoryKey() { return TEXT("UseCustomDirectory"); }

// 默认生成到运行时模块 JSON2StructRuntime 的 Public/Generated，模块自包含
FString FJSON2StructSettings::GetDefaultGeneratedCodeDir()
{
	TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("JSON2Struct"));
	if (Plugin.IsValid())
		return FPaths::Combine(Plugin->GetBaseDir(), TEXT("Source"), TEXT("JSON2StructRuntime"), TEXT("Public"), TEXT("Generated"));
	return FPaths::ProjectContentDir() / TEXT("JSON2Struct");
}

FString FJSON2StructSettings::LoadLastStructureDirectory()
{
	FString Path;
	GConfig->GetString(GetConfigSection(), GetLastStructureDirectoryKey(), Path, GEditorPerProjectIni);
	if (Path.IsEmpty()) Path = GetDefaultGeneratedCodeDir();
	return Path;
}

void FJSON2StructSettings::SaveLastStructureDirectory(const FString& Path)
{
	GConfig->SetString(GetConfigSection(), GetLastStructureDirectoryKey(), *Path, GEditorPerProjectIni);
	GConfig->Flush(false, GEditorPerProjectIni);
}

bool FJSON2StructSettings::LoadUseCustomDirectory()
{
	bool bUse = false;
	GConfig->GetBool(GetConfigSection(), GetUseCustomDirectoryKey(), bUse, GEditorPerProjectIni);
	return bUse;
}

void FJSON2StructSettings::SaveUseCustomDirectory(bool bUseCustom)
{
	GConfig->SetBool(GetConfigSection(), GetUseCustomDirectoryKey(), bUseCustom, GEditorPerProjectIni);
	GConfig->Flush(false, GEditorPerProjectIni);
}

FString FJSON2StructGeneratedItem::GetTypeLabel() const
{
	return Type == EJSON2StructGeneratedType::RestAPI ? TEXT("Rest API") : TEXT("Structure");
}

static FString ItemToString(const FJSON2StructGeneratedItem& Item)
{
	const TCHAR* TypeStr = Item.Type == EJSON2StructGeneratedType::RestAPI ? TEXT("RestAPI") : TEXT("Structure");
	return FString::Printf(TEXT("%s|%s|%s"), TypeStr, *Item.DisplayName, *Item.Path);
}

static bool StringToItem(const FString& Line, FJSON2StructGeneratedItem& OutItem)
{
	TArray<FString> Parts;
	Line.ParseIntoArray(Parts, TEXT("|"));
	if (Parts.Num() >= 3)
	{
		OutItem.Type = Parts[0] == TEXT("RestAPI") ? EJSON2StructGeneratedType::RestAPI : EJSON2StructGeneratedType::Structure;
		OutItem.DisplayName = Parts[1];
		OutItem.Path = Parts[2];
		return true;
	}
	return false;
}

TArray<FJSON2StructGeneratedItem> FJSON2StructSettings::LoadGeneratedItems()
{
	TArray<FJSON2StructGeneratedItem> Result;
	FString Value;
	GConfig->GetString(GetConfigSection(), GetGeneratedItemsKey(), Value, GEditorPerProjectIni);
	TArray<FString> Lines;
	Value.ParseIntoArray(Lines, TEXT("\n"));
	for (const FString& Line : Lines)
	{
		FJSON2StructGeneratedItem Item;
		if (StringToItem(Line, Item)) Result.Add(Item);
	}
	return Result;
}

void FJSON2StructSettings::SaveGeneratedItems(const TArray<FJSON2StructGeneratedItem>& Items)
{
	TArray<FString> Lines;
	for (const FJSON2StructGeneratedItem& Item : Items) Lines.Add(ItemToString(Item));
	GConfig->SetString(GetConfigSection(), GetGeneratedItemsKey(), *FString::Join(Lines, TEXT("\n")), GEditorPerProjectIni);
	GConfig->Flush(false, GEditorPerProjectIni);
}

#undef LOCTEXT_NAMESPACE
