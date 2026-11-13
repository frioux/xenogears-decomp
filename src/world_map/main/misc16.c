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
/* 93.90%
void func_800980D4(WorldMapMisc16Struct* p) {
    s32 y;
    D_8009D558 = 0;
    y = p->unk8;
    if (p->unk0 < -0x800000) {
        p->unk0 += 0x800000;
        D_8009D558 = 4;
    } else if (p->unk0 > 0x800000) {
        p->unk0 -= 0x800000;
        D_8009D558 = 8;
    }
    if (y < -0x800000) {
        p->unk8 += 0x800000;
        D_8009D558 |= 1;
    } else if (y > 0x800000) {
        p->unk8 -= 0x800000;
        D_8009D558 |= 2;
    }
    y = p->unk0 >> 23;
    D_8009C838 = y + 2;
    D_8009C83C = (p->unk8 >> 23) + 2;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc16", func_800981C8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc16", func_800983A0);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc16", func_800987AC);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc16", func_80098CC0);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc16", func_8009932C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc16", func_80099708);
/* 56.68%: shape read off the .s; our cc1 keeps the plain `bgez` loop iduction
   where retail has the countdown `addiu t7,t7,-1; bne t7,-1` form, so every
   register after that point is shifted (retail uses all of t0-t9, s1).
   -O3 and cdk score the same 56.68, so it is source-level, not the preset.
void func_80099708(u32* pSrc, s32 unk1, s32 unk2, WorldMapMisc16Rect* pRect) {
    WorldMapMisc16Pkt* pPkt;
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
    p = pSrc;
    x = pRect->unk0;
    y = pRect->unk4;
    col = D_8009C618;
    for (oi = 8; oi >= 0; oi--) {
        xCur = x;
        iCol = D_800523F0[col & 0xFFF].unk0 * 2;
        idx = D_8009C5BC;
        for (ii = 8; ii >= 0; ii--) {
            v = *p;
            c = (s8)v << 3;
            if (v & 0x1000) {
                c += (D_800523F0[idx & 0xFFF].unk0 * iCol) >> 20;
            }
            pPkt->unk0 = (c << 16) | (xCur & 0xFFFF);
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
