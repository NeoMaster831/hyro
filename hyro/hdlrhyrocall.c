#include "hdlrhyrocall.h"

BOOL HdlrHyclVmcall(PVCPU pVCpu) {
  UINT64 callCode = pVCpu->guestRegs->rcx;
  UINT64 param1 = pVCpu->guestRegs->rdx;
  UINT64 param2 = pVCpu->guestRegs->r8;
  UINT64 param3 = pVCpu->guestRegs->r9;
  BOOL status = FALSE;

  switch (callCode) {
// --------------------------------------------
  case HYRO_VMCALL_TEST: {
    HV_LOG_INFO("HYRO_VMCALL_TEST: %llx %llx %llx", param1, param2, param3);
    status = TRUE;
  } break;
  case HYRO_VMCALL_VMXOFF: {
    HV_LOG_INFO("HYRO_VMCALL_VMXOFF");
    VmxDisable(pVCpu);
    status = TRUE;
  } break;
  case HYRO_VMCALL_EPT_ADDITION: {
    HV_LOG_INFO("HYRO_VMCALL_EPT_ADDITION: physAddr - %llx", param1);
    status = MEptHookAddHook(param1);
    if (!status) {
      HV_LOG_ERROR("Failed to add hook");
    }
  } break;
  case HYRO_VMCALL_EPT_REMOVAL: {
    HV_LOG_INFO("HYRO_VMCALL_EPT_REMOVAL: physAddr - %llx", param1);
    MEptHookRemoveHook(param1);
    status = TRUE;
  } break;
  case HYRO_VMCALL_EPT_ENABLE: {
    HV_LOG_INFO("HYRO_VMCALL_EPT_ENABLE: physAddr - %llx", param1);
    PEPT_HOOK_PAGE pEptHook = MEptHookGetHook(param1);
    if (!pEptHook) {
      HV_LOG_ERROR("Failed to get hook");
      status = FALSE;
      break;
    }
    MEptHookActivateHook(pEptHook);
    status = TRUE;
  } break;
  case HYRO_VMCALL_EPT_DISABLE: {
    HV_LOG_INFO("HYRO_VMCALL_EPT_DISABLE: physAddr - %llx", param1);
    PEPT_HOOK_PAGE pEptHook = MEptHookGetHook(param1);
    if (!pEptHook) {
      HV_LOG_ERROR("Failed to get hook");
      status = FALSE;
      break;
    }
    MEptHookDeactivateHook(pEptHook);
    status = TRUE;
  } break;
  default: {
    HV_LOG_ERROR("Unimplemented hyro vmcall");
  } break;
// --------------------------------------------
  }

  return status;
}