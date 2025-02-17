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

      BOOL result = HyroApiwTest();
      *(BOOL*)ioBuf = result;

      Irp->IoStatus.Information = sizeof(BOOL);

    } break;
    
    case IOCTL_HYROAPI_EPT_ADD: {

      if (inBufLength != sizeof(UINT64))
        goto INVALID_IOCTL;
      if (outBufLength != sizeof(BOOL))
        goto INVALID_IOCTL;


      BOOL result = HyroApiwEptAdd(*(UINT64*)ioBuf);
      *(BOOL*)ioBuf = result;

      Irp->IoStatus.Information = sizeof(BOOL);

    } break;

    case IOCTL_HYROAPI_EPT_REMOVE: {

      if (inBufLength != sizeof(UINT64))
        goto INVALID_IOCTL;
      
      HyroApiwEptRemove(*(UINT64*)ioBuf);

    } break;

    case IOCTL_HYROAPI_EPT_ENABLE: {

      if (inBufLength != sizeof(UINT64))
        goto INVALID_IOCTL;
      if (outBufLength != sizeof(BOOL))
        goto INVALID_IOCTL;

      BOOL result = HyroApiwEptEnable(*(UINT64*)ioBuf);
      *(BOOL*)ioBuf = result;

      Irp->IoStatus.Information = sizeof(BOOL);

    } break;

    case IOCTL_HYROAPI_EPT_DISABLE: {
      if (inBufLength != sizeof(UINT64))
        goto INVALID_IOCTL;
      HyroApiwEptDisable(*(UINT64*)ioBuf);
    } break;

    case IOCTL_HYROAPI_EPT_MODIFY: {

      if (inBufLength != sizeof(APIW_EPT_MODIFY_REQUEST))
        goto INVALID_IOCTL;
      if (outBufLength != sizeof(BOOL))
        goto INVALID_IOCTL;

      PAPIW_EPT_MODIFY_REQUEST req = (APIW_EPT_MODIFY_REQUEST*)ioBuf;
      BOOL result = HyroApiwEptModify(req->physicalAddr, req->hookCtx);
      *(BOOL*)ioBuf = result;

      Irp->IoStatus.Information = sizeof(BOOL);

    } break;

    case IOCTL_HYROAPI_GENERAL_GET_PHYSICAL_ADDRESS: {

      if (inBufLength != sizeof(APIW_GET_PHYS_ADDR_REQUEST))
        goto INVALID_IOCTL;
      if (outBufLength != sizeof(UINT64))
        goto INVALID_IOCTL;

      PAPIW_GET_PHYS_ADDR_REQUEST req = (APIW_GET_PHYS_ADDR_REQUEST*)ioBuf;
      UINT64 result = HyroApiwGnrGetPhysAddr(req->virtualAddress, req->cr3);
      *(UINT64*)ioBuf = result;

      Irp->IoStatus.Information = sizeof(UINT64);

    } break;
    
    case IOCTL_HYROAPI_GENERAL_ALLOC_NONPAGED_BUFFER: {
      if (inBufLength != sizeof(SIZE_T))
        goto INVALID_IOCTL;
      if (outBufLength != sizeof(PVOID))
        goto INVALID_IOCTL;

      PVOID result = HyroApiwGnrAllocNonPagedBuffer(*(SIZE_T*)ioBuf);
      *(PVOID*)ioBuf = result;
      Irp->IoStatus.Information = sizeof(PVOID);

    } break;

    case IOCTL_HYROAPI_GENERAL_FREE_NONPAGED_BUFFER: {
      if (inBufLength != sizeof(PVOID))
        goto INVALID_IOCTL;
      HyroApiwGnrFreeNonPagedBuffer(*(PVOID*)ioBuf);

    } break;
    
    case IOCTL_HYROAPI_GENERAL_COPY_NONPAGED_BUFFER: {
      if (inBufLength != sizeof(APIW_COPY_NONPAGED_BUFFER_REQUEST))
        goto INVALID_IOCTL;
      PAPIW_COPY_NONPAGED_BUFFER_REQUEST req = (APIW_COPY_NONPAGED_BUFFER_REQUEST*)ioBuf;
      HyroApiwGnrCopyNonPagedBuffer(req->dest, req->src, req->size);
    } break;
    
    case IOCTL_HYROAPI_GENERAL_EXECUTE_NONPAGED_BUFFER: {
      if (inBufLength != sizeof(APIW_EXECUTE_NONPAGED_BUFFER_REQUEST))
        goto INVALID_IOCTL;
      if (outBufLength != sizeof(UINT64))
        goto INVALID_IOCTL;
      PAPIW_EXECUTE_NONPAGED_BUFFER_REQUEST req = (APIW_EXECUTE_NONPAGED_BUFFER_REQUEST*)ioBuf;
      UINT64 result = HyroApiwGnrExecuteNonPagedBuffer(req->buffer, req->maxExecuteLength);
      *(UINT64*)ioBuf = result;

      Irp->IoStatus.Information = sizeof(UINT64);

    } break;
    
    case IOCTL_HYROAPI_GUEST_GENERAL_COPY_NONPAGED_BUFFER: {
      if (inBufLength != sizeof(APIW_COPY_NONPAGED_BUFFER_REQUEST))
        goto INVALID_IOCTL;
      PAPIW_COPY_NONPAGED_BUFFER_REQUEST req = (APIW_COPY_NONPAGED_BUFFER_REQUEST*)ioBuf;
      HyroApiwGuestGnrCopyNonPagedBuffer(req->dest, req->src, req->size);
    } break;

    case IOCTL_HYROAPI_GUEST_GENERAL_EXECUTE_NONPAGED_BUFFER: {
      if (inBufLength != sizeof(APIW_EXECUTE_NONPAGED_BUFFER_REQUEST))
        goto INVALID_IOCTL;
      if (outBufLength != sizeof(UINT64))
        goto INVALID_IOCTL;
      PAPIW_EXECUTE_NONPAGED_BUFFER_REQUEST req = (APIW_EXECUTE_NONPAGED_BUFFER_REQUEST*)ioBuf;
      UINT64 result = HyroApiwGuestGnrExecuteNonPagedBuffer(req->buffer, req->maxExecuteLength);
      *(UINT64*)ioBuf = result;

      Irp->IoStatus.Information = sizeof(UINT64);

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