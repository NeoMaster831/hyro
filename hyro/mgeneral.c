#include "mgeneral.h"

BOOL MGeneralInitalize() {
  return TRUE;
}

VOID MGeneralTerminate() {
  return;
}

#define A(a, b) a |= b
UINT64 MGeneralGetPhysicalAddress(UINT64 virtualAddress, UINT64 cr3) {
  UINT8 s = 0;
  UINT64 originalCr3 = __readcr3();
  UINT64 targetCr3 = cr3;
  UNUSED_PARAMETER(s);
  
  if (targetCr3 == 0) { // means the caller wants to use the current process's CR3
    A(s, __vmx_vmread(VMCS_GUEST_CR3, &targetCr3));
  }
  __writecr3(targetCr3);
  
  UINT64 physAddr = MmGetPhysicalAddress((PVOID)virtualAddress).QuadPart;
  HV_LOG_INFO("Converted Physical Address: %llx", physAddr);

  __writecr3(originalCr3);

  return physAddr;
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