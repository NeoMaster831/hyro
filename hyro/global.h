#pragma once

#include <ntddk.h>
#include "ia32.h"

#define EPT_STRUCTURES //

#define MAX_VARIABLE_RANGE_MTRRS 255
#define NUM_FIXED_RANGE_MTRRS                                                  \
  (11 * 8) // ((1 + 2 + 8) * RTL_NUMBER_OF_FIELD(IA32_MTRR_FIXED_RANGE_TYPE,
           // s.Types))
#define NUM_MTRR_ENTRIES                                                       \
  (MAX_VARIABLE_RANGE_MTRRS + NUM_FIXED_RANGE_MTRRS) // 255 + 88 = 343

#define PML4E_COUNT 512
#define PDPTE_COUNT 512
#define PDE_COUNT 512
#define PTE_COUNT 512

#define ADDRMASK_EPT_PML1_INDEX(_VAR_) (((_VAR_) & 0x1FF000ULL) >> 12)
#define ADDRMASK_EPT_PML2_INDEX(_VAR_) (((_VAR_) & 0x3FE00000ULL) >> 21)
#define ADDRMASK_EPT_PML3_INDEX(_VAR_) (((_VAR_) & 0x7FC0000000ULL) >> 30)
#define ADDRMASK_EPT_PML4_INDEX(_VAR_) (((_VAR_) & 0xFF8000000000ULL) >> 39)

typedef EPT_PML4E EPT_PML4_POINTER, *PEPT_PML4_POINTER;
typedef EPT_PDPTE EPT_PML3_POINTER, *PEPT_PML3_POINTER;
typedef EPT_PDE_2MB EPT_PML2_ENTRY, *PEPT_PML2_ENTRY;
typedef EPT_PDE EPT_PML2_POINTER, *PEPT_PML2_POINTER;
typedef EPT_PTE EPT_PML1_ENTRY, *PEPT_PML1_ENTRY;

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

typedef struct _EPT_DYNAMIC_SPLIT {
  EPT_PML1_ENTRY pml1[PTE_COUNT];
  union {
    PEPT_PML2_ENTRY entry;
    PEPT_PML2_POINTER pointer;
  } u;
  LIST_ENTRY dynamicSplitList;
} EPT_DYNAMIC_SPLIT, *PEPT_DYNAMIC_SPLIT;

typedef struct _EPT_PAGE_TABLE {
  EPT_PML4_POINTER pml4[PML4E_COUNT];
  EPT_PML3_POINTER pml3[PDPTE_COUNT];
  // We do not use 4kb pages, instead we use 2mb pages, so we don't need to make pml1 entries
  EPT_PML2_ENTRY pml2[PDPTE_COUNT][PDE_COUNT];
} EPT_PAGE_TABLE, *PEPT_PAGE_TABLE;

typedef struct _EPT_STATE {
  UINT8 defaultMemoryType;
  SIZE_T mtrrCount;
  MTRR_RANGE_DESCRIPTOR mtrrMap[NUM_MTRR_ENTRIES];
} EPT_STATE, *PEPT_STATE;

/*
 * @brief Global EPT state
 */
extern EPT_STATE g_EptState;

// #enddef

#define VTX_STRUCTURES //

#define CPUID_PROCESSOR_INFO 0x1
#define VMX_SUPPORT_BIT                                                        \
  (1 << 5) // CPUID.1:ECX.VMX[bit 5] (Intel SDM Vol. 3C, Section 23.6)

#define VMXON_REGION_SIZE 0x1000
#define VMCS_REGION_SIZE 0x1000

typedef struct _VMXON_REGION_DESCRIPTOR {
  PVOID vmxonRegion;
  PHYSICAL_ADDRESS vmxonRegionPhys;
} VMXON_REGION_DESCRIPTOR, *PVMXON_REGION_DESCRIPTOR;

typedef struct _VMCS_REGION_DESCRIPTOR {
  PVOID vmcsRegion;
  PHYSICAL_ADDRESS vmcsRegionPhys;
} VMCS_REGION_DESCRIPTOR, *PVMCS_REGION_DESCRIPTOR;

typedef struct _VCPU {
  VMXON_REGION_DESCRIPTOR vmxonRegionDescriptor;
  VMCS_REGION_DESCRIPTOR vmcsRegionDescriptor;
  PEPT_PAGE_TABLE eptPageTable;
  EPT_POINTER eptPointer;
} VCPU, *PVCPU;

/*
 * @brief Global virtual CPU array
 */
extern VCPU *g_arrVCpu;

// #enddef