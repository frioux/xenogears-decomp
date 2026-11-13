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
extern u8 D_800C3E8C;
extern u8 *D_800C3DDC;
extern u8 D_800D2E5F[];
extern u8 D_800D3718[];
extern u8 D_800D3720[];
extern u8 D_800D3726[];
extern u16 g_SystemPalette1;
extern u16 g_SystemPalette2;

extern s32 func_8008ABB8(s32, s32);
extern s32 func_800338D8(s32);
extern u16 func_80098AF8(s32, s32);
extern void func_80077074(void);
extern void HeapFree(void *pMem);
extern u8 *GetStringEntry(u8 *, s32);
extern s32 SystemRenderStringEntry(u8 *, void *, s32, s32);
extern void func_800769E8(RECT *, u_long *);
extern void bzero(void *p, s32 n);
extern void func_8008F8F4(s32 arg0, u16 arg1, u16 arg2, u16 arg3, u16 arg4, u8 arg5, u8 arg6);
extern void *func_8008AC00(s32 arg0);
extern u8 D_800D36C0[];
extern u8 D_800D36C8[];
extern u8 *D_800D2E38[];
extern u8 D_800594D4[];
extern u8 D_800D2F3C[];
extern u8 D_800D2F3D[];
extern u8 D_800D2F3E[];
extern u8 D_800D2F3F[];
extern u8 D_800D2F68[];
extern u8 D_800D2F69[];
extern u8 *D_800C3E5C[];


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
/* 92.31%: the do-while below reproduces retail's loop tail exactly
   (`addiu s0,s0,0x28; addiu v0,s2,0x50; slt; bnez`, signed, no pre-loop
   guard) and the whole switch. The only difference is gcc's combine_givs
   re-basing the induction onto the address of the last memory reference:
   mine keeps p (calls + compare) in one register AND p+0xE (all the field
   accesses, displacements -0xA/-9/-8/0/8) in another, so it saves a 6th
   register (frame 0x30 vs 0x28). Retail keeps one register (p) with literal
   displacements 4/5/6/0xE/0x16. Tried: POLY_FT4* p++ with p < pInfo+2
   (83.14, also sltu + hoisted guard), u8* for-loop (83.14), do-while on u8*
   (92.31, this one), s32 cursor (85.66). Not found: the shape that stops the
   re-base.
   Parked body (92.31):
void func_80076D58(u8 *pInfo, u8 flag, u8 mode) {
    u8 *p;

    p = pInfo;
    do {
        SetPolyFT4(p);
        p[4] = 0x80;
        p[5] = 0x80;
        p[6] = 0x80;
        SetSemiTrans(p, 0);
        SetShadeTex(p, 1);
        switch (mode) {
        case 0:
            *(u16 *)(p + 0x16) = GetTPage(0, 0, 0x380, 0);
            break;
        case 1:
            *(u16 *)(p + 0x16) = GetTPage(0, 0, 0x380, 0x100);
            break;
        case 2:
            *(u16 *)(p + 0x16) = GetTPage(0, 0, 0x3C0, 0x100);
            break;
        case 3:
            *(u16 *)(p + 0x16) = GetTPage(0, 0, 0x3C0, 0);
            break;
        }
        if (flag != 0) {
            *(u16 *)(p + 0xE) = g_SystemPalette2;
        } else {
            *(u16 *)(p + 0xE) = g_SystemPalette1;
        }
        p += 0x28;
    } while ((s32)p < (s32)pInfo + 0x50);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_80076D58);
/* 88.70%: three shapes left. (1) retail keeps the pEntry base (la D_800D2F68
   + off) in a scratch register emitted BEFORE the D_800D2F68[off] `sb`, while
   gcc emits it after the store here (putting the assignment first makes gcc
   compute it before the call in a saved register instead). (2) `addu
   s0,s5,s0` (base first) vs this body's `addu s0,s0,s5`. (3) the third loop's
   `D_800C3E5C[i]` base: retail re-forms `lui at,%hi; addu at,at,s1; lw %lo(at)`
   every iteration, gcc hoists `la` out of the loop (an explicit `off += 4`
   byte-offset variable instead makes gcc emit a bogus 16-byte pointer stride,
   i.e. the combine_givs double-scale bug). Tried: pEntry assigned before the
   call (saved register, worse), `*(u8 **)(D_800C3E5C + off)` with off += 4
   (miscompiles), `*(u8 **)(D_800C3E5C + i * 4)`, rect.x as `0x3DE + i * 2`.
   Parked body (88.70):
void func_80076EA4(void) {
    u8 *pBuf[11];
    RECT rect;
    u8 **p;
    u8 *pEntry;
    s32 i;
    s32 off;
    s32 y;
    s32 h;

    y = 0x10D;
    h = 0xD;
    off = 0x2C;
    for (i = 0; i < 11; i++) {
        pBuf[i] = func_8008AC00(0x1B);
        bzero(pBuf[i], 0x30C);
        D_800D2F3C[off] = SystemRenderStringEntry(func_800338D8(i + 0xA), pBuf[i], 0x1B, 0);
        D_800D2F68[off] = SystemRenderStringEntry(func_800338D8(i + 0x15), pBuf[i], 0x1B, 1);
        pEntry = (u8 *)(D_800D2F68 + off);
        pEntry[2] = 0x78;
        D_800D2F3E[off] = 0x78;
        pEntry[3] = h;
        D_800D2F3F[off] = h;
        D_800D2F3D[off] = 0;
        D_800D2F69[off] = 1;
        rect.x = 0x3DE;
        rect.y = y;
        rect.w = 0x1E;
        rect.h = 0xD;
        func_800769E8(&rect, pBuf[i]);
        y += 0xD;
        h += 0xD;
        off += 4;
    }
    p = pBuf;
    for (i = 0; i < 11; i++) {
        HeapFree(*p++);
    }
    for (i = 0; i < 0xA; i++) {
        rect.x = 0x3DE + i * 2;
        rect.y = 0x100;
        rect.w = 6;
        rect.h = 0xD;
        func_800769E8(&rect, D_800C3E5C[i]);
    }
}
*/
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
void func_80077454(u8 index) {
    u8 *pBase;
    u8 i;

    pBase = D_800D2E38[index];
    for (i = 0; i < 2; i++) {
        SetPolyG4((POLY_G4 *)&pBase[i * 0x24 + 0x3C0]);
        pBase[i * 0x24 + 0x3C4] = D_800594D4[0];
        pBase[i * 0x24 + 0x3C5] = D_800594D4[1];
        pBase[i * 0x24 + 0x3C6] = D_800594D4[2];
        pBase[i * 0x24 + 0x3CC] = D_800594D4[0];
        pBase[i * 0x24 + 0x3CD] = D_800594D4[1];
        pBase[i * 0x24 + 0x3CE] = D_800594D4[2];
        pBase[i * 0x24 + 0x3D4] = D_800594D4[0];
        pBase[i * 0x24 + 0x3D5] = D_800594D4[1];
        pBase[i * 0x24 + 0x3D6] = D_800594D4[2];
        pBase[i * 0x24 + 0x3DC] = D_800594D4[0];
        pBase[i * 0x24 + 0x3DD] = D_800594D4[1];
        pBase[i * 0x24 + 0x3DE] = D_800594D4[2];
        SetSemiTrans((POLY_G4 *)&pBase[i * 0x24 + 0x3C0], 1);
        SetDrawMode((DR_MODE *)&pBase[i * 0xC + 0x408], 0, 0,
                    GetTPage(0, D_800595A0, *(s32 *)(D_800C3EA4 + 0xA25C),
                             *(s32 *)(D_800C3EA4 + 0xA260)),
                    (RECT *)D_800D2D28);
    }
    func_80077364((POLY_FT4 *)&pBase[0x140], 1);
    func_80077364((POLY_FT4 *)&pBase[0x1E0], 2);
    func_80077364((POLY_FT4 *)&pBase[0x280], 3);
    func_80077364((POLY_FT4 *)&pBase[0x320], 4);
}

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
/* 94.68%: structure, every call and every field offset is right, and both
   objects are the same size; only the three addresses that sit 0x60 above
   D_800D36C0 / D_800D3720 / D_800D3718 differ. Retail keeps ONE bare offset
   register (0x60 + 0xC0k) shared by all three uses - two `lui $at,%hi(sym);
   addu $at,$at,s3; sw %lo(sym)($at)` stores and one `addu v0,s3,s5` for the
   RECT copy destination - so a gcc that wanted that shape would need the
   0x60 in the register, not in a displacement. With i * 0x60 + 0x60 inline
   gcc turns the D_800D36C0 store into an absolute pointer giv (s3 =
   D_800D36C0 + 0x60, incremented) and folds the other two as displacements.
   Tried: explicit offset/offset2 locals incremented in the loop (89.73 - it
   does keep the $at shape for both stores, but the RECT destination then
   becomes a third pointer giv, 7 saved regs vs retail's 6); offset2 =
   offset + 0x60 inside the loop (90.42); u8 *pBase local as the base
   (89.68); (s32) cast sum, index-first (89.73); RECT destination as
   ((u8 *)pRect) + 0x60 (90.54).
   Parked body (94.68):
void func_8007819C(void) {
    RECT *pRect;
    u8 *pBuf;
    s32 i;

    func_8008F8F4(5, 8, 0x2A, 0x70, 0x12, 0, 0);
    D_800D2D28[0xB5] = 0;
    func_8008F8F4(4, 0x20, 0xC8, 0xF4, 0x12, 0, 0);
    D_800D2D28[0xB4] = 0;
    for (i = 0; i < 8; i += 2) {
        pBuf = func_8008AC00(0x39);
        *(u8 **)(D_800D36C0 + i * 0x60 + 0x60) = pBuf;
        *(u8 **)(D_800D3720 + i * 0x60 + 0x60) = pBuf;
        pRect = (RECT *)(D_800D3718 + i * 0x60);
        pRect->x = 0x3C0;
        pRect->y = (i / 2) * 13;
        pRect->w = 0x3C;
        pRect->h = 0xD;
        *(RECT *)(D_800D3718 + i * 0x60 + 0x60) = *pRect;
        func_800780A8(D_800D3718 - 0x50 + i * 0x60, i);
        func_800780A8(D_800D3718 + i * 0x60 + 0x10, i + 1);
    }
}
*/
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

/* 99.94%: the entire instruction stream is identical except the stack frame:
   retail allocates 0x18 and saves s0/ra at 0x10/0x14, this body allocates
   0x20 and saves them at 0x18/0x1C, i.e. gcc reserves 8 extra bytes of local
   space for something that never gets used (both objects are 392 bytes, so
   the frame is the only difference). Tried: pStr temp for GetStringEntry
   (same), inlining GetStringEntry (same), splitting `D_800C3E8C++` into a
   load + separate increment (87.44 - breaks the store/increment order).
   The D_800D3718/D_800D3720/D_800D3726 arrays are one struct of 0x60
   (RECT rect @0, u_long *pBuf @8, u8 @0xE, POLY_FT4 prim[2] @0x10); the
   spellings below use byte offsets instead of that struct so no shared
   header had to change.
   Parked body (99.94):
   void func_80078658(u8 index, u8 color) {
       u8 *pEntry;

       if (D_800C3E8C < 9) {
           *(u8 *)((u8 *)&D_800D3726 + D_800C3E8C * 0x60) =
               SystemRenderStringEntry(GetStringEntry(D_800C3DDC, D_800D2E5F[index * 8]),
                                       *(u8 **)((u8 *)&D_800D3720 + D_800C3E8C * 0x60), 0x39,
                                       D_800C3E8C & 1);
           func_800769E8((RECT *)((u8 *)&D_800D3718 + D_800C3E8C * 0x60),
                         *(u_long **)((u8 *)&D_800D3720 + D_800C3E8C * 0x60));
           D_800C400B[D_800C3EAC[0x2DA] * 0x48] = color;
           D_800C402F[D_800C3EAC[0x2DA] * 0x48] = 0xFA;
           pEntry = D_800C3EAC;
           *(u16 *)(D_800C4022 + pEntry[0x2DA] * 0x48) = D_800C3E8C++;
           pEntry[0x2DA]++;
       }
   }
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_80078658);
void func_800787E0(u8 color, u8 flag) {
    u8 *pEntry;

    D_800C400B[D_800C3EAC[0x2DA] * 0x48] = flag;
    D_800C402F[D_800C3EAC[0x2DA] * 0x48] = 0xF7;
    pEntry = D_800C3EAC;
    *(u16 *)(D_800C4022 + pEntry[0x2DA] * 0x48) = color;
    pEntry[0x2DA]++;
}
