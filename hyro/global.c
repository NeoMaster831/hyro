#include "global.h"

VCPU *g_arrVCpu = NULL;
UINT64 *g_invalidMsrBitmap = NULL;
EPT_STATE g_EptState = {0};

void SetBit(int BitNumber, unsigned long* Addr) {
  BITMAP_ENTRY(BitNumber, Addr) |= (1UL << BITMAP_SHIFT(BitNumber));
}

UINT64 GetSsDTBase() {
  PNT_KPROCESS sysProc = (PNT_KPROCESS)PsInitialSystemProcess;
  return sysProc->DirectoryTableBase;
}

BOOL GetSegmentDescriptor(PUINT8 gdtBase, UINT16 selector,
                          PVMX_SEGMENT_SELECTOR segSel) {
  SEGMENT_DESCRIPTOR_32 *descTable, *desc;
  SEGMENT_SELECTOR seg = {.AsUInt = selector};

  // Ignore LDTs
  if (!seg.AsUInt || seg.Table)
    return FALSE;

  descTable = (SEGMENT_DESCRIPTOR_32 *)gdtBase;
  desc = &descTable[seg.Index];

  segSel->Selector = seg.AsUInt;
  segSel->Limit = __segmentlimit(seg.AsUInt);
  segSel->Base = ((UINT64)desc->BaseAddressLow | (UINT64)desc->BaseAddressMiddle << 16 |
                  (UINT64)desc->BaseAddressHigh << 24);
  segSel->Attributes.AsUInt = (AGetAccessRights(seg.AsUInt) >> 8);

  if (seg.Index == 0) {
    segSel->Attributes.s.Unusable = 1;
  }

  if ((desc->Type == SEGMENT_DESCRIPTOR_TYPE_TSS_BUSY) ||
      (desc->Type == SEGMENT_DESCRIPTOR_TYPE_CALL_GATE)) {
    UINT64 high = *(UINT64 *)((PUINT8)desc + 8);
    segSel->Base = (segSel->Base & 0xFFFFFFFF) | (high << 32);
  }

  if (segSel->Attributes.s.Granularity) {
    segSel->Limit = (segSel->Limit << 12) | 0xFFF;
  }

  return TRUE;
}