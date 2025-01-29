#pragma once

#include <ntddk.h>
#include <intrin.h>

#include "ia32.h"
#include "mem.h"
#include "utils.h"

#define MAX_VARIABLE_RANGE_MTRRS 255
#define NUM_FIXED_RANGE_MTRRS (11 * 8) // ((1 + 2 + 8) * RTL_NUMBER_OF_FIELD(IA32_MTRR_FIXED_RANGE_TYPE, s.Types))
#define NUM_MTRR_ENTRIES (MAX_VARIABLE_RANGE_MTRRS + NUM_FIXED_RANGE_MTRRS) // 255 + 88 = 343

typedef union _IA32_MTRR_FIXED_RANGE_TYPE {
  UINT64 AsUInt;
  struct {
    UINT8 Types[8];
  } s;
} IA32_MTRR_FIXED_RANGE_TYPE;

typedef struct _MTRR_RANGE_DESCRIPTOR {
  SIZE_T physicalBaseAddr;
  SIZE_T physicalEndAddress;
  UINT8 memoryType;
  BOOLEAN fixedRange;
} MTRR_RANGE_DESCRIPTOR, *PMTRR_RANGE_DESCRIPTOR;

typedef struct _EPT_STATE {
  UINT8 defaultMemoryType;
  SIZE_T mtrrCount;
  MTRR_RANGE_DESCRIPTOR mtrrMap[NUM_MTRR_ENTRIES];
} EPT_STATE, *PEPT_STATE;

/*
 * @brief Check if EPT is supported on the current processor
 * @return `BOOL` - TRUE if the operation was successful
 */
BOOL CheckEptSupport();

/*
 * @brief Build the MTRR map
 * @param pEptState - The EPT state pointer
 */
VOID EptBuildMtrrMap(PEPT_STATE pEptState);

/*
 * @brief Global EPT state 
 */
EPT_STATE g_EptState;