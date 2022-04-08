#pragma once
#include<fltKernel.h>

#pragma prefast(disable:__WARNING_ENCODE_MEMBER_FUNCTION_POINTER, "Not valid for kernel mode drivers")


//Global variables
PFLT_FILTER __FilterHandle;
ULONG_PTR OperationStatusCtx = 1;

CONST PWCHAR __ExcludeFiles[] =
{
	L"\\Device\\HarddiskVolume1\\TestFileMiniFilter.txt",
	NULL
};
CONST PWCHAR __ExcludeDirectorys[] = {
	L"\\Device\\HarddiskVolume1\\TestDirectoryMiniFilter",
	NULL
};


#define PTDBG_TRACE_ROUTINES            0x00000001
#define PTDBG_TRACE_OPERATION_STATUS    0x00000002

ULONG gTraceFlags = 0;


/*************************************************************************
    Prototypes
*************************************************************************/

EXTERN_C_START

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath);
VOID DriverUnload(_In_ PDRIVER_OBJECT DriverObject);

NTSTATUS MiniFilterInstanceSetup(_In_ PCFLT_RELATED_OBJECTS FltObjects, _In_ FLT_INSTANCE_SETUP_FLAGS Flags, _In_ DEVICE_TYPE VolumeDeviceType, _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType);
NTSTATUS MiniFilterUnload(_In_ FLT_FILTER_UNLOAD_FLAGS Flags);

FLT_PREOP_CALLBACK_STATUS CreatePreviousOperation(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext);
FLT_PREOP_CALLBACK_STATUS DirectoryCtrlPreviousOperation(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext);
FLT_POSTOP_CALLBACK_STATUS DirectoryCtrlPostOperation(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext, FLT_POST_OPERATION_FLAGS Flags);

NTSTATUS InitializeMiniFilter(PDRIVER_OBJECT DriverObject);
NTSTATUS DestroyMiniFilter();
BOOLEAN IsMiniFilterActive();
BOOLEAN IsProcessExcluded(HANDLE ProcessIdentify);

NTSTATUS CleanFileFullDirectoryInformation(PFILE_FULL_DIR_INFORMATION FileFullDirInfo, PFLT_FILE_NAME_INFORMATION FltFileNameInfo);
NTSTATUS CleanFileBothDirectoryInformation(PFILE_BOTH_DIR_INFORMATION FileBothInfo, PFLT_FILE_NAME_INFORMATION FltFileNameInfo);
NTSTATUS CleanFileDirectoryInformation(PFILE_DIRECTORY_INFORMATION FileDirectoryInfo, PFLT_FILE_NAME_INFORMATION FltFileNameInfo);
NTSTATUS CleanFileIdFullDirectoryInformation(PFILE_ID_FULL_DIR_INFORMATION FileIdFullDirInfo, PFLT_FILE_NAME_INFORMATION FltFileNameInfo);
NTSTATUS CleanFileIdBothDirectoryInformation(PFILE_ID_BOTH_DIR_INFORMATION FileIdBothDirInfo, PFLT_FILE_NAME_INFORMATION FltFileNameInfo);
NTSTATUS CleanFileNamesInformation(PFILE_NAMES_INFORMATION FileNamesInfo, PFLT_FILE_NAME_INFORMATION FltNameInfo);

NTSTATUS
MiniFilterInstanceSetup(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_SETUP_FLAGS Flags,
    _In_ DEVICE_TYPE VolumeDeviceType,
    _In_ FLT_FILESYSTEM_TYPE VolumeFilesystemType
);

VOID
MiniFilterInstanceTeardownStart(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
);

VOID
MiniFilterInstanceTeardownComplete(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_TEARDOWN_FLAGS Flags
);

NTSTATUS
MiniFilterUnload(
    _In_ FLT_FILTER_UNLOAD_FLAGS Flags
);

NTSTATUS
MiniFilterInstanceQueryTeardown(
    _In_ PCFLT_RELATED_OBJECTS FltObjects,
    _In_ FLT_INSTANCE_QUERY_TEARDOWN_FLAGS Flags
);

EXTERN_C_END

//
//  Assign text sections for each routine.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, MiniFilterUnload)
#pragma alloc_text(PAGE, MiniFilterInstanceQueryTeardown)
#pragma alloc_text(PAGE, MiniFilterInstanceSetup)
#pragma alloc_text(PAGE, MiniFilterInstanceTeardownStart)
#pragma alloc_text(PAGE, MiniFilterInstanceTeardownComplete)
#endif

