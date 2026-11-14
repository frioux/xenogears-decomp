#include "common.h"
#include "psyq/libgte.h"
#include "psyq/inline_c.h"

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

typedef struct {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ u8 unk2;
    /* 0x03 */ u8 unk3;
    /* 0x04 */ void *unk4;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s16 unkC;
    /* 0x0E */ s16 unkE;
    /* 0x10 */ s16 unk10;
    /* 0x12 */ s16 pad12;
    /* 0x14 */ s16 unk14;
    /* 0x16 */ s16 unk16;
    /* 0x18 */ s16 unk18;
    /* 0x1A */ u8 pad1A[0x42];
    /* 0x5C */ s16 unk5C;
    /* 0x5E */ s16 unk5E;
    /* 0x60 */ s16 unk60;
    /* 0x62 */ u16 unk62;
    /* 0x64 */ s16 unk64;
    /* 0x66 */ s16 unk66;
    /* 0x68 */ s16 unk68;
    /* 0x6A */ s16 unk6A;
    /* 0x6C */ s16 unk6C;
    /* 0x6E */ s16 unk6E;
} BattleMisc22Cmd; // Size: 0x70

extern s32 D_800D3344;
extern s32 D_800D39CC;
extern s32 D_800D3348;
extern s32 BattleTestPointInTriangle(void *, void *, void *, s32);
extern void BattleProjectPointOntoTriangle(void *, void *, void *, s32, s32);
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
extern void HeapFreeIfNotNull(void *p);
extern void GfxLineScrollFree(void *pLineScroll);
extern void func_800B10EC(void *pArg, s16 x, s16 y, s32 z);
extern void HeapFree(void *pMem);

s32 BattleAllocSpriteSlot(BattleMisc22Unk *pArg, s16 arg1) {
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

s32 BattleEndSpriteSlotList(BattleMisc22Unk *pArg, u8 *pEnd) {
    s32 count = ((u32)pEnd - pArg->unk0) / 124;

    if (pArg->unk6 >= count) {
        pArg->unk6 = count;
    }
    ((s16 *)pEnd)[0xB] = -1;
    return count;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A2FD8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A32D8);
/* 74.17%, identical registers and exactly 428 bytes; the one difference is that
   gcc forwards the stored angle into the dividend instead of reloading the
   field (original: sh 0x64 ... lh 0x64, div). Making unk64/66/68 volatile
   forces the reload but costs a lhu + sll/sra pair: 86.36%, 452 bytes.
s32 func_800A32D8(BattleMisc22Cmd *pCmd, void *pData, s16 arg2, u8 arg3, s16 count,
                  u16 divisor, u8 a, u8 b, u8 c, u8 d, u8 e, u8 f,
                  u16 w0, u16 w1, u16 w2, u16 w3, u16 w4, u16 w5, u16 b3) {
    if (pCmd != 0) {
        pCmd->unk3 = b3;
        pCmd->unk5E = -1;
        pCmd->unk0 = arg2;
        pCmd->unk2 = arg3;
        pCmd->unk4 = pData;
        pCmd->unkC = w0;
        pCmd->unkE = w1;
        pCmd->unk10 = w2;
        pCmd->unk14 = w3;
        pCmd->unk16 = w4;
        pCmd->unk18 = w5;
        pCmd->unk5C = 0;
        if (count < 7) {
            pCmd->unk60 = count;
        } else {
            pCmd->unk60 = 7;
        }
        pCmd->unk64 = a * 64;
        pCmd->unk6A = (pCmd->unk64 - d * 64) / (s16)divisor;
        pCmd->unk66 = b * 64;
        pCmd->unk6C = (pCmd->unk66 - e * 64) / (s16)divisor;
        pCmd->unk68 = c * 64;
        pCmd->unk6E = (pCmd->unk68 - f * 64) / (s16)divisor;
        pCmd->unk62 = divisor;
        pCmd->unk8 = 0;
        return 0;
    }
}
*/
void BattleStoreMinusOneS16(s16 *pArg) {
    *pArg = -1;
}
s16 BattleOffsetBySin(s16 angle, s16 divisor, s16 base) {
    return base + (rsin(angle) + 0x1000) / divisor;
}
s16 BattleOffsetByAngleClamped(s16 angle, s16 divisor, s16 base) {
    s16 value = base + angle / divisor;

    if (value >= 33) {
        return -1;
    }
    return value;
}
/* byte-exact under maspsx --expand-div; the Default preset does not pass it, so
   the div-by-zero trap sequence is missing from our object (99.41% otherwise).
s16 BattleOffsetByAngleClamped(s16 angle, s16 divisor, s16 base) {
    s16 result = base + angle / divisor;

    if (result >= 0x21) {
        return -1;
    }
    return result;
}
*/
s16 BattleOffsetByNegatedAngle(s16 angle, s16 divisor, s16 base) {
    return base - angle / divisor;
}
s16 BattleCountdownOffsetByAngle(s16 angle, s16 divisor, s16 base) {
    s16 value = 0x20 - angle / divisor;

    if (value < base) {
        value = base;
    }
    return value;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A3640);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A3E98);
void BattleFreeImageEntry(BattleMisc22Image *pArg) {
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
void BattleScaleImageRegion(BattleMisc22Image *pArg, s16 scale) {
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
void BattleBlendImageRegion(BattleMisc22Image *pArg, s16 scale) {
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
extern u8 D_800D3304[];
extern u8 D_800D3306[];
extern u8 D_800D3308[];
extern u8 D_800D330A[];
extern u8 D_800D330C[];
extern u8 D_800D330E[];
extern u8 D_800D3310[];
extern u8 D_800D3314[];
extern u8 D_800D3316[];
extern u8 D_800D3330[];

INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A44C0);
/* 99.46% (404 bytes, identical instruction multiset): prologue, frame, loop,
   GTE block and both branches are exact; the only difference is the register
   rotation in the CompMatrix argument setup - original keeps pMat in $v1 and the
   "n * 124" chain in $a1 (lw v1 / lh v0 / addiu a0,v1 / sll a1,v0), ours puts
   pMat in $a1 and the chain in $v0 (lw a1 / lh v1 / addiu a0,a1 / sll v0,v1).
   Tried: separate s16 and s32 locals for the 0x3316 index, both operand orders of
   "pMat + offset" and "offset + pMat", a prepass offset local. Allocator tie.
void func_800A44C0(u8 **pArg) {
    u16 *pZ;
    u16 *pY;
    u16 *pX;
    void *pMatrix;
    s32 i;

    pMatrix = PSX_SCRATCH;
    pZ = (u16 *)D_800D3308;
    pY = pZ - 1;
    pX = pZ - 2;
    i = 0;
    do {
        if (*(s16 *)(D_800D330A + i) != 0) {
            s16 index = *(s16 *)(D_800D3314 + i);
            u8 *pEntry;

            if (index >= 0 && (pEntry = pArg[index]) != 0) {
                u8 *pMat = *(u8 **)(pEntry + 4);
                s32 offset = *(s16 *)(D_800D3316 + i) * 124;
                s32 vec[4];

                CompMatrix(pMat + 0xC, pMat + offset + 0xA8, pMatrix);
                SetRotMatrix(pMatrix);
                SetTransMatrix(pMatrix);
                gte_ldv0((SVECTOR *)(i + D_800D330C));
                gte_mvmva(1, 0, 0, 0, 0);
                gte_stlvnl(vec);
                *pX = *(u16 *)&vec[0];
                *pY = *(u16 *)&vec[1];
                *pZ = *(u16 *)&vec[2];
            } else {
                *pX = *(u16 *)(D_800D330C + i);
                *pY = *(u16 *)(D_800D330E + i);
                *pZ = *(u16 *)(D_800D3310 + i);
            }
        }
        pZ += 10;
        pY += 10;
        pX += 10;
        i += 0x14;
    } while ((s32)pZ < (s32)D_800D3330);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A4654);
void BattleFreeSceneResources(void) {
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
            HeapFreeIfNotNull(D_800C3D50[i]);
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A48EC);
s32 BattlePushTargetOutOfCircles(u16 *pPos, s16 *pTarget) {
    s32 result = 0;
    u16 *pSrc = D_800D2FD0;
    s32 i;

    for (i = 0; i < D_800D2FC8; i++) {
        s32 dx = *pSrc++ - pPos[0];
        s32 dz = *pSrc++ - pPos[2];
        s16 limit = *pSrc++;
        s32 d0 = pTarget[0] - (s16)dx;
        s32 d1 = pTarget[2] - (s16)dz;
        s32 dist = SquareRoot0(d0 * d0 + d1 * d1) + 1;

        if (dist < limit) {
            pTarget[0] = dx + d0 * limit / dist;
            pTarget[2] = dz + d1 * limit / dist;
            result = 1;
            break;
        }
    }
    return result;
}
void BattleSteerActorTowardCircles(void *pArg) {
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
s32 BattleGetTriangleVertexTable(void) {
    return D_800D3344;
}

s32 BattleGetTriangleEntryTable(void) {
    return D_800D39CC;
}

s32 BattleFindTriangleContainingPoint(s32 arg0) {
    BattleMisc22Entry *pEntry;
    s32 i;

    if (D_800D3344 != 0 && D_800D39CC != 0) {
        for (i = 0; i < D_800D3348; i++) {
            pEntry = (BattleMisc22Entry *)(i * 14 + D_800D39CC);
            if (BattleTestPointInTriangle((BattleMisc22SVec *)(pEntry->unk0 * 8 + D_800D3344),
                              (BattleMisc22SVec *)(pEntry->unk2 * 8 + D_800D3344),
                              (BattleMisc22SVec *)(pEntry->unk4 * 8 + D_800D3344), arg0) == -1) {
                return i;
            }
        }
    }
    return -1;
}

s32 BattleProjectPointOntoIndexedTriangle(s32 arg0, s32 index, s32 arg2) {
    BattleMisc22Entry *pEntry;

    if (D_800D3344 == 0 || D_800D39CC == 0 || index < 0) {
        return -1;
    }
    pEntry = (BattleMisc22Entry *)(index * 14 + D_800D39CC);
    BattleProjectPointOntoTriangle((BattleMisc22SVec *)(pEntry->unk0 * 8 + D_800D3344),
                  (BattleMisc22SVec *)(pEntry->unk2 * 8 + D_800D3344),
                  (BattleMisc22SVec *)(pEntry->unk4 * 8 + D_800D3344), arg0, arg2);
    return ((BattleMisc22Entry *)(index * 14 + D_800D39CC))->unkC;
}
