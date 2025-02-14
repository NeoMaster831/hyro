#pragma once

#include <ntddk.h>
#include <intrin.h>

#include "ia32.h"
#include "global.h"
#include "mem.h"
#include "utils.h"

/*
 * @brief Check if EPT is supported on the current processor
 * @return `BOOL` - TRUE if the operation was successful
 */
BOOL CheckEptSupport();

/*
 * @brief Build the MTRR map
 */
VOID EptBuildMtrrMap();

/*
 * @brief Get the PML2 entry
 * @param pEptPageTable - The EPT page table pointer
 * @param physicalAddr - The physical address
 * @return `PEPT_PML2_ENTRY` - The PML2 entry
 */
PEPT_PML2_ENTRY EptGetPml2(PEPT_PAGE_TABLE pEptPageTable, SIZE_T physicalAddr);

/*
 * @brief Get the PML1 entry
 * @param pEptPageTable - The EPT page table pointer
 * @param physicalAddr - The physical address
 * @return `PEPT_PML1_ENTRY` - The PML1 entry
 */
PEPT_PML1_ENTRY EptGetPml1(PEPT_PAGE_TABLE pEptPageTable, SIZE_T physicalAddr);

/*
 * @brief Get the PML1 or PML2 entry
 * @param pEptPageTable - The EPT page table pointer
 * @param physicalAddr - The physical address
 * @return `PVOID` - The PML1 or PML2 entry
 */
PVOID EptGetPml1OrPml2(PEPT_PAGE_TABLE pEptPageTable, SIZE_T physicalAddr, BOOL *isPml1);

/*
 * @brief Check if the EPT is valid for large page
 * @param pfn - The page frame number
 * @return `BOOL` - TRUE if the operation was successful
 */
BOOL EptIsValidForLargePage(SIZE_T pfn);

/*
 * @brief Get the memory type for the EPT
 * @param pfn - The page frame number
 * @param isLargePage - TRUE if the page is a large page
 * @return `UINT8` - The memory type
 */
UINT8 EptGetMemoryType(SIZE_T pfn, BOOL isLargePage);

/*
 * @brief Split the large page
 * @param pEptPageTable - The EPT page table pointer
 * @param physicalAddr - The physical address where we want to split the page
 */
BOOL EptSplitLargePage(PEPT_PAGE_TABLE pEptPageTable,
                       SIZE_T physicalAddr);

/*
 * @brief Build the PML2 entry
 * @param pEptPageTable - The EPT page table pointer
 * @param newEntry - The new entry
 * @param pfn - The page frame number
 * @return `BOOL` - TRUE if the operation was successful
 */
BOOL EptBuildPml2(PEPT_PAGE_TABLE pEptPageTable, PEPT_PML2_ENTRY newEntry,
                     SIZE_T pfn);

/*
 * @brief Build the EPT page table
 * @return `EPT_PAGE_TABLE` - The EPT page table
 */
PEPT_PAGE_TABLE EptBuildPageTable();

/*
 * @brief Initialize the EPT in all VCPUs
 * @return `BOOL` - TRUE if the operation was successful
 */
BOOL EptInitialize();