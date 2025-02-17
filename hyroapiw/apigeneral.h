#pragma once

#include "global.h"

typedef struct _APIW_GET_PHYS_ADDR_REQUEST {
  UINT64 virtualAddress;
  UINT64 cr3;
} APIW_GET_PHYS_ADDR_REQUEST, * PAPIW_GET_PHYS_ADDR_REQUEST;

UINT64 HyroApiwGnrGetPhysAddr(UINT64 virtualAddress, UINT64 cr3);

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