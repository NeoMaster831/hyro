#pragma once

#include <ntddk.h>

#include "utils.h"

#define TYPES

typedef unsigned long BOOL;

// #enddef TYPES

#define IOCTL_CODES

/*
 * @brief IOCTL code for testing the hypervisor
 */
#define IOCTL_HYROAPI_TEST \
	CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)

// #enddef

#define HYRO_CALL

/*
 * @brief Call the hypervisor
 * @return `HYRO_VMCALL_SUCCESS` if the call was successful
 */
extern UINT64 ACallHyro(UINT64 c, UINT64 p1, UINT64 p2, UINT64 p3);

// #enddef

#define HYRO_VMCALL_NUMBERS

#define HYRO_VMCALL_SUCCESS 0xd81b57669c771f7d
#define HYROCALL_SUCCESS(x) x == HYRO_VMCALL_SUCCESS ? TRUE : FALSE

#define HYRO_VMCALL_TEST 0x0
#define HYRO_VMCALL_VMXOFF 0x1
#define HYRO_VMCALL_EPT_ADDITION 0x2
#define HYRO_VMCALL_EPT_REMOVAL 0x3
#define HYRO_VMCALL_EPT_DISABLE 0x4
#define HYRO_VMCALL_EPT_ENABLE 0x5

// #enddef