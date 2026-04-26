//Author WeChat: wujifeng_mr
// Auto-generated per-interface Blueprint API.

#include "Generated/GET_posts_1/JSON2StructGeneratedBPApi_GET_posts_1.h"
#include "JSON2StructRest.h"
#include "JSON2StructParseUtils.h"

void UJSON2StructGeneratedBPApi_GET_posts_1::Request_GET_posts_1(
	const FString& URL,
	EJSON2StructHttpMethod Method,
	const FString& BodyJson,
	const TMap<FString, FString>& QueryParams,
	const TMap<FString, FString>& Headers,
	float TimeoutSeconds,
	FJSON2StructRequest_GET_posts_1_Delegate OnComplete)
{
	FHTTPRequestParams EffectiveParams;
	EffectiveParams.URL = URL;
	EffectiveParams.Method = Method;
	EffectiveParams.BodyJson = BodyJson;
	EffectiveParams.QueryParams = QueryParams;
	EffectiveParams.Headers = Headers;
	EffectiveParams.TimeoutSeconds = TimeoutSeconds;

	JSON2StructParseUtils::RequestAndParseBySpec<FGET_posts_1_posts_1>(EffectiveParams, OnComplete);
}
