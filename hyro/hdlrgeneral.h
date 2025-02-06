#pragma once

#include "global.h"
#include <intrin.h>

/*
 * @brief Handler for triple fault.
 * We just stop for triple fault, because it is uncommon and severe.
 */
void HdlrTripleFault();

/*
 * @brief Handler for unconditional exit.
 * We inject #UD exception.
 * @param pVCpu - The virtual CPU pointer
 */
void HdlrUnconditionalExit(PVCPU pVCpu);

/*
 * @brief Handler for MOV CR instruction.
 * @param pVCpu - The virtual CPU pointer
 */ 
void HdlrMovCr(PVCPU pVCpu);

/*
 * @brief Handler for WRMSR instruction.
 * @param pGuestRegs - The guest registers
 */
void HdlrWrmsr(PGUEST_REGS pGuestRegs);

/*
 * @brief Handler for CPUID instruction.
 * @param pVCpu - The virtual CPU pointer
 */
void HdlrCpuid(PVCPU pVCpu);