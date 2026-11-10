#include "common.h"

typedef struct {
    /* 0x0 */ u32 unk0;
    /* 0x4 */ s16 unk4;
    /* 0x6 */ s16 unk6;
} BattleMisc22Unk; // Size: 0x8

typedef struct {
    /* 0x0 */ s16 vx;
    /* 0x2 */ s16 vy;
    /* 0x4 */ s16 vz;
    /* 0x6 */ s16 pad6;
} BattleMisc22SVec; // Size: 0x8

typedef struct {
    /* 0x0 */ s16 x;
    /* 0x2 */ s16 y;
    /* 0x4 */ s16 w;
    /* 0x6 */ s16 h;
} BattleMisc22Rect; // Size: 0x8

typedef struct {
    /* 0x0 */ u8 pad0[0x4];
    /* 0x4 */ u16 *unk4;
    /* 0x8 */ u8 *unk8;
    /* 0xC */ u8 *unkC;
    /* 0x10 */ u8 unk10;
    /* 0x11 */ u8 pad11[0x9];
    /* 0x1A */ u16 unk1A;
    /* 0x1C */ u8 *unk1C;
    /* 0x20 */ u8 pad20[0x8];
    /* 0x28 */ BattleMisc22Rect rect28;
} BattleMisc22Image; // Size: 0x30

typedef struct {
    /* 0x0 */ s16 unk0;
    /* 0x2 */ s16 unk2;
    /* 0x4 */ s16 unk4;
    /* 0x6 */ u8 pad6[6];
    /* 0xC */ u8 unkC;
    /* 0xD */ u8 padD;
} BattleMisc22Entry; // Size: 0xE

extern s32 D_800D3344;
extern s32 D_800D39CC;
extern void func_800A5BE8(void *, void *, void *, s32, s32);
extern int LoadImage(void *pRect, void *p);
extern s16 D_800D2FC8;
extern u16 *D_800D2FD0;
extern s32 func_800AA650(void *pArg);
extern void func_800B10EC(void *pArg, s16 x, s16 y, s32 z);
extern void HeapFree(void *pMem);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A2E88);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A2F94);
/* 99.41%: instruction-for-instruction identical except that gcc puts the multu
   high part in a2 and shifts into v1, where retail mfhi's straight into v1.
   Source is right: 124 = 4*31 is the only divisor shape that reproduces the
   0x21084211/srl 2 sequence (a plain /31 gives the +1-magic variant).
s32 func_800A2F94(BattleMisc22Unk *pArg, u8 *pEnd) {
    s32 count = ((u32)pEnd - pArg->unk0) / 124;

    if (pArg->unk6 >= count) {
        pArg->unk6 = count;
    }
    ((s16 *)pEnd)[0xB] = -1;
    return count;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A2FD8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A32D8);
void func_800A3484(s16 *pArg) {
    *pArg = -1;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A3490);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A3514);
/* byte-exact under maspsx --expand-div; the Default preset does not pass it, so
   the div-by-zero trap sequence is missing from our object (99.41% otherwise).
s16 func_800A3514(s16 angle, s16 divisor, s16 base) {
    s16 result = base + angle / divisor;

    if (result >= 0x21) {
        return -1;
    }
    return result;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A3578);
/* 55%: the C is byte-exact under maspsx --expand-div (the original object has
   the break-7 / break-6 trap sequence around every div); the Default preset
   does not pass --expand-div. Same for func_800A3490/3514/35C8.
s16 func_800A3578(s16 angle, s16 divisor, s16 base) {
    return base - angle / divisor;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A35C8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A3640);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A3E98);
void func_800A429C(BattleMisc22Image *pArg) {
    if (pArg->unk1A != 0) {
        if (pArg->unk4 != 0) {
            if (pArg->unk10 < 4) {
                LoadImage(&pArg->rect28, pArg->unk4);
            }
            HeapFree(pArg->unk4);
            pArg->unk4 = 0;
        }
        if (pArg->unk8 != 0) {
            HeapFree(pArg->unk8);
            pArg->unk8 = 0;
        }
        if (pArg->unkC != 0) {
            HeapFree(pArg->unkC);
            pArg->unkC = 0;
        }
        pArg->unk1A = 0;
    }
}
void func_800A4348(BattleMisc22Image *pArg, s16 scale) {
    s32 i;
    s32 j;
    u16 *pSrc = pArg->unk4;

    for (i = 0; i < pArg->rect28.h; i++) {
        for (j = 0; j < pArg->rect28.w; j++) {
            s16 *pRow = (s16 *)((pArg->rect28.y + i) * 6 + (s32)pArg->unk1C);

            pRow[pArg->rect28.x + j] = *pSrc * scale / 32;
            pSrc++;
        }
    }
}
void func_800A43F8(BattleMisc22Image *pArg, s16 scale) {
    s32 i;
    s32 j;
    u16 *pSrc1 = pArg->unk4;
    u16 *pSrc2 = pArg->unk8;

    for (i = 0; i < pArg->rect28.h; i++) {
        for (j = 0; j < pArg->rect28.w; j++) {
            s16 *pRow = (s16 *)((pArg->rect28.y + i) * 6 + (s32)pArg->unk1C);

            pRow[pArg->rect28.x + j] = *pSrc2 + (*pSrc1 - *pSrc2) * scale / 32;
            pSrc1++;
            pSrc2++;
        }
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A44C0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A4654);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A4820);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A48EC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A4B3C);
void func_800A4CF8(void *pArg) {
    s16 a;
    s16 b;
    s16 c;
    u16 *pSrc = D_800D2FD0;
    s32 i;

    for (i = 0; i < D_800D2FC8; i++) {
        a = *pSrc++;
        b = *pSrc++;
        c = *pSrc++;
        func_800B10EC(pArg, a, b, func_800AA650(pArg) + c);
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A4DB8);
s32 func_800A577C(void) {
    return D_800D3344;
}

s32 func_800A578C(void) {
    return D_800D39CC;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A579C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A5870);
/* 99.27%: everything but the operand order of the three `addu` that form the
   SVECTOR pointers - retail has the scaled index first (addu a0,a0,v1), gcc
   canonicalises the base first. Tried pointer+int, int+ptr, u32 arithmetic and
   typed-array indexing; all give base first.
s32 func_800A5870(s32 arg0, s32 index, s32 arg2) {
    if (D_800D3344 == 0 || D_800D39CC == 0 || index < 0) {
        return -1;
    }
    func_800A5BE8(&((BattleMisc22SVec *)D_800D3344)[((BattleMisc22Entry *)D_800D39CC)[index].unk0],
                  &((BattleMisc22SVec *)D_800D3344)[((BattleMisc22Entry *)D_800D39CC)[index].unk2],
                  &((BattleMisc22SVec *)D_800D3344)[((BattleMisc22Entry *)D_800D39CC)[index].unk4],
                  arg0, arg2);
    return ((BattleMisc22Entry *)D_800D39CC)[index].unkC;
}
*/
