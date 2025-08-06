#pragma once

#include "global.h"

/*
 * @brief Handler for Hyro vmcalls.
 * @param pVCpu - The virtual CPU pointer
 * @return `BOOL` - TRUE if the operation was successful
 */
BOOL HdlrHyclVmcall(PVCPU pVCpu);

#define HYRO_VMCALL_TEST 0x0