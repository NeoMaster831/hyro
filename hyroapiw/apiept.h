#pragma once

#include "global.h"

BOOL HyroApiwEptAdd(UINT64 physicalAddr);

VOID HyroApiwEptRemove(UINT64 physicalAddr);

BOOL HyroApiwEptEnable(UINT64 physicalAddr);

VOID HyroApiwEptDisable(UINT64 physicalAddr);

typedef struct _APIW_EPT_MODIFY_REQUEST {
  UINT64 physicalAddr;
  PVOID hookCtx;
} APIW_EPT_MODIFY_REQUEST, *PAPIW_EPT_MODIFY_REQUEST;

BOOL HyroApiwEptModify(UINT64 physicalAddr, PVOID hookCtx);