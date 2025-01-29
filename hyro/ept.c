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

VOID EptBuildMtrrMap(PEPT_STATE pEptState) { 
  IA32_MTRR_CAPABILITIES_REGISTER mtrrCapReg = {0};
  IA32_MTRR_DEF_TYPE_REGISTER mtrrDefTypeReg = {0};
  PMTRR_RANGE_DESCRIPTOR mtrrRangeDesc = {0};

  UINT32 currentReg;
  IA32_MTRR_PHYSBASE_REGISTER mtrrPhysBaseReg = {0};
  IA32_MTRR_PHYSMASK_REGISTER mtrrPhysMaskReg = {0};
  UINT32 numberOfBitsInMask;

  mtrrCapReg.AsUInt = __readmsr(IA32_MTRR_CAPABILITIES);
  mtrrDefTypeReg.AsUInt = __readmsr(IA32_MTRR_DEF_TYPE);

  pEptState->defaultMemoryType = (UINT8)mtrrDefTypeReg.DefaultMemoryType;

  if (mtrrCapReg.FixedRangeSupported && mtrrDefTypeReg.FixedRangeMtrrEnable) {
    const UINT32 k64Base = 0;
    const UINT32 k64Size = 0x10000;
    IA32_MTRR_FIXED_RANGE_TYPE k64Types = {__readmsr(IA32_MTRR_FIX64K_00000)};
    for (UINT32 i = 0; i < 8; i++) {
      mtrrRangeDesc = &pEptState->mtrrMap[pEptState->mtrrCount++];
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
        mtrrRangeDesc = &pEptState->mtrrMap[pEptState->mtrrCount++];
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
        mtrrRangeDesc = &pEptState->mtrrMap[pEptState->mtrrCount++];
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
      mtrrRangeDesc = &pEptState->mtrrMap[pEptState->mtrrCount++];
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

  HV_LOG_INFO("EPT MTRR map built (count of %lld)", pEptState->mtrrCount);
  return;
}