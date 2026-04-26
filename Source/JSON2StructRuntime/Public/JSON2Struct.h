// Copyright Epic Games, Inc. All Rights Reserved.
//Author WeChat: wujifeng_mr

#pragma once

/**
 * 运行时唯一对外入口：包含 REST API、蓝图库及按 URL 分发的根结构体。
 * 使用时仅需：链接 JSON2StructRuntime，且 #include "JSON2Struct.h"。
 *
 * - C++ 一函数拿 USTRUCT：FJSON2StructRest::RequestToStruct(URL, [](bool ok, const FString& RootTypeName, const void* Ptr){ ... })，
 *   一 URL 对应一根 .h，回调中根据 RootTypeName 强转为对应结构体，Ptr 仅回调内有效。
 * - C++ 原始 JSON：FJSON2StructRest::Request(URL, Callback)。
 * - Blueprint：调用 "Request JSON" 节点。
 */
#include "JSON2StructRest.h"
#include "JSON2StructBPLibrary.h"
#if __has_include("Generated/JSON2StructGeneratedBPApi.h")
#include "Generated/JSON2StructGeneratedBPApi.h"
#endif
