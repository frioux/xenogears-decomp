#include "common.h"
#include "main/game.h"

extern u8 D_800D2D24[];
extern u8 *D_800D2D28;
extern u8 D_800C3E50;
extern u8 *D_800C34B0;
extern u8 *D_800C3EA4;
extern s32 D_800CCB34;
extern u8 *D_800C3DFC;
extern u8 *D_800C3E34;
extern u8 D_800CCD3E[];
extern u8 D_800D2C54[];
extern u8 D_800D2C88[];
extern u16 D_800D2C9E;
extern u16 D_800C324E;
extern u16 D_800C3250;
extern u16 D_800C3252;
extern u16 D_800C323A;
extern u8 D_800CCD62[];
extern u8 *D_800C3E00;
extern u8 D_800CCE42[];
extern u8 D_800D32A1[];

void func_8007FB70(void);
void func_8009AC48(u8, s32);
void func_800916D4(u8, u8, u8);
void func_8009187C(u8, u8, u8);
void func_80091B38(u8, u8, u8);
void func_80093578(u8, u8);
void func_8009382C(u8, u8);
s32 func_80076A10(s32, u8 *, s32, s16);
s32 func_80089C6C(s32, u8);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80091064);
/* 94.88%: three addu operand orders + one load-scheduling slot differ; the
   instruction multiset and every load/store are identical. Tried =f()+a
   instead of += (flips the accumulate but moves the 0xF8 reload) and both
   source orders of offset+arg0 (gcc prints arg0 first regardless). */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80091604);
/*
void func_80091604(s16 arg0) {
    s32 i;
    s32 offset;

    *(s32 *)(D_800D2D28 + 0xF8) = 0;
    for (i = 0, offset = 0x38; i < 8; i++, offset += 8) {
        *(s32 *)(D_800D2D28 + 0xF8) += func_80076A10(0x66,
            D_800C3EA4 + 0xBA8 + *(s32 *)(D_800D2D28 + 0xF8) * 80, 0x20,
            (s16)(offset + arg0));
    }
    D_800D2D28[0xA5] = *(u8 *)&D_800CCB34;
    D_800D2D28[0x9C] = 1;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_800916D4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_8009187C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80091B38);
void func_80091D38(u8 arg0, s32 arg1, u8 arg2) {
    u8 ret = 0;

    if (D_800D32A1[arg0 * 8] == 0) {
        ret = (u16)func_80089C6C(
                  (u16)g_GameState.unk16BC[D_800D2D24[arg0]].unk6,
                  arg1 + arg2 * 2) != 0;
    } else if ((u16)func_80089C6C(
                   (u16)g_GameState.unk16BC[D_800D2D24[arg0]].unkA,
                   arg1 + arg2 * 2) != 0) {
        ret = 1;
    }
    if (ret != 0) {
        func_800916D4(arg1, arg2, arg0);
        func_8009187C(arg0, arg1, arg2);
        func_80091B38(arg0, arg1, arg2);
        (*(u8 **)(D_800C3EA4 + 0xA230))[0x66A] = *(u8 *)&D_800CCB34;
        (*(u8 **)(D_800C3EA4 + 0xA230))[0x66B] = 1;
    } else {
        (*(u8 **)(D_800C3EA4 + 0xA230))[0x66B] = 0;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80091EC4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_8009209C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80092298);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80092784);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80092B74);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_800930AC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80093578);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_8009382C);
/* 99.81%: instruction multiset and every operand identical except the two
   loads feeding the `and` - the original puts them in v0/v1, gcc 2.7.2 picks
   a0/v0 for the same insn order. Tried both source orders of the operands,
   separate `mask`/`flags` locals in both declaration orders, and the
   index/table loads inline; only the argument-setup order is source level. */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_800939CC);
/*
void func_800939CC(u8 arg0, u8 arg1) {
    u16 tbl[4];
    s32 o;

    tbl[0] = D_800C324E;
    tbl[1] = D_800C3250;
    tbl[2] = D_800C3252;
    tbl[3] = D_800C323A;
    if ((u16)func_80089C6C((u16)g_GameState.unk16BC[D_800D2D24[arg0]].unk1E,
                           arg1)) {
        o = arg0 * 0x170;
        if ((*(u16 *)(D_800CCD62 + o) & tbl[arg1]) == 0) {
            func_80093578(arg0, arg1);
            func_8009382C(arg0, arg1);
            (*(u8 **)(D_800C3EA4 + 0xA230))[0x66A] = *(u8 *)&D_800CCB34;
            (*(u8 **)(D_800C3EA4 + 0xA230))[0x66B] = 1;
        } else {
            (*(u8 **)(D_800C3EA4 + 0xA230))[0x66B] = 0;
        }
    } else {
        (*(u8 **)(D_800C3EA4 + 0xA230))[0x66B] = 0;
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80093B08);
void func_8009413C(u8 arg0, u8 arg1) {
    if (D_800D2D24[arg0] == 4) {
        D_800D2D28[0xB7] = 0;
        D_800D2D28[0xB0] = 0;
        if (arg1 != 0) {
            func_8007FB70();
        }
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_800941A4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_800946F4);
/* 99.77%: instruction multiset identical; the only difference is the operand
   order of one addu (i*0x170 + pBase). Tried array-ref, pointer-plus, integer
   base and a pointer local for that sum; gcc always emits index-first there. */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80094C78);
/*
void func_80094C78(void) {
    u8 *pBase = D_800C34B0;
    u8 *pOther = (u8 *)(D_800C3E50 * 4 + (u32)pBase);
    u8 i;

    for (i = 0; i < 3; i++) {
        if (((u8 *)(pBase + i))[0x5FA0] != 0) {
            continue;
        }
        if (*(u8 *)((u32)pBase + i * 0x170 + 0x15A) & 0x80) {
            *(u32 *)(pBase + i * 4 + 0x5F60) += *(u32 *)(pOther + 0x5F6C);
        } else {
            *(u32 *)(pBase + i * 4 + 0x5F54) += *(u32 *)(pOther + 0x5F6C);
        }
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80094D24);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_80094EE4);
void func_80095690(void) {
    u8 *p = D_800C3E00;
    u8 *q = D_800C3DFC;
    u8 *r;
    s16 v;
    u16 f;

    v = p[0x5B] * q[0x11];
    if (*(u16 *)(p + 0x8A) & 0x2000) {
        v *= 2;
    }
    r = D_800C3E34;
    f = *(u16 *)(r + 0x8C) | *(u16 *)(r + 0x8E);
    if (f & 0x100) {
        v = 7 * v / 10;
    }
    if (f & 0x200) {
        v = 13 * v / 10;
    }
    if (*(u8 *)(D_800CCE42 + D_800C3E50 * 0x170) & 0x80) {
        v = 0;
    }
    *(u8 *)(D_800C34B0 + D_800C3E50 + 0x5FA0) = 2;
    *(s32 *)(D_800C3E50 * 4 + D_800C34B0 + 0x5F6C) = v;
}
/* 95.00%: instruction multiset identical; the only difference is that gcc
   emits the sb's `li 2` before the five `sh zero` stores (the original keeps it
   after) plus the v0/v1 register colour. Tried the index as a local vs the
   global inline, both declaration orders and both source orders of the index
   load; the five stores go through a pointer loaded from a global, so the extra
   `nop` after that load is the load-delay hazard it introduces. */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_800957D8);
/*
void func_800957D8(void) {
    s32 t;
    u8 *p;
    s32 v;

    if (D_800CCD3E[D_800C3E50 * 0x170] == 2) {
        func_8009AC48(D_800C3E50, 1);
    }
    t = D_800C3E50;
    p = D_800C3E34;
    *(u16 *)(p + 0x7C) = 0;
    *(u16 *)(p + 0x80) = 0;
    *(u16 *)(p + 0x84) = 0;
    *(u16 *)(p + 0x88) = 0;
    *(u16 *)(p + 0x8C) = 0;
    D_800D2C88[t] = 2;
    p = D_800C3E34;
    v = *(u16 *)(p + 0x4E) * D_800C3DFC[0x11];
    t = D_800C3E50;
    *(s32 *)(t * 4 + D_800D2C54) = v / 10;
    D_800D2C9E |= 1 << t;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc17", func_800958D8);
