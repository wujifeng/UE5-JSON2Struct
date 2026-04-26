//Author WeChat: wujifeng_mr
// Auto-generated per-interface Blueprint API.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "JSON2StructRequestTypes.h"
#include "GET_v1_forecast_v1_forecast.h"
#include "JSON2StructGeneratedBPApi_GET_v1_forecast.generated.h"

DECLARE_DYNAMIC_DELEGATE_TwoParams(
	FJSON2StructRequest_GET_v1_forecast_Delegate,
	int32, HttpStatus,
	const FGET_v1_forecast_v1_forecast&, Data);

UCLASS(meta = (DisplayName = "JSON2Struct Generated API"))
class JSON2STRUCTRUNTIME_API UJSON2StructGeneratedBPApi_GET_v1_forecast : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "JSON2Struct", meta = (DisplayName = "Request GET GET_v1_forecast_v1_forecast", AutoCreateRefTerm = "BodyJson,QueryParams,Headers", CPP_Default_URL = "https://api.open-meteo.com/v1/forecast?latitude=22.54&longitude=114.07&current_weather=true", CPP_Default_Method = "GET", CPP_Default_BodyJson = "", CPP_Default_TimeoutSeconds = "30.0"))
	static void Request_GET_v1_forecast(
		const FString& URL,
		EJSON2StructHttpMethod Method,
		const FString& BodyJson,
		const TMap<FString, FString>& QueryParams,
		const TMap<FString, FString>& Headers,
		float TimeoutSeconds,
		FJSON2StructRequest_GET_v1_forecast_Delegate OnComplete);
};
