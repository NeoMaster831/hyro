#include "exit.h"

#define A(a, b) a |= b
BOOL VmxExitHandler(PGUEST_REGS pGuestRegs) {
  size_t exitReason = 0;
  UINT8 s = 0;
  BOOL result = FALSE;
  PVCPU pVCpu = &g_arrVCpu[KeGetCurrentProcessorNumberEx(NULL)];

  pVCpu->guestRegs = pGuestRegs;
  pVCpu->isOnVmxRootMode = TRUE;
  
  A(s, __vmx_vmread(VMCS_EXIT_REASON, &exitReason));
  exitReason &= 0xFFFF;

  pVCpu->incrementRip = TRUE;

  A(s, __vmx_vmread(VMCS_GUEST_RIP, &pVCpu->lastVmexitRip));
  A(s, __vmx_vmread(VMCS_GUEST_RSP, &pVCpu->guestRegs->rsp));
  A(s, __vmx_vmread(VMCS_EXIT_QUALIFICATION, &pVCpu->exitQual));
  pVCpu->exitQual &= 0xFFFFFFFF;

  if (s) {
    HV_LOG_ERROR("Something went wrong while reading VMCS values");
    goto RETURN_ROUTINE;
  }

  // Those are causing freaky lags
  //HV_LOG_INFO("VM-exit reason: 0x%llx", exitReason);
  //HV_LOG_INFO("VM-exit qual: 0x%llx", pVCpu->exitQual);

  switch (exitReason) {
// ---------------------------------------------------------------
  case VMX_EXIT_REASON_TRIPLE_FAULT: {
    HdlrTripleFault();
  } break;
  case VMX_EXIT_REASON_EXECUTE_VMCLEAR:
  case VMX_EXIT_REASON_EXECUTE_VMPTRLD:
  case VMX_EXIT_REASON_EXECUTE_VMPTRST:
  case VMX_EXIT_REASON_EXECUTE_VMREAD:
  case VMX_EXIT_REASON_EXECUTE_VMRESUME:
  case VMX_EXIT_REASON_EXECUTE_VMWRITE:
  case VMX_EXIT_REASON_EXECUTE_VMXOFF:
  case VMX_EXIT_REASON_EXECUTE_VMXON:
  case VMX_EXIT_REASON_EXECUTE_VMLAUNCH: {
    HdlrUnconditionalExit(pVCpu);
  } break;
  case VMX_EXIT_REASON_EXECUTE_INVEPT:
  case VMX_EXIT_REASON_EXECUTE_INVVPID:
  case VMX_EXIT_REASON_EXECUTE_GETSEC:
  case VMX_EXIT_REASON_EXECUTE_INVD: {
    // TODO: Will implement later
    HdlrUnconditionalExit(pVCpu);
  } break;
  case VMX_EXIT_REASON_MOV_CR: {
    HdlrMovCr(pVCpu);
  } break;
  case VMX_EXIT_REASON_EXECUTE_RDMSR: {
    // Do nothing
  } break;
  case VMX_EXIT_REASON_IO_SMI:
  case VMX_EXIT_REASON_SMI: {
    // Do nothing
  } break;
  case VMX_EXIT_REASON_EXECUTE_WRMSR: {
    HdlrWrmsr(pVCpu->guestRegs);
  } break;
  case VMX_EXIT_REASON_EXECUTE_CPUID: {
    HdlrCpuid(pVCpu);
  } break;
// ---------------------------------------------------------------
  }


RETURN_ROUTINE:
  
  if (!pVCpu->vmxoffState.IsVmxoffExecuted && pVCpu->incrementRip) {
    UINT64 resumeRip = 0, currentRip = 0;
    size_t exitInstructionLength = 0;

    A(s, __vmx_vmread(VMCS_GUEST_RIP, &currentRip));
    A(s, __vmx_vmread(VMCS_VMEXIT_INSTRUCTION_LENGTH, &exitInstructionLength));
    resumeRip = currentRip + exitInstructionLength;
    A(s, __vmx_vmwrite(VMCS_GUEST_RIP, resumeRip));

    if (s) {
      HV_LOG_ERROR("Something went wrong while reading VMCS values");
      __debugbreak(); // we can't resume anymore, we just break it
    }
  }
  if (pVCpu->vmxoffState.IsVmxoffExecuted) {
    result = TRUE;
  }

  pVCpu->isOnVmxRootMode = FALSE;

  return result;
}
#undef A