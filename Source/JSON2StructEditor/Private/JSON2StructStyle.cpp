// Copyright Epic Games, Inc. All Rights Reserved.
//Author WeChat: wujifeng_mr

#include "JSON2StructStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleMacros.h"

#define RootToContentDir Style->RootToContentDir

TSharedPtr<FSlateStyleSet> FJSON2StructStyle::StyleInstance = nullptr;

void FJSON2StructStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FJSON2StructStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

FName FJSON2StructStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("JSON2StructStyle"));
	return StyleSetName;
}

const FVector2D Icon20x20(20.0f, 20.0f);

TSharedRef<FSlateStyleSet> FJSON2StructStyle::Create()
{
	TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet("JSON2StructStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("JSON2Struct")->GetBaseDir() / TEXT("Resources"));
	Style->Set("JSON2Struct.OpenPluginWindow", new IMAGE_BRUSH_SVG(TEXT("Icons/JSON2StructToolIcon"), Icon20x20));
	return Style;
}

void FJSON2StructStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
}

const ISlateStyle& FJSON2StructStyle::Get()
{
	return *StyleInstance;
}
