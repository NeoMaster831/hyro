#pragma once

#include <ntddk.h>

//
// Debug Print Levels
//
#define DBG_ERROR 0x00000000   // Error messages
#define DBG_WARNING 0x00000001 // Warning messages
#define DBG_INFO 0x00000002    // Information messages
#define DBG_TRACE 0x00000003   // Trace messages

//
// Debug Print Macros
//
#if DBG

#define HV_LOG_ERROR(Format, ...)                                              \
  DbgPrintEx(DPFLTR_IHVDRIVER_ID, DBG_ERROR,                                   \
             "ERROR: [HyperVisor] " Format "\n", __VA_ARGS__)

#define HV_LOG_WARNING(Format, ...)                                            \
  DbgPrintEx(DPFLTR_IHVDRIVER_ID, DBG_WARNING,                                 \
             "WARNING: [HyperVisor] " Format "\n", __VA_ARGS__)

#define HV_LOG_INFO(Format, ...)                                               \
  DbgPrintEx(DPFLTR_IHVDRIVER_ID, DBG_INFO, "INFO: [HyperVisor] " Format "\n", \
             __VA_ARGS__)

#define HV_LOG_TRACE(Format, ...)                                              \
  DbgPrintEx(DPFLTR_IHVDRIVER_ID, DBG_TRACE,                                   \
             "TRACE: [HyperVisor] " Format "\n", __VA_ARGS__)

#else

#define HV_LOG_ERROR(Format, ...)
#define HV_LOG_WARNING(Format, ...)
#define HV_LOG_INFO(Format, ...)
#define HV_LOG_TRACE(Format, ...)

#endif

#define UNUSED_PARAMETER(x) (void)(x)

#define HYRO_SIGNATURE_LOW 0x80187c4d01ad09cc
#define HYRO_SIGNATURE_MEDIUM 0x1a2b99b4c7f9d191
#define HYRO_SIGNATURE_HIGH 0x913a2b99b4c7f9d1