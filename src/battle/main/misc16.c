#include "common.h"
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

extern void func_800716D8(void);
extern void HeapFree(void *pMem);
extern void func_8008FC1C(s32, s32, s32, s32, s32);
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
extern void func_8008F6E4(s32, s32, s32, s32, s32);
extern void func_80090310(u8, u8);
extern void func_800904A0(s32, s32);
extern void func_8009070C(s32, s32);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008CDE4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008CFB8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008D328);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008D598);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008DC34);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008DE04);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008E430);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008EA70);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008F0A8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008F6E4);
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
void func_8008FA60(u8 arg0) {
    u8 *p;

    p = D_800D2D28 + arg0;
    p[0xB0] = 0;
    p = D_800D2D28 + arg0;
    p[0xB8] = 0;
    func_800716D8();
    HeapFree(D_800D2E38[arg0]);
    HeapFree(D_800D2D90[arg0]);
}
/* 87.51%: the four halfword fields, the clamping, the count==2 flag and the
   func_8008F6E4 call are identified; gcc lays the clamp arms out the other way
   round (it emits beqz with the then-arm inline where the original has bnez
   with the else-arm inline) and folds the size temp away. Tried: a u16 size
   temp per axis, both condition polarities, `+=` vs store-at-join, and
   `*(base + i + K)` for the 0xBF flag store.

void func_8008FAD8(void) {
    s32 i;
    u8 *p;
    u8 *pFlags;
    u8 count;
    u16 x;
    u16 y;

    for (i = 0; i < 7; i++) {
        p = D_800D2D90[i];
        pFlags = D_800D2D28 + i;
        if (pFlags[0xB8] == 0) {
            continue;
        }
        if (pFlags[0xBF] != 0) {
            continue;
        }
        count = 0;
        if (((u16 *)p)[4] + 0x20 < ((u16 *)p)[2]) {
            ((u16 *)p)[4] += 0x20;
        } else {
            ((u16 *)p)[4] = ((u16 *)p)[2];
            count++;
        }
        if (((u16 *)p)[5] + 0x20 < ((u16 *)p)[3]) {
            ((u16 *)p)[5] += 0x20;
        } else {
            ((u16 *)p)[5] = ((u16 *)p)[3];
            count++;
        }
        if (count == 2) {
            D_800D2D28[i + 0xBF] = 1;
        }
        x = ((u16 *)p)[0] + (((u16 *)p)[2] >> 1) - (((u16 *)p)[4] >> 1);
        y = ((u16 *)p)[1] + (((u16 *)p)[3] >> 1) - (((u16 *)p)[5] >> 1);
        func_8008F6E4(p[0xC], x, y, ((u16 *)p)[4], ((u16 *)p)[5]);
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008FAD8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008FC1C);
void func_8008FDE4(void) {
    func_8008FC1C(0x20, 0x5C, 0xCC, 0x60, 0xE);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008FE18);
/* 99.62%: two commutative operand orders. The original loads the D_800C3EA4
   index part first (sll/addu/sll) then `addiu a1,a1,0xBA8` then `addu a1,a1,v0`
   with the pointer base; gcc either reverses the final addu (pointer base, as
   below) or, with the base cast to s32, folds 0xBA8 onto the base side
   (`addiu v0,v0,0xBA8; addu a1,a1,v0`). For the 4th argument gcc moves the -4
   onto the loop induction variable (`addiu a3,s0,-4; addu a3,a3,s2`) whichever
   way the sum is written, because fold attaches constants to the GIV; the
   original has `addiu a3,s2,-4; addu a3,s0,a3`. Tried: arg0-4+offset,
   (arg0-4)+offset, offset+arg0-4, offset+(arg0-4), base-first and
   index-first with and without an (s32) cast on the base, and a `size` temp for
   the block-size accumulate (that temp fixed the rest of the loop, 65 -> 99.62).
void func_8009023C(s32 arg0) {
    s32 i;
    s32 offset;
    s32 size;

    *(s32 *)(D_800D2D28 + 0xF8) = 0;
    i = 0;
    for (offset = 0x68; i < 9; i++) {
        size = func_80076A10(0x66, D_800C3EA4 + (*(s32 *)(D_800D2D28 + 0xF8) * 0x50 + 0xBA8), 0x20, arg0 - 4 + offset);
        *(s32 *)(D_800D2D28 + 0xF8) = size + *(s32 *)(D_800D2D28 + 0xF8);
        offset += 8;
    }
    D_800D2D28[0xA5] = *(u8 *)&D_800CCB34;
    D_800D2D28[0x9C] = 1;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8009023C);
/* 50.34%: signature and inner arithmetic recovered (u8,u8; index = arg1*2+arg0;
   if (index >= 0x11) index -= 0x10 and the palette/UV base switches; the store
   is *(u16 *)(P + D_800CCB34*0x28 + 0x14E) = palette with
   P = *(u8 **)(D_800C3EA4 + 0xA230); u = (index % 2) * 0x78, v = (index / 2) * 13;
   two func_80076C78 calls at P + D_800CCB34*0x28 + 0x140 (0x18,0x33,...,0x60) and
   + 0x190 (0x84,0x33,((index%2)<<6)|ofs,...,0x10), and the andi 0xF8/0xD0 masks
   are gcc's range-narrowed u8 argument truncation, not source masks).
   The gap is structural: the original hoists the D_800CCB34 and D_800C3EA4 loads
   of the store address INTO both if-arms (cse.c load hoisting) and keeps `index`
   in a1-a3, giving a 0x30 frame with s0-s3; gcc keeps the loads after the join,
   puts index in s1 and emits a 0x28 frame with three saves. Tried: if/else vs
   pre-assigning g_SystemPalette1 before the if.
   Next idea: force the hoisting by making the store address a local pointer
   computed before the if, or by duplicating the store into both arms. */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_80090310);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_800904A0);
/* 78.72%: RECT setup, bzero/GetStringEntry/SystemRenderStringEntry order and the
   stack args all match; the poly pointer is the problem - gcc loads the
   D_800CCB34 index first and sinks the final `addiu a0,a0,0x280` into the jal
   delay slot, where the original loads *(D_800C3EA4+0xA230) first and does the
   addiu before the base addu. Tried: base-first and index-first grouping of
   the address.
void func_8009070C(s32 arg0, s32 arg1) {
    RECT rect;
    u8 *pBuf;
    s32 index;
    s32 len;

    index = D_800D2CE0[arg1 * 2 + arg0];
    pBuf = (u8 *)func_8008AC00(0x39);
    bzero(pBuf, 0x618);
    len = SystemRenderStringEntry(GetStringEntry(D_800D329C, index), pBuf, 0x39, 0);
    rect.x = 0x3C0;
    rect.y = 0;
    rect.w = 0x3C;
    rect.h = 0xD;
    func_800769E8(&rect, pBuf);
    func_80076C78((POLY_FT4 *)((*(u8 **)(D_800C3EA4 + 0xA230)) + D_800CCB34 * 0x28 + 0x280), 0x30, 0x42, 0, 0, len & 0xFF);
    HeapFree(pBuf);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8009070C);
/* 99.87%: one register. The 0x66D store loads its pointer into v1 where the
   original uses v0; everything else matches. Tried: p/flag declaration order.
   NOTE arg0..arg2 are u8: that is what produces the per-branch andi of
   arg0/arg1 for the table index and the andi of arg2 twice.

void func_8009080C(u8 arg0, u8 arg1, u8 arg2) {
    u8 flag;
    u8 *p;

    if (arg2 == 0) {
        flag = D_800D2CE0[arg1 * 2 + arg0];
    } else {
        flag = D_800C3D70[arg1 * 2 + arg0];
    }
    p = *(u8 **)(D_800C3EA4 + 0xA230);
    p[0x66D] = 0;
    func_80090310(arg0, arg1);
    if (flag != 0 && arg2 == 0) {
        func_8009070C(arg0, arg1);
        func_800904A0(arg0, arg1);
        p = *(u8 **)(D_800C3EA4 + 0xA230);
        p[0x66D] = 1;
    }
    p = *(u8 **)(D_800C3EA4 + 0xA230);
    p[0x66A] = *(u8 *)&D_800CCB34;
    p = *(u8 **)(D_800C3EA4 + 0xA230);
    p[0x66B] = 1;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8009080C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8009093C);
void func_80090B90(s32 arg0, s32 arg1, s32 *pIndex, u8 *pCount) {
    u8 *p;

    if (++*pCount >= 3) {
        if (--*pIndex < 0) {
            *pIndex = 4;
        }
        *pCount = 0;
    }
    *(s32 *)(D_800D2D28 + 0x100) = func_80076A10(*pIndex + 0xE0, D_800C3EA4 + 0x27C8, arg0, arg1);
    p = D_800D2D28;
    p[0xA7] = *(u8 *)&D_800CCB34;
    *(volatile u8 *)(D_800D2D28 + 0x9E) = 1;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_80090C44);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_80090E7C);
