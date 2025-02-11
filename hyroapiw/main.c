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

  DbgPrintEx(DPFLTR_IHVDRIVER_ID, 0, "Hello, Nigga!\n");

  return STATUS_SUCCESS;
}