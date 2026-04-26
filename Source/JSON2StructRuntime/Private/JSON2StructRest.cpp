// Copyright Epic Games, Inc. All Rights Reserved.
//Author WeChat: wujifeng_mr

#include "JSON2StructRest.h"
#include "Http.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Async/Async.h"
#include "JsonObjectConverter.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	static FString HttpMethodToVerb(EJSON2StructHttpMethod Method)
	{
		switch (Method)
		{
		case EJSON2StructHttpMethod::POST: return TEXT("POST");
		case EJSON2StructHttpMethod::PUT: return TEXT("PUT");
		case EJSON2StructHttpMethod::DELETE: return TEXT("DELETE");
		case EJSON2StructHttpMethod::PATCH: return TEXT("PATCH");
		case EJSON2StructHttpMethod::HEAD: return TEXT("HEAD");
		case EJSON2StructHttpMethod::OPTIONS: return TEXT("OPTIONS");
		case EJSON2StructHttpMethod::GET:
		default:
			return TEXT("GET");
		}
	}

	static FString BuildUrlWithQuery(const FHTTPRequestParams& Spec)
	{
		if (Spec.QueryParams.Num() == 0)
		{
			return Spec.URL;
		}

		FString EncodedPairs;
		bool bFirst = true;
		for (const auto& Pair : Spec.QueryParams)
		{
			const FString EncodedKey = FGenericPlatformHttp::UrlEncode(Pair.Key);
			const FString EncodedValue = FGenericPlatformHttp::UrlEncode(Pair.Value);
			EncodedPairs += FString::Printf(TEXT("%s%s=%s"), bFirst ? TEXT("") : TEXT("&"), *EncodedKey, *EncodedValue);
			bFirst = false;
		}

		FString BaseUrl = Spec.URL;
		const TCHAR Separator = BaseUrl.Contains(TEXT("?")) ? TEXT('&') : TEXT('?');
		BaseUrl += FString::Printf(TEXT("%c%s"), Separator, *EncodedPairs);
		return BaseUrl;
	}

	static void BindAndProcessRequest(const FHTTPRequestParams& Spec, FOnJSON2StructRequestCompleteWithStatus OnComplete)
	{
		TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();
		HttpRequest->SetURL(BuildUrlWithQuery(Spec));
		HttpRequest->SetVerb(HttpMethodToVerb(Spec.Method));
		HttpRequest->SetTimeout(FMath::Max(1.0f, Spec.TimeoutSeconds));

		for (const auto& Header : Spec.Headers)
		{
			HttpRequest->SetHeader(Header.Key, Header.Value);
		}

		const FString Verb = HttpMethodToVerb(Spec.Method);
		const bool bCanHaveBody = (Verb == TEXT("POST") || Verb == TEXT("PUT") || Verb == TEXT("PATCH"));
		if (bCanHaveBody && !Spec.BodyJson.IsEmpty())
		{
			if (HttpRequest->GetHeader(TEXT("Content-Type")).IsEmpty())
			{
				HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json; charset=utf-8"));
			}
			HttpRequest->SetContentAsString(Spec.BodyJson);
		}

		HttpRequest->OnProcessRequestComplete().BindLambda(
			[OnComplete](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bSuccess)
			{
				FString Content;
				int32 HttpStatus = 0;
				if (bSuccess && Response.IsValid())
				{
					Content = Response->GetContentAsString();
					HttpStatus = Response->GetResponseCode();
					if (Response->GetResponseCode() < 200 || Response->GetResponseCode() >= 300)
					{
						bSuccess = false;
						if (Content.IsEmpty())
						{
							Content = FString::Printf(TEXT("HTTP %d"), Response->GetResponseCode());
						}
					}
				}
				else if (Content.IsEmpty() && Request.IsValid())
				{
					EHttpFailureReason Reason = Request->GetFailureReason();
					switch (Reason)
					{
					case EHttpFailureReason::ConnectionError:
						Content = TEXT("连接错误（请尝试 https:// 或检查网络）");
						break;
					case EHttpFailureReason::TimedOut:
						Content = TEXT("请求超时");
						break;
					case EHttpFailureReason::Cancelled:
						Content = TEXT("请求已取消");
						break;
					case EHttpFailureReason::Other:
					case EHttpFailureReason::None:
					default:
						Content = TEXT("请求失败（请尝试 https:// 或检查网络）");
						break;
					}
				}

				AsyncTask(ENamedThreads::GameThread, [OnComplete, Content, bSuccess, HttpStatus]()
				{
					OnComplete.ExecuteIfBound(Content, bSuccess, HttpStatus);
				});
			});

		HttpRequest->ProcessRequest();
	}
}

FString FJSON2StructRest::BuildDispatchKey(const FHTTPRequestParams& Spec)
{
	const FString Method = HttpMethodToVerb(Spec.Method);
	return FString::Printf(TEXT("%s %s"), *Method, *BuildUrlWithQuery(Spec));
}

void FJSON2StructRest::Request(const FString& URL, FOnJSON2StructRequestComplete OnComplete)
{
	FHTTPRequestParams Spec;
	Spec.URL = URL;
	RequestBySpec(Spec, OnComplete);
}

void FJSON2StructRest::RequestBySpec(const FHTTPRequestParams& Spec, FOnJSON2StructRequestComplete OnComplete)
{
	RequestBySpecWithStatus(Spec, FOnJSON2StructRequestCompleteWithStatus::CreateLambda(
		[OnComplete](const FString& Content, bool bSuccess, int32 HttpStatus)
		{
			(void)HttpStatus;
			OnComplete.ExecuteIfBound(Content, bSuccess);
		}));
}

void FJSON2StructRest::RequestBySpecWithStatus(const FHTTPRequestParams& Spec, FOnJSON2StructRequestCompleteWithStatus OnComplete)
{
	BindAndProcessRequest(Spec, OnComplete);
}

void FJSON2StructRest::RequestToStruct(const FString& URL, FOnRequestToStructComplete OnComplete)
{
	FHTTPRequestParams Spec;
	Spec.URL = URL;
	RequestToStructBySpec(Spec, OnComplete);
}

void FJSON2StructRest::RequestToStructBySpec(const FHTTPRequestParams& Spec, FOnRequestToStructComplete OnComplete)
{
	const FString DispatchKey = BuildDispatchKey(Spec);
	const FString LegacyUrl = BuildUrlWithQuery(Spec);
	RequestBySpec(Spec, FOnJSON2StructRequestComplete::CreateLambda(
		[DispatchKey, LegacyUrl, OnComplete](const FString& Content, bool bSuccess)
		{
			if (!bSuccess || Content.IsEmpty())
			{
				OnComplete(false, TEXT(""), nullptr);
				return;
			}

			bool bHandled = false;
			JSON2StructDispatchParse(DispatchKey, Content, [&OnComplete, &bHandled](bool bParseSuccess, const FString& RootTypeName, const void* StructPtr)
			{
				if (bParseSuccess)
				{
					bHandled = true;
					OnComplete(true, RootTypeName, StructPtr);
				}
			});
			if (bHandled)
			{
				return;
			}

			JSON2StructDispatchParse(LegacyUrl, Content, [&OnComplete, &bHandled](bool bParseSuccess, const FString& RootTypeName, const void* StructPtr)
			{
				if (bParseSuccess)
				{
					bHandled = true;
					OnComplete(true, RootTypeName, StructPtr);
				}
			});
			if (!bHandled)
			{
				OnComplete(false, TEXT(""), nullptr);
			}
		}));
}

#if __has_include("Generated/JSON2StructGenerated.inl")
#include "Generated/JSON2StructGenerated.inl"
#else
void JSON2StructDispatchParse(const FString& URL, const FString& Json, TFunction<void(bool bSuccess, const FString& RootTypeName, const void* StructPtr)> Callback)
{
	(void)URL;
	(void)Json;
	Callback(false, TEXT(""), nullptr);
}
#endif
