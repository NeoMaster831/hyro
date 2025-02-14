#pragma once

#include "global.h"

#include "hdlrhyrocall.h"

#include "mepthook.h"

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

/*
 * @brief Handler for I/O instruction.
 * @param pVCpu - The virtual CPU pointer
 */
void HdlrIoInstruction(PVCPU pVCpu);

/*
 * @brief Handler for EPT misconfiguration.
 */
void HdlrEptMisconfiguration();

/*
 * @brief Handler for VMCALL instruction.
 * @param pVCpu - The virtual CPU pointer
 * @return `BOOL` - TRUE if the operation was successful
 */
BOOL HdlrVmcall(PVCPU pVCpu);

/*
 * @brief Handler for NMI WINDOW EXIT
 * @param pVCpu - The virtual CPU pointer
 */
void HdlrNmiWindowExit(PVCPU pVCpu);

/*
 * @brief Handler for RDPMC instruction
 * @param pVCpu - The virtual CPU pointer
 */ 
void HdlrRdpmc(PVCPU pVCpu);

/*
 * @brief Handler for MOV DR instruction.
 * @param pVCpu - The virtual CPU pointer
 */
void HdlrMovDr(PVCPU pVCpu);

/*
 * @brief Handler for XSETBV instruction.
 * @param pVCpu - The virtual CPU pointer
 */
void HdlrXsetbv(PVCPU pVCpu);

/*
 * @brief Handler for preemption timer expirement... but why??
 * @param pVCpu - The virtual CPU pointer
 */
void HdlrVmxPreemptionTimerExpired(PVCPU pVCpu);

/*
 * @brief Handler for dirty logging
 * @param pVCpu - The virtual CPU pointer
 */
void HdlrDirtyLogging(PVCPU pVCpu);

/*
 * @brief Handler for EPT violation
 * @param pVCpu - The virtual CPU pointer
 */
void HdlrEptViolation(PVCPU pVCpu);