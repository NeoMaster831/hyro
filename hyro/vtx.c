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

BOOL VmxInitHypervisorIdPr(PVCPU pVCpu) {
  if (!VmxInitVmxon(pVCpu)) {
    return FALSE;
  }
  return TRUE;
}

BOOL VmxInitHypervisor() {
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

  // TODO: Change this to all cpu using dpc routines
  if (!VmxInitHypervisorIdPr(&g_arrVCpu[0])) {
    return FALSE;
  }
  HV_LOG_INFO("Hypervisor initialized");
  return TRUE;
}

BOOLEAN VmxAllocVmxonRegion(PVMXON_REGION_DESCRIPTOR pVmxonRegDesc) {
  IA32_VMX_BASIC_REGISTER vmxBasicMsr = {0};
  SIZE_T vmxonRegionSize;
  PVOID vmxonRegion;
  PHYSICAL_ADDRESS vmxonRegionPhys;

  // We cannot allocate memory when the IRQL is greater than DISPATCH_LEVEL
  if (KeGetCurrentIrql() > DISPATCH_LEVEL) {
    KeRaiseIrqlToDpcLevel();
  }

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

BOOLEAN VmxAllocVmcsRegion(PVMCS_REGION_DESCRIPTOR pVmcsRegDesc) {
  IA32_VMX_BASIC_REGISTER vmxBasicMsr = {0};
  SIZE_T vmcsRegionSize;
  PVOID vmcsRegion;
  PHYSICAL_ADDRESS vmcsRegionPhys;

  // We cannot allocate memory when the IRQL is greater than DISPATCH_LEVEL
  if (KeGetCurrentIrql() > DISPATCH_LEVEL) {
    KeRaiseIrqlToDpcLevel();
  }

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