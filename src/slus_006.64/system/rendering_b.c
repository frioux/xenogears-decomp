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


// .sdata
s16 g_SpriteVramSlotRow;
s16 g_SpriteVramSlotXOffset;

// Tail of system/rendering (from SpriteNextVramSlot at file 0xFD30), on the same preset
// as the head; split so system/rendering keeps its name.
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

/* Best attempt: 97.95% / 244 bytes - retail's EXACT .size (round 455). The body was
   stuck one instruction SHORT at 240 bytes / 60 insns for three rounds; the missing
   instruction is retail's LAZY load of the 5th (stack) argument, which cc1/dbr was
   filling into the load-delay nop after `lhu v1,4(v0)` and keeping in a free
   caller-saved register to the merge. TWO SPELLINGS TOGETHER FIXED IT, both
   load-bearing, do not tidy them away:
     1. ESCAPE THE STACK ARGUMENT: `ppOutZ = &pOutZ; **ppOutZ = value;` (levers.md
        a-store-hoist-park-closes-when-the-escaped: taking the address of a 4-byte
        stack parameter costs no frame slot and stops the hoist), 240 -> 244 bytes
        and 94.67 at that point - the load now lands at retail's merge point with
        `lw v1,0x10(sp)`.
     2. NAME THE SECOND DIVISION'S RESULT: `value = pFrame[1] * scale / 0x1000;
        **ppOutZ = value;` - with the store written inline the load goes back into
        the mflo delay slot (`mflo / lw v1,0x10(sp) / bgez`); named, the stream is
        retail's and the score is 97.95.
   RESIDUAL, both pure allocator rotations on an otherwise instruction-identical
   61-insn / 244-byte body: (a) pSpriteData t0 (retail a1), pFrame a1 (retail a0),
   pFrames a0 in both but retail's final `addu a0,v0,a0` coalesces IN PLACE into
   pFrames' register while ours allocates a1; the scale reads follow pSpriteData
   (t0 vs a1). (b) the second division's product/pointer pair is mirrored (retail
   product v1 + pointer v0, ours product v0 + pointer v1); divisions 1 and 3 match.
   Measured this round and NOT better: `pZ = pOutZ; *pZ = ...` and
   `**ppOutZ = value` with a `s32* pZ = *ppOutZ` local (both coalesce, 97.95
   identical stream); the second division's value through a local WITHOUT the escape
   (95.33, 240); the pOutZ store moved first in source order Z,Y,X (95.21, still one
   early load - the hoist is not tied to the store's position); `u8* pFrame` declared
   first among the locals (97.95, identical); a 3-minute permuter run (1550
   iterations, base 125 best 70) offered only two pure copies
   (`void* new_var = pPackage->pAnimations;` and a pFrame copy for the last store)
   which compile to the identical stream, so its "better" was scoring noise - do not
   re-port them.
   Earlier rounds on the pre-escape body (all 240 bytes, all 95.33): 5th parameter as
   a 4-byte aggregate; embedded assignment `*(pOutZLocal = pOutZ) = ...`; base-first
   frame sum (95.16). Preset is not the answer: gcc-2.7.2-cdk 97.12 and cdk is ruled
   out for the whole unit (SpriteDrawFrameKeepTop/Bottom drop to 88.57 under it).
   NEXT LEVER: the two rotations above are one hand-out chain - retail's pFrame is
   pFrames' own register, so getting the final sum to coalesce in place (a0) is
   probably what also un-mirrors v0/v1. `mem_addr`-sum spellings of that statement
   have all been measured (index-first is the right one).
void func_8001F5BC(SpriteData* pSpriteData, s32 unusedIndex, s32* pOutX, s32* pOutY, s32* pOutZ) {
    u8* pFrame;
    s32 index;
    SpriteAnimPackage* pPackage;
    u8* pAnimations;
    u16* pFrames;
    s32** ppOutZ;
    s32 value;

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
    ppOutZ = &pOutZ;
    value = pFrame[1] * pSpriteData->scale / 0x1000;
    **ppOutZ = value;
    *pOutX = pFrame[2] * pSpriteData->scale / 0x1000;
}*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering_b", func_8001F5BC);


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
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering_b", func_8001F6B0);


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
