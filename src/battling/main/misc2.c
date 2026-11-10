#include "common.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_80072D18);

/* The depth is consumed out of $a2 by gte_ldir123_dp_a2, so the third
   parameter must stay in place. */
void func_80073064(SVECTOR *p0, SVECTOR *p1, s32 ir0) {
    gte_ldir123_lh(p0);
    gte_ldir123_dp_a2(ir0);
    gte_nop();
    gte_nop();
    gte_gpf12_b();
    gte_stir123_lh(p1);
}

void func_800730AC(SVECTOR *p0, SVECTOR *p1, s32 ir0) {
    gte_ldir123_lh(p0);
    gte_ldir123_dp_a2(ir0);
    gte_nop();
    gte_nop();
    gte_gpf0_b();
    gte_stir123_lh(p1);
}

void func_800730F4(SVECTOR *p0, SVECTOR *p1, s32 ir0) {
    gte_ldir123_lh4(p0);
    gte_ldir123_dp_a2(ir0);
    gte_nop();
    gte_nop();
    gte_gpf12_b();
    gte_stir123_lh(p1);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_8007313C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_800731F8);

/* 98.75%: only the load temp differs, the original uses $t0 where gcc picks
   $v0 (`lw t0,0(a1)` vs `lw v0,0(a1)`). Tried u8*/u32* parameters, `p0 += 4`
   against `p0++`, a separate `u32`/`u32 *end` local and a while loop (that one
   also grows a pre-test, 71.25%). The do/while with `n += (s32)p1` is right;
   the register is an allocator pick - $2/$3/$7 are all free here so gcc's
   first fresh pseudo is $v0, and no source shape moved it to $t0. */
/* void func_800732AC(u32 *p0, u32 *p1, s32 n) {
    n += (s32)p1;
    do {
        *p0 = *p1;
        p1++;
        p0++;
    } while ((s32)p1 != n);
} */
INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_800732AC);

extern u8 *func_8008D3F4(s32, s32);
extern void func_8008D5C0(u8 *, s32);
extern u8 *D_80092644;

void func_800732CC(void) {
    u8 *p = func_8008D3F4(3, 0);

    p[0x74] = 0x80;
    p[0x75] = 0x80;
    p[0x76] = 0xC0;
    func_8008D5C0(p, 0x60);
    *(s16 *)(p + 0x00) = 4;
    *(s16 *)(p + 0x44) = 0x300;
    *(s16 *)(p + 0x48) = 8;
    *(s16 *)(p + 0x4A) = 0x20;
    *(s16 *)(p + 0x68) = 0;
    *(s16 *)(p + 0x6A) = 0x20;
    D_80092644 = p;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_8007334C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_80073424);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_80073644);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_80073B7C);

void func_80073CA4(u8 *p0) {
    s32 i;

    for (i = 0; i < 16; i++) {
        u8 *p = p0 + 0x104 + i * 0x54;

        if (p[0x45] == 2) {
            p[0x45] = 1;
        } else {
            p[0x45] = 0;
        }
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_80073CEC);

/* 98.06%: inverting the condition (`if (t >= 0x1001)`) is what makes reorg put
   `li v0,1` in the bnez delay slot instead of a separate block with a `j`, and
   `hw + arg1` fixes the addu operand order. Left over: the original keeps the
   sum in the parameter's $a1 and the hw temp in $v1 (`lh v1; addu a1,v1,a1;
   slti v1,a1`), ours assigns the sum to $v1 and the temp to $v0. Also tried
   `arg1 += hw` in place, a separate s32/s16 local, and `t` declared after the
   load - the sum pseudo never coalesces back into $a1. */
/* s32 func_80073DE4(u8 *p0, s32 arg1) {
    s32 t = *(s16 *)(p0 + 0xB6) + arg1;

    if (t >= 0x1001) {
        return ((t - 0xFF1) / 20) < *(s16 *)(p0 + 0xB4);
    }
    return 1;
} */
INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_80073DE4);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_80073E2C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_80073F34);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_800740E4);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_80074678);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_80074998);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_80074AB4);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_80074BA4);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_80075060);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_800751C8);

s32 func_8007570C(u8 *p0, u8 *p1, s32 scale) {
    return (*(s16 *)(p1 + 0x15EC) * scale) / 32 - *(s16 *)(p0 + 0x15EE);
}
