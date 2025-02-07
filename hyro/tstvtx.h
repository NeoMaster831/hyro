#pragma once

#include "global.h"

/*
 * @brief Test Hyrocall
 * @param p1 - Optional Param 1
 * @param p2 - Optional Param 2
 * @param p3 - Optional Param 3
 */
void TstHyroTestcall(UINT64 p1, UINT64 p2, UINT64 p3);

extern void ATstHyroVmcall(UINT64 c, UINT64 p1, UINT64 p2, UINT64 p3);