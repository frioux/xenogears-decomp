#include "common.h"
#include "psyq/libgpu.h"

extern u8 *D_800C3EA4;
extern u8 *D_800C3EAC;
extern u8 *D_800D2D28;
extern u8 D_800C400B[];
extern u8 D_800C4022[];
extern u8 D_800C402F[];
extern u8 D_800D2E60[];
extern u8 D_800D2E61[];
extern s32 D_800595A0;
extern u8 D_800D2DCC[];
extern u8 D_800D2DE4[];
extern u16 D_800D2DF0[];
extern u16 g_SystemPalette1;
extern u16 g_SystemPalette2;

extern s32 func_8008ABB8(s32, s32);
extern u16 func_80098AF8(s32, s32);
extern void func_80077074(void);
extern void HeapFree(void *pMem);
extern void bzero(void *p, s32 n);

void func_80076AC8(u8 *p) {
    SetSemiTrans(p, 1);
    SetShadeTex(p, 0);
}
void func_80076B00(u8 *p) {
    func_80076AC8(p);
    p[4] = 0x80;
    p[5] = 0x80;
    p[6] = 0x80;
    if (D_800595A0 != 0) {
        *(u16 *)(p + 0x16) |= 0x40;
    } else {
        *(u16 *)(p + 0x16) &= ~0x40;
    }
}
void func_80076B68(u8 *p) {
    func_80076AC8(p);
    p[4] = 0x80;
    p[5] = 0x80;
    p[6] = 0x80;
    *(u16 *)(p + 0x16) |= 0x20;
}

void func_80076BAC(u8 *p) {
    func_80076AC8(p);
    p[4] = 0x40;
    p[5] = 0x40;
    p[6] = 0x40;
    *(u16 *)(p + 0x16) |= 0x20;
}

void func_80076BF0(u8 *p) {
    func_80076AC8(p);
    p[4] = 0x80;
    p[5] = 0x80;
    p[6] = 0x80;
    *(u16 *)(p + 0x16) |= 0x40;
}

void func_80076C34(u8 *p) {
    func_80076AC8(p);
    p[4] = 0x40;
    p[5] = 0x40;
    p[6] = 0x40;
    *(u16 *)(p + 0x16) |= 0x40;
}

void func_80076C78(POLY_FT4 *p, s16 x, s16 y, u8 u, u8 v, u8 dx) {
    p->x0 = x;
    p->y0 = y;
    p->y1 = y;
    p->x2 = x;
    p->y2 = y + 13;
    p->y3 = y + 13;
    p->u0 = u;
    p->u2 = u;
    p->x1 = x + dx;
    p->x3 = x + dx;
    p->v0 = v;
    p->u1 = u + dx;
    p->v1 = v;
    p->v2 = v + 13;
    p->u3 = u + dx;
    p->v3 = v + 13;
}
void func_80076CE8(POLY_FT4 *p, s16 x, s16 y, u8 u, u8 v, s16 dx, s16 dy) {
    p->x0 = x;
    p->y0 = y;
    p->y1 = y;
    p->x2 = x;
    p->u0 = u;
    p->u2 = u;
    p->x1 = x + dx;
    p->y2 = y + dy;
    p->x3 = x + dx;
    p->y3 = y + dy;
    p->v0 = v;
    p->u1 = u + dx;
    p->v1 = v;
    p->v2 = v + dy;
    p->u3 = u + dx;
    p->v3 = v + dy;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_80076D58);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_80076EA4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_80077074);
void func_80077364(POLY_FT4 *p, u8 index) {
    s32 i;
    s32 offset;

    for (i = 0; i < 4; i++) {
        SetPolyFT4(&p[i]);
        SetShadeTex(&p[i], 1);
        p[i].r0 = 0xFF;
        p[i].g0 = 0xFF;
        p[i].b0 = 0xFF;
        offset = index * 0x18;
        p[i].tpage = GetTPage(*(s32 *)(D_800C3EA4 + offset + 0xA238), 0,
                              *(s32 *)(D_800C3EA4 + offset + 0xA244),
                              *(s32 *)(D_800C3EA4 + offset + 0xA248));
        p[i].clut = GetClut(*(s32 *)(D_800C3EA4 + offset + 0xA23C),
                            *(s32 *)(D_800C3EA4 + offset + 0xA240));
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_80077454);
void func_80077610(void) {
    u8 *p = (u8 *)func_8008ABB8(0x670, 0);

    *(u8 **)(D_800C3EA4 + 0xA230) = p;
    bzero(p, 0x670);
    func_80077074();
}

void func_8007765C(void) {
    func_800716D8();
    HeapFree(*(u8 **)(D_800C3EA4 + 0xA230));
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_80077698);
void func_80077980(void) {
    D_800D2D28[0xC6] = 0;
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_80077990);
/* 61.46%: retail walks two 0x28-byte packets from pInfo to pInfo+0x50 and
   finishes with pInfo[0x5D] = 0. What retail does that no attempt so far
   reproduces:
   1. its loop compare is `slt` (signed) against a register holding pInfo+0x50,
      and it re-forms that bound inside the loop (addiu s0,s0,0x28; addiu
      v0,s1,0x50; slt; bnez) with NO pre-loop guard. A POLY_FT4* compare (the
      body below) still gives `sltu` plus a hoisted `addiu v0,s2,0x50; sltu;
      beqz` guard, and so does a u8* compare, so the signed compare is not a
      pointer-vs-u8* question; it wants a signed integer cursor/limit.
   2. retail stores the flag and RE-READS it (andi v0,s3,1; sb v0,0x5C(s1);
      lbu v0,0x5C(s1); beqz) - every plain store-then-test spelling lets cse
      forward the store and compare the register instead.
   3. retail's GetTPage has a0=0 preloaded in the beqz delay slot and
      p->clut = tile lands in the jal's delay slot.
   Parked body (61.46):
void func_800780A8(u8 *pInfo, s32 arg) {
    POLY_FT4 *p;
    u16 clut;
    s32 offset;

    for (p = (POLY_FT4 *)pInfo; p < (POLY_FT4 *)(pInfo + 0x50); p++) {
        SetPolyFT4(p);
        p->r0 = 0x80;
        p->g0 = 0x80;
        p->b0 = 0x80;
        SetSemiTrans(p, 0);
        SetShadeTex(p, 1);
        pInfo[0x5C] = arg & 1;
        if (pInfo[0x5C] != 0) {
            clut = g_SystemPalette2;
        } else {
            clut = g_SystemPalette1;
        }
        p->clut = clut;
        offset = arg / 2;
        p->tpage = GetTPage(0, 0, 0x3C0, offset * 13);
    }
    pInfo[0x5D] = 0;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_800780A8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_8007819C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_80078310);
/* 99.80%: only the two `sh` in the then-branch are swapped (original stores
   [i+0xB] first, then [i]); the prologue pointer derivation, both loop
   increments and the whole else-branch are identical. The primary induction
   register is chosen from the giv of the then-branch's FIRST store, so making
   that store [i+0xB] (the original's order) turns the base into
   %lo(D_800D2DF0+0x16) and derives the other pointers from it, which costs
   3 addius. Tried: then [i],[i+0xB] / else [i+0xB],[i] (this one),
   both branches [i],[i+0xB], both [i+0xB],[i].
   The blocker is precise: gcc picks the base giv from the FIRST D_800D2DF0
   store in the loop body, so with all four stores [i+0xB]-first the prologue
   becomes `addiu v0,%lo+0x16` and needs a third addiu (91.04%); the original
   keeps base+0 while its two `sh` are [i+0xB]-first, i.e. store order and giv
   order disagree there. else-branch order does not influence it (99.57%).

   99.80% body (everything identical but the then-branch's two sh swapped):
void func_80078508(u8 *pOut) {
    s32 i;
    u16 value;

    for (i = 0; i < 11; i++) {
        if (D_800D2DCC[i] != 0) {
            value = func_80098AF8(i, 0);
            D_800D2DF0[i] = value;
            D_800D2DF0[i + 0xB] = value;
        } else {
            D_800D2DF0[i + 0xB] = 0xFF;
            D_800D2DF0[i] = 0xFF;
        }
        D_800D2DE4[i] = 0;
        D_800D2DF0[i + 0x16] = 0;
        pOut[i] = 0;
    }
}

   91.04% body (all four stores [i+0xB] first): insn stream identical to
   retail except gcc makes the add_val=22 giv the base, so the prologue is
   `addiu v0,%lo+0x16; addiu s2,v0,-0x16; addiu s3,v0,0x16` instead of
   `addiu v0,%lo; addiu s3,v0,0x2C; addiu s2,s1,0x16` and s1/s2 swap in the
   store and increment slots. Tried this session on top of the above: three
   explicit pointer givs (pLo/pHi/pEnd, 57.98 - base right but increments get
   hoisted above the branch join), `pDst = D_800D2DF0 + i` indexed (75.06,
   collapses to one base + displacements), declaration order of i/value,
   gcc 2.6.0 (91.04), 2.7.2-cdk (54.04), -O1 -G0 (35.08). The base-register
   pick is gcc's combine_givs; no source spelling found that keeps store order
   [i+0xB],[i] and base add_val 0.
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_80078508);

/* 75.45%: with a pEntry local (one load of the global shared by the two uses
   after the sb) the first store and everything from the `sh` tail on is
   byte-identical; the ONLY difference is that retail's two tile `lbu`s are
   scheduled BEFORE the sb while gcc puts them after it. Source order does not
   matter (tile first / tile last / an s32 offset local all compile identically),
   so it is gcc's scheduler placement. Also tried: no pEntry (three loads of the
   global, 48.18), inline tile.
   Parked body (75.45):
void func_800785D4(u8 color, u8 index) {
    u16 tile;
    u8 *pEntry;

    pEntry = D_800C3EAC;
    tile = D_800D2E60[index * 8] | (D_800D2E61[index * 8] << 8);
    D_800C400B[pEntry[0x2DA] * 0x48] = color;
    pEntry = D_800C3EAC;
    *(u16 *)(D_800C4022 + pEntry[0x2DA] * 0x48) = tile;
    pEntry[0x2DA]++;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_800785D4);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_80078658);
void func_800787E0(u8 color, u8 flag) {
    u8 *pEntry;

    D_800C400B[D_800C3EAC[0x2DA] * 0x48] = flag;
    D_800C402F[D_800C3EAC[0x2DA] * 0x48] = 0xF7;
    pEntry = D_800C3EAC;
    *(u16 *)(D_800C4022 + pEntry[0x2DA] * 0x48) = color;
    pEntry[0x2DA]++;
}
