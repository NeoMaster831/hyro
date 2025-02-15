#include "apitest.h"

BOOL HyroApiwTest() {
  return HYROCALL_SUCCESS(ACallHyro(HYRO_VMCALL_TEST, 0x69, 0x74, 0x420));
}

#define A(a) if (!(a)) { return FALSE; }
BOOL HyroApiwTestEpt() {
  // Build Test function
  PVOID testFunc = (PVOID)KeGetCurrentIrql;
  
  APIW_LOG_INFO("Test function: %p", testFunc);
  APIW_LOG_INFO("Test function phys: 0x%llx", MmGetPhysicalAddress(testFunc).QuadPart);

  PVOID preBuf = ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, 'hapw');

  A(preBuf)
  RtlZeroMemory(preBuf, PAGE_SIZE);

  PMDL mdl = IoAllocateMdl(preBuf, PAGE_SIZE, FALSE, FALSE, NULL);
  A(mdl)

  MmBuildMdlForNonPagedPool(mdl);
  PVOID rwxBuffer = MmMapLockedPagesSpecifyCache(mdl, KernelMode, MmCached, NULL, FALSE, NormalPagePriority);
  A(!MmProtectMdlSystemAddress(mdl, PAGE_EXECUTE_READWRITE))

  RtlCopyMemory(rwxBuffer, testFunc, PAGE_SIZE);

  APIW_LOG_INFO("Test function copied to RWX buffer: %p", rwxBuffer);

  PHYSICAL_ADDRESS rwxBufferPhys = MmGetPhysicalAddress(rwxBuffer);

  APIW_LOG_INFO("Test function phys (RWX buffer): 0x%llx", rwxBufferPhys.QuadPart);

  // Add EPT hook for test function
  A(HYROCALL_SUCCESS(ACallHyro(HYRO_VMCALL_EPT_ADDITION, rwxBufferPhys.QuadPart, 0, 0)))
  A(HYROCALL_SUCCESS(ACallHyro(HYRO_VMCALL_EPT_ENABLE, rwxBufferPhys.QuadPart, 0, 0)))

  // Now the EPT hook should be active
  // First, test reading (should exit)

  UCHAR test = *(UCHAR *)rwxBuffer;
  APIW_LOG_INFO("Test function byte: %x", test);

  // Secondly execute the function
  UCHAR returnVal = ((UCHAR(*)())(rwxBuffer))();
  APIW_LOG_INFO("Test function return value: %x", returnVal);
  A(!returnVal)

  // Now try modifying the hook context
  PUCHAR hookCtx = (PUCHAR)ExAllocatePoolWithTag(NonPagedPool, PAGE_SIZE, 'hapw');
  A(hookCtx)

  RtlCopyMemory(hookCtx, rwxBuffer, PAGE_SIZE);
  *(UCHAR*)(hookCtx) = 0xB8;
  *(UINT32*)(hookCtx + 1) = 0x69;
  *(UCHAR*)(hookCtx + 5) = 0xC3;

  A(HYROCALL_SUCCESS(ACallHyro(HYRO_VMCALL_EPT_MODIFY, rwxBufferPhys.QuadPart, MmGetPhysicalAddress(hookCtx).QuadPart, 0)))

  // Now the hook should be modified
  
  test = *(UCHAR*)rwxBuffer;
  APIW_LOG_INFO("Test function byte after hook: %x", test); // Should NOT be 0xb8

  // Secondly execute the function
  returnVal = ((UCHAR(*)())(rwxBuffer))();
  APIW_LOG_INFO("Test function return value after hook: %x", returnVal); // should be 0x69
  A(returnVal == 0x69)

  // Disable the hook
  A(HYROCALL_SUCCESS(ACallHyro(HYRO_VMCALL_EPT_DISABLE, rwxBufferPhys.QuadPart, 0, 0)))

  // Now try the same
  // It shouldn't exit
  test = *(UCHAR*)rwxBuffer;
  APIW_LOG_INFO("Test function byte (Second try): %x", test);

  returnVal = ((UCHAR(*)())(rwxBuffer))();
  APIW_LOG_INFO("Test function return value (Second try): %x", returnVal);
  A(!returnVal)

  // Remove the hook
  A(HYROCALL_SUCCESS(ACallHyro(HYRO_VMCALL_EPT_REMOVAL, rwxBufferPhys.QuadPart, 0, 0)))


  ExFreePoolWithTag(hookCtx, 'hapw');
  MmUnmapLockedPages(rwxBuffer, mdl);
  IoFreeMdl(mdl);
  ExFreePoolWithTag(preBuf, 'hapw');

  return TRUE;
}
#undef A