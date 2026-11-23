#include "common.h"

extern void (*D_800593B8)();
extern long D_80062640;
extern long D_80062644;

extern int DMACallback(int id, void (*pCallbackFn)());
extern void _SendPAD();

/* Best 97.80% (40 bytes = retail 40, 10 insns both, exactly 2 lines differ, and
   neither is a source fact:
   - retail keeps the call target in $t1 (lui $t1 / lw $t1 / jalr $t1); every
     spelling tried puts it in $v0. Measured with cc1_probe (psx 2.7.2 -O3 -G0):
     direct (*hook)(), local VoidFn, register-free cast from u_long, array
     element, struct member, hook reached through a pointer-to-pointer, volatile
     hook, int-returning hook, and nothing changes; -O1/-O2, gcc-2.6.0 and
     2.7.2-cdk are identical (the load is hoisted above the prologue in all of
     them, so that part already agrees).
   - retail saves ra at 0x14 of a 0x18 frame, i.e. its .frame line reads vars= 4:
     one 4-byte stack slot that no instruction writes - a pseudo spilled across
     the call whose every use was rematerialised. Every spelling above measures
     vars= 0 (ra at 0x10); the only bodies that grow the var area are ones with
     an address-taken local, and one of those measures vars= 8, not 4.
   Consistent with the region: func_80040BA4 and _remove_ChgclrPAD on either
   side of it are hand-patched asm (sw $ra to a global, BIOS vector via $t2 with
   li $t1,0x57), and $t1 as the jump register is that style, not gcc's.
   Re-measured (round 420): 97.80% / 40 bytes / 10 insns, same two lines. Extra
   probe: every address-taken 4-byte local (dead `long t` + (void)&t, `char t[4]`,
   and `t = 0` before the call, which also stores to 0x10(sp)) costs vars= 8
   (frame 0x20, ra at 0x18), so vars= 4 has no source spelling here. */
INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi_b", _SendPAD);
/*
void _SendPAD(void) {
    (*D_800593B8)();
}*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi_b", func_80040BA4);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi_b", func_80040C20);

void SendPAD(void) {
    _SendPAD();
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi_b", _remove_ChgclrPAD);

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
- func_80040BA4 / _remove_ChgclrPAD save $ra to a global (D_800593B4 / D_800593C4),
  enter the BIOS through a constant vector (`li t2,0xB0; jalr t2; li t1,0x57`)
  and patch two tables from a here-doc blob starting at func_80040C20. No C
  definition emits `sw $ra,<global>`, the $t1 BIOS selector, or the 0x0x20-style
  `addi` maspsx flags as handwritten (same blocker as psyq/libcard.c).
- _remove_ChgclrPAD additionally has a pad `nop` after `.size`, and func_80040C20
  owns the 12 data bytes D_80040C30, so tools/scripts/convertible.py marks both
  BLOCKED.
- func_80040C20 is not a function at all: its symbol is one word (`and
  $v0,$v0,$s5`, the first word of the patch blob), then three pad words, then the
  D_80040C30 data it owns. The shortest C definition is two words, so no C body
  can size this region; it stays asm.
*/
