#include "common.h"

extern void (*g_PadSendHook)();
extern long D_80062640;
extern long D_80062644;

extern int DMACallback(int id, void (*pCallbackFn)());
extern void _SendPAD();

/* Best 97.80% (40 bytes = retail 40, 10 insns both; exactly 2 residuals, both
   hand-asm facts, re-measured and re-probed this round):
   - the call target sits in $t1 where cc1 uses $v0.  Retail's lui/lw name $t1 as
     the base register explicitly; a bare `lw $2,sym` is expanded by gas/maspsx
     through its own destination, so no C spelling can pick $t1.  11 spellings
     measured (direct call, hook local, cast from u_long, array element, struct
     member, pointer-to-pointer, volatile, int-returning, -O1, -O2, gcc 2.6.0,
     2.7.2-cdk) all give $v0; permuter 4872 iterations base=20 best=20.
   - ra at 0x14 of a 0x18 frame, so 20 bytes sit below it.  args is 16 for any
     unprototyped pointer call, and every addressable local rounds up to a
     multiple of 8 (cc1_probe on this unit's flags: u_long t[1] and dead scalars
     give vars 0, u_long t[2] / u_short t[2] / char t[3] / a 4-byte struct give
     vars 8, u_long t[3] and u_short t[6] give vars 16), so vars= 4 has no source
     spelling, and args= 20 would need a 5-word prototype whose argument setup
     adds instructions.  Retail's 4 bytes are prologue slack.
   Both neighbours (func_80040BA4, _remove_ChgclrPAD) are hand-patched asm, and an
   explicit `lui $t1` base is that style: treat this stub as asm-origin. */
INCLUDE_ASM("asm/slus_006.64/nonmatchings/psyq/libapi_b", _SendPAD);
/*
void _SendPAD(void) {
    (*g_PadSendHook)();
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
