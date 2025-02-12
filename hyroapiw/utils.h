#pragma once

#include <ntddk.h>

//
// Debug Print Levels
//
#define DBG_ERROR 0x00000004   // Error messages
#define DBG_WARNING 0x00000005 // Warning messages
#define DBG_INFO 0x00000006    // Information messages
#define DBG_TRACE 0x00000007   // Trace messages

//
// Debug Print Macros
//
#if DBG

#define APIW_LOG_ERROR(Format, ...)                                              \
  DbgPrintEx(DPFLTR_IHVDRIVER_ID, DBG_ERROR,                                   \
             "ERROR: [APIWrapper] " Format "\n", __VA_ARGS__)

#define APIW_LOG_WARNING(Format, ...)                                            \
  DbgPrintEx(DPFLTR_IHVDRIVER_ID, DBG_WARNING,                                 \
             "WARNING: [APIWrapper] " Format "\n", __VA_ARGS__)

#define APIW_LOG_INFO(Format, ...)                                               \
  DbgPrintEx(DPFLTR_IHVDRIVER_ID, DBG_INFO, "INFO: [APIWrapper] " Format "\n", \
             __VA_ARGS__)

#define APIW_LOG_TRACE(Format, ...)                                              \
  DbgPrintEx(DPFLTR_IHVDRIVER_ID, DBG_TRACE,                                   \
             "TRACE: [APIWrapper] " Format "\n", __VA_ARGS__)

#else

#define HV_LOG_ERROR(Format, ...)
#define HV_LOG_WARNING(Format, ...)
#define HV_LOG_INFO(Format, ...)
#define HV_LOG_TRACE(Format, ...)

#endif

#define UNUSED_PARAMETER(x) (void)(x)
