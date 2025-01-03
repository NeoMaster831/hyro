#include "vtx.h"
#include "utils.h"
#include <ntddk.h>

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, DriverUnload)
#endif

//
// Logging is enabled by default
//
#define LOGGING_ENABLED

NTSTATUS
DriverEntry(_In_ PDRIVER_OBJECT DriverObject,
            _In_ PUNICODE_STRING RegistryPath) {
  NTSTATUS status = STATUS_SUCCESS;
  BOOLEAN vtxSupported = FALSE;

  UNREFERENCED_PARAMETER(RegistryPath);

  DriverObject->DriverUnload = DriverUnload;

  status = CheckVtxSupport(&vtxSupported);
  if (!NT_SUCCESS(status)) {
    HV_LOG_ERROR("Failed to check VT-x support: 0x%X", status);
    return status;
  }

  if (!vtxSupported) {
    HV_LOG_ERROR("This processor does not support VT-x");
    return STATUS_NOT_SUPPORTED;
  }

  HV_LOG_INFO("VT-x is supported on this processor");
  return status;
}

VOID DriverUnload(_In_ PDRIVER_OBJECT DriverObject) {
  UNREFERENCED_PARAMETER(DriverObject);
  PAGED_CODE();
}