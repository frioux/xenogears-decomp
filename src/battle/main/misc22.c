#include "common.h"
#include "battle/actor.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "psyq/inline_c.h"
#include "system/graphics.h"

typedef struct {
    /* 0x0 */ struct _BattleMisc22Spr *pSprites;
    /* 0x4 */ s16 capacity;
    /* 0x6 */ s16 curIndex;
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

struct _BattleMisc22Spr {
    // NOTE: These first four blocks are sometimes used as SVECTORs, but they
    // can't be defined as such since the padding field is actually used.
    /* 0x00 */ u_short x0;
    /* 0x02 */ u_short y0;
    /* 0x04 */ u_short z0;
    /* 0x06 */ u16 unk6;
    /* 0x08 */ u_short x1;
    /* 0x0A */ u_short y1;
    /* 0x0C */ u_short z1;
    /* 0x0E */ s16 unkE;
    /* 0x10 */ u_short x2;
    /* 0x12 */ u_short y2;
    /* 0x14 */ u_short z2;
    /* 0x16 */ s16 unk16;
    /* 0x18 */ u_short x3;
    /* 0x1A */ u_short y3;
    /* 0x1C */ u_short z3;
    /* 0x1E */ s16 unk1E;
    /* 0x20 */ u_short red;
    /* 0x22 */ u_short green;
    /* 0x24 */ u_short blue;
    /* 0x26 */ s16 redDelta;
    /* 0x28 */ s16 greenDelta;
    /* 0x2A */ s16 blueDelta;
    /* 0x2C */ POLY_FT4 polys[2];
}; // Size: 0x7C

typedef struct _BattleMisc22Spr BattleMisc22Spr;

typedef struct {
    /* 0x0 */ s16 pixels[3];
} BattleMisc22ImageRow; // Size: 0x6

typedef struct {
    /* 0x0 */ u8 pad0[0x4];
    /* 0x4 */ u16 *unk4;
    /* 0x8 */ u8 *unk8;
    /* 0xC */ u8 *unkC;
    /* 0x10 */ u8 unk10;
    /* 0x11 */ u8 pad11[0x9];
    /* 0x1A */ u16 unk1A;
    /* 0x1C */ BattleMisc22ImageRow *unk1C;
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

extern BattleMisc22SVec *g_BattleTriangleVertices; // vertex table the battle scene sets up
extern s32 g_GfxOTDepthShift;
extern BattleMisc22Entry *g_BattleTriangles; // triangle table
extern s32 g_BattleTriangleCount;
extern s32 BattleTestPointInTriangle(void *, void *, void *, s32);
extern void BattleProjectPointOntoTriangle(void *, void *, void *, s32, s32);
extern s16 D_800D2FC8;
extern u16 *D_800D2FD0;
extern s32 D_800C3E38;
extern void ModelRenderPacket(void *pPart, void *pPacket, u_long *pOt, s32 flags);
extern void *g_BattleStageData;
extern void *D_800C3D50[2];
extern void *D_800C3EA0;
extern LineScroll g_BattleLineScrolls[];
extern s32 BattleGetActorRadius(void *pArg);
extern void func_800A9FF0(s32 arg0);
extern void HeapFreeIfNotNull(void *p);
extern void GfxLineScrollFree(LineScroll *pLineScroll);
extern void BattlePushActorOutOfCircle(void *pArg, s16 x, s16 y, s32 z);
extern void HeapFree(void *pMem);

s32 BattleAllocSpriteSlot(BattleMisc22Unk *pArg, s16 arg1) {
    BattleMisc22Spr *pSpr;
    s16 i;

    i = pArg->curIndex;
    if (i < pArg->capacity) {
        pSpr = &pArg->pSprites[i];
        if (pSpr->unk16 == -1) {
            pArg->curIndex = i + 1;
            while (pArg->curIndex < pArg->capacity) {
                BattleMisc22Spr *pNext = &pArg->pSprites[pArg->curIndex];

                if (pNext->unk16 == -1) {
                    break;
                }
                pArg->curIndex++;
            }
            SetSemiTrans(&pSpr->polys[0], arg1);
            SetSemiTrans(&pSpr->polys[1], arg1);
            return (s32)pSpr;
        }
    }
    return (s32)&pArg->pSprites[pArg->capacity];
}

s32 BattleEndSpriteSlotList(BattleMisc22Unk *pArg, BattleMisc22Spr *pEnd) {
    s32 count = ((mem_addr)pEnd - (mem_addr)pArg->pSprites) / sizeof(BattleMisc22Spr);

    if (pArg->curIndex >= count) {
        pArg->curIndex = count;
    }
    pEnd->unk16 = -1;
    return count;
}

void BattleRenderSpriteSlots(BattleMisc22Unk *pBuffer, MATRIX *pMatrix, s32 step, u_long *pOt, s32 packet) {
    BattleMisc22Spr *pSpr;
    long otz;
    s32 i;

    SetRotMatrix(pMatrix);
    SetTransMatrix(pMatrix);

    pSpr = pBuffer->pSprites;
    for (i = 0; i < pBuffer->capacity; i++, pSpr++) {
        if (pSpr->unk16 == -1) {
            continue;
        }
        if (pSpr->unk16 >= pSpr->unk1E) {
            BattleEndSpriteSlotList(pBuffer, pSpr);
            continue;
        }

        pSpr->polys[packet].r0 = pSpr->red >> 6;
        pSpr->polys[packet].g0 = pSpr->green >> 6;
        pSpr->polys[packet].b0 = pSpr->blue >> 6;

        if (pSpr->unkE == 0) {
            setXY4(&pSpr->polys[packet], pSpr->x0, pSpr->y0, pSpr->x1, pSpr->y1, pSpr->x2, pSpr->y2,
                   pSpr->x3, pSpr->y3);
            addPrim(pOt, &pSpr->polys[packet]);
        } else {
            gte_ldv3((SVECTOR *)&pSpr->x0, (SVECTOR *)&pSpr->x1, (SVECTOR *)&pSpr->x2);
            gte_rtpt();
            gte_stsxy3((long *)&pSpr->polys[packet].x0, (long *)&pSpr->polys[packet].x1,
                       (long *)&pSpr->polys[packet].x2);
            gte_stszotz(&otz);
            otz >>= g_GfxOTDepthShift;

            gte_ldv0((SVECTOR *)&pSpr->x3);
            gte_rtps();
            gte_stsxy((long *)&pSpr->polys[packet].x3);

            addPrim(&pOt[otz], &pSpr->polys[packet]);
        }

        pSpr->unk16 += step;
        pSpr->red -= pSpr->redDelta * step;
        pSpr->green -= pSpr->greenDelta * step;
        pSpr->blue -= pSpr->blueDelta * step;
    }
}
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
            s16 *pRow = pArg->unk1C[pArg->rect28.y + i].pixels;

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
            s16 *pRow = pArg->unk1C[pArg->rect28.y + i].pixels;

            pRow[pArg->rect28.x + j] = *pSrc2 + (*pSrc1 - *pSrc2) * scale / 32;
            pSrc1++;
            pSrc2++;
        }
    }
}
// One entry per part the battle scene lays out: the position the part ends up at
// (x/y/z, written by this function) and, for the parts that are driven by an
// actor's joint, the actor to follow plus the source position to transform.
typedef struct {
    /* 0x00 */ u16 x;
    /* 0x02 */ u16 y;
    /* 0x04 */ u16 z;
    /* 0x06 */ s16 unk6;
    /* 0x08 */ SVECTOR sourcePos;
    /* 0x10 */ s16 actorIndex;
    /* 0x12 */ s16 jointIndex;
} BattleMisc22PartSlot; // Size: 0x14

extern BattleMisc22PartSlot g_BattlePartSlots[];
extern u8 D_800D3330[];

void BattleTransformPartPositions(BattleActor **pArg) {
    u16 *pZ;
    u16 *pY;
    u16 *pX;
    void *pMatrix;
    s32 k;

    pMatrix = PSX_SCRATCH;
    pZ = &g_BattlePartSlots[0].z;
    pY = pZ - 1;
    pX = pZ - 2;
    k = 0;
    do {
        if (g_BattlePartSlots[k].unk6 != 0) {
            s16 index = g_BattlePartSlots[k].actorIndex;
            BattleActor *pActor;

            if (index >= 0 && (pActor = pArg[index]) != 0) {
                BattleSprite *pSprites = pActor->pSprites;
                BattleSprite *pJoint = &pSprites[g_BattlePartSlots[k].jointIndex + 1];
                s32 vec[4];

                CompMatrix(&pSprites->localTransform, &pJoint->globalTransform, pMatrix);
                SetRotMatrix(pMatrix);
                SetTransMatrix(pMatrix);
                gte_ldv0(&g_BattlePartSlots[k].sourcePos);
                gte_mvmva(1, 0, 0, 0, 0);
                gte_stlvnl(vec);
                *pX = *(u16 *)&vec[0];
                *pY = *(u16 *)&vec[1];
                *pZ = *(u16 *)&vec[2];
            } else {
                *pX = g_BattlePartSlots[k].sourcePos.vx;
                *pY = g_BattlePartSlots[k].sourcePos.vy;
                *pZ = g_BattlePartSlots[k].sourcePos.vz;
            }
        }
        pZ += 10;
        pY += 10;
        pX += 10;
        k++;
    } while ((s32)pZ < (s32)D_800D3330);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A4654);
void BattleFreeSceneResources(void) {
    s32 i;

    func_800A9FF0(0x1F);
    D_800C3E38 = 0;
    i = 0;
    if (g_BattleStageData != 0) {
        HeapFree(g_BattleStageData);
    }
    g_BattleStageData = 0;
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
        GfxLineScrollFree(&g_BattleLineScrolls[i]);
    }
}
typedef struct {
    /* 0x00 */ u8 unk00[0xA];
    /* 0x0A */ u16 count;
    /* 0x0C */ MATRIX localTransform;
    /* 0x2C */ MATRIX globalTransform;
    /* 0x4C */ u8 unk4C[0x8];
    /* 0x54 */ s16 rot[3];
    /* 0x5A */ u16 pad5A;
    /* 0x5C */ s32 pos[3];
    /* 0x68 */ void *p68;
    /* 0x6C */ void *p6C;
    /* 0x70 */ u8 *p70;
    /* 0x74 */ u8 *p74;
    /* 0x78 */ u8 *p78;
} BattleMisc22Sprite; // Size: 0x7C

typedef struct {
    /* 0x0 */ void **pParts;
    /* 0x4 */ u32 count;
} BattleMisc22Mesh; // Size: 0x8

INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A48EC);
/* 78.13% (556 bytes, retail 588) first attempt, built from the sibling
   BattleRenderModel in battle/main/misc21 which has the same skeleton walk:
   numChildren = pSkeleton->count (0xA), pSkeleton++, then
   for (i = 1; i < numChildren; i++) over 0x7C-byte BattleSprite entries.
   Remaining diff: retail keeps TWO sprite cursors (s1 = s2 + 8, the
   unk00+8 field, and s2 = s2 + 0x7C) and a separate i in s4, and its frame is
   0x40 against our 0x38; the loop tail is also 4 insns longer. Also untested:
   the 4..7 flags dispatch (delta a3 = state - 2) and whether the state-1/2
   test is written as a switch rather than (u16)(state - 1) < 2.
void func_800A48EC(BattleMisc22Mesh *pMesh, BattleMisc22Sprite *pSkeleton, MATRIX *pMatrix,
                   s32 arg3, s32 arg4, s32 arg5, s32 arg6, s32 arg7, s32 arg8,
                   u_long *pOt, s32 packetIndex, s32 otIndex) {
    MATRIX *pMat = (MATRIX *)((u8 *)PSX_SCRATCH + 0x40);
    s32 oldShift;
    u_int numChildren;
    u_int i;
    s16 state;
    s32 flags;

    oldShift = g_GfxOTDepthShift;
    numChildren = pSkeleton->count;
    pSkeleton++;
    for (i = 1; i < numChildren; i++) {
        if (*(u16 *)(pSkeleton->unk00 + 8) != 0xFFFF && pSkeleton->unk00[7] != 0) {
            state = *(s16 *)(pSkeleton->unk4C + 6);
            CompMatrix(pMatrix, &pSkeleton->globalTransform, pMat);
            if ((u16)(state - 1) < 2) {
                pMat->m[0][0] = 0x1000;
                pMat->m[0][2] = 0;
                pMat->m[1][0] = 0;
                pMat->m[1][2] = 0;
                pMat->m[2][0] = 0;
                pMat->m[2][2] = 0x1000;
                if (state == 1) {
                    pMat->m[0][1] = pMatrix->m[0][1];
                    pMat->m[1][1] = pMatrix->m[1][1];
                    pMat->m[2][1] = pMatrix->m[2][1];
                } else {
                    pMat->m[0][1] = 0;
                    pMat->m[1][1] = 0x1000;
                    pMat->m[2][1] = 0;
                }
            }
            SetRotMatrix(pMat);
            SetTransMatrix(pMat);
            switch (state) {
            case 4:
                flags = 2;
                break;
            case 5:
                flags = 3;
                break;
            case 6:
                flags = 4;
                break;
            case 7:
                flags = 5;
                break;
            default:
                flags = 0;
                break;
            }
            if (state == 0) {
                g_GfxOTDepthShift = 0x10;
                ModelRenderPacket(pMesh->pParts[*(u16 *)(pSkeleton->unk00 + 8)],
                                  ((void **)&pSkeleton->p68)[packetIndex], &pOt[otIndex - 1], flags);
            } else {
                g_GfxOTDepthShift = oldShift;
                ModelRenderPacket(pMesh->pParts[*(u16 *)(pSkeleton->unk00 + 8)],
                                  ((void **)&pSkeleton->p68)[packetIndex], pOt, flags);
            }
        }
        pSkeleton++;
    }
    g_GfxOTDepthShift = oldShift;
}
*/
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
        BattlePushActorOutOfCircle(pArg, a, b, BattleGetActorRadius(pArg) + c);
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A4DB8);
s32 BattleGetTriangleVertexTable(void) {
    return (s32)g_BattleTriangleVertices;
}

s32 BattleGetTriangleEntryTable(void) {
    return (s32)g_BattleTriangles;
}

s32 BattleFindTriangleContainingPoint(s32 arg0) {
    BattleMisc22Entry *pEntry;
    s32 i;

    if (g_BattleTriangleVertices != 0 && g_BattleTriangles != 0) {
        for (i = 0; i < g_BattleTriangleCount; i++) {
            pEntry = &g_BattleTriangles[i];
            if (BattleTestPointInTriangle(&g_BattleTriangleVertices[pEntry->unk0], &g_BattleTriangleVertices[pEntry->unk2],
                              &g_BattleTriangleVertices[pEntry->unk4], arg0) == -1) {
                return i;
            }
        }
    }
    return -1;
}

s32 BattleProjectPointOntoIndexedTriangle(s32 arg0, s32 index, s32 arg2) {
    BattleMisc22Entry *pEntry;

    if (g_BattleTriangleVertices == 0 || g_BattleTriangles == 0 || index < 0) {
        return -1;
    }
    pEntry = &g_BattleTriangles[index];
    BattleProjectPointOntoTriangle(&g_BattleTriangleVertices[pEntry->unk0], &g_BattleTriangleVertices[pEntry->unk2],
                  &g_BattleTriangleVertices[pEntry->unk4], arg0, arg2);
    return g_BattleTriangles[index].unkC;
}
