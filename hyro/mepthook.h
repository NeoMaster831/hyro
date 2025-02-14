#pragma once

/*
 * We should realize that we setup EPT page mapping like f(x) = x
 * which means, GPA and HPA are same. So we can use GPA as HPA.
 */

#include "global.h"
#include "ept.h"
#include "dsll.h"

extern LinkedList g_MEptHookPagesList;

// We only accept 4KB pages.
typedef struct _EPT_HOOK_PAGE {
  UINT64 physAligned; // aligned by PAGE_SIZE
  BOOL active;
  EPT_PML1_ENTRY origEntry;
  EPT_PML1_ENTRY hookEntry;
  PVOID hookCtx;
} EPT_HOOK_PAGE, *PEPT_HOOK_PAGE;

/*
 * @brief Add EPT hook
 * @param physicalAddr - The physical address to hook
 * @return `BOOL` - TRUE if the operation was successful
 */
BOOL MEptHookAddHook(UINT64 physicalAddr);

// ---------------------------------------------------------------------------
// DO NOT USE THIS FUNCTION IF VMX IS ACTIVE
// DO NOT USE THIS FUNCTION IF VMX IS ACTIVE
// DO NOT USE THIS FUNCTION IF VMX IS ACTIVE

/*
 * @brief Remove EPT hook
 * @param physicalAddr - The physical address to unhook
 */
VOID MEptHookRemoveHook(UINT64 physicalAddr);

// DO NOT USE THIS FUNCTION IF VMX IS ACTIVE
// DO NOT USE THIS FUNCTION IF VMX IS ACTIVE
// DO NOT USE THIS FUNCTION IF VMX IS ACTIVE
// ---------------------------------------------------------------------------

/*
 * @brief Get page of specific physical Address
 * @param physAddr - desired address
 * @return Ept hook page pointer, if it doesnt exist, returns `NULL`
 */
PEPT_HOOK_PAGE MEptHookGetHook(UINT64 physAddr);

/*
 * @brief Repair original page of specific hook
 * @param pVCpu - The virtual CPU pointer
 * @param pEptHook - The hook
 */
VOID MEptHookRepairOriginal(PVCPU pVCpu, PEPT_HOOK_PAGE pEptHook);

/*
 * @brief Change page of specific hook
 * @param pVCpu - The virtual CPU pointer
 * @param pEptHook - The hook
 * @param toHook - `TRUE` if you want to change the page to hook, or original
 */
VOID MEptHookChangePage(PVCPU pVCpu, PEPT_HOOK_PAGE pEptHook, BOOL toHook);

/*
 * @brief Handle the EPT hook violation
 * @param pVCpu - The virtual CPU pointer
 * @return `TRUE` if it is EPT hook violation, and successfully handled. `FALSE` otherwise
 */
BOOL MEptHookHandleViolation(PVCPU pVCpu);

/*
 * @brief Activate EPT hook
 * @param pEptHook - The hook
 */
VOID MEptHookActivateHook(PEPT_HOOK_PAGE pEptHook);

// ---------------------------------------------------------------------------
// DO NOT USE THIS FUNCTION IF VMX IS ACTIVE
// DO NOT USE THIS FUNCTION IF VMX IS ACTIVE
// DO NOT USE THIS FUNCTION IF VMX IS ACTIVE

/*
 * @brief Deactivate EPT hook
 * @param pEptHook - The hook
 */
VOID MEptHookDeactivateHook(PEPT_HOOK_PAGE pEptHook);

// DO NOT USE THIS FUNCTION IF VMX IS ACTIVE
// DO NOT USE THIS FUNCTION IF VMX IS ACTIVE
// DO NOT USE THIS FUNCTION IF VMX IS ACTIVE
// ---------------------------------------------------------------------------

/*
 * @brief Initialize EPT hook module
 * @return `BOOL` - TRUE if the operation was successful
 */
BOOL MEptHookInitialize();

/*
 * @brief Terminate EPT hook module
 */
VOID MEptHookTerminate();

/*
 * @brief Synchronize EPT hooks with invept
 */
VOID MEptHookSynchronize();