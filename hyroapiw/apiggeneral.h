#pragma once

#include "global.h"

// Using same structure as in hyroapiw/apigeneral.h
VOID HyroApiwGuestGnrCopyNonPagedBuffer(PVOID dest, PVOID src, SIZE_T size);

// Using same structure as in hyroapiw/apigeneral.h
UINT64 HyroApiwGuestGnrExecuteNonPagedBuffer(PVOID buffer, ULONG maxExecuteLength);