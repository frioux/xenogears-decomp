#include "common.h"
#include "field/actor.h"
#include "system/memory.h"

extern void func_8001E148(SpriteData* pSpriteData);
extern void func_8001E3D8(SpriteData* pSpriteData, void* pArg);
extern void func_8001E9BC(SpriteData* pSpriteData, void* pArg);
extern void func_8001EE88(SpriteData* pSpriteData, void* pArg1, void* pArg2);
extern void func_8001F1D4(SpriteData* pSpriteData, void* pArg1, void* pArg2);
extern void SpriteApplyDirTransforms(SpriteData* pSpriteData, s32 index, SpriteAnimPackage* pAnimPackage);
extern void SpriteClearDirTransforms(SpriteData* pSpriteData);

// Rendering-related stuff

// Allocate directions array
void func_8001D4E8(SpriteData* pSpriteData) {
    if (pSpriteData->pBase->pDirTransforms == 0) {
        pSpriteData->pBase->pDirTransforms = HeapAlloc(sizeof(SpriteDirectionTransforms), 0);
        SpriteClearDirTransforms(pSpriteData);
    }
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering", func_8001D53C);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering", func_8001DAE8);

extern MATRIX D_8004FBB8;
extern u8 D_800591AD;
extern u8 D_800591AE;

/* Best attempt: 90.89% (332 bytes, the original is 336). Field/cast shapes are
   right (the `lb` loads want the (s8) offsets, the flags read wants the (u32)
   cast to stay srl, and the `pMat = &D_8004FBB8` local is required or gcc
   re-forms lui/addiu for every t[] access after the ApplyMatrix call). The
   remaining delta is gcc's callee-saved priority: the original is s0 sprite
   data, s1 matrix address, s2 xOffset, s3 yOffset; this C gives the matrix
   address s3 because its live range starts at the top of the function.
   Reordering the declarations changes nothing (gcc ranks pseudos by references
   and live length), assigning pMat only next to ApplyMatrix makes gcc drop the
   pseudo and re-form the address per use (86.02%), and splitting the shifted
   offsets into extra locals reaches the original's 336 bytes but still hoists
   the matrix address (86.93%). Also 86.76% under gcc-2.7.2-cdk-psx, so it is
   not the preset.

void func_8001E148(SpriteData* pSpriteData) {
    MATRIX* pMat = &D_8004FBB8;
    SVECTOR position;
    VECTOR vec;
    MATRIX* pMatrix;
    s32 n;
    s32 yOffset;
    s32 xOffset;

    if ((D_800591AD != 0) || (D_800591AE != 0)) {
        SpriteUpdateTransformMatrix(pSpriteData);
    }

    n = (pSpriteData->field_0x40 >> 8) & 0x1F;
    yOffset = (s8)pSpriteData->pBase->offsetY;
    yOffset <<= n;
    xOffset = (s8)pSpriteData->pBase->offsetX;
    xOffset <<= n;

    if (((u32)pSpriteData->flags_0xAC >> 2) & 1) {
        xOffset = -xOffset;
    }

    yOffset = yOffset * pSpriteData->scale / 0x1000;
    xOffset = xOffset * pSpriteData->scale / 0x1000;

    position.vx = pSpriteData->position.x >> 0x10;
    position.vy = pSpriteData->position.y >> 0x10;
    position.vz = pSpriteData->position.z >> 0x10;
    ApplyMatrix(pMat, &position, &vec);
    pMatrix = &pSpriteData->pBase->transformMatrix;
    pMatrix->t[0] = pMat->t[0] + vec.vx + xOffset;
    pMatrix->t[1] = pMat->t[1] + vec.vy + yOffset;
    pMatrix->t[2] = pMat->t[2] + vec.vz;
    SetRotMatrix(pMatrix);
    SetTransMatrix(pMatrix);
}
*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering", func_8001E148);

void func_8001E298(SpriteData* pSpriteData, void* pArg) {
    func_8001E148(pSpriteData);
    func_8001E3D8(pSpriteData, pArg);

    if ((pSpriteData->field_0x3C_0 >> 2) & 1) {
        func_8001E9BC(pSpriteData, pArg);
    }
}

void func_8001E2F8(SpriteData* pSpriteData, void* pArg1, void* pArg2) {
    func_8001E148(pSpriteData);
    func_8001EE88(pSpriteData, pArg1, pArg2);

    if ((pSpriteData->field_0x3C_0 >> 2) & 1) {
        func_8001E9BC(pSpriteData, pArg1);
    }
}

void func_8001E368(SpriteData* pSpriteData, void* pArg1, void* pArg2) {
    func_8001E148(pSpriteData);
    func_8001F1D4(pSpriteData, pArg1, pArg2);

    if ((pSpriteData->field_0x3C_0 >> 2) & 1) {
        func_8001E9BC(pSpriteData, pArg1);
    }
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering", func_8001E3D8);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering", func_8001E9BC);

s32 func_8001EE68(u8* pFlags) {
    return pFlags[1] >> 7;
}

s32 func_8001EE74(u16* pFlags) {
    return (*pFlags >> 9) & 0x3F;
}

/* Attempt (0.00%: cc1 emits an EMPTY body for this shape - the .s comes out as
   just a frame + j $ra, and maspsx then leaves the stale .o, which objdiff
   reports as "0.00% (? bytes)". Removing halves of the body does not change
   that, so the trigger is structural and unidentified. Everything below is a
   verified transcription of the assembly except for the two open questions.

   func_8001EE88 is the twin of func_8001F1D4 (same prologue, same RotAverage4
   call, mirrored clip arithmetic), so whichever gets matched first hands the
   other its shape.

   Verified from the .s (address order):
   - n = (field_0x40 >> 8) & 0x1F; z = arg3 << n; count = (field_0x40 >> 2) & 0x3F
   - early out when (u8*)g_GfxCurWorkBuffer + count*40 >= g_GfxCurWorkBufferEnd,
     then when count == 0; pFrameData = pBase->pCurRenderFramesData is loaded
     BEFORE the branches (a0 = pBase, lw 0x30(a0)).
   - loop bound is a BYTE reload: (*(u8*)((u8*)pSpriteData + 0x40)) >> 2, not
     the word-based `count`; pFrameData advances by 0x18 (sizeof SpriteFrameData).
   - pPrim = g_GfxCurWorkBuffer++ (POLY_FT4, 0x28); code byte 3 = 9; then
     *(u32*)&pPrim->r0 = pFrameData->colorAndCode; tpage = texPage; clut = clutID.
   - tileX(tileX s16), width = lbu(6) + (s8)lb(8) deltaWidth, tileY, height =
     lbu(7) + (s8)lb(9) deltaHeight, all <<= n; flips are field_0x3C bits 3 and 4
     (bit 3 negates width and tileX, bit 4 negates height and tileY).
   - then SpriteFrameData->flags 0x14 bit 4 selects the x quads: {tx, tx+w, tx+w,
     tx} or {tx+w, tx, tx, tx+w} into D_8004FB98[0..3].vx. Element 0 is stored
     through the SYMBOL (%lo(D_8004FB98)($t1)) and elements 1-3 through a pointer
     local (+8/+0x10/+0x18), which is what keeps the hi part of the address live
     across the call and produces the spill/reload at 0x30($sp).
   - yMin/yMax = min/max(tileY, tileY+height) via a sign test on height; if
     (z < yMin) continue; clip = (z < yMax) ? yMax - z : 0; height -= clip;
     if (height < 0) tileY -= clip; then flags bit 5 selects {ty, ty, ty+h, ty+h}
     or the reverse for .vy.
   - RotAverage4(D_8004FB98, +1, +2, +3, (long*)&pPrim->x0, &pPrim->x1, &pPrim->x3,
     &pPrim->x2, &zDepth, &flag) - note x3 before x2; the two stack locals are at
     sp+0x28 and sp+0x2C.
   - after the call: n is reloaded, s1 (clip) is sra'd by n, texY = lbu(5) loaded
     before the height>0 test (else arm also does texY -= s1), texHeight =
     lbu(7) - s1 in BOTH arms (gcc really duplicates the load);
     texX = lbu(4); if (pPrim->x3 < pPrim->x0) { if (--texX < 0) { texX = 0;
     width = lbu(6) - 1; } }
   - UV stores in this exact order: u1=u3=texX+width, u0=u2=texX, v0=v1=texY,
     v2=v3=texY+texHeight.
   - tail is addPrim(pOt, pPrim): tag = (tag & 0xFF000000) | (*pOt & 0xFFFFFF),
     then *pOt = (*pOt & 0xFF000000) | ((u32)pPrim & 0xFFFFFF).
   Needs #include "psyq/libgpu.h" for POLY_FT4 and
   `extern SVECTOR D_8004FB98[4];` (0x20 bytes, defined by
   asm/slus_006.64/data/3F290.sdata.s, so no data definition is needed here).

   extern SVECTOR D_8004FB98[4];

   void func_8001EE88(SpriteData* pSpriteData, u_long* pOt, s32 depth) {
       SVECTOR* pVerts;
       SpriteFrameData* pFrameData;
       POLY_FT4* pPrim;
       s32 n;
       s32 count;
       s32 z;
       s32 i;
       s32 zDepth;
       s32 flag;
       s32 tileX;
       s32 tileY;
       s32 width;
       s32 height;
       s32 texX;
       s32 texY;
       s32 texHeight;
       s32 clip;
       u32 spriteFlags;
       s32 yMin;
       s32 yMax;

       n = (pSpriteData->field_0x40 >> 8) & 0x1F;
       z = depth << n;
       count = (pSpriteData->field_0x40 >> 2) & 0x3F;
       pFrameData = pSpriteData->pBase->pCurRenderFramesData;
       if ((u8*)g_GfxCurWorkBuffer + count * sizeof(POLY_FT4) >= (u8*)g_GfxCurWorkBufferEnd) {
           return;
       }
       if (count == 0) {
           return;
       }

       pVerts = D_8004FB98;
       for (i = 0; i < (*(u8*)((u8*)pSpriteData + 0x40) >> 2); i++, pFrameData++) {
           pPrim = g_GfxCurWorkBuffer;
           g_GfxCurWorkBuffer = pPrim + 1;
           pPrim->code = 9;
           *(u32*)&pPrim->r0 = pFrameData->colorAndCode;
           pPrim->tpage = pFrameData->texPage;
           pPrim->clut = pFrameData->clutID;
           tileX = pFrameData->tileX;
           width = pFrameData->width;
           width += (s8)pFrameData->deltaWidth;
           tileY = pFrameData->tileY;
           height = pFrameData->height;
           height += (s8)pFrameData->deltaHeight;
           n = (pSpriteData->field_0x40 >> 8) & 0x1F;
           width <<= n;
           height <<= n;
           tileX <<= n;
           spriteFlags = pSpriteData->field_0x3C;
           tileY <<= n;
           if (spriteFlags & 8) {
               width = -width;
               tileX = -tileX;
           }
           if (spriteFlags & 0x10) {
               height = -height;
               tileY = -tileY;
           }
           if (!(pFrameData->flags & 0x10)) {
               D_8004FB98[0].vx = tileX;
               pVerts[1].vx = tileX + width;
               pVerts[2].vx = tileX + width;
               pVerts[3].vx = tileX;
           } else {
               D_8004FB98[0].vx = tileX + width;
               pVerts[1].vx = tileX;
               pVerts[2].vx = tileX;
               pVerts[3].vx = tileX + width;
           }

           if (height > 0) {
               yMin = tileY;
               yMax = tileY + height;
           } else {
               yMin = tileY + height;
               yMax = tileY;
           }
           if (z < yMin) {
               continue;
           }
           if (z < yMax) {
               clip = yMax - z;
           } else {
               clip = 0;
           }
           height -= clip;
           if (height < 0) {
               tileY -= clip;
           }

           if (!(pFrameData->flags & 0x20)) {
               pVerts[0].vy = tileY;
               pVerts[1].vy = tileY;
               pVerts[2].vy = tileY + height;
               pVerts[3].vy = tileY + height;
           } else {
               pVerts[0].vy = tileY + height;
               pVerts[1].vy = tileY + height;
               pVerts[2].vy = tileY;
               pVerts[3].vy = tileY;
           }

           RotAverage4(pVerts, &pVerts[1], &pVerts[2], &pVerts[3], (long*)&pPrim->x0, (long*)&pPrim->x1,
               (long*)&pPrim->x3, (long*)&pPrim->x2, &zDepth, &flag);

           n = (pSpriteData->field_0x40 >> 8) & 0x1F;
           texY = pFrameData->texY;
           clip >>= n;
           if (height > 0) {
               texHeight = pFrameData->height - clip;
           } else {
               texY -= clip;
               texHeight = pFrameData->height - clip;
           }
           texX = pFrameData->texX;
           if (pPrim->x3 < pPrim->x0) {
               if (--texX < 0) {
                   texX = 0;
                   width = pFrameData->width - 1;
               }
           }
           pPrim->u1 = texX + width;
           pPrim->u3 = texX + width;
           pPrim->u0 = texX;
           pPrim->u2 = texX;
           pPrim->v0 = texY;
           pPrim->v1 = texY;
           pPrim->v2 = texY + texHeight;
           pPrim->v3 = texY + texHeight;

           pPrim->tag = (pPrim->tag & 0xFF000000) | (*pOt & 0xFFFFFF);
           *pOt = (*pOt & 0xFF000000) | ((u32)pPrim & 0xFFFFFF);
       }
   }
*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering", func_8001EE88);

/* Best attempt: 77.38% (896 bytes, the original is 892). The whole function is a
   verified transcription and the frame is the original 0x60; what is left is
   register allocation plus one folded base pointer (last paragraph).
   Facts a future attempt needs, all confirmed against the .s:
   - `setlen(pPrim, 9)` IS where the 9 goes: byte 3 of the tag (P_TAG.len, bits
     24-31), NOT pPrim->code (byte 7). The parked func_8001EE88 attempt above
     writes `pPrim->code = 9`, which lands in byte 7 - that one mistake may be
     why that function never got past 0%.
   - `extern void* g_GfxCurWorkBuffer;` and `extern void* g_GfxCurWorkBufferEnd;`
     must be declared in this unit: including system/animation.h turns the loads
     into `lw $v0,0($gp)`, the original uses lui/lw (%hi/%lo).
   - POLY_FT4 and setlen need #include "psyq/libgpu.h" (RotAverage4/SVECTOR come
     from psyq/libgte.h via field/actor.h). D_8004FB98 is
     `extern SVECTOR D_8004FB98[4]`; it is already in the sdata asm, do not
     define it here.
   - element 0 of D_8004FB98 is stored through the SYMBOL (%lo(D_8004FB98)($t1))
     and elements 1-3 through the pointer local `pVerts = D_8004FB98`; that is
     what keeps the hi half live and spills it to 0x30($sp) across the call.
   - width must be re-loaded from pFrameData->width AFTER the RotAverage4 call
     (the original reloads lbu 0x6(s3) there); keeping one local across the call
     grows the frame to 0x70.
   - the clip arithmetic here is the mirror of func_8001EE88: yMin/yMax from the
     sign of height, `if (z > yMax) continue;`, `clip = (z > yMin) ? z - yMin : 0`,
     and `tileY += clip` is duplicated inside BOTH arms of the height test.
   Still wrong at 77.38%: gcc folds the u8 frame fields onto a second base
   (`addu $s3,$s6,6`, then accesses 0/1/2/3 and 10/-4) where the original uses
   $s3 = pFrameData with offsets 0,2,4..9. That costs one callee-saved register,
   so pOt gets $t1 instead of $fp and is spilled to 0x30($sp) every iteration.

 extern SVECTOR D_8004FB98[4];
 extern void* g_GfxCurWorkBuffer;
 extern void* g_GfxCurWorkBufferEnd;

void func_8001F1D4(SpriteData* pSpriteData, u_long* pOt, s32 depth) {
    SVECTOR* pVerts;
    SpriteFrameData* pFrameData;
    POLY_FT4* pPrim;
    s32 n;
    s32 count;
    s32 z;
    s32 i;
    s32 zDepth;
    s32 flag;
    s32 tileX;
    s32 tileY;
    s32 width;
    s32 height;
    s32 texX;
    s32 texY;
    s32 texHeight;
    s32 clip;
    u32 spriteFlags;
    s32 yMin;
    s32 yMax;

    n = (pSpriteData->field_0x40 >> 8) & 0x1F;
    z = depth << n;
    count = (pSpriteData->field_0x40 >> 2) & 0x3F;
    pFrameData = pSpriteData->pBase->pCurRenderFramesData;
    if ((u8*)g_GfxCurWorkBuffer + count * sizeof(POLY_FT4) >= (u8*)g_GfxCurWorkBufferEnd) {
        return;
    }
    if (count == 0) {
        return;
    }

    pVerts = D_8004FB98;
    for (i = 0; i != (*(u8*)((u8*)pSpriteData + 0x40) >> 2); i++, pFrameData++) {
        pPrim = g_GfxCurWorkBuffer;
        g_GfxCurWorkBuffer = pPrim + 1;
        setlen(pPrim, 9);
        *(u32*)&pPrim->r0 = pFrameData->colorAndCode;
        pPrim->tpage = pFrameData->texPage;
        pPrim->clut = pFrameData->clutID;
        tileX = pFrameData->tileX;
        width = pFrameData->width;
        width += (s8)pFrameData->deltaWidth;
        tileY = pFrameData->tileY;
        height = pFrameData->height;
        height += (s8)pFrameData->deltaHeight;
        n = (pSpriteData->field_0x40 >> 8) & 0x1F;
        width <<= n;
        height <<= n;
        tileX <<= n;
        spriteFlags = *(u32*)((u8*)pSpriteData + 0x3C);
        tileY <<= n;
        if (spriteFlags & 8) {
            width = -width;
            tileX = -tileX;
        }
        if (spriteFlags & 0x10) {
            height = -height;
            tileY = -tileY;
        }
        if (!(pFrameData->flags & 0x10)) {
            D_8004FB98[0].vx = tileX;
            pVerts[1].vx = tileX + width;
            pVerts[2].vx = tileX + width;
            pVerts[3].vx = tileX;
        } else {
            D_8004FB98[0].vx = tileX + width;
            pVerts[1].vx = tileX;
            pVerts[2].vx = tileX;
            pVerts[3].vx = tileX + width;
        }

        if (height > 0) {
            yMin = tileY;
            yMax = tileY + height;
        } else {
            yMin = tileY + height;
            yMax = tileY;
        }
        if (z > yMax) {
            continue;
        }
        clip = (z > yMin) ? z - yMin : 0;
        if (height > 0) {
            tileY += clip;
            height -= clip;
        } else {
            tileY += clip;
            height += clip;
        }

        if (!(pFrameData->flags & 0x20)) {
            pVerts[0].vy = tileY;
            pVerts[1].vy = tileY;
            pVerts[2].vy = tileY + height;
            pVerts[3].vy = tileY + height;
        } else {
            pVerts[0].vy = tileY + height;
            pVerts[1].vy = tileY + height;
            pVerts[2].vy = tileY;
            pVerts[3].vy = tileY;
        }

        RotAverage4(pVerts, &pVerts[1], &pVerts[2], &pVerts[3], (long*)&pPrim->x0, (long*)&pPrim->x1,
            (long*)&pPrim->x3, (long*)&pPrim->x2, &zDepth, &flag);

        n = (pSpriteData->field_0x40 >> 8) & 0x1F;
        clip >>= n;
        if (height > 0) {
            texY = pFrameData->texY + clip;
            texHeight = pFrameData->height - clip;
        } else {
            texY = pFrameData->texY - clip;
            texHeight = pFrameData->height - clip;
        }
        texX = pFrameData->texX;
        width = pFrameData->width;
        if (pPrim->x3 < pPrim->x0) {
            if (--texX < 0) {
                texX = 0;
                width--;
            }
        }
        pPrim->u1 = texX + width;
        pPrim->u3 = texX + width;
        pPrim->v0 = texY;
        pPrim->v1 = texY;
        pPrim->u0 = texX;
        pPrim->u2 = texX;
        pPrim->v2 = texY + texHeight;
        pPrim->v3 = texY + texHeight;

        pPrim->tag = (pPrim->tag & 0xFF000000) | (*pOt & 0xFFFFFF);
        *pOt = (*pOt & 0xFF000000) | ((u32)pPrim & 0xFFFFFF);
    }
}*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering", func_8001F1D4);

// .sdata
s16 D_80059194;
s16 D_80059196;

SVEC2* func_8001F530(SVEC2* pDest, s32 amount) {
    SVEC2 pos;
    u16 offset;

    if (D_80059196 + amount >= 0x41) {
        D_80059196 = 0;
        D_80059194++;
        if (D_80059194 >= 3) {
            D_80059194 = 0;
        }
    }
    offset = D_80059196;
    pos.x = offset + 0x300;
    pos.y = D_80059194 * 0x40 + 0x140;
    *pDest = pos;
    D_80059196 = offset + amount;
    return pDest;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering", func_8001F5BC);
/* Best attempt: 95.16%. UPDATE: this C scores 98.52% under gcc-2.7.2-cdk-psx
   too, so the preset is not the answer here; the miss is the allocator.
   Also tried: loading pPackage->pFrames after the `index = pFrame[4]` byte (the
   original's load order) -- 91.38%, so the hoisted pFrames load is correct.
   The instruction stream (including the multiply/divide
   sequences) is identical; only the register allocation differs. The original
   keeps the sprite data in a1 and loads its 5th argument lazily inside the
   second division, this version hoists that pointer to t1 and keeps the sprite
   in t0.

void func_8001F5BC(SpriteData* pSpriteData, s32 unusedIndex, s32* pOutX, s32* pOutY, s32* pOutZ) {
    s32 index;
    SpriteAnimPackage* pPackage;
    u8* pAnimations;
    u16* pFrames;
    u8* pFrame;

    pPackage = pSpriteData->pVramData;
    pFrames = pPackage->pFrames;
    pAnimations = pPackage->pAnimations;
    pAnimations += *(u16*)(pAnimations + 2);
    pFrame = pAnimations + *(u16*)(pAnimations + 4);

    index = pFrame[4];
    if (index != 0) {
        index--;
    }
    if (pFrames[index] < index) {
        index = 0;
    }
    pFrame = (u8*)pFrames + ((u16*)((u8*)pFrames + 2))[index];

    *pOutY = pFrame[3] * pSpriteData->scale / 0x1000;
    *pOutZ = pFrame[1] * pSpriteData->scale / 0x1000;
    *pOutX = pFrame[2] * pSpriteData->scale / 0x1000;
}
*/

/* Best attempt: 96.50%. UPDATE: this C scores 97.12% under gcc-2.7.2-cdk-psx,
   so the preset is not the answer; the miss is the allocator. Round r12: cdk is
   ruled out for the whole unit -- under gcc-2.7.2-cdk-psx func_8001E2F8 and
   func_8001E368 (both matched at 100% under 2.7.2) drop to 88.57%,
   func_8001D4E8 to 97.14%, and func_8001E298/func_8001EE68/func_8001F530 fail
   to compile at all.
   Instruction stream is identical; only the temp
   assignment differs (original: pSpriteData->t1, texPageBits->a3, color->a1,
   pFrameData->a0, i->t0; this C: a3, t0, a2, v1, a0). Reordering the locals has
   no effect at all (re-confirmed). The sprite reaching t1 rather than a3 is the
   whole difference: a0 goes to the flags word, so the parameter has to be copied
   out of its incoming register.
   Round 3 (session 4) tried 18 more source shapes and every preset: gcc-2.6.0
   -O2 95.00, -O3 95.00 (2.7.2 and cdk), -G0 does not build; the shapes
   (pre-shifted texPageBits, tied srl/andi, pointer-increment loop, pPackage and
   pBase temps, u16/s32 types, an extra `type` temp, no texPage local) all land
   79-96.5. Clue for the next attempt: a2 is NEVER used in the original, so its
   block 1 held one more block-local pseudo than any shape tried here -- that is
   what would push the parameter out to t1.
   Re-checked round r12: the compiled stream is instruction-for-instruction the
   original's; the only delta is a permutation of the five live values
   (param/texPageBits/color/pFrameData/i): original t1/a3/a1/a0/t0, ours
   a3/t0/a2/v1/a0. Both sides copy the parameter out of a0 at entry because the
   block-local `flags` load takes a0; only the copy's destination differs.
*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering", func_8001F6B0);
/* Best attempt: 96.50% (see the note above the INCLUDE_ASM).
void func_8001F6B0(SpriteData* pSpriteData) {
    SpriteFrameData* pFrameData;
    u32 flags;
    u32 color;
    u32 texPageBits;
    u16 texPage;
    s32 i;
    SVEC2 tex;

    flags = *(u32*)((u8*)pSpriteData + 0x3C);
    if ((flags & 3) != 1) {
        return;
    }

    texPageBits = (flags >> 5) & 7;
    if (texPageBits != 0) {
        texPageBits--;
    }
    tex = ((SpriteAnimPackage*)pSpriteData->pVramData)->tex;
    color = *(u32*)&pSpriteData->red;
    pFrameData = pSpriteData->pBase->pCurRenderFramesData;
    for (i = 0; i != (*(u8*)((u8*)pSpriteData + 0x40) >> 2); i++) {
        texPage = pFrameData[i].texPage;
        pFrameData[i].colorAndCode = color;
        pFrameData[i].texPage = (texPage & 0xFF9F) | (texPageBits << 5);
    }
}
*/

void SpriteApplyDirTransforms(SpriteData* pSpriteData, s32 index, SpriteAnimPackage* pAnimPackage) {
    u8* pFrames;
    u8* pBlock;
    u8* pEntry;
    u8 flags;
    u8 unkFlag;
    u32 count;
    u8 entryFlags;
    s16 rotZ;
    s32 dir;
    s32 i;

    pFrames = pAnimPackage->pFrames;
    pBlock = (u8*)((mem_addr)((u16*)pFrames)[index] + (mem_addr)pFrames);
    flags = *pBlock;
    i = 0;
    unkFlag = flags & 0x80;
    count = flags & 0x3F;
    pEntry = pBlock + (count * 2 + 4);
    if (count != 0) {
        for (;;) {
            entryFlags = *pEntry;
            if (entryFlags & 0x80) {
                pEntry++;
                if (entryFlags & 0x40) {
                    dir = entryFlags & 7;
                    if (pSpriteData->pBase->pDirTransforms == 0) {
                        pSpriteData->pBase->pDirTransforms = HeapAlloc(sizeof(SpriteDirectionTransforms), 0);
                        SpriteClearDirTransforms(pSpriteData);
                    }
                    if (entryFlags & 0x20) {
                        *(u8*)((mem_addr)pSpriteData->pBase->pDirTransforms + dir * 8) = *pEntry++;
                        *(u8*)((mem_addr)pSpriteData->pBase->pDirTransforms + (dir * 8 + 1)) = *pEntry++;
                    }
                    if (entryFlags & 0x10) {
                        rotZ = *pEntry++ << 4;
                        *(s16*)((mem_addr)pSpriteData->pBase->pDirTransforms + (dir * 8 + 6)) = rotZ;
                    } else {
                        *(s16*)((mem_addr)pSpriteData->pBase->pDirTransforms + (dir * 8 + 6)) = 0;
                    }
                } else {
                    if (entryFlags & 1) {
                        pEntry++;
                    }
                    if (entryFlags & 2) {
                        pEntry++;
                    }
                }
            } else {
                if (unkFlag != 0) {
                    pEntry += 2;
                }
                i++;
                if (i == count) {
                    break;
                }
                pEntry += 3;
            }
        }
    }
}

// Animation frame and tile data stuff
void func_8001F8E8(SpriteData* pSpriteData, s32 index, SpriteAnimPackage* pAnimPackage) {
    u8* pFrames;
    u8* pBlock;
    u8* pEntry;
    u16 header;
    u8 flags;
    u8 unkFlag;
    u32 count;
    u8 entryFlags;
    s32 dir;
    s32 i;
    s16 rotZ;
    VECTOR vec; /* unused: retail keeps these 0x10 stack bytes */

    pFrames = pAnimPackage->pFrames;
    header = *(u16*)pFrames;
    if (index < (header & 0x1FF) + 1) {
        if (header & 0x8000) {
            SpriteApplyDirTransforms(pSpriteData, index, pAnimPackage);
            return;
        }
        pBlock = (u8*)((mem_addr)((u16*)pFrames)[index] + (mem_addr)pFrames);
        flags = *pBlock;
        i = 0;
        unkFlag = flags & 0x80;
        count = flags & 0x3F;
        pEntry = pBlock + (count * 4 + 6);
        if (count != 0) {
            for (;;) {
                entryFlags = *pEntry;
                if (entryFlags & 0x80) {
                    pEntry++;
                    if (entryFlags & 0x40) {
                        if (pSpriteData->pBase->pDirTransforms == 0) {
                            pSpriteData->pBase->pDirTransforms = HeapAlloc(sizeof(SpriteDirectionTransforms), 0);
                            SpriteClearDirTransforms(pSpriteData);
                        }
                        dir = entryFlags & 7;
                        if (entryFlags & 0x20) {
                            (dir + pSpriteData->pBase->pDirTransforms->dirTransforms)->translationX = *pEntry++;
                            (dir + pSpriteData->pBase->pDirTransforms->dirTransforms)->translationY = *pEntry++;
                        }
                        if (entryFlags & 0x10) {
                            rotZ = *pEntry++ << 4;
                            (dir + pSpriteData->pBase->pDirTransforms->dirTransforms)->rotationZ = rotZ;
                        } else {
                            (dir + pSpriteData->pBase->pDirTransforms->dirTransforms)->rotationZ = 0;
                        }
                    } else {
                        if (entryFlags & 1) {
                            pEntry++;
                        }
                        if (entryFlags & 2) {
                            pEntry++;
                        }
                    }
                } else {
                    if (unkFlag != 0) {
                        pEntry += 2;
                    }
                    i++;
                    if (i == count) {
                        break;
                    }
                    pEntry += 3;
                }
            }
        }
    }
}

// GraphicsDrawPauseLetters and func_8001FB30 were built by a cdk-compiler TU;
// they live in system/rendering_pause_text.c, which carries that preset.
