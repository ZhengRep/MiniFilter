#include "ExcludeList.h"
#include "CommonSetting.h"

extern PEXCLUDE_CONTEXT __ExcludeFileContext;        //нд╪Ч
extern PEXCLUDE_CONTEXT __ExcludeDirectoryContext;   //д©б╪

NTSTATUS InitializeExcludeListContext(PPEXCLUDE_CONTEXT ExcludeContext)
{
	NTSTATUS status = STATUS_SUCCESS;
	PEXCLUDE_FILE_CONTEXT TempExcludeFileContext = ExAllocatePool(NonPagedPool, sizeof(EXCLUDE_FILE_CONTEXT));
	if (!TempExcludeFileContext)
	{
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	InitializeListHead(&TempExcludeFileContext->ListEntry);
	ExInitializeFastMutex(&TempExcludeFileContext->FastMutex);
	*ExcludeContext = TempExcludeFileContext;
	return status;
}

NTSTATUS AddExcludeFileList(PEXCLUDE_CONTEXT ExcludeContext, PUNICODE_STRING FilePath)
{
	NTSTATUS status = STATUS_SUCCESS;
	status = AddExcludeListEntry(ExcludeContext, FilePath);
	return status;
}

NTSTATUS AddExcludeDirectoryList(PEXCLUDE_CONTEXT ExcludeContext, PUNICODE_STRING DirectoryPath)
{
	NTSTATUS status = STATUS_SUCCESS;
	status = AddExcludeListEntry(ExcludeContext, DirectoryPath);
	return status;
}

NTSTATUS AddExcludeListEntry(PEXCLUDE_CONTEXT ExcludeContext, PUNICODE_STRING FilePath)
{
	NTSTATUS status = STATUS_SUCCESS;
	if (FilePath->Length == 0 || FilePath->Length > 1024)
	{
		return STATUS_INVALID_PARAMETER_2;
	}

	SIZE_T BufferLength = sizeof(EXCLUDE_FILE_LIST_ENTRY) + FilePath->Length;
	PEXCLUDE_FILE_LIST_ENTRY ExcludeFileListEntry = ExAllocatePool(NonPagedPool, BufferLength);
	if(!ExcludeFileListEntry)
	{ 
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	RtlZeroMemory(ExcludeFileListEntry, BufferLength);

	if (!FillDirectoryFromPath(&(ExcludeFileListEntry->ExculdeFilePath), FilePath))
	{
		ExFreePool(ExcludeFileListEntry);
		return STATUS_ACCESS_DENIED;
	}

	//Add ListEntry to the pointer of ExcludeFileContext
	ExAcquireFastMutex(&((PEXCLUDE_FILE_CONTEXT)ExcludeContext)->FastMutex);
	InsertTailList(&((PEXCLUDE_FILE_CONTEXT)ExcludeContext)->ListEntry, &ExcludeFileListEntry->ListEntry);
	ExReleaseFastMutex(&((PEXCLUDE_FILE_CONTEXT)ExcludeContext)->FastMutex);

	return status;
}

BOOLEAN FillDirectoryFromPath(PEXCULDE_FILE_PATH ExculdeFilePath, PUNICODE_STRING FilePath)
{
	BOOLEAN IsOk = FALSE;
	if (FilePath->Length == 0 || FilePath->Length > 1024)
	{
		return FALSE;
	}
	SIZE_T IndexOfWChar = FilePath->Length / sizeof(WCHAR);
	SIZE_T OriginLength = IndexOfWChar;

	for (; IndexOfWChar > 0; IndexOfWChar--)
	{
		if (FilePath->Buffer[IndexOfWChar] == L'\\')
		{
			if (IndexOfWChar + 1 >= OriginLength)
			{
				return FALSE;
			}
			ExculdeFilePath->FileName.Buffer = FilePath->Buffer + IndexOfWChar + 1;
			ExculdeFilePath->FileName.MaximumLength = ExculdeFilePath->FileName.Length = (OriginLength - IndexOfWChar - 1) * sizeof(WCHAR);

			ExculdeFilePath->FullPath = *FilePath;

			ExculdeFilePath->DirectoryName.Buffer = FilePath->Buffer;
			ExculdeFilePath->DirectoryName.MaximumLength = ExculdeFilePath->DirectoryName.Length = IndexOfWChar * sizeof(WCHAR);
			return TRUE;
		}
	}
	return IsOk;
}

VOID DestroyExcludeListContext(PEXCLUDE_CONTEXT ExcludeContext)
{
	PEXCLUDE_FILE_CONTEXT Temp = (PEXCLUDE_FILE_CONTEXT)ExcludeContext;
	RemoveAllExcludeListEntry(Temp);
	ExFreePool(Temp);
}

NTSTATUS RemoveAllExcludeListEntry(PEXCLUDE_CONTEXT ExcludeContext)
{
	PEXCLUDE_FILE_CONTEXT ExcludeFileContext = (PEXCLUDE_FILE_CONTEXT)ExcludeFileContext;
	PEXCLUDE_FILE_LIST_ENTRY Head = (PEXCLUDE_FILE_LIST_ENTRY)ExcludeFileContext->ListEntry.Flink;

	ExAcquireFastMutex(&ExcludeFileContext->FastMutex);

	while (Head != (PEXCLUDE_FILE_LIST_ENTRY)&ExcludeFileContext->ListEntry)
	{
		PEXCLUDE_FILE_LIST_ENTRY Temp = Head;
		Head = (PEXCLUDE_FILE_LIST_ENTRY)Head->ListEntry.Flink;
		RemoveEntryList(&(Temp->ListEntry));
		ExFreePool(Temp);
	}
	ExReleaseFastMutex(&ExcludeFileContext->FastMutex);
	return STATUS_SUCCESS;

}

BOOLEAN CheckExcludeFileList(PEXCLUDE_CONTEXT ExcludeContext, PCUNICODE_STRING FilePath)
{
	BOOLEAN IsOk = TRUE;
	PEXCLUDE_FILE_CONTEXT ExcludeFileContext = (PEXCLUDE_FILE_CONTEXT)ExcludeFileContext;
	PEXCLUDE_FILE_LIST_ENTRY Head = (PEXCLUDE_FILE_LIST_ENTRY)ExcludeFileContext->ListEntry.Flink;
	
	UNICODE_STRING TempStr = *FilePath;
	if (FilePath->Length > 0 && FilePath->Buffer[FilePath->Length / sizeof(WCHAR) - 1] == L'\\')
	{
		TempStr.Length -= sizeof(WCHAR);
	}

	ExAcquireFastMutex(&ExcludeFileContext->FastMutex);

	while (Head != (PEXCLUDE_FILE_LIST_ENTRY)&ExcludeFileContext->ListEntry)
	{
		if (TempStr.Length >= Head->ExculdeFilePath.FullPath.Length)
		{
			BOOLEAN IsCompare = TRUE;

			if (TempStr.Length > Head->ExculdeFilePath.FullPath.Length)
			{
				if (TempStr.Buffer[Head->ExculdeFilePath.FullPath.Length / sizeof(WCHAR)] != L'\\')
				{
					IsCompare = FALSE;
				}
				else
				{
					TempStr.Length = Head->ExculdeFilePath.FullPath.Length;
				}
			}
			if (IsCompare && RtlCompareUnicodeString(&Head->ExculdeFilePath.FullPath, &TempStr, TRUE) == 0)
			{
				IsOk = TRUE;
				break;
			}
		}
		Head = (PEXCLUDE_FILE_LIST_ENTRY)Head->ListEntry.Flink;
	}

	ExReleaseFastMutex(&ExcludeFileContext->FastMutex);
	return IsOk;
}

BOOLEAN CheckExcludeDirecoryFileList(PEXCLUDE_CONTEXT ExcludeContext, PCUNICODE_STRING DirectoryName, PCUNICODE_STRING FileName)
{
	BOOLEAN IsOk = TRUE;
	PEXCLUDE_FILE_CONTEXT ExcludeFileContext = (PEXCLUDE_FILE_CONTEXT)ExcludeFileContext;
	PEXCLUDE_FILE_LIST_ENTRY Head = (PEXCLUDE_FILE_LIST_ENTRY)ExcludeFileContext->ListEntry.Flink;

	UNICODE_STRING TempStr = *DirectoryName;
	if (TempStr.Length > 0 && TempStr.Buffer[TempStr.Length / sizeof(WCHAR) - 1] == L'\\')
	{
		TempStr.Length -= sizeof(WCHAR);
	}

	ExAcquireFastMutex(&ExcludeFileContext->FastMutex);

	while (Head != (PEXCLUDE_FILE_LIST_ENTRY)&ExcludeFileContext->ListEntry)
	{
		if (RtlCompareUnicodeString(&Head->ExculdeFilePath.DirectoryName, &TempStr, TRUE) == 0 && RtlCompareUnicodeString(&Head->ExculdeFilePath.FileName, FileName, TRUE) == 0)
		{
			IsOk = TRUE;
			break;
		}
		Head = (PEXCLUDE_FILE_LIST_ENTRY)Head->ListEntry.Flink;
	}

	ExReleaseFastMutex(&ExcludeFileContext->FastMutex);
	return IsOk;
}
