#include <ntddk.h>

#include "ioctl.h"

VOID DriverUnload(_In_ PDRIVER_OBJECT DriverObject) {
  UNREFERENCED_PARAMETER(DriverObject);
  PAGED_CODE();
  IoctlTerminate(DriverObject);

  APIW_LOG_INFO("API Wrapper unloaded");
}

NTSTATUS
DriverEntry(_In_ PDRIVER_OBJECT DriverObject,
            _In_ PUNICODE_STRING RegistryPath) {
  UNREFERENCED_PARAMETER(RegistryPath);

  DriverObject->DriverUnload = DriverUnload;
  
  if (!IoctlInit(DriverObject)) {
    return STATUS_UNSUCCESSFUL;
  }

  APIW_LOG_INFO("Successfully registered API Wrapper");

  return STATUS_SUCCESS;
}