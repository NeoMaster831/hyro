#include "hdlrhyrocall.h"

BOOL HdlrHyclVmcall(PVCPU pVCpu) {
  UINT64 callCode = pVCpu->guestRegs->rcx;
  UINT64 param1 = pVCpu->guestRegs->rdx;
  UINT64 param2 = pVCpu->guestRegs->r8;
  UINT64 param3 = pVCpu->guestRegs->r9;
  BOOL status = FALSE;

#ifndef DBG
  UNREFERENCED_PARAMETER(param3);
  UNREFERENCED_PARAMETER(param2);
  UNREFERENCED_PARAMETER(param1);
#endif

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

void HdlrHyclCpuid(PVCPU pVCpu) {
  UINT64 callCode = pVCpu->guestRegs->rcx;
  UINT64 param1 = pVCpu->guestRegs->rdx;
  UINT64 param2 = pVCpu->guestRegs->r8;

  callCode &= 0b11;
  param1 &= 0xF;

  switch (callCode) {
  case HYRO_CPUID_XORCALL_CONST:
    g_Hvm.memory[param1] ^= param2;
    break;
  case HYRO_CPUID_XORCALL_MEMORY:
    g_Hvm.memory[param1] ^= g_Hvm.memory[param2 & 0xF];
    break;
  case HYRO_CPUID_OUT:
    pVCpu->guestRegs->r15 = g_Hvm.memory[param1];
  default:
    break;
  }

  return;
}