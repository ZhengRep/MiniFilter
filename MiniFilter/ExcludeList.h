#pragma once
#include<fltKernel.h>

typedef PVOID PEXCLUDE_CONTEXT;
typedef PEXCLUDE_CONTEXT* PPEXCLUDE_CONTEXT;

typedef struct _EXCLUDE_FILE_CONTEXT
{
	LIST_ENTRY			ListEntry;  //Flink Blink
	FAST_MUTEX			FastMutex;
} EXCLUDE_FILE_CONTEXT, * PEXCLUDE_FILE_CONTEXT;

typedef struct _EXCULDE_FILE_PATH
{
	UNICODE_STRING		FullPath;
	UNICODE_STRING		DirectoryName;
	UNICODE_STRING		FileName;
} EXCULDE_FILE_PATH, * PEXCULDE_FILE_PATH;

typedef struct _EXCLUDE_FILE_LIST_ENTRY 
{
	LIST_ENTRY			ListEntry;
	EXCULDE_FILE_PATH	ExculdeFilePath;
} EXCLUDE_FILE_LIST_ENTRY, * PEXCLUDE_FILE_LIST_ENTRY;

//��ʼ��ListEntry
NTSTATUS InitializeExcludeListContext(PPEXCLUDE_CONTEXT ExcludeContext);

//�����ڴ���ListEntry�����
NTSTATUS AddExcludeFileList(PEXCLUDE_CONTEXT ExcludeContext, PUNICODE_STRING FilePath);
NTSTATUS AddExcludeDirectoryList(PEXCLUDE_CONTEXT ExcludeContext, PUNICODE_STRING DirectoryPath);
NTSTATUS AddExcludeListEntry(PEXCLUDE_CONTEXT ExcludeContext, PUNICODE_STRING FilePath);   //Ŀ¼Ҳ���ļ��е�һ��
BOOLEAN FillDirectoryFromPath(PEXCULDE_FILE_PATH ExculdeFilePath, PUNICODE_STRING FilePath);

//��ListEntry���Ƴ��ͷ��ڴ�
VOID DestroyExcludeListContext(PEXCLUDE_CONTEXT ExcludeContext);
NTSTATUS RemoveAllExcludeListEntry(PEXCLUDE_CONTEXT ExcludeContext);

//�ж������Ƿ������ݽṹ��
BOOLEAN CheckExcludeFileList(PEXCLUDE_CONTEXT ExcludeContext, PCUNICODE_STRING FilePath);
BOOLEAN CheckExcludeDirecoryFileList(PEXCLUDE_CONTEXT ExcludeContext, PCUNICODE_STRING DirectoryName, PCUNICODE_STRING FileName);