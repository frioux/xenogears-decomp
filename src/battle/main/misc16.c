#include "common.h"
#include "main/game.h"
#include "psyq/libgpu.h"

extern u8 D_800D2CE0[];
extern u8 *D_800D329C;
extern u8 *D_800C3EA4;
extern s32 D_800CCB34;
extern u8 *D_800D2D28;
extern u8 *D_800D2E38[];
extern u8 *D_800D2D90[];
extern u16 g_SystemPalette1;
extern u16 g_SystemPalette2;
extern u8 D_800CCD38[];
extern u8 D_800CCD3A[];
extern u8 D_800D2F7C;
extern u8 D_800D2F7D;
extern u8 D_800D2F7E;
extern u8 D_800D2F7F;
extern u8 D_800D2F80;
extern u8 D_800D2F81;
extern u8 D_800D2F82;
extern u8 D_800D2F83;

extern void func_800716D8(void);
extern void HeapFree(void *pMem);
extern void func_8008FC1C(s32, s32, s32, s32, u8);
extern s32 func_80076A10(s32, s32, s16, s16);
extern void func_80076C78(POLY_FT4 *p, s16 x, s16 y, u8 u, u8 v, s16 dx);
extern void func_800769E8(RECT *pRect, u_long *pData);
extern void *func_8008AC00(s32);
extern void bzero(void *p, s32 n);
extern u8 *GetStringEntry(u8 *, s32);
extern s32 SystemRenderStringEntry(u8 *, void *, s32, s32);
extern u8 D_800C3D70[];
extern void *func_8008ABB8(s32, s32);
extern void func_80077454(s32);
extern void func_8008F6E4(u8, s32, s32, s32, s32);
extern void func_8008DC34(u8, s32, s32, s32, u16);
extern void func_8008DE04(u8, s32, s32, s32);
extern void func_8008E430(u8, s32, s32, s32, u16);
extern void func_8008EA70(u8, s32, s32, s32);
extern void func_8008F0A8(u8, s32, s32, s32, u16);
extern void func_80090310(u8, u8);
extern void func_800904A0(s32, s32);
extern void func_8009070C(u8, u8);
extern void *D_800D2F5C;
extern u8 D_800C3E4C;
extern void func_80076B00(void *);
extern s32 func_800263E4(void *, s32, void *, s32, s16, s16, s32, s32, s32);
extern u16 D_800C324E;
extern u16 D_800C3250;
extern u16 D_800C3252;
extern u16 D_800C323A;
extern u8 D_800CEF34[];
extern u8 D_800CEF10[];
extern u8 D_800CCDC4[];
extern u8 D_800CCD62[];
extern u8 D_800D2D24[];
extern s32 func_80089C6C(s32, u8);
extern u8 func_80085084(u16, u8, s32);
extern void func_8008CCCC(u8);
extern void func_8008CD28(u8);
extern void func_8008AA74(u8);
extern void func_80076CE8(POLY_FT4 *, s16, s16, u8, u8, s16, s16);

u8 func_8008CDE4(u8 arg0, u8 arg1) {
    u16 hdr[4];
    u8 flag;
    u8 ret;
    s32 idx;
    s32 ofs;
    u16 trigger;
    u16 cur;
    u16 dmg;

    hdr[0] = D_800C324E;
    hdr[1] = D_800C3250;
    hdr[2] = D_800C3252;
    hdr[3] = D_800C323A;
    ret = 0;
    flag = 1;
    idx = (arg1 + 0x25) * 0x28 + arg0 * 0x690;
    ofs = arg0 * 0x170;
    trigger = *(u16 *)(D_800CEF34 + idx);
    cur = *(u16 *)(D_800CCDC4 + ofs);
    dmg = *(u16 *)(D_800CEF10 + idx);
    if (cur >= trigger &&
        (u16)func_80089C6C((u16)g_GameState.unk16BC[D_800D2D24[arg0]].unk1E, arg1) != 0 &&
        (*(u16 *)(D_800CCD62 + ofs) & hdr[arg1]) == 0) {
        func_8008CCCC(1);
        D_800D2D28[0xC6] = 1;
        if (func_80085084(dmg, arg0, 0) != 0) {
            *(u16 *)(D_800CCDC4 + ofs) -= trigger;
            ret = 1;
        } else {
            D_800D2D28[0xC6] = 0;
            func_8008CD28(arg0);
        }
        flag = 0;
    }
    if (flag != 0) {
        func_8008AA74(0x4F);
    }
    return ret;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008CFB8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008D328);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008D598);
/* 99.91%: identical except ONE word -- the original ends the dy computation
   with addu s0,s4,s6 (arg2's register first) and gcc emits addu s0,s6,s4.
   Tried: source order arg2+arg4 and arg4+arg2 (gcc emits s6,s4 for BOTH, so it
   is not an RTL operand order issue -- the allocator/emit order is canonical),
   implicit narrowing instead of the (s16) cast, an (s32) cast on arg4, a split
   s32 sum temp, and gcc-2.7.2-cdk (84.94, so the preset is right). Everything
   else in the 464-byte body matches byte for byte: the codes[] branch pair, the
   four func_80076A10 calls with the 0x5A0 accumulate operand order, dx between
   calls 1 and 2, dy between calls 2 and 3, and the 4-iteration func_80076B00
   loop.
void func_8008DC34(u8 arg0, s32 arg1, s32 arg2, s32 arg3, u16 arg4) {
    u8 codes[4];
    u8 *p;
    s16 dx;
    s16 dy;
    s32 sum;
    s32 i;

    p = D_800D2E38[arg0];
    if (D_800C3E4C != 0) {
        codes[0] = 0x4A;
        codes[1] = 0x4C;
        codes[2] = 0x4F;
        codes[3] = 0x51;
    } else {
        codes[0] = 0xF0;
        codes[1] = 0xF2;
        codes[2] = 0xF5;
        codes[3] = 0xF7;
    }
    *(s32 *)(p + 0x5A0) = 0;
    *(s32 *)(p + 0x5A0) = func_80076A10(codes[0], (s32)(p + *(s32 *)(p + 0x5A0) * 0x50), (s16)arg1, (s16)arg2) + *(s32 *)(p + 0x5A0);
    dx = arg1 + arg3 - 8;
    *(s32 *)(p + 0x5A0) = func_80076A10(codes[1], (s32)(p + *(s32 *)(p + 0x5A0) * 0x50), dx, (s16)arg2) + *(s32 *)(p + 0x5A0);
    dy = (s16)(arg2 + arg4 - 8);
    *(s32 *)(p + 0x5A0) = func_80076A10(codes[2], (s32)(p + *(s32 *)(p + 0x5A0) * 0x50), (s16)arg1, dy) + *(s32 *)(p + 0x5A0);
    *(s32 *)(p + 0x5A0) = func_80076A10(codes[3], (s32)(p + *(s32 *)(p + 0x5A0) * 0x50), dx, dy) + *(s32 *)(p + 0x5A0);
    for (i = 0; i < 4; i++) {
        func_80076B00(p + (i * 2 + D_800CCB34) * 0x28);
    }
}

*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008DC34);


INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008DE04);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008E430);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008EA70);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008F0A8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008F6E4);
/* 57.38% (was 48.61 with inline sums, 53.06 with u16 params): the five calls
   and the four u16 arg masks are right; the gap is the eight sh stores. Retail
   recomputes `lw D_800CCB34 / sll / addu / sll` in front of EVERY store, which
   the full-expression form below does reproduce, but retail also keeps the same
   s1/s0 (== arg1/arg2) for the store values while gcc puts the sums in t0/t1 and
   runs the frame 0x38 with s0-s5 (here a bigger frame and one fewer recompute --
   416 bytes vs 528). Tried: u16 params (53.06), s32 params + w/h locals (48.61),
   inline sums (50.12), per-store full expression (57.38). Same &D_800CCB34 wall
   as func_80090E7C/func_80090310 in this unit.
void func_8008F6E4(u8 arg0, s32 arg1, s32 arg2, s32 arg3, s32 arg4) {
    u8 *p;

    D_800D2D28[arg0 + 0xB0] = 0;
    *(u16 *)(D_800D2E38[arg0] + D_800CCB34 * 0x24 + 0x3C8) = arg1;
    *(u16 *)(D_800D2E38[arg0] + D_800CCB34 * 0x24 + 0x3CA) = arg2;
    *(u16 *)(D_800D2E38[arg0] + D_800CCB34 * 0x24 + 0x3D0) = arg1 + arg3;
    *(u16 *)(D_800D2E38[arg0] + D_800CCB34 * 0x24 + 0x3D2) = arg2;
    *(u16 *)(D_800D2E38[arg0] + D_800CCB34 * 0x24 + 0x3D8) = arg1;
    *(u16 *)(D_800D2E38[arg0] + D_800CCB34 * 0x24 + 0x3DA) = arg2 + arg4;
    *(u16 *)(D_800D2E38[arg0] + D_800CCB34 * 0x24 + 0x3E0) = arg1 + arg3;
    *(u16 *)(D_800D2E38[arg0] + D_800CCB34 * 0x24 + 0x3E2) = arg2 + arg4;
    func_8008DC34(arg0, arg1, arg2, arg3, arg4);
    func_8008DE04(arg0, arg1, arg2, arg3);
    func_8008E430(arg0, arg1, arg2, arg3, arg4);
    func_8008EA70(arg0, arg1, arg2, arg4);
    func_8008F0A8(arg0, arg1, arg2, arg3, arg4);
    *(u8 *)(D_800D2E38[arg0] + 0x5A4) = *(u8 *)&D_800CCB34;
    D_800D2D28[arg0 + 0xB0] = 1;
}
*/
/* 96.44%: whole body matches; only the prologue save/copy order differs (gcc
   gives s4 to arg5 and s5 to arg1 and copies arg0 last; the original has
   s4=arg1, s5=arg5 and copies arg0 first). Tried: separate pointer temps per
   block, a pFlags temp for D_800D2D28 (that load has to sit before the sh
   stores or gcc moves it after them), `*(base + index + K)` instead of
   `base[index + K]` (only the former gives the base-first addu), inverted
   if/else, and two declaration orders. Worth one more try by someone who can
   read gcc's allocno order.

void func_8008F8F4(s32 arg0, u16 arg1, u16 arg2, u16 arg3, u16 arg4, u8 arg5, u8 arg6) {
    u8 *pFlags;
    u8 index;
    u8 *p;
    u8 *q;

    index = arg0;
    if (*(D_800D2D28 + index + 0xB0) == 0) {
        q = func_8008ABB8(0x5A8, 0);
        D_800D2E38[index] = q;
        bzero(q, 0x5A8);
        q = func_8008ABB8(0xE, 0);
        D_800D2D90[index] = q;
        bzero(q, 0xE);
        func_80077454(index);
    }
    if (arg5 != 0) {
        p = D_800D2D90[index];
        p[0xC] = arg0;
        pFlags = D_800D2D28;
        ((u16 *)p)[0] = arg1;
        ((u16 *)p)[1] = arg2;
        ((u16 *)p)[2] = arg3;
        ((u16 *)p)[3] = arg4;
        ((u16 *)p)[4] = 0;
        ((u16 *)p)[5] = 0;
        *(pFlags + index + 0xBF) = 0;
        pFlags = D_800D2D28;
        *(pFlags + index + 0xB8) = 1;
    } else {
        func_8008F6E4(index, arg1, arg2, arg3, arg4);
        if (arg6 != 0) {
            func_800716D8();
        }
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008F8F4);
