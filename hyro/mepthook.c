#include "mepthook.h"

LinkedList g_MEptHookPagesList = { 0 };

BOOL MEptHookAddHook(UINT64 physicalAddr) {
  physicalAddr = (UINT64)PAGE_ALIGN(physicalAddr);
  PHYSICAL_ADDRESS physAddrStrt = { .QuadPart = physicalAddr };
  PVOID virtualAddr = MmMapIoSpace(physAddrStrt, PAGE_SIZE, MmNonCached);

  if (virtualAddr == NULL) {
    HV_LOG_ERROR("Failed to get virtual address for 0x%llx", physicalAddr);
    return FALSE;
  }

  if (MEptHookGetHook(physicalAddr) != NULL) {
    HV_LOG_WARNING("Hook already exists for 0x%llx", physicalAddr);
    return FALSE;
  }

  PEPT_HOOK_PAGE pHook = (PEPT_HOOK_PAGE)MemAlloc_ZNP(sizeof(EPT_HOOK_PAGE));

  pHook->physAligned = physicalAddr;
  pHook->active = FALSE;
  pHook->hookCtx = MemAlloc_ZNP(PAGE_SIZE);
  if (pHook->hookCtx == NULL || ((UINT64)pHook->hookCtx & 0xFFF) != 0) {
    MemFree_P(pHook);
    HV_LOG_ERROR("Failed to allocate hook context");
    return FALSE;
  }

  RtlCopyMemory(pHook->hookCtx, (PVOID)virtualAddr, PAGE_SIZE);

  // Get PML1 entry for 1th processor, it works because all of PML1 entry for same GPA
  // is same for all processor. So we're getting PML1 entry sample for 1th processor.
  // It works because also all of GPAs are same for all processor.
  if (!EptSplitLargePage((&g_arrVCpu[0])->eptPageTable, pHook->physAligned)) {
    HV_LOG_ERROR("Failed to split large page");
    MemFree_P(pHook->hookCtx);
    MemFree_P(pHook);
    return FALSE;
  }

  PEPT_PML1_ENTRY pml1 = (PEPT_PML1_ENTRY)EptGetPml1((&g_arrVCpu[0])->eptPageTable, pHook->physAligned);

  if (!pml1) {
    HV_LOG_ERROR("Failed to get PML1 entry");
    MemFree_P(pHook->hookCtx);
    MemFree_P(pHook);
    return FALSE;
  }

  pHook->origEntry.AsUInt = pml1->AsUInt;
  pHook->hookEntry.AsUInt = pml1->AsUInt;

  pHook->hookEntry.PageFrameNumber = MmGetPhysicalAddress(pHook->hookCtx).QuadPart / PAGE_SIZE;
  pHook->hookEntry.ReadAccess = 0;
  pHook->hookEntry.WriteAccess = 0;
  pHook->hookEntry.ExecuteAccess = 1;

  pHook->origEntry.ReadAccess = 1;
  pHook->origEntry.WriteAccess = 1;
  pHook->origEntry.ExecuteAccess = 0;

  LinkedListInsertHead(&g_MEptHookPagesList, (PVOID)pHook);

  HV_LOG_INFO("Hook added for 0x%llx", physicalAddr);

  return TRUE;
}

VOID MEptHookRemoveHook(UINT64 physicalAddr) {
  physicalAddr = (UINT64)PAGE_ALIGN(physicalAddr);
  PEPT_HOOK_PAGE pHook = MEptHookGetHook(physicalAddr);
  if (pHook == NULL) {
    HV_LOG_WARNING("Hook does not exist for 0x%llx", physicalAddr);
    return;
  }

  // Deactivate hook if it's active
  MEptHookDeactivateHook(pHook);
  LinkedListRemoveSpecific(&g_MEptHookPagesList, (PVOID)pHook);
  
  MemFree_P(pHook->hookCtx);
  MemFree_P(pHook);

  HV_LOG_INFO("Hook removed for 0x%llx", physicalAddr);

  return;
}

VOID MEptHookRepairOriginal(PVCPU pVCpu, PEPT_HOOK_PAGE pEptHook) {
  PEPT_PML1_ENTRY pml1 = (PEPT_PML1_ENTRY)EptGetPml1(pVCpu->eptPageTable, pEptHook->physAligned);

  if (!pml1) {
    HV_LOG_ERROR("Failed to get PML1 entry (is it already large page?)");
    return;
  }

  pml1->PageFrameNumber = pEptHook->origEntry.PageFrameNumber;
  pml1->ReadAccess = 1;
  pml1->WriteAccess = 1;
  pml1->ExecuteAccess = 1;

  return;
}

VOID MEptHookChangePage(PVCPU pVCpu, PEPT_HOOK_PAGE pEptHook, BOOL toHook) {
  PEPT_PML1_ENTRY pml1 = (PEPT_PML1_ENTRY)EptGetPml1(pVCpu->eptPageTable, pEptHook->physAligned);

  if (!pml1) {
    HV_LOG_ERROR("Failed to get PML1 entry (is it already large page?)");
    return;
  }

  pml1->AsUInt = toHook ? pEptHook->hookEntry.AsUInt : pEptHook->origEntry.AsUInt;

  return;
}

PEPT_HOOK_PAGE MEptHookGetHook(UINT64 physAddr) {
  physAddr = (UINT64)PAGE_ALIGN(physAddr);
  FOR_EACH_LIST_ENTRY(listEntry, &g_MEptHookPagesList) {
    PEPT_HOOK_PAGE pHook = (PEPT_HOOK_PAGE)listEntry->value;
    if (pHook->physAligned == physAddr) {
      return pHook;
    }
  }
  return NULL;
}

#define A(a, b) a |= b
BOOL MEptHookHandleViolation(PVCPU pVCpu) {
  VMX_EXIT_QUALIFICATION_EPT_VIOLATION eptQual = { .AsUInt = pVCpu->exitQual };
  UINT64 physAddr = 0;
  UINT8 s = 0;

  UNUSED_PARAMETER(s);

  A(s, __vmx_vmread(VMCS_GUEST_PHYSICAL_ADDRESS, &physAddr));

  PEPT_HOOK_PAGE pHook = MEptHookGetHook(physAddr);
  if (pHook == NULL) {
    return FALSE;
  }
  if (!pHook->active) {
    return FALSE;
  }

  if ((eptQual.ReadAccess && !eptQual.EptReadable) || (eptQual.WriteAccess && !eptQual.EptWriteable)) {
    HV_LOG_INFO("EPT violation on hooked page @ 0x%llx with RW access on hooked page", physAddr);
    MEptHookChangePage(pVCpu, pHook, FALSE);
    pVCpu->incrementRip = FALSE;
    return TRUE;
  }
  else if (eptQual.ExecuteAccess && !eptQual.EptExecutable) {
    HV_LOG_INFO("EPT violation on hooked page @ 0x%llx with X access on original page", physAddr);
    MEptHookChangePage(pVCpu, pHook, TRUE);
    pVCpu->incrementRip = FALSE;
    return TRUE;
  }
  else {
    return FALSE;
  }
}
#undef A

VOID MEptHookActivateHook(PEPT_HOOK_PAGE pEptHook) {

  if (pEptHook->active) {
    HV_LOG_WARNING("Hook already activated for 0x%llx", pEptHook->physAligned);
    return;
  }

  for (ULONG i = 0; i < KeQueryActiveProcessorCount(0); i++) {
    PVCPU pVCpu = &g_arrVCpu[i];

    if (!EptSplitLargePage(pVCpu->eptPageTable, pEptHook->physAligned)) {
      HV_LOG_ERROR("Failed to split large page @ VCPU %ld", i);
      return;
    }

    MEptHookChangePage(pVCpu, pEptHook, TRUE);
    HV_LOG_INFO("Activated all pending hooks @ VCPU %ld", i);
  }

  MEptHookSynchronize();

  pEptHook->active = TRUE;

}

VOID MEptHookDeactivateHook(PEPT_HOOK_PAGE pEptHook) {

  if (!pEptHook->active) {
    HV_LOG_WARNING("Hook already deactivated for 0x%llx", pEptHook->physAligned);
    return;
  }


  for (ULONG i = 0; i < KeQueryActiveProcessorCount(0); i++) {
    PVCPU pVCpu = &g_arrVCpu[i];
    MEptHookChangePage(pVCpu, pEptHook, FALSE);
    MEptHookRepairOriginal(pVCpu, pEptHook);

    HV_LOG_INFO("Deactivated all pending hooks @ VCPU %ld", i);
  }

  MEptHookSynchronize();

  pEptHook->active = FALSE;
}

// Called in vtx.c - VmxInitHypervisor
BOOL MEptHookInitialize() {
  // We don't need to do anything here
  return TRUE;
}

// Called in vtx.c - VmxTerminate
VOID MEptHookTerminate() {

  // Unhook all hooks - but we don't need to deactivate, because we're going to
  // terminate EPT
  FOR_EACH_LIST_ENTRY(listEntry, &g_MEptHookPagesList) {
    PEPT_HOOK_PAGE pHook = (PEPT_HOOK_PAGE)listEntry->value;
    LinkedListRemoveSpecific(&g_MEptHookPagesList, (PVOID)pHook);
    HV_LOG_INFO("Hook removed for 0x%llx", pHook->physAligned);
    MemFree_P(pHook->hookCtx);
    MemFree_P(pHook);
  }

  return;
}

VOID MEptHookSynchronize() {
  if (InvEpt(InveptAllContext, NULL)) {
    HV_LOG_ERROR("Failed to invalidate EPT");
  }
  HV_LOG_INFO("EPT invalidated");
  return;
}

BOOL MEptHookModifyHook(UINT64 physAddr, UINT64 hookCtxPhys) {
  PEPT_HOOK_PAGE pHook = MEptHookGetHook(physAddr);
  PHYSICAL_ADDRESS hookCtxPhysStrt = { .QuadPart = hookCtxPhys };
  PVOID hookCtx = MmMapIoSpace(hookCtxPhysStrt, PAGE_SIZE, MmNonCached);
  BOOL wasActive = FALSE;

  if (pHook == NULL) {
    HV_LOG_ERROR("Hook does not exist for 0x%llx", physAddr);
    return FALSE;
  }

  if (hookCtx == NULL) {
    HV_LOG_ERROR("Failed to get virtual address for 0x%llx", hookCtxPhys);
    return FALSE;
  }

  // Deactivate hook if it's active
  if (pHook->active) {
    MEptHookDeactivateHook(pHook);
    wasActive = TRUE;
  }

  RtlCopyMemory(pHook->hookCtx, hookCtx, PAGE_SIZE);

  // Reactivate the hook if it was active
  if (wasActive) {
    MEptHookActivateHook(pHook);
  }

  HV_LOG_INFO("Modified hook context for 0x%llx", physAddr);

  return TRUE;
}