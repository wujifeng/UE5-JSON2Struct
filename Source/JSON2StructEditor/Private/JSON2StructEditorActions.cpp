// Copyright Epic Games, Inc. All Rights Reserved.
//Author WeChat: wujifeng_mr

#include "JSON2StructEditorActions.h"
#include "JSON2StructEditorGenerator.h"
#include "JSON2StructRest.h"
#include "JSON2StructSettings.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformFileManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Dom/JsonObject.h"
#include "Async/Async.h"
#include "Engine/Engine.h"

namespace JSON2StructPanel
{
	static bool FormatJsonString(const FString& Raw, FString& Out)
	{
		TSharedPtr<FJsonObject> Obj;
		TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Raw);
		if (!FJsonSerializer::Deserialize(Reader, Obj) || !Obj.IsValid())
		{
			return false;
		}
		TSharedRef<TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>>::Create(&Out);
		FJsonSerializer::Serialize(Obj.ToSharedRef(), Writer);
		return true;
	}

	static bool CopyDirectoryRecursive(const FString& SourceDir, const FString& TargetDir, int32& OutCopiedFiles, FString& OutError)
	{
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		if (!PlatformFile.DirectoryExists(*SourceDir))
		{
			OutError = FString::Printf(TEXT("源目录不存在: %s"), *SourceDir);
			return false;
		}
		if (!PlatformFile.CreateDirectoryTree(*TargetDir))
		{
			OutError = FString::Printf(TEXT("无法创建目标目录: %s"), *TargetDir);
			return false;
		}

		TArray<FString> Files;
		IFileManager::Get().FindFiles(Files, *(FPaths::Combine(SourceDir, TEXT("*"))), true, false);
		for (const FString& FileName : Files)
		{
			const FString SourceFile = FPaths::Combine(SourceDir, FileName);
			const FString TargetFile = FPaths::Combine(TargetDir, FileName);
			if (!PlatformFile.CopyFile(*TargetFile, *SourceFile))
			{
				OutError = FString::Printf(TEXT("复制失败: %s -> %s"), *SourceFile, *TargetFile);
				return false;
			}
			++OutCopiedFiles;
		}

		TArray<FString> SubDirs;
		IFileManager::Get().FindFiles(SubDirs, *(FPaths::Combine(SourceDir, TEXT("*"))), false, true);
		for (const FString& SubDirName : SubDirs)
		{
			const FString SourceSubDir = FPaths::Combine(SourceDir, SubDirName);
			const FString TargetSubDir = FPaths::Combine(TargetDir, SubDirName);
			if (!CopyDirectoryRecursive(SourceSubDir, TargetSubDir, OutCopiedFiles, OutError))
			{
				return false;
			}
		}

		return true;
	}

	static bool RepairRegistryFromGenerated(const FString& OutputDir, TFunction<void(const FString&)> AppendLog)
	{
		if (OutputDir.IsEmpty() || !IFileManager::Get().DirectoryExists(*OutputDir))
		{
			AppendLog(TEXT("修复 Registry 失败：Generated 目录不存在"));
			return false;
		}

		TArray<FString> InterfaceFolders;
		IFileManager::Get().FindFiles(InterfaceFolders, *(FPaths::Combine(OutputDir, TEXT("*"))), false, true);

		struct FRegistryRow
		{
			FString Folder;
			FString Method;
			FString URL;
			FString RootType;
		};
		TArray<FRegistryRow> Rows;

		for (const FString& FolderName : InterfaceFolders)
		{
			const FString FolderPath = FPaths::Combine(OutputDir, FolderName);
			TArray<FString> HeaderFiles;
			IFileManager::Get().FindFilesRecursive(HeaderFiles, *FolderPath, TEXT("*.h"), true, false, false);
			if (HeaderFiles.Num() == 0)
			{
				continue;
			}

			FString Method = TEXT("GET");
			int32 UnderScorePos = INDEX_NONE;
			if (FolderName.FindChar(TEXT('_'), UnderScorePos) && UnderScorePos > 0)
			{
				Method = FolderName.Left(UnderScorePos).ToUpper();
			}

			FString FoundUrl;
			FString FoundRootType;
			for (const FString& HeaderPath : HeaderFiles)
			{
				const FString FileName = FPaths::GetCleanFilename(HeaderPath);
				if (FileName.StartsWith(TEXT("JSON2StructGeneratedBPApi_")))
				{
					continue;
				}

				FString Content;
				if (!FFileHelper::LoadFileToString(Content, *HeaderPath))
				{
					continue;
				}

				const FString UrlTag = TEXT("// JSON2Struct URL:");
				int32 UrlTagPos = Content.Find(UrlTag, ESearchCase::CaseSensitive);
				if (UrlTagPos != INDEX_NONE)
				{
					int32 UrlLineEnd = Content.Find(TEXT("\n"), ESearchCase::CaseSensitive, ESearchDir::FromStart, UrlTagPos);
					const FString UrlLine = (UrlLineEnd == INDEX_NONE) ? Content.Mid(UrlTagPos) : Content.Mid(UrlTagPos, UrlLineEnd - UrlTagPos);
					FoundUrl = UrlLine.Mid(UrlTag.Len()).TrimStartAndEnd();
				}

				int32 StructPos = Content.Find(TEXT("struct "), ESearchCase::CaseSensitive);
				if (StructPos != INDEX_NONE)
				{
					int32 TypeStart = StructPos + FCString::Strlen(TEXT("struct "));
					while (TypeStart < Content.Len() && FChar::IsWhitespace(Content[TypeStart])) { ++TypeStart; }
					const FString ApiToken = TEXT("JSON2STRUCTRUNTIME_API");
					if (Content.Mid(TypeStart, ApiToken.Len()) == ApiToken)
					{
						TypeStart += ApiToken.Len();
						while (TypeStart < Content.Len() && FChar::IsWhitespace(Content[TypeStart])) { ++TypeStart; }
					}
					int32 TypeEnd = TypeStart;
					while (TypeEnd < Content.Len() && (FChar::IsAlnum(Content[TypeEnd]) || Content[TypeEnd] == TEXT('_'))) { ++TypeEnd; }
					FoundRootType = Content.Mid(TypeStart, TypeEnd - TypeStart);
				}

				if (!FoundUrl.IsEmpty() && !FoundRootType.IsEmpty())
				{
					break;
				}
			}

			if (!FoundUrl.IsEmpty() && !FoundRootType.IsEmpty())
			{
				Rows.Add({ FolderName, Method, FoundUrl, FoundRootType });
			}
		}

		Rows.Sort([](const FRegistryRow& A, const FRegistryRow& B) { return A.Folder < B.Folder; });
		FString RegistryContent = TEXT("# Folder\tMethod\tURL\tRootType\n");
		for (const FRegistryRow& Row : Rows)
		{
			RegistryContent += Row.Folder + TEXT("\t") + Row.Method + TEXT("\t") + Row.URL + TEXT("\t") + Row.RootType + TEXT("\n");
		}

		const FString RegistryPath = FPaths::Combine(OutputDir, TEXT("JSON2StructRegistry.txt"));
		if (!FFileHelper::SaveStringToFile(RegistryContent, *RegistryPath))
		{
			AppendLog(FString::Printf(TEXT("修复 Registry 失败，写入失败：%s"), *RegistryPath));
			return false;
		}

		AppendLog(FString::Printf(TEXT("Registry 维护完成：%d 个接口文件夹映射"), Rows.Num()));
		return true;
	}

	FReply HandleFixRegistryClicked(
		TFunction<void(const FString&)> AppendLog,
		TSharedPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>> TreeView,
		TFunction<void(TSharedPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>>, const FString&, const TSharedRef<FString>&)> RefreshTreeViewSafely,
		FString FixedTreeRootDir,
		TSharedRef<FString> LastTreeSnapshot)
	{
		RepairRegistryFromGenerated(FixedTreeRootDir, AppendLog);
		RefreshTreeViewSafely(TreeView, FixedTreeRootDir, LastTreeSnapshot);
		return FReply::Handled();
	}

	FReply HandleRequestClicked(
		TSharedPtr<SEditableTextBox> UrlEditable,
		TWeakPtr<SMultiLineEditableTextBox> JsonEditableWeak,
		TFunction<void(const FString&)> AppendLog,
		TSharedRef<FString> SelectedHttpMethodValue)
	{
		AppendLog(TEXT("请求按钮点击"));
		AppendLog(TEXT("开始请求..."));
		const FString Url = UrlEditable.IsValid() ? UrlEditable->GetText().ToString() : FString();
		if (Url.IsEmpty())
		{
			AppendLog(TEXT("URL 为空"));
			return FReply::Handled();
		}

		FHTTPRequestParams RequestSpec;
		RequestSpec.URL = Url;
		const FString Method = SelectedHttpMethodValue->ToUpper();
		if (Method == TEXT("POST")) RequestSpec.Method = EJSON2StructHttpMethod::POST;
		else if (Method == TEXT("PUT")) RequestSpec.Method = EJSON2StructHttpMethod::PUT;
		else if (Method == TEXT("DELETE")) RequestSpec.Method = EJSON2StructHttpMethod::DELETE;
		else if (Method == TEXT("PATCH")) RequestSpec.Method = EJSON2StructHttpMethod::PATCH;
		else RequestSpec.Method = EJSON2StructHttpMethod::GET;
		AppendLog(FString::Printf(TEXT("请求方法: %s"), *Method));

		FJSON2StructRest::RequestBySpec(RequestSpec, FOnJSON2StructRequestComplete::CreateLambda(
			[JsonEditableWeak, AppendLog](const FString& Content, bool bSuccess)
			{
				if (TSharedPtr<SMultiLineEditableTextBox> JsonP = JsonEditableWeak.Pin())
				{
					FString Display = Content;
					if (bSuccess && FormatJsonString(Content, Display))
					{
						JsonP->SetText(FText::FromString(Display));
					}
					else
					{
						JsonP->SetText(FText::FromString(Content));
					}
				}
				AppendLog(bSuccess ? TEXT("请求成功") : FString::Printf(TEXT("请求失败: %s"), *Content));
			}));
		return FReply::Handled();
	}

	FReply HandleGenerateStructureClicked(
		TSharedPtr<SEditableTextBox> UrlEditable,
		TSharedPtr<SMultiLineEditableTextBox> JsonEditable,
		TSharedPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>> TreeView,
		TFunction<void(const FString&)> AppendLog,
		TFunction<void(TSharedPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>>, const FString&, const TSharedRef<FString>&)> RefreshTreeViewSafely,
		TSharedRef<FString> LastTreeSnapshot,
		FString FixedTreeRootDir,
		TSharedRef<FString> SelectedHttpMethodValue)
	{
		AppendLog(TEXT("生成 Structure 点击"));
		AppendLog(TEXT("生成开始..."));
		const FString JsonText = JsonEditable.IsValid() ? JsonEditable->GetText().ToString() : FString();
		if (JsonText.TrimStartAndEnd().IsEmpty())
		{
			AppendLog(TEXT("JSON 为空，请先请求或粘贴"));
			return FReply::Handled();
		}

		const FString RequestUrl = UrlEditable.IsValid() ? UrlEditable->GetText().ToString() : FString();
		const FString RequestMethod = SelectedHttpMethodValue->ToUpper();
		const FString OutDir = FixedTreeRootDir;
		const bool bMergeWithExisting = true;
		AppendLog(FString::Printf(TEXT("生成参数 Method=%s OutDir=%s bMerge=%d"), *RequestMethod, *OutDir, bMergeWithExisting ? 1 : 0));
		TArray<FJSON2StructGeneratedItem> NewItems;
		if (GenerateStructureToFolder(JsonText, OutDir, bMergeWithExisting, RequestUrl, RequestMethod, NewItems, AppendLog))
		{
			RefreshTreeViewSafely(TreeView, FixedTreeRootDir, LastTreeSnapshot);
			AppendLog(FString::Printf(TEXT("生成成功，新增 %d 项"), NewItems.Num()));
		}
		else
		{
			AppendLog(TEXT("生成失败，请查看面板日志或上方 Output Log"));
		}
		return FReply::Handled();
	}

	FReply HandleHotCompileClicked(TFunction<void(const FString&)> AppendLog)
	{
		if (!GEngine)
		{
			AppendLog(TEXT("热编译失败：GEngine 不可用"));
			return FReply::Handled();
		}
		AppendLog(TEXT("开始热编译（Live Coding）..."));
		const bool bExecOk = GEngine->Exec(nullptr, TEXT("LiveCoding.Compile"));
		AppendLog(bExecOk ? TEXT("已发送热编译命令") : TEXT("热编译命令未执行，请确认 Live Coding 已启用"));
		return FReply::Handled();
	}

	FReply HandleRefreshAndHotCompileClicked(TFunction<void(const FString&)> AppendLog)
	{
		if (!GEngine)
		{
			AppendLog(TEXT("热编译失败：GEngine 不可用"));
			return FReply::Handled();
		}

		const FString BuildBatPath = FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::EngineDir(), TEXT("Build/BatchFiles/Build.bat")));
		const FString UProjectPath = FPaths::ConvertRelativePathToFull(FPaths::GetProjectFilePath());
		if (!FPaths::FileExists(BuildBatPath))
		{
			AppendLog(FString::Printf(TEXT("刷新工程失败：未找到 Build.bat (%s)"), *BuildBatPath));
			return FReply::Handled();
		}
		if (UProjectPath.IsEmpty() || !FPaths::FileExists(UProjectPath))
		{
			AppendLog(FString::Printf(TEXT("刷新工程失败：未找到 .uproject (%s)"), *UProjectPath));
			return FReply::Handled();
		}

		const FString BuildArgs = FString::Printf(TEXT("-projectfiles \"%s\" -game -engine -WaitMutex"), *UProjectPath);
		AppendLog(TEXT("开始刷新工程文件（projectfiles）..."));
		Async(EAsyncExecution::ThreadPool, [AppendLog, BuildBatPath, BuildArgs]()
		{
			int32 ReturnCode = -1;
			FString StdOut;
			FString StdErr;
			FPlatformProcess::ExecProcess(*BuildBatPath, *BuildArgs, &ReturnCode, &StdOut, &StdErr);

			AsyncTask(ENamedThreads::GameThread, [AppendLog, ReturnCode, StdOut, StdErr]()
			{
				if (ReturnCode != 0)
				{
					AppendLog(FString::Printf(TEXT("刷新工程文件失败，返回码=%d"), ReturnCode));
					if (!StdErr.IsEmpty())
					{
						AppendLog(FString::Printf(TEXT("projectfiles 错误：%s"), *StdErr.Left(300)));
					}
					return;
				}

				AppendLog(TEXT("工程文件刷新完成，开始触发热编译（Live Coding）..."));
				const bool bExecOk = GEngine && GEngine->Exec(nullptr, TEXT("LiveCoding.Compile"));
				AppendLog(bExecOk ? TEXT("已发送热编译命令") : TEXT("热编译命令未执行，请确认 Live Coding 已启用"));
			});
		});
		return FReply::Handled();
	}

	FReply HandleBrowseDirectoryClicked(
		TSharedPtr<SEditableTextBox> DirEditable,
		TSharedPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>> TreeView,
		TFunction<void(const FString&)> AppendLog,
		TFunction<void(TSharedPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>>, const FString&, const TSharedRef<FString>&)> RefreshTreeViewSafely,
		TSharedRef<FString> LastTreeSnapshot,
		FString FixedTreeRootDir)
	{
		AppendLog(TEXT("浏览目录"));
		IDesktopPlatform* Desktop = FDesktopPlatformModule::Get();
		if (!Desktop)
		{
			return FReply::Handled();
		}

		FString Current = DirEditable.IsValid() ? DirEditable->GetText().ToString() : FString();
		if (Current.IsEmpty())
		{
			Current = FJSON2StructSettings::GetDefaultGeneratedCodeDir();
		}

		FString Out;
		if (Desktop->OpenDirectoryDialog(FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr), TEXT("选择生成目录"), Current, Out))
		{
			if (DirEditable.IsValid()) DirEditable->SetText(FText::FromString(Out));
			FJSON2StructSettings::SaveLastStructureDirectory(Out);
			RefreshTreeViewSafely(TreeView, FixedTreeRootDir, LastTreeSnapshot);
			AppendLog(FString::Printf(TEXT("已选择输出目录: %s（左侧树固定显示 Generated）"), *Out));
		}
		return FReply::Handled();
	}

	FReply HandleCopyGeneratedClicked(
		TSharedPtr<SEditableTextBox> DirEditable,
		TFunction<void(const FString&)> AppendLog,
		FString FixedTreeRootDir,
		TFunction<void(TSharedPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>>, const FString&, const TSharedRef<FString>&)> RefreshTreeViewSafely,
		TSharedPtr<STreeView<TSharedPtr<FGeneratedFileTreeItem>>> TreeView,
		TSharedRef<FString> LastTreeSnapshot)
	{
		const FString TargetDir = DirEditable.IsValid() ? FPaths::ConvertRelativePathToFull(DirEditable->GetText().ToString()) : FString();
		if (TargetDir.IsEmpty())
		{
			AppendLog(TEXT("目标目录为空，请先选择目录"));
			return FReply::Handled();
		}

		const FString SourceDir = FPaths::ConvertRelativePathToFull(FixedTreeRootDir);
		if (FPaths::IsSamePath(SourceDir, TargetDir))
		{
			AppendLog(TEXT("目标目录与 Generated 相同，无需复制"));
			return FReply::Handled();
		}

		int32 CopiedFiles = 0;
		FString CopyError;
		if (CopyDirectoryRecursive(SourceDir, TargetDir, CopiedFiles, CopyError))
		{
			AppendLog(FString::Printf(TEXT("复制完成，共复制 %d 个文件到: %s"), CopiedFiles, *TargetDir));
			RefreshTreeViewSafely(TreeView, FixedTreeRootDir, LastTreeSnapshot);
		}
		else
		{
			AppendLog(FString::Printf(TEXT("复制失败: %s"), *CopyError));
		}
		return FReply::Handled();
	}

	FReply HandleToggleTreeFolderClicked(
		TSharedRef<STreeView<TSharedPtr<FGeneratedFileTreeItem>>> TreeOwner,
		TSharedPtr<FGeneratedFileTreeItem> Item)
	{
		const bool bExpanded = TreeOwner->IsItemExpanded(Item);
		TreeOwner->SetItemExpansion(Item, !bExpanded);
		TreeOwner->SetSelection(Item, ESelectInfo::OnMouseClick);
		return FReply::Handled();
	}
}

