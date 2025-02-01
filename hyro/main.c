#include "vtx.h"
#include "utils.h"
#include <ntddk.h>

VOID DriverUnload(_In_ PDRIVER_OBJECT DriverObject) {
  UNREFERENCED_PARAMETER(DriverObject);
  PAGED_CODE();
}

NTSTATUS
DriverEntry(_In_ PDRIVER_OBJECT DriverObject,
            _In_ PUNICODE_STRING RegistryPath) {
  UNREFERENCED_PARAMETER(RegistryPath);

  DriverObject->DriverUnload = DriverUnload;

  if (!VmxInitHypervisor()) {
    return STATUS_UNSUCCESSFUL;
  }

  return STATUS_SUCCESS;
}