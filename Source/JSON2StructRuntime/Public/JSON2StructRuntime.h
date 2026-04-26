// Copyright Epic Games, Inc. All Rights Reserved.
//Author WeChat: wujifeng_mr

#pragma once

#include "Modules/ModuleManager.h"

/** 运行时模块：无界面，仅提供 Rest API、Blueprint 请求与生成的 USTRUCT；编辑器界面与生成逻辑在 JSON2Struct 模块 */
class FJSON2StructRuntimeModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
