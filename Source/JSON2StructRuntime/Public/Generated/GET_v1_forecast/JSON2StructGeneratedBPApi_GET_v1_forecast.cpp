//Author WeChat: wujifeng_mr
// Auto-generated per-interface Blueprint API.

#include "Generated/GET_v1_forecast/JSON2StructGeneratedBPApi_GET_v1_forecast.h"
#include "JSON2StructRest.h"
#include "JSON2StructParseUtils.h"

void UJSON2StructGeneratedBPApi_GET_v1_forecast::Request_GET_v1_forecast(
	const FString& URL,
	EJSON2StructHttpMethod Method,
	const FString& BodyJson,
	const TMap<FString, FString>& QueryParams,
	const TMap<FString, FString>& Headers,
	float TimeoutSeconds,
	FJSON2StructRequest_GET_v1_forecast_Delegate OnComplete)
{
	FHTTPRequestParams EffectiveParams;
	EffectiveParams.URL = URL;
	EffectiveParams.Method = Method;
	EffectiveParams.BodyJson = BodyJson;
	EffectiveParams.QueryParams = QueryParams;
	EffectiveParams.Headers = Headers;
	EffectiveParams.TimeoutSeconds = TimeoutSeconds;

	JSON2StructParseUtils::RequestAndParseBySpec<FGET_v1_forecast_v1_forecast>(EffectiveParams, OnComplete);
}
