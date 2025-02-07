#include "tstvtx.h"

void TstHyroTestcall(UINT64 p1, UINT64 p2, UINT64 p3) {
  ATstHyroVmcall(0, p1, p2, p3);
}

UINT64 TstHyroXorcall(UINT64 p1, UINT64 p2) { return ATstHyroCpuid(0, p1, p2); }