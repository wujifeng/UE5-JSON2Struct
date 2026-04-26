// Copyright Epic Games, Inc. All Rights Reserved.
//Author WeChat: wujifeng_mr

#include "JSON2StructEditorGenerator.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace JSON2StructPanel
{
	static FString SanitizeStructName(const FString& Name)
	{
		FString Out;
		for (int32 i = 0; i < Name.Len(); ++i)
		{
			TCHAR c = Name[i];
			if (FChar::IsAlpha(c) || (i > 0 && FChar::IsDigit(c)))
				Out += c;
			else if (c == '_' || c == '-' || c == ' ' || c == '.')
				Out += TEXT('_');
		}
		if (Out.IsEmpty() || FChar::IsDigit(Out[0]))
			Out = TEXT("Field") + Out;
		return Out;
	}

	static FString ComputeStructSignature(const TMap<FString, FString>& FieldNameToType)
	{
		TArray<FString> Pairs;
		for (const auto& Kv : FieldNameToType)
			Pairs.Add(FString::Printf(TEXT("%s:%s"), *Kv.Key, *Kv.Value));
		Pairs.Sort();
		return FString::Join(Pairs, TEXT("|"));
	}

	static bool IsBuiltinUEType(const FString& TypeStr)
	{
		return TypeStr == TEXT("FString")
			|| TypeStr == TEXT("double")
			|| TypeStr == TEXT("bool")
			|| TypeStr == TEXT("int32")
			|| TypeStr == TEXT("int64");
	}

	static FString NormalizeStoredTypeName(const FString& TypeStr)
	{
		const FString Trimmed = TypeStr.TrimStartAndEnd();
		if (Trimmed.StartsWith(TEXT("TArray<")) && Trimmed.EndsWith(TEXT(">")))
		{
			const FString Inner = Trimmed.Mid(7, Trimmed.Len() - 8);
			return FString::Printf(TEXT("TArray<%s>"), *NormalizeStoredTypeName(Inner));
		}
		if (IsBuiltinUEType(Trimmed))
		{
			return Trimmed;
		}
		if (Trimmed.StartsWith(TEXT("F")) && Trimmed.Len() > 1)
		{
			return Trimmed.Mid(1);
		}
		return Trimmed;
	}

	struct FStructGenContext
	{
		TMap<FString, TMap<FString, FString>> StructDefinitions;
		TMap<FString, FString> StructSignatures;
		FString RootStructName;
		FString RootNameOverride;
		FString InterfacePrefix;

		FString MakeUniqueStructName(const FString& BaseName)
		{
			FString Sanitized = SanitizeStructName(BaseName);
			if (!InterfacePrefix.IsEmpty())
			{
				Sanitized = InterfacePrefix + TEXT("_") + Sanitized;
			}
			FString Candidate = Sanitized;
			int32 Suffix = 0;
			while (StructDefinitions.Contains(Candidate))
				Candidate = Sanitized + FString::Printf(TEXT("_%d"), ++Suffix);
			return Candidate;
		}
	};

	static FString JsonTypeToUE(const TSharedPtr<FJsonValue>& Value, FStructGenContext& Ctx, const FString& ParentPath, const FString& FieldName);

	static FString ParseUrlToRootName(const FString& Url)
	{
		FString Trimmed = Url.TrimStartAndEnd();
		if (Trimmed.IsEmpty()) return FString();
		int32 SchemeEnd = Trimmed.Find(TEXT("://"));
		if (SchemeEnd != INDEX_NONE)
			Trimmed = Trimmed.Mid(SchemeEnd + 3);
		int32 PathStart = Trimmed.Find(TEXT("/"));
		FString PathPart;
		if (PathStart != INDEX_NONE)
			PathPart = Trimmed.Mid(PathStart + 1);
		else
			PathPart = Trimmed;
		int32 QueryPos = PathPart.Find(TEXT("?"));
		if (QueryPos != INDEX_NONE)
		{
			PathPart = PathPart.Left(QueryPos);
		}
		int32 FragmentPos = PathPart.Find(TEXT("#"));
		if (FragmentPos != INDEX_NONE)
		{
			PathPart = PathPart.Left(FragmentPos);
		}
		FString Out;
		for (int32 i = 0; i < PathPart.Len(); ++i)
		{
			TCHAR c = PathPart[i];
			if (c == '/' || c == '.')
				Out += TEXT('_');
			else if (FChar::IsAlpha(c) || (i > 0 && FChar::IsDigit(c)) || c == '_' || c == '-')
				Out += c;
		}
		Out = Out.TrimStartAndEnd();
		while (Out.StartsWith(TEXT("_"))) Out = Out.Mid(1);
		while (Out.EndsWith(TEXT("_"))) Out = Out.Left(Out.Len() - 1);
		if (Out.IsEmpty() || FChar::IsDigit(Out[0]))
			Out = TEXT("Api") + Out;
		return Out;
	}

	static FString VisitJsonObject(const TSharedPtr<FJsonObject>& Obj, FStructGenContext& Ctx, const FString& RelativePath)
	{
		FString BaseName = RelativePath.IsEmpty() ? (Ctx.RootNameOverride.IsEmpty() ? TEXT("JsonRoot") : Ctx.RootNameOverride) : RelativePath;
		TMap<FString, FString> Fields;
		for (const auto& Kv : Obj->Values)
		{
			FString FieldSanitized = SanitizeStructName(Kv.Key);
			FString FieldType = JsonTypeToUE(Kv.Value, Ctx, RelativePath, Kv.Key);
			Fields.Add(FieldSanitized, FieldType);
		}
		const FString Signature = ComputeStructSignature(Fields);
		if (!RelativePath.IsEmpty())
		{
			for (const auto& Existing : Ctx.StructSignatures)
			{
				if (Existing.Value == Signature)
				{
					return Existing.Key;
				}
			}
		}
		FString StructName = Ctx.MakeUniqueStructName(BaseName);
		Ctx.StructDefinitions.Add(StructName, Fields);
		Ctx.StructSignatures.Add(StructName, Signature);
		return StructName;
	}

	static FString JsonTypeToUE(const TSharedPtr<FJsonValue>& Value, FStructGenContext& Ctx, const FString& ParentPath, const FString& FieldName)
	{
		if (!Value.IsValid()) return TEXT("FString");
		switch (Value->Type)
		{
		case EJson::String:
			return TEXT("FString");
		case EJson::Number:
			return TEXT("double");
		case EJson::Boolean:
			return TEXT("bool");
		case EJson::Object:
			{
				FString ChildPath;
				if (!ParentPath.IsEmpty() && FieldName.EndsWith(TEXT("_Elem")))
				{
					ChildPath = ParentPath;
				}
				else
				{
					ChildPath = ParentPath.IsEmpty() ? SanitizeStructName(FieldName) : (ParentPath + TEXT("_") + SanitizeStructName(FieldName));
				}
				return VisitJsonObject(Value->AsObject(), Ctx, ChildPath);
			}
		case EJson::Array:
			{
				const TArray<TSharedPtr<FJsonValue>>& Arr = Value->AsArray();
				if (Arr.Num() == 0)
					return TEXT("TArray<FString>");
				FString ElemRelativePath = ParentPath.IsEmpty() ? (SanitizeStructName(FieldName) + TEXT("_Elem")) : (ParentPath + TEXT("_") + SanitizeStructName(FieldName) + TEXT("_Elem"));
				FString ElemType = JsonTypeToUE(Arr[0], Ctx, ElemRelativePath, SanitizeStructName(FieldName) + TEXT("_Elem"));
				return FString::Printf(TEXT("TArray<%s>"), *ElemType);
			}
		case EJson::Null:
		default:
			return TEXT("FString");
		}
	}

	static void GatherStructSet(const FString& TypeStr, TSet<FString>& Out)
	{
		if (TypeStr.StartsWith(TEXT("TArray<")) && TypeStr.EndsWith(TEXT(">")))
		{
			FString Inner = TypeStr.Mid(7, TypeStr.Len() - 8);
			if (Inner != TEXT("FString") && Inner != TEXT("double") && Inner != TEXT("bool") && Inner != TEXT("int32") && Inner != TEXT("int64"))
				Out.Add(Inner);
		}
		else if (TypeStr != TEXT("FString") && TypeStr != TEXT("double") && TypeStr != TEXT("bool") && TypeStr != TEXT("int32") && TypeStr != TEXT("int64"))
			Out.Add(TypeStr);
	}

	static TArray<FString> GetStructDependencies(const TMap<FString, FString>& Fields)
	{
		TSet<FString> Deps;
		for (const auto& Kv : Fields)
			GatherStructSet(Kv.Value, Deps);
		return Deps.Array();
	}

	static TArray<FString> TopoSortStructs(const TMap<FString, TMap<FString, FString>>& StructDefs)
	{
		TArray<FString> Sorted;
		TSet<FString> Visited;
		TFunction<void(const FString&)> Visit = [&](const FString& Name)
		{
			if (Visited.Contains(Name)) return;
			Visited.Add(Name);
			const TMap<FString, FString>* Fields = StructDefs.Find(Name);
			if (Fields)
			{
				for (const FString& Dep : GetStructDependencies(*Fields))
					if (StructDefs.Contains(Dep)) Visit(Dep);
			}
			Sorted.Add(Name);
		};
		for (const auto& Kv : StructDefs)
			Visit(Kv.Key);
		return Sorted;
	}

	static FString GetStructFileName(const FString& StructName)
	{
		FString Name = StructName.StartsWith(TEXT("F")) ? StructName.Mid(1) : StructName;
		return Name + TEXT(".h");
	}

	static bool ParseOneStructFieldsFromContent(const FString& Content, const FString& StructName, TMap<FString, FString>& OutFields)
	{
		OutFields.Empty();
		FString SearchStruct = StructName.StartsWith(TEXT("F")) ? StructName : TEXT("F") + StructName;
		int32 BlockStart = Content.Find(TEXT("struct ") + SearchStruct, ESearchCase::CaseSensitive);
		if (BlockStart == INDEX_NONE) return false;
		int32 BlockEnd = Content.Find(TEXT("};"), ESearchCase::CaseSensitive, ESearchDir::FromStart, BlockStart);
		if (BlockEnd == INDEX_NONE) return false;
		FString Block = Content.Mid(BlockStart, BlockEnd + 2 - BlockStart);
		int32 j = 0;
		while ((j = Block.Find(TEXT("UPROPERTY"), ESearchCase::CaseSensitive, ESearchDir::FromStart, j)) != INDEX_NONE)
		{
			int32 LineEnd = Block.Find(TEXT("\n"), ESearchCase::CaseSensitive, ESearchDir::FromStart, j);
			if (LineEnd == INDEX_NONE) break;
			int32 NextLine = LineEnd + 1;
			int32 TypeEnd = Block.Find(TEXT(";"), ESearchCase::CaseSensitive, ESearchDir::FromStart, NextLine);
			if (TypeEnd == INDEX_NONE) { j = NextLine; continue; }
			FString Line = Block.Mid(NextLine, TypeEnd - NextLine).TrimStartAndEnd();
			int32 EqualPos = INDEX_NONE;
			if (Line.FindChar(TEXT('='), EqualPos))
			{
				Line = Line.Left(EqualPos).TrimEnd();
			}
			TArray<FString> Parts;
			Line.ParseIntoArray(Parts, TEXT(" "), true);
			if (Parts.Num() >= 2)
			{
				const FString ParsedFieldName = Parts.Last();
				const FString ParsedFieldType = Parts[Parts.Num() - 2];
				const bool bFieldNameValid = !ParsedFieldName.IsEmpty()
					&& (FChar::IsAlpha(ParsedFieldName[0]) || ParsedFieldName[0] == TEXT('_'))
					&& ParsedFieldName.Find(TEXT(".")) == INDEX_NONE;
				const bool bTypeValid = !ParsedFieldType.IsEmpty() && ParsedFieldType != TEXT("=");
				if (bFieldNameValid && bTypeValid)
				{
					OutFields.Add(ParsedFieldName, NormalizeStoredTypeName(ParsedFieldType));
				}
			}
			j = TypeEnd + 1;
		}
		return true;
	}

	static TMap<FString, FString> MergeFieldsAddOnly(const TMap<FString, FString>& Existing, const TMap<FString, FString>& New)
	{
		TMap<FString, FString> Out = Existing;
		for (const auto& Kv : New)
		{
			if (!Out.Contains(Kv.Key))
				Out.Add(Kv.Key, Kv.Value);
		}
		return Out;
	}

	static FString GetDefaultInitForType(const FString& UEType)
	{
		if (UEType == TEXT("double")) return TEXT(" = 0.0");
		if (UEType == TEXT("int32") || UEType == TEXT("int64")) return TEXT(" = 0");
		if (UEType == TEXT("bool")) return TEXT(" = false");
		return FString();
	}

	static FString EmitTypeName(const FString& TypeStr)
	{
		if (TypeStr == TEXT("FString") || TypeStr == TEXT("double") || TypeStr == TEXT("bool") || TypeStr == TEXT("int32") || TypeStr == TEXT("int64"))
			return TypeStr;
		if (TypeStr.StartsWith(TEXT("TArray<")) && TypeStr.EndsWith(TEXT(">")))
		{
			FString Inner = TypeStr.Mid(7, TypeStr.Len() - 8);
			return FString::Printf(TEXT("TArray<%s>"), *EmitTypeName(Inner));
		}
		return (TypeStr.StartsWith(TEXT("F")) ? TypeStr : TEXT("F") + TypeStr);
	}

	static FString BuildOneStructBlock(const FString& StructName, const TMap<FString, FString>& Fields)
	{
		FString DisplayName = StructName.StartsWith(TEXT("F")) ? StructName : TEXT("F") + StructName;
		FString Block = FString::Printf(TEXT("USTRUCT(BlueprintType)\nstruct %s\n{\n\tGENERATED_BODY()\n\n"), *DisplayName);
		for (const auto& Kv : Fields)
			Block += FString::Printf(TEXT("\tUPROPERTY(EditAnywhere, BlueprintReadWrite)\n\t%s %s%s;\n\n"), *EmitTypeName(Kv.Value), *Kv.Key, *GetDefaultInitForType(Kv.Value));
		Block += TEXT("};\n\n");
		return Block;
	}

	static TArray<FString> GetDependencyIncludeFiles(const TMap<FString, FString>& Fields, const TFunction<FString(const FString&)>& ResolveType, const TSet<FString>& AllStructNames)
	{
		TSet<FString> Deps;
		for (const auto& Kv : Fields)
			GatherStructSet(ResolveType(Kv.Value), Deps);
		TArray<FString> Includes;
		for (const FString& Dep : Deps)
		{
			if (AllStructNames.Contains(Dep))
				Includes.AddUnique(GetStructFileName(Dep));
		}
		return Includes;
	}

	static FString BuildSingleStructHeaderContent(
		const FString& StructName,
		const TMap<FString, FString>& Fields,
		const TFunction<FString(const FString&)>& ResolveType,
		const TSet<FString>& AllStructNames)
	{
		FString BaseName = StructName.StartsWith(TEXT("F")) ? StructName.Mid(1) : StructName;
		FString Out = TEXT("// Auto-generated by JSON2Struct (one struct per file)\n");
		Out += TEXT("#pragma once\n\n");
		Out += TEXT("#include \"CoreMinimal.h\"\n");
		TArray<FString> DepIncludes = GetDependencyIncludeFiles(Fields, ResolveType, AllStructNames);
		for (const FString& Inc : DepIncludes)
			Out += FString::Printf(TEXT("#include \"%s\"\n"), *Inc);
		Out += FString::Printf(TEXT("#include \"%s.generated.h\"\n\n"), *BaseName);
		Out += BuildOneStructBlock(StructName, Fields);
		return Out;
	}

	bool GenerateStructureToFolder(
		const FString& JsonText,
		const FString& OutputDir,
		bool bMergeWithExisting,
		const FString& RequestUrl,
		const FString& RequestMethod,
		TArray<FJSON2StructGeneratedItem>& OutGeneratedItems,
		TFunction<void(const FString&)> AppendLog)
	{
		OutGeneratedItems.Reset();
		TSharedPtr<FJsonObject> RootObj;
		TSharedPtr<FJsonValue> RootValue;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
		if (!FJsonSerializer::Deserialize(Reader, RootValue) || !RootValue.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("JSON2Struct: JSON 解析失败，请检查 JSON 格式"));
			AppendLog(TEXT("JSON 解析失败"));
			return false;
		}
		if (RootValue->Type == EJson::Object)
		{
			RootObj = RootValue->AsObject();
		}
		else if (RootValue->Type == EJson::Array)
		{
			RootObj = MakeShared<FJsonObject>();
			RootObj->SetArrayField(TEXT("items"), RootValue->AsArray());
			AppendLog(TEXT("检测到根数组，已自动包装为 {\"items\": [...]} 进行结构体生成"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("JSON2Struct: 仅支持根对象或根数组"));
			AppendLog(TEXT("JSON 解析失败：仅支持根对象或根数组"));
			return false;
		}
		if (!RootObj.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("JSON2Struct: 根对象无效"));
			AppendLog(TEXT("JSON 解析失败：根对象无效"));
			return false;
		}

		FStructGenContext Ctx;
		const FString EffectiveMethod = RequestMethod.IsEmpty() ? TEXT("GET") : RequestMethod.ToUpper();
		Ctx.InterfacePrefix = SanitizeStructName(EffectiveMethod + TEXT("_") + ParseUrlToRootName(RequestUrl));
		Ctx.RootNameOverride = ParseUrlToRootName(RequestUrl);
		Ctx.RootStructName = VisitJsonObject(RootObj, Ctx, TEXT(""));
		if (Ctx.RootNameOverride.IsEmpty())
		{
			AppendLog(TEXT("未提供请求 URL，根结构体使用 JsonRoot"));
		}
		else
		{
			AppendLog(FString::Printf(TEXT("根结构体（由 URL 推导）: %s"), *Ctx.RootStructName));
		}
		AppendLog(FString::Printf(TEXT("输出目录: %s"), *OutputDir));
		AppendLog(FString::Printf(TEXT("根=%s, 结构体数=%d"), *Ctx.RootStructName, Ctx.StructDefinitions.Num()));
		UE_LOG(LogTemp, Log, TEXT("JSON2Struct: 输出目录=%s, 根=%s, 结构体数=%d"), *OutputDir, *Ctx.RootStructName, Ctx.StructDefinitions.Num());

		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		if (!PlatformFile.DirectoryExists(*OutputDir))
		{
			if (!PlatformFile.CreateDirectoryTree(*OutputDir))
			{
				UE_LOG(LogTemp, Error, TEXT("JSON2Struct: 无法创建目录: %s"), *OutputDir);
				AppendLog(FString::Printf(TEXT("无法创建目录: %s"), *OutputDir));
				return false;
			}
			AppendLog(FString::Printf(TEXT("已创建目录: %s"), *OutputDir));
		}

		TSet<FString> AllStructNames;
		for (const auto& Kv : Ctx.StructDefinitions)
			AllStructNames.Add(Kv.Key);

		auto ResolveType = [](const FString& TypeStr) -> FString { return TypeStr; };

		TArray<FString> Order = TopoSortStructs(Ctx.StructDefinitions);
		if (Order.Num() == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("JSON2Struct: 拓扑排序后结构体列表为空"));
			AppendLog(TEXT("错误: 未解析出任何结构体"));
			return false;
		}
		for (const FString& StructName : Order)
		{
			FString FileName = GetStructFileName(StructName);
			FString HeaderPath = FPaths::Combine(OutputDir, FileName);

			TMap<FString, FString> MergedFields = Ctx.StructDefinitions.FindRef(StructName);
			if (bMergeWithExisting)
			{
				FString ExistingContent;
				if (FFileHelper::LoadFileToString(ExistingContent, *HeaderPath))
				{
					TMap<FString, FString> ExistingFields;
					FString SearchName = StructName.StartsWith(TEXT("F")) ? StructName : TEXT("F") + StructName;
					if (ParseOneStructFieldsFromContent(ExistingContent, SearchName, ExistingFields))
						MergedFields = MergeFieldsAddOnly(ExistingFields, MergedFields);
				}
			}

			FString NewContent = BuildSingleStructHeaderContent(StructName, MergedFields, ResolveType, AllStructNames);
			if (StructName == Ctx.RootStructName && !RequestUrl.IsEmpty())
			{
				FString UrlNameSuffix = ParseUrlToRootName(RequestUrl);
				if (UrlNameSuffix.IsEmpty())
				{
					UrlNameSuffix = TEXT("Api");
				}
				FString MethodPrefix = RequestMethod.IsEmpty() ? TEXT("GET") : RequestMethod.ToUpper();
				FString UniqueUrlVarName = FString::Printf(TEXT("JSON2StructRootURL_%s_%s"), *MethodPrefix, *UrlNameSuffix);
				UniqueUrlVarName = SanitizeStructName(UniqueUrlVarName);
				FString EscapedUrl = RequestUrl.Replace(TEXT("\\"), TEXT("\\\\")).Replace(TEXT("\""), TEXT("\\\""));
				FString UrlBlock = FString::Printf(
					TEXT("// JSON2Struct URL: %s\nstatic const TCHAR* const %s = TEXT(\"%s\");\n\n"),
					*RequestUrl, *UniqueUrlVarName, *EscapedUrl);
				int32 InsertPos = NewContent.Find(TEXT("#pragma once"));
				if (InsertPos != INDEX_NONE)
				{
					int32 AfterPragma = NewContent.Find(TEXT("\n\n"), ESearchCase::IgnoreCase, ESearchDir::FromStart, InsertPos);
					if (AfterPragma != INDEX_NONE) AfterPragma += 2;
					else AfterPragma = InsertPos + 12;
					NewContent = NewContent.Left(AfterPragma) + UrlBlock + NewContent.Mid(AfterPragma);
				}
			}
			if (!FFileHelper::SaveStringToFile(NewContent, *HeaderPath))
			{
				UE_LOG(LogTemp, Error, TEXT("JSON2Struct: 写入失败 %s"), *HeaderPath);
				AppendLog(FString::Printf(TEXT("写入失败: %s"), *HeaderPath));
				return false;
			}
			OutGeneratedItems.Add({ EJSON2StructGeneratedType::Structure, FileName, HeaderPath });
			AppendLog(FString::Printf(TEXT("已生成: %s"), *HeaderPath));
		}

		auto SanitizeIdentifier = [](FString In) -> FString
		{
			FString Out;
			for (int32 i = 0; i < In.Len(); ++i)
			{
				const TCHAR C = In[i];
				if (FChar::IsAlpha(C) || FChar::IsDigit(C) || C == TEXT('_'))
				{
					Out += C;
				}
				else
				{
					Out += TEXT('_');
				}
			}
			while (Out.Contains(TEXT("__")))
			{
				Out = Out.Replace(TEXT("__"), TEXT("_"));
			}
			if (Out.IsEmpty() || FChar::IsDigit(Out[0]))
			{
				Out = TEXT("Req_") + Out;
			}
			return Out;
		};
		auto RootTypeToReadableName = [](const FString& RootType) -> FString
		{
			if (RootType.StartsWith(TEXT("F")) && RootType.Len() > 1)
			{
				return RootType.Mid(1);
			}
			return RootType;
		};

		const FString RootTypeName = Ctx.RootStructName.StartsWith(TEXT("F")) ? Ctx.RootStructName : (TEXT("F") + Ctx.RootStructName);
		const FString InterfaceFolderName = SanitizeIdentifier(EffectiveMethod + TEXT("_") + ParseUrlToRootName(RequestUrl));
		const FString InterfacePublicDir = FPaths::Combine(OutputDir, InterfaceFolderName);
		if (PlatformFile.DirectoryExists(*InterfacePublicDir))
		{
			if (!PlatformFile.DeleteDirectoryRecursively(*InterfacePublicDir))
			{
				AppendLog(FString::Printf(TEXT("无法清理旧接口目录: %s"), *InterfacePublicDir));
				return false;
			}
			AppendLog(FString::Printf(TEXT("已删除旧接口目录: %s"), *InterfacePublicDir));
		}
		if (!PlatformFile.CreateDirectoryTree(*InterfacePublicDir))
		{
			AppendLog(FString::Printf(TEXT("无法创建接口目录: %s"), *InterfacePublicDir));
			return false;
		}
		AppendLog(FString::Printf(TEXT("接口目录: %s"), *InterfacePublicDir));

		for (const FString& StructName : Order)
		{
			const FString FileName = GetStructFileName(StructName);
			const FString OldPath = FPaths::Combine(OutputDir, FileName);
			const FString NewPath = FPaths::Combine(InterfacePublicDir, FileName);
			if (!OldPath.Equals(NewPath, ESearchCase::IgnoreCase) && IFileManager::Get().FileExists(*OldPath))
			{
				IFileManager::Get().Move(*NewPath, *OldPath, true, true, false, false);
			}
			OutGeneratedItems.Add({ EJSON2StructGeneratedType::Structure, FileName, NewPath });
		}

		// 兼容清理历史版本写入的 Private/Generated 接口目录，新的生成统一写入 Public/Generated
		const FString RuntimeModulePrivateDir = FPaths::Combine(FPaths::ProjectPluginsDir(), TEXT("JSON2Struct/Source/JSON2StructRuntime/Private/Generated"));
		const FString LegacyInterfacePrivateDir = FPaths::Combine(RuntimeModulePrivateDir, InterfaceFolderName);
		if (PlatformFile.DirectoryExists(*LegacyInterfacePrivateDir))
		{
			if (!PlatformFile.DeleteDirectoryRecursively(*LegacyInterfacePrivateDir))
			{
				AppendLog(FString::Printf(TEXT("无法清理旧接口私有目录: %s"), *LegacyInterfacePrivateDir));
				return false;
			}
			AppendLog(FString::Printf(TEXT("已清理历史私有目录: %s"), *LegacyInterfacePrivateDir));
		}

		const FString ReadableRootName = RootTypeToReadableName(RootTypeName);
		const FString RequestId = SanitizeIdentifier(InterfaceFolderName);
		const FString DelegateName = FString::Printf(TEXT("FJSON2StructRequest_%s_Delegate"), *RequestId);
		const FString FuncName = FString::Printf(TEXT("Request_%s"), *RequestId);
		const FString ApiBaseName = FString::Printf(TEXT("JSON2StructGeneratedBPApi_%s"), *RequestId);
		const FString BPApiHeaderPath = FPaths::Combine(InterfacePublicDir, ApiBaseName + TEXT(".h"));
		const FString BPApiCppPath = FPaths::Combine(InterfacePublicDir, ApiBaseName + TEXT(".cpp"));
		const FString EscapedUrl = RequestUrl.Replace(TEXT("\\"), TEXT("\\\\")).Replace(TEXT("\""), TEXT("\\\""));

		FString BPApiHeader;
		BPApiHeader += TEXT("// Auto-generated per-interface Blueprint API.\n#pragma once\n\n");
		BPApiHeader += TEXT("#include \"CoreMinimal.h\"\n");
		BPApiHeader += TEXT("#include \"Kismet/BlueprintFunctionLibrary.h\"\n");
		BPApiHeader += TEXT("#include \"JSON2StructRequestTypes.h\"\n");
		BPApiHeader += FString::Printf(TEXT("#include \"%s\"\n"), *GetStructFileName(RootTypeName));
		BPApiHeader += FString::Printf(TEXT("#include \"%s.generated.h\"\n\n"), *ApiBaseName);
		BPApiHeader += FString::Printf(
			TEXT("DECLARE_DYNAMIC_DELEGATE_TwoParams(\n\t%s,\n\tint32, HttpStatus,\n\tconst %s&, Data);\n\n"),
			*DelegateName, *RootTypeName);
		BPApiHeader += TEXT("UCLASS(meta = (DisplayName = \"JSON2Struct Generated API\"))\n");
		BPApiHeader += FString::Printf(TEXT("class JSON2STRUCTRUNTIME_API U%s : public UBlueprintFunctionLibrary\n{\n\tGENERATED_BODY()\n\npublic:\n"), *ApiBaseName);
		BPApiHeader += FString::Printf(
			TEXT("\tUFUNCTION(BlueprintCallable, Category = \"JSON2Struct\", meta = (DisplayName = \"Request %s %s\", AutoCreateRefTerm = \"BodyJson,QueryParams,Headers\", CPP_Default_URL = \"%s\", CPP_Default_Method = \"%s\", CPP_Default_BodyJson = \"\", CPP_Default_TimeoutSeconds = \"30.0\"))\n")
			TEXT("\tstatic void %s(\n")
			TEXT("\t\tconst FString& URL,\n")
			TEXT("\t\tEJSON2StructHttpMethod Method,\n")
			TEXT("\t\tconst FString& BodyJson,\n")
			TEXT("\t\tconst TMap<FString, FString>& QueryParams,\n")
			TEXT("\t\tconst TMap<FString, FString>& Headers,\n")
			TEXT("\t\tfloat TimeoutSeconds,\n")
			TEXT("\t\t%s OnComplete);\n")
			TEXT("};\n"),
			*EffectiveMethod, *ReadableRootName, *EscapedUrl, *EffectiveMethod, *FuncName, *DelegateName);

		FString BPApiCpp;
		BPApiCpp += TEXT("// Auto-generated per-interface Blueprint API.\n\n");
		BPApiCpp += FString::Printf(TEXT("#include \"Generated/%s/%s.h\"\n"), *InterfaceFolderName, *ApiBaseName);
		BPApiCpp += TEXT("#include \"JSON2StructRest.h\"\n");
		BPApiCpp += TEXT("#include \"JSON2StructParseUtils.h\"\n\n");
		BPApiCpp += FString::Printf(
			TEXT("void U%s::%s(\n")
			TEXT("\tconst FString& URL,\n")
			TEXT("\tEJSON2StructHttpMethod Method,\n")
			TEXT("\tconst FString& BodyJson,\n")
			TEXT("\tconst TMap<FString, FString>& QueryParams,\n")
			TEXT("\tconst TMap<FString, FString>& Headers,\n")
			TEXT("\tfloat TimeoutSeconds,\n")
			TEXT("\t%s OnComplete)\n{\n")
			TEXT("\tFHTTPRequestParams EffectiveParams;\n")
			TEXT("\tEffectiveParams.URL = URL;\n")
			TEXT("\tEffectiveParams.Method = Method;\n")
			TEXT("\tEffectiveParams.BodyJson = BodyJson;\n")
			TEXT("\tEffectiveParams.QueryParams = QueryParams;\n")
			TEXT("\tEffectiveParams.Headers = Headers;\n")
			TEXT("\tEffectiveParams.TimeoutSeconds = TimeoutSeconds;\n\n")
			TEXT("\tJSON2StructParseUtils::RequestAndParseBySpec<%s>(EffectiveParams, OnComplete);\n}\n"),
			*ApiBaseName, *FuncName, *DelegateName, *RootTypeName);

		if (!FFileHelper::SaveStringToFile(BPApiHeader, *BPApiHeaderPath))
		{
			AppendLog(FString::Printf(TEXT("写入失败: %s"), *BPApiHeaderPath));
			return false;
		}
		if (!FFileHelper::SaveStringToFile(BPApiCpp, *BPApiCppPath))
		{
			AppendLog(FString::Printf(TEXT("写入失败: %s"), *BPApiCppPath));
			return false;
		}
		OutGeneratedItems.Add({ EJSON2StructGeneratedType::Structure, FPaths::GetCleanFilename(BPApiHeaderPath), BPApiHeaderPath });
		OutGeneratedItems.Add({ EJSON2StructGeneratedType::Structure, FPaths::GetCleanFilename(BPApiCppPath), BPApiCppPath });
		AppendLog(FString::Printf(TEXT("已生成接口 BP API: %s"), *BPApiHeaderPath));
		AppendLog(FString::Printf(TEXT("已生成接口 BP API: %s"), *BPApiCppPath));

		AppendLog(FString::Printf(TEXT("生成完成，共 %d 个文件，目录=%s"), OutGeneratedItems.Num(), *OutputDir));
		UE_LOG(LogTemp, Log, TEXT("JSON2Struct: 生成完成，共 %d 个文件，目录=%s"), OutGeneratedItems.Num(), *OutputDir);
		return true;
	}
}

