// Copyright Epic Games, Inc. All Rights Reserved.
//Author WeChat: wujifeng_mr

#include "JSON2StructBPLibrary.h"
#include "JSON2StructRest.h"
#include "Dom/JsonObject.h"
#include "JsonObjectConverter.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/UnrealType.h"

void UJSON2StructBPLibrary::RequestJson(const FString& URL, FJSON2StructRequestCompleteDelegate OnComplete)
{
	FJSON2StructRest::Request(URL, FOnJSON2StructRequestComplete::CreateLambda(
		[OnComplete](const FString& ResponseContent, bool bSuccess)
		{
			OnComplete.ExecuteIfBound(bSuccess, ResponseContent);
		}));
}

void UJSON2StructBPLibrary::HTTPRequestToStruct(const FHTTPRequestParams& RequestParams, FJSON2StructRequestToStructCompleteDelegate OnComplete)
{
	FJSON2StructRest::RequestBySpec(RequestParams, FOnJSON2StructRequestComplete::CreateLambda(
		[RequestParams, OnComplete](const FString& ResponseContent, bool bSuccess)
		{
			if (!bSuccess || ResponseContent.IsEmpty())
			{
				OnComplete.ExecuteIfBound(false, ResponseContent, TEXT(""));
				return;
			}

			const FString DispatchKey = FJSON2StructRest::BuildDispatchKey(RequestParams);
			const FString LegacyUrl = RequestParams.URL;
			FString RootTypeName;
			bool bParsed = false;
			JSON2StructDispatchParse(DispatchKey, ResponseContent, [&RootTypeName, &bParsed](bool bParseSuccess, const FString& InRootTypeName, const void* StructPtr)
			{
				if (bParseSuccess && StructPtr != nullptr)
				{
					bParsed = true;
					RootTypeName = InRootTypeName;
				}
			});
			if (!bParsed)
			{
				JSON2StructDispatchParse(LegacyUrl, ResponseContent, [&RootTypeName, &bParsed](bool bParseSuccess, const FString& InRootTypeName, const void* StructPtr)
				{
					if (bParseSuccess && StructPtr != nullptr)
					{
						bParsed = true;
						RootTypeName = InRootTypeName;
					}
				});
			}

			OnComplete.ExecuteIfBound(bParsed, ResponseContent, RootTypeName);
		}));
}

bool UJSON2StructBPLibrary::JsonToStruct(const FString& JsonString, int32& OutStruct)
{
	(void)JsonString;
	(void)OutStruct;
	return false;
}

bool UJSON2StructBPLibrary::StructToJson(const int32& InStruct, FString& OutJsonString)
{
	(void)InStruct;
	OutJsonString.Reset();
	return false;
}

bool UJSON2StructBPLibrary::JsonToStructImpl(const FString& JsonString, const UScriptStruct* ScriptStruct, void* OutStructPtr)
{
	if (!ScriptStruct || !OutStructPtr || JsonString.IsEmpty())
	{
		return false;
	}

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonString);
	if (!FJsonSerializer::Deserialize(JsonReader, JsonObject) || !JsonObject.IsValid())
	{
		return false;
	}

	return FJsonObjectConverter::JsonObjectToUStruct(JsonObject.ToSharedRef(), ScriptStruct, OutStructPtr, 0, 0);
}

bool UJSON2StructBPLibrary::StructToJsonImpl(const UScriptStruct* ScriptStruct, const void* InStructPtr, FString& OutJsonString)
{
	OutJsonString.Reset();
	if (!ScriptStruct || !InStructPtr)
	{
		return false;
	}
	return FJsonObjectConverter::UStructToJsonObjectString(ScriptStruct, InStructPtr, OutJsonString, 0, 0, 0, nullptr, false);
}

DEFINE_FUNCTION(UJSON2StructBPLibrary::execJsonToStruct)
{
	P_GET_PROPERTY(FStrProperty, Z_Param_JsonString);
	Stack.StepCompiledIn<FStructProperty>(nullptr);
	void* OutStructPtr = Stack.MostRecentPropertyAddress;
	const FStructProperty* StructProperty = CastField<FStructProperty>(Stack.MostRecentProperty);
	P_FINISH;

	bool bSuccess = false;
	if (StructProperty && OutStructPtr)
	{
		bSuccess = UJSON2StructBPLibrary::JsonToStructImpl(Z_Param_JsonString, StructProperty->Struct, OutStructPtr);
	}
	*static_cast<bool*>(RESULT_PARAM) = bSuccess;
}

DEFINE_FUNCTION(UJSON2StructBPLibrary::execStructToJson)
{
	Stack.StepCompiledIn<FStructProperty>(nullptr);
	const void* InStructPtr = Stack.MostRecentPropertyAddress;
	const FStructProperty* StructProperty = CastField<FStructProperty>(Stack.MostRecentProperty);
	P_GET_PROPERTY_REF(FStrProperty, Z_Param_Out_OutJsonString);
	P_FINISH;

	const bool bSuccess = UJSON2StructBPLibrary::StructToJsonImpl(
		StructProperty ? StructProperty->Struct : nullptr,
		InStructPtr,
		Z_Param_Out_OutJsonString);
	*static_cast<bool*>(RESULT_PARAM) = bSuccess;
}

