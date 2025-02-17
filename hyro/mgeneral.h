#pragma once

#include "global.h"
#include "mem.h"
#include "dsll.h"

/*
 * @brief Initialize general module
 * @return `BOOL` - TRUE if the operation was successful
 */
BOOL MGeneralInitalize();

/*
 * @brief Terminate general module
 */
VOID MGeneralTerminate();

/*
 * @brief Get the physical address of a virtual address (NON PAGED MEMORY IN KERNEL, ATM)
 * @param virtualAddress - The virtual address
 */
UINT64 MGeneralGetPhysicalAddress(UINT64 virtualAddress);

/*
 * @brief Allocate non-paged buffer
 * @param size - The size of the buffer
 */
PVOID MGeneralAllocateNonPagedBuffer(UINT64 size);

/*
 * @brief Free non-paged buffer
 * @param buffer - The buffer to free
 */
VOID MGeneralFreeNonPagedBuffer(PVOID buffer);

/*
 * @brief Copy non-paged buffer (src, dst from guest CR3)
 * @param dest - The destination
 * @param src - The source
 * @param size - The size of the memory
 */
VOID MGeneralCopyNonPagedBuffer(PVOID dest, PVOID src, SIZE_T size);

/*
 * @brief Execute non-paged buffer (unstable!)
 * @param buffer - The buffer to execute
 * @param maxExecuteLength - The maximum length to execute (the maximum length of the shellcode)
 * @return `UINT64` - The return value of the shellcode. If the shellcode is not executed, it will return 0xFFFF'FFFF'FFFF'FFFF
 */
UINT64 MGeneralExecuteNonPagedBuffer(PVOID buffer, ULONG maxExecuteLength);