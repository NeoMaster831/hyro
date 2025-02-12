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