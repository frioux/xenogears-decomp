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
    /* 0x00 */ u8 pad0[0x16];
    /* 0x16 */ s16 unk16;
    /* 0x18 */ u8 pad18[0x14];
    /* 0x2C */ u8 unk2C[0x28];
    /* 0x54 */ u8 unk54[0x28];
} BattleMisc22Spr; // Size: 0x7C

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
extern s32 D_800D3348;
extern s32 func_800A5A48(void *, void *, void *, s32);
extern void func_800A5BE8(void *, void *, void *, s32, s32);
extern int LoadImage(void *pRect, void *p);
extern s16 D_800D2FC8;
extern u16 *D_800D2FD0;
extern s32 D_800C3E38;
extern void *D_800658C8;
extern void *D_800C3D50[2];
extern void *D_800C3EA0;
extern u8 D_800C3DA0[];
extern s32 func_800AA650(void *pArg);
extern void func_800A9FF0(s32 arg0);
extern void func_80027D40(void *p);
extern void GfxLineScrollFree(void *pLineScroll);
extern void func_800B10EC(void *pArg, s16 x, s16 y, s32 z);
extern void HeapFree(void *pMem);

s32 func_800A2E88(BattleMisc22Unk *pArg, s16 arg1) {
    BattleMisc22Spr *pSpr;
    s16 i;

    i = pArg->unk6;
    if (i < pArg->unk4) {
        pSpr = (BattleMisc22Spr *)(i * 124 + pArg->unk0);
        if (pSpr->unk16 == -1) {
            pArg->unk6 = i + 1;
            while (pArg->unk6 < pArg->unk4) {
                BattleMisc22Spr *pNext = (BattleMisc22Spr *)(pArg->unk6 * 124 + pArg->unk0);

                if (pNext->unk16 == -1) {
                    break;
                }
                pArg->unk6++;
            }
            SetSemiTrans(&pSpr->unk2C, arg1);
            SetSemiTrans(&pSpr->unk54, arg1);
            return (s32)pSpr;
        }
    }
    return pArg->unk4 * 124 + pArg->unk0;
}

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
s16 func_800A3490(s16 angle, s16 divisor, s16 base) {
    return base + (rsin(angle) + 0x1000) / divisor;
}
s16 func_800A3514(s16 angle, s16 divisor, s16 base) {
    s16 value = base + angle / divisor;

    if (value >= 33) {
        return -1;
    }
    return value;
}
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
s16 func_800A3578(s16 angle, s16 divisor, s16 base) {
    return base - angle / divisor;
}
s16 func_800A35C8(s16 angle, s16 divisor, s16 base) {
    s16 value = 0x20 - angle / divisor;

    if (value < base) {
        value = base;
    }
    return value;
}
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A43F8);
/* 96.30%: same loops and address arithmetic as func_800A4348 (which is 100%);
   retail re-loads *pSrc2 for the accumulate instead of keeping it in a
   register, i.e. that pseudo was spilled and rematerialised there, and the
   scalars land in t2/t0/t3 instead of t3/t2/t4. Tried a delta local (76.50%),
   ++ inside the expression and swapped declarations.
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
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A44C0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A4654);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A4820);
/* 96.86%: every instruction is right except the first loop's i++, which retail
   has in the delay slot of the `beqz` before the func_80027D40 call and we emit
   after the store, one insn too late for reorg to steal it. The i = 0 hoists
   (before each if) are needed to put `move s1,zero` in the beqz delay slots.
void func_800A4820(void) {
    s32 i;

    func_800A9FF0(0x1F);
    D_800C3E38 = 0;
    i = 0;
    if (D_800658C8 != 0) {
        HeapFree(D_800658C8);
    }
    D_800658C8 = 0;
    for (; i < 2;) {
        if (D_800C3D50[i] != 0) {
            func_80027D40(D_800C3D50[i]);
        }
        D_800C3D50[i++] = 0;
    }
    i = 0;
    if (D_800C3EA0 != 0) {
        HeapFree(D_800C3EA0);
    }
    D_800C3EA0 = 0;
    for (; i < 2; i++) {
        GfxLineScrollFree(&D_800C3DA0[i * 0x18]);
    }
}
*/
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
s32 func_800A579C(s32 arg0) {
    BattleMisc22Entry *pEntry;
    s32 i;

    if (D_800D3344 != 0 && D_800D39CC != 0) {
        for (i = 0; i < D_800D3348; i++) {
            pEntry = (BattleMisc22Entry *)(i * 14 + D_800D39CC);
            if (func_800A5A48((BattleMisc22SVec *)(pEntry->unk0 * 8 + D_800D3344),
                              (BattleMisc22SVec *)(pEntry->unk2 * 8 + D_800D3344),
                              (BattleMisc22SVec *)(pEntry->unk4 * 8 + D_800D3344), arg0) == -1) {
                return i;
            }
        }
    }
    return -1;
}
s32 func_800A5870(s32 arg0, s32 index, s32 arg2) {
    BattleMisc22Entry *pEntry;

    if (D_800D3344 == 0 || D_800D39CC == 0 || index < 0) {
        return -1;
    }
    pEntry = (BattleMisc22Entry *)(index * 14 + D_800D39CC);
    func_800A5BE8((BattleMisc22SVec *)(pEntry->unk0 * 8 + D_800D3344),
                  (BattleMisc22SVec *)(pEntry->unk2 * 8 + D_800D3344),
                  (BattleMisc22SVec *)(pEntry->unk4 * 8 + D_800D3344), arg0, arg2);
    return ((BattleMisc22Entry *)(index * 14 + D_800D39CC))->unkC;
}
