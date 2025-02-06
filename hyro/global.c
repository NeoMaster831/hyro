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

#define A(a, b) a |= b
void InjectInterruption(INTERRUPT_TYPE InterruptionType,
    EXCEPTION_VECTORS Vector, BOOLEAN DeliverErrorCode,
    UINT32 ErrorCode) {
  UINT8 s = 0;
  INTERRUPT_INFO inject = {0};

  UNUSED_PARAMETER(s);

  inject.Fields.Valid = TRUE;
  inject.Fields.InterruptType = InterruptionType;
  inject.Fields.Vector = Vector;
  inject.Fields.DeliverCode = DeliverErrorCode;
  
  A(s, __vmx_vmwrite(VMCS_CTRL_VMENTRY_INTERRUPTION_INFORMATION_FIELD, inject.Flags));
  if (DeliverErrorCode) {
    A(s, __vmx_vmwrite(VMCS_CTRL_VMENTRY_EXCEPTION_ERROR_CODE, ErrorCode));
  }
}
#undef A

void InjectUD(PVCPU pVCpu) {
  InjectInterruption(INTERRUPT_TYPE_HARDWARE_EXCEPTION,
                     EXCEPTION_VECTOR_UNDEFINED_OPCODE, FALSE, 0);
  pVCpu->incrementRip = FALSE;
}

#define A(a, b) a |= b
void InjectGP() {
  UINT64 exitInstrLength;
  UINT8 s = 0;

  UNUSED_PARAMETER(s);

  InjectInterruption(INTERRUPT_TYPE_HARDWARE_EXCEPTION,
                     EXCEPTION_VECTOR_GENERAL_PROTECTION_FAULT, TRUE, 0);

  A(s, __vmx_vmread(VMCS_VMEXIT_INSTRUCTION_LENGTH, &exitInstrLength));
  A(s, __vmx_vmwrite(VMCS_CTRL_VMENTRY_INSTRUCTION_LENGTH, exitInstrLength));
}
#undef A

void InvVpid(INVVPID_TYPE Type, INVVPID_DESCRIPTOR *Descriptor) {
  INVVPID_DESCRIPTOR *TargetDescriptor = NULL;
  INVVPID_DESCRIPTOR ZeroDescriptor = {0};

  if (!Descriptor) {
    TargetDescriptor = &ZeroDescriptor;
  } else {
    TargetDescriptor = Descriptor;
  }

  AInvVpid(Type, TargetDescriptor);
}

BOOL CheckAddressCanonical(UINT64 address) {
  // TODO: add compatibility (but who the fuck doesnt use 48-bit address???)
  return ((address & 0xFFFF000000000000) == 0) ||
         ((address & 0xFFFF000000000000) == 0xFFFF000000000000);
}