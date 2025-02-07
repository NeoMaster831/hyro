#pragma once

#include <ntddk.h>
#include "ia32.h"
#include "utils.h"
#include <intrin.h>

#define TYPES

#ifndef BOOL
typedef ULONG BOOL;
#endif

// #enddef

#define GUEST_REGISTER_STRUCTURE

typedef struct GUEST_REGS {
  UINT64 rax; // 0x00
  UINT64 rcx; // 0x08
  UINT64 rdx; // 0x10
  UINT64 rbx; // 0x18
  UINT64 rsp; // 0x20
  UINT64 rbp; // 0x28
  UINT64 rsi; // 0x30
  UINT64 rdi; // 0x38
  UINT64 r8;  // 0x40
  UINT64 r9;  // 0x48
  UINT64 r10; // 0x50
  UINT64 r11; // 0x58
  UINT64 r12; // 0x60
  UINT64 r13; // 0x68
  UINT64 r14; // 0x70
  UINT64 r15; // 0x78
} GUEST_REGS, *PGUEST_REGS;

// #enddef

#define NT_IMPORTS

typedef struct _NT_KPROCESS {
  DISPATCHER_HEADER Header;
  LIST_ENTRY ProfileListHead;
  ULONG_PTR DirectoryTableBase;
  UCHAR Data[1];
} NT_KPROCESS, *PNT_KPROCESS;

// #enddef

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
#define VMM_STACK_SIZE 0x8000

#define HOST_IDT_DESCRIPTOR_COUNT 256
#define HOST_GDT_DESCRIPTOR_COUNT 10
#define HOST_INTERRUPT_STACK_SIZE 0x4000

#define VMCS_GUEST_DEBUGCTL_HIGH 0x00002803

typedef struct _VMXON_REGION_DESCRIPTOR {
  PVOID vmxonRegion;
  PHYSICAL_ADDRESS vmxonRegionPhys;
} VMXON_REGION_DESCRIPTOR, *PVMXON_REGION_DESCRIPTOR;

typedef struct _VMCS_REGION_DESCRIPTOR {
  PVOID vmcsRegion;
  PHYSICAL_ADDRESS vmcsRegionPhys;
} VMCS_REGION_DESCRIPTOR, *PVMCS_REGION_DESCRIPTOR;

typedef struct _VMX_VMXOFF_STATE {
  BOOLEAN IsVmxoffExecuted; // Shows whether the VMXOFF executed or not
  UINT64 GuestRip;          // Rip address of guest to return
  UINT64 GuestRsp;          // Rsp address of guest to return
} VMX_VMXOFF_STATE, *PVMX_VMXOFF_STATE;

typedef struct _VCPU {
  VMXON_REGION_DESCRIPTOR vmxonRegionDescriptor;
  VMCS_REGION_DESCRIPTOR vmcsRegionDescriptor;

  PEPT_PAGE_TABLE eptPageTable;

  EPT_POINTER eptPointer;
  PVOID vmmStack;

  PVOID msrBitmapVirt;
  PHYSICAL_ADDRESS msrBitmapPhys;

  PVOID ioBitmapAVirt;
  PHYSICAL_ADDRESS ioBitmapAPhys;
  PVOID ioBitmapBVirt;
  PHYSICAL_ADDRESS ioBitmapBPhys;

  PVOID hostIdt;
  PVOID hostGdt;
  PVOID hostTss;
  PVOID hostInterruptStack;

  // -- Live state
  BOOL launched;
  PGUEST_REGS guestRegs;
  BOOL isOnVmxRootMode;
  BOOL incrementRip;
  UINT64 lastVmexitRip;
  UINT64 exitQual;
  VMX_VMXOFF_STATE vmxoffState;
  // --

} VCPU, *PVCPU;

/*
 * @brief Global virtual CPU array
 */
extern VCPU *g_arrVCpu;

/*
 * @brief Global invalid MSR bitmap
 */
extern UINT64 *g_invalidMsrBitmap;

// #enddef

#define DPC_IMPORTS //

NTKERNELAPI
_IRQL_requires_max_(APC_LEVEL)
    _IRQL_requires_min_(PASSIVE_LEVEL) _IRQL_requires_same_ VOID
    KeGenericCallDpc(_In_ PKDEFERRED_ROUTINE Routine, _In_opt_ PVOID Context);

NTKERNELAPI
_IRQL_requires_(DISPATCH_LEVEL) _IRQL_requires_same_ VOID
    KeSignalCallDpcDone(_In_ PVOID SystemArgument1);

NTKERNELAPI
_IRQL_requires_(DISPATCH_LEVEL) _IRQL_requires_same_ LOGICAL
    KeSignalCallDpcSynchronize(_In_ PVOID SystemArgument2);

// #enddef

#define BITWISE_OPERATIONS //

#define BITS_PER_LONG (sizeof(unsigned long) * 8)
#define BITMAP_ENTRY(_nr, _bmap) ((_bmap))[(_nr) / BITS_PER_LONG]
#define BITMAP_SHIFT(_nr) ((_nr) % BITS_PER_LONG)

void SetBit(int BitNumber, unsigned long * Addr);

// #enddef

#define ASM_GET_SET_REGISTERS

extern unsigned short AGetCs();
extern unsigned short AGetDs();
extern unsigned short AGetEs();
extern unsigned short AGetFs();
extern unsigned short AGetGs();
extern unsigned short AGetSs();
extern unsigned short AGetLdtr();
extern unsigned short AGetTr();
extern unsigned long long AGetGdtBase();
extern unsigned long long AGetIdtBase();
extern unsigned short AGetGdtLimit();
extern unsigned short AGetIdtLimit();
extern void ASetDs(unsigned short ds);
extern void ASetEs(unsigned short es);
extern void ASetFs(unsigned short fs);
extern void ASetSs(unsigned short ss);
extern UINT32 AGetAccessRights(unsigned short sel);
extern unsigned short AGetRflags();

// #enddef

#define GET_SET_REGISTERS

typedef union {
  struct {
    UINT32 Type : 4;
    UINT32 DescriptorType : 1;
    UINT32 DescriptorPrivilegeLevel : 2;
    UINT32 Present : 1;
    UINT32 Reserved1 : 4;
    UINT32 AvailableBit : 1;
    UINT32 LongMode : 1;
    UINT32 DefaultBig : 1;
    UINT32 Granularity : 1;
    UINT32 Unusable : 1;
    UINT32 Reserved2 : 15;
  } s;

  UINT32 AsUInt;
} VMX_SEGMENT_ACCESS_RIGHTS_TYPE0;

typedef struct _VMX_SEGMENT_SELECTOR {
  UINT16 Selector;
  VMX_SEGMENT_ACCESS_RIGHTS_TYPE0 Attributes;
  UINT32 Limit;
  UINT64 Base;
} VMX_SEGMENT_SELECTOR, *PVMX_SEGMENT_SELECTOR;

/*
 * @brief Get the segment descriptor
 * @param gdtBase - The GDT base
 * @param selector - The selector
 * @param segSel - The segment selector, where the result will be stored
 * @return `BOOL` - TRUE if the operation was successful
 */
BOOL GetSegmentDescriptor(PUINT8 gdtBase, UINT16 selector,
                     PVMX_SEGMENT_SELECTOR segSel);

/*
 * @brief Get System Directory Table Base
 */
UINT64 GetSsDTBase();

/*
 * @brief check if xcr0 is valid
 */
BOOL isXCr0Valid(XCR0 XCr0);

// #enddef

#define MSR_STRUCTURE

typedef union _MSR {
  struct {
    ULONG Low;
    ULONG High;
  } Fields;

  UINT64 Flags;

} MSR, *PMSR;

// #enddef

#define INJECT_EVENTS

typedef union _INTERRUPT_INFO {
  struct {
    UINT32 Vector : 8;
    /* 0=Ext Int, 1=Rsvd, 2=NMI, 3=Exception, 4=Soft INT,
     * 5=Priv Soft Trap, 6=Unpriv Soft Trap, 7=Other */
    UINT32 InterruptType : 3;
    UINT32 DeliverCode : 1; /* 0=Do not deliver, 1=Deliver */
    UINT32 Reserved : 19;
    UINT32 Valid : 1; /* 0=Not valid, 1=Valid. Must be checked first */
  } Fields;
  UINT32 Flags;
} INTERRUPT_INFO, *PINTERRUPT_INFO;

typedef enum _INTERRUPT_TYPE {
  INTERRUPT_TYPE_EXTERNAL_INTERRUPT = 0,
  INTERRUPT_TYPE_RESERVED = 1,
  INTERRUPT_TYPE_NMI = 2,
  INTERRUPT_TYPE_HARDWARE_EXCEPTION = 3,
  INTERRUPT_TYPE_SOFTWARE_INTERRUPT = 4,
  INTERRUPT_TYPE_PRIVILEGED_SOFTWARE_INTERRUPT = 5,
  INTERRUPT_TYPE_SOFTWARE_EXCEPTION = 6,
  INTERRUPT_TYPE_OTHER_EVENT = 7
} INTERRUPT_TYPE;

typedef enum _EXCEPTION_VECTORS {
  EXCEPTION_VECTOR_DIVIDE_ERROR,
  EXCEPTION_VECTOR_DEBUG_BREAKPOINT,
  EXCEPTION_VECTOR_NMI,
  EXCEPTION_VECTOR_BREAKPOINT,
  EXCEPTION_VECTOR_OVERFLOW,
  EXCEPTION_VECTOR_BOUND_RANGE_EXCEEDED,
  EXCEPTION_VECTOR_UNDEFINED_OPCODE,
  EXCEPTION_VECTOR_NO_MATH_COPROCESSOR,
  EXCEPTION_VECTOR_DOUBLE_FAULT,
  EXCEPTION_VECTOR_RESERVED0,
  EXCEPTION_VECTOR_INVALID_TASK_SEGMENT_SELECTOR,
  EXCEPTION_VECTOR_SEGMENT_NOT_PRESENT,
  EXCEPTION_VECTOR_STACK_SEGMENT_FAULT,
  EXCEPTION_VECTOR_GENERAL_PROTECTION_FAULT,
  EXCEPTION_VECTOR_PAGE_FAULT,
  EXCEPTION_VECTOR_RESERVED1,
  EXCEPTION_VECTOR_MATH_FAULT,
  EXCEPTION_VECTOR_ALIGNMENT_CHECK,
  EXCEPTION_VECTOR_MACHINE_CHECK,
  EXCEPTION_VECTOR_SIMD_FLOATING_POINT_NUMERIC_ERROR,
  EXCEPTION_VECTOR_VIRTUAL_EXCEPTION,
  EXCEPTION_VECTOR_RESERVED2,
  EXCEPTION_VECTOR_RESERVED3,
  EXCEPTION_VECTOR_RESERVED4,
  EXCEPTION_VECTOR_RESERVED5,
  EXCEPTION_VECTOR_RESERVED6,
  EXCEPTION_VECTOR_RESERVED7,
  EXCEPTION_VECTOR_RESERVED8,
  EXCEPTION_VECTOR_RESERVED9,
  EXCEPTION_VECTOR_RESERVED10,
  EXCEPTION_VECTOR_RESERVED11,
  EXCEPTION_VECTOR_RESERVED12,

  //
  // NT (Windows) specific exception vectors.
  //
  APC_INTERRUPT = 31,
  DPC_INTERRUPT = 47,
  CLOCK_INTERRUPT = 209,
  IPI_INTERRUPT = 225,
  PMI_INTERRUPT = 254,

} EXCEPTION_VECTORS;

/*
 * @brief Injects interruption to a guest
 *
 * @param InterruptionType Type of interrupt
 * @param Vector Vector Number of Interrupt (IDT Index)
 * @param DeliverErrorCode Deliver Error Code or Not
 * @param ErrorCode Error Code (If DeliverErrorCode is true)
 */
void InjectInterruption(INTERRUPT_TYPE InterruptionType,
                        EXCEPTION_VECTORS Vector, BOOLEAN DeliverErrorCode,
                        UINT32 ErrorCode);

/*
 * @brief Inject a #UD exception
 * @param pVCpu - The virtual CPU pointer
 */
void InjectUD(PVCPU pVCpu);

/*
 * @brief Inject a #GP exception
 */
void InjectGP();

/*
 * @brief Inject a breakpoint exception
 */
void InjectBP();

// #enddef

#define INVVPID_STUFF

extern unsigned char inline AInvVpid(unsigned long Type, void *Descriptors);

/*
 * @brief perform InvVpid
 * @param Type - The type of the operation
 * @param Descriptor - The descriptor
 */
void InvVpid(INVVPID_TYPE Type, INVVPID_DESCRIPTOR *Descriptor);

// #enddef

#define ADDRESS_STUFF

/*
 * @brief Check the address canonicality
 * @param address - The address to check
 * @return `BOOL` - TRUE if the address is canonical
 */
BOOL CheckAddressCanonical(UINT64 address);

// #enddef

#define NMI_STUFF

typedef enum _NMI_BROADCAST_ACTION_TYPE {
  NMI_BROADCAST_ACTION_NONE = 0,
  NMI_BROADCAST_ACTION_TEST,
  NMI_BROADCAST_ACTION_REQUEST,
  NMI_BROADCAST_ACTION_INVALIDATE_EPT_CACHE_SINGLE_CONTEXT,
  NMI_BROADCAST_ACTION_INVALIDATE_EPT_CACHE_ALL_CONTEXTS,

} NMI_BROADCAST_ACTION_TYPE;

// #enddef