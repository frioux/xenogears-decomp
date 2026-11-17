#include "common.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"

// NormalClip. It is its own translation unit because every neighbour carries tail
// pad words in retail (the shipped .OBJ pads every function's tail to a 16-byte
// multiple and the linker kept them): RotTrans's two words at 0x3AF04,
// NormalClip's three at 0x3AF30 and RotTransPers4's two at 0x3AFB4 are `asm`
// subsegments in config/slus_006.64.yaml, so this file covers 0x3AF0C-0x3AF30.

/* Sony's vector.s. The three SXY writes go in as 0, 2, 1 -- exactly
   gte_ldsxy3's order -- the NCLIP takes two pipeline nops, and the opz read-back
   is written out because gte_stopz_val appends a nop retail does not have. */
long NormalClip(long sxy0, long sxy1, long sxy2) {
    long r;

    gte_ldsxy3(sxy0, sxy1, sxy2);
    gte_nop();
    gte_nop();
    gte_nclip_b();
    __asm__ volatile ("mfc2\t%0, $24" : "=r"(r));
    return r;
}
