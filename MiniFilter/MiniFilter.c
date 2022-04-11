#include "MiniFilter.h"
#include "CommonSetting.h"
#include "ExcludeList.h"

//Global Variables
BOOLEAN					__FsMonitorActive = FALSE;        //Ring3
BOOLEAN					__FsMonitorInitialized = FALSE;   //开启或关闭文件系统监控
BOOLEAN					__PsMonitorInitialized = FALSE;   //开启或关闭进线程监控
PEXCLUDE_CONTEXT		__ExcludeFileContext;        //文件
PEXCLUDE_CONTEXT		__ExcludeDirectoryContext;   //目录

//
//  operation registration 
//

CONST FLT_OPERATION_REGISTRATION __Callbacks[] = {

	{
		IRP_MJ_READ,
		0,
		CreatePreviousOperation,
		NULL
},
	{
		IRP_MJ_DIRECTORY_CONTROL,
		0,
		DirectoryCtrlPreviousOperation,
		DirectoryCtrlPostOperation
},
	{ IRP_MJ_OPERATION_END }
};

//
//  This defines what we want to filter with FltMgr
//
const FLT_CONTEXT_REGISTRATION __Context[] = {
	{ FLT_CONTEXT_END }
};

CONST FLT_REGISTRATION __FilterRegistration = {

	sizeof(FLT_REGISTRATION),         //  Size
	FLT_REGISTRATION_VERSION,           //  Version
	0,                                  //  Flags

	__Context,                               //  Context
	__Callbacks,                          //  Operation callbacks

	MiniFilterUnload,                           //  MiniFilterUnload

	MiniFilterInstanceSetup,                    //  InstanceSetup
	MiniFilterInstanceQueryTeardown,            //  InstanceQueryTeardown
	MiniFilterInstanceTeardownStart,            //  InstanceTeardownStart
	MiniFilterInstanceTeardownComplete,         //  InstanceTeardownComplete

	NULL,                               //  GenerateFileName
	NULL,                               //  GenerateDestinationFileName
	NULL                                //  NormalizeNameComponent
};

NTSTATUS
MiniFilterInstanceSetup (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
    )
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );
    UNREFERENCED_PARAMETER( VolumeDeviceType );
    UNREFERENCED_PARAMETER( VolumeFilesystemType );

    PAGED_CODE();


    return STATUS_SUCCESS;
}


NTSTATUS
MiniFilterInstanceQueryTeardown (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
    )
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();


    return STATUS_SUCCESS;
}


VOID
MiniFilterInstanceTeardownStart(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
)
{
    UNREFERENCED_PARAMETER(FltObjects);
    UNREFERENCED_PARAMETER(Flags);

    PAGED_CODE();

}


VOID
MiniFilterInstanceTeardownComplete (
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
    )
{
    UNREFERENCED_PARAMETER( FltObjects );
    UNREFERENCED_PARAMETER( Flags );

    PAGED_CODE();

}


/*************************************************************************
    MiniFilter initialization and unload routines.
*************************************************************************/

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS status;

    UNREFERENCED_PARAMETER( RegistryPath );

	DbgPrint("Driver Entry");

    status = InitializeMiniFilter(DriverObject);
    if (!NT_SUCCESS(status))
    {
        DbgPrint("InitailizeMiniFilter Error.\n");
    }

    DriverObject->DriverUnload = DriverUnload;

    return status;
}

VOID DriverUnload(_In_ PDRIVER_OBJECT DriverObject)
{
    UNREFERENCED_PARAMETER(DriverObject);
    DestroyMiniFilter();
}

NTSTATUS InitializeMiniFilter(PDRIVER_OBJECT DriverObject)
{
	PAGED_CODE();
	NTSTATUS Status = STATUS_SUCCESS;
	//构建文件隐藏的黑白名单
	Status = InitializeExcludeListContext(&__ExcludeFileContext);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint("InitializeExcludeListContext() Error\r\n");
		return Status;
	}
	//加入测试数据
	int i;
	UNICODE_STRING TempStr;
	for (i = 0; __ExcludeFiles[i]; i++)
	{
		RtlInitUnicodeString(&TempStr, __ExcludeFiles[i]);
		AddExcludeFileList(__ExcludeFileContext, &TempStr);
	}
	//构建目录隐藏的黑白名单
	Status = InitializeExcludeListContext(&__ExcludeDirectoryContext);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint("SeInitializeExcludeListContext() Error\r\n");
		DestroyExcludeListContext(__ExcludeFileContext);
		return Status;
	}
	//加入测试数据
	for (i = 0; __ExcludeDirectorys[i]; i++)
	{
		RtlInitUnicodeString(&TempStr, __ExcludeDirectorys[i]);
		AddExcludeDirectoryList(__ExcludeDirectoryContext, &TempStr);
	}
	//注册过滤函数
	Status = FltRegisterFilter(DriverObject, &__FilterRegistration, &__FilterHandle);
	if (NT_SUCCESS(Status))
	{
		Status = FltStartFiltering(__FilterHandle);
		if (!NT_SUCCESS(Status))
		{
			DbgPrint("FltStartFiltering() Error\r\n");
			FltUnregisterFilter(__FilterHandle);
		}
	}
	else
	{
		DbgPrint("FltRegisterFilter() Error\r\n");
	}

	if (!NT_SUCCESS(Status))
	{
		DestroyExcludeListContext(__ExcludeFileContext);
		DestroyExcludeListContext(__ExcludeDirectoryContext);
		return Status;
	}

	__FsMonitorInitialized = TRUE;   //当前驱动具有文件过滤功能
	__FsMonitorActive = TRUE;        //文件过滤功能是否开启      在IRP_MJ_DEVICE_CONTROL派遣函数中设置
	return Status;
}

NTSTATUS DestroyMiniFilter()
{
	PAGED_CODE();
	NTSTATUS status = STATUS_SUCCESS;
	if (!__FsMonitorInitialized)
	{
		return STATUS_NOT_FOUND;
	}

	FltUnregisterFilter(__FilterHandle);
	__FilterHandle = NULL;

	DestroyExcludeListContext(__ExcludeFileContext);
	DestroyExcludeListContext(__ExcludeDirectoryContext);
	__FsMonitorInitialized = FALSE;
	__FsMonitorActive = FALSE;
	return status;
}

BOOLEAN IsMiniFilterActive()
{
	PAGED_CODE();
	return (__FsMonitorActive ? TRUE : FALSE);
}

BOOLEAN IsProcessExcluded(HANDLE ProcessIdentify)
{
	PAGED_CODE();
	BOOLEAN IsOk = TRUE;
	return IsOk;
}

NTSTATUS
MiniFilterUnload(
	_In_ FLT_FILTER_UNLOAD_FLAGS Flags
)
{
	UNREFERENCED_PARAMETER(Flags);

	PAGED_CODE();

	return STATUS_SUCCESS;
}


FLT_PREOP_CALLBACK_STATUS CreatePreviousOperation(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
{
	PAGED_CODE();

	NTSTATUS Status;
	//驱动监控是否开启
	if (!IsMiniFilterActive())
	{
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	DbgPrint("%wZ (options:%x)", &Data->Iopb->TargetFileObject->FileName, Data->Iopb->Parameters.Create.Options);

	//进程是否在黑白名单
	if (!IsProcessExcluded(PsGetCurrentProcessId()))
	{
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	UINT32 Options = Data->Iopb->Parameters.Create.Options & 0x00FFFFFF;

	PFLT_FILE_NAME_INFORMATION FltFileNameInfo;
	Status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED, &FltFileNameInfo);
	if (!NT_SUCCESS(Status))
	{
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	BOOLEAN NeededPrevent = FALSE;
	if (!(Options & FILE_DIRECTORY_FILE))    //不是目录
	{
		// If it is create file event
		if (CheckExcludeFileList(__ExcludeFileContext, &(FltFileNameInfo->Name)))   //判断文件是否在黑白名单中
		{
			NeededPrevent = TRUE;
		}
	}

	// If it is create directory/file event
	if (!NeededPrevent && CheckExcludeFileList(__ExcludeDirectoryContext, &FltFileNameInfo->Name))   //判断目录是否在黑白名单中
	{
		NeededPrevent = TRUE;
	}

	FltReleaseFileNameInformation(FltFileNameInfo);
	if (NeededPrevent)
	{
		Data->IoStatus.Status = STATUS_NO_SUCH_FILE;    //Io管理返回给Ring3的结果   
		return FLT_PREOP_COMPLETE;
	}
	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

FLT_PREOP_CALLBACK_STATUS DirectoryCtrlPreviousOperation(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
{
	PAGED_CODE();
	if (!IsMiniFilterActive())
	{
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	if (Data->Iopb->MinorFunction != IRP_MN_QUERY_DIRECTORY)   //如果不是查询 向下发送Irp 并且不需要 调用Post函数
	{
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	switch (Data->Iopb->Parameters.DirectoryControl.QueryDirectory.FileInformationClass)
	{
	case FileIdFullDirectoryInformation:
	case FileIdBothDirectoryInformation:
	case FileBothDirectoryInformation:
	case FileDirectoryInformation:
	case FileFullDirectoryInformation:
	case FileNamesInformation:
		break;
	default:
		return FLT_PREOP_SUCCESS_NO_CALLBACK;
	}

	//如果是查询 向下发送Irp 并且需要 调用Post函数
	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}
FLT_POSTOP_CALLBACK_STATUS DirectoryCtrlPostOperation(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext, FLT_POST_OPERATION_FLAGS Flags)
{
	PAGED_CODE();
	NTSTATUS Status;
	if (!IsMiniFilterActive())
	{
		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	if (!NT_SUCCESS(Data->IoStatus.Status))
	{
		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	if (!IsProcessExcluded(PsGetCurrentProcessId()))
	{
		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	PFLT_FILE_NAME_INFORMATION FltFileNameInfo;
	Status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED, &FltFileNameInfo);
	if (!NT_SUCCESS(Status))
	{
		DbgPrint("FltGetFileNameInformation() Failed with Code:%08x", Status);
		return FLT_POSTOP_FINISHED_PROCESSING;
	}

	__try
	{
		Status = STATUS_SUCCESS;
		
		PFLT_PARAMETERS	FltParameters = &Data->Iopb->Parameters;
		switch (FltParameters->DirectoryControl.QueryDirectory.FileInformationClass)
		{
		case FileFullDirectoryInformation:
		{
			Status = CleanFileFullDirectoryInformation((PFILE_FULL_DIR_INFORMATION)FltParameters->DirectoryControl.QueryDirectory.DirectoryBuffer,FltFileNameInfo);
			break;
		}
		case FileBothDirectoryInformation:
		{
			Status = CleanFileBothDirectoryInformation((PFILE_BOTH_DIR_INFORMATION)FltParameters->DirectoryControl.QueryDirectory.DirectoryBuffer, FltFileNameInfo);
			break;
		}
		case FileDirectoryInformation:
		{
			Status = CleanFileDirectoryInformation((PFILE_DIRECTORY_INFORMATION)FltParameters->DirectoryControl.QueryDirectory.DirectoryBuffer, FltFileNameInfo);
			break;
		}
		case FileIdFullDirectoryInformation:
		{
			Status = CleanFileIdFullDirectoryInformation((PFILE_ID_FULL_DIR_INFORMATION)FltParameters->DirectoryControl.QueryDirectory.DirectoryBuffer, FltFileNameInfo);
			break;
		}
		case FileIdBothDirectoryInformation:
		{
			Status = CleanFileIdBothDirectoryInformation((PFILE_ID_BOTH_DIR_INFORMATION)FltParameters->DirectoryControl.QueryDirectory.DirectoryBuffer, FltFileNameInfo);
			break;
		}
		case FileNamesInformation:
		{
			Status = CleanFileNamesInformation((PFILE_NAMES_INFORMATION)FltParameters->DirectoryControl.QueryDirectory.DirectoryBuffer, FltFileNameInfo);
			break;
		}
		}

		Data->IoStatus.Status = Status;
	}
	__finally
	{
		FltReleaseFileNameInformation(FltFileNameInfo);
	}

	return FLT_POSTOP_FINISHED_PROCESSING;
}


NTSTATUS CleanFileFullDirectoryInformation(PFILE_FULL_DIR_INFORMATION FileFullDirInfo, PFLT_FILE_NAME_INFORMATION FltFileNameInfo)
{
	PAGED_CODE();
	NTSTATUS Status = STATUS_SUCCESS;
	UNICODE_STRING TempStr;
	BOOLEAN IsLoop = TRUE, IsMatched = FALSE;
	PFILE_FULL_DIR_INFORMATION Previous = NULL, Next;
	ULONG32 NextEntryOffset, MoveLength;
	do
	{
		TempStr.Buffer = FileFullDirInfo->FileName;
		TempStr.MaximumLength = TempStr.Length = (USHORT)FileFullDirInfo->FileNameLength;

		if (FileFullDirInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			IsMatched = CheckExcludeDirecoryFileList(__ExcludeDirectoryContext, &FltFileNameInfo->Name, &TempStr);
		}
		else
		{
			IsMatched = CheckExcludeDirecoryFileList(__ExcludeFileContext, &FltFileNameInfo->Name, &TempStr);
		}

		if (IsMatched)
		{
			BOOLEAN IsOk = FALSE;
			
			if (Previous != NULL)
			{
				if (FileFullDirInfo->NextEntryOffset != 0)
				{
					Previous->NextEntryOffset += FileFullDirInfo->NextEntryOffset;
					NextEntryOffset = FileFullDirInfo->NextEntryOffset;
				}
				else
				{
					Previous->NextEntryOffset = 0;
					Status = STATUS_SUCCESS;
					IsOk = TRUE;
				}

				RtlFillMemory(FileFullDirInfo, sizeof(FILE_FULL_DIR_INFORMATION), 0);
			}
			else
			{
				if (FileFullDirInfo->NextEntryOffset != 0)
				{
					Next = (PFILE_FULL_DIR_INFORMATION)((PUCHAR)FileFullDirInfo + FileFullDirInfo->NextEntryOffset);
					MoveLength = 0;
					while (Next->NextEntryOffset != 0)
					{
						MoveLength += Next->NextEntryOffset;
						Next = (PFILE_FULL_DIR_INFORMATION)((PUCHAR)Next + Next->NextEntryOffset);
					}
					MoveLength += FIELD_OFFSET(FILE_FULL_DIR_INFORMATION, FileName) + Next->FileNameLength;
					RtlMoveMemory(FileFullDirInfo, (PUCHAR)FileFullDirInfo + FileFullDirInfo->NextEntryOffset, MoveLength);//continue
				}
				else
				{
					Status = STATUS_NO_MORE_ENTRIES;
					IsOk = TRUE;
				}
			}
			if (IsOk)
			{
				return Status;
			}

			FileFullDirInfo = (PFILE_FULL_DIR_INFORMATION)((PCHAR)FileFullDirInfo + NextEntryOffset);
			continue;
		}

		NextEntryOffset = FileFullDirInfo->NextEntryOffset;
		Previous = FileFullDirInfo;
		FileFullDirInfo = (PFILE_FULL_DIR_INFORMATION)((PCHAR)FileFullDirInfo + NextEntryOffset);

		if (NextEntryOffset == 0)
		{
			IsLoop = FALSE;
		}

	} while (IsLoop);
	return Status;
}

NTSTATUS CleanFileBothDirectoryInformation(PFILE_BOTH_DIR_INFORMATION FileBothInfo, PFLT_FILE_NAME_INFORMATION FltFileNameInfo)
{
	PAGED_CODE();
	NTSTATUS Status;
	UNICODE_STRING TempStr;
	BOOLEAN IsLoop = TRUE, IsMatched = FALSE;
	PFILE_BOTH_DIR_INFORMATION Previous = NULL, Next;
	ULONG32 NextEntryOffset, MoveLength;
	do
	{
		TempStr.Buffer = FileBothInfo->FileName;
		TempStr.MaximumLength = TempStr.Length = (USHORT)FileBothInfo->FileNameLength;

		if (FileBothInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			IsMatched = CheckExcludeDirecoryFileList(__ExcludeDirectoryContext, &FltFileNameInfo->Name, &TempStr);
		}
		else
		{
			IsMatched = CheckExcludeDirecoryFileList(__ExcludeFileContext, &FltFileNameInfo->Name, &TempStr);
		}

		if (IsMatched)
		{
			BOOLEAN IsOk = FALSE;
			if (Previous != NULL)
			{
				if (FileBothInfo->NextEntryOffset != 0)
				{
					Previous->NextEntryOffset += FileBothInfo->NextEntryOffset;
					NextEntryOffset = FileBothInfo->NextEntryOffset;
				}
				else
				{
					Previous->NextEntryOffset = 0;
					Status = STATUS_SUCCESS;
					IsOk = TRUE;
				}
				RtlFillMemory(FileBothInfo, sizeof(FILE_BOTH_DIR_INFORMATION), 0);
			}
			else
			{
				if (FileBothInfo->NextEntryOffset != 0)
				{
					Next = (PFILE_BOTH_DIR_INFORMATION)((PUCHAR)FileBothInfo + FileBothInfo->NextEntryOffset);
					MoveLength = 0;
					while (Next->NextEntryOffset != 0)
					{
						MoveLength += Next->NextEntryOffset;
						Next = (PFILE_BOTH_DIR_INFORMATION)((PUCHAR)Next + Next->NextEntryOffset);
					}
					MoveLength += FIELD_OFFSET(FILE_BOTH_DIR_INFORMATION, FileName) + Next->FileNameLength;
					RtlMoveMemory(FileBothInfo, (PUCHAR)FileBothInfo + FileBothInfo->NextEntryOffset, MoveLength);//continue
				}
				else
				{
					Status = STATUS_NO_MORE_ENTRIES;
					IsOk = TRUE;
				}
			}

			if (IsOk)
			{
				return Status;
			}

			FileBothInfo = (PFILE_BOTH_DIR_INFORMATION)((PCHAR)FileBothInfo + NextEntryOffset);
			continue;
		}

		NextEntryOffset = FileBothInfo->NextEntryOffset;
		Previous = FileBothInfo;
		FileBothInfo = (PFILE_BOTH_DIR_INFORMATION)((PCHAR)FileBothInfo + NextEntryOffset);

		if (NextEntryOffset == 0)
		{
			IsLoop = FALSE;
		}
	} while (IsLoop);
	return Status;
}

NTSTATUS CleanFileDirectoryInformation(PFILE_DIRECTORY_INFORMATION FileDirectoryInfo, PFLT_FILE_NAME_INFORMATION FltFileNameInfo)
{
	PAGED_CODE();
	NTSTATUS Status;
	UNICODE_STRING TempStr;
	BOOLEAN IsLoop = TRUE, IsMatched = FALSE;
	PFILE_DIRECTORY_INFORMATION Previous = NULL, Next;
	ULONG32 NextEntryOffset, MoveLength;
	do
	{
		TempStr.Buffer = FileDirectoryInfo->FileName;
		TempStr.MaximumLength = TempStr.Length = (USHORT)FileDirectoryInfo->FileNameLength;

		if (FileDirectoryInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			IsMatched = CheckExcludeDirecoryFileList(__ExcludeDirectoryContext, &FltFileNameInfo->Name, &TempStr);
		}
		else
		{
			IsMatched = CheckExcludeDirecoryFileList(__ExcludeFileContext, &FltFileNameInfo->Name, &TempStr);
		}

		if (IsMatched)
		{
			BOOLEAN IsOk = FALSE;
			if (Previous != NULL)
			{
				if (FileDirectoryInfo->NextEntryOffset != 0)
				{
					Previous->NextEntryOffset += FileDirectoryInfo->NextEntryOffset;
					NextEntryOffset = FileDirectoryInfo->NextEntryOffset;
				}
				else
				{
					Previous->NextEntryOffset = 0;
					Status = STATUS_SUCCESS;
					IsOk = TRUE;
				}
				RtlFillMemory(FileDirectoryInfo, sizeof(FILE_BOTH_DIR_INFORMATION), 0);
			}
			else
			{
				if (FileDirectoryInfo->NextEntryOffset != 0)
				{
					Next = (PFILE_DIRECTORY_INFORMATION)((PUCHAR)FileDirectoryInfo + FileDirectoryInfo->NextEntryOffset);
					MoveLength = 0;
					while (Next->NextEntryOffset != 0)
					{
						MoveLength += Next->NextEntryOffset;
						Next = (PFILE_DIRECTORY_INFORMATION)((PUCHAR)Next + Next->NextEntryOffset);
					}
					MoveLength += FIELD_OFFSET(FILE_DIRECTORY_INFORMATION, FileName) + Next->FileNameLength;
					RtlMoveMemory(FileDirectoryInfo, (PUCHAR)FileDirectoryInfo + FileDirectoryInfo->NextEntryOffset, MoveLength);//continue
				}
				else
				{
					Status = STATUS_NO_MORE_ENTRIES;
					IsOk = TRUE;
				}
			}

			if (IsOk)
			{
				return Status;
			}

			FileDirectoryInfo = (PFILE_DIRECTORY_INFORMATION)((PCHAR)FileDirectoryInfo + NextEntryOffset);
			continue;
		}

		NextEntryOffset = FileDirectoryInfo->NextEntryOffset;
		Previous = FileDirectoryInfo;
		FileDirectoryInfo = (PFILE_DIRECTORY_INFORMATION)((PCHAR)FileDirectoryInfo + NextEntryOffset);

		if (NextEntryOffset == 0)
		{
			IsLoop = FALSE;
		}
	} while (IsLoop);
	return Status;
}

NTSTATUS CleanFileIdFullDirectoryInformation(PFILE_ID_FULL_DIR_INFORMATION FileIdFullDirInfo, PFLT_FILE_NAME_INFORMATION FltFileNameInfo)
{
	PAGED_CODE();
	NTSTATUS Status;
	UNICODE_STRING TempStr;
	BOOLEAN IsLoop = TRUE, IsMatched = FALSE;
	PFILE_ID_FULL_DIR_INFORMATION Previous = NULL, Next;
	ULONG32 NextEntryOffset, MoveLength;
	do
	{
		TempStr.Buffer = FileIdFullDirInfo->FileName;
		TempStr.MaximumLength = TempStr.Length = (USHORT)FileIdFullDirInfo->FileNameLength;

		if (FileIdFullDirInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			IsMatched = CheckExcludeDirecoryFileList(__ExcludeDirectoryContext, &FltFileNameInfo->Name, &TempStr);
		}
		else
		{
			IsMatched = CheckExcludeDirecoryFileList(__ExcludeFileContext, &FltFileNameInfo->Name, &TempStr);
		}
		if (IsMatched)
		{
			BOOLEAN IsOk = FALSE;
			if (Previous != NULL)
			{
				if (FileIdFullDirInfo->NextEntryOffset != 0)
				{
					Previous->NextEntryOffset += FileIdFullDirInfo->NextEntryOffset;
					NextEntryOffset = FileIdFullDirInfo->NextEntryOffset;
				}
				else
				{
					Previous->NextEntryOffset = 0;
					Status = STATUS_SUCCESS;
					IsOk = TRUE;
				}

				RtlFillMemory(FileIdFullDirInfo, sizeof(FILE_ID_FULL_DIR_INFORMATION), 0);
			}
			else
			{
				if (FileIdFullDirInfo->NextEntryOffset != 0)
				{
					Next = (PFILE_ID_FULL_DIR_INFORMATION)((PUCHAR)FileIdFullDirInfo + FileIdFullDirInfo->NextEntryOffset);
					MoveLength = 0;
					while (Next->NextEntryOffset != 0)
					{
						MoveLength += Next->NextEntryOffset;
						Next = (PFILE_ID_FULL_DIR_INFORMATION)((PUCHAR)Next + Next->NextEntryOffset);
					}

					MoveLength += FIELD_OFFSET(FILE_ID_FULL_DIR_INFORMATION, FileName) + Next->FileNameLength;
					RtlMoveMemory(FileIdFullDirInfo, (PUCHAR)FileIdFullDirInfo + FileIdFullDirInfo->NextEntryOffset, MoveLength);//continue
				}
				else
				{
					Status = STATUS_NO_MORE_ENTRIES;
					IsOk = TRUE;
				}
			}

			if (IsOk)
				return Status;

			FileIdFullDirInfo = (PFILE_ID_FULL_DIR_INFORMATION)((PCHAR)FileIdFullDirInfo + NextEntryOffset);
			continue;
		}

		NextEntryOffset = FileIdFullDirInfo->NextEntryOffset;
		Previous = FileIdFullDirInfo;
		FileIdFullDirInfo = (PFILE_ID_FULL_DIR_INFORMATION)((PCHAR)FileIdFullDirInfo + NextEntryOffset);

		if (NextEntryOffset == 0)
			IsLoop = FALSE;
	} while (IsLoop);

	return STATUS_SUCCESS;
}

NTSTATUS CleanFileIdBothDirectoryInformation(PFILE_ID_BOTH_DIR_INFORMATION FileIdBothDirInfo, PFLT_FILE_NAME_INFORMATION FltFileNameInfo)
{
	PAGED_CODE();
	NTSTATUS Status;
	UNICODE_STRING TempStr;
	BOOLEAN IsLoop = TRUE, IsMatched = FALSE;
	PFILE_ID_BOTH_DIR_INFORMATION Previous = NULL, Next;
	ULONG32 NextEntryOffset, MoveLength;
	do
	{
		TempStr.Buffer = FileIdBothDirInfo->FileName;
		TempStr.MaximumLength = TempStr.Length = (USHORT)FileIdBothDirInfo->FileNameLength;

		if (FileIdBothDirInfo->FileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			IsMatched = CheckExcludeDirecoryFileList(__ExcludeDirectoryContext, &FltFileNameInfo->Name, &TempStr);
		}
		else
		{
			IsMatched = CheckExcludeDirecoryFileList(__ExcludeFileContext, &FltFileNameInfo->Name, &TempStr);
		}
		if (IsMatched)
		{
			BOOLEAN IsOk = FALSE;

			if (Previous != NULL)
			{
				if (FileIdBothDirInfo->NextEntryOffset != 0)
				{
					Previous->NextEntryOffset += FileIdBothDirInfo->NextEntryOffset;
					NextEntryOffset = FileIdBothDirInfo->NextEntryOffset;
				}
				else
				{
					Previous->NextEntryOffset = 0;
					Status = STATUS_SUCCESS;
					IsOk = TRUE;
				}

				RtlFillMemory(FileIdBothDirInfo, sizeof(FILE_ID_BOTH_DIR_INFORMATION), 0);
			}
			else
			{
				if (FileIdBothDirInfo->NextEntryOffset != 0)
				{
					Next = (PFILE_ID_BOTH_DIR_INFORMATION)((PUCHAR)FileIdBothDirInfo + FileIdBothDirInfo->NextEntryOffset);
					MoveLength = 0;
					while (Next->NextEntryOffset != 0)
					{
						MoveLength += Next->NextEntryOffset;
						Next = (PFILE_ID_BOTH_DIR_INFORMATION)((PUCHAR)Next + Next->NextEntryOffset);
					}

					MoveLength += FIELD_OFFSET(FILE_ID_BOTH_DIR_INFORMATION, FileName) + Next->FileNameLength;
					RtlMoveMemory(FileIdBothDirInfo, (PUCHAR)FileIdBothDirInfo + FileIdBothDirInfo->NextEntryOffset, MoveLength);//continue
				}
				else
				{
					Status = STATUS_NO_MORE_ENTRIES;
					IsOk = TRUE;
				}
			}


			if (IsOk)
				return Status;

			FileIdBothDirInfo = (PFILE_ID_BOTH_DIR_INFORMATION)((PCHAR)FileIdBothDirInfo + NextEntryOffset);
			continue;
		}

		NextEntryOffset = FileIdBothDirInfo->NextEntryOffset;
		Previous = FileIdBothDirInfo;
		FileIdBothDirInfo = (PFILE_ID_BOTH_DIR_INFORMATION)((PCHAR)FileIdBothDirInfo + NextEntryOffset);

		if (NextEntryOffset == 0)
			IsLoop = FALSE;
	} while (IsLoop);

	return Status;
}

NTSTATUS CleanFileNamesInformation(PFILE_NAMES_INFORMATION FileNamesInfo, PFLT_FILE_NAME_INFORMATION FltNameInfo)
{
	PAGED_CODE();
	NTSTATUS Status;
	UNICODE_STRING TempStr;
	BOOLEAN IsLoop = TRUE;
	PFILE_NAMES_INFORMATION Previous = NULL, Next;
	ULONG32 NextEntryOffset, MoveLength;
	do
	{
		TempStr.Buffer = FileNamesInfo->FileName;
		TempStr.MaximumLength = TempStr.Length = (USHORT)FileNamesInfo->FileNameLength;

		if (CheckExcludeDirecoryFileList(__ExcludeFileContext, &FltNameInfo->Name, &TempStr))
		{
			BOOLEAN IsOk = FALSE;
			if (Previous != NULL)
			{
				if (FileNamesInfo->NextEntryOffset != 0)
				{
					Previous->NextEntryOffset += FileNamesInfo->NextEntryOffset;
					NextEntryOffset = FileNamesInfo->NextEntryOffset;
				}
				else
				{
					Previous->NextEntryOffset = 0;
					Status = STATUS_SUCCESS;
					IsOk = TRUE;
				}

				RtlFillMemory(FileNamesInfo, sizeof(FILE_NAMES_INFORMATION), 0);
			}
			else
			{
				if (FileNamesInfo->NextEntryOffset != 0)
				{
					Next = (PFILE_NAMES_INFORMATION)((PUCHAR)FileNamesInfo + FileNamesInfo->NextEntryOffset);
					MoveLength = 0;
					while (Next->NextEntryOffset != 0)
					{
						MoveLength += Next->NextEntryOffset;
						Next = (PFILE_NAMES_INFORMATION)((PUCHAR)Next + Next->NextEntryOffset);
					}

					MoveLength += FIELD_OFFSET(FILE_NAMES_INFORMATION, FileName) + Next->FileNameLength;
					RtlMoveMemory(FileNamesInfo, (PUCHAR)FileNamesInfo + FileNamesInfo->NextEntryOffset, MoveLength);//continue
				}
				else
				{
					Status = STATUS_NO_MORE_ENTRIES;
					IsOk = TRUE;
				}
			}

		if (IsOk)
			return Status;

		FileNamesInfo = (PFILE_NAMES_INFORMATION)((PCHAR)FileNamesInfo + NextEntryOffset);
		continue;
		}

		NextEntryOffset = FileNamesInfo->NextEntryOffset;
		Previous = FileNamesInfo;
		FileNamesInfo = (PFILE_NAMES_INFORMATION)((PCHAR)FileNamesInfo + NextEntryOffset);

		if (NextEntryOffset == 0)
			IsLoop = FALSE;
	} while (IsLoop);

	return STATUS_SUCCESS;
}