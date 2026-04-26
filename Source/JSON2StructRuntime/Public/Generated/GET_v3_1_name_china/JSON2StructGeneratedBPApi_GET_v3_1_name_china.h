//Author WeChat: wujifeng_mr
// Auto-generated per-interface Blueprint API.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "JSON2StructRequestTypes.h"
#include "GET_v3_1_name_china_v3_1_name_china.h"
#include "JSON2StructGeneratedBPApi_GET_v3_1_name_china.generated.h"

DECLARE_DYNAMIC_DELEGATE_TwoParams(
	FJSON2StructRequest_GET_v3_1_name_china_Delegate,
	int32, HttpStatus,
	const FGET_v3_1_name_china_v3_1_name_china&, Data);

UCLASS(meta = (DisplayName = "JSON2Struct Generated API"))
class JSON2STRUCTRUNTIME_API UJSON2StructGeneratedBPApi_GET_v3_1_name_china : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "JSON2Struct", meta = (DisplayName = "Request GET GET_v3_1_name_china_v3_1_name_china", AutoCreateRefTerm = "BodyJson,QueryParams,Headers", CPP_Default_URL = "https://restcountries.com/v3.1/name/china", CPP_Default_Method = "GET", CPP_Default_BodyJson = "", CPP_Default_TimeoutSeconds = "30.0"))
	static void Request_GET_v3_1_name_china(
		const FString& URL,
		EJSON2StructHttpMethod Method,
		const FString& BodyJson,
		const TMap<FString, FString>& QueryParams,
		const TMap<FString, FString>& Headers,
		float TimeoutSeconds,
		FJSON2StructRequest_GET_v3_1_name_china_Delegate OnComplete);
};
