// Copyright Epic Games, Inc. All Rights Reserved.
//Author WeChat: wujifeng_mr

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "JSON2StructRequestTypes.h"
#include "JSON2StructBPLibrary.generated.h"

/** Blueprint 可用的请求完成委托：bSuccess, ResponseText */
DECLARE_DYNAMIC_DELEGATE_TwoParams(FJSON2StructRequestCompleteDelegate, bool, bSuccess, const FString&, ResponseText);
/** Blueprint 可用的统一请求完成委托：bSuccess, ResponseText, RootTypeName */
DECLARE_DYNAMIC_DELEGATE_ThreeParams(FJSON2StructRequestToStructCompleteDelegate, bool, bSuccess, const FString&, ResponseText, const FString&, RootTypeName);

/**
 * JSON2Struct 蓝图函数库（仅运行时，无界面）。
 * 与 JSON2StructRest 一起由 JSON2Struct.h 对外提供；根结构体及依赖在模块内部通过总览头引入。
 */
UCLASS(meta = (DisplayName = "JSON2Struct"))
class JSON2STRUCTRUNTIME_API UJSON2StructBPLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 发起 JSON HTTP GET 请求（Blueprint 可用）。
	 * 完成后在游戏线程执行 OnComplete；可在 Blueprint 中绑定自定义事件处理返回内容。
	 */
	UFUNCTION(BlueprintCallable, Category = "JSON2Struct", meta = (DisplayName = "Request JSON"))
	static void RequestJson(const FString& URL, FJSON2StructRequestCompleteDelegate OnComplete);

	/** 统一请求入口：按 Method/Headers/Query/Body 发起请求，并尝试分发解析为已生成结构。 */
	UFUNCTION(BlueprintCallable, Category = "JSON2Struct", meta = (DisplayName = "HTTPRequestToStruct"))
	static void HTTPRequestToStruct(const FHTTPRequestParams& RequestParams, FJSON2StructRequestToStructCompleteDelegate OnComplete);
	/** JSON 字符串转任意结构体（通配） */
	UFUNCTION(BlueprintCallable, CustomThunk, Category = "JSON2Struct", meta = (DisplayName = "Json To Struct", CustomStructureParam = "OutStruct"))
	static bool JsonToStruct(const FString& JsonString, int32& OutStruct);

	/** 任意结构体转 JSON 字符串（通配） */
	UFUNCTION(BlueprintCallable, CustomThunk, Category = "JSON2Struct", meta = (DisplayName = "Struct To Json", CustomStructureParam = "InStruct"))
	static bool StructToJson(const int32& InStruct, FString& OutJsonString);

	DECLARE_FUNCTION(execJsonToStruct);
	DECLARE_FUNCTION(execStructToJson);

private:
	static bool JsonToStructImpl(const FString& JsonString, const UScriptStruct* ScriptStruct, void* OutStructPtr);
	static bool StructToJsonImpl(const UScriptStruct* ScriptStruct, const void* InStructPtr, FString& OutJsonString);
};
