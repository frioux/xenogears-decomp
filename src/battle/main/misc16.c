#include "common.h"
#include "main/game.h"
#include "psyq/libgpu.h"

extern u8 D_800D2CE0[];
extern u8 *D_800D329C;
extern u8 *D_800C3EA4;
extern u8 *D_800C3EAC;
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

extern void BattleUpdateFrame(void);
extern void HeapFree(void *pMem);
extern void func_8008FC1C(s32, s32, s32, s32, u8);
extern s32 BattleDrawSpritePackage(s32, s32, s16, s16);
extern void BattleSetPolyFT4RectH13(POLY_FT4 *p, s16 x, s16 y, u8 u, u8 v, s16 dx);
extern void BattleLoadImageAndSync(RECT *pRect, u_long *pData);
extern void *BattleAllocTextBuffer(s32);
extern void bzero(void *p, s32 n);
extern u8 *GetStringEntry(u8 *, s32);
extern s32 SystemRenderStringEntry(u8 *, void *, s32, s32);
extern u8 D_800C3D70[];
extern void *BattleAllocHeapBlock(s32, s32);
extern void BattleInitSlotPrims(s32);
extern void func_8008F6E4(u8, s32, s32, s32, s32);
extern void BattleDrawSpriteQuad(u8, s32, s32, s32, u16);
extern void func_8008DE04(u8, s32, s32, s32);
extern void func_8008E430(u8, s32, s32, s32, u16);
extern void func_8008EA70(u8, s32, s32, s32);
extern void func_8008F0A8(u8, s32, s32, s32, u16);
extern void func_80090310(u8, u8);
extern void func_800904A0(s32, s32);
extern void BattleDrawMenuEntryString(u8, u8);
extern void *D_800D2F5C;
extern u8 D_800C3E4C;
extern void BattleSetPrimGrey80BlendModeFromGlobal(void *);
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
extern s32 BattleMaskHasStatusBit(s32, u8);
extern u8 func_80085084(u16, u8, s32);
extern void BattleClearPrimFlagsThreeSlots(u8);
extern void BattleSetPrimFlagsThreeSlots(u8);
extern void BattlePlaySoundIfEnabled(u8);
extern void BattleSetPolyFT4Rect(POLY_FT4 *, s16, s16, u8, u8, s16, s16);

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
        (u16)BattleMaskHasStatusBit((u16)g_GameState.unk16BC[D_800D2D24[arg0]].unk1E, arg1) != 0 &&
        (*(u16 *)(D_800CCD62 + ofs) & hdr[arg1]) == 0) {
        BattleClearPrimFlagsThreeSlots(1);
        D_800D2D28[0xC6] = 1;
        if (func_80085084(dmg, arg0, 0) != 0) {
            *(u16 *)(D_800CCDC4 + ofs) -= trigger;
            ret = 1;
        } else {
            D_800D2D28[0xC6] = 0;
            BattleSetPrimFlagsThreeSlots(arg0);
        }
        flag = 0;
    }
    if (flag != 0) {
        BattlePlaySoundIfEnabled(0x4F);
    }
    return ret;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008CFB8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008D328);
/* 93.85% (was 57.0 with a count local and 66.0 with an s32 k local): the loop
   shapes, both guards, the two SetX calls and the three byte stores all match.
   What is left: gcc CSEs 'D_800C3EA4 + j * 0xFA0' as one address base across the
   |-0x20 store and the following byte load (the original reloads the pointer and
   re-adds it), and the inner loop's back-branch delay slot takes the 'li a1,1'
   of the SetSemiTrans call (the original has that constant as the first insn of
   the body and a nop in the slot). Tried: count local plus two continue guards
   (57.0), s32 k local (66.0), the offset sum before the pointer, and three
   declaration orders. The count must stay a live re-read of the global in the
   for-condition - that is what makes the original reload it at the loop tail.
void func_8008D328(void) {
    u8 found;
    s32 j;
    s32 i;

    found = 0;
    for (j = 0; j < 2; j++) {
        if (*(s32 *)(D_800D2D28 + j * 4 + 0xD0) != 0) {
            for (i = 0; i < *(s32 *)(D_800D2D28 + j * 4 + 0xD0) * 2; i += 2) {
                u8 c;

                SetSemiTrans(D_800C3EA4 + 0x641C + j * 0xFA0 + (i + D_800D2D28[0xA3]) * 0x28, 1);
                SetShadeTex(D_800C3EA4 + 0x641C + j * 0xFA0 + (i + D_800D2D28[0xA3]) * 0x28, 0);
                *(u16 *)(D_800C3EA4 + j * 0xFA0 + 0x6432 + (i + D_800D2D28[0xA3]) * 0x28) |= 0x20;
                c = *(u8 *)(D_800C3EA4 + j * 0xFA0 + 0x6420 + (i + D_800D2D28[0xA3]) * 0x28);
                if (c != 0) {
                    found = 1;
                    *(u8 *)(D_800C3EA4 + j * 0xFA0 + 0x6420 + (i + D_800D2D28[0xA3]) * 0x28) = c - (*(u8 *)(D_800C3EAC + 0x2E0) << 4);
                    *(u8 *)(D_800C3EA4 + j * 0xFA0 + 0x6421 + (i + D_800D2D28[0xA3]) * 0x28) = c - (*(u8 *)(D_800C3EAC + 0x2E0) << 4);
                    *(u8 *)(D_800C3EA4 + j * 0xFA0 + 0x6422 + (i + D_800D2D28[0xA3]) * 0x28) = c - (*(u8 *)(D_800C3EAC + 0x2E0) << 4);
                }
            }
        }
    }
    if (found == 0) {
        D_800D2D28[0xCB] = 0;
    }
}

*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008D598);
void BattleDrawSpriteQuad(u8 arg0, s32 arg1, s32 arg2, s32 arg3, u16 arg4) {
    u8 codes[4];
    u8 *p;
    s16 dx;
    s16 dy;
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
    *(s32 *)(p + 0x5A0) = BattleDrawSpritePackage(codes[0], (s32)(p + *(s32 *)(p + 0x5A0) * 0x50), (s16)arg1, (s16)arg2) + *(s32 *)(p + 0x5A0);
    dy = arg2; // the sum must go through dy: dy + arg4 keeps retail's addu operand order
    dx = arg1 + arg3 - 8;
    *(s32 *)(p + 0x5A0) = BattleDrawSpritePackage(codes[1], (s32)(p + *(s32 *)(p + 0x5A0) * 0x50), dx, (s16)dy) + *(s32 *)(p + 0x5A0);
    dy = (s16)(dy + arg4 - 8);
    *(s32 *)(p + 0x5A0) = BattleDrawSpritePackage(codes[2], (s32)(p + *(s32 *)(p + 0x5A0) * 0x50), (s16)arg1, dy) + *(s32 *)(p + 0x5A0);
    *(s32 *)(p + 0x5A0) = BattleDrawSpritePackage(codes[3], (s32)(p + *(s32 *)(p + 0x5A0) * 0x50), dx, dy) + *(s32 *)(p + 0x5A0);
    for (i = 0; i < 4; i++) {
        BattleSetPrimGrey80BlendModeFromGlobal(p + (i * 2 + D_800CCB34) * 0x28);
    }
}

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
    BattleDrawSpriteQuad(arg0, arg1, arg2, arg3, arg4);
    func_8008DE04(arg0, arg1, arg2, arg3);
    func_8008E430(arg0, arg1, arg2, arg3, arg4);
    func_8008EA70(arg0, arg1, arg2, arg4);
    func_8008F0A8(arg0, arg1, arg2, arg3, arg4);
    *(u8 *)(D_800D2E38[arg0] + 0x5A4) = *(u8 *)&D_800CCB34;
    D_800D2D28[arg0 + 0xB0] = 1;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008F8F4);
/* 96.44%: whole body matches; only the prologue save/copy order differs (gcc
   gives s4 to arg5 and s5 to arg1 and copies arg0 last; the original has
   s4=arg1, s5=arg5 and copies arg0 first). Tried: separate pointer temps per
   block, a pFlags temp for D_800D2D28 (that load has to sit before the sh
   stores or gcc moves it after them), `*(base + index + K)` instead of
   `base[index + K]` (only the former gives the base-first addu), inverted
   if/else, and two declaration orders. Worth one more try by someone who can
   read gcc's allocno order.
   Also tried later: a live u16 copy of arg1 used at both sites -- same 96.44,
   gcc still gives arg5 the s4 home and pushes arg0's copy-later pair further
   down, so the copy does not raise arg1's allocno priority.

void func_8008F8F4(s32 arg0, u16 arg1, u16 arg2, u16 arg3, u16 arg4, u8 arg5, u8 arg6) {
    u8 *pFlags;
    u8 index;
    u8 *p;
    u8 *q;

    index = arg0;
    if (*(D_800D2D28 + index + 0xB0) == 0) {
        q = BattleAllocHeapBlock(0x5A8, 0);
        D_800D2E38[index] = q;
        bzero(q, 0x5A8);
        q = BattleAllocHeapBlock(0xE, 0);
        D_800D2D90[index] = q;
        bzero(q, 0xE);
        BattleInitSlotPrims(index);
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
            BattleUpdateFrame();
        }
    }
}
*/
