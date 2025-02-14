#include "ept.h"

BOOL CheckEptSupport() { 
  IA32_VMX_EPT_VPID_CAP_REGISTER eptVpidCapReg = {0};
  IA32_MTRR_DEF_TYPE_REGISTER mtrrDefTypeReg = {0};

  eptVpidCapReg.AsUInt = __readmsr(IA32_VMX_EPT_VPID_CAP);
  mtrrDefTypeReg.AsUInt = __readmsr(IA32_MTRR_DEF_TYPE);

  if (!eptVpidCapReg.PageWalkLength4 || !eptVpidCapReg.MemoryTypeWriteBack ||
    !eptVpidCapReg.Pde2MbPages || !eptVpidCapReg.AdvancedVmexitEptViolationsInformation ||
    !eptVpidCapReg.ExecuteOnlyPages) {
    HV_LOG_ERROR("EPT is not supported on this processor");
    return FALSE;
  }

  if (!mtrrDefTypeReg.MtrrEnable) {
    HV_LOG_ERROR("MTRR is not enabled");
    return FALSE;
  }

  HV_LOG_INFO("EPT is supported on this processor");
  return TRUE;
}

VOID EptBuildMtrrMap() { 
  IA32_MTRR_CAPABILITIES_REGISTER mtrrCapReg = {0};
  IA32_MTRR_DEF_TYPE_REGISTER mtrrDefTypeReg = {0};
  PMTRR_RANGE_DESCRIPTOR mtrrRangeDesc = {0};

  UINT32 currentReg;
  IA32_MTRR_PHYSBASE_REGISTER mtrrPhysBaseReg = {0};
  IA32_MTRR_PHYSMASK_REGISTER mtrrPhysMaskReg = {0};
  UINT32 numberOfBitsInMask;

  mtrrCapReg.AsUInt = __readmsr(IA32_MTRR_CAPABILITIES);
  mtrrDefTypeReg.AsUInt = __readmsr(IA32_MTRR_DEF_TYPE);

  g_EptState.defaultMemoryType = (UINT8)mtrrDefTypeReg.DefaultMemoryType;

  if (mtrrCapReg.FixedRangeSupported && mtrrDefTypeReg.FixedRangeMtrrEnable) {
    const UINT32 k64Base = 0;
    const UINT32 k64Size = 0x10000;
    IA32_MTRR_FIXED_RANGE_TYPE k64Types = {__readmsr(IA32_MTRR_FIX64K_00000)};
    for (UINT32 i = 0; i < 8; i++) {
      mtrrRangeDesc = &g_EptState.mtrrMap[g_EptState.mtrrCount++];
      mtrrRangeDesc->physicalBaseAddr = k64Base + i * k64Size;
      mtrrRangeDesc->physicalEndAddress = k64Base + (i + 1) * k64Size - 1;
      mtrrRangeDesc->memoryType = k64Types.s.Types[i];
      mtrrRangeDesc->fixedRange = TRUE;
    }

    const UINT32 k16Base = 0x80000;
    const UINT32 k16Size = 0x4000;
    for (UINT32 i = 0; i < 2; i++) {
      IA32_MTRR_FIXED_RANGE_TYPE k16Types = {
          __readmsr(IA32_MTRR_FIX16K_80000 + i)};
      for (UINT32 j = 0; j < 8; j++) {
        mtrrRangeDesc = &g_EptState.mtrrMap[g_EptState.mtrrCount++];
        mtrrRangeDesc->physicalBaseAddr = k16Base + i * k16Size * 8 + j * k16Size;
        mtrrRangeDesc->physicalEndAddress =
            k16Base + i * k16Size * 8 + (j + 1) * k16Size - 1;
        mtrrRangeDesc->memoryType = k16Types.s.Types[j];
        mtrrRangeDesc->fixedRange = TRUE;
      }
    }

    const UINT32 k4Base = 0xC0000;
    const UINT32 k4Size = 0x1000;
    for (UINT32 i = 0; i < 8; i++) {
      IA32_MTRR_FIXED_RANGE_TYPE k4Types = {
          __readmsr(IA32_MTRR_FIX4K_C0000 + i)};
      for (UINT32 j = 0; j < 8; j++) {
        mtrrRangeDesc = &g_EptState.mtrrMap[g_EptState.mtrrCount++];
        mtrrRangeDesc->physicalBaseAddr = k4Base + i * k4Size * 8 + j * k4Size;
        mtrrRangeDesc->physicalEndAddress =
            k4Base + i * k4Size * 8 + (j + 1) * k4Size - 1;
        mtrrRangeDesc->memoryType = k4Types.s.Types[j];
        mtrrRangeDesc->fixedRange = TRUE;
      }
    }
  }

  for (currentReg = 0; currentReg < mtrrCapReg.VariableRangeCount;
       currentReg++) {
    mtrrPhysBaseReg.AsUInt = __readmsr(IA32_MTRR_PHYSBASE0 + 2 * currentReg);
    mtrrPhysMaskReg.AsUInt = __readmsr(IA32_MTRR_PHYSMASK0 + 2 * currentReg);

    if (mtrrPhysMaskReg.Valid) {
      mtrrRangeDesc = &g_EptState.mtrrMap[g_EptState.mtrrCount++];
      mtrrRangeDesc->physicalBaseAddr = mtrrPhysBaseReg.PageFrameNumber * PAGE_SIZE;
      _BitScanForward64((ULONG *)&numberOfBitsInMask,
                        mtrrPhysMaskReg.PageFrameNumber * PAGE_SIZE);
      mtrrRangeDesc->physicalEndAddress = mtrrRangeDesc->physicalBaseAddr +
                                          ((1ULL << numberOfBitsInMask) - 1ULL);
      mtrrRangeDesc->memoryType = (UINT8)mtrrPhysBaseReg.Type;
      mtrrRangeDesc->fixedRange = FALSE;

      HV_LOG_INFO("MTRR[%d]: 0x%llx - 0x%llx, Type: 0x%x", currentReg,
                  mtrrRangeDesc->physicalBaseAddr,
                  mtrrRangeDesc->physicalEndAddress, mtrrRangeDesc->memoryType);

    }
  }

  HV_LOG_INFO("EPT MTRR map built (count of %lld)", g_EptState.mtrrCount);
  return;
}

PEPT_PML2_ENTRY EptGetPml2(PEPT_PAGE_TABLE pEptPageTable, SIZE_T physicalAddr) {
  SIZE_T i, j, k;
  PEPT_PML2_ENTRY pml2Entry;

  i = ADDRMASK_EPT_PML2_INDEX(physicalAddr);
  j = ADDRMASK_EPT_PML3_INDEX(physicalAddr);
  k = ADDRMASK_EPT_PML4_INDEX(physicalAddr);

  if (k > 0)
    return NULL;

  pml2Entry = &pEptPageTable->pml2[j][i];
  return pml2Entry;
}

PEPT_PML1_ENTRY EptGetPml1(PEPT_PAGE_TABLE pEptPageTable, SIZE_T physicalAddr) {
  SIZE_T i, j, k;
  PEPT_PML2_ENTRY pml2;
  PEPT_PML1_ENTRY pml1;
  PEPT_PML2_POINTER pml2Pointer;

  i = ADDRMASK_EPT_PML2_INDEX(physicalAddr);
  j = ADDRMASK_EPT_PML3_INDEX(physicalAddr);
  k = ADDRMASK_EPT_PML4_INDEX(physicalAddr);

  if (k > 0)
    return NULL;

  pml2 = &pEptPageTable->pml2[j][i];

  if (pml2->LargePage)
    return NULL;

  pml2Pointer = (PEPT_PML2_POINTER)pml2;
  PHYSICAL_ADDRESS pml1Phys = { .QuadPart = pml2Pointer->PageFrameNumber * PAGE_SIZE };
  pml1 = (PEPT_PML1_ENTRY)MmGetVirtualForPhysical(pml1Phys);

  if (!pml1)
    return NULL;

  pml1 = &pml1[ADDRMASK_EPT_PML1_INDEX(physicalAddr)];

  return pml1;
}

BOOL EptIsValidForLargePage(SIZE_T pfn) {
  SIZE_T startAddr = pfn * 512 * PAGE_SIZE;
  SIZE_T endAddr = startAddr + 512 * PAGE_SIZE - 1;
  PMTRR_RANGE_DESCRIPTOR mtrrRangeDesc;

  for (SIZE_T i = 0; i < g_EptState.mtrrCount; i++) {
    mtrrRangeDesc = &g_EptState.mtrrMap[i];
    if ((startAddr <= mtrrRangeDesc->physicalEndAddress &&
         endAddr > mtrrRangeDesc->physicalEndAddress) ||
        (startAddr < mtrrRangeDesc->physicalBaseAddr &&
         endAddr >= mtrrRangeDesc->physicalBaseAddr)) {
      return FALSE;
    }
  }
  return TRUE;
}

UINT8 EptGetMemoryType(SIZE_T pfn, BOOL isLargePage) {
  UINT8 targetMemoryType = 255;
  SIZE_T address = isLargePage ? pfn * 512 * PAGE_SIZE : pfn * PAGE_SIZE;
  PMTRR_RANGE_DESCRIPTOR mtrrRangeDesc;

  for (SIZE_T curMtrrRange = 0; curMtrrRange < g_EptState.mtrrCount;
       curMtrrRange++) {
    mtrrRangeDesc = &g_EptState.mtrrMap[curMtrrRange];
    if (address >= mtrrRangeDesc->physicalBaseAddr &&
        address < mtrrRangeDesc->physicalEndAddress) {
      targetMemoryType = mtrrRangeDesc->memoryType;
    }
  }
  if (targetMemoryType == 255) {
    targetMemoryType = g_EptState.defaultMemoryType;
  }

  return targetMemoryType;
}

BOOL EptSplitLargePage(PEPT_PAGE_TABLE pEptPageTable,
                       SIZE_T physicalAddr) {
  PEPT_PML2_ENTRY pml2Entry;
  EPT_PML2_POINTER pml2Template;
  PEPT_DYNAMIC_SPLIT dynamicSplit;
  EPT_PML1_ENTRY pml1Template;

  pml2Entry = EptGetPml2(pEptPageTable, physicalAddr);
  if (!pml2Entry) {
    HV_LOG_ERROR("Failed to get the PML2 entry");
    return FALSE;
  }

  if (!pml2Entry->LargePage) {
    return TRUE;
  }

  dynamicSplit = (PEPT_DYNAMIC_SPLIT)MemAlloc_ZNP(sizeof(EPT_DYNAMIC_SPLIT));
  dynamicSplit->u.entry = pml2Entry;

  pml1Template.AsUInt = 0;
  pml1Template.ReadAccess = 1;
  pml1Template.WriteAccess = 1;
  pml1Template.ExecuteAccess = 1;

  pml1Template.MemoryType = pml2Entry->MemoryType;
  pml1Template.IgnorePat = pml2Entry->IgnorePat;
  pml1Template.SuppressVe = pml2Entry->SuppressVe;

  __stosq((SIZE_T *)&dynamicSplit->pml1[0], pml1Template.AsUInt, PTE_COUNT);

  for (SIZE_T idx = 0; idx < PTE_COUNT; idx++) {
    dynamicSplit->pml1[idx].PageFrameNumber =
        pml2Entry->PageFrameNumber * 512 + idx;
    dynamicSplit->pml1[idx].MemoryType =
        EptGetMemoryType(dynamicSplit->pml1[idx].PageFrameNumber, FALSE);
  }

  pml2Template.AsUInt = 0;
  pml2Template.ReadAccess = 1;
  pml2Template.WriteAccess = 1;
  pml2Template.ExecuteAccess = 1;
  pml2Template.PageFrameNumber =
      MmGetPhysicalAddress(&dynamicSplit->pml1[0]).QuadPart / PAGE_SIZE;

  RtlCopyMemory(pml2Entry, &pml2Template, sizeof(EPT_PML2_POINTER));

  return TRUE;
}

BOOL EptBuildPml2(PEPT_PAGE_TABLE pEptPageTable, PEPT_PML2_ENTRY newEntry,
    SIZE_T pfn) {
  
  newEntry->PageFrameNumber = pfn;

  if (EptIsValidForLargePage(pfn)) {
    newEntry->MemoryType = EptGetMemoryType(pfn, TRUE);
    return TRUE;
  } else {
    return EptSplitLargePage(pEptPageTable, pfn * 512 * PAGE_SIZE);
  }
}

PEPT_PAGE_TABLE EptBuildPageTable() {
  PEPT_PAGE_TABLE pEptPageTable;
  EPT_PML3_POINTER rwxTemplate;
  EPT_PML2_ENTRY pml2EntryTemplate;
  SIZE_T groupIdx, idx;

  pEptPageTable = (PEPT_PAGE_TABLE)MemAlloc_ZCM(sizeof(EPT_PAGE_TABLE));
  if (!pEptPageTable) {
    HV_LOG_ERROR("Failed to allocate the EPT page table");
    return NULL;
  }

  pEptPageTable->pml4[0].PageFrameNumber =
      MmGetPhysicalAddress(&pEptPageTable->pml3[0]).QuadPart / PAGE_SIZE;
  pEptPageTable->pml4[0].ReadAccess = 1;
  pEptPageTable->pml4[0].WriteAccess = 1;
  pEptPageTable->pml4[0].ExecuteAccess = 1;

  rwxTemplate.AsUInt = 0;
  rwxTemplate.ReadAccess = 1;
  rwxTemplate.WriteAccess = 1;
  rwxTemplate.ExecuteAccess = 1;

  __stosq((SIZE_T *)&pEptPageTable->pml3[0], rwxTemplate.AsUInt, PDPTE_COUNT);

  for (idx = 0; idx < PDPTE_COUNT; idx++) {
    pEptPageTable->pml3[idx].PageFrameNumber =
        MmGetPhysicalAddress(&pEptPageTable->pml2[idx][0]).QuadPart / PAGE_SIZE;
  }

  pml2EntryTemplate.AsUInt = 0;
  pml2EntryTemplate.ReadAccess = 1;
  pml2EntryTemplate.WriteAccess = 1;
  pml2EntryTemplate.ExecuteAccess = 1;

  pml2EntryTemplate.LargePage = 1;

  __stosq((SIZE_T *)&pEptPageTable->pml2[0], pml2EntryTemplate.AsUInt,
          PDE_COUNT * PDPTE_COUNT);

  for (groupIdx = 0; groupIdx < PDPTE_COUNT; groupIdx++) {
    for (idx = 0; idx < PDE_COUNT; idx++) {
      if (!EptBuildPml2(pEptPageTable, &pEptPageTable->pml2[groupIdx][idx],
                        groupIdx * PDE_COUNT + idx))
        return NULL;
    }
  }

  return pEptPageTable;
}

BOOL EptInitialize() { 
  ULONG processorCount = KeQueryActiveProcessorCount(0);
  PEPT_PAGE_TABLE pCurEptPageTable = NULL;
  EPT_POINTER curEptp = {0};

  for (ULONG i = 0; i < processorCount; i++) {
    pCurEptPageTable = EptBuildPageTable();
    if (!pCurEptPageTable) {
      HV_LOG_ERROR("Failed to build the EPT page table clearing pool...");
      for (ULONG j = 0; j < processorCount; j++) {
        if (g_arrVCpu[j].eptPageTable) {
          MemFree_C(g_arrVCpu[j].eptPageTable);
          g_arrVCpu[j].eptPageTable = NULL;
        }
      }
      return FALSE;
    }
    g_arrVCpu[i].eptPageTable = pCurEptPageTable;

    curEptp.MemoryType = g_EptState.defaultMemoryType;
    curEptp.EnableAccessAndDirtyFlags = 1;
    curEptp.PageWalkLength = 3; // As we use 2mb pages
    curEptp.PageFrameNumber =
        MmGetPhysicalAddress(&pCurEptPageTable->pml4).QuadPart / PAGE_SIZE;

    g_arrVCpu[i].eptPointer = curEptp;

    HV_LOG_INFO("EPT initialized for VCPU[%d]", i);
    HV_LOG_INFO("PageTable Struct: 0x%llx", pCurEptPageTable);
    HV_LOG_INFO("EPT Pointer: 0x%llx", curEptp.AsUInt);
  }

  HV_LOG_INFO("EPT initialized");
  return TRUE;
}