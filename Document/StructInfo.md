# StructInfo

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
```

