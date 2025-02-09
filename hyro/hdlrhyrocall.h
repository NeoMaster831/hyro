#pragma once

#include "global.h"

// from `vtx.h`
VOID VmxDisable(PVCPU pVCpu);

/*
 * @brief Handler for Hyro vmcalls.
 * @param pVCpu - The virtual CPU pointer
 * @return `BOOL` - TRUE if the operation was successful
 */
BOOL HdlrHyclVmcall(PVCPU pVCpu);