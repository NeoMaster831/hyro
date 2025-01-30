#pragma once

#include <ntddk.h>
#include <intrin.h>

#include "ia32.h"
#include "ept.h"
#include "global.h"
#include "mem.h"
#include "utils.h"

/*
 * @brief Check if VT-x is supported on the current processor
 * @return `BOOL` - TRUE if the operation was successful
 */
BOOL CheckVtxSupport();

/*
 * @brief Allocate VCpu state
 */
BOOL VmxAllocVCpuState();

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
 * @return `BOOL` - TRUE if the operation was successful
 */
BOOL VmxInitHypervisor();

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