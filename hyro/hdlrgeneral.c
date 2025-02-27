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
      A(s, InvVpid(InvvpidSingleContext, &desc));
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
void HdlrRdmsr(PGUEST_REGS pGuestRegs) {
  const UINT32 RESERVED_MSR_RANGE_LOW = 0x40000000;
  const UINT32 RESERVED_MSR_RANGE_HIGH = 0x400000F0;
  
  MSR msr = { 0 };
  UINT32 targetMsr;
  UINT8 s = 0;

  targetMsr = (UINT32)(pGuestRegs->rcx & 0xFFFFFFFF);
  if (targetMsr <= 0x00001FFF ||
    (0xC0000000 <= targetMsr && targetMsr <= 0xC0001FFF)
    || (targetMsr >= RESERVED_MSR_RANGE_LOW &&
      targetMsr <= RESERVED_MSR_RANGE_HIGH)) {

    switch (targetMsr) {
    case IA32_SYSENTER_CS:
      A(s, __vmx_vmread(VMCS_GUEST_SYSENTER_CS, &msr.Flags));
      break;
    case IA32_SYSENTER_ESP:
      A(s, __vmx_vmread(VMCS_GUEST_SYSENTER_ESP, &msr.Flags));
      break;
    case IA32_SYSENTER_EIP:
      A(s, __vmx_vmread(VMCS_GUEST_SYSENTER_EIP, &msr.Flags));
      break;
    case IA32_GS_BASE:
      A(s, __vmx_vmread(VMCS_GUEST_GS_BASE, &msr.Flags));
      break;
    case IA32_FS_BASE:
      A(s, __vmx_vmread(VMCS_GUEST_FS_BASE, &msr.Flags));
      break;
    default: {

      if (targetMsr < 0x1000 && BitTest((const LONG*)g_invalidMsrBitmap, targetMsr) != 0) {
        InjectGP();
        return;
      }

      msr.Flags = __readmsr(targetMsr);

      pGuestRegs->rax = 0;
      pGuestRegs->rdx = 0;
      pGuestRegs->rax = msr.Fields.Low;
      pGuestRegs->rdx = msr.Fields.High;

    } break;

    }
  }
  else {
    InjectGP();
    return;
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

void HdlrIoInstruction(PVCPU pVCpu) {
  UINT16 port = 0;
  UINT32 size = 0;
  UINT32 count = 0;
  PGUEST_REGS pGuestRegs = pVCpu->guestRegs;
  VMX_EXIT_QUALIFICATION_IO_INSTRUCTION ioQual =
      *(VMX_EXIT_QUALIFICATION_IO_INSTRUCTION *)(&pVCpu->exitQual);

  union {
    unsigned char *AsBytePtr;
    unsigned short *AsWordPtr;
    unsigned long *AsDwordPtr;
    void *AsPtr;
    UINT64 AsUint64;
  } PortValue = {.AsUint64 = 0};

  if (ioQual.StringInstruction) {
    PortValue.AsPtr = (PVOID)(ioQual.DirectionOfAccess == 1 ? pGuestRegs->rdi
                                                            : pGuestRegs->rsi);
  } else {
    PortValue.AsPtr = &pGuestRegs->rax;
  }

  port = (UINT16)ioQual.PortNumber;
  count = ioQual.RepPrefixed ? (pGuestRegs->rcx & 0xFFFFFFFF) : 1;
  size = (UINT32)(ioQual.SizeOfAccess + 1);

  switch (ioQual.DirectionOfAccess) {
  case 1: // AccessIn
    if (ioQual.StringInstruction) {
      switch (size) {
      case 1:
        __inbytestring(port, (UINT8 *)PortValue.AsBytePtr, count);
        break;
      case 2:
        __inwordstring(port, (UINT16 *)PortValue.AsWordPtr, count);
        break;
      case 4:
        __indwordstring(port, (unsigned long *)PortValue.AsDwordPtr, count);
        break;
      }
    } else {
      switch (size) {
      case 1:
        *PortValue.AsBytePtr = __inbyte(port);
        break;
      case 2:
        *PortValue.AsWordPtr = __inword(port);
        break;
      case 4:
        *PortValue.AsDwordPtr = __indword(port);
        break;
      }
    }
    break;
  case 0: // AccessOut
    if (ioQual.StringInstruction) {
      switch (size) {
      case 1:
        __outbytestring(port, (UINT8 *)PortValue.AsBytePtr, count);
        break;
      case 2:
        __outwordstring(port, (UINT16 *)PortValue.AsWordPtr, count);
        break;
      case 4:
        __outdwordstring(port, (unsigned long *)PortValue.AsDwordPtr, count);
        break;
      }
    } else {
      switch (size) {
      case 1:
        __outbyte(port, *PortValue.AsBytePtr);
        break;
      case 2:
        __outword(port, *PortValue.AsWordPtr);
        break;
      case 4:
        __outdword(port, *PortValue.AsDwordPtr);
        break;
      }
    }
    break;
  }

  if (ioQual.StringInstruction && ioQual.RepPrefixed) {
    pGuestRegs->rcx = 0;
  }
}

#define A(a, b) a |= b
void HdlrEptMisconfiguration() {
  UINT64 guestPhysAddr = 0;
  UINT8 s = 0;

  UNUSED_PARAMETER(s);

  A(s, __vmx_vmread(VMCS_GUEST_PHYSICAL_ADDRESS, &guestPhysAddr));

  HV_LOG_INFO("Fuck my coding issue just fucked this ept configuration lol");
  HV_LOG_ERROR("EPT misconfiguration at 0x%llx", guestPhysAddr);

  // Can't continue
  // __halt(); // Oh wait if we do this then we fucking can't see the error!
}
#undef A

BOOL HdlrVmcall(PVCPU pVCpu) {
  BOOLEAN isHyroVmcall = FALSE;
  PGUEST_REGS pGuestRegs = pVCpu->guestRegs;

  isHyroVmcall = (pGuestRegs->r10 == HYRO_SIGNATURE_LOW &&
                  pGuestRegs->r11 == HYRO_SIGNATURE_MEDIUM &&
                  pGuestRegs->r12 == HYRO_SIGNATURE_HIGH);

  if (isHyroVmcall) {
    BOOL result = HdlrHyclVmcall(pVCpu);
    pGuestRegs->rax = result ? HYRO_VMCALL_SUCCESS : 0;
    return result;
  } else {
    // Since we don't use Hyper-V, we don't need to handle this shit
  }

  return FALSE;
}

void HdlrNmiWindowExit(PVCPU pVCpu) {
  InjectInterruption(INTERRUPT_TYPE_NMI, EXCEPTION_VECTOR_NMI, FALSE, 0);
  pVCpu->incrementRip = FALSE;
}

void HdlrRdpmc(PVCPU pVCpu) {
  UINT32 ecx = 0;
  PGUEST_REGS pGuestRegs = pVCpu->guestRegs;
  UINT64 pmc = 0;

  ecx = (UINT32)(pGuestRegs->rcx & 0xFFFFFFFF);
  pmc = __readpmc(ecx);
  pGuestRegs->rax = (pmc & 0xFFFFFFFF);
  pGuestRegs->rdx = ((pmc >> 32) & 0xFFFFFFFF);
}

#define A(a, b) a |= b
void HdlrMovDr(PVCPU pVCpu) {
  VMX_EXIT_QUALIFICATION_MOV_DR drQual =
      *(VMX_EXIT_QUALIFICATION_MOV_DR *)&pVCpu->exitQual;
  CR4 cr4 = {0};
  DR7 dr7 = {0};
  VMX_SEGMENT_SELECTOR cs = {0};
  UINT64 *gpRegs = (UINT64 *)pVCpu->guestRegs;
  UINT8 s = 0;

  UNUSED_PARAMETER(s);

  UINT64 gpReg = gpRegs[drQual.GeneralPurposeRegister];
  A(s, __vmx_vmread(VMCS_GUEST_CS_BASE, &cs.Base));
  A(s, __vmx_vmread(VMCS_GUEST_CS_LIMIT, (UINT64 *)&cs.Limit));
  A(s,
    __vmx_vmread(VMCS_GUEST_CS_ACCESS_RIGHTS, (UINT64 *)&cs.Attributes.AsUInt));
  A(s, __vmx_vmread(VMCS_GUEST_CS_SELECTOR, (UINT64 *)&cs.Selector));
  
  if (cs.Attributes.s.DescriptorPrivilegeLevel != 0) {
    InjectGP();
    pVCpu->incrementRip = FALSE;
    return;
  }

  A(s, __vmx_vmread(VMCS_GUEST_CR4, &cr4.AsUInt));

  if (drQual.DebugRegister == 4 || drQual.DebugRegister == 5) {
    if (cr4.DebuggingExtensions) {
      InjectUD(pVCpu);
      return;
    } else
      drQual.DebugRegister += 2;
  }

  A(s, __vmx_vmread(VMCS_GUEST_DR7, &dr7.AsUInt));

  if (dr7.GeneralDetect) {
    DR6 dr6 = {.AsUInt = __readdr(6),
               .BreakpointCondition = 0,
               .DebugRegisterAccessDetected = TRUE};
    __writedr(6, dr6.AsUInt);

    dr7.GeneralDetect = FALSE;

    A(s, __vmx_vmwrite(VMCS_GUEST_DR7, dr7.AsUInt));

    InjectBP();

    pVCpu->incrementRip = FALSE;
    return;
  }

  if (drQual.DirectionOfAccess == VMX_EXIT_QUALIFICATION_DIRECTION_MOV_TO_DR &&
      (drQual.DebugRegister == VMX_EXIT_QUALIFICATION_REGISTER_DR6 ||
       drQual.DebugRegister == VMX_EXIT_QUALIFICATION_REGISTER_DR7) &&
      (gpReg >> 32) != 0) {
    InjectGP();
    pVCpu->incrementRip = FALSE;
    return;
  }

  switch (drQual.DirectionOfAccess) {
  case VMX_EXIT_QUALIFICATION_DIRECTION_MOV_TO_DR:
    switch (drQual.DebugRegister) {
    case VMX_EXIT_QUALIFICATION_REGISTER_DR0:
      __writedr(VMX_EXIT_QUALIFICATION_REGISTER_DR0, gpReg);
      break;
    case VMX_EXIT_QUALIFICATION_REGISTER_DR1:
      __writedr(VMX_EXIT_QUALIFICATION_REGISTER_DR1, gpReg);
      break;
    case VMX_EXIT_QUALIFICATION_REGISTER_DR2:
      __writedr(VMX_EXIT_QUALIFICATION_REGISTER_DR2, gpReg);
      break;
    case VMX_EXIT_QUALIFICATION_REGISTER_DR3:
      __writedr(VMX_EXIT_QUALIFICATION_REGISTER_DR3, gpReg);
      break;
    case VMX_EXIT_QUALIFICATION_REGISTER_DR6:
      __writedr(VMX_EXIT_QUALIFICATION_REGISTER_DR6, gpReg);
      break;
    case VMX_EXIT_QUALIFICATION_REGISTER_DR7:
      __writedr(VMX_EXIT_QUALIFICATION_REGISTER_DR7, gpReg);
      break;
    default:
      break;
    }
    break;
  default: // Don't need to handle from dr
    break;
  }
}
#undef A

#define A(a, b) a |= b
void HdlrXsetbv(PVCPU pVCpu) {
  CPUID_EAX_01 cpuidInfo = {0};
  VMX_SEGMENT_SELECTOR cs = {0};
  UINT32 xCrIdx = (UINT32)(pVCpu->guestRegs->rcx & 0xFFFFFFFF);
  XCR0 xCr0 = {.AsUInt =
                   ((pVCpu->guestRegs->rdx << 32) | pVCpu->guestRegs->rax)};
  CR4 cr4 = {0};
  UINT8 s = 0;

  UNUSED_PARAMETER(s);

  A(s, __vmx_vmread(VMCS_GUEST_CR4, &cr4.AsUInt));
  A(s, __vmx_vmread(VMCS_GUEST_CS_BASE, &cs.Base));
  A(s, __vmx_vmread(VMCS_GUEST_CS_LIMIT, (UINT64 *)&cs.Limit));
  A(s,
    __vmx_vmread(VMCS_GUEST_CS_ACCESS_RIGHTS, (UINT64 *)&cs.Attributes.AsUInt));
  A(s, __vmx_vmread(VMCS_GUEST_CS_SELECTOR, (UINT64 *)&cs.Selector));

  if (xCrIdx != 0 || cs.Attributes.s.DescriptorPrivilegeLevel != 0 ||
      !isXCr0Valid(xCr0)) {
    InjectGP();
    return;
  }

  __cpuidex((INT32 *)&cpuidInfo, CPUID_VERSION_INFORMATION, 0);

  if (cpuidInfo.CpuidFeatureInformationEcx.XsaveXrstorInstruction == 0 ||
      cr4.OsXsave == 0) {
    InjectUD(pVCpu);
    return;
  }

  _xsetbv(xCrIdx, xCr0.AsUInt);
}
#undef A

void HdlrVmxPreemptionTimerExpired(PVCPU pVCpu) {
  HV_LOG_WARNING("VMX Preemption timer expired");
  pVCpu->incrementRip = FALSE;
}

void HdlrDirtyLogging(PVCPU pVCpu) {
  HV_LOG_INFO("Dirty logging exit");
  // TODO: PML buffer flush
  pVCpu->incrementRip = FALSE;
}

void HdlrEptViolation(PVCPU pVCpu) {
  if (!MEptHookHandleViolation(pVCpu)) {
    HV_LOG_ERROR("EPT violation (by unexpected vector)");
    __debugbreak();
  }
}