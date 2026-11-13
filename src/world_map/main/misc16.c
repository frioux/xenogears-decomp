#include "common.h"
#include "psyq/libgte.h"

extern VECTOR D_8009BB4C;
extern VECTOR D_8009BB5C;
extern VECTOR D_8009BB6C;
extern VECTOR D_8009BB7C;
extern VECTOR D_8009BB8C;
extern VECTOR D_8009BB9C;
extern VECTOR D_8009C7F0;
extern VECTOR D_8009C828;
extern VECTOR D_8009C844;
extern VECTOR D_8009C874;

typedef struct {
    /* 0x0 */ s32 unk0;
    /* 0x4 */ s32 unk4;
    /* 0x8 */ s32 unk8;
} WorldMapMisc16Struct; // Size: 0xC

extern u16 D_8009D558;
extern s16 D_8009C838;
extern s16 D_8009C83C;

typedef struct {
    /* 0x0 */ s16 unk0;
    /* 0x2 */ s16 pad2;
} WorldMapMisc16TblEntry; // Size: 0x4

typedef struct {
    /* 0x0 */ u32 unk0;
    /* 0x4 */ s16 unk4;
    /* 0x6 */ u16 pad6;
} WorldMapMisc16Pkt; // Size: 0x8

typedef struct {
    /* 0x0 */ s16 unk0;
    /* 0x2 */ s16 pad2;
    /* 0x4 */ s16 unk4;
} WorldMapMisc16Rect; // Size: 0x6

typedef struct {
    /* 0x0 */ s16 grid[81];
} WorldMapMisc16Grid; // Size: 0xA2

extern WorldMapMisc16Grid D_8009D318;
extern WorldMapMisc16Grid D_8009D570;
extern s32 D_8009D160;
extern s32 D_8009D2B4;

extern WorldMapMisc16TblEntry D_800523F0[];
extern s32 D_8009C618;
extern s32 D_8009C5BC;

void func_80098044(void) {
    OuterProduct0(&D_8009BB6C, &D_8009BB4C, &D_8009C828);
    OuterProduct0(&D_8009BB4C, &D_8009BB7C, &D_8009C844);
    OuterProduct0(&D_8009BB8C, &D_8009BB5C, &D_8009C874);
    OuterProduct0(&D_8009BB5C, &D_8009BB9C, &D_8009C7F0);
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc16", func_800980D4);
/* 93.98%, body is 240 bytes like retail: only the register names of the 0x800000
   constant and of the y local are swapped (retail y=a2, const=a0; ours y=a0,
   const=a2), an allocator tie - 4 decl/init orders all give the same pairing.
   Shapes that are now right and must stay: x and y are BOTH locals loaded before
   the D_8009D558 store (that is the retail top: unk0 load, unk8 load, li, sh);
   the x branch writes p->unk0 = x + / - 0x800000 (a += reloads and emits a
   per-branch flag store); the y branch reads the y local but reloads p->unk8 for
   the store; the tail reloads through a y temp (y = p->unk0; D_8009C838 = (y >> 23) + 2;
   y = p->unk8; D_8009C83C = (y >> 23) + 2;) - the direct form instead lets gcc hoist
   the second load into the first load's delay slot and defer both stores (59 insns, 236 bytes).
void func_800980D4(WorldMapMisc16Struct* p) {
    s32 y;
    s32 x;
    x = p->unk0;
    y = p->unk8;
    D_8009D558 = 0;
    if (x < -0x800000) {
        p->unk0 = x + 0x800000;
        D_8009D558 = 4;
    } else if (x > 0x800000) {
        p->unk0 = x - 0x800000;
        D_8009D558 = 8;
    }
    if (y < -0x800000) {
        p->unk8 += 0x800000;
        D_8009D558 |= 1;
    } else if (y > 0x800000) {
        p->unk8 -= 0x800000;
        D_8009D558 |= 2;
    }
    y = p->unk0;
    D_8009C838 = (y >> 23) + 2;
    y = p->unk8;
    D_8009C83C = (y >> 23) + 2;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc16", func_800981C8);
/* 96.44%: same instructions in the same order, only the register names differ
   (retail col=a3 row=t1 w=t2 dst=t0 bound=t3 colBase=t3 pGrid=a0 colCur=a3 off=a1;
   ours col=t1 row=t0 w=t3 dst=a3 bound=t2 colBase=a0 pGrid=a2 colCur=v1 off=a3), so
   objdiff can never reach 100 here without snapping gcc's allocator tie-break. Shapes
   that matter and are now right: w/h must be locals or the in-loop loads are not hoisted;
   `for (i = 8; i != -1; i--)` both loops; the if/else-with-value form for the inner clamp
   (that is what duplicates `addu v0,off,colCur` into the delay slot and after r=0);
   row >>= 8 before the 0xA2-byte struct copy, colBase = col after it.
void func_800981C8(WorldMapMisc16Struct* p) {
    s32 col;
    s32 row;
    s32 colBase;
    s32 rowOff;
    s32 colCur;
    s32 i;
    s32 j;
    s32 w;
    s32 h;
    s16* pGrid;

    w = D_8009D160;
    h = D_8009D2B4;
    col = (p->unk0 >> 12) / 8 - ((D_8009C838 + 2) << 8);
    row = (p->unk8 >> 12) / 8 - ((D_8009C83C + 2) << 8);
    if (col < 0) {
        col += w << 8;
    } else if ((w << 8) < col) {
        col -= w << 8;
    }
    if (row < 0) {
        row += h << 8;
    } else if ((h << 8) < row) {
        row -= h << 8;
    }
    col >>= 8;
    row >>= 8;
    D_8009D318 = D_8009D570;
    colBase = col;
    pGrid = D_8009D570.grid;
    for (i = 8; i != -1; i--) {
        if (!(row < h)) {
            row = 0;
        }
        colCur = colBase;
        rowOff = row * w;
        for (j = 8; j != -1; j--) {
            if (colCur >= w) {
                colCur = 0;
            }
            *pGrid++ = rowOff + colCur;
            colCur++;
        }
        row++;
    }
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc16", func_800983A0);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc16", func_800987AC);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc16", func_80098CC0);
typedef struct {
    /* 0x0 */ u16 unk0;
    /* 0x2 */ u16 unk2;
    /* 0x4 */ u32 unk4;
} WorldMapMisc16Tex; // Size: 0x8

extern WorldMapMisc16Tex D_8009D650[];
extern WorldMapMisc16Tex D_8009D654[];
extern s16 D_8009D618[];
extern u8* D_8009C184[];
extern u16 D_8009CCB4[];
extern u16 D_8009CD54[];
extern MATRIX D_8009D534;
extern MATRIX D_8009C808;
extern s32 D_8009D7DC;
extern void func_80099708(u8* pSrc, u8* p1, u8* p2, WorldMapMisc16Rect* pRect);

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc16", func_8009932C);
/* 63.53% first attempt. Structure read off the asm and believed right: two u16 copy
   loops from D_8009CCB4 (0x40) and D_8009CD54 (7) into scratchpad 0x1F800288/0x1F800308,
   the 0x20-byte MATRIX copy of D_8009D534 to 0x1F800350, CompMatrix/SetRotMatrix/
   SetTransMatrix on 0x1F800370, the 5x5 tile loop (row/fp, col/s4, flat k/s7 whose *8
   biv is retail's s3), the four RECT corners built at 0x1F800330/0x338 from the volatile
   0x1F800328/0x1F80032C coords, and the four func_80099708 calls selected by
   (s16)(D_8009D650[k].unk0 | .unk2 | D_8009D654[k].unk0 | .unk2) == -1. Scratchpad
   accesses MUST stay volatile: retail reloads 0x1F800320 right after storing it and
   loads 0x1F800328 twice per iteration, which cse only allows through volatile.
   What is wrong: allocation only. Retail col=s4 k=s2? no - retail has base=s2, col=s4,
   row=s8, k=s7 and frame 0x38 (10 saves); ours puts base in s6, col in s5, spills to a
   0x40 frame (11 slots) and reorders the k++/col++ epilogue. 6 shapes not tried yet.
   Next: try col/k as a single flat counter, and read D_8009D650[k] into locals before
   the coords so the OR chain does not force the base register.
void func_8009932C(u8* p0, u8* p1, WorldMapMisc16Struct* p) {
    s32 i;
    s32 row;
    s32 col;
    s32 k;
    s16 x;
    s16 y;
    s16 all;
    u8* pImg;

    for (i = 0; i < 0x40; i++) {
        ((u16*)0x1F800288)[i] = D_8009CCB4[i];
    }
    for (i = 0; i < 7; i++) {
        ((u16*)0x1F800308)[i] = D_8009CD54[i];
    }
    *(MATRIX*)0x1F800350 = D_8009D534;
    CompMatrix(&D_8009C808, (MATRIX*)0x1F800350, (MATRIX*)0x1F800370);
    SetRotMatrix((MATRIX*)0x1F800370);
    SetTransMatrix((MATRIX*)0x1F800370);
    *(s32*)0x1F800318 = -((p->unk0 >> 12) & 0x7FF) - 0x1000;
    *(s32*)0x1F800320 = -((p->unk8 >> 12) & 0x7FF) - 0x1000;
    *(volatile u16*)0x1F80032C = -*(s32*)0x1F800320;
    D_8009D7DC = 0;
    k = 0;
    for (row = 0; row < 5; row++) {
        *(volatile u16*)0x1F800328 = *(s32*)0x1F800318;
        for (col = 0; col < 5; col++, k++) {
            if (D_8009D618[k] == -1) {
                continue;
            }
            y = *(volatile u16*)0x1F80032C;
            x = *(volatile u16*)0x1F800328;
            pImg = D_8009C184[D_8009D570.grid[(row + D_8009C83C) * 9 + col + D_8009C838]];
            *(volatile u16*)0x1F800330 = *(volatile u16*)0x1F800340 = *(volatile u16*)0x1F800328 + 0x400;
            *(volatile u16*)0x1F800334 = y;
            *(volatile u16*)0x1F800338 = x;
            *(volatile u16*)0x1F80033C = *(volatile u16*)0x1F800344 = *(volatile u16*)0x1F80032C - 0x400;
            all = D_8009D650[k].unk0 | D_8009D650[k].unk2 | D_8009D654[k].unk0 | D_8009D654[k].unk2;
            if (all != -1) {
                func_80099708(pImg, p0, p1 + D_8009D7DC * 0x20, (WorldMapMisc16Rect*)0x1F800328);
                func_80099708(pImg + 0x144, p0, p1 + D_8009D7DC * 0x20, (WorldMapMisc16Rect*)0x1F800330);
                func_80099708(pImg + 0x288, p0, p1 + D_8009D7DC * 0x20, (WorldMapMisc16Rect*)0x1F800338);
                func_80099708(pImg + 0x3CC, p0, p1 + D_8009D7DC * 0x20, (WorldMapMisc16Rect*)0x1F800340);
            } else {
                if (y != all) {
                    func_80099708(pImg, p0, p1 + D_8009D7DC * 0x20, (WorldMapMisc16Rect*)0x1F800328);
                }
                if (D_8009D650[k].unk2 != all) {
                    func_80099708(pImg + 0x144, p0, p1 + D_8009D7DC * 0x20, (WorldMapMisc16Rect*)0x1F800330);
                }
                if (D_8009D654[k].unk0 != all) {
                    func_80099708(pImg + 0x288, p0, p1 + D_8009D7DC * 0x20, (WorldMapMisc16Rect*)0x1F800338);
                }
                if (D_8009D654[k].unk2 != all) {
                    func_80099708(pImg + 0x3CC, p0, p1 + D_8009D7DC * 0x20, (WorldMapMisc16Rect*)0x1F800340);
                }
            }
        }
        *(volatile u16*)0x1F800328 += 0x800;
        *(volatile u16*)0x1F80032C -= 0x800;
    }
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc16", func_80099708);
/* 78.20%: loop shape and instruction multiset now match retail (`for (i = 8; i != -1; i--)`
   gives the addiu/-1 countdown; the if/else assigns the *already <<16-shifted* value in each
   branch, which is what puts retail's sll in the `j` delay slot and folds the else path to
   `sll 24; sra 5`). What is left is a global register-allocation rotation: retail keeps
   pSrc in s0 (frame 0x20, saves s0+s1) and the mult temp in s1, and uses a0=x / a3=xCur /
   v1=v, while our compile keeps pSrc in a0 (frame 0x18) and puts idx/ii in a2/a3. 6 source
   shapes (incl. named product local) all score 78.20, and cdk is also 78.2, so it is not
   the preset. Retail's `addiu t0,t0,%lo(D_1F800008)` is splat naming the +8 of pPkt++, i.e.
   a reloc the C cannot emit; objdiff can never reach 100 here even if the allocation did.
void func_80099708(u32* pSrc, s32 unk1, s32 unk2, WorldMapMisc16Rect* pRect) {
    WorldMapMisc16Pkt* pPkt;
    WorldMapMisc16TblEntry* pTbl;
    u32* p;
    s32 x;
    s32 y;
    s32 xCur;
    s32 col;
    s32 iCol;
    s32 idx;
    s32 oi;
    s32 ii;
    u32 v;
    s32 c;

    pPkt = (WorldMapMisc16Pkt*)0x1F800000;
    pTbl = D_800523F0;
    p = pSrc;
    x = pRect->unk0;
    y = pRect->unk4;
    col = D_8009C618;
    for (oi = 8; oi != -1; oi--) {
        xCur = x;
        iCol = pTbl[col & 0xFFF].unk0 * 2;
        idx = D_8009C5BC;
        for (ii = 8; ii != -1; ii--) {
            v = *p;
            if (v & 0x1000) {
                c = (((pTbl[idx & 0xFFF].unk0 * iCol) >> 20) + ((s8)v << 3)) << 16;
            } else {
                c = (s8)v << 19;
            }
            pPkt->unk0 = c | (xCur & 0xFFFF);
            pPkt->unk4 = y;
            xCur += 0x80;
            pPkt++;
            p++;
            idx += 0x200;
        }
        y -= 0x80;
        col += 0x200;
    }
    func_8009980C(pSrc);
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc16", func_8009980C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc16", func_80099BFC);
