#include "apiept.h"

BOOL HyroApiwEptAdd(UINT64 physicalAddr) {
  BOOL status = HYROCALL_SUCCESS(ACallHyro(HYRO_VMCALL_EPT_ADDITION, physicalAddr, 0, 0));
  if (!status) {
    APIW_LOG_ERROR("Failed to add EPT hook");
    return FALSE;
  }
  return TRUE;
}

VOID HyroApiwEptRemove(UINT64 physicalAddr) {
  ACallHyro(HYRO_VMCALL_EPT_REMOVAL, physicalAddr, 0, 0);
}

BOOL HyroApiwEptEnable(UINT64 physicalAddr) {
  BOOL status = HYROCALL_SUCCESS(ACallHyro(HYRO_VMCALL_EPT_ENABLE, physicalAddr, 0, 0));
  if (!status) {
    APIW_LOG_ERROR("Failed to enable EPT hook");
    return FALSE;
  }
  return TRUE;
}

VOID HyroApiwEptDisable(UINT64 physicalAddr) {
  ACallHyro(HYRO_VMCALL_EPT_DISABLE, physicalAddr, 0, 0);
}

BOOL HyroApiwEptModify(UINT64 physicalAddr, PVOID hookCtx) {
  // TODO: add additional check for sanity check of hookCtx
  UINT64 hookCtxPhys = MmGetPhysicalAddress(hookCtx).QuadPart;
  BOOL status = HYROCALL_SUCCESS(ACallHyro(HYRO_VMCALL_EPT_MODIFY, physicalAddr, hookCtxPhys, 0));
  if (!status) {
    APIW_LOG_ERROR("Failed to modify EPT hook");
    return FALSE;
  }
  return TRUE;
}