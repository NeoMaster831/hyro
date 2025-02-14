#include "apitest.h"

BOOL HyroApiwTest() {
  return HYROCALL_SUCCESS(ACallHyro(HYRO_VMCALL_TEST, 0x69, 0x74, 0x420));
}

#define A(a) if (!(a)) { return FALSE; }
BOOL HyroApiwTestEpt() {
  PHYSICAL_ADDRESS testFuncPhys = MmGetPhysicalAddress((PVOID)KeGetCurrentIrql);

  // Add EPT hook for test function
  A(HYROCALL_SUCCESS(ACallHyro(HYRO_VMCALL_EPT_ADDITION, testFuncPhys.QuadPart, 0, 0)))
  A(HYROCALL_SUCCESS(ACallHyro(HYRO_VMCALL_EPT_ENABLE, testFuncPhys.QuadPart, 0, 0)))

  // Now the EPT hook should be active
  // First, test reading (should exit)

  UCHAR test = *(UCHAR *)KeGetCurrentIrql;
  APIW_LOG_INFO("Test function byte: %x", test);

  // Secondly execute the function
  A(!KeGetCurrentIrql())

  // Disable the hook
  A(HYROCALL_SUCCESS(ACallHyro(HYRO_VMCALL_EPT_DISABLE, testFuncPhys.QuadPart, 0, 0)))

  // Now try the same
  // It shouldn't exit
  test = *(UCHAR*)KeGetCurrentIrql;
  APIW_LOG_INFO("Test function byte (Second try): %x", test);

  A(!KeGetCurrentIrql())

  // Remove the hook
  A(HYROCALL_SUCCESS(ACallHyro(HYRO_VMCALL_EPT_REMOVAL, testFuncPhys.QuadPart, 0, 0)))

  return TRUE;
}
#undef A