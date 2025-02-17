#include "apiggeneral.h"

VOID HyroApiwGuestGnrCopyNonPagedBuffer(PVOID dest, PVOID src, SIZE_T size) {
  RtlCopyMemory(dest, src, size);
}

UINT64 HyroApiwGuestGnrExecuteNonPagedBuffer(PVOID buffer, ULONG maxExecuteLength) {
  PMDL mdl = IoAllocateMdl(buffer, maxExecuteLength, FALSE, FALSE, NULL);

  if (mdl == NULL) {
    APIW_LOG_ERROR("Failed to allocate MDL");
    return 0xFFFF'FFFF'FFFF'FFFF;
  }

  MmBuildMdlForNonPagedPool(mdl);

  PVOID rwxBuffer = MmMapLockedPagesSpecifyCache(mdl, KernelMode, MmCached, NULL, FALSE, NormalPagePriority);
  if (rwxBuffer == NULL) {
    APIW_LOG_ERROR("Failed to map locked pages");
    IoFreeMdl(mdl);
    return 0xFFFF'FFFF'FFFF'FFFF;
  }

  if (MmProtectMdlSystemAddress(mdl, PAGE_EXECUTE_READWRITE) != STATUS_SUCCESS) {
    APIW_LOG_ERROR("Failed to protect MDL system address");
    MmUnmapLockedPages(rwxBuffer, mdl);
    IoFreeMdl(mdl);
    return 0xFFFF'FFFF'FFFF'FFFF;
  }

  // Execute the buffer
  UINT64 result = ((UINT64 (*)())rwxBuffer)();

  MmUnmapLockedPages(rwxBuffer, mdl);
  IoFreeMdl(mdl);

  return result;
}