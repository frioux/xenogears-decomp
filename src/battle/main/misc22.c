#include "common.h"
#include "battle/actor.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "psyq/inline_c.h"
#include "system/graphics.h"

#define BATTLE_TWEEN_STEP_MAX 7

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
    /* 0x06 */ u16 pad6;
    /* 0x08 */ u_short x1;
    /* 0x0A */ u_short y1;
    /* 0x0C */ u_short z1;
    /* 0x0E */ s16 is3D; // 0 = the triples are screen coordinates, otherwise they are projected
    /* 0x10 */ u_short x2;
    /* 0x12 */ u_short y2;
    /* 0x14 */ u_short z2;
    /* 0x16 */ s16 age; // advances by the render step; -1 = slot free
    /* 0x18 */ u_short x3;
    /* 0x1A */ u_short y3;
    /* 0x1C */ u_short z3;
    /* 0x1E */ s16 lifetime; // the slot retires once age reaches it
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

// The texture animation record the script commands of func_800AE2A4 fill in through
// func_800A3640: the region's pixel buffers, the fade mode and the angle that walks the
// blend value. Every entry allocated keeps its own copies, all pixelCount pixels wide.
typedef struct {
    /* 0x0 */ void *pSource; // the record this one was set up from; when set, the upload is left to it
    /* 0x4 */ u16 *pPixels; // the region's own pixels, uploaded back to VRAM when the record is freed
    /* 0x8 */ u8 *pStartPixels; // the colours a fade or blend starts from
    /* 0xC */ u8 *pResultPixels; // where a fade or blend writes; modes 0 and 1 upload this
    /* 0x10 */ u8 mode; // below 4 the region at pPixels is uploaded back to VRAM when freed
    /* 0x11 */ u8 unk11;
    /* 0x12 */ s16 pixelCount; // rect28.w * rect28.h: the count the pixel helpers loop over
    /* 0x14 */ u16 angle; // advanced by angleStep per step, first argument of pInterpolate
    /* 0x16 */ u16 angleStep;
    /* 0x18 */ u16 prevFrame; // the value pInterpolate last returned; the same value again skips the work
    /* 0x1A */ u16 isLoaded; // nonzero while pPixels/pStartPixels/pResultPixels are held
    /* 0x1C */ BattleMisc22ImageRow *pRows; // region rows, indexed by rect28.y + row
    /* 0x20 */ s16 divisor; // second argument of pInterpolate
    /* 0x22 */ s16 offset; // third argument of pInterpolate
    /* 0x24 */ s16 (*pInterpolate)(u16 angle, s16 divisor, s16 offset); // one of the BattleOffsetBy* helpers
    /* 0x28 */ BattleMisc22Rect rect28;
} BattleMisc22Image; // Size: 0x30

typedef struct {
    /* 0x0 */ s16 vertexIndex0; // the triangle's three vertices, indexes into g_BattleTriangleVertices
    /* 0x2 */ s16 vertexIndex1;
    /* 0x4 */ s16 vertexIndex2;
    /* 0x6 */ u8 pad6[6];
    /* 0xC */ u8 unkC;
    /* 0xD */ u8 padD;
} BattleMisc22Entry; // Size: 0xE

typedef struct {
    /* 0x00 */ s16 spriteIndex; // negative = idle record; indexes the sprite the start/end are transformed by
    /* 0x02 */ u8 project3D; // nonzero = the endpoints are projected to 3D, 0 = they are screen coordinates
    /* 0x03 */ u8 semiTrans; // second argument of BattleAllocSpriteSlot
    /* 0x04 */ void *pPool; // the battle sprite pool the caller passes
    /* 0x08 */ s32 pSlot; // the slot BattleAllocSpriteSlot handed out; 0 until the first step
    /* 0x0C */ s16 startVx; // the endpoints the interpolation walks between
    /* 0x0E */ s16 startVy;
    /* 0x10 */ s16 startVz;
    /* 0x12 */ s16 pad12;
    /* 0x14 */ s16 endVx;
    /* 0x16 */ s16 endVy;
    /* 0x18 */ s16 endVz;
    /* 0x1A */ u8 pad1A[0x42];
    /* 0x5C */ s16 slot; // which of the eight step slots this record is on
    /* 0x5E */ s16 step;
    /* 0x60 */ s16 count; // steps the interpolation runs, capped at 7
    /* 0x62 */ u16 divisor; // count the start-to-end distance is divided into per step
    /* 0x64 */ s16 startX; // start of the interpolation, 8-bit coordinate << 6
    /* 0x66 */ s16 startY;
    /* 0x68 */ s16 startZ;
    /* 0x6A */ s16 deltaX; // per-step change from start towards the end position
    /* 0x6C */ s16 deltaY;
    /* 0x6E */ s16 deltaZ;
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
        if (pSpr->age == -1) {
            pArg->curIndex = i + 1;
            while (pArg->curIndex < pArg->capacity) {
                BattleMisc22Spr *pNext = &pArg->pSprites[pArg->curIndex];

                if (pNext->age == -1) {
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
    pEnd->age = -1;
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
        if (pSpr->age == -1) {
            continue;
        }
        if (pSpr->age >= pSpr->lifetime) {
            BattleEndSpriteSlotList(pBuffer, pSpr);
            continue;
        }

        pSpr->polys[packet].r0 = pSpr->red >> 6;
        pSpr->polys[packet].g0 = pSpr->green >> 6;
        pSpr->polys[packet].b0 = pSpr->blue >> 6;

        if (pSpr->is3D == 0) {
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

        pSpr->age += step;
        pSpr->red -= pSpr->redDelta * step;
        pSpr->green -= pSpr->greenDelta * step;
        pSpr->blue -= pSpr->blueDelta * step;
    }
}
s32 BattleSetupPositionTween(BattleMisc22Cmd *pCmd, void *pData, s16 arg2, u8 arg3, s16 count,
                  u16 divisor, u8 a, u8 b, u8 c, u8 d, u8 e, u8 f,
                  u16 w0, u16 w1, u16 w2, u16 w3, u16 w4, u16 w5, u16 b3) {
    if (pCmd != 0) {
        pCmd->semiTrans = b3;
        pCmd->step = -1;
        pCmd->spriteIndex = arg2;
        pCmd->project3D = arg3;
        pCmd->pPool = pData;
        pCmd->startVx = w0;
        pCmd->startVy = w1;
        pCmd->startVz = w2;
        pCmd->endVx = w3;
        pCmd->endVy = w4;
        pCmd->endVz = w5;
        pCmd->slot = 0;
        if (count < BATTLE_TWEEN_STEP_MAX) {
            pCmd->count = count;
        } else {
            pCmd->count = BATTLE_TWEEN_STEP_MAX;
        }
        // the step is derived from the value the 16-bit field holds, so the store must be re-read
        *(volatile s16 *)&pCmd->startX = a * 64;
        pCmd->deltaX = (pCmd->startX - d * 64) / (s16)divisor;
        *(volatile s16 *)&pCmd->startY = b * 64;
        pCmd->deltaY = (pCmd->startY - e * 64) / (s16)divisor;
        *(volatile s16 *)&pCmd->startZ = c * 64;
        pCmd->deltaZ = (pCmd->startZ - f * 64) / (s16)divisor;
        pCmd->divisor = divisor;
        pCmd->pSlot = 0;
        return 0;
    }
}
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
    if (pArg->isLoaded != 0) {
        if (pArg->pPixels != 0) {
            if (pArg->mode < 4) {
                LoadImage(&pArg->rect28, pArg->pPixels);
            }
            HeapFree(pArg->pPixels);
            pArg->pPixels = 0;
        }
        if (pArg->pStartPixels != 0) {
            HeapFree(pArg->pStartPixels);
            pArg->pStartPixels = 0;
        }
        if (pArg->pResultPixels != 0) {
            HeapFree(pArg->pResultPixels);
            pArg->pResultPixels = 0;
        }
        pArg->isLoaded = 0;
    }
}
void BattleScaleImageRegion(BattleMisc22Image *pArg, s16 scale) {
    s32 i;
    s32 j;
    u16 *pSrc = pArg->pPixels;

    for (i = 0; i < pArg->rect28.h; i++) {
        for (j = 0; j < pArg->rect28.w; j++) {
            s16 *pRow = pArg->pRows[pArg->rect28.y + i].pixels;

            pRow[pArg->rect28.x + j] = *pSrc * scale / 32;
            pSrc++;
        }
    }
}
void BattleBlendImageRegion(BattleMisc22Image *pArg, s16 scale) {
    s32 i;
    s32 j;
    u16 *pSrc1 = pArg->pPixels;
    u16 *pSrc2 = pArg->pStartPixels;

    for (i = 0; i < pArg->rect28.h; i++) {
        for (j = 0; j < pArg->rect28.w; j++) {
            s16 *pRow = pArg->pRows[pArg->rect28.y + i].pixels;

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
    /* 0x06 */ s16 inUse; // zero = the part is not laid out from a joint this frame
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
        if (g_BattlePartSlots[k].inUse != 0) {
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
    /* 0x00 */ u8 unk00[0x7];
    /* 0x07 */ u8 isVisible; // nonzero = the part is drawn
    /* 0x08 */ u16 jointIndex; // indexes the mesh's pParts; 0xFFFF = no part
    /* 0x0A */ u16 count;
    /* 0x0C */ MATRIX localTransform;
    /* 0x2C */ MATRIX globalTransform;
    /* 0x4C */ u8 unk4C[0x6];
    /* 0x52 */ s16 state; // selects how the part is drawn and which packet path it takes
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
/* 97.30% (592 bytes, retail 588); identical instruction multiset, ONE store
   position left. Everything else is byte-for-byte: prologue save order, the
   loop bound (count - 1, hoisted to $s7), the (u16)(state + 0xFFFF) < 2 range
   test, the matrix fixup block, the 4..7 flag switch tree, the two per-branch
   part/packet computations and the loop tail. Facts established here:
   - the signature has EIGHT params, not twelve: gcc 2.6.0 addresses incoming
     stack args at frame_size + 16 + (n - 5) * 4, so pOt/packetIndex/otIndex
     land on 0x54/0x58/0x5C only when they are params 6/7/8. A twelve-param
     prototype puts them at 0x64 and scores 90.8%.
   - the state field at sprite+0x52 is read as u16 for the range test
     (lhu + li 0xFFFF + addu + andi + sltiu) but as s16 at each later use
     (three `lh` reloads, not one CSE'd value): write the cast per use.
   - `pOt + otIndex - 1` emits the original's sll/addu + addiu (-4 in the j
     delay slot); `&pOt[otIndex - 1]` folds the -4 before the base add.
   - the part argument MUST be computed in the state==0 branch itself, or
     gcc cross-jumps it into the else branch's copy (5 instructions shared,
     572 bytes). A `void *pPart` local in that one branch breaks the merge.
   Remaining: the else branch's `g_GfxOTDepthShift = oldShift` store sits
   third in our RTL and last before the jal in retail. Tried: the store last
   in source (unchanged), pPart/pPacket locals in both orders (interleaves
   the two address computations, 95.6%), &pOt[] vs pointer arithmetic, and a
   4-minute permuter run (PERMUTER_NONE, base=120 best=120). The mips
   scheduler, not the source, puts the two incoming-stack loads above the
   store.
void func_800A48EC(BattleMisc22Mesh *pMesh, BattleMisc22Sprite *pSkeleton, MATRIX *pMatrix,
                   s32 arg3, s32 arg4, u_long *pOt, s32 packetIndex, s32 otIndex) {
    MATRIX *pMat = (MATRIX *)((u8 *)PSX_SCRATCH + 0x40);
    s32 oldShift;
    u_int numChildren;
    u_int i;
    s32 flags;

    oldShift = g_GfxOTDepthShift;
    numChildren = pSkeleton->count - 1;
    pSkeleton++;
    for (i = 0; i < numChildren; i++, pSkeleton++) {
        if (pSkeleton->jointIndex != 0xFFFF && pSkeleton->isVisible != 0) {
            CompMatrix(pMatrix, &pSkeleton->globalTransform, pMat);
            if ((u16)(pSkeleton->state + 0xFFFF) < 2) {
                pMat->m[0][0] = 0x1000;
                pMat->m[0][2] = 0;
                pMat->m[1][0] = 0;
                pMat->m[1][2] = 0;
                pMat->m[2][0] = 0;
                pMat->m[2][2] = 0x1000;
                if (pSkeleton->state == 1) {
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
            switch (pSkeleton->state) {
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
            if (pSkeleton->state == 0) {
                void *pPart = pMesh->pParts[pSkeleton->jointIndex];

                g_GfxOTDepthShift = 0x10;
                ModelRenderPacket(pPart, ((void **)&pSkeleton->p68)[packetIndex], pOt + otIndex - 1,
                                  flags);
            } else {
                g_GfxOTDepthShift = oldShift;
                ModelRenderPacket(pMesh->pParts[pSkeleton->jointIndex],
                                  ((void **)&pSkeleton->p68)[packetIndex], pOt, flags);
            }
        }
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
            if (BattleTestPointInTriangle(&g_BattleTriangleVertices[pEntry->vertexIndex0],
                                          &g_BattleTriangleVertices[pEntry->vertexIndex1],
                                          &g_BattleTriangleVertices[pEntry->vertexIndex2], arg0) == -1) {
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
    BattleProjectPointOntoTriangle(&g_BattleTriangleVertices[pEntry->vertexIndex0],
                                   &g_BattleTriangleVertices[pEntry->vertexIndex1],
                                   &g_BattleTriangleVertices[pEntry->vertexIndex2], arg0, arg2);
    return g_BattleTriangles[index].unkC;
}
