// Copyright Epic Games, Inc. All Rights Reserved.
//Author WeChat: wujifeng_mr

#pragma once

#include "CoreMinimal.h"
#include "JSON2StructRequestTypes.h"
#if __has_include("Generated/JSON2StructGenerated.h")
#include "Generated/JSON2StructGenerated.h"
#else
// 无生成文件时提供声明，定义见 JSON2StructRest.cpp 的 fallback 实现。
void JSON2StructDispatchParse(const FString& URL, const FString& Json, TFunction<void(bool bSuccess, const FString& RootTypeName, const void* StructPtr)> Callback);
#endif

/** 完成回调：参数为响应内容与是否成功（原始 JSON 字符串） */
DECLARE_DELEGATE_TwoParams(FOnJSON2StructRequestComplete, const FString& /*ResponseContent*/, bool /*bSuccess*/);
/** 完成回调：参数为响应内容、是否成功、HTTP 状态码（无响应时为 0） */
DECLARE_DELEGATE_ThreeParams(FOnJSON2StructRequestCompleteWithStatus, const FString& /*ResponseContent*/, bool /*bSuccess*/, int32 /*HttpStatus*/);

/** 按 URL 请求并解析为对应根结构体：bSuccess、根类型名、结构体指针（仅回调内有效，可根据 RootTypeName 强转） */
using FOnRequestToStructComplete = TFunction<void(bool bSuccess, const FString& RootTypeName, const void* StructPtr)>;

/**
 * REST HTTP 请求封装。请求完成后在主线程回调。
 * - Request：返回原始 JSON 字符串。
 * - RequestToStruct：一 URL 对应一根 .h；传入 URL 与回调，回调中拿到解析好的根结构体（根据 RootTypeName 强转为具体类型）。
 */
class JSON2STRUCTRUNTIME_API FJSON2StructRest
{
public:
	/** 根据 Method + URL 生成分发键，供 Generated dispatcher 匹配。 */
	static FString BuildDispatchKey(const FHTTPRequestParams& Spec);

	/** 发起 GET 请求，完成后在主线程调用 OnComplete，传入原始 JSON 字符串 */
	static void Request(const FString& URL, FOnJSON2StructRequestComplete OnComplete);
	/** 根据请求描述发起 HTTP 请求，完成后在主线程调用 OnComplete，传入原始 JSON 字符串 */
	static void RequestBySpec(const FHTTPRequestParams& Spec, FOnJSON2StructRequestComplete OnComplete);
	/** 根据请求描述发起 HTTP 请求，回调返回原始 JSON + HTTP 状态码 */
	static void RequestBySpecWithStatus(const FHTTPRequestParams& Spec, FOnJSON2StructRequestCompleteWithStatus OnComplete);

	/**
	 * 发起 GET 请求，按 URL 自动解析为对应根结构体，在主线程回调中拿到 (bSuccess, RootTypeName, StructPtr)。
	 * StructPtr 仅在回调执行期间有效；可根据 RootTypeName 强转为对应 FXXX（与生成根 .h 一致）。
	 */
	static void RequestToStruct(const FString& URL, FOnRequestToStructComplete OnComplete);
	/** 根据请求描述发起 HTTP 请求并分发解析为对应根结构体 */
	static void RequestToStructBySpec(const FHTTPRequestParams& Spec, FOnRequestToStructComplete OnComplete);
};
