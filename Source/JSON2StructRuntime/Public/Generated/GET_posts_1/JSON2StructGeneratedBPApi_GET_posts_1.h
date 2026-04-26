//Author WeChat: wujifeng_mr
// Auto-generated per-interface Blueprint API.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "JSON2StructRequestTypes.h"
#include "GET_posts_1_posts_1.h"
#include "JSON2StructGeneratedBPApi_GET_posts_1.generated.h"

DECLARE_DYNAMIC_DELEGATE_TwoParams(
	FJSON2StructRequest_GET_posts_1_Delegate,
	int32, HttpStatus,
	const FGET_posts_1_posts_1&, Data);

UCLASS(meta = (DisplayName = "JSON2Struct Generated API"))
class JSON2STRUCTRUNTIME_API UJSON2StructGeneratedBPApi_GET_posts_1 : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "JSON2Struct", meta = (DisplayName = "Request GET GET_posts_1_posts_1", AutoCreateRefTerm = "BodyJson,QueryParams,Headers", CPP_Default_URL = "https://jsonplaceholder.typicode.com/posts/1", CPP_Default_Method = "GET", CPP_Default_BodyJson = "", CPP_Default_TimeoutSeconds = "30.0"))
	static void Request_GET_posts_1(
		const FString& URL,
		EJSON2StructHttpMethod Method,
		const FString& BodyJson,
		const TMap<FString, FString>& QueryParams,
		const TMap<FString, FString>& Headers,
		float TimeoutSeconds,
		FJSON2StructRequest_GET_posts_1_Delegate OnComplete);
};
