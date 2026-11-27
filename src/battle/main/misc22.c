#include "common.h"
#include "battle/actor.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "psyq/inline_c.h"
#include "system/graphics.h"

#define BATTLE_TWEEN_STEP_MAX 7
#define BATTLE_PART_NO_JOINT 0xFFFF

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
    /* 0x8 */ u16 *pStartPixels; // the colours a fade or blend starts from
    /* 0xC */ u16 *pResultPixels; // where a fade or blend writes; modes 0 and 1 upload this
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
    /* 0x24 */ s32 (*pInterpolate)(u16 angle, s16 divisor, s16 offset); // one of the BattleOffsetBy* helpers
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
extern s16 g_BattleCircleCount;
extern u16 *g_BattleCircles;
extern s32 g_BattleSceneSpriteList;
extern void ModelRenderPacket(void *pPart, void *pPacket, u_long *pOt, s32 flags);
extern void *g_BattleStageData;
extern void *g_BattlePartPrimBuffers[2];
extern void *g_BattleSpriteRenderData;
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
extern void *HeapAlloc(u_int allocSize, u_int allocFlags);
extern void HeapChangeCurrentUser(u_int userTag, char **pContentTypes);

// best 89.33% (2100 bytes / 525 insns; retail 2136 = 0x858 / 534). REWRITTEN from the lib_gear/main/main func_801E0A00 sibling park (same 19-param function, 534 insns there): 81.30 -> 89.33. The gains were (a) u16 w = width; u16 h = height; as NARROW WORKING COPIES of the two s32 stack args with every narrow use going through (s16)w / (s16)h (mult, loop bounds, HeapAlloc sizes, pixel += (s16)w), which is what puts w in s3 and h in s2 as retail has them, (b) the early isLoaded exit RETURNS NULL (retail has 'addu v0,zero,zero' in the bnez delay slot), (c) switch (modeSel) with case 0/1 and 4/5 grouped and NO default, (d) the flat colour as (((red & 0x3F) << 10) + ((areaY & 0x1F) << 5)) | (areaX & 0x1F) with + then |, (e) pRows[(s16)areaY0 + row].pixels[(s16)areaX0 + col] with the (s16) casts hoisted per row, (f) switch (((s16)rectY + row) % 3). WHAT IS LEFT, unchanged from the lib_gear lane's finding: the frame is 0xE8 here vs retail 0x108 and retail has THREE extra spill slots - 0x20 = the mode word (retail sh s4,0x20(sp), re-read as lbu for the mode byte store and lhu for the switch), 0x78 = the WIDE width and 0x80 = the WIDE height, which gcc spills only while the two '== 0' default tests keep the wide values live; retail then tests them 16-bit (lw 0x78(sp) / sll 16 / bnez). Our body narrows them at entry and tests the narrow copy, so the two homes disappear. Also ours costs an extra callee-saved save (11 vs retail's 10). MEASURED THIS ROUND: 'if ((s16)width == 0) w = 0x100;' + same for height (the wide tested, one home restored) 89.29 / 540 insns / 2160 bytes - the byte count moves TOWARD retail's 534 but the percentage is a hair worse, so the 16-bit test shape is NOT the missing 9 instructions on its own. NEXT: find the source shape that spills the WIDE width/height while keeping them 16-bit-tested - the lib_gear lane's untried lead was explicit narrow locals refreshed after w = half * 2. No *.rodata/data in this stub.
/*
BattleMisc22Image *func_800A3640(BattleMisc22Image *pArg, void *pSource, s32 mode, s32 effects,
                                 BattleMisc22ImageRow *pRows, u16 areaX0, u16 areaY0, u16 red0,
                                 u16 areaX1, u16 areaY1, u16 red1, u16 rectX, u16 rectY,
                                 s32 width, s32 height, s16 angleStep, s16 divisor, s16 offset,
                                 s32 (*pInterpolate)(u16 angle, s16 divisor, s16 offset)) {
    RECT rect;
    u16 w = width;
    u16 h = height;
    u16 mask;
    u16 colour;
    u16 modeSel;
    int half;
    int row;
    int col;
    int pixel;
    int pixelB;
    int i;

    if (pArg->isLoaded != 0) {
        return NULL;
    }

    HeapChangeCurrentUser(4, 0);

    pArg->isLoaded = 1;
    pArg->unk11 = 0;
    pArg->mode = mode;
    pArg->pSource = pSource;
    pArg->angle = 0;
    pArg->prevFrame = 0xFFFF;
    pArg->angleStep = angleStep;
    pArg->divisor = divisor;
    pArg->offset = offset;
    pArg->pInterpolate = pInterpolate;
    pArg->pRows = pRows;

    mask = effects;
    if ((mode & 1) == 0) {
        mask &= 0xFD0F;
    }
    if (mode & 4) {
        mask &= 0xFEFF;
    }
    if (w == 0) {
        w = 0x100;
    }
    if (h == 0) {
        h = 0x100;
    }

    modeSel = mode;

    switch (modeSel) {
    case 0:
    case 1:
        half = ((s16)w + 1) / 2;
        w = half * 2;
        pArg->rect28.x = rectX;
        pArg->rect28.y = rectY;
        pArg->rect28.w = w;
        pArg->rect28.h = h;
        pArg->pixelCount = w * h;
        if (mask & 0x100) {
            pArg->pResultPixels = HeapAlloc((s16)w * (s16)h * 2, 0);
        }
        if (mask & 0x200) {
            pArg->pStartPixels = HeapAlloc((s16)w * (s16)h * 2, 0);
        }
        if (mask & 0x400) {
            pArg->pPixels = HeapAlloc((s16)w * (s16)h * 2, 0);
        }
        switch (mask & 0xF) {
        case 1:
            rect.x = areaX0;
            rect.y = areaY0;
            rect.w = w;
            rect.h = h;
            StoreImage(&rect, (u_long *)pArg->pPixels);
            DrawSync(0);
            break;
        case 2:
            colour = (((red0 & 0x3F) << 10) + ((areaY0 & 0x1F) << 5)) | (areaX0 & 0x1F);
            for (i = 0; i < (s16)w * (s16)h; i++) {
                pArg->pPixels[i] = colour;
            }
            break;
        }
        switch ((mask >> 4) & 0xF) {
        case 1:
            rect.x = areaX1;
            rect.y = areaY1;
            rect.w = w;
            rect.h = h;
            StoreImage(&rect, (u_long *)pArg->pStartPixels);
            DrawSync(0);
            break;
        case 2:
            colour = (((red1 & 0x3F) << 10) + ((areaY1 & 0x1F) << 5)) | (areaX1 & 0x1F);
            for (i = 0; i < (s16)w * (s16)h; i++) {
                pArg->pStartPixels[i] = colour;
            }
            break;
        }
        break;
    case 4:
    case 5:
        pArg->rect28.x = rectX;
        pArg->rect28.y = rectY;
        pArg->rect28.w = w;
        pArg->rect28.h = h;
        pArg->pixelCount = w * h;
        if (mask & 0x100) {
            pArg->pResultPixels = HeapAlloc((s16)w * (s16)h * 2, 0);
        }
        if (mask & 0x200) {
            pArg->pStartPixels = HeapAlloc((s16)w * (s16)h * 2, 0);
        }
        if (mask & 0x400) {
            pArg->pPixels = HeapAlloc((s16)w * (s16)h * 2, 0);
        }
        switch (mask & 0xF) {
        case 1:
            for (row = 0; row < (s16)h; row++) {
                for (col = 0; col < (s16)w; col++) {
                    pArg->pPixels[row * (s16)w + col] =
                        pRows[(s16)areaY0 + row].pixels[(s16)areaX0 + col];
                }
            }
            break;
        case 2:
            pixel = 0;
            for (row = 0; row < (s16)h; row++) {
                for (col = 0; col < (s16)w; col++) {
                    switch (((s16)rectY + row) % 3) {
                    case 0:
                        colour = areaX0;
                        break;
                    case 1:
                        colour = areaY0;
                        break;
                    case 2:
                        colour = red0;
                        break;
                    }
                    pArg->pPixels[pixel + col] = colour;
                }
                pixel += (s16)w;
            }
            break;
        }
        switch ((mask >> 4) & 0xF) {
        case 1:
            for (row = 0; row < (s16)h; row++) {
                for (col = 0; col < (s16)w; col++) {
                    pArg->pStartPixels[row * (s16)w + col] =
                        pRows[(s16)areaY1 + row].pixels[(s16)areaX1 + col];
                }
            }
            break;
        case 2:
            pixelB = 0;
            for (row = 0; row < (s16)h; row++) {
                for (col = 0; col < (s16)w; col++) {
                    switch (((s16)rectY + row) % 3) {
                    case 0:
                        colour = areaX1;
                        break;
                    case 1:
                        colour = areaY1;
                        break;
                    case 2:
                        colour = red1;
                        break;
                    }
                    pArg->pStartPixels[pixelB + col] = colour;
                }
                pixelB += (s16)w;
            }
            break;
        }
        break;
    }
    return pArg;
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A3640);


extern void func_80026F44(s32 count, s32 level, u16 *pDst, u16 *pSrc);
extern u32 func_80026FE8(s32 count, s32 level, u16 *pDst, u16 *pSrc, u16 *pSrc2);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A3E98);
/* best 90.65% (1040 bytes; retail 1028 = 0x404, 257 insns, so three over).
   The simplest form of the tail (early `if (frame == prevFrame) return
   prevFrame;`, in the history) measured 90.63 / 1036, two over. Shape is right
   end to end: the pInterpolate sign-extension pair, the four case bodies in
   source order 0/1/4/5, the two cross-jumped LoadImage tails, the rect clip
   with its lh-then-lhu reload pairs, and both copy loops.
   WHAT IS LEFT, and what was measured this round:
   - The prevFrame early exit's BRANCH SHAPE is solved by moving the early
     return out: `if (frame != prevFrame) { <everything> return (s16)level; }`
     then `return prevFrame;`. That gives retail's `beq s1,v0,<epilogue>`
     directly (retail branch, not the `bne` + `j` + `move v0,s1` the early-
     return form produces). The cost moves to the tail: on the path where the
     two compare equal gcc folds prevFrame to the s16 frame (record_jump_equiv
     maps the loaded field to the compared register), so the equal path still
     needs one `move v0,s1`, that block lands after the truncating exit, and
     the truncating exit therefore needs a `j` -> +2 there. No spelling of the
     returned expression avoided the fold: the field, a u16 local, an s32 and a
     u32 local of it, and both compare operand orders all fold.
   - `sh s2,0x18(s0)`: retail stores the raw callback result; ours copies it
     into v1 in the bgez delay slot (`move v1,s2`) and stores v1 - the store of
     an s32 into the u16 field materialises the truncation. prevFrame declared
     s32 / u16 / s16 changes nothing.
   - The mode dispatch costs one extra instruction: retail has
     `blez v0,<sll block>` with `li v0,0x1` in the delay slot (reused by the
     `sb v0,17(t1)`); ours fills that slot with `sll v0,s2,0x10` and needs the
     `li` as an instruction.
   - Registers and interleave in both loops: retail pSource->pPixels=t3 and
     pArg->pResultPixels=t2, ours t2 and a1; retail puts the store-row product
     in a2 and the load-row product in v1, ours a0 and v0; retail interleaves
     the two address chains with the two mults, ours runs them serially. Both
     loops are the same length as retail's (25 and 24 instructions), so this is
     allocation plus statement interleave, not structure.
   Facts that still hold (measured in earlier rounds): pInterpolate returns
   something wider than s16 - keep `level` s32 and `frame` s16 as two
   variables; loop B's source address needs the embedded assignment
   `*(pSrcRow = pRows + 3 * (srcY + y) + (srcX + x))`; case 1 needs the
   pixelCount in its own s16 local; pDstBase/pSrcBase must be locals assigned
   before the mode test (1036 vs 1048 bytes).
   The lib_gear twin func_801E1258 is the same function, parked at 88.98%.
   Two earlier permuter runs took this body 88.34 -> 90.63; a 4-minute run on
   the early-return form was PERMUTER_NONE (base=120 best=120).
   ROUND 492 re-measured this exact body: 90.65%, 1040 bytes, 260 insns
   (retail 1028 / 257), so the note is live, and re-confirmed the early-return
   form above at 1036 / 259 / 90.63 - one instruction closer to linking, two
   over. The three extra here are the `move v1,s2` that materialises the 0x18
   store plus the equal path's own `j` and `move v0,s1`. Newly measured,
   none of it moved the count:
   - `return frame;` at ALL THREE exits instead of `return (s16)level;` does
     remove the store truncation, and `sh s2,0x18(s0)` then lands in retail's
     `beq` delay slot, but the body stays 260 / 1040 and scores 90.28; at the
     tail exit alone it is worse (264 / 1056).
   - The realistic typed loops (`pDst = pDstBase + dstRect.x + x;
     pDst[row] = pSrc[row2];` and `pArg->pRows[y].pixels[x]`) are byte-count
     identical (260 / 1040) and score 89.29, because pointer arithmetic emits
     the BASE first in every address addu where retail has the index first
     (`addu a0,a0,t3`). The u32-cast form below carries retail's operand
     order, so keep it.
   - Presets for the residual: gcc 2.7.2-cdk 86.68; plain 2.7.2 cannot
     compile this unit's C.
s32 func_800A3E98(BattleMisc22Image *pArg, s32 arg1) {
    BattleMisc22Image *pSource;
    BattleMisc22Rect srcRect;
    BattleMisc22Rect dstRect;
    s32 level;
    s16 frame;
    s16 pixelCount;
    u16 prevFrame;
    s32 y;
    s32 x;
    s32 dstAddr;
    s32 srcAddr;
    u16 *pDstBase;
    u16 *pSrcBase;
    u16 *pSrcRow;

    if (pArg->isLoaded == 0) {
        return -1;
    }

    pArg->angle += pArg->angleStep * (arg1 + 1);
    level = pArg->pInterpolate(pArg->angle, pArg->divisor, pArg->offset);
    frame = level;
    if (frame < 0) {
        BattleFreeImageEntry(pArg);
        return frame;
    }
    prevFrame = pArg->prevFrame;
    if (frame != prevFrame) {
        pArg->prevFrame = level;

        switch (pArg->mode) {
        case 0:
            func_80026F44(pArg->pixelCount, frame, pArg->pResultPixels, pArg->pPixels);
            if (pArg->pSource == 0) {
                LoadImage(&pArg->rect28, pArg->pResultPixels);
            }
            break;
        case 1:
            pixelCount = pArg->pixelCount;
            func_80026FE8(pixelCount, frame, pArg->pResultPixels, pArg->pStartPixels,
                          pArg->pPixels);
            if (pArg->pSource == 0) {
                LoadImage(&pArg->rect28, pArg->pResultPixels);
            }
            break;
        case 4:
            BattleScaleImageRegion(pArg, frame);
            break;
        case 5:
            BattleBlendImageRegion(pArg, frame);
            break;
        }

        pSource = pArg->pSource;
        if (pSource == 0) {
        return (s16)level;
    }
    if (pSource->isLoaded == 0) {
        return (s16)level;
    }

    // Clip the copy to the overlap of the two regions
    if (pSource->rect28.x < pArg->rect28.x) {
        dstRect.x = pArg->rect28.x - pSource->rect28.x;
        srcRect.x = 0;
        dstRect.w = pSource->rect28.x + pSource->rect28.w - pArg->rect28.x;
    } else {
        dstRect.x = 0;
        srcRect.x = pSource->rect28.x - pArg->rect28.x;
        dstRect.w = pArg->rect28.x + pArg->rect28.w - pSource->rect28.x;
    }
    if (pSource->rect28.y < pArg->rect28.y) {
        dstRect.y = pArg->rect28.y - pSource->rect28.y;
        srcRect.y = 0;
        dstRect.h = pSource->rect28.y + pSource->rect28.h - pArg->rect28.y;
    } else {
        dstRect.y = 0;
        srcRect.y = pSource->rect28.y - pArg->rect28.y;
        dstRect.h = pArg->rect28.y + pArg->rect28.h - pSource->rect28.y;
    }

    if (dstRect.w > 0 && dstRect.h > 0) {
        pSource->unk11 = 1;
        pDstBase = pSource->pPixels;
        if (pSource->mode < 4) {
            pSrcBase = (pSrcBase = pArg->pResultPixels);
            for (y = 0; y < dstRect.h; y++) {
                for (x = 0; x < dstRect.w; x++) {
                    dstAddr = dstRect.x * 2 + (u32)pDstBase;
                    srcAddr = srcRect.x * 2 + (u32)pSrcBase;
                    dstAddr = x * 2 + dstAddr;
                    srcAddr = x * 2 + srcAddr;
                    *(u16 *)((dstRect.y + y) * pSource->rect28.w * 2 + dstAddr) =
                        *(u16 *)((srcRect.y + y) * pArg->rect28.w * 2 + srcAddr);
                }
            }
        } else {
            for (y = 0; y < dstRect.h; y++) {
                for (x = 0; x < dstRect.w; x++) {
                    dstAddr = dstRect.x;
                    dstAddr = dstAddr * 2 + (u32)pDstBase;
                    dstAddr = x * 2 + dstAddr;
                    *(u16 *)((dstRect.y + y) * pSource->rect28.w * 2 + dstAddr) =
                        *(pSrcRow = (u16 *)pArg->pRows + 3 * (srcRect.y + y) + (srcRect.x + x));
                }
            }
        }
    }

        return (s16)level;
    }
    return prevFrame;
}
*/
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
    g_BattleSceneSpriteList = 0;
    i = 0;
    if (g_BattleStageData != 0) {
        HeapFree(g_BattleStageData);
    }
    g_BattleStageData = 0;
    for (; i < 2;) {
        if (g_BattlePartPrimBuffers[i] != 0) {
            HeapFreeIfNotNull(g_BattlePartPrimBuffers[i]);
        }
        g_BattlePartPrimBuffers[i++] = 0;
    }
    i = 0;
    if (g_BattleSpriteRenderData != 0) {
        HeapFree(g_BattleSpriteRenderData);
    }
    g_BattleSpriteRenderData = 0;
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
    /* 0x52 */ u16 state; // selects how the part is drawn and which packet path it takes
    /* 0x54 */ s16 rot[3];
    /* 0x5A */ u16 pad5A;
    /* 0x5C */ s32 pos[3];
    /* 0x68 */ void *packets[5]; // one render packet per packetIndex
} BattleMisc22Sprite; // Size: 0x7C

typedef struct {
    /* 0x0 */ void **pParts;
    /* 0x4 */ u32 count;
} BattleMisc22Mesh; // Size: 0x8

void func_800A48EC(BattleMisc22Mesh *pMesh, BattleMisc22Sprite *pSkeleton, MATRIX *pMatrix,
                   s32 arg3, s32 arg4, u_long *pOt, s32 packetIndex, s32 otIndex) {
    MATRIX *pMat = (MATRIX *)((u8 *)PSX_SCRATCH + 0x40);
    s32 oldShift;
    u_int numChildren;
    u_int i;
    s32 flags;
    u_int stateMinusOne;

    oldShift = g_GfxOTDepthShift;
    numChildren = pSkeleton->count - 1;
    pSkeleton++;
    for (i = 0; i < numChildren; i++, pSkeleton++) {
        if (pSkeleton->jointIndex != BATTLE_PART_NO_JOINT && pSkeleton->isVisible != 0) {
            CompMatrix(pMatrix, &pSkeleton->globalTransform, pMat);
            // state - 1 modulo 2^16, so states 0 and 1 both land below 2
            stateMinusOne = pSkeleton->state + 0xFFFF;
            if ((u16)stateMinusOne < 2) {
                pMat->m[0][0] = 0x1000;
                pMat->m[0][2] = 0;
                pMat->m[1][0] = 0;
                pMat->m[1][2] = 0;
                pMat->m[2][0] = 0;
                pMat->m[2][2] = 0x1000;
                if ((s16)pSkeleton->state == 1) {
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
            switch ((s16)pSkeleton->state) {
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
            if ((s16)pSkeleton->state == 0) {
                void *pPart = pMesh->pParts[pSkeleton->jointIndex];

                g_GfxOTDepthShift = 0x10;
                ModelRenderPacket(pPart, pSkeleton->packets[packetIndex], pOt + otIndex - 1,
                                  flags);
            } else {
                g_GfxOTDepthShift = oldShift;
                ModelRenderPacket(pMesh->pParts[pSkeleton->jointIndex],
                                  pSkeleton->packets[packetIndex], pOt, flags);
            }
        }
    }
    g_GfxOTDepthShift = oldShift;
}
s32 BattlePushTargetOutOfCircles(u16 *pPos, s16 *pTarget) {
    s32 result = 0;
    u16 *pSrc = g_BattleCircles;
    s32 i;

    for (i = 0; i < g_BattleCircleCount; i++) {
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
    u16 *pSrc = g_BattleCircles;
    s32 i;

    for (i = 0; i < g_BattleCircleCount; i++) {
        a = *pSrc++;
        b = *pSrc++;
        c = *pSrc++;
        BattlePushActorOutOfCircle(pArg, a, b, BattleGetActorRadius(pArg) + c);
    }
}
typedef struct {
    /* 0x0 */ u_short x;
    /* 0x2 */ u_short y;
} BattleMisc22ScreenPos; // Size: 0x4

// A 12-byte primitive this function only ever chains into the ordering table; the
// caller fills its two code words in.
typedef struct {
    /* 0x0 */ u_long tag;
    /* 0x4 */ u_long code[2];
} BattleMisc22OtPrim; // Size: 0xC

// What func_800A4DB8 raises over a part: the scrolling texture grid it projects the
// vertices of, and the primitives that frame it. The quads come in pairs per bank and
// the flat/gradient ones carry their y coordinates from the projected corners, so the
// caller only has to fill in the x's.
typedef struct {
    /* 0x000 */ u16 scrollX; // advanced by scrollStepX every frame; the texture grid slides with it
    /* 0x002 */ u16 scrollY;
    /* 0x004 */ u16 scrollStepX;
    /* 0x006 */ u16 scrollStepY;
    /* 0x008 */ u16 texPeriod; // the texture is one tile of texPeriod square texels, tiled
    /* 0x00A */ s16 heightOffset; // subtracted from the eye's y before the grid is placed
    /* 0x00C */ s16 farScale; // 8-bit scale of the corner that faces away
    /* 0x00E */ s16 nearScale; // 8-bit scale of the corner that faces the camera
    /* 0x010 */ s16 geomScreen;
    /* 0x012 */ s16 pitchScale;
    /* 0x014 */ s16 unk14;
    /* 0x016 */ s16 topY;
    /* 0x018 */ s16 faceScale; // 12-bit scale of the normal offset that gives the corners
    /* 0x01A */ u8 pad1A[0xA];
    /* 0x024 */ BattleMisc22OtPrim otPrims[4];
    /* 0x054 */ SVECTOR verts[9][9]; // the grid; one row and one column more than the quad count
    /* 0x2DC */ POLY_FT4 quads[2][64];
    /* 0x16DC */ POLY_F4 flatQuads[4];
    /* 0x173C */ POLY_G4 gradQuads[4];
} BattleMisc22RenderData; // Size: 0x17CC

// best 90.76% (2492 bytes, 617 insns; retail 0x9C4 = 2500 bytes, 619 insns).
// THE FRAME IS RETAIL'S EXACTLY NOW (0xE8, vars 0xB0, saves at 0xC0..0xE4): the old
// note called it unreachable, and it took two edits, both in the body below:
//   1. a DEAD `u_long unusedFrame[2]` as the first declaration (lever
//      j-an-unused-local-array-is): retail's 0x10..0x17 is referenced nowhere and an
//      array local gets its slot at expand time. That alone grew the frame 0xE0 -> 0xE8.
//   2. the DECLARATIONS IN RETAIL'S SLOT ORDER - rotGrid, rotFlat, rot, vec, cornerSxy,
//      faceSxy, dir, corner, normal, nclip - because 2.6.0 hands stack slots out in
//      declaration order here (first declared = lowest). With the old order
//      (vec, dir, normal, corner, ...) the +8 went in but the objects stayed permuted
//      and the diff was not a uniform shift (522 -> 398 differing lines).
// WHAT IS LEFT is one callee-saved tie and its knock-on: retail keeps pArg in s3 and
// the addPrim 0xFF000000 mask in s2, this body mirrors them (pArg s2, mask s3), and
// because a2 is spent on the mask the gradQuads[index] block re-loads cornerSxy.x a
// third time ('lhu 0x70(sp)' twice in retail, three times here) and the diff carries
// 2 instructions fewer than retail. Read as quotients (floor_log2(refs)*refs/live) pArg
// outranks the mask ~2.4x, so retail's order contradicts the pseudo set THIS instruction
// stream pins: not source-reachable from here without changing the emitted code.
// Measured and dead this round: permute.sh cannot parse the body at all (the gte_ inline
// asm macros are a syntax error for its C parser), gcc-2.7.2 scores 89.50 on it, and the
// earlier rounds' 8-byte screen-pos record and (x + 0xFF) >> 8 spellings are still worse.
/*
void func_800A4DB8(BattleMisc22RenderData *pArg, SVECTOR *pFrom, SVECTOR *pTo, MATRIX *pMatrix,
                   u_long *pOt, s32 index) {
    u_long unusedFrame[2];
    MATRIX rotGrid;
    MATRIX rotFlat;
    SVECTOR rot;
    VECTOR vec;
    BattleMisc22ScreenPos cornerSxy;
    BattleMisc22ScreenPos faceSxy;
    VECTOR dir;
    SVECTOR corner;
    SVECTOR normal;
    s32 nclip;
    SVECTOR *pVert;
    POLY_FT4 *pQuad;
    s32 angle;
    s32 zoom;
    s32 row;
    s32 col;
    s32 halfPeriod;
    s32 uBase;
    s32 vBase;
    s32 uStart;
    s32 uEnd;
    s32 vStart;
    s32 vEnd;

    if (pArg == 0) {
        return;
    }

    addPrim(pOt, &pArg->otPrims[index + 2]);

    dir.vx = pTo->vx - pFrom->vx;
    dir.vy = 0;
    dir.vz = pTo->vz - pFrom->vz;
    VectorNormalS(&dir, &normal);

    corner.vx = pTo->vx + normal.vx * pArg->faceScale / 0x1000;
    corner.vy = pArg->topY;
    corner.vz = pTo->vz + normal.vz * pArg->faceScale / 0x1000;
    SetRotMatrix(pMatrix);
    SetTransMatrix(pMatrix);
    gte_ldv0(&corner);
    gte_rtps();
    gte_stsxy(&cornerSxy);
    cornerSxy.x = cornerSxy.y;
    if ((s16)cornerSxy.y >= 0xF1) {
        cornerSxy.y = 0xF0;
    }

    corner.vx = pTo->vx + (normal.vx * pArg->faceScale / 0x1000) * pArg->nearScale / 256;
    corner.vy = pArg->topY * pArg->nearScale / 256;
    corner.vz = pTo->vz + (normal.vz * pArg->faceScale / 0x1000) * pArg->nearScale / 256;
    gte_ldv0(&corner);
    gte_rtps();
    gte_stsxy(&faceSxy);
    if ((s16)faceSxy.y >= 0) {
        if ((s16)cornerSxy.y < 0xF0) {
            pArg->gradQuads[index + 2].y0 = cornerSxy.y;
            pArg->gradQuads[index + 2].y1 = cornerSxy.y;
            pArg->gradQuads[index + 2].y2 = faceSxy.y;
            pArg->gradQuads[index + 2].y3 = faceSxy.y;
            addPrim(pOt, &pArg->gradQuads[index + 2]);
        }
    }
    if ((s16)faceSxy.y < 0) {
        faceSxy.y = 0;
    }
    if ((s16)faceSxy.y < 0xF0) {
        pArg->flatQuads[index + 2].y0 = faceSxy.y;
        pArg->flatQuads[index + 2].y1 = faceSxy.y;
        addPrim(pOt, &pArg->flatQuads[index + 2]);
    }

    corner.vx = pTo->vx + (normal.vx * pArg->faceScale / 0x1000) * pArg->farScale / 256;
    corner.vy = pArg->topY * pArg->farScale / 256;
    corner.vz = pTo->vz + (normal.vz * pArg->faceScale / 0x1000) * pArg->farScale / 256;
    gte_ldv0(&corner);
    gte_rtps();
    gte_stsxy(&faceSxy);
    faceSxy.y = (s16)cornerSxy.y * 2 - faceSxy.y - 8;
    if (cornerSxy.x < 0x1E0U && (s16)faceSxy.y < 0xF0) {
        pArg->gradQuads[index].y0 = cornerSxy.x;
        pArg->gradQuads[index].y1 = cornerSxy.x;
        pArg->gradQuads[index].y2 = faceSxy.y;
        pArg->gradQuads[index].y3 = faceSxy.y;
        addPrim(pOt, &pArg->gradQuads[index]);
    }

    vec.vx = pTo->vx - pFrom->vx;
    vec.vy = 0;
    vec.vz = pTo->vz - pFrom->vz;
    angle = ratan2(pTo->vy - pFrom->vy, SquareRoot0(vec.vx * vec.vx + vec.vz * vec.vz));
    // the grid is seen edge on as the part turns away, so the projection shortens with the pitch
    zoom = (angle - 0x100) * pArg->pitchScale / 0x200;
    zoom = zoom * (0x400 - (angle < 0 ? -angle : angle)) / 0x400;
    SetGeomScreen(pArg->geomScreen);
    rot.vx = 0;
    rot.vy = -ratan2(vec.vx, vec.vz);
    rot.vz = 0;
    RotMatrix(&rot, &rotFlat);
    rotFlat.t[0] = rotFlat.t[1] = rotFlat.t[2] = 0;
    rot.vx = zoom;
    rot.vy = -rot.vy;
    RotMatrixYXZ(&rot, &rotGrid);
    VectorNormalS(&vec, &rot);
    rotGrid.t[0] = vec.vx = pFrom->vx + rot.vx * 2;
    rotGrid.t[1] = vec.vy = pFrom->vy / 4 - pArg->heightOffset;
    rotGrid.t[2] = vec.vz = pFrom->vz + rot.vz * 2;
    CompMatrix(&rotGrid, &rotFlat, &rotGrid);
    CompMatrix(pMatrix, &rotGrid, &rotGrid);
    SetRotMatrix(&rotGrid);
    SetTransMatrix(&rotGrid);

    pArg->scrollX += pArg->scrollStepX;
    pArg->scrollY += pArg->scrollStepY;
    uBase = ((s16)pArg->scrollX / 16 + vec.vx / 12) & (pArg->texPeriod - 1);
    vBase = ((s16)pArg->scrollY / 16 + vec.vz / 12) & (pArg->texPeriod - 1);
    halfPeriod = (s16)pArg->texPeriod / 2;

    pQuad = &pArg->quads[index][0];
    pVert = &pArg->verts[0][0];
    for (row = 0; row < 8; row++) {
        vStart = (row & 1) * halfPeriod + vBase;
        vEnd = vStart + halfPeriod - 1;
        for (col = 0; col < 8; col++) {
            gte_ldv3(pVert, pVert + 1, pVert + 9);
            gte_rtpt();
            gte_nclip();
            gte_stopz(&nclip);
            if (nclip >= 0) {
                gte_stsxy3(&pQuad->x0, &pQuad->x1, &pQuad->x2);
                gte_ldv0(pVert + 10);
                gte_rtps();
                gte_stsxy(&pQuad->x3);
                uStart = (col & 1) * halfPeriod + uBase;
                uEnd = uStart + halfPeriod - 1;
                pQuad->v0 = pQuad->v1 = vStart;
                pQuad->v2 = pQuad->v3 = vEnd;
                pQuad->u0 = pQuad->u2 = uStart;
                pQuad->u1 = pQuad->u3 = uEnd;
                addPrim(pOt, pQuad);
            }
            pQuad++;
            pVert++;
        }
        pVert++;
    }

    SetGeomScreen(0x200);
    if ((s16)cornerSxy.y >= 0) {
        pArg->flatQuads[index].y2 = cornerSxy.y;
        pArg->flatQuads[index].y3 = cornerSxy.y;
        addPrim(pOt, &pArg->flatQuads[index]);
    }
    addPrim(pOt, &pArg->otPrims[index]);
}*/
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
