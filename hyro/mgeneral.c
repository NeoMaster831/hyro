#include "mgeneral.h"

BOOL MGeneralInitalize() {
  return TRUE;
}

VOID MGeneralTerminate() {
  return;
}

UINT64 MGeneralGetPhysicalAddress(UINT64 virtualAddress) {
  return MmGetPhysicalAddress((PVOID)virtualAddress).QuadPart;
}

PVOID MGeneralAllocateNonPagedBuffer(UINT64 size) {
  PVOID buf = MemAlloc_ZNP(size);
  return buf;
}

VOID MGeneralFreeNonPagedBuffer(PVOID buffer) {
  MemFree_P(buffer);
}

VOID MGeneralCopyNonPagedBuffer(PVOID dest, PVOID src, SIZE_T size) {
  RtlCopyMemory(dest, src, size);
}

UINT64 MGeneralExecuteNonPagedBuffer(PVOID buffer, ULONG maxExecuteLength) {

  PMDL mdl = IoAllocateMdl(buffer, maxExecuteLength, FALSE, FALSE, NULL);

  if (mdl == NULL) {
    HV_LOG_ERROR("Failed to allocate MDL");
    return 0xFFFF'FFFF'FFFF'FFFF;
  }

  MmBuildMdlForNonPagedPool(mdl);

  PVOID rwxBuffer = MmMapLockedPagesSpecifyCache(mdl, KernelMode, MmCached, NULL, FALSE, NormalPagePriority);

  if (rwxBuffer == NULL) {
    HV_LOG_ERROR("Failed to map locked pages");
    IoFreeMdl(mdl);
    return 0xFFFF'FFFF'FFFF'FFFF;
  }

  if (MmProtectMdlSystemAddress(mdl, PAGE_EXECUTE_READWRITE) != STATUS_SUCCESS) {
    HV_LOG_ERROR("Failed to protect MDL system address");
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