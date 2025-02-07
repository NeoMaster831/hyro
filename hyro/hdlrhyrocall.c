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
    HV_LOG_WARNING("HYRO_VMCALL_VMXOFF: unimplemented");
    status = TRUE;
  } break;
  default: {
    HV_LOG_ERROR("Unimplemented hyro vmcall");
  } break;
// --------------------------------------------
  }

  return status;
}