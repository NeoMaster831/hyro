#include "mem.h"

PVOID MemAlloc_ZCM(SIZE_T size) {
  PVOID result = NULL;
  PHYSICAL_ADDRESS highestAcceptableAddress = {0};

  highestAcceptableAddress.QuadPart = MAXULONG64;

  result = MmAllocateContiguousMemory(size, highestAcceptableAddress);
  if (result != NULL) {
    RtlZeroMemory(result, size);
  }

  return result;
}

PVOID MemAlloc_ZNP(SIZE_T size) { 
  PVOID result = ExAllocatePoolWithTag(NonPagedPool, size, POOL_TAG);
  if (result != NULL) {
    RtlZeroMemory(result, size);
  }
  return result;
}

VOID MemFree_P(PVOID p) { ExFreePoolWithTag(p, POOL_TAG); }

VOID MemFree_C(PVOID p) { MmFreeContiguousMemory(p); }