#include "apigeneral.h"

UINT64 HyroApiwGnrGetPhysAddr(UINT64 virtualAddress, UINT64 cr3) {
  UINT64 buffer = 0;

  BOOL status = HYROCALL_SUCCESS(ACallHyro(HYRO_VMCALL_GENERAL_GET_PHYSICAL_ADDRESS, virtualAddress, cr3, (UINT64)&buffer));
  if (!status) {
    APIW_LOG_ERROR("Failed to get physical address");
    return 0;
  }
  return buffer;
}

PVOID HyroApiwGnrAllocNonPagedBuffer(SIZE_T size) {
  PVOID buf = NULL;

  BOOL status = HYROCALL_SUCCESS(ACallHyro(HYRO_VMCALL_GENERAL_ALLOC_NONPAGED_BUFFER, size, (UINT64)&buf, 0));
  if (!status) {
    APIW_LOG_ERROR("Failed to allocate non-paged buffer");
    return NULL;
  }

  return buf;
}

VOID HyroApiwGnrFreeNonPagedBuffer(PVOID buffer) {
  ACallHyro(HYRO_VMCALL_GENERAL_FREE_NONPAGED_BUFFER, (UINT64)buffer, 0, 0);
}

VOID HyroApiwGnrCopyNonPagedBuffer(PVOID dest, PVOID src, SIZE_T size) {
  // src may be user-mode address, migrate it to kernel-mode
  PVOID srcKrnl = ExAllocatePoolWithTag(NonPagedPool, size, 'hapw');
  PVOID dstKrnl = ExAllocatePoolWithTag(NonPagedPool, size, 'hapw');
  if (srcKrnl == NULL || dstKrnl == NULL) {
    APIW_LOG_ERROR("Failed to allocate kernel buffer");
    goto COPY_NONPAGED_BUFFER_EXIT;
  }
  
  RtlCopyMemory(srcKrnl, src, size);

  ACallHyro(HYRO_VMCALL_GENERAL_COPY_NONPAGED_BUFFER, (UINT64)dstKrnl, (UINT64)srcKrnl, size);

  RtlCopyMemory(dest, dstKrnl, size);

  // free the kernel buffer
COPY_NONPAGED_BUFFER_EXIT:
  ExFreePoolWithTag(srcKrnl, 'hapw');
  ExFreePoolWithTag(dstKrnl, 'hapw');

  return;
}

UINT64 HyroApiwGnrExecuteNonPagedBuffer(PVOID buffer, ULONG maxExecuteLength) {
  UINT64 result = 0;
  ACallHyro(HYRO_VMCALL_GENERAL_EXECUTE_NONPAGED_BUFFER, (UINT64)buffer, (UINT64)maxExecuteLength, (UINT64)&result);
  return result;
}