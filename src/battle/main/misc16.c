#include "common.h"
#include "psyq/libgpu.h"

extern u8 D_800D2CE0[];
extern u8 *D_800D329C;
extern u8 *D_800C3EA4;
extern s32 D_800CCB34;
extern u8 *D_800D2D28;
extern u8 *D_800D2E38[];
extern u8 *D_800D2D90[];

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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008FAD8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008FC1C);
void func_8008FDE4(void) {
    func_8008FC1C(0x20, 0x5C, 0xCC, 0x60, 0xE);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8008FE18);
/* 99.62%: only two commutative operand orders differ. gcc attaches the -4 to
   the loop induction variable (addiu a3,s0,-4) where the original attaches it
   to the argument (addiu a3,s2,-4), and emits `addu a1,v0,a1` where the
   original has `addu a1,a1,v0`. Tried: arg0 - 4 + offset, (arg0 - 4) + offset,
   offset + arg0 - 4, offset + (arg0 - 4), base+offset and offset+base casts for
   the destination pointer, and a `size` temp for the block-size accumulate
   (that temp is what fixed the rest of the loop body from 65%).
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8009080C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_8009093C);
/* 95.11%: identical instruction multiset; the only difference is that gcc sinks
   the final `sb v0,0x9E(v1)` past the `lw ra`/`addiu sp` epilogue where the
   original keeps it before them. Tried: direct byte stores, a hoisted u8 *p
   local for both tail stores, `*(u8 *)&D_800CCB34` for the 0xA7 source.
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
    p = D_800D2D28;
    p[0x9E] = 1;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_80090B90);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_80090C44);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc16", func_80090E7C);
