#pragma once

#include <ntddk.h>

// Intel SDM Vol. 3C, Section 23.6
#define CPUID_PROCESSOR_INFO 0x1
#define VMX_SUPPORT_BIT (1 << 5) // CPUID.1:ECX.VMX[bit 5]

// Return codes
typedef enum _VTX_STATUS {
  VTX_SUCCESS = 0,
  VTX_ERROR_NO_VMX_SUPPORT,
  VTX_ERROR_UNKNOWN
} VTX_STATUS;

NTSTATUS CheckVtxSupport(BOOLEAN *Supported);