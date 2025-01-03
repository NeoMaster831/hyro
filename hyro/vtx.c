#include "vtx.h"

NTSTATUS
CheckVtxSupport(_Out_ BOOLEAN *Supported) {
  int cpuInfo[4] = {0};

  if (Supported == NULL) {
    return STATUS_INVALID_PARAMETER;
  }

  __cpuid(cpuInfo, CPUID_PROCESSOR_INFO);

  // Check if VMX bit is set in ECX
  *Supported = ((cpuInfo[2] & VMX_SUPPORT_BIT) != 0);

  return STATUS_SUCCESS;
}