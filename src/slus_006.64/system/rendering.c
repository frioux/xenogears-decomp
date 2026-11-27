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
extern SVEC2 g_SpriteVramSlotPair[];

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

/* PARKED 80.76% (1436 bytes; retail 1452, 359 vs 363 insns, frame 0x80 vs retail
//    0x68). Round 430 measured the residual to ONE construct; everything else in the
//    body below is instruction-for-instruction retail's shape.
//    THE RESIDUAL: gcc derives a SECOND induction base for pFrameData at +2
//    ('addu $18,$fp,2' in the preheader, then BOTH $fp and $18 are bumped by 24 at the
//    loop end). That costs $fp, so `i` - retail's $s7 - is in a stack slot instead
//    (lw/addu/sw of 0x10(sp) at the loop end) and every sp-relative operand differs.
//    Retail has ONE base: $s0 = pFrameData, displacements 0x0 (tileX), 0x2 (tileY),
//    0x4..0x7, 0x8, 0x9, 0xA, 0xC, 0x10, 0x14.
//    Register census, both bodies 10 saved (ra, fp, s0-s7) + 16 home:
//      retail = pFrameData s0, entryFlags/spriteFlags s1, pEntry s2, pSprite s3,
//               texX s4, pSpriteData s5, dir s6, i s7, pAnimPackage fp;
//               stack: pFrames 0x10, pOffsets 0x14, texY 0x18, defaultMode 0x20,
//               color 0x28, count 0x2C, unkFlag 0x30, a spilled pSpriteData 0x38.
//      this body = same minus i (memory) plus the +2 base in s2; its spilled
//               pSpriteData (pOwner below) sits at 0x50. So the SOURCE pseudo set is
//               now retail's - only the base split differs.
//    MEASURED THIS ROUND (each was a scored run; keep them, do not redo):
//    - retail really does hold TWO source pseudos of pSpriteData: $s5 is used for
//      pBase->pDirTransforms in the walk and field_0x40 at the end, while the clut arm
//      reads flags_0xA8 and field_0x7C through a RELOAD of 0x38(sp). Adding pOwner
//      (used only in the clut arm) is what moved 79.36 -> 80.76% and 1424 -> 1436
//      bytes; retail's 0x38 slot is that pseudo, not a spill of the parameter.
//    - The base split is chosen by REFERENCE COUNT, never by the loop's own pointer:
//      moving the tile stores onto pOwner (semantic probe) drops the +2 base but gcc
//      then picks +0x14 ('lw 0($18)', 'sb -11($18)', 'sb -12($18)') - 356 insns /
//      1424 bytes / 80.04%, i still in memory. So freeing $fp alone does not buy `i`:
//      gcc simply keeps `i` in memory (biv elimination never runs here) and retail's
//      `lw count-from-0x2C(sp)` per iteration is the eliminated-biv shape.
//    - `u16* pTile = (u16*)pFrameData; pTile[0]/pTile[1]` is CSE'd back into
//      pFrameData: 80.76% / 1436, identical to the field form. Not a lever.
//    - Flipping the tile block to else-first (semantics-preserving) = 77.00% (1432).
//    - do/while with i++ in the body vs for(...; i++) ties exactly (80.76% / 1436);
//      do/while kept because retail's 'bne s7,t0' carries 'addiu s2,s2,3' in its slot.
//    - The permuter is NOT the next lever (a 4-minute run in round 415 found only
//      counter-clobbering candidates); the prize is 'i' in $s2, which is a source-shape
//      question: something must stop loop.c creating the (reg+2) giv while keeping the
//      pointer's own value live. Ideas not yet tried: (a) split the tile stores into a
//      separate 'SpriteFrameData* pFr = pFrameData;' declared INSIDE the loop so the
//      two bases have different giv origins; (b) type the frame's leading pair
//      (tileX/tileY) as a 2-element s16 array member so the second store is
//      pFrameData->tiles[1] (index form, maybe not giv-ised); (c) make count
//      'volatile'-free but read through a u8 pointer so the comparison is on memory.
//    Anatomy and the rest (frame slots, arm shapes, the five hand-packed ORs, the
//    s32 spriteFlags, the byte-offset on g_SpriteVramSlotPair, the signed
//    /0x1000 divisions) is unchanged from earlier rounds and still correct.
// */
//    Round 478 (this round): un-parked and re-measured 80.76% / 1436 bytes / 359
//    insns, frame 0x80 - the note above is exact. Two of its untried ideas are now
//    measured, do not repeat them: (a) the tile stores hoisted into tileX/tileY
//    locals with ONE store each (fewer references to pFrameData+2) = 77.15% / 1412
//    bytes / 353 insns, and (b) a 'SpriteFrameData* pFr = pFrameData;' declared
//    inside the loop for the tile stores = 80.76% / 1436, byte-identical to the
//    field form because cse folds it back. Idea (c) is still untried.
// PARK 83.99% (1436 bytes; retail 1452, 359 vs 363 insns, frame 0x78 vs retail
//    0x68). Round 491 closed the long-standing '+2 induction base' residual: the body
//    walks the frame array with an INDEX (pFrameData[i].field, i++ at the loop end)
//    instead of incrementing a pointer, and gcc now keeps ONE base ($s2 = pFrameData,
//    'addu $18,$18,24' at the loop end, every field offset 0x0..0x14 a displacement)
//    with the loop tail finally retail's exact addiu base,24 / addiu i,1 / lw count /
//    bne (addu pEntry,3 in the delay slot). The second base was loop.c turning a USER
//    pointer biv into two derived givs (+0 and +2); indexing removes the biv. 80.76 ->
//    83.99 at the same 1436 bytes.
//    The byte store at 0x13 must be '((u8*)&pFrameData[i])[0x13] |= 2' under the index
//    form (a raw pFrameData + 0x13 cast addresses element 0: that slip measured 82.07
//    with an extra stack slot).
//    REMAINING RESIDUAL, largest first:
//    - statement order at the top of the function. Retail issues the field_0x36 mult,
//      then runs the WHOLE 'flags = *pBlock' block plus the pFrameData load, and only
//      then that division's mflo (13 instructions between mult and mflo); the
//      field_0x38 division comes after. Forcing -fschedule-insns and -fschedule-insns2
//      on this body changes nothing (byte-identical to the default compile), so
//      retail's interleaving is RTL order: the ORIGINAL source really did compute
//      field_0x36, then read the frame-block header, then load pFrameData, then
//      compute field_0x38. Reordering the park's statements that way is the untried
//      experiment (do it together with the texY read below).
//    - same block: retail reads texY with 'lbu $t0,0x6($fp)' and stores a halfword,
//      so the source read a BYTE where this body reads a u16.
//    - frame: vars 64 vs retail 48 (one extra spill slot) and a different register map
//      from 0x2C on (pSpriteData $s5 retail / $s3 here, pFrameData $s0 retail / $s2
//      here), 430 differing lines, 4 instructions short of retail's 363.
//    Earlier rounds' measurements, do not redo: pointer-increment form 80.76/1436;
//    tile stores hoisted into locals 77.15/1412; pFr inside the loop 80.76 (cse folds
//    it); else-first 77.00/1432; the round-415 4-minute permuter found only
//    wrong-code candidates.
// PARK 86.75% (1440 bytes; retail 1452, 360 vs 363 insns, frame 0x78 vs retail 0x68).
//    Round 505 did the reorder the round-430 note prescribed: field_0x36, then the
//    flags block, then pFrameData/pOwner, then field_0x38 (was adjacent to 0x36), and
//    texY read through a byte ((u8)pAnimPackage->tex.y). That measured 83.99 -> 86.75
//    on the same body, so both statements are retail source; the low byte read is the
//    lbu at 0x6($fp), confirmed.
//    TWO residuals, both named:
//    - multiply latency hiding is on the WRONG statement. retail issues pBlock[3]*
//      scale, then the whole flags block + the pFrameData load, then that division's
//      mflo (13 insns between); this body finishes the field_0x36 division inline and
//      hoists field_0x38's mult above the flags block instead. gcc emits mult/mflo as
//      one pair, so retail's gap is scheduler-shaped; forcing the pass was already
//      measured as a no-op in round 430.
//    - frame/register cascade. Our locals get 8-byte stack slots (pFrames 0x10,
//      pOffsets 0x18, unkFlag 0x20, count 0x28, texY 0x30, defaultMode 0x38, color
//      0x40, pOwner 0x48) where retail packs them 4-byte (0x10, 0x14, 0x18, 0x20,
//      0x28, 0x2C, 0x30, 0x38), so the frame is 0x78 vs 0x68, the saved set and every
//      sp-relative operand differ and the whole s-register map shifts one place
//      (retail pFrameData s0 / pEntry s2 / entryFlags s1 vs this body s2 / s1 / s0).
//    Measured this round, do not redo: gcc-2.6.0 on this body 86.30 (worse, so not
//    the preset); the pre-reorder body sat at 83.99/1436/359 insns.
// PARK 92.72% (1440 bytes; retail 1452, 360 vs 363 insns, frame 0x78 vars=64 vs retail 0x68 vars=48). Round 530 found a REAL lever the round-505 note had written off as 'scheduler-shaped': the field_0x36 multiply is a SOURCE TEMPORARY. Writing
//      scaled36 = pBlock[3] * pSpriteData->scale;   (before the flags block)
//      pSpriteData->field_0x36 = scaled36 / 0x1000; (after the pOwner store)
//    reproduces retail's 13-instruction gap between the mult and its mflo (mult, whole flags block, pFrameData load, sw s5,0x38(sp), then mflo/bgez/sra/sh 0x36). 86.75 -> 92.72% at the same 1440 bytes / 360 insns, differing lines 385 -> 319. So gcc does emit mult and mflo adjacently and retail's gap is source order, not a scheduling pass: do not re-test -fschedule-insns.
//    REMAINING RESIDUAL, one construct: the SPILL SLOTS. Our eight values each take an 8-byte, 8-aligned slot (0x10,0x18,0x20,...,0x48) so vars=64 and the frame is 0x78; retail packs the same eight values into 48 bytes at 0x10 pFrames, 0x14 pOffsets, 0x18 texY, 0x20 defaultMode, 0x28 color, 0x2C count, 0x30 unkFlag, 0x38 the second pSpriteData pseudo. Retail's ALLOCATION ORDER also differs (texY/defaultMode/color before count/unkFlag). Measured this round: 4-mod-8 vars slots are reachable from C in this compiler (372 functions in the tree have them, e.g. battle/misc31 vars=64 with slots 20,36,52,68), and a probe of 20 ints live across calls gets 8-aligned 8-byte slots, so the align-4 path is some pseudo mode / allocation path we have not identified. That frame difference is what keeps the whole s-register map one place off (430 differing lines in round 491).
//    Preset screened this round on the 92.72 body: gcc-2.6.0 91.23%, gcc-2.7.2-cdk 91.91% - both below the unit compiler, so XenoRendering is right.
//    Still 3 instructions short of retail (360 vs 363); not yet localised.
/*
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
    s32 scaled36;

    pFrames = pAnimPackage->pFrames;
    pBlock = (u8*)((mem_addr)((u16*)pFrames)[index] + (mem_addr)pFrames);
    scaled36 = pBlock[3] * pSpriteData->scale;
    flags = *pBlock;
    unkFlag = flags & 0x80;
    count = flags & 0x3F;
    pOffsets = (u16*)(pBlock + 4);
    pEntry = pBlock + (count * 2 + 4);
    pFrameData = pSpriteData->pBase->pCurRenderFramesData;
    pOwner = pSpriteData;
    pSpriteData->field_0x36 = scaled36 / 0x1000;
    pSpriteData->field_0x38 = pBlock[1] * pSpriteData->scale / 0x1000;
    texY = (u8)pAnimPackage->tex.y;
    defaultMode = *(u8*)((u8*)pSpriteData + 0x3C) >> 5;
    color = *(u32*)&pSpriteData->red;
    dir = 4;
    i = 0;

    if (count != 0) {
        do {
            pFrameData[i].deltaHeight = 0;
            pFrameData[i].deltaWidth = 0;
            pFrameData[i].flags &= ~0x20;
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
                        pFrameData[i].flags |= 0x20;
                    }
                    if (entryFlags & 1) {
                        pFrameData[i].deltaWidth = *pEntry++;
                    }
                    if (entryFlags & 2) {
                        pFrameData[i].deltaHeight = *pEntry++;
                    }
                }
            }

            pSprite = (u8*)pFrames + *pOffsets++;
            spriteFlags = *pSprite;
            if (spriteFlags & 1) {
                pFrameData[i].flags |= 8;
                texX = (pAnimPackage->tex.x & 0x3F) >> 1;
            } else {
                pFrameData[i].flags &= ~8;
                texX = (pAnimPackage->tex.x & 0x3F) >> 2;
            }

            tpageMode = (*pEntry >> 4) & 3;
            clutBase = *pEntry & 0xF;
            pFrameData[i].colorAndCode = color;
            if (tpageMode == 0) {
                tpageMode = defaultMode;
            }
            if (tpageMode != 0) {
                ((u8*)&pFrameData[i])[0x13] |= 2;
                tpageMode--;
            }

            if ((spriteFlags >> 4) & 1) {
                pSprite++;
                spriteFlags |= *pSprite << 8;
                pTexEntry = (SVEC2*)((mem_addr)g_SpriteVramSlotPair + ((spriteFlags << 1) & 0x1C));
                pFrameData[i].texPage = GetTPage(spriteFlags & 1, (s16)tpageMode, pTexEntry->x, pTexEntry->y);
                pFrameData[i].clutID = GetClut((spriteFlags >> 1) & 0xF0, ((spriteFlags >> 9) & 0xF) + 0x1CC);
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
                    pFrameData[i].texPage = texPage;
                } else {
                    texPage = ((spriteFlags & 1) << 7) | ((tpageMode & 3) << 5);
                    texPage |= (pAnimPackage->tex.y & 0x100) >> 4;
                    texPage |= (pAnimPackage->tex.y & 0x200) << 2;
                    texPage |= (((pAnimPackage->tex.x + ((spriteFlags << 5) & 0x1C0)) & 0x3FF) >> 6);
                    pFrameData[i].texPage = texPage;
                }
                pFrameData[i].clutID =
                    (pAnimPackage->clut.y << 6) | (((pAnimPackage->clut.x + clutBase * 16) >> 4) & 0x3F);
            }

            pFrameData[i].flags = (pFrameData[i].flags & ~7) | dir;
            pFrameData[i].texX = texX + pSprite[1];
            pFrameData[i].texY = texY + pSprite[2];
            pFrameData[i].width = pSprite[3];
            pFrameData[i].height = pSprite[4];
            pFrameData[i].flags = (pFrameData[i].flags & ~0x10) | ((*pEntry >> 2) & 0x10);

            if (unkFlag != 0) {
                pFrameData[i].tileX = pEntry[1] | ((s8)pEntry[2] << 8);
                pFrameData[i].tileY = pEntry[3] | ((s8)pEntry[4] << 8);
                pEntry += 2;
            } else {
                pFrameData[i].tileX = (s8)pEntry[1];
                pFrameData[i].tileY = (s8)pEntry[2];
            }

            i++;
            pEntry += 3;
        } while (i != count);
    }
    pSpriteData->field_0x40 = (pSpriteData->field_0x40 & ~0xFC) | ((i & 0x3F) << 2);
}*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering", func_8001D53C);






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
   Round 537: un-parked and re-measured LIVE: 80.81% / 1652 bytes / 413 insns
   against retail's 1632 / 408, so the number above is reproducible. The body as
   stored did not actually compile (it read an undeclared `pPackage`); the cast
   below is the fix: retail loads 0x24(s3) once and block-copies bytes 4..7 with
   lwl/lwr, so the source has no pPackage local at all. Measured this round on the
   home-slot question: `SpriteAnimPackage* volatile pAnimPackage` DOES reproduce
   retail's `sw $a2,0x80(sp)` (frame+8) and the `lw $t1,0x80(sp)` reloads, but it
   scores 77.87% / 1684 bytes / 421 insns (frame 0x90, not 0x78) because volatile
   reloads PER USE and retail's clut arm reads clut.x/.y four times off ONE load
   (0x8/0xA off one $t1), i.e. retail reloads PER BLOCK. Per-block reload of a
   parameter is TREE_ADDRESSABLE (its address is taken in the original), not
   volatile, and nothing in the visible body takes it - so the open question is
   narrowed to: which construct in this source takes &pAnimPackage.
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
    tex = ((SpriteAnimPackage*)pSpriteData->pVramData)->tex;
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
// PARK 91.05% (332 bytes; retail 336, 83 vs 84 insns). Round 478: swapping the
//    two offset computations (xOffset first) moved 90.89 -> 91.05, so statement order
//    does steer the allocator. The residual is unchanged: a callee-saved ranking
//    permutation plus the address formation. retail = spriteData s0, pMat s1,
//    xOffset s2, yOffset s3; this body = spriteData s1, pMat s3, xOffset s0,
//    yOffset s2.
//    pMat (the g_GfxBaseMatrix address) is the structural difference: retail's
//    lui/addiu pair sits MID-block, right before the position loads, and the pseudo
//    dies at the last t[] read, while this body births it in the declaration's
//    initializer (prologue) so it ranks last and takes s3. Measured this round:
//    assigning pMat after the SpriteUpdateTransformMatrix call (birth in the block
//    that ends at the flags beqz) = 81.98%, 340 bytes, and the pair still lands in
//    the prologue. Any birth next to the uses lets cse fold the constant back into
//    every use (86.02%, Round 170), so retail's same-block pair needs a cse decision
//    no source shape here has reproduced.
//    Also 1 instruction short: retail has a nop after the third t[] load, this body
//    forms pBase+0xC late (addiu s0,a1,12) so it has no hole to fill.
//    Already measured, do not redo: mid-block pMat 86.02, pMatrix before the call
//    88.15, pBase local across the call 79.11/344, do-while(0) 81.98, two 4-minute
//    permuter runs found only wrong-code candidates, 86.76 under cdk so not the
//    preset.
// PARK 91.05% (332 bytes; retail 336, 83 vs 84 insns). Round 505: the declaration
//    slot of pMat is not the knob - initializer-first, initializer-last and an
//    entry-block statement before the g_IsBattle test are all byte-identical at
//    91.05%, i.e. gcc numbers that pseudo by its first use, not by text. Residual
//    as before: retail = spriteData s0, pMat s1, xOffset s2, yOffset s3; this body
//    = spriteData s1, pMat s3, xOffset s0, yOffset s2, plus retail's nop after the
//    second t[] load (this body has no free slot to fill).
// PARK 91.05% (332 bytes; retail 336, 83 vs 84 insns). Round 491 re-measured the\n//    two open axes and closed both: a mid-block birth of pMat (assignment after the\n//    flags test, before the scale math) measures 86.18% with FOUR separate lui/addiu\n//    formations - cse propagates the symbol constant out of the pseudo in every block\n//    whenever the set is not in the entry block, so only the declaration-initializer\n//    keeps it a register, and that makes it the longest-lived pseudo and hence s3.\n//    Preset screens on this body: gcc-2.6.0 91.05 (identical allocation), cdk 89.36,\n//    -O3 does not compile. Not a compiler-version question.\n//    Residual unchanged: retail = spriteData s0, pMat s1, xOffset s2, yOffset s3;\n//    this body = spriteData s1, pMat s3, xOffset s0, yOffset s2. pMat must lose s3,\n//    which needs its birth mid-block (cse refuses) or a shorter live range than the\n//    offset temps (it is live from the prologue, necessarily).\n
/*
void func_8001E148(SpriteData* pSpriteData) {
    MATRIX* pMat = &g_GfxBaseMatrix;
    SVECTOR position;
    VECTOR vec;
    MATRIX* pMatrix;
    s32 n;
    s32 xOffset;
    s32 yOffset;

    if ((g_IsBattle != 0) || (g_GfxSpritesUseBaseMatrix != 0)) {
        SpriteUpdateTransformMatrix(pSpriteData);
    }

    n = (pSpriteData->field_0x40 >> 8) & 0x1F;
    xOffset = (s8)pSpriteData->pBase->offsetX;
    xOffset <<= n;
    yOffset = (s8)pSpriteData->pBase->offsetY;
    yOffset <<= n;

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
}*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering", func_8001E148);






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


// GraphicsDrawPauseLetters and func_8001FB30 were built by a cdk-compiler TU;
// they live in system/rendering_pause_text.c, which carries that preset.
