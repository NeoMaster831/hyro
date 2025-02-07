#pragma once

#include <ntddk.h>
#include <intrin.h>
#include <wdm.h>

#include "ia32.h"
#include "ept.h"
#include "global.h"
#include "mem.h"
#include "utils.h"
#include "tstvtx.h"

/*
 * @brief PIN-Based Execution
 */
#define PIN_BASED_VM_EXECUTION_CONTROLS_EXTERNAL_INTERRUPT 0x00000001
#define PIN_BASED_VM_EXECUTION_CONTROLS_NMI_EXITING 0x00000008
#define PIN_BASED_VM_EXECUTION_CONTROLS_VIRTUAL_NMI 0x00000020
#define PIN_BASED_VM_EXECUTION_CONTROLS_ACTIVE_VMX_TIMER 0x00000040
#define PIN_BASED_VM_EXECUTION_CONTROLS_PROCESS_POSTED_INTERRUPTS 0x00000080

/*
 * @brief CPU-Based Controls
 */
#define CPU_BASED_VIRTUAL_INTR_PENDING 0x00000004
#define CPU_BASED_USE_TSC_OFFSETTING 0x00000008
#define CPU_BASED_HLT_EXITING 0x00000080
#define CPU_BASED_INVLPG_EXITING 0x00000200
#define CPU_BASED_MWAIT_EXITING 0x00000400
#define CPU_BASED_RDPMC_EXITING 0x00000800
#define CPU_BASED_RDTSC_EXITING 0x00001000
#define CPU_BASED_CR3_LOAD_EXITING 0x00008000
#define CPU_BASED_CR3_STORE_EXITING 0x00010000
#define CPU_BASED_CR8_LOAD_EXITING 0x00080000
#define CPU_BASED_CR8_STORE_EXITING 0x00100000
#define CPU_BASED_TPR_SHADOW 0x00200000
#define CPU_BASED_VIRTUAL_NMI_PENDING 0x00400000
#define CPU_BASED_MOV_DR_EXITING 0x00800000
#define CPU_BASED_UNCOND_IO_EXITING 0x01000000
#define CPU_BASED_ACTIVATE_IO_BITMAP 0x02000000
#define CPU_BASED_MONITOR_TRAP_FLAG 0x08000000
#define CPU_BASED_ACTIVATE_MSR_BITMAP 0x10000000
#define CPU_BASED_MONITOR_EXITING 0x20000000
#define CPU_BASED_PAUSE_EXITING 0x40000000
#define CPU_BASED_ACTIVATE_SECONDARY_CONTROLS 0x80000000

/*
 * @brief Secondary CPU-Based Controls
 */
#define CPU_BASED_CTL2_ENABLE_EPT 0x2
#define CPU_BASED_CTL2_RDTSCP 0x8
#define CPU_BASED_CTL2_ENABLE_VPID 0x20
#define CPU_BASED_CTL2_UNRESTRICTED_GUEST 0x80
#define CPU_BASED_CTL2_VIRTUAL_INTERRUPT_DELIVERY 0x200
#define CPU_BASED_CTL2_ENABLE_INVPCID 0x1000
#define CPU_BASED_CTL2_ENABLE_VMFUNC 0x2000
#define CPU_BASED_CTL2_ENABLE_XSAVE_XRSTORS 0x100000

/*
 * @brief VM-exit Control Bits
 */
#define VM_EXIT_SAVE_DEBUG_CONTROLS 0x00000004
#define VM_EXIT_HOST_ADDR_SPACE_SIZE 0x00000200
#define VM_EXIT_LOAD_IA32_PERF_GLOBAL_CTRL 0x00001000
#define VM_EXIT_ACK_INTR_ON_EXIT 0x00008000
#define VM_EXIT_SAVE_IA32_PAT 0x00040000
#define VM_EXIT_LOAD_IA32_PAT 0x00080000
#define VM_EXIT_SAVE_IA32_EFER 0x00100000
#define VM_EXIT_LOAD_IA32_EFER 0x00200000
#define VM_EXIT_SAVE_VMX_PREEMPTION_TIMER 0x00400000

/*
 * @brief VM-entry Control Bits
 */
#define VM_ENTRY_LOAD_DEBUG_CONTROLS 0x00000004
#define VM_ENTRY_IA32E_MODE 0x00000200
#define VM_ENTRY_SMM 0x00000400
#define VM_ENTRY_DEACT_DUAL_MONITOR 0x00000800
#define VM_ENTRY_LOAD_IA32_PERF_GLOBAL_CTRL 0x00002000
#define VM_ENTRY_LOAD_IA32_PAT 0x00004000
#define VM_ENTRY_LOAD_IA32_EFER 0x00008000

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
 * @brief Dpc wrapper for `VmxInitVmxonIdPr`
 */
BOOLEAN DpcInitVmxonIdPr(KDPC *Dpc, PVOID DeferredContext,
                              PVOID SystemArgument1, PVOID SystemArgument2);

/*
 * @brief Initialize VMXON in Individual Processor.
 * @param pVCpu - The virtual CPU pointer
 * @return `BOOL` - TRUE if the operation was successful
 */
BOOL VmxInitVmxonIdPr();

/*
 * @brief Initialize the hypervisor
 * @return `BOOL` - TRUE if the operation was successful
 */
BOOL VmxInitHypervisor();

/*
 * @brief Allocate the VMXON region
 * @param pVmxonRegDesc - The VMXON region descriptor pointer
 */
BOOL VmxAllocVmxonRegion(PVMXON_REGION_DESCRIPTOR pVmxonRegDesc);

/*
 * @brief Allocate the VMCS region
 * @param pVmcsRegDesc - The VMCS region descriptor pointer
 */
BOOL VmxAllocVmcsRegion(PVMCS_REGION_DESCRIPTOR pVmcsRegDesc);

/*
 * @brief Allocate the VMM stack
 */
PVOID VmxAllocVmmStack();

/*
 * @brief Allocate the MSR bitmap
 */
PVOID VmxAllocMsrBitmap();

/*
 * @brief Allocate the I/O bitmaps
 * @param pVCpu - The virtual CPU pointer
 */
BOOL VmxAllocIoBitmaps(PVCPU pVCpu);

/*
 * @brief Allocate the invalid MSR bitmap.
 * It is allocated in global buffer
 */
BOOL VmxAllocInvalidMsrBitmap();

/*
 * @brief Allocate the host IDT
 */
PVOID VmxAllocHostIdt();

/*
 * @brief Allocate the host GDT
 */
PVOID VmxAllocHostGdt();

/*
 * @brief Allocate the host TSS
 */
PVOID VmxAllocHostTss();

/*
 * @brief Allocate the host interrupt stack
 */
PVOID VmxAllocHostInterruptStack();

/*
 * @brief Dpc wrapper for `AVmxLaunchGuestIdPr`
 */
BOOLEAN DpcAVmxLaunchGuestIdPr(KDPC *Dpc, PVOID DeferredContext,
                         PVOID SystemArgument1, PVOID SystemArgument2);

/*
 * @brief Save the state, and launch the guest in Individual Processor.
 * It is likely a bridge to the main function: `VmxLaunchGuestIdPr`
 */
extern void AVmxLaunchGuestBrdgIdPr();

/*
 * @brief Restore the state
 */
extern void AVmxRestoreState();

/*
 * @brief Launch the guest in Individual Processor.
 * @param guestStack - The guest stack pointer
 * @return `BOOL` - TRUE if the operation was successful
 */
BOOL VmxLaunchGuestIdPr(PVOID guestStack);

/*
 * @brief Prepare the host IDT
 * @param pVCpu - The virtual CPU pointer
 */
VOID VmxPrepareHostIdt(PVCPU pVCpu);

/*
 * @brief Prepare the host GDT
 * @param pVCpu - The virtual CPU pointer
 */
VOID VmxPrepareHostGdt(PVCPU pVCpu);

/*
 * @brief Clear VMCS State
 * @param pVCpu - The virtual CPU pointer
 * @return `BOOL` - TRUE if the operation was successful
 */
BOOL VmxClearVmcs(PVCPU pVCpu);

/*
 * @brief Clear VMCS State
 * @param pVCpu - The virtual CPU pointer
 * @return `BOOL` - TRUE if the operation was successful
 */
BOOL VmxLoadVmcs(PVCPU pVCpu);

/*
 * @brief Setup VMCS
 * @param pVCpu - The virtual CPU pointer
 * @param guestStack - The guest pointer
 * @return `BOOL` - TRUE if the operation was successful
 */
BOOL VmxSetupVmcs(PVCPU pVCpu, PVOID guestStack);

/*
 * @brief Fill the guest selector data
 * @param gdtBase - The GDT base
 * @param segReg - The segment register
 * @param selector - The selector
 * @return `BOOL` - TRUE if the operation was successful
 */
BOOL VmxFillGuestSelectorData(PVOID gdtBase, UINT32 segReg, UINT16 selector);

/*
 * @brief Adjust controls for VMCS based on processor capabilities
 * @param ctl - The control
 * @param msr - The MSR
 * @return UINT32 Returns the Cpu Based and Secondary Processor Based Controls
 *  and other controls based on hardware support
 */
UINT32 VmxAdjustControls(UINT32 ctl, UINT32 msr);

/*
 * @brief VMX Exit Handler
 * It is likely a bridge to the main function: `VmxExitHandler`
 */
extern void AVmxExitHandlerBrdg();

/*
 * @brief VM resume
 */
void VmxResume();

/*
 * @brief Return the stack pointer for VMXOFF
 * @return UINT64 - The stack pointer
 */
UINT64 VmxReturnStackPointerForVmxoff();

/*
 * @brief Return the instruction pointer for VMXOFF
 * @return UINT64 - The instruction pointer
 */
UINT64 VmxReturnInstructionPointerForVmxoff();