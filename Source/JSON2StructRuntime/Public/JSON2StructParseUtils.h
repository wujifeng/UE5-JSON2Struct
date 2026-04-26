// Copyright Epic Games, Inc. All Rights Reserved.
//Author WeChat: wujifeng_mr

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "JsonObjectConverter.h"
#include "JSON2StructRest.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace JSON2StructParseUtils
{
	template<typename StructType>
	static bool ParseJsonToStructAutoRoot(const FString& JsonText, StructType& OutData)
	{
		TSharedPtr<FJsonValue> RootValue;
		TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonText);
		if (!FJsonSerializer::Deserialize(JsonReader, RootValue) || !RootValue.IsValid())
		{
			return false;
		}

		if (RootValue->Type == EJson::Object)
		{
			const TSharedPtr<FJsonObject> JsonObject = RootValue->AsObject();
			return JsonObject.IsValid() && FJsonObjectConverter::JsonObjectToUStruct(JsonObject.ToSharedRef(), &OutData, 0, 0);
		}

		if (RootValue->Type == EJson::Array)
		{
			TSharedPtr<FJsonObject> WrapperObject = MakeShared<FJsonObject>();
			WrapperObject->SetArrayField(TEXT("items"), RootValue->AsArray());
			return FJsonObjectConverter::JsonObjectToUStruct(WrapperObject.ToSharedRef(), &OutData, 0, 0);
		}

		return false;
	}

	template<typename StructType, typename DelegateType>
	static void RequestAndParseBySpec(const FHTTPRequestParams& RequestParams, const DelegateType& OnComplete)
	{
		FJSON2StructRest::RequestBySpecWithStatus(RequestParams, FOnJSON2StructRequestCompleteWithStatus::CreateLambda(
			[OnComplete](const FString& ResponseContent, bool bSuccess, int32 HttpStatus)
			{
				StructType Data;
				if (bSuccess)
				{
					bSuccess = ParseJsonToStructAutoRoot(ResponseContent, Data);
				}

				if (bSuccess)
				{
					OnComplete.ExecuteIfBound(HttpStatus, Data);
				}
				else
				{
					const StructType EmptyData{};
					OnComplete.ExecuteIfBound(HttpStatus, EmptyData);
				}
			}));
	}
}

