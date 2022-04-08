# StructInfo

- FLT_RELATED_OBJECTS

```c++
typedef struct _FLT_RELATED_OBJECTS {
  USHORT        Size;
  USHORT        TransactionContext; //事务context
  PFLT_FILTER   Filter;
  PFLT_VOLUME   Volume;
  PFLT_INSTANCE Instance;
  PFILE_OBJECT  FileObject;
  PKTRANSACTION Transaction;
} FLT_RELATED_OBJECTS, *PFLT_RELATED_OBJECTS;
//此结构由Filter Manger填充好，下发给MiniFilter，一般作为回调函数的参数

```

- PfltInstanceSetupCallback

```c++
PFLT_INSTANCE_SETUP_CALLBACK PfltInstanceSetupCallback;

NTSTATUS PfltInstanceSetupCallback(
  [in] PCFLT_RELATED_OBJECTS FltObjects,
  [in] FLT_INSTANCE_SETUP_FLAGS Flags, //why the instance beging attached
  [in] DEVICE_TYPE VolumeDeviceType,
  [in] FLT_FILESYSTEM_TYPE VolumeFilesystemType
)
{...}

```

- FLT_CALLBACK_DATA

```C++
typedef struct _FLT_CALLBACK_DATA {
  FLT_CALLBACK_DATA_FLAGS     Flags;
  PETHREAD                    Thread;
  PFLT_IO_PARAMETER_BLOCK     Iopb;
  IO_STATUS_BLOCK             IoStatus;
  struct _FLT_TAG_DATA_BUFFER *TagData;
  union {
    struct {
      LIST_ENTRY QueueLinks;
      PVOID      QueueContext[2];
    };
    PVOID FilterContext[4];
  };
  KPROCESSOR_MODE             RequestorMode;
} FLT_CALLBACK_DATA, *PFLT_CALLBACK_DATA;

Flags
//FLTFL_CALLBACK_DATA_DIRTY MiniFilter by calling FltSetCallbackDataDirty to indiate that it has modified the contents of the callback data structure.
    
```

- IO_PARAMETER_BLOCK

```C++
typedef struct _FLT_IO_PARAMETER_BLOCK {
  ULONG          IrpFlags; //Specify various aspects of I/O ope
  UCHAR          MajorFunction;
  UCHAR          MinorFunction;
  UCHAR          OperationFlags;
  UCHAR          Reserved;
  PFILE_OBJECT   TargetFileObject;
  PFLT_INSTANCE  TargetInstance;
  FLT_PARAMETERS Parameters;
} FLT_IO_PARAMETER_BLOCK, *PFLT_IO_PARAMETER_BLOCK;	
```

- FLT_FILE_NAME_INFORMATION

```C++
typedef struct _FLT_FILE_NAME_INFORMATION {
  USHORT                     Size;
  FLT_FILE_NAME_PARSED_FLAGS NamesParsed;
  FLT_FILE_NAME_OPTIONS      Format;
  UNICODE_STRING             Name;
  UNICODE_STRING             Volume;
  UNICODE_STRING             Share;
  UNICODE_STRING             Extension;
  UNICODE_STRING             Stream;
  UNICODE_STRING             FinalComponent;
  UNICODE_STRING             ParentDir;
} FLT_FILE_NAME_INFORMATION, *PFLT_FILE_NAME_INFORMATION;
```

It contains file name information.

- FILE_FULL_DIR_INFORMATION

```C++
typedef struct _FILE_FULL_DIR_INFORMATION {
  ULONG         NextEntryOffset;
  ULONG         FileIndex;
  LARGE_INTEGER CreationTime;
  LARGE_INTEGER LastAccessTime;
  LARGE_INTEGER LastWriteTime;
  LARGE_INTEGER ChangeTime;
  LARGE_INTEGER EndOfFile;
  LARGE_INTEGER AllocationSize;
  ULONG         FileAttributes;
  ULONG         FileNameLength;
  ULONG         EaSize;
  WCHAR         FileName[1];
} FILE_FULL_DIR_INFORMATION, *PFILE_FULL_DIR_INFORMATION;
```

