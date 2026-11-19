#include "common.h"
#include "field/actor.h"
#include "system/memory.h"
#include "psyq/libgpu.h"

extern void func_8001E148(SpriteData* pSpriteData);
extern void func_8001E3D8(SpriteData* pSpriteData, void* pArg);
extern void func_8001E9BC(SpriteData* pSpriteData, u_long* pOt);
extern void func_8001EE88(SpriteData* pSpriteData, void* pArg1, void* pArg2);
extern void func_8001F1D4(SpriteData* pSpriteData, u_long* pOt, s32 depth);
extern void SpriteApplyDirTransforms(SpriteData* pSpriteData, s32 index, SpriteAnimPackage* pAnimPackage);
extern void SpriteClearDirTransforms(SpriteData* pSpriteData);

// Rendering-related stuff

// Allocate directions array
void SpriteEnsureDirTransforms(SpriteData* pSpriteData) {
    if (pSpriteData->pBase->pDirTransforms == 0) {
        pSpriteData->pBase->pDirTransforms = HeapAlloc(sizeof(SpriteDirectionTransforms), 0);
        SpriteClearDirTransforms(pSpriteData);
    }
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering", func_8001D53C);

/* Best attempt: 78.37% (1416 bytes, the original is 1452). NOT a wall and not a
   permuter job about register order: the remaining deficit is pure register
   PRESSURE, and it is one callee-saved register too many for gcc to fit.
   Facts a future attempt needs, all confirmed against the .s:
   - This is the FRAME-BUILDING twin of SpriteApplyDirTransforms (matched, above):
     the dir/0x80/0x40 bytecode walk is byte-identical to that function's once the
     source is written the same way. Keep the walk shaped like that one.
   - The frame loop is a do/while, not a for: retail's `bne s7,t0,head` with
     `addiu s2,s2,3` (pEntry += 3) in its DELAY slot only comes out of a do/while
     whose body ends `pFrameData++; i++; pEntry += 3;`.
   - field_0x40's mask is ~0xFC (retail `li v1,-253`), it keeps bits 0 and 1.
   - retail's spriteFlags is a SIGNED 32-bit local: `s32 spriteFlags` gives the
     sra at each of the four shift sites and no re-sign-extension after
     `spriteFlags |= *pSprite << 8`; s16 emits sll16/sra16 around every shift.
   - the g_SpriteVramSlotPair entry is indexed as a BYTE offset: retail is
     `sll 1 / andi 0x1C / addu`, so write `(SVEC2*)((mem_addr)g_SpriteVramSlotPair +
     ((spriteFlags << 1) & 0x1C))`; the tidy `g_SpriteVramSlotPair[(spriteFlags >> 1) & 7]`
     gives sra/andi/sll (3 insns) or the sll16/sra17 extract form.
   - the inline tpage packing in the last two arms is GetTPage/GetClut written out
     by hand (the first arm really does CALL them), so keep the five ORs.
   What is still wrong, measured: retail holds nine values in s0-s7+fp - sprite
   data, anim package, pFrameData, pEntry, pSprite, entryFlags/spriteFlags (they
   SHARE one register), texX, dir and i. This C needs TEN (entryFlags and
   spriteFlags both live across the frame build), so gcc spills pFrameData to the
   stack (and re-bases the register copy to pFrameData+2, which is why the tileX
   store reloads the pointer) and keeps i in memory (extra lw/addiu/sw per
   iteration at the loop end). Tried and measured, none better: an indexed
   `pFrameData[i]` loop (the usual induction-base fix, worse), `*pEntry` instead
   of the entryFlags local at the tail (78.02% - it does free a register but gcc
   spends it on entryFlags anyway), the `== 1 &&` short-circuit form for the
   flags_0xA8/pClutPair test, `(s16)(x & 0x100) >> 4` for retail's
   andi/sll16/sra20 shape (folds to srl/andi), do/while - the current 78.37%.
   A 4-minute permuter run found nothing portable: the three best candidates
   either clobber the loop counter (`if ((i = spriteFlags) & 1)`, `i = 7`) - i.e.
   wrong code - or only re-spell `count` as `flags & 0x3F` (7247 -> 7173).
   The next lever is the one that removes a live value from the frame path: the
   frame code only needs `spriteFlags` for bits 1-3, bit 4 and the >>1/>>9 fields,
   so a version that re-reads `*pSprite` per use (or types the sprite header as a
   union of bytes and a u16) may fit in nine.
*/
/*
extern SVEC2 g_SpriteVramSlotPair[];

void func_8001D53C(SpriteData* pSpriteData, s32 index, SpriteAnimPackage* pAnimPackage) {
    u32 i;
    SpriteFrameData* pFrameData;
    u8* pFrames;
    u8* pBlock;
    u16* pOffsets;
    u8* pEntry;
    u8* pSprite;
    u16* pClutPair;
    SVEC2* pTexEntry;
    u8 flags;
    u8 unkFlag;
    u8 entryFlags;
    s32 spriteFlags;
    u32 count;
    s32 dir;
    s32 clutBase;
    s32 texX;
    u16 texY;
    u16 tpageMode;
    u16 defaultMode;
    u16 texPage;
    u32 color;
    s16 rotZ;

    pFrames = pAnimPackage->pFrames;
    pBlock = (u8*)((mem_addr)((u16*)pFrames)[index] + (mem_addr)pFrames);
    pSpriteData->field_0x36 = pBlock[3] * pSpriteData->scale / 0x1000;
    pSpriteData->field_0x38 = pBlock[1] * pSpriteData->scale / 0x1000;
    flags = *pBlock;
    unkFlag = flags & 0x80;
    count = flags & 0x3F;
    pOffsets = (u16*)(pBlock + 4);
    pEntry = pBlock + (count * 2 + 4);
    pFrameData = pSpriteData->pBase->pCurRenderFramesData;
    texY = pAnimPackage->tex.y;
    defaultMode = *(u8*)((u8*)pSpriteData + 0x3C) >> 5;
    color = *(u32*)&pSpriteData->red;
    dir = 4;
    i = 0;

    if (count != 0) {
        do {
            pFrameData->deltaHeight = 0;
            pFrameData->deltaWidth = 0;
            pFrameData->flags &= ~0x20;
            while (1) {
                entryFlags = *pEntry;
                if (!(entryFlags & 0x80)) {
                    break;
                }
                pEntry++;
                if (entryFlags & 0x40) {
                    dir = entryFlags & 7;
                    if (pSpriteData->pBase->pDirTransforms == 0) {
                        pSpriteData->pBase->pDirTransforms = HeapAlloc(sizeof(SpriteDirectionTransforms), 0);
                        SpriteClearDirTransforms(pSpriteData);
                    }
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
                    if (entryFlags & 4) {
                        pFrameData->flags |= 0x20;
                    }
                    if (entryFlags & 1) {
                        pFrameData->deltaWidth = *pEntry++;
                    }
                    if (entryFlags & 2) {
                        pFrameData->deltaHeight = *pEntry++;
                    }
                }
            }

            pSprite = (u8*)pFrames + *pOffsets++;
            spriteFlags = *pSprite;
            if (spriteFlags & 1) {
                pFrameData->flags |= 8;
                texX = (pAnimPackage->tex.x & 0x3F) >> 1;
            } else {
                pFrameData->flags &= ~8;
                texX = (pAnimPackage->tex.x & 0x3F) >> 2;
            }

            tpageMode = (*pEntry >> 4) & 3;
            clutBase = *pEntry & 0xF;
            pFrameData->colorAndCode = color;
            if (tpageMode == 0) {
                tpageMode = defaultMode;
            }
            if (tpageMode != 0) {
                *(u8*)((u8*)pFrameData + 0x13) |= 2;
                tpageMode--;
            }

            if ((spriteFlags >> 4) & 1) {
                pSprite++;
                spriteFlags |= *pSprite << 8;
                pTexEntry = (SVEC2*)((mem_addr)g_SpriteVramSlotPair + ((spriteFlags << 1) & 0x1C));
                pFrameData->texPage = GetTPage(spriteFlags & 1, (s16)tpageMode, pTexEntry->x, pTexEntry->y);
                pFrameData->clutID = GetClut((spriteFlags >> 1) & 0xF0, ((spriteFlags >> 9) & 0xF) + 0x1CC);
            } else {
                if (((pSpriteData->flags_0xA8 & 1) == 1) &&
                    (pClutPair = *(u16**)((u8*)pSpriteData->field_0x7C + 0x18)) != 0) {
                    texX = (pClutPair[0] & 0x3F) >> 2;
                    texY = pClutPair[1] & 0xFF;
                    texPage = ((spriteFlags & 1) << 7) | ((tpageMode & 3) << 5);
                    texPage |= (pClutPair[1] & 0x100) >> 4;
                    texPage |= (pClutPair[0] & 0x3FF) >> 6;
                    texPage |= (pClutPair[1] & 0x200) << 2;
                    pFrameData->texPage = texPage;
                } else {
                    texPage = ((spriteFlags & 1) << 7) | ((tpageMode & 3) << 5);
                    texPage |= (pAnimPackage->tex.y & 0x100) >> 4;
                    texPage |= (pAnimPackage->tex.y & 0x200) << 2;
                    texPage |= (((pAnimPackage->tex.x + ((spriteFlags << 5) & 0x1C0)) & 0x3FF) >> 6);
                    pFrameData->texPage = texPage;
                }
                pFrameData->clutID =
                    (pAnimPackage->clut.y << 6) | (((pAnimPackage->clut.x + clutBase * 16) >> 4) & 0x3F);
            }

            pFrameData->flags = (pFrameData->flags & ~7) | dir;
            pFrameData->texX = texX + pSprite[1];
            pFrameData->texY = texY + pSprite[2];
            pFrameData->width = pSprite[3];
            pFrameData->height = pSprite[4];
            pFrameData->flags = (pFrameData->flags & ~0x10) | ((*pEntry >> 2) & 0x10);

            if (unkFlag != 0) {
                pFrameData->tileX = pEntry[1] | ((s8)pEntry[2] << 8);
                pFrameData->tileY = pEntry[3] | ((s8)pEntry[4] << 8);
                pEntry += 2;
            } else {
                pFrameData->tileX = (s8)pEntry[1];
                pFrameData->tileY = (s8)pEntry[2];
            }

            pFrameData++;
            i++;
            pEntry += 3;
        } while (i != count);
    }
    pSpriteData->field_0x40 = (pSpriteData->field_0x40 & ~0xFC) | ((i & 0x3F) << 2);
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering", func_8001DAE8);

extern MATRIX g_GfxBaseMatrix;
extern u8 g_IsBattle;
extern u8 g_GfxSpritesUseBaseMatrix;

/* Best attempt: 90.89% (332 bytes, the original is 336). Field/cast shapes are
   right (the `lb` loads want the (s8) offsets, the flags read wants the (u32)
   cast to stay srl, and the `pMat = &g_GfxBaseMatrix` local is required or gcc
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
   Round 166: the body below is this note's C made to compile again (the flags
   read had to become pSpriteData->flags_0xAC.flags) - 90.89% confirmed, and a
   4-minute permuter run (1759 iterations, base 930) found only candidates that
   change what ApplyMatrix/pMatrix see, i.e. wrong code, so nothing to port.
   `pMatrix = &pSpriteData->pBase->transformMatrix` is also the reverse of the
   usual subobject advice: the original keeps the transformMatrix address in s0
   and stores at +0x14, this C keeps pBase and stores at +0x20.

   Round 170: the wall is now diagnosed exactly. Retail's two-insn lui/addiu of
   g_GfxBaseMatrix sits MID-block (right after the /0x1000 math) and the value
   is then held in a callee-saved register (s1), with the call argument copied
   out of it (addu a0,s1,zero). gcc only keeps a symbol-address pseudo in a
   register when its birth is in a different basic block from its uses (i.e.
   declared at the top), and that makes it the longest-lived pseudo, so it
   sorts last and gets s3. Placing the birth anywhere inside the uses' block
   (even at exactly retail's source position, before the position loads) makes
   cse fold the pseudo back into the constant at every use: 86.02%, 332 bytes,
   four lui/addiu pairs, shrunken frame. gcse does not hoist it back either
   (writing ApplyMatrix(&g_GfxBaseMatrix, ...) plus g_GfxBaseMatrix.t[k] gives
   the same 86.02%). Other 336-byte shapes exist but rotate registers instead:
   `xOffset = (s8)offsetX << n` as one expression 89.13%, pMatrix computed
   before the call 88.15%, `do { } while (0);` after the declaration moves the
   whole prologue below the g_IsBattle load 81.98%, a pBase local live across
   the call 79.11%/344 bytes. A second 4-minute permuter run (1669 iterations,
   base 930, best 690) again only found candidates that pass an uninitialised
   pMatrix to ApplyMatrix, i.e. wrong code.
*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering", func_8001E148);

/* Best attempt at the C above (90.89%):
void func_8001E148(SpriteData* pSpriteData) {
    MATRIX* pMat = &g_GfxBaseMatrix;
    SVECTOR position;
    VECTOR vec;
    MATRIX* pMatrix;
    s32 n;
    s32 yOffset;
    s32 xOffset;

    if ((g_IsBattle != 0) || (g_GfxSpritesUseBaseMatrix != 0)) {
        SpriteUpdateTransformMatrix(pSpriteData);
    }

    n = (pSpriteData->field_0x40 >> 8) & 0x1F;
    yOffset = (s8)pSpriteData->pBase->offsetY;
    yOffset <<= n;
    xOffset = (s8)pSpriteData->pBase->offsetX;
    xOffset <<= n;

    if (((u32)pSpriteData->flags_0xAC.flags >> 2) & 1) {
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



void SpriteDrawFrame(SpriteData* pSpriteData, void* pArg) {
    func_8001E148(pSpriteData);
    func_8001E3D8(pSpriteData, pArg);

    if ((pSpriteData->field_0x3C_0 >> 2) & 1) {
        func_8001E9BC(pSpriteData, pArg);
    }
}

void SpriteDrawFrameKeepTop(SpriteData* pSpriteData, void* pArg1, void* pArg2) {
    func_8001E148(pSpriteData);
    func_8001EE88(pSpriteData, pArg1, pArg2);

    if ((pSpriteData->field_0x3C_0 >> 2) & 1) {
        func_8001E9BC(pSpriteData, pArg1);
    }
}

void SpriteDrawFrameKeepBottom(SpriteData* pSpriteData, void* pArg1, void* pArg2) {
    func_8001E148(pSpriteData);
    func_8001F1D4(pSpriteData, pArg1, pArg2);

    if ((pSpriteData->field_0x3C_0 >> 2) & 1) {
        func_8001E9BC(pSpriteData, pArg1);
    }
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering", func_8001E3D8);

extern SVECTOR D_8004FAD8[4];
extern u16 D_8004FAF8[16];
extern void* g_GfxCurWorkBuffer;
extern void* g_GfxCurWorkBufferEnd;

// SpriteData's two animation flag words are also read a byte at a time (the
// frame count, the plane mask), which the bitfields in field/actor.h cannot say.
typedef struct {
    /* 0x00 */ u8 unk0[0x3C];
    /* 0x3C */ SpriteFlagWord flags_0x3C;
    /* 0x40 */ SpriteFlagWord flags_0x40;
} SpriteRenderFlagWords; // Size: 0x44

/* Best attempt: 76.49% (1208 bytes, the original is 1196). The whole body is a
   verified transcription; what is left is statement order and two allocator
   choices. Facts a future attempt needs, all confirmed against the .s:
   - The prim IS a POLY_FT4 (this tree's libgpu.h has the shipped per-corner
     layout: u0/v0/clut at 0xC, u1/v1/tpage at 0x14, u2/v2/pad at 0x1C,
     u3/v3/pad at 0x24, sizeof 0x28). setlen(pPrim, 9) is the tag len byte at
     3 and setcode 0x2C at 7 -> `setPolyFT4(pPrim)` then `setRGB0(pPrim,0,0,0)`.
   - 0x8004FAD8 is NOT g_BattleSpriteQuadCorners (that one is 0x8004FB98).
     It is 4 SVECTORs of scratch (D_8004FAD8), written x into .vx and the
     screen-vertical into .vz because the quad lies in the XZ plane (vy is
     never written, the .sdata is zeroed). 0x8004FAF8 is a u16[16] of
     1,2,4..0x8000 used as the per-layer mask.
   - The sprite's plane byte is byte 1 of the 0x3C word (lbu 0x3D) and the
     frame count is byte 0 of the 0x40 word >> 2 (lbu 0x40); the shared header
     has bitfields/u32 there, so both need the local view below.
   - The frame count is re-read for the loop bound (lbu 0x40 each iteration) and
     compared with `bne i,count`, so write the expression in the for test, not
     the local.
   - INDEXED access is required: with `i++, pFrameData++` gcc re-bases the
     induction register to pFrameData+7 and every offset goes negative (1216
     bytes, 70.98%); `pFrameData[i].field` keeps the base and retail's literal
     offsets 0,2,4,6,7,8,9,0x14.
   - `visible` must stay in a register: the cast `(SpriteRenderFlagWords*)
     pSpriteData` has to be written inline at each use, not held in a pointer
     local (a live pointer local costs a callee-saved register, spills the flag
     to 0x78 and grows the frame to 0xA8).
   Measured and still wrong (each re-diffed):
   - retail emits width/height/n interleaved: width, then n, then width<<=n,
     then height, then height<<=n, then tileX, tileY - that order is worth
     74.90 -> 76.48 and is what the body below does.
   - the quad y stores are in the order [2],[3],[0],[1] in both arms.
   - STILL WRONG, in address order: (a) retail splits the g_GfxBaseMatrix
     address into `lui s0,%hi` + two `addiu ...%lo` (one for the MATRIX copy in
     a1, one for ApplyMatrix' argument in s0) where this C CSEs it into one
     callee-saved register; (b) retail copies the matrix in 3/3/2-word groups,
     this C in 4/4; (c) the two y averages need `(s16)(y0+y1)/2` to get
     retail's lhu+sll16/sra16 (without the cast gcc uses lh/sra and the body
     drops to 1188 bytes / 72.65%); (d) retail keeps the 0xFFFFFF/0xFF000000
     masks of addPrim call-clobbered inside the loop (lui/ori each iteration),
     this C hoists them into a saved register; (e) the uv right/bottom edges
     are `(u - 1) + extent` in retail (`addiu v0,v0,255; addu v1,v1,v0`), this C
     folds them to `(u + extent) - 1`; (f) the x-quad sum `tileX + width` is
     CSE'd into one addu by retail and emitted twice here.
*/

/*
// Draws the sprite's frames as one rotated textured quad each: the base matrix
// scaled by the sprite's scale, offset by its projected position, then the four
// corners of a frame rotated through it. Frames are skipped when the mask for
// their layer is set in the sprite's plane byte.
void func_8001E9BC(SpriteData* pSpriteData, u_long* pOt) {
    MATRIX mat;
    SVECTOR vec;
    VECTOR vecResult;
    VECTOR scale;
    SVECTOR* pVerts;
    SpriteFrameData* pFrameData;
    POLY_FT4* pPrim;
    u32 spriteFlags;
    u32 frameFlags;
    s32 count;
    s32 lastPlane;
    u8 visible;
    s32 i;
    s32 n;
    s32 tileX;
    s32 tileY;
    s32 width;
    s32 height;
    s32 depth;
    s32 flag;

    mat = g_GfxBaseMatrix;

    vec.vx = pSpriteData->position.x >> 16;
    vec.vy = pSpriteData->position.y >> 16;
    vec.vz = pSpriteData->position.z >> 16;

    scale.vx = pSpriteData->scale;
    scale.vz = 0;
    scale.vy = pSpriteData->scale / 2;
    ScaleMatrixL(&mat, &scale);

    vec.vy = pSpriteData->field_0x84;
    ApplyMatrix(&g_GfxBaseMatrix, &vec, &vecResult);
    mat.t[0] += vecResult.vx;
    mat.t[1] += vecResult.vy;
    mat.t[2] += vecResult.vz;
    SetRotMatrix(&mat);
    SetTransMatrix(&mat);

    count = ((SpriteRenderFlagWords*)pSpriteData)->flags_0x40.bytes[0] >> 2;
    if ((u8*)g_GfxCurWorkBuffer + count * sizeof(POLY_FT4) >= (u8*)g_GfxCurWorkBufferEnd) {
        return;
    }
    pFrameData = pSpriteData->pBase->pCurRenderFramesData;
    if (count == 0) {
        return;
    }

    pVerts = D_8004FAD8;
    lastPlane = -1;
    for (i = 0; i != ((SpriteRenderFlagWords*)pSpriteData)->flags_0x40.bytes[0] >> 2; i++) {
        frameFlags = pFrameData[i].flags;
        if (lastPlane != (frameFlags & 7)) {
            lastPlane = frameFlags & 7;
            visible = (D_8004FAF8[lastPlane] & ((SpriteRenderFlagWords*)pSpriteData)->flags_0x3C.bytes[1]) == 0;
        }
        if (visible == 0) {
            continue;
        }

        width = (s16)(pFrameData[i].width + (s8)pFrameData[i].deltaWidth);
        n = (((SpriteRenderFlagWords*)pSpriteData)->flags_0x40.flags >> 8) & 0x1F;
        width <<= n;
        height = (s16)(pFrameData[i].height + (s8)pFrameData[i].deltaHeight);
        height <<= n;
        tileX = pFrameData[i].tileX << n;
        tileY = pFrameData[i].tileY << n;

        spriteFlags = ((SpriteRenderFlagWords*)pSpriteData)->flags_0x3C.flags;
        if ((spriteFlags >> 3) & 1) {
            width = -width;
            tileX = -tileX;
        }
        if (((spriteFlags >> 4) & 1) != ((frameFlags >> 5) & 1)) {
            height = -height;
            tileY = -tileY;
        }

        if (!((frameFlags >> 4) & 1)) {
            pVerts[0].vx = tileX;
            pVerts[1].vx = tileX + width;
            pVerts[2].vx = tileX + width;
            pVerts[3].vx = tileX;
        } else {
            pVerts[0].vx = tileX + width;
            pVerts[1].vx = tileX;
            pVerts[2].vx = tileX;
            pVerts[3].vx = tileX + width;
        }

        if (!((frameFlags >> 5) & 1)) {
            pVerts[2].vz = tileY + height;
            pVerts[3].vz = tileY + height;
            pVerts[0].vz = tileY;
            pVerts[1].vz = tileY;
        } else {
            pVerts[2].vz = tileY;
            pVerts[3].vz = tileY;
            pVerts[0].vz = tileY + height;
            pVerts[1].vz = tileY + height;
        }

        pPrim = g_GfxCurWorkBuffer;
        g_GfxCurWorkBuffer = pPrim + 1;
        setPolyFT4(pPrim);
        setRGB0(pPrim, 0, 0, 0);

        RotAverage4(&pVerts[0], &pVerts[1], &pVerts[2], &pVerts[3], (long*)&pPrim->x0, (long*)&pPrim->x1,
            (long*)&pPrim->x3, (long*)&pPrim->x2, &depth, &flag);

        pPrim->y0 = pPrim->y1 = (s16)(pPrim->y0 + pPrim->y1) / 2;
        pPrim->y2 = pPrim->y3 = (s16)(pPrim->y2 + pPrim->y3) / 2;
        pPrim->tpage = pFrameData[i].texPage;
        pPrim->clut = pFrameData[i].clutID;
        pPrim->u0 = pFrameData[i].texX;
        pPrim->v0 = pFrameData[i].texY;
        pPrim->u1 = pFrameData[i].texX + pFrameData[i].width - 1;
        pPrim->v1 = pFrameData[i].texY;
        pPrim->u2 = pFrameData[i].texX;
        pPrim->v2 = pFrameData[i].texY + pFrameData[i].height - 1;
        pPrim->u3 = pFrameData[i].texX + pFrameData[i].width - 1;
        pPrim->v3 = pFrameData[i].texY + pFrameData[i].height - 1;

        addPrim(pOt, pPrim);
    }
}
*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering", func_8001E9BC);

s32 SpriteAnimPackageTestFlag8000(u8* pFlags) {
    return pFlags[1] >> 7;
}

s32 SpriteAnimPackageGetFrameCount(u16* pFlags) {
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
     tx} or {tx+w, tx, tx, tx+w} into g_BattleSpriteQuadCorners[0..3].vx. Element 0 is stored
     through the SYMBOL (%lo(g_BattleSpriteQuadCorners)($t1)) and elements 1-3 through a pointer
     local (+8/+0x10/+0x18), which is what keeps the hi part of the address live
     across the call and produces the spill/reload at 0x30($sp).
   - yMin/yMax = min/max(tileY, tileY+height) via a sign test on height; if
     (z < yMin) continue; clip = (z < yMax) ? yMax - z : 0; height -= clip;
     if (height < 0) tileY -= clip; then flags bit 5 selects {ty, ty, ty+h, ty+h}
     or the reverse for .vy.
   - RotAverage4(g_BattleSpriteQuadCorners, +1, +2, +3, (long*)&pPrim->x0, &pPrim->x1, &pPrim->x3,
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
   `extern SVECTOR g_BattleSpriteQuadCorners[4];` (0x20 bytes, defined by
   asm/slus_006.64/data/3F290.sdata.s, so no data definition is needed here).

   extern SVECTOR g_BattleSpriteQuadCorners[4];

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

       pVerts = g_BattleSpriteQuadCorners;
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
               g_BattleSpriteQuadCorners[0].vx = tileX;
               pVerts[1].vx = tileX + width;
               pVerts[2].vx = tileX + width;
               pVerts[3].vx = tileX;
           } else {
               g_BattleSpriteQuadCorners[0].vx = tileX + width;
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
     from psyq/libgte.h via field/actor.h). g_BattleSpriteQuadCorners is
     `extern SVECTOR g_BattleSpriteQuadCorners[4]`; it is already in the sdata asm, do not
     define it here.
   - element 0 of g_BattleSpriteQuadCorners is stored through the SYMBOL (%lo(g_BattleSpriteQuadCorners)($t1))
     and elements 1-3 through the pointer local `pVerts = g_BattleSpriteQuadCorners`; that is
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

   Round 166: the body below now COMPILES against today's headers and scores the
   same 77.38% (896 bytes against retail's 892). What had to be fixed for that:
   `func_8001F1D4`'s extern is really (SpriteData*, u_long* pOt, s32 depth) -
   the old (void*, void*) declaration conflicted with the body - POLY_FT4 needs
   psyq/libgpu.h, and g_BattleSpriteQuadCorners / g_GfxCurWorkBuffer / g_GfxCurWorkBufferEnd
   stay declared at file scope (outside this comment).

 extern SVECTOR g_BattleSpriteQuadCorners[4];
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

    pVerts = g_BattleSpriteQuadCorners;
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
            g_BattleSpriteQuadCorners[0].vx = tileX;
            pVerts[1].vx = tileX + width;
            pVerts[2].vx = tileX + width;
            pVerts[3].vx = tileX;
        } else {
            g_BattleSpriteQuadCorners[0].vx = tileX + width;
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
s16 g_SpriteVramSlotRow;
s16 g_SpriteVramSlotXOffset;

SVEC2* SpriteNextVramSlot(SVEC2* pDest, s32 amount) {
    SVEC2 pos;
    u16 offset;

    if (g_SpriteVramSlotXOffset + amount >= 0x41) {
        g_SpriteVramSlotXOffset = 0;
        g_SpriteVramSlotRow++;
        if (g_SpriteVramSlotRow >= 3) {
            g_SpriteVramSlotRow = 0;
        }
    }
    offset = g_SpriteVramSlotXOffset;
    pos.x = offset + 0x300;
    pos.y = g_SpriteVramSlotRow * 0x40 + 0x140;
    *pDest = pos;
    g_SpriteVramSlotXOffset = offset + amount;
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
   ruled out for the whole unit -- under gcc-2.7.2-cdk-psx SpriteDrawFrameKeepTop and
   SpriteDrawFrameKeepBottom (both matched at 100% under 2.7.2) drop to 88.57%,
   SpriteEnsureDirTransforms to 97.14%, and SpriteDrawFrame/SpriteAnimPackageTestFlag8000/SpriteNextVramSlot fail
   to compile at all.
   Instruction stream is identical; only the temp
   assignment differs (original: pSpriteData->t1, texPageBits->a3, color->a1,
   pFrameData->a0, i->t0; this C: t0, a3, a1, v1, a0). Reordering the locals has
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
*/
/* Best attempt: 97.38% (was 96.50). Two permuter edits in the body below are
   load-bearing and look gratuitous -- keep both:
     1. `color = flags;` then `texPageBits = (color >> 5) & 7;` (96.50 -> 97.38).
     2. `do { texPageBits--; } while (0);` (96.50 -> 97.12): the extra block
        scope is what puts texPageBits in a3, the original's register.
   The instruction stream is identical; the remaining delta is a 3-cycle of the
   live values. Original: param t1, color a1, texPageBits a3, pFrameData a0,
   i t0, shift temp v1, a2 free. Ours: param t0, color a1, texPageBits a3,
   pFrameData v1, i a0, shift temp a2, t1 free. a0 is the induction pointer on
   both sides and the value load order matches, so this is the allocator only.
   Two permuter runs from score 115: 115 -> 105, then no gain in 867 iterations.
   Re-tried by hand this round, none better: u32 i (97.38), i declared before
   pFrameData (97.38), pFrameData pointer local in the loop (97.38), loop guard
   read once and ++i in the body (97.38), `i < bound` (91.95), pFrameData
   assigned before tex (85.20). gcc-2.7.2-cdk scores 97.12, so not the preset.
*/
/*
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

    color = flags;
    texPageBits = (color >> 5) & 7;
    if (texPageBits != 0) {
        do { texPageBits--; } while (0);
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
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering", func_8001F6B0);

void SpriteApplyDirTransforms(SpriteData* pSpriteData, s32 index, SpriteAnimPackage* pAnimPackage) {
    u16* pFrameOffsets;
    u8* pBlock;
    u8* pEntry;
    u8 flags;
    u8 unkFlag;
    u32 count;
    u8 entryFlags;
    s16 rotZ;
    s32 dir;
    s32 i;

    pFrameOffsets = pAnimPackage->pFrames;
    // mem_addr arithmetic: the plain pointer form reassigns the base register (99.85%)
    pBlock = (u8*)(pFrameOffsets[index] + (mem_addr)pFrameOffsets);
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
                    // (dir + arr) keeps retail's offset-first addu; dirTransforms[dir] reverses it
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

// Animation frame and tile data stuff
void SpriteApplyFrameDirTransforms(SpriteData* pSpriteData, s32 index, SpriteAnimPackage* pAnimPackage) {
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
