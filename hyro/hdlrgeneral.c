#include "hdlrgeneral.h"

void HdlrTripleFault() { __debugbreak(); }

void HdlrUnconditionalExit(PVCPU pVCpu) { InjectUD(pVCpu); }

#define A(a, b) a |= b
void HdlrMovCr(PVCPU pVCpu) {
  VMX_EXIT_QUALIFICATION_MOV_CR movCrQual =
      *(VMX_EXIT_QUALIFICATION_MOV_CR *)(&pVCpu->exitQual);
  UINT64 *regPtr;
  UINT64 newCr3;
  UINT8 s = 0;
  INVVPID_DESCRIPTOR desc = {1, 0, 0, 0};

  UNUSED_PARAMETER(s);

  regPtr = (UINT64 *)&pVCpu->guestRegs->rax + movCrQual.GeneralPurposeRegister;

  switch (movCrQual.AccessType) {
  case VMX_EXIT_QUALIFICATION_ACCESS_MOV_TO_CR: {
    switch (movCrQual.ControlRegister) {
    case VMX_EXIT_QUALIFICATION_REGISTER_CR0:
      A(s, __vmx_vmwrite(VMCS_GUEST_CR0, *regPtr));
      A(s, __vmx_vmwrite(VMCS_CTRL_CR0_READ_SHADOW, *regPtr));
      break;
    case VMX_EXIT_QUALIFICATION_REGISTER_CR3:
      newCr3 = (*regPtr & ~(1ULL << 63));
      A(s, __vmx_vmwrite(VMCS_GUEST_CR3, newCr3));
      InvVpid(InvvpidSingleContext, &desc);
      break;
    case VMX_EXIT_QUALIFICATION_REGISTER_CR4:
      A(s, __vmx_vmwrite(VMCS_GUEST_CR4, *regPtr));
      A(s, __vmx_vmwrite(VMCS_CTRL_CR4_READ_SHADOW, *regPtr));
      break;
    default:
      HV_LOG_WARNING("Unsupported CR register: %d", movCrQual.ControlRegister);
      break;
    }
  } break;
  case VMX_EXIT_QUALIFICATION_ACCESS_MOV_FROM_CR: {
    switch (movCrQual.ControlRegister) {
    case VMX_EXIT_QUALIFICATION_REGISTER_CR0:
      A(s, __vmx_vmread(VMCS_GUEST_CR0, regPtr));
      break;
    case VMX_EXIT_QUALIFICATION_REGISTER_CR3:
      A(s, __vmx_vmread(VMCS_GUEST_CR3, regPtr));
      break;
    case VMX_EXIT_QUALIFICATION_REGISTER_CR4:
      A(s, __vmx_vmread(VMCS_GUEST_CR4, regPtr));
      break;
    default:
      HV_LOG_WARNING("Unsupported CR register: %d", movCrQual.ControlRegister);
      break;
    }
  } break;
  default:
    HV_LOG_WARNING("Unsupported CR access type: %d", movCrQual.AccessType);
    break;
  }
}
#undef A

#define A(a, b) a |= b
void HdlrWrmsr(PGUEST_REGS pGuestRegs) {
  MSR msr = {0};
  UINT32 targetMsr;
  UINT8 s = 0;

  UNUSED_PARAMETER(s);

  targetMsr = (UINT32)(pGuestRegs->rcx & 0xFFFFFFFF);
  msr.Fields.Low = (ULONG)(pGuestRegs->rax & 0xFFFFFFFF);
  msr.Fields.High = (ULONG)(pGuestRegs->rdx & 0xFFFFFFFF);

  const UINT32 RESERVED_MSR_RANGE_LOW = 0x40000000;
  const UINT32 RESERVED_MSR_RANGE_HIGH = 0x400000F0;
  if (targetMsr <= 0x00001FFF ||
      (0xC0000000 <= targetMsr && targetMsr <= 0xC0001FFF)
    || (targetMsr >= RESERVED_MSR_RANGE_LOW &&
        targetMsr <= RESERVED_MSR_RANGE_HIGH)) {
    
    switch (targetMsr) {
    case IA32_DS_AREA:
    case IA32_FS_BASE:
    case IA32_GS_BASE:
    case IA32_KERNEL_GS_BASE:
    case IA32_LSTAR:
    case IA32_SYSENTER_EIP:
    case IA32_SYSENTER_ESP:
      if (!CheckAddressCanonical(msr.Flags)) {
        InjectGP();
        return;
      }
      break;
    }

    switch (targetMsr) {
    case IA32_SYSENTER_CS:
      A(s, __vmx_vmwrite(VMCS_GUEST_SYSENTER_CS, msr.Flags));
      break;
    case IA32_SYSENTER_ESP:
      A(s, __vmx_vmwrite(VMCS_GUEST_SYSENTER_ESP, msr.Flags));
      break;
    case IA32_SYSENTER_EIP:
      A(s, __vmx_vmwrite(VMCS_GUEST_SYSENTER_EIP, msr.Flags));
      break;
    case IA32_GS_BASE:
      A(s, __vmx_vmwrite(VMCS_GUEST_GS_BASE, msr.Flags));
      break;
    case IA32_FS_BASE:
      A(s, __vmx_vmwrite(VMCS_GUEST_FS_BASE, msr.Flags));
      break;
    default:
      __writemsr(targetMsr, msr.Flags);
      break;
    }
  } else {
    InjectGP();
    return;
  }
}
#undef A

void HdlrCpuid(PVCPU pVCpu) {
  INT32 cpuInfo[4] = {0};
  PGUEST_REGS pGuestRegs = pVCpu->guestRegs;

  __cpuidex(cpuInfo, (INT32)pGuestRegs->rax, (INT32)pGuestRegs->rcx);

  pGuestRegs->rax = cpuInfo[0];
  pGuestRegs->rbx = cpuInfo[1];
  pGuestRegs->rcx = cpuInfo[2];
  pGuestRegs->rdx = cpuInfo[3];
}