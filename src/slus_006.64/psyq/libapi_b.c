#include "common.h"

extern void (*D_800593B8)();
extern long D_80062640;
extern long D_80062644;

extern int DMACallback(int id, void (*pCallbackFn)());
extern void func_80040B7C();

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi_b", func_80040B7C);
/*
Best 97.80%: everything but the call-target register and one stack slot. The
original loads the hook pointer into $t1 and keeps ra at 0x14 of a 0x18 frame
(0x10 outgoing-args area + a live 4-byte local that no spelling here produces);
gcc 2.7.2 puts the target in $v0 and saves ra at 0x10.
void func_80040B7C(void) {
    (*D_800593B8)();
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi_b", func_80040BA4);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi_b", func_80040C20);

void func_80040C3C(void) {
    func_80040B7C();
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi_b", func_80040C5C);

long func_80040CBC(long value) {
    long* pHook = &D_80062640;
    long old = *pHook;

    *pHook = value;
    return old;
}

long func_80040CD0(long value) {
    long* pHook = &D_80062644;
    long old = *pHook;

    *pHook = value;
    return old;
}

void* func_80040CE4(void (*pCallbackFn)()) {
    DMACallback(3, pCallbackFn);
}

/*
The rest of this unit is the hand-patched PAD/BiOS-call region and cannot be
converted:
- func_80040BA4 / func_80040C5C save $ra to a global (D_800593B4 / D_800593C4),
  enter the BIOS through a constant vector (`li t2,0xB0; jalr t2; li t1,0x57`)
  and patch two tables from a here-doc blob starting at func_80040C20. No C
  definition emits `sw $ra,<global>`, the $t1 BIOS selector, or the 0x0x20-style
  `addi` maspsx flags as handwritten (same blocker as psyq/libcard.c).
- func_80040C5C additionally has a pad `nop` after `.size`, and func_80040C20
  owns the 12 data bytes D_80040C30, so tools/scripts/convertible.py marks both
  BLOCKED.
*/
