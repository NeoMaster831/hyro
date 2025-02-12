#include "ioctl.h"

BOOL IoctlInit(PDRIVER_OBJECT DriverObject) {
  NTSTATUS status;
  UNICODE_STRING deviceName;
  UNICODE_STRING symLinkName;
  PDEVICE_OBJECT deviceObject;
  PDEVICE_EXTENSION deviceExtension;

  RtlInitUnicodeString(&deviceName, L"\\Device\\HyroAPIW");
  RtlInitUnicodeString(&symLinkName, L"\\DosDevices\\HyroAPIW");

  status = IoCreateDevice(DriverObject, sizeof(DEVICE_EXTENSION), &deviceName,
                          FILE_DEVICE_UNKNOWN, 0, FALSE, &deviceObject);

  if (status != STATUS_SUCCESS) {
    APIW_LOG_ERROR("Failed to create device object (0x%08X)", status);
    return FALSE;
  }

  deviceExtension = (PDEVICE_EXTENSION)deviceObject->DeviceExtension;
  deviceExtension->DeviceName = deviceName;
  deviceExtension->SymLinkName = symLinkName;

  status = IoCreateSymbolicLink(&symLinkName, &deviceName);

  if (status != STATUS_SUCCESS) {
    APIW_LOG_ERROR("Failed to create symbolic link (0x%08X)", status);
    IoDeleteDevice(deviceObject);
    return FALSE;
  }

  DriverObject->MajorFunction[IRP_MJ_CREATE] = IoctlHandle;
  DriverObject->MajorFunction[IRP_MJ_CLOSE] = IoctlHandle;
  DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IoctlHandle;

  return TRUE;
}

NTSTATUS IoctlHandle(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp) {
  UNREFERENCED_PARAMETER(DeviceObject);
  
  NTSTATUS status = STATUS_SUCCESS;
  PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(Irp);
  ULONG inBufLength, outBufLength;
  PVOID ioBuf;

  switch (irpSp->MajorFunction) {
  case IRP_MJ_CREATE:
    APIW_LOG_INFO("IRP_MJ_CREATE");
    break;
  case IRP_MJ_CLOSE:
    APIW_LOG_INFO("IRP_MJ_CLOSE");
    break;
  case IRP_MJ_DEVICE_CONTROL:
    inBufLength = irpSp->Parameters.DeviceIoControl.InputBufferLength;
    outBufLength = irpSp->Parameters.DeviceIoControl.OutputBufferLength;
    ioBuf = Irp->AssociatedIrp.SystemBuffer;

    switch (irpSp->Parameters.DeviceIoControl.IoControlCode) {

    case IOCTL_HYROAPI_TEST: {

      if (outBufLength != sizeof(BOOL))
        goto INVALID_IOCTL;
			*(BOOL*)ioBuf = HyroApiwTest();
			Irp->IoStatus.Information = sizeof(BOOL);

    } break;

    default: 
      goto INVALID_IOCTL;

    }

    break;
  default:
INVALID_IOCTL:
    status = STATUS_INVALID_DEVICE_REQUEST;
    Irp->IoStatus.Information = 0;
    break;
  }

  Irp->IoStatus.Status = status;
  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return status;
}

VOID IoctlTerminate(PDRIVER_OBJECT DriverObject) {
  PDEVICE_EXTENSION deviceExtension =
      (PDEVICE_EXTENSION)DriverObject->DeviceObject->DeviceExtension;
  UNICODE_STRING symLinkName = deviceExtension->SymLinkName;
  IoDeleteSymbolicLink(&symLinkName);
  IoDeleteDevice(DriverObject->DeviceObject);
}