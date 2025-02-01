#pragma once

#include <intrin.h>

#include "global.h"
#include "utils.h"

/*
 * @brief VM-exit handler
 * It handles the VM-exit and returns TRUE if the operation was successful
 * @param pGuestRegs - The guest registers - it is automatically filled by the
 * `pushaq` instruction (although I just implemented as push/pop)
 * @return Return True if VMXOFF executed (not in vmx anymore),
 *  or return false if we are still in vmx (so we should use vm resume)
 */
BOOL VmxExitHandler(PGUEST_REGS pGuestRegs);