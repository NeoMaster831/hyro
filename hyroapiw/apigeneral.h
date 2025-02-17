#pragma once

#include "global.h"

UINT64 HyroApiwGnrGetPhysAddr(UINT64 virtualAddress);

PVOID HyroApiwGnrAllocNonPagedBuffer(SIZE_T size);

VOID HyroApiwGnrFreeNonPagedBuffer(PVOID buffer);

typedef struct _APIW_COPY_NONPAGED_BUFFER_REQUEST {
  PVOID dest;
  PVOID src;
  SIZE_T size;
} APIW_COPY_NONPAGED_BUFFER_REQUEST, * PAPIW_COPY_NONPAGED_BUFFER_REQUEST;

VOID HyroApiwGnrCopyNonPagedBuffer(PVOID dest, PVOID src, SIZE_T size);

typedef struct _APIW_EXECUTE_NONPAGED_BUFFER_REQUEST {
  PVOID buffer;
  ULONG maxExecuteLength;
} APIW_EXECUTE_NONPAGED_BUFFER_REQUEST, * PAPIW_EXECUTE_NONPAGED_BUFFER_REQUEST;

UINT64 HyroApiwGnrExecuteNonPagedBuffer(PVOID buffer, ULONG maxExecuteLength);