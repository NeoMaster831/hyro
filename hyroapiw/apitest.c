#include "apitest.h"

BOOL HyroApiwTest() {
  return HYROCALL_SUCCESS(ACallHyro(HYRO_VMCALL_TEST, 0x69, 0x74, 0x420));
}