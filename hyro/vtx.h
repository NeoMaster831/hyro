#pragma once

#include <ntddk.h>
#include <intrin.h>

#include "ia32.h"
#include "ept.h"
#include "mem.h"
#include "utils.h"

#define CPUID_PROCESSOR_INFO 0x1
#define VMX_SUPPORT_BIT (1 << 5) // CPUID.1:ECX.VMX[bit 5] (Intel SDM Vol. 3C, Section 23.6)

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
} VCPU, *PVCPU;

/*
 * @brief Check if VT-x is supported on the current processor
 * @return `BOOL` - TRUE if the operation was successful
 */
BOOL CheckVtxSupport();

/*
 * @brief Allocate VCpu state
 * @param pArrVCpu - The virtual CPU array pointer's address
 */
BOOL VmxAllocVCpuState(PVCPU *pArrVCpu);

/*
 * @brief Enable VMX operation
 */
VOID VmxEnableVmxOperation();

/*
 * @brief Fix the VMX feature in the CR4 register & CR0 register
 */
VOID VmxFixCR4AndCR0();

/*
 * @brief Initialize VMXON
 * @param pVCpu - The virtual CPU pointer
 * @return `BOOL` - TRUE if the operation was successful
 */
BOOL VmxInitVmxon(PVCPU pVCpu);

/*
 * @brief Initialize the hypervisor in Individual Processor.
 * @param pVCpu - The virtual CPU pointer
 * @return `BOOL` - TRUE if the operation was successful
 */
BOOL VmxInitHypervisorIdPr(PVCPU pVCpu);

/*
 * @brief Initialize the hypervisor
 * @param pArrVCpu - The virtual CPU array pointer, modern CPUs have multiple
 * cores
 * @return `BOOL` - TRUE if the operation was successful
 */
BOOL VmxInitHypervisor(VCPU *arrVCpu);

/*
 * @brief Allocate the VMXON region
 * @param pVmxonRegDesc - The VMXON region descriptor pointer
 */
BOOLEAN VmxAllocVmxonRegion(PVMXON_REGION_DESCRIPTOR pVmxonRegDesc);

/*
 * @brief Allocate the VMCS region
 * @param pVmcsRegDesc - The VMCS region descriptor pointer
 */
BOOLEAN VmxAllocVmcsRegion(PVMCS_REGION_DESCRIPTOR pVmcsRegDesc);

/*
 * @brief Global virtual CPU array
 */
VCPU *g_arrVCpu;