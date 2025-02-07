#include "vtx.h"

BOOL CheckVtxSupport() {
  int cpuInfo[4] = {0};
  IA32_FEATURE_CONTROL_REGISTER featureControlRegister = {0};

  __cpuid(cpuInfo, CPUID_PROCESSOR_INFO);

  if ((cpuInfo[2] & VMX_SUPPORT_BIT) == 0) {
    HV_LOG_ERROR("VT-x is not supported on this processor");
    return FALSE;
  }

  featureControlRegister.AsUInt = __readmsr(IA32_FEATURE_CONTROL);

  if (featureControlRegister.EnableVmxOutsideSmx == FALSE) {
    HV_LOG_ERROR("VT-x is not enabled in the BIOS");
    return FALSE;
  }

  return TRUE;
}

BOOL VmxAllocVCpuState() { 
  ULONG processorCount = KeQueryActiveProcessorCount(0);
  VCPU *arrVCpu;

  arrVCpu = (VCPU *)MemAlloc_ZNP(sizeof(VCPU) * processorCount);
  if (!arrVCpu) {
    HV_LOG_ERROR("Failed to allocate the VCpu state");
    return FALSE;
  }

  g_arrVCpu = arrVCpu;
  HV_LOG_INFO("VCpu state allocated @ 0x%llx", arrVCpu);

  return TRUE;
}

VOID VmxEnableVmxOperation() { 
  CR4 cr4 = {0};

  cr4.AsUInt = __readcr4();
  if (cr4.VmxEnable == TRUE) {
    HV_LOG_WARNING("VMX operation already enabled");
    return;
  }
  cr4.VmxEnable = TRUE;
  __writecr4(cr4.AsUInt);

  HV_LOG_INFO("VMX operation enabled");
}

VOID VmxFixCR4AndCR0() {
  UINT64 temp;
  CR4 cr4 = {0};
  CR0 cr0 = {0};
  CR4 originalCr4 = {0};
  CR0 originalCr0 = {0};

  cr0.AsUInt = __readcr0();
  originalCr0.AsUInt = cr0.AsUInt;
  temp = __readmsr(IA32_VMX_CR0_FIXED0);
  cr0.AsUInt |= (temp & 0xFFFFFFFF);
  temp = __readmsr(IA32_VMX_CR0_FIXED1);
  cr0.AsUInt &= (temp & 0xFFFFFFFF);

  cr4.AsUInt = __readcr4();
  originalCr4.AsUInt = cr4.AsUInt;
  temp = __readmsr(IA32_VMX_CR4_FIXED0);
  cr4.AsUInt |= (temp & 0xFFFFFFFF);
  temp = __readmsr(IA32_VMX_CR4_FIXED1);
  cr4.AsUInt &= (temp & 0xFFFFFFFF);

  __writecr0(cr0.AsUInt);
  __writecr4(cr4.AsUInt);

  HV_LOG_INFO("CR0 & CR4 fixed");
  HV_LOG_INFO("CR0: 0x%llX -> 0x%llX", originalCr0.AsUInt, cr0.AsUInt);
  HV_LOG_INFO("CR4: 0x%llX -> 0x%llX", originalCr4.AsUInt, cr4.AsUInt);
}

BOOL VmxInitVmxon(PVCPU pVCpu) {
  UINT8 vmxonStatus;

  VmxEnableVmxOperation();
  VmxFixCR4AndCR0();

  HV_LOG_INFO("VMX Operation enabled");

  if (!VmxAllocVmxonRegion(&pVCpu->vmxonRegionDescriptor)) {
    return FALSE;
  }

  vmxonStatus = __vmx_on((unsigned long long*)&pVCpu->vmxonRegionDescriptor.vmxonRegionPhys);

  if (vmxonStatus) {
    HV_LOG_ERROR("VMXON failed with status: %d", vmxonStatus);
    return FALSE;
  }

  if (!VmxAllocVmcsRegion(&pVCpu->vmcsRegionDescriptor)) {
    return FALSE;
  }

  HV_LOG_INFO("VMXON succeeded");
  return TRUE;
}

BOOLEAN DpcInitVmxonIdPr(KDPC* Dpc, PVOID DeferredContext,
    PVOID SystemArgument1, PVOID SystemArgument2) {
  UNREFERENCED_PARAMETER(Dpc);
  UNREFERENCED_PARAMETER(DeferredContext);
  VmxInitVmxonIdPr();
  KeSignalCallDpcSynchronize(SystemArgument2);
  KeSignalCallDpcDone(SystemArgument1);
  return TRUE;
}

BOOL VmxInitVmxonIdPr() {
  ULONG processorNum = KeGetCurrentProcessorNumberEx(NULL);
  PVCPU pVCpu = &g_arrVCpu[processorNum];

  HV_LOG_INFO("Initializing hypervisor for VCPU[%d]", processorNum);

  if (!VmxInitVmxon(pVCpu)) {
    return FALSE;
  }

  HV_LOG_INFO("Hypervisor initialized for VCPU[%d]", processorNum);

  return TRUE;
}

BOOL VmxInitHypervisor() {
  ULONG processorCount = KeQueryActiveProcessorCount(0);

  if (!CheckVtxSupport() || !CheckEptSupport()) {
    return FALSE;
  }

  // Allocation of VCPU state
  if (!VmxAllocVCpuState()) {
    return FALSE;
  }

  // EPT Initialization
  EptBuildMtrrMap();
  if (!EptInitialize()) {
    return FALSE;
  }

  KeGenericCallDpc(DpcInitVmxonIdPr, NULL);

  for (ULONG i = 0; i < processorCount; i++) {
    PVCPU pVCpu = &g_arrVCpu[i];
    
    pVCpu->vmmStack = VmxAllocVmmStack();
    if (pVCpu->vmmStack == NULL) {
      HV_LOG_ERROR("Failed to allocate the VMM stack");
      return FALSE;
    }
    HV_LOG_INFO("VMM stack allocated for VCPU[%d]: 0x%llx", i, pVCpu->vmmStack);

    pVCpu->msrBitmapVirt = VmxAllocMsrBitmap();
    if (pVCpu->msrBitmapVirt == NULL) {
      HV_LOG_ERROR("Failed to allocate the MSR bitmap");
      return FALSE;
    }
    pVCpu->msrBitmapPhys = MmGetPhysicalAddress(pVCpu->msrBitmapVirt);
    HV_LOG_INFO("MSR bitmap allocated for VCPU[%d]: 0x%llx", i,
                pVCpu->msrBitmapVirt);

    if (!VmxAllocIoBitmaps(pVCpu)) {
      return FALSE;
    }

    pVCpu->hostIdt = VmxAllocHostIdt();
    if (pVCpu->hostIdt == NULL) {
      HV_LOG_ERROR("Failed to allocate the host IDT");
      return FALSE;
    }
    HV_LOG_INFO("Host IDT allocated for VCPU[%d]: 0x%llx", i, pVCpu->hostIdt);

    pVCpu->hostGdt = VmxAllocHostGdt();
    if (pVCpu->hostGdt == NULL) {
      HV_LOG_ERROR("Failed to allocate the host GDT");
      return FALSE;
    }
    HV_LOG_INFO("Host GDT allocated for VCPU[%d]: 0x%llx", i, pVCpu->hostGdt);

    pVCpu->hostTss = VmxAllocHostTss();
    if (pVCpu->hostTss == NULL) {
      HV_LOG_ERROR("Failed to allocate the host TSS");
      return FALSE;
    }
    HV_LOG_INFO("Host TSS allocated for VCPU[%d]: 0x%llx", i, pVCpu->hostTss);

    pVCpu->hostInterruptStack = MemAlloc_ZNP(HOST_INTERRUPT_STACK_SIZE);
    if (pVCpu->hostInterruptStack == NULL) {
      HV_LOG_ERROR("Failed to allocate the host interrupt stack");
      return FALSE;
    }
    HV_LOG_INFO("Host interrupt stack allocated for VCPU[%d]: 0x%llx", i,
                pVCpu->hostInterruptStack);
  }

  if (!VmxAllocInvalidMsrBitmap()) {
    return FALSE;
  }

  KeGenericCallDpc(DpcAVmxLaunchGuestIdPr, NULL);

  TstHyroTestcall(0x1337, 0x31337, 0x6974); // Individually test VMCALLs

  HV_LOG_INFO("Hypervisor initialized");
  return TRUE;
}

BOOL VmxAllocVmxonRegion(PVMXON_REGION_DESCRIPTOR pVmxonRegDesc) {
  IA32_VMX_BASIC_REGISTER vmxBasicMsr = {0};
  SIZE_T vmxonRegionSize;
  PVOID vmxonRegion;
  PHYSICAL_ADDRESS vmxonRegionPhys;

  vmxonRegionSize = VMXON_REGION_SIZE;
  vmxonRegion = MemAlloc_ZCM(vmxonRegionSize);
  if (vmxonRegion == NULL) {
    HV_LOG_ERROR("Failed to allocate the VMXON region");
    return FALSE;
  }

  vmxonRegionPhys = MmGetPhysicalAddress(vmxonRegion);

  HV_LOG_INFO("VMXON region allocated");
  HV_LOG_INFO("VMXON region: 0x%llx", vmxonRegion);
  HV_LOG_INFO("VMXON region physical address: 0x%llx",
              vmxonRegionPhys.QuadPart);

  vmxBasicMsr.AsUInt = __readmsr(IA32_VMX_BASIC);

  HV_LOG_INFO("Revision Identifier: 0x%x", vmxBasicMsr.VmcsRevisionId);

  *(UINT64 *)vmxonRegion = vmxBasicMsr.VmcsRevisionId;

  pVmxonRegDesc->vmxonRegion = vmxonRegion;
  pVmxonRegDesc->vmxonRegionPhys = vmxonRegionPhys;

  return TRUE;
}

BOOL VmxAllocVmcsRegion(PVMCS_REGION_DESCRIPTOR pVmcsRegDesc) {
  IA32_VMX_BASIC_REGISTER vmxBasicMsr = {0};
  SIZE_T vmcsRegionSize;
  PVOID vmcsRegion;
  PHYSICAL_ADDRESS vmcsRegionPhys;

  vmcsRegionSize = VMCS_REGION_SIZE;
  vmcsRegion = MemAlloc_ZCM(vmcsRegionSize);
  if (vmcsRegion == NULL) {
    HV_LOG_ERROR("Failed to allocate the VMCS region");
    return FALSE;
  }

  vmcsRegionPhys = MmGetPhysicalAddress(vmcsRegion);

  HV_LOG_INFO("VMCS region allocated");
  HV_LOG_INFO("VMCS region: 0x%llx", vmcsRegion);
  HV_LOG_INFO("VMCS region physical address: 0x%llx", vmcsRegionPhys.QuadPart);

  vmxBasicMsr.AsUInt = __readmsr(IA32_VMX_BASIC);

  HV_LOG_INFO("Revision Identifier: 0x%x", vmxBasicMsr.VmcsRevisionId);

  *(UINT64 *)vmcsRegion = vmxBasicMsr.VmcsRevisionId;

  pVmcsRegDesc->vmcsRegion = vmcsRegion;
  pVmcsRegDesc->vmcsRegionPhys = vmcsRegionPhys;

  return TRUE;
}

PVOID VmxAllocVmmStack() {
  return MemAlloc_ZNP(VMM_STACK_SIZE);
}

PVOID VmxAllocMsrBitmap() { 
  return MemAlloc_ZNP(PAGE_SIZE); 
}

BOOL VmxAllocIoBitmaps(PVCPU pVCpu) {
  pVCpu->ioBitmapAVirt = MemAlloc_ZNP(PAGE_SIZE);
  if (pVCpu->ioBitmapAVirt == NULL) {
    HV_LOG_ERROR("Failed to allocate the I/O bitmap A");
    return FALSE;
  }
  pVCpu->ioBitmapAPhys = MmGetPhysicalAddress(pVCpu->ioBitmapAVirt);
  HV_LOG_INFO("I/O bitmap A allocated: 0x%llx", pVCpu->ioBitmapAVirt);
  pVCpu->ioBitmapBVirt = MemAlloc_ZNP(PAGE_SIZE);
  if (pVCpu->ioBitmapBVirt == NULL) {
    HV_LOG_ERROR("Failed to allocate the I/O bitmap B");
    return FALSE;
  }
  pVCpu->ioBitmapBPhys = MmGetPhysicalAddress(pVCpu->ioBitmapBVirt);
  HV_LOG_INFO("I/O bitmap B allocated: 0x%llx", pVCpu->ioBitmapBVirt);
  return TRUE;
}

BOOL VmxAllocInvalidMsrBitmap() {
  g_invalidMsrBitmap = MemAlloc_ZNP(PAGE_SIZE / 8);
  if (g_invalidMsrBitmap == NULL) {
    HV_LOG_ERROR("Failed to allocate the invalid MSR bitmap");
    return FALSE;
  }
  for (UINT32 i = 0; i < 0x1000; i++) {
    try {
      __readmsr(i);
    }
    except(EXCEPTION_EXECUTE_HANDLER) {
      SetBit(i, (unsigned long *)g_invalidMsrBitmap);
    }
  }
  return TRUE;
}

PVOID VmxAllocHostIdt() {
  UINT32 idtSize = HOST_IDT_DESCRIPTOR_COUNT * sizeof(SEGMENT_DESCRIPTOR_INTERRUPT_GATE_64);
  
  if (PAGE_SIZE > idtSize)
    idtSize = PAGE_SIZE;

  PVOID hostIdt = MemAlloc_ZNP(idtSize);
  return hostIdt;
}

PVOID VmxAllocHostGdt() {
  UINT32 gdtSize = HOST_GDT_DESCRIPTOR_COUNT * sizeof(SEGMENT_DESCRIPTOR_64);

  if (PAGE_SIZE > gdtSize)
    gdtSize = PAGE_SIZE;

  PVOID hostGdt = MemAlloc_ZNP(gdtSize);
  return hostGdt;
}

PVOID VmxAllocHostTss() {
  return MemAlloc_ZNP(PAGE_SIZE);
}

PVOID VmxAllocHostInterruptStack() {
  return MemAlloc_ZNP(HOST_INTERRUPT_STACK_SIZE);
}

BOOLEAN DpcAVmxLaunchGuestIdPr(KDPC *Dpc, PVOID DeferredContext,
                               PVOID SystemArgument1, PVOID SystemArgument2) {
  UNREFERENCED_PARAMETER(Dpc);
  UNREFERENCED_PARAMETER(DeferredContext);
  AVmxLaunchGuestBrdgIdPr();
  KeSignalCallDpcSynchronize(SystemArgument2);
  KeSignalCallDpcDone(SystemArgument1);
  return TRUE;
}

BOOL VmxLaunchGuestIdPr(PVOID guestStack) {
  UINT64 errorCode = 0;
  ULONG processorNum = KeGetCurrentProcessorNumberEx(NULL);
  PVCPU pVCpu = &g_arrVCpu[processorNum];

  VmxPrepareHostIdt(pVCpu);
  VmxPrepareHostGdt(pVCpu);

  if (!VmxClearVmcs(pVCpu)) {
    return FALSE;
  }

  if (!VmxLoadVmcs(pVCpu)) {
    return FALSE;
  }

  if (!VmxSetupVmcs(pVCpu, guestStack)) {
    HV_LOG_ERROR("Something just went wrong with the VMCS setup");
    return FALSE;
  }

  HV_LOG_INFO("Launching the guest on VCPU[%d]", processorNum);
  pVCpu->launched = TRUE;
  
  __vmx_vmlaunch();

  // Oops, something went wrong, why you are here?

  pVCpu->launched = FALSE;
  
  UINT8 s = __vmx_vmread(VMCS_VM_INSTRUCTION_ERROR, &errorCode);
  if (s) {
    HV_LOG_ERROR("Huh, fucking what ???");
    return FALSE;
  }

  HV_LOG_INFO("VM-Instruction error: 0x%llx", errorCode);

  __vmx_off();

  HV_LOG_INFO("VMXOFF succeed but idk why it caused error bruh");
  return FALSE;
}

VOID VmxPrepareHostIdt(PVCPU pVCpu) {
  SEGMENT_DESCRIPTOR_INTERRUPT_GATE_64 *vmxIdt =
      (SEGMENT_DESCRIPTOR_INTERRUPT_GATE_64 *)pVCpu->hostIdt;
  SEGMENT_DESCRIPTOR_INTERRUPT_GATE_64 *winIdt =
      (SEGMENT_DESCRIPTOR_INTERRUPT_GATE_64 *)AGetIdtBase();

  RtlZeroMemory(vmxIdt, HOST_IDT_DESCRIPTOR_COUNT *
                            sizeof(SEGMENT_DESCRIPTOR_INTERRUPT_GATE_64));

  RtlCopyMemory(vmxIdt, winIdt,
                HOST_IDT_DESCRIPTOR_COUNT *
                    sizeof(SEGMENT_DESCRIPTOR_INTERRUPT_GATE_64));

  // TODO: Add the VMX specific IDT entries

  return;
}

VOID VmxPrepareHostGdt(PVCPU pVCpu) {
  SEGMENT_DESCRIPTOR_32 *vmxGdt = (SEGMENT_DESCRIPTOR_32 *)pVCpu->hostGdt;
  SEGMENT_DESCRIPTOR_32 *winGdt = (SEGMENT_DESCRIPTOR_32 *)AGetGdtBase();
  unsigned short gdtLimit = AGetGdtLimit();
  unsigned short tr = AGetTr();
  PVOID hostStack = pVCpu->hostInterruptStack;
  TASK_STATE_SEGMENT_64* hostTss = (TASK_STATE_SEGMENT_64 *)pVCpu->hostTss;
  UINT64 endOfStack = 0;

  RtlZeroMemory(vmxGdt, gdtLimit + 1);
  RtlCopyMemory(vmxGdt, winGdt, gdtLimit + 1);

  endOfStack = (UINT64)hostStack + HOST_INTERRUPT_STACK_SIZE - 1;
  endOfStack = (endOfStack & ~(16 - 1));

  hostTss->Rsp0 = endOfStack;
  hostTss->Rsp1 = endOfStack;
  hostTss->Rsp2 = endOfStack;
  hostTss->Ist1 = endOfStack;
  hostTss->Ist2 = endOfStack;
  hostTss->Ist3 = endOfStack;
  hostTss->Ist4 = endOfStack;
  hostTss->Ist5 = endOfStack;
  hostTss->Ist6 = endOfStack;
  hostTss->Ist7 = endOfStack;

  SEGMENT_DESCRIPTOR_64 *tssDesc = (SEGMENT_DESCRIPTOR_64*)&vmxGdt[tr];

  tssDesc->BaseAddressLow = ((UINT64)hostTss >> 0) & 0xFFFF;
  tssDesc->BaseAddressMiddle = ((UINT64)hostTss >> 16) & 0xFF;
  tssDesc->BaseAddressHigh = ((UINT64)hostTss >> 24) & 0xFF;
  tssDesc->BaseAddressUpper = ((UINT64)hostTss >> 32) & 0xFFFFFFFF;

  return;
}

BOOL VmxClearVmcs(PVCPU pVCpu) {
  UINT8 status = __vmx_vmclear((unsigned long long*)&pVCpu->vmcsRegionDescriptor.vmcsRegionPhys);

  if (status) {
    HV_LOG_ERROR("VMCS clear failed with status: %d", status);
    __vmx_off();
    return FALSE;
  }

  HV_LOG_INFO("VMCS cleared with status: %d", status);
  return TRUE;
}

BOOL VmxLoadVmcs(PVCPU pVCpu) {
  UINT8 status =
      __vmx_vmptrld((unsigned long long*)&pVCpu->vmcsRegionDescriptor.vmcsRegionPhys);
  if (status) {
    HV_LOG_ERROR("VMCS load failed with status: %d", status);
    __vmx_off();
    return FALSE;
  }
  HV_LOG_INFO("VMCS loaded with status: %d", status);
  return TRUE;
}

#define A(a, b) a |= b
BOOL VmxSetupVmcs(PVCPU pVCpu, PVOID guestStack) {
  // The final stretch, let it go hypervisor,
  // __debugbreak();
  UINT8 s = 0;
  IA32_VMX_BASIC_REGISTER vmxBasicMsr = {0};
  VMX_SEGMENT_SELECTOR segSel = {0};

  vmxBasicMsr.AsUInt = __readmsr(IA32_VMX_BASIC);

  A(s, __vmx_vmwrite(VMCS_HOST_ES_SELECTOR, AGetEs() & 0xF8));
  A(s, __vmx_vmwrite(VMCS_HOST_CS_SELECTOR, AGetCs() & 0xF8));
  A(s, __vmx_vmwrite(VMCS_HOST_SS_SELECTOR, AGetSs() & 0xF8));
  A(s, __vmx_vmwrite(VMCS_HOST_DS_SELECTOR, AGetDs() & 0xF8));
  A(s, __vmx_vmwrite(VMCS_HOST_FS_SELECTOR, AGetFs() & 0xF8));
  A(s, __vmx_vmwrite(VMCS_HOST_GS_SELECTOR, AGetGs() & 0xF8));
  A(s, __vmx_vmwrite(VMCS_HOST_TR_SELECTOR, AGetTr() & 0xF8));

  A(s, __vmx_vmwrite(VMCS_GUEST_VMCS_LINK_POINTER, ~0ULL));

  A(s, __vmx_vmwrite(VMCS_GUEST_DEBUGCTL, __readmsr(IA32_DEBUGCTL) & 0xFFFFFFFF));
  A(s, __vmx_vmwrite(VMCS_GUEST_DEBUGCTL_HIGH, __readmsr(IA32_DEBUGCTL) >> 32));

  A(s, __vmx_vmwrite(VMCS_CTRL_TSC_OFFSET, 0));
  A(s, __vmx_vmwrite(VMCS_CTRL_PAGEFAULT_ERROR_CODE_MASK, 0));
  A(s, __vmx_vmwrite(VMCS_CTRL_PAGEFAULT_ERROR_CODE_MATCH, 0));

  A(s, __vmx_vmwrite(VMCS_CTRL_VMEXIT_MSR_STORE_COUNT, 0));
  A(s, __vmx_vmwrite(VMCS_CTRL_VMEXIT_MSR_LOAD_COUNT, 0));
  A(s, __vmx_vmwrite(VMCS_CTRL_VMENTRY_MSR_LOAD_COUNT, 0));
  A(s, __vmx_vmwrite(VMCS_CTRL_VMENTRY_INTERRUPTION_INFORMATION_FIELD, 0));

  unsigned long long gdtBase = AGetGdtBase();

  A(s, VmxFillGuestSelectorData((PVOID)gdtBase, 0, AGetEs()));
  A(s, VmxFillGuestSelectorData((PVOID)gdtBase, 1, AGetCs()));
  A(s, VmxFillGuestSelectorData((PVOID)gdtBase, 2, AGetSs()));
  A(s, VmxFillGuestSelectorData((PVOID)gdtBase, 3, AGetDs()));
  A(s, VmxFillGuestSelectorData((PVOID)gdtBase, 4, AGetFs()));
  A(s, VmxFillGuestSelectorData((PVOID)gdtBase, 5, AGetGs()));
  A(s, VmxFillGuestSelectorData((PVOID)gdtBase, 6, AGetLdtr()));
  A(s, VmxFillGuestSelectorData((PVOID)gdtBase, 7, AGetTr()));

  A(s, __vmx_vmwrite(VMCS_GUEST_FS_BASE, __readmsr(IA32_FS_BASE)));
  A(s, __vmx_vmwrite(VMCS_GUEST_GS_BASE, __readmsr(IA32_GS_BASE)));

  UINT32 cpuBasedExecControls = VmxAdjustControls(
      CPU_BASED_ACTIVATE_IO_BITMAP | CPU_BASED_ACTIVATE_MSR_BITMAP |
          CPU_BASED_ACTIVATE_SECONDARY_CONTROLS,
      vmxBasicMsr.VmxControls ? IA32_VMX_TRUE_PROCBASED_CTLS
                              : IA32_VMX_PROCBASED_CTLS);
  
  A(s, __vmx_vmwrite(VMCS_CTRL_PROCESSOR_BASED_VM_EXECUTION_CONTROLS,
                     cpuBasedExecControls));

  UINT32 secondaryExecControls = VmxAdjustControls(
      CPU_BASED_CTL2_RDTSCP | CPU_BASED_CTL2_ENABLE_EPT |
          CPU_BASED_CTL2_ENABLE_INVPCID | CPU_BASED_CTL2_ENABLE_XSAVE_XRSTORS |
          CPU_BASED_CTL2_ENABLE_VPID,
      IA32_VMX_PROCBASED_CTLS2);

  A(s, __vmx_vmwrite(VMCS_CTRL_SECONDARY_PROCESSOR_BASED_VM_EXECUTION_CONTROLS,
                     secondaryExecControls));

  A(s, __vmx_vmwrite(VMCS_CTRL_PIN_BASED_VM_EXECUTION_CONTROLS,
                     VmxAdjustControls(0, vmxBasicMsr.VmxControls
                                              ? IA32_VMX_TRUE_PINBASED_CTLS
                                              : IA32_VMX_PINBASED_CTLS)));

  A(s, __vmx_vmwrite(VMCS_CTRL_PRIMARY_VMEXIT_CONTROLS,
                     VmxAdjustControls(VM_EXIT_HOST_ADDR_SPACE_SIZE,
                                       vmxBasicMsr.VmxControls
                                           ? IA32_VMX_TRUE_EXIT_CTLS
                                           : IA32_VMX_EXIT_CTLS)));

  A(s, __vmx_vmwrite(
           VMCS_CTRL_VMENTRY_CONTROLS,
           VmxAdjustControls(VM_ENTRY_IA32E_MODE, vmxBasicMsr.VmxControls
                                                      ? IA32_VMX_TRUE_ENTRY_CTLS
                                                      : IA32_VMX_ENTRY_CTLS)));

  A(s, __vmx_vmwrite(VMCS_CTRL_CR0_GUEST_HOST_MASK, 0));
  A(s, __vmx_vmwrite(VMCS_CTRL_CR4_GUEST_HOST_MASK, 0));

  A(s, __vmx_vmwrite(VMCS_CTRL_CR0_READ_SHADOW, 0));
  A(s, __vmx_vmwrite(VMCS_CTRL_CR4_READ_SHADOW, 0));

  A(s, __vmx_vmwrite(VMCS_GUEST_CR0, __readcr0()));
  A(s, __vmx_vmwrite(VMCS_GUEST_CR4, __readcr4()));
  A(s, __vmx_vmwrite(VMCS_GUEST_CR3, __readcr3()));

  A(s, __vmx_vmwrite(VMCS_GUEST_DR7, 0x400));

  A(s, __vmx_vmwrite(VMCS_HOST_CR0, __readcr0()));
  A(s, __vmx_vmwrite(VMCS_HOST_CR4, __readcr4()));
  A(s, __vmx_vmwrite(VMCS_HOST_CR3, GetSsDTBase()));

  A(s, __vmx_vmwrite(VMCS_GUEST_GDTR_BASE, AGetGdtBase()));
  A(s, __vmx_vmwrite(VMCS_GUEST_IDTR_BASE, AGetIdtBase()));
  A(s, __vmx_vmwrite(VMCS_GUEST_GDTR_LIMIT, AGetGdtLimit()));
  A(s, __vmx_vmwrite(VMCS_GUEST_IDTR_LIMIT, AGetIdtLimit()));
  A(s, __vmx_vmwrite(VMCS_GUEST_RFLAGS, AGetRflags()));

  A(s, __vmx_vmwrite(VMCS_GUEST_SYSENTER_CS, __readmsr(IA32_SYSENTER_CS)));
  A(s, __vmx_vmwrite(VMCS_GUEST_SYSENTER_ESP, __readmsr(IA32_SYSENTER_ESP)));
  A(s, __vmx_vmwrite(VMCS_GUEST_SYSENTER_EIP, __readmsr(IA32_SYSENTER_EIP)));

  if (!GetSegmentDescriptor((PUINT8)pVCpu->hostGdt, AGetTr(), &segSel))
      return FALSE;
  A(s, __vmx_vmwrite(VMCS_HOST_TR_BASE, segSel.Base));
  A(s, __vmx_vmwrite(VMCS_HOST_GDTR_BASE, (UINT64)pVCpu->hostGdt));

  A(s, __vmx_vmwrite(VMCS_HOST_FS_BASE, __readmsr(IA32_FS_BASE)));
  A(s, __vmx_vmwrite(VMCS_HOST_GS_BASE, __readmsr(IA32_GS_BASE)));

  A(s, __vmx_vmwrite(VMCS_HOST_IDTR_BASE, (UINT64)pVCpu->hostIdt));

  A(s, __vmx_vmwrite(VMCS_HOST_SYSENTER_CS, __readmsr(IA32_SYSENTER_CS)));
  A(s, __vmx_vmwrite(VMCS_HOST_SYSENTER_ESP, __readmsr(IA32_SYSENTER_ESP)));
  A(s, __vmx_vmwrite(VMCS_HOST_SYSENTER_EIP, __readmsr(IA32_SYSENTER_EIP)));

  // Setup the real area

  // Msr Bitmap
  A(s, __vmx_vmwrite(VMCS_CTRL_MSR_BITMAP_ADDRESS, pVCpu->msrBitmapPhys.QuadPart));

  // I/O Bitmap
  A(s, __vmx_vmwrite(VMCS_CTRL_IO_BITMAP_A_ADDRESS,
                     pVCpu->ioBitmapAPhys.QuadPart));
  A(s, __vmx_vmwrite(VMCS_CTRL_IO_BITMAP_B_ADDRESS,
                     pVCpu->ioBitmapBPhys.QuadPart));

  // EPT
  A(s, __vmx_vmwrite(VMCS_CTRL_EPT_POINTER, pVCpu->eptPointer.AsUInt));

  //
  // VPID (Virtual Processor Identifier)

  //
  // For all processors, we will use a VPID = 1. This allows the processor to
  // separate caching
  //  of EPT structures away from the regular OS page translation tables in the
  //  TLB.
  //
  A(s, __vmx_vmwrite(0, 1));
  // VIRTUAL_PROCESSOR_ID, VPID_TAG

  // Guest Stack
  A(s, __vmx_vmwrite(VMCS_GUEST_RSP, (UINT64)guestStack));

  // Guest RIP
  A(s, __vmx_vmwrite(VMCS_GUEST_RIP, (UINT64)AVmxRestoreState));

  PVOID hostRsp = (PVOID)((UINT64)pVCpu->vmmStack + VMM_STACK_SIZE - 1);
  hostRsp = (PVOID)((UINT64)hostRsp & ~(16 - 1));

  // Host Stack
  A(s, __vmx_vmwrite(VMCS_HOST_RSP, (UINT64)hostRsp));

  // Host RIP 
  A(s, __vmx_vmwrite(VMCS_HOST_RIP, (UINT64)AVmxExitHandlerBrdg));

  return s == 0 ? TRUE : FALSE;
}
#undef A

#define A(a, b) a |= b
BOOL VmxFillGuestSelectorData(PVOID gdtBase, UINT32 segReg, UINT16 selector) {
  VMX_SEGMENT_SELECTOR segSel = {0};
  UINT8 s = 0;

  GetSegmentDescriptor(gdtBase, selector, &segSel);

  if (selector == 0) {
    segSel.Attributes.s.Unusable = 1;
  }

  segSel.Attributes.s.Reserved1 = 0;
  segSel.Attributes.s.Reserved2 = 0;

  A(s, __vmx_vmwrite(VMCS_GUEST_ES_SELECTOR + segReg * 2, selector));
  A(s, __vmx_vmwrite(VMCS_GUEST_ES_LIMIT + segReg * 2, segSel.Limit));
  A(s, __vmx_vmwrite(VMCS_GUEST_ES_ACCESS_RIGHTS + segReg * 2,
                     segSel.Attributes.AsUInt));
  A(s, __vmx_vmwrite(VMCS_GUEST_ES_BASE + segReg * 2, segSel.Base));

  return s;
}
#undef A

UINT32 VmxAdjustControls(UINT32 ctl, UINT32 msr) {
  MSR msrValue = {0};
  msrValue.Flags = __readmsr(msr);
  ctl &= msrValue.Fields.High;
  ctl |= msrValue.Fields.Low;
  return ctl;
}

#define A(a, b) a |= b
void VmxResume() {
  UINT64 errorCode = 0;
  UINT8 _ = 0;

  UNUSED_PARAMETER(_);
  A(_, __vmx_vmresume());
  A(_, __vmx_vmread(VMCS_VM_INSTRUCTION_ERROR, &errorCode));
  __vmx_off();

  HV_LOG_ERROR("VM-Instruction error: 0x%llx", errorCode);
}
#undef A

UINT64 VmxReturnStackPointerForVmxoff() {
  return g_arrVCpu[KeGetCurrentProcessorNumberEx(NULL)].vmxoffState.GuestRsp;
}

UINT64 VmxReturnInstructionPointerForVmxoff() {
  return g_arrVCpu[KeGetCurrentProcessorNumberEx(NULL)].vmxoffState.GuestRip;
}