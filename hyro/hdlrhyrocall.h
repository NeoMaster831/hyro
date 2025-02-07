#pragma once

#include "global.h"

/*
 * @brief Handler for Hyro vmcalls.
 * @param pVCpu - The virtual CPU pointer
 * @return `BOOL` - TRUE if the operation was successful
 */
BOOL HdlrHyclVmcall(PVCPU pVCpu);

#define HYRO_VMCALL_TEST 0x0
#define HYRO_VMCALL_VMXOFF 0x1

void HdlrHyclCpuid(PVCPU pVCpu);

#define HYRO_CPUID_XORCALL_CONST 0x0
#define HYRO_CPUID_XORCALL_MEMORY 0x1
#define HYRO_CPUID_OUT 0x2