// Copyright Epic Games, Inc. All Rights Reserved.
//Author WeChat: wujifeng_mr

#pragma once

#include "CoreMinimal.h"
#include "JSON2StructRequestTypes.generated.h"

UENUM(BlueprintType)
enum class EJSON2StructHttpMethod : uint8
{
	GET		UMETA(DisplayName = "GET"),
	POST	UMETA(DisplayName = "POST"),
	PUT		UMETA(DisplayName = "PUT"),
	DELETE	UMETA(DisplayName = "DELETE"),
	PATCH	UMETA(DisplayName = "PATCH"),
	HEAD	UMETA(DisplayName = "HEAD"),
	OPTIONS	UMETA(DisplayName = "OPTIONS")
};

USTRUCT(BlueprintType)
struct JSON2STRUCTRUNTIME_API FHTTPRequestParams
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JSON2Struct|Request")
	FString URL;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JSON2Struct|Request")
	EJSON2StructHttpMethod Method = EJSON2StructHttpMethod::GET;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JSON2Struct|Request")
	FString BodyJson;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JSON2Struct|Request")
	TMap<FString, FString> QueryParams;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JSON2Struct|Request")
	TMap<FString, FString> Headers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "JSON2Struct|Request", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float TimeoutSeconds = 30.0f;
};
