#include "common.h"
#include "field/actor.h"
#include "system/memory.h"
#include "psyq/libgpu.h"

extern void func_8001E148(SpriteData* pSpriteData);
extern void func_8001E3D8(SpriteData* pSpriteData, void* pArg);
extern void func_8001E9BC(SpriteData* pSpriteData, u_long* pOt);
extern void func_8001EE88(SpriteData* pSpriteData, u_long* pOt, s32 depth);
extern void func_8001F1D4(SpriteData* pSpriteData, u_long* pOt, s32 depth);
extern void SpriteApplyDirTransforms(SpriteData* pSpriteData, s32 index, SpriteAnimPackage* pAnimPackage);
extern void SpriteClearDirTransforms(SpriteData* pSpriteData);
extern void GfxQueueImageTransfer(u_long* pAddr, s32 x, s32 y, s32 width, s16 height);

// Rendering-related stuff

// The source rectangle of one animation frame on the animation texture, in
// pixels: the loop below fills x/y from the frame's descriptor entry and the
// palette upload fills the same fields with the clut's position and size.
typedef struct {
    /* 0x0 */ u16 x;
    /* 0x2 */ u16 y;
    /* 0x4 */ u16 w;
    /* 0x6 */ u16 h;
} SpriteFrameRect; // Size: 0x8

// Allocate directions array
void SpriteEnsureDirTransforms(SpriteData* pSpriteData) {
    if (pSpriteData->pBase->pDirTransforms == 0) {
        pSpriteData->pBase->pDirTransforms = HeapAlloc(sizeof(SpriteDirectionTransforms), 0);
        SpriteClearDirTransforms(pSpriteData);
    }
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering", func_8001D53C);

/* PARKED 80.76% (1436 bytes; retail 1452, 359 vs 363 insns, frame 0x80 vs retail
   0x68). Round 430 measured the residual to ONE construct; everything else in the
   body below is instruction-for-instruction retail's shape.
   THE RESIDUAL: gcc derives a SECOND induction base for pFrameData at +2
   ('addu $18,$fp,2' in the preheader, then BOTH $fp and $18 are bumped by 24 at the
   loop end). That costs $fp, so `i` - retail's $s7 - is in a stack slot instead
   (lw/addu/sw of 0x10(sp) at the loop end) and every sp-relative operand differs.
   Retail has ONE base: $s0 = pFrameData, displacements 0x0 (tileX), 0x2 (tileY),
   0x4..0x7, 0x8, 0x9, 0xA, 0xC, 0x10, 0x14.
   Register census, both bodies 10 saved (ra, fp, s0-s7) + 16 home:
     retail = pFrameData s0, entryFlags/spriteFlags s1, pEntry s2, pSprite s3,
              texX s4, pSpriteData s5, dir s6, i s7, pAnimPackage fp;
              stack: pFrames 0x10, pOffsets 0x14, texY 0x18, defaultMode 0x20,
              color 0x28, count 0x2C, unkFlag 0x30, a spilled pSpriteData 0x38.
     this body = same minus i (memory) plus the +2 base in s2; its spilled
              pSpriteData (pOwner below) sits at 0x50. So the SOURCE pseudo set is
              now retail's - only the base split differs.
   MEASURED THIS ROUND (each was a scored run; keep them, do not redo):
   - retail really does hold TWO source pseudos of pSpriteData: $s5 is used for
     pBase->pDirTransforms in the walk and field_0x40 at the end, while the clut arm
     reads flags_0xA8 and field_0x7C through a RELOAD of 0x38(sp). Adding pOwner
     (used only in the clut arm) is what moved 79.36 -> 80.76% and 1424 -> 1436
     bytes; retail's 0x38 slot is that pseudo, not a spill of the parameter.
   - The base split is chosen by REFERENCE COUNT, never by the loop's own pointer:
     moving the tile stores onto pOwner (semantic probe) drops the +2 base but gcc
     then picks +0x14 ('lw 0($18)', 'sb -11($18)', 'sb -12($18)') - 356 insns /
     1424 bytes / 80.04%, i still in memory. So freeing $fp alone does not buy `i`:
     gcc simply keeps `i` in memory (biv elimination never runs here) and retail's
     `lw count-from-0x2C(sp)` per iteration is the eliminated-biv shape.
   - `u16* pTile = (u16*)pFrameData; pTile[0]/pTile[1]` is CSE'd back into
     pFrameData: 80.76% / 1436, identical to the field form. Not a lever.
   - Flipping the tile block to else-first (semantics-preserving) = 77.00% (1432).
   - do/while with i++ in the body vs for(...; i++) ties exactly (80.76% / 1436);
     do/while kept because retail's 'bne s7,t0' carries 'addiu s2,s2,3' in its slot.
   - The permuter is NOT the next lever (a 4-minute run in round 415 found only
     counter-clobbering candidates); the prize is 'i' in $s2, which is a source-shape
     question: something must stop loop.c creating the (reg+2) giv while keeping the
     pointer's own value live. Ideas not yet tried: (a) split the tile stores into a
     separate 'SpriteFrameData* pFr = pFrameData;' declared INSIDE the loop so the
     two bases have different giv origins; (b) type the frame's leading pair
     (tileX/tileY) as a 2-element s16 array member so the second store is
     pFrameData->tiles[1] (index form, maybe not giv-ised); (c) make count
     'volatile'-free but read through a u8 pointer so the comparison is on memory.
   Anatomy and the rest (frame slots, arm shapes, the five hand-packed ORs, the
   s32 spriteFlags, the byte-offset on g_SpriteVramSlotPair, the signed
   /0x1000 divisions) is unchanged from earlier rounds and still correct.
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
    SpriteAnimPackage* pCurPackage;
    s32 texX;
    u16 texY;
    u16 tpageMode;
    u16 defaultMode;
    u16 texPage;
    u32 color;
    s16 rotZ;
    SpriteData* pOwner;

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
    pOwner = pSpriteData;
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
                if (((pOwner->flags_0xA8 & 1) == 1) &&
                    (pClutPair = *(u16**)((u8*)pOwner->field_0x7C + 0x18)) != 0) {
                    pClutPair = (u16*)((mem_addr)pClutPair + ((spriteFlags << 1) & 0x1C));
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

/* Best attempt: 80.81% (1652 bytes, the original is 1632). NOT a register-order
   park and not a permuter job about statement order: the residual is the FRAME.
   Retail is 0x78 with vars= 56; this body is 0x98 with vars= 88, so every
   sp-relative operand differs and objdiff cannot climb. Facts confirmed against
   the .s (all of them are needed, do not "clean" them up):
   - retail spills pAnimPackage to its own home slot (sw a2,0x80(sp)) and reloads
     it 4 times, and keeps pBlock in $fp. This body keeps pAnimPackage in $s7 and
     spills pBlock, which is the whole cascade. An extra `pCurPackage =
     pAnimPackage;` inside the loop (kept below) is what moved this body 76.6 ->
     80.5 by splitting that pseudo; the remaining two spills it needs are pBlock
     and a rematerialised copy of pFrameData.
   - retail local slots, in order: 0x18 tex, 0x20 rect (8 bytes), 0x28 pFrames,
     0x2C defaultMode, 0x30 color, 0x34 (tex.x&0x3F)<<2, 0x38 (tex.x&0x3F)<<1,
     0x3C (u8)tex.y, 0x40 count, 0x48 unkFlag. Those three precomputed atlas
     values are SOURCE locals (they are computed before the beqz on count, in the
     preheader block, so they are not loop.c hoists) and they must land on exactly
     those slots, which is the part this body still misses (it puts them at
     0x58/0x60/0x68).
   - the texX arms need the MIXED spelling kept below:
     `texX8 + (rect.x * 2)` / `texX4 + (rect.x * 4)`. Both-shift
     (`(a<<1)+(b<<1)`) and both-multiply spellings make gcc reassociate to
     `(a+b)<<n` and the precomputed value disappears (measured, probe-verified).
   - pSpriteData->field_0x40 is cleared with TWO ands on one load (0xFFFDFFFF
     then 0xFFF7FFFF): two statements on the lvalue give two loads and two
     stores, one expression folds to 0xFFF5FFFF. A `flags` local spelled as
     three statements is what produces retail exactly.
   - the rect at 0x20 is UNSIGNED (u16 fields): the stores are lhu/sh and the
     reloads are lhu with no sign extension, so RECT (short) is wrong there; it
     is also filled by the palette upload and per-frame by the descriptor.
   - the palette upload block really does leave dead stores: rect.x/rect.y/
     rect.w/rect.h are written from pAnimPackage->clut and *pPalette << 4 and
     then the call passes freshly loaded expressions, not the struct fields.
   - *pPalette must be read through a u16 pointer (lhu) and the upload address
     built as `pPalette + (*pPalette * (u16 at 0x3E & 0xF0)) + 2` in u16 units;
     that reproduces `sll 20 / sra 16` on the width argument.
   Two permuter runs on the body below (base 9854 -> 7546 -> 7110, i.e.
   76.6 -> 80.8) produced the pCurPackage split and the texX reuse for tex.y,
   both applied. A third run found only `count = flags;`, which drops the 0x3F
   mask - wrong code, rejected.
   Round 384 (frame root-caused further; all of this is measured, and the three
   spellings above were re-verified):
   - retail's `sw $a2,0x80(sp)` is the o32 register-argument HOME SLOT, not a
     spill: the home area starts at the callee's frame_size, so a0 is frame+0,
     a1 +4, a2 +8. Retail's frame is 0x78, and 0x78+8 = 0x80, i.e. pAnimPackage
     is a MEMORY parameter in its home slot, reloaded (not spilled) at each of
     its four uses. Same mechanism as the matched battle/main/misc30
     BattleCreateAnimTask (frame 0x58, `sw $a0,0x58(sp)`, `lw $a1,0x58(sp)`) - so
     a plain pointer parameter CAN live in the home slot; what is unmeasured is
     what makes cc1 2.7.2 choose it there instead of in a vars slot. Passing the
     package BY VALUE reproduces the store and the reloads exactly
     (`sw $a2,152(sp)` = frame+8, then `lw $t1,152(sp)` + deref, and vars drops
     88 -> 80), BUT a 0x14-byte struct is passed as two registers plus stack, so
     cc1 also emits `sw $a3,156(sp)` and reads struct bytes 8..0x14 out of the
     caller's stack - bytes SpriteAnimFrameListFlush never passes (it loads only
     a2). So by value is WRONG CODE here and measures 79.58%, not an answer.
   - the remaining 32 bytes are slot ALIGNMENT, not slot count: this body's vars
     slots are 0x28,0x30,0x38,...,0x68 - every spill slot 8-byte aligned, 8
     apart - while retail's are 0x28,0x2C,0x30,0x34,... 4 apart. Retail's ONLY
     8-aligned slot is unkFlag (visible as the 0x44 hole before its 0x48 slot),
     which is gcc's BLKmode slot rule (align = BIGGEST_ALIGNMENT = 8). So retail's
     other ten values sit in mode-sized (4-aligned) slots and ours all sit in
     BLKmode/reload spills. Count the frame with `.frame` in the cc1 output (or
     `grep vars=` in build/src/<unit>.c.s) after every change; the .i.lreg /
     .i.greg dumps next to the input are produced by cc1 -dl -dg and list each
     pseudo's refs, live length and "crosses N calls".
   - retail has NO pCurPackage local: the GetClut call reloads the home slot a
     fourth time (`lw $t1,0x80(sp)`, `lh $v0,8($t1)`, `lh $a1,0xA($t1)`). Writing
     that faithfully (drop the local, `GetClut(pAnimPackage->clut.x + ...)` in
     place) measures WORSE - 77.19% / 1672 bytes - because it re-merges the
     pseudo, so the split below is load-bearing despite being un-retail-like.
   - not a preset: gcc 2.6.0 gives the identical vars= 88, and gcc 2.7.2-cdk
     gives vars= 64 (frame 0x80) - closer but still not 56, and the unit cannot
     switch anyway.
*/
/*
void func_8001DAE8(SpriteData* pSpriteData, u16 animFrame, SpriteAnimPackage* pAnimPackage) {
    SpriteFrameData* pFrameData;
    u8* pFrames;
    u8* pBlock;
    u8* pSprite;
    u8* pEntry;
    u16* pPalette;
    u16* pDescriptor;
    SVEC2 tex;
    SpriteFrameRect rect;
    u32 flags;
    u32 count;
    u32 spriteFlags;
    u32 vramWidth;
    u16 defaultMode;
    u16 tpageMode;
    u16 texPage;
    u32 color;
    s32 clutBase;
    SpriteAnimPackage* pCurPackage;
    s32 texX;
    s32 texX4;
    s32 texX8;
    s32 dir;
    s32 i;
    u8 entryFlags;
    u8 unkFlag;

    flags = pSpriteData->field_0x40;
    flags &= 0xFFFDFFFF;
    flags &= 0xFFF7FFFF;
    pSpriteData->field_0x40 = flags;
    pFrames = pAnimPackage->pFrames;
    pFrameData = pSpriteData->pBase->pCurRenderFramesData;
    if (animFrame < (*(u16*)pFrames & 0x1FF) + 1) {
        flags = *(u32*)((u8*)pSpriteData + 0x3C);
        if ((flags >> 30) & 1) {
            *(u32*)((u8*)pSpriteData + 0x3C) = flags & 0xBFFFFFFF;
            pPalette = pAnimPackage->pPalette;
            if (*(u16*)pPalette != 0) {
                rect.x = pAnimPackage->clut.x;
                rect.y = pAnimPackage->clut.y;
                rect.h = 1;
                rect.w = *pPalette << 4;
                GfxQueueImageTransfer((u_long*)(pPalette + (*pPalette * (*(u16*)((u8*)pSpriteData + 0x3E) & 0xF0)) + 2),
                                      pAnimPackage->clut.x, pAnimPackage->clut.y, (s16)(*pPalette << 4), 1);
            }
        }
        if (*(u16*)pFrames & 0x8000) {
            func_8001D53C(pSpriteData, animFrame, pAnimPackage);
            return;
        }
        pBlock = (u8*)((mem_addr)((u16*)pFrames)[animFrame] + (mem_addr)pFrames);
        pPackage = pSpriteData->pVramData;
    tex = pPackage->tex;
        if (((pSpriteData->field_0x40 >> 13) & 0xF) == 0xE) {
            SpriteNextVramSlot(&tex, pBlock[4]);
        }
        pSpriteData->field_0x36 = pBlock[3] * pSpriteData->scale / 0x1000;
        pSpriteData->field_0x38 = pBlock[1] * pSpriteData->scale / 0x1000;
        defaultMode = *(u8*)((u8*)pSpriteData + 0x3C) >> 5;
        color = *(u32*)&pSpriteData->red;
        texX4 = (tex.x & 0x3F) << 2;
        texX8 = (tex.x & 0x3F) << 1;
        texX = tex.y;
        flags = *pBlock;
        unkFlag = flags & 0x80;
        count = flags & 0x3F;
        pDescriptor = (u16*)(pBlock + 6);
        pEntry = pBlock + count * 4 + 6;
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
                            (dir + pSpriteData->pBase->pDirTransforms->dirTransforms)->rotationZ = *pEntry++ << 4;
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

                pCurPackage = pAnimPackage;
                pSprite = (u8*)(pFrames + pDescriptor[0] * 4);
                rect.x = pDescriptor[1] & 0x1F;
                rect.y = (pDescriptor[1] >> 5) & 0x3F;
                pDescriptor += 2;
                spriteFlags = *(u16*)(pSprite + 2);
                if (spriteFlags & 1) {
                    pFrameData->flags |= 8;
                    vramWidth = *pSprite >> 1;
                    texX = texX8 + (rect.x * 2);
                } else {
                    pFrameData->flags &= ~8;
                    vramWidth = *pSprite >> 2;
                    texX = texX4 + (rect.x * 4);
                }
                pFrameData->texX = texX;
                pFrameData->flags = (pFrameData->flags & ~7) | dir;
                pFrameData->texY = rect.y + texX;
                pFrameData->width = *pSprite;
                pFrameData->height = pSprite[1];
                pFrameData->flags = (pFrameData->flags & ~0x10) | ((*pEntry >> 2) & 0x10);
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
                texPage = ((spriteFlags & 1) << 7) | ((tpageMode & 3) << 5);
                texPage |= (tex.y & 0x100) >> 4;
                texPage |= (tex.x & 0x3FF) >> 6;
                texPage |= (tex.y & 0x200) << 2;
                pFrameData->texPage = texPage;
                pFrameData->clutID = GetClut(pCurPackage->clut.x + clutBase * 16, pCurPackage->clut.y);
                GfxQueueImageTransfer((u_long*)(pSprite + 4), (s16)(tex.x + rect.x), (s16)(tex.y + rect.y),
                                      vramWidth, pSprite[1]);
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
        GfxQueueImageTransfer(0, tex.x, tex.y, pBlock[4], pBlock[5]);
    }
}

*/

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

extern SVECTOR g_SpriteQuadCorners[4];
extern SVECTOR g_BattleSpriteQuadCorners[4];
extern u16 g_SpritePlaneMasks[16];
extern void* g_GfxCurWorkBuffer;
extern void* g_GfxCurWorkBufferEnd;

// SpriteData's two animation flag words are also read a byte at a time (the
// frame count, the plane mask), which the bitfields in field/actor.h cannot say.
typedef struct {
    /* 0x00 */ u8 unk0[0x3C];
    /* 0x3C */ SpriteFlagWord flags_0x3C;
    /* 0x40 */ SpriteFlagWord flags_0x40;
} SpriteRenderFlagWords; // Size: 0x44

// The whole 0x3C flag word: its bit tests need the raw word, which the bitfields
// in field/actor.h only expose as separate fields.
#define SpriteFlags0x3C(pSpriteData) ((u32)((SpriteRenderFlagWords*)(pSpriteData))->flags_0x3C.flags)


/* Best attempt parked below: 89.01% (1504 bytes; retail is 1508 = 377
   instructions, so the body is ONE instruction short). This is the ROUND 349
   base: the same body with the deltaHeight hoist reverted; the 89.45% / 1496
   byte variant (three short) is described in the notes below.
   Rounds 346-349 took the body 84.08 -> 86.78 -> 89.01/89.45 in single measured
   steps; read the residues below before changing anything: one of the earlier
   fixes is a MISCOMPILE, which is why the best body was not the best number.
   The tail (prim build, quad corners, RotTransPers4, uv, addPrim) is the same
   code as func_8001E9BC / func_8001EE88 / func_8001F1D4 in this file.

   Facts confirmed by measurement, each of which is load-bearing:
   - The frame loop is a DO/WHILE, not a `for`: retail has no top exit test
     (it enters at .L8001E494 straight from the preheader), and the `for` form
     makes gcc duplicate the loop exit test (+3 instructions, 1524 bytes).
   - The loop tail must be `} while (++i != <count>);` with NO `i++;` in the
     body: retail is `lw t4,i(sp); addiu t4,t4,1; bne t4,v0,.L; sw t4,i(sp)`
     (the store fills the branch delay slot). With `i++;` in the body and the
     bare `i != ...` test gcc stores i first, RELOADS it for the test and leaves
     a nop in the delay slot - 4 bytes longer, 0.9 points lower.
   - INDEXED access pFrameData[i] is REQUIRED. With `i++, pFrameData++` and
     `pFrameData->`, gcc re-bases the induction register to pFrameData+7 and
     every field goes negative; because the pointer is then also spilled the
     frame grows to 0xB0. With pFrameData[i] the frame is retail's exact 0x98
     and there is no re-basing (same finding as the func_8001E9BC note).
   - The mirror test `(flags_0xAC >> 2) & 1` must be HOISTED into a local
     (flipX) assigned BEFORE the two offset computations: retail loads pBase,
     field_0x40 and field_0xAC back to back and only then does the lb's. Reading
     the flag inside the `if` instead puts the 0xAC load after offsetX's shift
     and costs a load-delay nop (+4 bytes).
   - `pTransform->translationX != 0 || pTransform->translationY != 0` DOES
     compile to retail's single `lhu 0(a0)` plus `bnez` (gcc merges the two
     adjacent s8 loads), so the realistic spelling is right.
   - The scaling must stay `y = y * scale / 0x1000;` - that is what emits
     retail's `bgez / addiu 0xFFF / sra 12` pair, and y (translationY) is
     computed before x (translationX), both negated by bit 3 of field_0x3C.
   - ONE `lastPlane` variable carries both the current and the remembered plane
     (initialised -1, compared against `pFrameData[i].flags & 7`): retail keeps
     it in one register and indexes the plane mask, the transform table and the
     tail's `<< 2` from it. Separate `mode`/`prevMode` locals cost a second
     callee-saved register and spill prevMode to the stack.
   - The 0x3C bit tests need the WHOLE u32 word: `(field_0x3C_0 >> 3) & 1` on
     the actor.h bitfield emits `and 0xFFFFF` + `sra` with the 0xFFFFF constant
     spilled and reloaded, where retail is `lw / srl 3 / andi 1`. Hence the
     SpriteFlags0x3C macro above.
   - The frame-flag tests need a u32 cast: `pFrameData[i].flags >> 4 & 1` on the
     s32 field gives `sra`, retail has `srl` (bits 4 and 5 both).
   - offsetX/offsetY (the sprite's scaled offsets, subtracted from the projected
     corners) and transX/transY (the layer transform's scaled translation, added
     to mat.t[]) must be SEPARATE locals: both pairs are live at the same time
     in retail (s6/fp versus s0/s1) and reusing one pair is also a semantic bug.
   - `n` must be recomputed in every block that shifts by it (retail re-reads
     field_0x40 in the matrix block and again in the quad block). One `n`
     assigned before the loop is loop-invariant, so it takes a callee-saved
     register and grows the frame.
   - the 0xFFFFFF mask (addPrim) is hoisted to a callee-saved register by both.
   - The QUAD-CORNER x stores of element 0 go through the SYMBOL in retail
     (`lui t4,%hi` + `sh %lo(g_BattleSpriteQuadCorners)($t4)`) in the x arm and
     in the offset block, while the y stores and elements 1-3 use the pVerts
     register. The two ARM stores can be spelled `g_BattleSpriteQuadCorners[0].vx`
     (worth +2.8 points) - but the OFFSET-BLOCK one CANNOT: with that spelling
     cc1 2.7.2 replaces the `&pVerts[2]` argument of RotTransPers4 with
     `la $6,g_BattleSpriteQuadCorners` (element 0 instead of 2), i.e. a silent
     miscompile that objdiff cannot see because the operand is a relocation.
     It scored 88.05% at 1512 bytes and is WRONG CODE - do not re-add it.
   What is still wrong, measured (this body, 86.78% / 1504 = one instruction
   short):
   - retail copies each shifted width/height/tileX/tileY into a second register
     unconditionally (`move t0,t2`) and negates THAT copy in the bit-3/bit-4
     arms. Writing the negation into separate drawWidth/drawHeight/drawTileX/
     drawTileY locals reproduces the width and height copies, but gcc still
     folds the tileX/tileY copies away (it negates those in place and uses the
     free slot for a nop). Those two `move`s plus the element-0 offset-block
     `lui` are the missing instructions.
   - the plane-mask read: retail forms the array base in a general register
     (`lui v1,%hi` + `addiu v1,v1,%lo`) and then `addu v0,index,v1`; this C
     folds the index into the load through $at (`sll` + `lui $at` + `addu $at`),
     which also costs the beq delay slot (retail puts the `lui` there). An
     unbounded-array extern and a `u16 *pMaskTable = g_SpritePlaneMasks;` local
     were both measured: identical 86.78% / 1504, gcc folds them back.
   - the transform block's `n` is computed first here and after the first lb in
     retail (`lb translationY` / `lw field_0x40` / `lb translationX` /
     `lw field_0x3C`), which is the one remaining extra nop in the preheader.
   - the saved-register assignment is a rotation of retail's: retail is
     s4=pSpriteData, s3=pFrameData, s2=pVerts, s1=transY, s0=transX, s5=mask,
     s6=offsetX, s7=lastPlane, s8=offsetY; this body has s3=pSpriteData,
     s2=pFrameData, s1=pVerts, s4=transY, s0=transX, s5=mask, s6=lastPlane,
     s7=offsetX, s8=offsetY. That rotation is most of objdiff's remaining 13%.
   - ROUND 348: THE PERMUTER RUNS NOW. It died on the pTransform spelling
     `(SpriteDirectionTransform*)(lastPlane * sizeof(SpriteDirectionTransform) +
     (mem_addr)pDirTransforms)`: decomp-permuter re-prints the AST and its printer
     turns `sizeof(Typedef)` into a cast, so cc1 reports "conversion to
     non-scalar type requested" on text that was fine. The spelling
     `&pSpriteData->pBase->pDirTransforms->dirTransforms[lastPlane]` (and the
     same expression for rot.vy/rot.vz, which is what retail's re-read of
     pDirTransforms shows) parses and costs nothing (86.78 -> 86.74).
   - ROUND 348 measured, each re-diffed: splitting the offsetX shift into two
     statements (offsetX = the lb, then offsetX <<= n; offsetY stays one
     statement) 86.74 -> 87.86; then scaling transX BEFORE transY (the opposite
     of the order retail's `mult s1,v1` shows) 87.86 -> 89.01; then hoisting
     `deltaHeight = pFrameData[i].deltaHeight;` above the tpage store and
     using `(s8)deltaHeight` 89.01 -> 89.45 at 1496 bytes, i.e. +0.44 for -8
     bytes (reverting it gives 89.01 / 1504, ONE instruction short - the better
     base if the missing instructions matter more than the score). Hoisting
     deltaWidth the same way is 87.72 - do not.
   - ROUND 348 permuter runs, each from the body it produced: 5262 -> 4687 ->
     4112 -> 3802 -> 3514 (its own score against retail's target.o). Everything
     it found after 3802 was WRONG CODE and was not ported: a pOt copy in the
     else addPrim arm (1452 bytes / 83.76, and it drops 13 instructions),
     pVerts[(spriteFlags >> 3) * 0], and drawTileX replaced by -tileX in the x
     arms (that negates unconditionally - a different result). So the next step
     is not another permuter run from this body.
   - ROUND 349 (all re-diffed, none kept): the four unconditional copies cannot
     be forced. Interleaving each copy right after its own shift (retail's
     spacing) is a no-op: same 89.01 / 1504. Reversing the copy order (tile
     copies first) is 88.25 / 1500. Splitting the tile shifts into two
     statements is 86.77 / 1500. ONE shared scratch variable carrying both tile
     copies (tileCopy = tileX; drawTileX = tileCopy; then again for tileY, the
     round-348 sibling lever) keeps 89.01 / 1504 and gcc coalesces the scratch
     away exactly as it does drawTileX/drawTileY. Read the surviving copies as
     the allocator's answer to liveness: drawWidth and drawHeight survive
     because `width`/`height` are REASSIGNED later in the uv block, while
     tileX/tileY are never reassigned, so cc1 puts drawTileX and tileX in one
     register and negates in place (`negu a1,a1`). Retail's own `negu t0,t2`
     shows the negation reading the SOURCE variable, so the source really is
     `drawX = -x;` for all four.
   - The one missing instruction is the `move v1,a3` (drawTileY) that retail
     puts in the second `beqz` delay slot; this body's `move a0,a2` slot holds a
     maspsx load-delay nop instead. Getting both tile copies would add two
     instructions, i.e. 1512, so one instruction elsewhere also differs (this
     body emits nop plus a second beqz pair in the loop preheader where retail
     fills both delay slots).
   - NOT A PRESET: under gcc-2.7.2-cdk-psx this body is 90.15, not 100.
   - ROUND 364 (un-parked, 5 measured edits, parked at 89.06 / 1500 = TWO
     instructions short, retail 1508):
     (a) THE `s32`-FOR-`int` LEVER IS A NO-OP IN THIS UNIT: `typedef signed int
     s32` (include/types.h), so `s32` and `int` are the same type and cc1 cannot
     see the difference. Every integer local here is already effectively `s32`.
     (b) `lastPlane = -1;` must sit ABOVE the two early returns (the resource
     check and `count == 0`), not below them: retail fills the FIRST `beqz`
     delay slot with `li s7,-1`, and only then does the loop preheader lose the
     `nop` this body used to emit. 89.01/1504 -> 88.89/1500.
     (c) With (b) in place, `i = 0;` must come BEFORE `pVerts =
     g_BattleSpriteQuadCorners;` (89.06/1500). The other order also gives 1500
     but 88.89, because the `li $21,0xFFFFFF` that loop.c hoists out of the
     addPrim bitfield store (P_TAG.addr is a 24-bit bitfield, which is where the
     `and 0xFF000000` comes from) then lands in the delay slot. In retail it
     sits after `la s2` - so with (b)+(c) every other instruction of segment 0
     lines up and the ONLY residual there is that one hoisted `li`.
     (d) Declaring tileX/tileY/drawTileX/drawTileY `u32` instead of `s32`:
     identical 89.06/1500 (same width, so the same RTL).
     (e) A `while (1) { ...; if (++i == count) break; }` dispatch instead of the
     do/while: identical size, 89.05 - the invariant hoist does not move.
     WHAT THE TWO MISSING INSTRUCTIONS ARE, now proven with cc1_probe: the
     `addu a0,a2,zero` (drawTileX = tileX) and `addu v1,a3,zero` (drawTileY =
     tileY) copies. A standalone cc1_probe replica of the block emits all four
     copies in retail's exact positions - the copy right after each
     shift, the LAST one in the `beqz` delay slot - as soon as tileX/tileY stay
     LIVE past the flip block; with them dead after it cc1 coalesces
     drawTileX/tileX into one register and negates in place, which is what this
     body does. So the residual is exactly the round-349 liveness reading:
     drawWidth/drawHeight keep their copies because `width`/`height` are
     REASSIGNED in the uv block, and nothing in retail's code reads tileX/tileY
     after the negations (a2 dies at `negu a0,a2`, a3 at `negu v1,a3`), so no
     honest source makes them live. Do not re-try the four copy spellings or the
     permuter (four runs in round 348; everything past 3802 was wrong code).
*/


INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering", func_8001E3D8);

/* Best attempt: 89.06% (1500 bytes, retail 1508 = TWO draw-tile copies short; see the note
   above). Round 378 re-measured, both on this body: splitting the transform block's
   shift into `transY = (s8)...; transX = (s8)...; n = ...; transY <<= n; transX <<= n;`
   is 88.49 (worse, so the one-expression form stays), and declaring
   `g_SpritePlaneMasks` unbounded (`extern u16 g_SpritePlaneMasks[];`) leaves the plane
   mask read at the $at form and the score identical at 89.06/1500 - the folded base is
   not reachable this way either.
   The two missing instructions are drawTileX = tileX and drawTileY = tileY (retail
   copies all four shifted values and negates the COPY; ours coalesces the two tile
   copies because tileX/tileY die at the negation). Nothing in retail reads them after
   the flip block, so no honest source makes them live.
   Body as measured below:
void func_8001E3D8(SpriteData* pSpriteData, void* pArg) {
    SpriteFrameData* pFrameData;
    SpriteDirectionTransform* pTransform;
    POLY_FT4* pPrim;
    SVECTOR* pVerts;
    MATRIX* pMatrix;
    MATRIX mat;
    SVECTOR rot;
    u_long* pOt;
    s32 count;
    s32 i;
    s32 n;
    s32 lastPlane;
    s32 offsetX;
    s32 offsetY;
    s32 flipX;
    s32 transX;
    s32 transY;
    s32 tileX;
    s32 tileY;
    s32 edgeX;
    s32 edgeY;
    s32 drawWidth;
    s32 drawHeight;
    s32 drawTileX;
    s32 drawTileY;
    s32 width;
    s32 height;
    s32 texX;
    s32 texY;
    s32 z;
    s32 flag;
    u32 spriteFlags;
    u8 visible;

    flipX = ((u32)pSpriteData->flags_0xAC.flags >> 2) & 1;
    offsetY = (s8)pSpriteData->pBase->offsetY << ((pSpriteData->field_0x40 >> 8) & 0x1F);
    offsetX = (s8)pSpriteData->pBase->offsetX;
    offsetX = offsetX << ((pSpriteData->field_0x40 >> 8) & 0x1F);
    pFrameData = pSpriteData->pBase->pCurRenderFramesData;
    if (flipX) {
        offsetX = -offsetX;
    }
    count = (pSpriteData->field_0x40 >> 2) & 0x3F;
    lastPlane = -1;
    if ((u8*)g_GfxCurWorkBuffer + count * sizeof(POLY_FT4) >= (u8*)g_GfxCurWorkBufferEnd) {
        return;
    }
    if (count == 0) {
        return;
    }
    i = 0;
    pVerts = g_BattleSpriteQuadCorners;
    do {
        // frames are grouped by layer: only the first frame of a run re-picks the
        // direction transform (and so the matrix the rest of the run is drawn with)
        if ((pFrameData[i].flags & 7) != lastPlane) {
            lastPlane = pFrameData[i].flags & 7;
            visible = (g_SpritePlaneMasks[lastPlane] & ((SpriteRenderFlagWords*)pSpriteData)->flags_0x3C.bytes[1]) == 0;
            pTransform = &pSpriteData->pBase->pDirTransforms->dirTransforms[lastPlane];
            if (pSpriteData->pBase->pDirTransforms != NULL &&
                (pTransform->translationX != 0 || pTransform->translationY != 0 || pTransform->rotationZ != 0)) {
                n = (pSpriteData->field_0x40 >> 8) & 0x1F;
                transY = (s8)pTransform->translationY << n;
                transX = (s8)pTransform->translationX << n;
                if ((SpriteFlags0x3C(pSpriteData) >> 3) & 1) {
                    transX = -transX;
                }
                transX = transX * pSpriteData->scale / 0x1000;
                transY = transY * pSpriteData->scale / 0x1000;
                rot.vx = pTransform->rotationX;
                rot.vy = pSpriteData->pBase->pDirTransforms->dirTransforms[lastPlane].rotationY;
                rot.vz = pSpriteData->pBase->pDirTransforms->dirTransforms[lastPlane].rotationZ;
                if ((SpriteFlags0x3C(pSpriteData) >> 3) & 1) {
                    rot.vz = -rot.vz;
                }
                RotMatrix(&rot, &mat);
                mat.t[0] = pSpriteData->pBase->transformMatrix.t[0] + transX;
                mat.t[1] = pSpriteData->pBase->transformMatrix.t[1] + transY;
                mat.t[2] = pSpriteData->pBase->transformMatrix.t[2];
                SetMulMatrix(&pSpriteData->pBase->transformMatrix, &mat);
                pMatrix = &mat;
            } else {
                SetRotMatrix(&pSpriteData->pBase->transformMatrix);
                pMatrix = &pSpriteData->pBase->transformMatrix;
            }
            SetTransMatrix(pMatrix);
        }
        if (visible == 0) {
            continue;
        }

        pPrim = g_GfxCurWorkBuffer;
        g_GfxCurWorkBuffer = pPrim + 1;
        setlen(pPrim, 9);
        *(u32*)&pPrim->r0 = pFrameData[i].colorAndCode;
        pPrim->tpage = pFrameData[i].texPage;
        pPrim->clut = pFrameData[i].clutID;
        n = (pSpriteData->field_0x40 >> 8) & 0x1F;
        width = (s16)(pFrameData[i].width + (s8)pFrameData[i].deltaWidth) << n;
        height = (s16)(pFrameData[i].height + (s8)pFrameData[i].deltaHeight) << n;
        tileX = pFrameData[i].tileX << n;
        tileY = pFrameData[i].tileY << n;
        spriteFlags = SpriteFlags0x3C(pSpriteData);
        drawWidth = width;
        drawHeight = height;
        drawTileX = tileX;
        drawTileY = tileY;
        if ((spriteFlags >> 3) & 1) {
            drawWidth = -width;
            drawTileX = -tileX;
        }
        if ((spriteFlags >> 4) & 1) {
            drawHeight = -height;
            drawTileY = -tileY;
        }

        edgeX = drawTileX + drawWidth;
        if (!((u32)pFrameData[i].flags >> 4 & 1)) {
            g_BattleSpriteQuadCorners[0].vx = drawTileX;
            pVerts[1].vx = edgeX;
            pVerts[2].vx = edgeX;
            pVerts[3].vx = drawTileX;
        } else {
            g_BattleSpriteQuadCorners[0].vx = edgeX;
            pVerts[1].vx = drawTileX;
            pVerts[2].vx = drawTileX;
            pVerts[3].vx = edgeX;
        }

        edgeY = drawTileY + drawHeight;
        if (!((u32)pFrameData[i].flags >> 5 & 1)) {
            pVerts[2].vy = edgeY;
            pVerts[3].vy = edgeY;
            pVerts[0].vy = drawTileY;
            pVerts[1].vy = drawTileY;
        } else {
            pVerts[2].vy = drawTileY;
            pVerts[3].vy = drawTileY;
            pVerts[0].vy = edgeY;
            pVerts[1].vy = edgeY;
        }

        pVerts[0].vy -= offsetY;
        pVerts[1].vy -= offsetY;
        pVerts[2].vy -= offsetY;
        pVerts[3].vy -= offsetY;
        pVerts[0].vx -= offsetX;
        pVerts[1].vx -= offsetX;
        pVerts[2].vx -= offsetX;
        pVerts[3].vx -= offsetX;
        RotTransPers4(&pVerts[0], &pVerts[1], &pVerts[2], &pVerts[3], (long*)&pPrim->x0, (long*)&pPrim->x1,
                      (long*)&pPrim->x3, (long*)&pPrim->x2, &z, &flag);

        texX = pFrameData[i].texX;
        texY = pFrameData[i].texY;
        width = pFrameData[i].width - 1;
        height = pFrameData[i].height - 1;
        if (pPrim->x3 < pPrim->x0) {
            if (--texX < 0) {
                texX = 0;
                width--;
            }
        }
        pPrim->u0 = texX;
        pPrim->v0 = texY;
        pPrim->u1 = texX + width;
        pPrim->v1 = texY;
        pPrim->u2 = texX;
        pPrim->v2 = texY + height;
        pPrim->u3 = texX + width;
        pPrim->v3 = texY + height;

        if ((SpriteFlags0x3C(pSpriteData) >> 27) & 1) {
            pOt = (u_long*)((u8*)pArg - lastPlane * 4);
            addPrim(pOt, pPrim);
        } else {
            addPrim((u_long*)pArg, pPrim);
        }
    } while (++i != ((SpriteRenderFlagWords*)pSpriteData)->flags_0x40.bytes[0] >> 2);
}


*/


/* Best attempt: 80.03% (1200 bytes, the original is 1196; round 294 took the
   parked 76.49% body down to this by the two fixes marked FIXED). The rest of
   the body is a verified transcription; what is left is the allocator's
   choice of which nine values get s0-s8, and two statement spells.
   Facts confirmed against the .s:
   - The prim IS a POLY_FT4 (this tree's libgpu.h has the shipped per-corner
     layout: u0/v0/clut at 0xC, u1/v1/tpage at 0x14, u2/v2/pad at 0x1C,
     u3/v3/pad at 0x24, sizeof 0x28). setlen(pPrim, 9) is the tag len byte at
     3 and setcode 0x2C at 7 -> `setPolyFT4(pPrim)` then `setRGB0(pPrim,0,0,0)`.
   - 0x8004FAD8 is NOT g_BattleSpriteQuadCorners (that one is 0x8004FB98).
     It is 4 SVECTORs of scratch (g_SpriteQuadCorners), written x into .vx and the
     screen-vertical into .vz because the quad lies in the XZ plane (vy is
     never written, the .sdata is zeroed). 0x8004FAF8 is a u16[16] of
     1,2,4..0x8000 used as the per-layer mask.
   - The sprite's plane byte is byte 1 of the 0x3C word (lbu 0x3D) and the
     frame count is byte 0 of the 0x40 word >> 2 (lbu 0x40); the shared header
     has bitfields/u32 there, so both need the local view above.
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
   - FIXED this round: the frame data pointer belongs BEFORE the work-buffer
     early-out (`pFrameData = pSpriteData->pBase->pCurRenderFramesData;` above
     the first if) - retail loads pBase in the branch shadow, and the two nop
     load-delay slots this C otherwise leaves in the preheader go away.
   - FIXED this round: the quad edges need two source locals (`edgeX = tileX +
     width;` before the x if, `edgeY = tileY + height;` before the z if) used by
     BOTH stores of the arm. Writing `tileX + width` twice makes gcc emit the
     addu twice (2 extra bytes per arm pair); with the local, reorg puts one
     addu in the branch delay slot exactly as retail does.
   Measured and still wrong (each re-diffed):
   - retail emits width/height/n interleaved: width, then n, then width<<=n,
     then height, then height<<=n, then tileX, tileY - that order is worth
     74.90 -> 76.48 and is what the body below does.
   - the quad y stores are in the order [2],[3],[0],[1] in both arms.
   - (a) the saved-register set is the same nine (s0-s8, retail's pOt is in
     s8/fp) but assigned differently: retail spends one on `%hi(g_SpriteQuad
     Corners)` kept alive across the loop for the element-0 store
     (`sh a0,%lo(g_SpriteQuadCorners)(s7)`), this C spends it on addPrim's
     0xFFFFFF mask (`lui s4,0xff; ori s4,s4,0xffff` in the preheader) and leaves
     four nop load-delay slots in the uv block that retail fills with its
     in-loop `lui a1,0xff` / `ori` / `lui a0,0xff00`. Spelling element 0 as
     `g_SpriteQuadCorners[0].vx` to force the %hi form does NOT work - cse folds
     it into the pVerts register and the frame grows to 0xA8 (1208 bytes); all
     four elements through the global is worse still (0xA8, `visible` spilled).
   - (b) retail splits the g_GfxBaseMatrix address into `lui s0,%hi` + TWO
     `addiu ...%lo` (one for the 8-word MATRIX copy in a1, one for ApplyMatrix'
     argument in s0, in 3/3/2-word groups); this C CSEs the address into one
     callee-saved register and copies the matrix in 4/4 groups (the spare a1 is
     what makes the fourth temp available).
   - (c) retail copies each shifted width/height/tileX/tileY into a second
     register unconditionally (`addu t0,t1,zero`) and negates THAT in the arm;
     this C negates in place. Not reachable by source shape: a 4-minute permuter
     run from this body (1630 iterations, base 7669 -> best 7224) only found
     edits that negate unconditionally, i.e. wrong code.
   - (d) retail's plane-mask read forms the array base in a register
     (`lui/addiu` then `addu`), this C folds the index into the load through
     $at (one instruction shorter).
   - (e) the uv right/bottom edges are `addiu 255` applied to texX before the
     addu (`(u + 0xFF) + width`) in retail, `(u + width) - 1` here. The C that
     would emit it is `texX + 0xFF + width`, which no programmer writes, so it
     was left alone.
   - (f) the second y average uses `sra` in retail and `srl` here, i.e. retail
     divides a signed value in both pairs; `(s16)(y2+y3)/2` is not enough to
     make the second one signed.
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering", func_8001E9BC);

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
    s32 edgeX;
    s32 edgeY;
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
    pFrameData = pSpriteData->pBase->pCurRenderFramesData;
    if ((u8*)g_GfxCurWorkBuffer + count * sizeof(POLY_FT4) >= (u8*)g_GfxCurWorkBufferEnd) {
        return;
    }
    if (count == 0) {
        return;
    }

    pVerts = g_SpriteQuadCorners;
    lastPlane = -1;
    for (i = 0; i != ((SpriteRenderFlagWords*)pSpriteData)->flags_0x40.bytes[0] >> 2; i++) {
        frameFlags = pFrameData[i].flags;
        if (lastPlane != (frameFlags & 7)) {
            lastPlane = frameFlags & 7;
            visible = (g_SpritePlaneMasks[lastPlane] & ((SpriteRenderFlagWords*)pSpriteData)->flags_0x3C.bytes[1]) == 0;
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

        edgeX = tileX + width;
        if (!((frameFlags >> 4) & 1)) {
            pVerts[0].vx = tileX;
            pVerts[1].vx = edgeX;
            pVerts[2].vx = edgeX;
            pVerts[3].vx = tileX;
        } else {
            pVerts[0].vx = edgeX;
            pVerts[1].vx = tileX;
            pVerts[2].vx = tileX;
            pVerts[3].vx = edgeX;
        }

        edgeY = tileY + height;
        if (!((frameFlags >> 5) & 1)) {
            pVerts[2].vz = edgeY;
            pVerts[3].vz = edgeY;
            pVerts[0].vz = tileY;
            pVerts[1].vz = tileY;
        } else {
            pVerts[2].vz = tileY;
            pVerts[3].vz = tileY;
            pVerts[0].vz = edgeY;
            pVerts[1].vz = edgeY;
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
/* Best attempt: 90.79% (852 bytes; retail is 844 = 211 instructions, so this is
   two instructions over). Everything in the middle of the loop is already
   retail's, instruction for instruction; what is left is ONE callee-saved
   register (the loop.c measurement at the end) plus the texX copy.
   Facts confirmed by measurement, in the order they were found:
   - The frame loop is a DO/WHILE over an INDEXED access, `pFrameData[i]`, ending
     `} while (++i != ((SpriteRenderFlagWords*)pSpriteData)->flags_0x40.bytes[0]
     >> 2);`. The pointer form the .s looks like (`pFrameData->` plus
     `pFrameData++`) makes gcc's combine_givs re-base the induction register onto
     pFrameData+6, so every u8 field goes negative (10(s3), -4(s3), ...): 83.57%
     and 856 bytes. With pFrameData[i] the register IS the base, the
     displacements are 0/2/4/6/7/8/9/A/C/10/14 (retail's) and `addiu s3,s3,24`
     lands in the back-branch delay slot. Same finding as the func_8001E9BC /
     func_8001F1D4 / func_8001E3D8 notes below.
   - The loop bound is the BYTE reload flags_0x40.bytes[0] >> 2, not the head's
     word-based `count`: retail re-reads the byte at the loop tail.
   - pFrameData is loaded into a SECOND local before `count` and copied into
     pFrameData after the two early returns (`pNextFrameData = pSpriteData->pBase
     ->pCurRenderFramesData;` before `count = ...`, then `pFrameData =
     pNextFrameData;`). Worth 83.57 -> 90.79, and it is what puts retail's
     `lw a0,32(s6)` / `lw v0,48(a0)` pair between the buffer-end compare and the
     `beqz`; without it the whole prologue is one register off. Permuter find.
   - The x-quad arm must WRAP the two negations in a duplicated if/else on a live
     global (`if (pSpriteData->field_0x40) { width = -width; tileX = -tileX; }
     else { width = -width; tileX = -tileX; }`). The arms are identical, so no
     semantics change, and it is worth 84.47 -> 85.77. Debris that is
     load-bearing; do not tidy it away.
   - element 0 of the quad corners is `pVerts[0].vx`, NOT
     `g_BattleSpriteQuadCorners[0].vx`: the symbol spelling makes cc1 emit
     `lui $at` + `sh` (two instructions per arm) instead of `sh a2,0(s4)`. That
     is the opposite of func_8001E3D8's park, which needs the symbol form.
   - The y arm stores elements 0,1,2,3 in that order (the x arm too), and
     RotAverage4's arguments are `&pVerts[0..3]`, `&pPrim->x0`, `&pPrim->x1`,
     `&pPrim->x3`, `&pPrim->x2` (x3 before x2), `&zDepth`, `&flag`; the zDepth
     and flag slots are sp+0x28 and sp+0x2C.
   What is still wrong, measured: gcc's loop.c HOISTS the 0x00FFFFFF that
   addPrim's setaddr masks with out of the loop. cc1 -dL on this unit reports
   `Loop from 71 to 677: 196 real insns` and `Insn 634: regno 263 (life 9),
   move-insn savings 1 moved to 688`, where retail materialises it INSIDE the
   loop (`lui a0,0xff` / `ori a0,a0,0xffff`, immediately before the u1/u3 sums).
   The hoist takes s6, so pSpriteData lands in s5, z in s8 and pOt (the third
   argument) in t1; retail has pSpriteData s6, z s5, pOt s8. That is the whole
   residual: the hoisted lui/ori sit in the preheader instead of the loop (two
   instructions) and the texX block is one instruction long -- retail is
   `addiu v0,a1,-1` / `bgez` / `move a1,v0`, i.e. the decremented value copied
   back into texX, where `--texX` and `texX--; if (texX < 0)` both compile to the
   in-place `addiu` here and then compute texX+width twice. loop.c's rule is
   `threshold * savings * lifetime >= insn_count`, so the next lever is that
   mask's lifetime, 9 -> 8: anything that moves its two `and` uses one insn
   closer, or grows the loop body, should unstick the register file. A 4-minute
   permuter run from this 90.79% body found only the shapes already ported.
*/
/*
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
               *(u32*)&pPrim->r0 = pFrameData[i].colorAndCode;
               pPrim->tpage = pFrameData[i].texPage;
               pPrim->clut = pFrameData[i].clutID;
               tileX = pFrameData[i].tileX;
               width = pFrameData[i].width;
               width += (s8)pFrameData[i].deltaWidth;
               tileY = pFrameData[i].tileY;
               height = pFrameData[i].height;
               height += (s8)pFrameData[i].deltaHeight;
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
               if (!(pFrameData[i].flags & 0x10)) {
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

               if (!(pFrameData[i].flags & 0x20)) {
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
               texY = pFrameData[i].texY;
               clip >>= n;
               if (height > 0) {
                   texHeight = pFrameData[i].height - clip;
               } else {
                   texY -= clip;
                   texHeight = pFrameData[i].height - clip;
               }
               texX = pFrameData[i].texX;
               if (pPrim->x3 < pPrim->x0) {
                   if (--texX < 0) {
                       texX = 0;
                       width = pFrameData[i].width - 1;
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

/* Best attempt: 95.33%. Round 442: BEST IS 240 BYTES AND RETAIL'S .size IS 0xf4 = 244,
   so this body is ONE INSTRUCTION SHORT and can never link - the older "240 bytes =
   retail's size" reading in the note below was wrong. The missing instruction is
   retail's LAZY load of the 5th (stack) argument: retail has
   `mflo v1 / bgez / nop / addiu v1,v1,0xFFF / .L: lw v0,0x10(sp) / sra v1,v1,12 /
   sw v1,0(v0)`, i.e. pOutZ is loaded at the merge point of the second division's
   sign fixup; cc1 hoists it instead into the load-delay nop after `lhu v1,4(v0)`
   (`lw t1,0x10(sp)` there) and keeps it in t1 for the rest of the body, which is
   also why the second division's mflo lands in v0 here and v1 there. That is the
   word-stack-argument hoist levers.md records as a wall (h-stack-arg-u16-load-hoist
   and j-a-narrow-stack-argument-is round 403: a 4-byte aggregate moves a NARROW
   load only, a word read stays hoisted).
   Measured this round on this body (all 240 bytes, all 95.33 except the last):
   - 5th parameter as a 4-byte aggregate (`typedef struct { s32* pOutZ; }`): the load
     does not move at all - 95.33, identical stream. Not the lever here.
   - embedded assignment `*(pOutZLocal = pOutZ) = pFrame[1] * ...` (levers.md
     j-a-narrow-stack-argument-is's trick for a narrow arg): 95.33, load still hoisted.
   - base-first form of the frame sum, `(u8*)pFrames + ((u16*)((u8*)pFrames + 2))[index]`:
     95.16 (retail's `addu a0,v0,a0` is the index-FIRST sum coalesced into pFrames'
     register, so the index-first spelling stays).
   Residual after the 4 bytes: pSpriteData t0 (retail a1), pFrame a1 (retail reuses
   a0 = pFrames), pFrames a0 (retail coalesces it away), scale read through t0 vs a1.
   Round 455 re-measured the park at 95.33 / 240 bytes and tried three more shapes,
   all 240 bytes / 60 insns with the stack-arg load still filling the load-delay nop
   and landing in the free caller-saved t1 (so the allocator keeps it live to the
   merge and dbr's slot fill costs the nop):
   - a local copy `pZ = pOutZ; *pZ = ...;` at the store: coalesced, identical stream;
   - the second division's value through a named local (`v = pFrame[1] * scale /
     0x1000; *pZ = v;`): identical stream;
   - the pOutZ store moved FIRST in source order (Z,Y,X): 95.21 and still one load in
     the early nop slot, so the hoist is NOT tied to the store's position in the body.
     That kills the "source order decides the load's slot" idea for good.
   Mechanism to attack next: dbr fills the load-delay nop with the stack load because
   the pointer's pseudo gets a FREE caller-saved register (t1) that nothing clobbers
   between the slot and the merge; retail's load lands in v0 at the merge, i.e. its
   pseudo could not be hoisted. Nobody has found a spelling that denies it a free
   register without adding an instruction.
*/
/*
void func_8001F5BC(SpriteData* pSpriteData, s32 unusedIndex, s32* pOutX, s32* pOutY, s32* pOutZ) {
    s32 index;
    SpriteAnimPackage* pPackage;
    u8* pAnimations;
    u16* pFrames;
    u8* pFrame;

    pPackage = pSpriteData->pVramData;
    pFrames = pPackage->pFrames;
    pAnimations = pPackage->pAnimations;
    pAnimations = *(u16*)(pAnimations + 2) + pAnimations;
    pFrame = *(u16*)(pAnimations + 4) + pAnimations;

    index = pFrame[4];
    if (index != 0) {
        index--;
    }
    if (pFrames[index] < index) {
        index = 0;
    }
    pFrame = (u8*)(((u16*)((u8*)pFrames + 2))[index] + (mem_addr)pFrames);

    *pOutY = pFrame[3] * pSpriteData->scale / 0x1000;
    *pOutZ = pFrame[1] * pSpriteData->scale / 0x1000;
    *pOutX = pFrame[2] * pSpriteData->scale / 0x1000;
}
*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering", func_8001F5BC);

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
   Round 442 re-measured the park at 97.38 / 160 bytes (both sides) and tried the
   one axis the note above had not: the shift/mask. `texPageBits = color >> 5;
   texPageBits &= 7;` as TWO statements reproduces retail's in-place
   `srl a3,a0,5 / andi a3,a3,7` (the single-expression form always leaves the srl
   in a temp: `srl v0,a0,5 / andi a3,v0,7`), but it hands the shifted value a3 and
   then gcc emits a `move a0,v1` copy of pFrameData and the loop counter moves to
   a1: 95.50% / 164 bytes, one instruction too long. Declaring texPage inside the
   loop is byte-identical to declaring it at the top (97.38 / 160). So the
   shift-result temp is FED by the allocator, not by the spelling: it disappears
   only in the body where the whole 4-cycle is already retail's.
*/
/* Best attempt: 97.38% / 160 bytes (retail 160). Instruction stream is retail's, 40 vs 40; the whole residual is one allocator permutation of the same three values: retail param t1 / i t0 / pFrameData a0, ours param t0 / i a0 / pFrameData v1 (and hence the sll temp v1 vs a2). Round 455 re-measured the park unchanged and tried three more shapes, all byte-identical at 97.38: (a) a per-iteration copy 'SpriteFrameData* pFr = pFrameData;' - coalesced away; (b) a named 'SpriteAnimPackage* pPackage = pSpriteData->pVramData;' before the tex read - coalesced away; (c) see the note below for the srl/andi split (95.50). The tie is the local-alloc hand-out of a0/t0/v1 only: retail's pFrameData takes a0 (freed when flags dies) while ours takes v1 (freed by pBase at the very load, 'lw v1,48(v1)'). Two permuter runs from score 115 found no gain, cdk scores 97.12, so this is parked as an allocator wall rather than a spelling question.
   Load-bearing permuter edits below, keep both: 'color = flags;' then
   'texPageBits = (color >> 5) & 7;' (96.50 -> 97.38) and
   'do { texPageBits--; } while (0);' (97.12 before the first).

void func_8001F6B0(SpriteData* pSpriteData) {
    SpriteFrameData* pFrameData;
    u32 flags;
    u32 color;
    u32 texPageBits;
    u16 texPage;
    s32 i;
    SVEC2 tex;
    SpriteAnimPackage* pPackage;

    flags = *(u32*)((u8*)pSpriteData + 0x3C);
    if ((flags & 3) != 1) {
        return;
    }

    color = flags;
    texPageBits = (color >> 5) & 7;
    if (texPageBits != 0) {
        do { texPageBits--; } while (0);
    }
    pPackage = pSpriteData->pVramData;
    tex = pPackage->tex;
    color = *(u32*)&pSpriteData->red;
    pFrameData = pSpriteData->pBase->pCurRenderFramesData;
    for (i = 0; i != (*(u8*)((u8*)pSpriteData + 0x40) >> 2); i++) {
        texPage = pFrameData[i].texPage;
        pFrameData[i].colorAndCode = color;
        pFrameData[i].texPage = (texPage & 0xFF9F) | (texPageBits << 5);
    }
}*/
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
