#include "common.h"

// setjmp and longjmp save and restore ra, gp, sp, fp and s0-s7 at fixed offsets,
// which no C definition reproduces, so both stay INCLUDE_ASM.
//
// This file covers 0x3C650-0x3C6C8 only: Psy-Q's shipped .OBJ pads a function's
// tail with nops and the retail linker kept them, so func_8004BEC0's two pad
// words at 0x3C6C8 are an `asm` subsegment in config/slus_006.64.yaml and the
// rest of the unit is psyq/libapi_2_b.

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi_2", setjmp);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi_2", longjmp);

// longjmp's own `jr $ra` tail -- longjmp's .size stops before it -- which is the
// only definition that reproduces the two instructions. Its 8 bytes of tail
// alignment padding are an `asm` subsegment in config/slus_006.64.yaml.
void func_8004BEC0(void) {
}
