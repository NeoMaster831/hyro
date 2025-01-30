#pragma once

#include <ntddk.h>

#define POOL_TAG 'hyro'

/*
 * @brief Allocate Zeroed Continuous Memory (ZCM)
 * @param size - The size of the memory to allocate
 * @return `PVOID` - The allocated memory
 */
PVOID MemAlloc_ZCM(SIZE_T size);

/*
 * @brief Allocate Zeroed Non-Paged Memory (ZNP)
 * @param size - The size of the memory to allocate
 * @return `PVOID` - The allocated memory
 */
PVOID MemAlloc_ZNP(SIZE_T size);

/*
 * @brief Free the Pool memory (P)
 * @param p - The memory to free
 */
VOID MemFree_P(PVOID p);

/*
 * @brief Free the Contiguous memory (C)
 * @param p - The memory to free
 */
VOID MemFree_C(PVOID p);