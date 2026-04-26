// Copyright Epic Games, Inc. All Rights Reserved.
//Author WeChat: wujifeng_mr

#include "JSON2StructEditorTree.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "ISourceCodeAccessModule.h"
#include "Misc/Paths.h"
#include "SourceCodeNavigation.h"

namespace JSON2StructPanel
{
	static bool ShouldShowGeneratedFile(const FString& FileName)
	{
		return FileName.EndsWith(TEXT(".h"), ESearchCase::IgnoreCase) ||
			FileName.EndsWith(TEXT(".cpp"), ESearchCase::IgnoreCase) ||
			FileName.EndsWith(TEXT(".inl"), ESearchCase::IgnoreCase) ||
			FileName.EndsWith(TEXT(".txt"), ESearchCase::IgnoreCase);
	}

	void OpenFolderAndSelectFile(const FString& FilePath)
	{
		if (!FPaths::FileExists(FilePath))
		{
			if (FPaths::DirectoryExists(FilePath))
			{
				FPlatformProcess::ExploreFolder(*FilePath);
			}
			return;
		}

		const FString Dir = FPaths::GetPath(FilePath);
#if PLATFORM_WINDOWS
		const FString Param = FString::Printf(TEXT("/select,\"%s\""), *FilePath);
		FPlatformProcess::CreateProc(TEXT("explorer.exe"), *Param, true, false, false, nullptr, 0, nullptr, nullptr, nullptr, nullptr);
#else
		FPlatformProcess::ExploreFolder(*Dir);
#endif
	}

	static void SortTreeNodeRecursive(const TSharedPtr<FGeneratedFileTreeItem>& Node)
	{
		if (!Node.IsValid())
		{
			return;
		}

		Node->Children.Sort([](const TSharedPtr<FGeneratedFileTreeItem>& A, const TSharedPtr<FGeneratedFileTreeItem>& B)
		{
			if (A->bIsFolder != B->bIsFolder)
			{
				return A->bIsFolder && !B->bIsFolder;
			}
			return A->DisplayName < B->DisplayName;
		});

		for (const TSharedPtr<FGeneratedFileTreeItem>& Child : Node->Children)
		{
			SortTreeNodeRecursive(Child);
		}
	}

	static void BuildTreeChildrenRecursive(const FString& Directory, const TSharedPtr<FGeneratedFileTreeItem>& Parent)
	{
		if (!Parent.IsValid() || !IFileManager::Get().DirectoryExists(*Directory))
		{
			return;
		}

		TArray<FString> SubDirs;
		IFileManager::Get().FindFiles(SubDirs, *(FPaths::Combine(Directory, TEXT("*"))), false, true);
		for (const FString& SubDirName : SubDirs)
		{
			const FString SubDirFullPath = FPaths::Combine(Directory, SubDirName);
			TSharedPtr<FGeneratedFileTreeItem> DirNode = MakeShared<FGeneratedFileTreeItem>();
			DirNode->DisplayName = SubDirName;
			DirNode->FullPath = SubDirFullPath;
			DirNode->bIsFolder = true;
			Parent->Children.Add(DirNode);
			BuildTreeChildrenRecursive(SubDirFullPath, DirNode);
		}

		TArray<FString> Files;
		IFileManager::Get().FindFiles(Files, *(FPaths::Combine(Directory, TEXT("*"))), true, false);
		for (const FString& FileName : Files)
		{
			if (!ShouldShowGeneratedFile(FileName))
			{
				continue;
			}

			TSharedPtr<FGeneratedFileTreeItem> FileNode = MakeShared<FGeneratedFileTreeItem>();
			FileNode->DisplayName = FileName;
			FileNode->FullPath = FPaths::Combine(Directory, FileName);
			FileNode->bIsFolder = false;
			Parent->Children.Add(FileNode);
		}
	}

	static void CollectTreeEntriesRecursive(const FString& Directory, const FString& RelativeDir, TArray<FString>& OutEntries)
	{
		if (!IFileManager::Get().DirectoryExists(*Directory))
		{
			return;
		}

		TArray<FString> SubDirs;
		IFileManager::Get().FindFiles(SubDirs, *(FPaths::Combine(Directory, TEXT("*"))), false, true);
		SubDirs.Sort();
		for (const FString& SubDirName : SubDirs)
		{
			const FString ChildRelative = RelativeDir.IsEmpty() ? SubDirName : (RelativeDir / SubDirName);
			const FString ChildFullPath = FPaths::Combine(Directory, SubDirName);
			OutEntries.Add(FString::Printf(TEXT("D|%s"), *ChildRelative));
			CollectTreeEntriesRecursive(ChildFullPath, ChildRelative, OutEntries);
		}

		TArray<FString> Files;
		IFileManager::Get().FindFiles(Files, *(FPaths::Combine(Directory, TEXT("*"))), true, false);
		Files.Sort();
		for (const FString& FileName : Files)
		{
			if (!ShouldShowGeneratedFile(FileName))
			{
				continue;
			}

			const FString FileRelative = RelativeDir.IsEmpty() ? FileName : (RelativeDir / FileName);
			OutEntries.Add(FString::Printf(TEXT("F|%s"), *FileRelative));
		}
	}

	TSharedPtr<FGeneratedFileTreeItem> BuildFileTreeFromDirectory(const FString& OutputDir)
	{
		const TSharedPtr<FGeneratedFileTreeItem> Root = MakeShared<FGeneratedFileTreeItem>();
		Root->DisplayName = FPaths::GetCleanFilename(OutputDir).IsEmpty() ? FString(TEXT("Generated")) : FPaths::GetCleanFilename(OutputDir);
		Root->FullPath = OutputDir;
		Root->bIsFolder = true;
		BuildTreeChildrenRecursive(OutputDir, Root);
		SortTreeNodeRecursive(Root);
		return Root;
	}

	FString BuildDirectoryTreeSnapshot(const FString& OutputDir)
	{
		const FString FullOutDir = FPaths::ConvertRelativePathToFull(OutputDir);
		if (!IFileManager::Get().DirectoryExists(*FullOutDir))
		{
			return TEXT("__MISSING__");
		}

		TArray<FString> Entries;
		CollectTreeEntriesRecursive(FullOutDir, FString(), Entries);
		Entries.Sort();
		return FString::Join(Entries, TEXT("\n"));
	}

	bool OpenFileInIDE(const FString& FilePath)
	{
		if (!FilePath.IsEmpty() && FPaths::FileExists(FilePath))
		{
			return FSourceCodeNavigation::OpenSourceFile(FilePath);
		}
		return false;
	}
}

