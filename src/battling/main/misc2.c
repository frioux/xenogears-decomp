#include "common.h"

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_80072D18);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_80073064);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_800730AC);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_800730F4);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_8007313C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_800731F8);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_800732AC);

/* 98.75%: identical, only the load temp is v0 where the original uses t0. */
/* void func_800732AC(u8 *p0, u8 *p1, s32 n) {
    n += (s32)p1;
    do {
        *(u32 *)p0 = *(u32 *)p1;
        p1 += 4;
        p0 += 4;
    } while ((s32)p1 != n);
} */

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

INCLUDE_ASM("asm/battling/nonmatchings/main/misc2", func_80073DE4);

/* 80.83%: same insns, but gcc puts `li v0,1` in its own block after the else
   path and jumps to the shared epilogue instead of filling the bnez delay slot. */
/* s32 func_80073DE4(u8 *p0, s32 arg1) {
    arg1 += *(s16 *)(p0 + 0xB6);
    if (arg1 < 0x1001) {
        return 1;
    }
    return ((arg1 - 0xFF1) / 20) < *(s16 *)(p0 + 0xB4);
} */

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
