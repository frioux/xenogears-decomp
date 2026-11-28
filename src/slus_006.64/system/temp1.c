#include "common.h"
#include "system/menu.h"
#include "system/animation.h"

// SpriteData's 0x3C flag word. The shared header only splits bits 20-31 into
// fields (bits 3-7 sit inside its 20-bit field_0x3C_0), so the bits this unit
// sets get names here; the word member is for the places that change two bits
// with one read-modify-write.
typedef union {
    /* 0x0 */ u32 flags;
    /* 0x0 */ u8 bytes[4];
    struct {
        /* 0x0 */ u32 unk0: 3;
        /* 0x3 */ u32 flipX: 1;
        /* 0x4 */ u32 flipY: 1;
        /* 0x5 */ u32 direction: 3;
        /* 0x8 */ u32 unk8: 24;
    } fields;
} SpriteFlagsWord;

typedef struct {
    /* 0x00 */ u8 unk0[0x3C];
    /* 0x3C */ SpriteFlagsWord flags_0x3C;
} SpriteDataFlagsView;

#define g_SpriteFlags0x3C(pSpriteData) \
    (((SpriteDataFlagsView*)(pSpriteData))->flags_0x3C)

// Sprite / Animation functions

// .sbss
s32 g_SpritePendingClut;
s32 g_AnimTimerTickCount;

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80022B2C);
/* Best attempt: 96.25% (376 bytes / 94 insns; the original is 380 / 96). Instruction stream
   and control flow agree; two residuals, and both are cc1/reorg emission order, not source:
   (1) else block (.L80022C68): gcc emits the arg0 copy `addu a0,s0` BEFORE arg1's `lw
   a1,16(s0)` in the RTL, so the third block starts with the copy and reorg's eager delay-slot
   fill hoists it into the `bnez` at the top of the function. Retail's block starts with the
   load (not delay-slot eligible), so retail keeps a `nop` there and re-forms the copy at
   0x1346C. Measured: hoisting `speed = pSpriteData->step.y;` as the else body's first
   statement compiles byte-identically (gcc folds it back), and so does declaring the else's
   load in its own local; the arg expansion order does not change.
   (2) bounce block (0x133E8): retail's quotient stays in the mflo register (`sra v1,v1,8`,
   no copy), the `sw v1,16(s0)` is emitted before `bgez v1` and reorg leaves a `nop` in that
   slot; every spelling here makes gcc copy the quotient into a fresh register
   (`sra a1,v1,8`) and reorg then moves the store into the `bgez` delay slot (one instruction
   fewer, no `nop`). A separate `bounce` local instead of reusing `speed` measured 95.99 with
   the bounce in v0 and gravity in a0 (retail: bounce v1, gravity v0).
   Tried: the preset is settled (2.6.0 / 2.7.2 / -O1..-O3 / -G4..-G8 sweep, best is the
   current XenoGraphics preset), plus for (1) both the hoisted-load and the own-local spellings
   and for (2) a separate local. lever.py: no lever for eager delay-slot fill from a branch
   target. Not re-briefable as hand work; both need a way to control arg expansion order.
   Re-measured 96.25% / 376 bytes this round; both residuals are reorg's eager delay-slot fill
   (lever c-eager-fill-steals-the-targets-arg0-copy is this function). New data point: writing
   the bounce as two statements (`speed = -step.y * mask; speed = speed / 256;`) so the quotient
   could coalesce in place gives 94.43% / 380 bytes / 95 insns with an explicit `move v1,a1` -
   the split costs a copy instead of saving one, so the div target and the mult result cannot be
   made one register from source. This unit is compiled with -fschedule-insns AND
   -fschedule-insns2 (see the options header in build/src/slus_006.64/system/temp1.c.s), so
   statement order is not a lever for either residual.
   Re-measured 96.25% / 376 bytes / 94 insns this round, unchanged, and one more spelling measured
   dead: wrapping the else arm's two statements in an empty do-while (a block-splitting diagnostic,
   see a-block-split-flips-a-caller-saved-pair) is byte-identical at 96.25 with the same 36 differing
   lines, so residual (1) is not a basic-block artefact either. Park this class.
extern s32 SpriteScaleValue(SpriteData* pSpriteData, s32 value);
extern void BattleActorUpdateGroundHeightAndTriangle(void);

void func_80022B2C(SpriteData* pSpriteData) {
    s32 speed;
    s32 gravity;

    if (((*(u32*)((u8*)pSpriteData + 0x3C) >> 26) & 1) == 0) {
        BattleActorUpdateGroundHeightAndTriangle();
        speed = pSpriteData->step.y;
        if (speed > 0 && (s32)pSpriteData->gravity > 0) {
            if (*(s16*)((u8*)pSpriteData + 0x6) != pSpriteData->field_0x84) {
                pSpriteData->position.y += SpriteScaleValue(pSpriteData, speed >> 4) << 4;
                if ((pSpriteData->position.y >> 16) < pSpriteData->field_0x84) {
                    pSpriteData->step.y += pSpriteData->gravity;
                    return;
                }
                pSpriteData->position.y = pSpriteData->field_0x84 << 16;
                speed = -pSpriteData->step.y * (s32)(((u32)pSpriteData->flags_0xA8 >> 1) & 0x3FF) / 256;
                pSpriteData->step.y = speed;
                gravity = (s32)pSpriteData->gravity;
                if (speed < 0) {
                    speed = -speed;
                }
                if (gravity < 0) {
                    gravity = -gravity;
                }
                if (speed < gravity) {
                    pSpriteData->step.y = 0;
                }
                return;
            }
            return;
        }
        pSpriteData->position.y += SpriteScaleValue(pSpriteData, speed >> 4) << 4;
        if ((pSpriteData->position.y >> 16) >= pSpriteData->field_0x84) {
            pSpriteData->position.y = pSpriteData->field_0x84 << 16;
        }
        pSpriteData->step.y += pSpriteData->gravity;
    } else {
        pSpriteData->position.y += SpriteScaleValue(pSpriteData, pSpriteData->step.y >> 4) << 4;
        pSpriteData->step.y += pSpriteData->gravity;
    }
}
*/

s32 SpriteScaleValue(SpriteData* pSpriteData, s32 value) {
    s32 scale = pSpriteData->field_0x3A;
    s32 result = value;

    if (scale) {
        // split so gcc keeps the sign-correction temp of the signed / 1024
        result = result * scale;
        result = result / 1024;
    }
    return result;
}

void SpriteApplyMovementStep(SpriteData* pSpriteData) {
    pSpriteData->position.x += SpriteScaleValue(pSpriteData, pSpriteData->step.x >> 4) << 4;
    pSpriteData->position.z += SpriteScaleValue(pSpriteData, pSpriteData->step.z >> 4) << 4;
    func_80022B2C(pSpriteData);
}

// non-prototype on purpose: the second argument is a frame index, not in the header's prototype
extern void SpriteAnimFrameListSetFrame();

void SpriteSetFrameFromAnimIndex(SpriteData* pSpriteData) {
    u16* pAnimFlags;
    s32 animIndex;
    u16 animFlags;
    s32 frameIndex;

    // split so gcc keeps the shift result in the register of the masked index
    animIndex = (u32)pSpriteData->flags_0xA8 >> 11;
    animIndex &= 0x3F;
    if (animIndex < 0) {
        animIndex = 0;
    }
    // field_0x54 holds a raw address (animation_scripts.c stores one) to u16 frame entries
    pAnimFlags = (u16*)(mem_addr)pSpriteData->field_0x54;
    animFlags = pAnimFlags[animIndex];
    frameIndex = animFlags & 0x1FF;
    if (animFlags & 0x200) {
        pSpriteData->flags_0xAC.flags |= 8;
    } else {
        pSpriteData->flags_0xAC.flags &= ~8;
    }
    // the direction bit is set from the pair of animation flags, so this stays a
    // single read-modify-write of the word's flip field
    g_SpriteFlags0x3C(pSpriteData).fields.flipX = (((u32)pSpriteData->flags_0xAC.flags >> 3) & 1) ^
                                                 (((u32)pSpriteData->flags_0xAC.flags >> 2) & 1);
    SpriteAnimFrameListSetFrame(pSpriteData, frameIndex);
}

extern void AnimScriptTick(SpriteData* pSpriteData);

// K&R definition: AnimTaskCountedTimerTick below calls it with no arguments, as the original does.
void func_80022DF4(pEntry) WorkListEntry* pEntry;
{
    SpriteData* pSpriteData = pEntry->unk4;

    AnimScriptTick(pSpriteData);
    SpriteApplyMovementStep(pSpriteData);
    if (pSpriteData->pSpriteBytecode == NULL) {
        pEntry->onFreeCallback(pEntry);
    } else if (((u32)pSpriteData->flags_0xAC.flags >> 6) & 1) {
        AnimScriptTick(pSpriteData);
        SpriteApplyMovementStep(pSpriteData);
        if (pSpriteData->pSpriteBytecode == NULL) {
            pEntry->onFreeCallback(pEntry);
        }
    }
}

void AnimTaskCountedTimerTick(void) {
    g_AnimTimerTickCount++;
    func_80022DF4();
}

extern void GfxQueueSpriteTileData(void* pData);
extern void WorkListsRemoveEntriesForData(WorkListEntry* pTask);
extern void TimerWorkListClearDataField70(WorkListEntry* pTask);

void AnimTaskFree(AnimTask* pAnimTask) {
    WorkListEntry* pTask = &pAnimTask->task1;
    SpriteData* pSpriteData = pTask->unk4;

    if (pSpriteData->pBase) {
        if (pSpriteData->pBase->pFramesData) {
            GfxQueueSpriteTileData(pSpriteData->pBase->pFramesData);
        }
    }
    if ((pSpriteData->field_0x3C_0 & 3) == 1) {
        if (pSpriteData->pBase->pDirTransforms) {
            HeapFree(pSpriteData->pBase->pDirTransforms);
        }
    }
    if (((u32)pSpriteData->flags_0xAC.flags >> 5) & 1) {
        WorkListsRemoveEntriesForData(pTask);
    }
    if (((u32)pSpriteData->flags_0xB0.flags >> 11) & 1) {
        TimerWorkListClearDataField70(pTask);
    }
    if ((pSpriteData->field_0x3C_0 & 3) == 1) {
        SpriteAnimFrameListRemove(pSpriteData);
    }
    TimerWorkListRemoveTask(pTask);
    WorkListRemoveTask(pTask + 1);
    HeapFree(pAnimTask);
}

void SpriteAllocFramesData(SpriteData* pSpriteData, s32 count, u_int allocFlags) {
    SpriteDataI1* pBase;
    SpriteDataI1* pSpriteBase;
    void* pFramesData;

    if (pSpriteData->pBase->pFramesData) {
        HeapFree(pSpriteData->pBase->pFramesData);
    }
    pFramesData = HeapAlloc(count * sizeof(SpriteFrameData), allocFlags);
    pBase = pSpriteData->pBase;
    pBase->pFramesData = pFramesData;
    pBase->pCurRenderFramesData = pFramesData;
}

void SpriteAllocClutRect(SpriteData* pSpriteData, s32 count, u_int allocFlags) {
    ((SpriteDataI3*)pSpriteData->field_0x7C)->field_0x18 = HeapAlloc(count * 4, allocFlags);
    ((RECT*)((SpriteDataI3*)pSpriteData->field_0x7C)->field_0x18)->y = ((RECT*)pSpriteData->pVramData)->h;
    ((RECT*)((SpriteDataI3*)pSpriteData->field_0x7C)->field_0x18)->x = ((RECT*)pSpriteData->pVramData)->w;
}

extern void SpriteAnimFrameListRemove(SpriteData* pSpriteData);

void SpriteDestroy(SpriteData* pSpriteData) {
    switch (pSpriteData->flags_0xA8 & 1) {
    case 1:
        if (((SpriteDataI3*)pSpriteData->field_0x7C)->field_0x18) {
            HeapFree(((SpriteDataI3*)pSpriteData->field_0x7C)->field_0x18);
        }
        break;
    }
    SpriteAnimFrameListRemove(pSpriteData);
    HeapFree(pSpriteData->pBase->pFramesData);
    HeapFree(pSpriteData);
}

typedef struct {
    /* 0x0 */ s16 xStart;
    /* 0x2 */ s16 yStart;
    /* 0x4 */ s16 xEnd;
    /* 0x6 */ s16 yEnd;
} SLineEnds;

typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
} SDelta;

s32 SpriteComputeAngleFromPoints(SLineEnds line) {
    SDelta deltas[2];

    deltas[0].x = line.xStart - line.xEnd;
    deltas[1].x = line.yStart - line.yEnd;
    return -ratan2(deltas[1].x, deltas[0].x) & 0xFFF;
}

void SpriteSetFrameAndFlips(SpriteData* pSpriteData, s32 arg1, s32 arg2, s32 arg3) {
    // both flip bits change together, so the word is copied out, edited and stored back
    SpriteFlagsWord flags = g_SpriteFlags0x3C(pSpriteData);

    flags.fields.flipY = arg3 & 1;
    flags.fields.flipX = arg2 & 1;
    g_SpriteFlags0x3C(pSpriteData) = flags;
    pSpriteData->animScriptWaitTimer = 0;
    pSpriteData->flags_0xA8 &= ~0x300000;
    pSpriteData->flags_0xA8 &= ~0xE0000;
    // arg1 is passed through in a1, which is what keeps a1 out of the mask allocation above
    SpriteAnimFrameListSetFrame(pSpriteData, arg1);
}
// Blocks whose data table sits at a self-relative byte offset stored at 0xC
typedef struct {
    /* 0x0 */ u32 unk0;
    /* 0x4 */ u32 unk4;
    /* 0x8 */ u32 unk8;
    /* 0xC */ u32 dataOffset;
} AnimScriptLoopBlock;

// The pair a block's 0xC offset points at, placed right after the block
typedef struct {
    /* 0x0 */ u32 target;
    /* 0x4 */ u32 count; // stored as count - 1
} AnimScriptLoopBlockData;

u32 AnimScriptGetBlockTarget(AnimScriptLoopBlock* pBlock) {
    AnimScriptLoopBlockData* pData = (AnimScriptLoopBlockData*)(pBlock->dataOffset + (mem_addr)pBlock);

    return pData->target;
}

u32 AnimScriptGetBlockCount(AnimScriptLoopBlock* pBlock) {
    AnimScriptLoopBlockData* pData = (AnimScriptLoopBlockData*)(pBlock->dataOffset + (mem_addr)pBlock);

    return pData->count + 1;
}

extern int g_AnimScriptExtraTicks;
extern void SpriteAnimScriptRun(SpriteData* pSpriteData);

void AnimScriptTick(SpriteData* pSpriteData) {
    int i;

    for (i = 0; i != g_AnimScriptExtraTicks + 1; i++) {
        if (pSpriteData->animScriptWaitTimer) {
            pSpriteData->animScriptWaitTimer--;
            if (pSpriteData->animScriptWaitTimer == 0) {
                SpriteAnimScriptRun(pSpriteData);
            }
        }
    }
}

void SpriteSetDirectionAndRefreshFrames(SpriteData* pSpriteData, s32 direction) {
    s32 animType;
    s32 dir;

    g_SpriteFlags0x3C(pSpriteData).fields.direction = direction & 7;
    if (direction & 7) {
        pSpriteData->prim |= 2;
    } else {
        pSpriteData->prim &= ~2;
    }
    animType = (pSpriteData->field_0x40 >> 13) & 0xF;
    if (animType == 8 || animType == 9) {
        dir = g_SpriteFlags0x3C(pSpriteData).fields.direction;
        if (dir) {
            g_SpriteFlags0x3C(pSpriteData).fields.direction = (dir - 1) & 7;
        }
    } else {
        func_8001F6B0(pSpriteData);
    }
}

void SpriteReallocFramesData(SpriteData* pSpriteData, s32 count) {
    SpriteDataI1* pBase;
    SpriteDataI1* pSpriteBase;
    void* pFramesData;

    HeapFree(pSpriteData->pBase->pFramesData);
    pFramesData = HeapAlloc(count * sizeof(SpriteFrameData), 0);
    pBase = pSpriteData->pBase;
    pBase->pCurRenderFramesData = pFramesData;
    pBase->pFramesData = pFramesData;
}

// Allocate struct stuff
AnimTask* AnimTaskAlloc(void* pData, int dataSize) {
    AnimTask* pEntry;
    WorkListEntry* pTask1;
    WorkListEntry* pTask2;

    pEntry = HeapAlloc(dataSize + 0xEC, g_TaskHeapAllocFlags);
    pTask1 = &pEntry->task1;
    pTask2 = &pEntry->task2;
    TimerWorkListAddTask(pData, pTask1);
    WorkListAddTask(pEntry, pTask2);
    func_80023804(&pEntry->spriteData);
    pTask1->unk4 = &pEntry->spriteData;
    pTask2->unk4 = &pEntry->spriteData;
    TimerWorkListSetTaskCallback(pEntry, &func_80022DF4);
    WorkListTaskSetOnFreeCallback(pEntry, &AnimTaskFree);
    return pEntry;
}

s32 AnimEntryGetNumDirections(u16* pFlags) {
    u32 flags = *pFlags;
    s32 numDirections = (flags >> 8) & 0x7;

    if ((flags >> 14) & 0x1) {
        numDirections += 8;
    }
    return numDirections;
}

// objdiff stops at 99.41% because its table reference is `jtbl_80018664` and ours is `.rodata`;
// `tools/scripts/check_bin.sh slus_006.64` prints OK, so this definition is byte-identical.
// K&R style so func_80023FD8 can call it with the single argument retail passes.
s32 func_80023468(index, value) u_int index;
s32 value;
{
    s32 result = value;

    switch (index) {
    case 0:
    case 5:
    case 6:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
        result = 1;
        break;
    case 1:
    case 4:
    case 8:
    case 9:
        result = 0;
        break;
    case 2:
    case 7:
    case 15:
        result = 2;
        break;
    }
    return result;
}

void SpriteClearDirTransforms(SpriteData* pSpriteData) {
    s32 i;

    for (i = 0; i != NUM_8DIR_MOVEMENT_DIRECTIONS; i++) {
        (i + pSpriteData->pBase->pDirTransforms->dirTransforms)->translationX = 0;
        (i + pSpriteData->pBase->pDirTransforms->dirTransforms)->translationY = 0;
        (i + pSpriteData->pBase->pDirTransforms->dirTransforms)->rotationX = 0;
        (i + pSpriteData->pBase->pDirTransforms->dirTransforms)->rotationY = 0;
        (i + pSpriteData->pBase->pDirTransforms->dirTransforms)->rotationZ = 0;
    }
}

extern void SpriteComputeTransformMatrix(SpriteData* pSpriteData);
extern void SpriteSetScale(SpriteData* pSpriteData, s32 scale);
extern u32 g_GfxWorkListCapacity;
extern u8 g_IsBattle;
extern SpriteData* D_800C3E1C;

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80023538);
/* Best attempt: 99.89% (716 bytes, the original's size; every instruction identical except one
   three-instruction register mirror at the tail). Two source facts are load-bearing and should be
   kept: the animation entry must stay a u16* parameter (a `u16* pAnimHeader = pAnimEntry` local
   defers the `move s1,a1` copy into the branch and loses the prologue), and each gravity stage
   needs its OWN local (gravity / gravityStep / scaled) because retail gives the three products
   three different registers. The remaining tie is the `(flags_0xA8 & 1) == 1U` test at the tail:
   retail emits `andi v0,v0,1 / li v1,1 / bne v0,v1` (in place, constant in v1), this body
   `andi v1,v0,1 / li v0,1 / bne v1,v0`. The `(u32)` cast + `1U` comparison is what makes cc1 keep
   the `li` at all - a plain `(flags & 1) == 1` folds to `andi/beqz` and costs the function 4 bytes.
   Tried for the tail tie: a s32/u32/u8 local for the masked value (copy instead of in place, the u8
   one also reloads the field), a constant-first comparison, folding the two tests into one `&&`
   (which is how the current shape was found - it is required), swapping the `&&` operands, and a
   3-minute permuter run (1244 iterations, nothing). Also measured: `pBase` must be reassigned as
   `pSpriteBase = pSpriteData->pBase` in the 0x3C block (one shared local keeps it in v1 and loses
   the second load), and the three rotation stores must be rotation.z, rotation.x, rotation.y in
   that order (that is what puts the third store in the jal delay slot).
// pAnimEntry is an animation entry: u16 word 0 holds the direction/angle flags,
// words 1 and 2 are byte offsets to the bytecode and to the frame script.

   RE-MEASURED this round after unpark: 99.89%, 716 bytes (retail's size), 178 vs 178 insns, and the
   only residual is the one in the paragraph above. Tried this round, all 99.89 / 716 bytes: the flag
   test as a nested `if` whose block opens with a declaration-with-initialiser local
   (`u32 flag = (u32)pSpriteData->flags_0xA8 & 1;`) - the lever for a mirrored load/derived-value pair
   - and the last flags RMW through a `u32 flags` local used for both the store and the test.
   Permuter, 3 minutes, 1085 iterations: base 20 best 20, no lead. gcc-2.6.0 screens this function at
   96.45 and breaks matched neighbours (SpriteApplyMovementStep 98.46, SpriteComputeAngleFromPoints
   81.05, AnimTaskCreate 88.14), so the preset is not the cause and the unit cannot be switched.
   Verdict: GIV_TIE - retail's flags word dies in place at the `andi`, ours gets a second pseudo.
void func_80023538(SpriteData* pSpriteData, u16* pAnimEntry) {
    SpriteDataI1* pBase;
    SpriteDataI1* pSpriteBase;
    SpriteDataI3* pOpaque;
    s32 offset;
    s32 cycles;
    s32 gravity;
    s32 gravityStep;
    s32 scaled;
    s32 total;
    s32 value;
    s32 factor;
    s32 scale;
    s32 speed;

    pSpriteData->pCurAnimation = pAnimEntry;
    offset = pAnimEntry[1] + 2;
    pSpriteData->pSpriteBytecode = (void*)(offset + (mem_addr)pAnimEntry);
    pSpriteData->flags_0xA8 = (pSpriteData->flags_0xA8 & 0xFFCFFFFF) | ((pAnimEntry[0] & 3) << 20);
    offset = pAnimEntry[2] + 4;
    pSpriteData->field_0x54 = offset + (mem_addr)pAnimEntry;
    speed = (pAnimEntry[0] >> 2) & 0x3F;
    if (speed & 0x20) {
        speed |= -0x40;
    }
    cycles = g_AnimScriptExtraTicks + 1;
    total = cycles * cycles * pSpriteData->field_0x82;
    gravity = speed << 10;
    pSpriteData->gravity = gravity;
    value = total / 4096;
    scale = ((u32)pSpriteData->flags_0xAC.flags >> 7) & 0xFFF;
    factor = 0x10000 / scale;
    gravityStep = gravity * value;
    pSpriteData->gravity = gravityStep;
    value = factor * factor / 256;
    scaled = gravityStep * value;
    pSpriteData->gravity = scaled;
    value = scaled / 256;
    pSpriteData->gravity = value;
    if (((pAnimEntry[0] >> 11) & 1) == 0) {
        pSpriteData->step.z = 0;
        pSpriteData->step.y = 0;
        pSpriteData->step.x = 0;
        pSpriteData->moveSpeed = 0;
    }
    pBase = pSpriteData->pBase;
    if (pBase != NULL) {
        if (((pAnimEntry[0] >> 12) & 1) == 0) {
            pBase->rotation.z = 0;
            pBase->rotation.x = 0;
            pBase->rotation.y = 0;
            SpriteComputeTransformMatrix(pSpriteData);
        }
        if (((pAnimEntry[0] >> 13) & 1) == 0) {
            if (g_IsBattle) {
                SpriteSetScale(pSpriteData, g_GfxWorkListCapacity);
            }
        }
        if (g_IsBattle) {
            SpriteComputeTransformMatrix(pSpriteData);
        }
        if ((pSpriteData->field_0x3C_0 & 3) == 1) {
            pSpriteBase = pSpriteData->pBase;
            pSpriteBase->offsetY = 0;
            pSpriteBase->offsetX = 0;
            if (((pSpriteData->field_0x40 >> 20) & 1) == 0) {
                if (pSpriteData->pBase->pDirTransforms) {
                    SpriteClearDirTransforms(pSpriteData);
                }
            }
        }
    }
    pSpriteData->stackIndex = 0x10;
    pSpriteData->field_0x30 = 0;
    pSpriteData->flags_0xA8 &= ~0x7FE;
    pSpriteData->animScriptWaitTimer = 1;
    pSpriteData->flags_0xA8 &= 0xF03FFFFF;
    pSpriteData->flags_0xA8 &= 0xCFFFFFFF;
    pSpriteData->flags_0xA8 |= 0x20000000;
    pSpriteData->flags_0xA8 |= 0x1F800;
    if ((pSpriteData->field_0x7C != NULL) && ((u32)(pSpriteData->flags_0xA8 & 1) == 1)) {
        pOpaque = pSpriteData->field_0x7C;
        pOpaque->field_0x4 = 0;
        pOpaque->field_0x0 = 0;
        ((SpriteDataI3*)pSpriteData->field_0x7C)->field_0xC = 0;
    }
}

   ROUND 408: re-parked at 99.89% / 716 bytes / 178 insns (retail's size) after re-measuring from this
   same body - unchanged. Preset question is now CLOSED with numbers: the unit's own gcc-2.7.2-cdk
   scores this body 99.89 while plain gcc-2.7.2-psx drops it to 97.54 (and AnimScriptTick 94.75,
   AnimTaskAlloc 79.92, AnimTaskCreate 88.14, func_80023468 86.41), and cdk -O3 collapses to 80.84, so
   cdk -O2 is right and the tie is not a preset. Mechanism analysis of the residual: retail's
   'andi v0,v0,0x1' ties its OUTPUT to the dying input (in place), ours allocates a fresh v1 because
   mips.md's andsi3 output is '=d' with no matching constraint - so v0 is still occupied by the input
   when the output is placed and the constant 1 then takes v0 (li v0,1) instead of retail's
   addiu v1,zero,1. The constant-1 pseudo and the andi result die at the SAME bne, so their allocation
   order is the coin flip. No C spelling of a pure test has been found that forces the in-place tie.
   Verdict: GIV_TIE.

   Also measured this round: the tail written as nested ifs with an in-place local
   ("flags = pSpriteData->flags_0xA8; flags &= 1; if (flags == 1)") - still 99.89 / 716 bytes, the AND
   again goes to a fresh v1, so even a compound assignment on a local does not force the in-place tie.

   AUDIT-LANE RE-MEASURE: 99.89% / 716 bytes / 178 insns, the same six-line residual (only the
   andi/li register pair). A fourth permuter run (4 min, 1488 iterations) came back base=20 best=20.
   One new mechanism fact, measured here for the first time: the in-place andi and the un-folded
   `li 1` are MUTUALLY EXCLUSIVE in this compiler. casting the AND's OPERAND instead of its result
   (`((u32)pSpriteData->flags_0xA8 & 1) == 1`, and the same with a 1U mask) DOES give retail's
   `andi v0,v0,0x1` in place, but cc1 then folds the same-type compare to `beqz v0` and the function
   drops to 177 insns / 712 bytes / 98.99%. Keeping the li (needed: 178 insns) requires the AND's
   operand and the comparison to disagree in type, which is exactly what makes the andi take a fresh
   v1. So no single expression of this test emits retail's `andi v0,v0,1 / li v1,1 / bne` - park,
   the tie is closed from the other side. See lever
   a-unsigned-and-folds-its-own-compare-before-it-can-be-in-place.

*/
// Parked AUDIT round: 99.10% (312 bytes / 78 insns = retail exact size), re-measured from this
// body after un-parking, unchanged. The residual is ONE mirrored caller-saved pair inside the two flag
// read-modify-writes: retail keeps the 0xFFF8007F mask in v0 and the 0xAC word in v1 (and the 0xA8
// word then takes the v0 the mask frees); ours compiles the mirror. Every other instruction, the
// frame, the A8 mask in a0 and the bgez delay slot are exact.
// Mechanism (lever a-flag-word-rmw-mask-can-never-take-v0, from cc1 -dl on the live body under this
// unit cdk -O2 -G8): the loaded word is reg 83 "used 6 times across 6 insns", the mask reg 112
// "2 times across 6", so local-alloc floor_log2(n_refs)*n_refs/live_length is 2.0 for the word
// against 0.33 for the mask, and force_reg creates the mask pseudo AFTER the word one; the mask could
// only win with 4 insns mentioning it, which the retail stream does not contain. Same flip as
// func_8002435C, so one park covers the family.
// Tried and measured (all 99.10% / 312 bytes / same pair unless noted): the 12-bit bitfield view,
// (0x100 & 0xFFF) << 7 and the or with 0x8000, constant-first operand order, the mask in a declared
// local, loaded-word temp (96.67), intermediate word-or-0x8000 (93.78), pFlagsA8[1] and [0] indexing
// (67.63), every access cast through (u8*)pSpriteData (70-67, so keep exactly this mix), statement
// swap (93.37), and the in-place chain on a named local (identical stream).
// Presets: cdk -G0 96.54, gcc-2.6.0-psx 74.94, gcc-2.7.2-psx 77.18, cdk -O3 does not compile, so
// XenoGraphics cdk -O2 -G8 is right. Permuter four runs (1309, 1001, 1342 and this round 4-minute
// 1338 iterations): base 70 best 70 every time. Verdict: GIV_TIE, do not re-brief as a hand job.
/*
void func_80023804(SpriteData* pSpriteData) {
    u32* pFlagsA8;
    s32 cycles;

    *(u32*)((u8*)pSpriteData + 0x3C) = 0;
    pSpriteData->prim = 0x2D;
    pSpriteData->field_0x40 = 0;
    pSpriteData->field_0x3A = 0;
    pSpriteData->field_0x30 = 0;
    pSpriteData->direction = 0;
    pSpriteData->curSpriteFrame = 0;
    pSpriteData->flags_0xA8 = 0;
    *(u32*)((u8*)pSpriteData + 0x3C) &= ~4;
    *(u32*)((u8*)pSpriteData + 0x3C) &= ~8;
    *(u32*)((u8*)pSpriteData + 0x3C) &= ~0x10;
    *(u32*)((u8*)pSpriteData + 0x3C) &= 0xFF0FFFFF;
    *(u32*)((u8*)pSpriteData + 0x3C) &= 0xFEFFFFFF;
    *(u32*)((u8*)pSpriteData + 0x3C) &= 0xFFF0FFFF;
    *(u32*)((u8*)pSpriteData + 0x40) &= ~0xFC;
    *(u32*)((u8*)pSpriteData + 0x40) &= ~0x1F00;
    *(u32*)((u8*)pSpriteData + 0x40) &= 0xFFFE1FFF;
    pFlagsA8 = (u32*)&pSpriteData->flags_0xA8;
    pFlagsA8[1] = 0;
    pFlagsA8[2] = 0;
    *(u8*)((u8*)pSpriteData + 0xB0) = 0;
    *(u8*)((u8*)pSpriteData + 0xAF) = 0;
    pSpriteData->flags_0xAC.flags = (pSpriteData->flags_0xAC.flags & 0xFFF8007F) | (0x100 << 7);
    pSpriteData->flags_0xA8 &= 0xFFF1FFFF;
    cycles = g_AnimScriptExtraTicks + 1;
    pSpriteData->gravity = (cycles * 0x4000) * cycles * pSpriteData->field_0x82 / 4096;
    pSpriteData->pSpriteBytecode = NULL;
    pSpriteData->field_0x70 = NULL;
    pSpriteData->pCurAnimFile = NULL;
    pSpriteData->field_0x68 = NULL;
    pSpriteData->field_0x80 = 0;
    pSpriteData->stackIndex = 0x10;
    pSpriteData->field_0x84 = 0;
    pSpriteData->field_0x6C = NULL;
    pSpriteData->field_0x50 = 0;
}*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80023804);




void SpriteResetBase(SpriteDataI1* pBase) {
    pBase->rotation.x = 0;
    pBase->rotation.y = 0;
    pBase->rotation.z = 0;
    pBase->pFramesData = NULL;
}

void SpriteClearBase(SpriteData* pSpriteData) {
    pSpriteData->pBase = NULL;
}

void SpriteInitBaseAndClearInner3(SpriteData* pSpriteData) {
    pSpriteData->pBase = &pSpriteData->inner1;
    SpriteResetBase(&pSpriteData->inner1);
    pSpriteData->pBase->pDirTransforms = NULL;
    // pBase points at inner1, so inner3 sits right after it
    ((SpriteDataI3*)(pSpriteData->pBase + 1))->field_0x0 = 0;
}

void SpriteInitBaseWithInternalStorage(SpriteData* pSpriteData) {
    pSpriteData->pBase = &pSpriteData->inner1;
    SpriteResetBase(&pSpriteData->inner1);
    pSpriteData->field_0x7C = &pSpriteData->inner3;
    pSpriteData->pBase->pDirTransforms = &pSpriteData->directionTransforms;
    pSpriteData->pVramData = &pSpriteData->i2CurPackage;
    pSpriteData->pBase->pNextSpriteData = NULL;
}

void SpriteInitBaseWithRenderFrames(SpriteData* pSpriteData) {
    pSpriteData->pBase = &pSpriteData->inner1;
    SpriteResetBase(&pSpriteData->inner1);
    pSpriteData->pBase->pCurRenderFramesData = &pSpriteData->inner3;
    pSpriteData->pBase->pDirTransforms = NULL;
    pSpriteData->pBase->pNextSpriteData = NULL;
}

extern u8 g_SpriteAnimPackageDirs5[];
extern u8 g_SpriteAnimPackageDirs6[];
extern s32 SpriteAnimPackageGetFrameCount(u16* pFlags);

AnimTask* AnimTaskCreate(s32 arg0, s32 type, void* pAnimPackage, s32 size, void* pData) {
    AnimTask* pEntry;
    SpriteData* pSpriteData;
    s32 dataSize;

    switch (type) {
    case 0:
        dataSize = 0;
        pEntry = AnimTaskAlloc(pData, dataSize + size);
        SpriteClearBase(&pEntry->spriteData);
        break;
    case 1:
        // indexed, not plain &g_SpriteAnimPackageDirs5: the index makes gcc build the address as
        // lui v0,%hi + addiu s2,v0,%lo (a temp for the high half) as the original does
        if (arg0 == 5) {
            pAnimPackage = &g_SpriteAnimPackageDirs5[0];
        }
        if (arg0 == 6) {
            pAnimPackage = &g_SpriteAnimPackageDirs6[0];
        }
        dataSize = (SpriteAnimPackageGetFrameCount(((SpriteAnimPackage*)pAnimPackage)->pFrames) - 1) *
                       sizeof(SpriteFrameData) + 0x58;
        pEntry = AnimTaskAlloc(pData, dataSize + size);
        SpriteInitBaseWithRenderFrames(&pEntry->spriteData);
        break;
    case 2:
        dataSize = 0x54;
        pEntry = AnimTaskAlloc(pData, dataSize + size);
        SpriteInitBaseAndClearInner3(&pEntry->spriteData);
        break;
    }
    pSpriteData = &pEntry->spriteData;
    pSpriteData->field_0x6C = pEntry;
    pSpriteData->allocatedDataSize = dataSize + 0xEC;
    pSpriteData->pVramData = pAnimPackage;
    return pEntry;
}


extern u8 g_FlagTimerWorkListAdds;
extern void func_80023538(SpriteData* pSpriteData, u16* pAnimEntry);
extern void func_80024730(AnimTask* pAnimTask);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80023B84);
// Best attempt: 85.47% (1100 bytes / 275 instructions; retail is 1108 / 277). The residual is
// retail's EIGHTH callee-saved register and the two instructions its save/restore pair costs, plus
// the register renaming that follows from it. Retail hands out pSpriteData=s2, pAnimEntry=s7,
// pAnimPackage=s3, numDirections=s1, type=s5, pAnimTask=s4, pTarget=s0, savedWorkListAdds=s6. This
// body hands out seven, because pAnimPackage (last use 0x1441C) and pTarget (born 0x14448) do not
// interfere and gcc gives both s0. Retail's s3 is `lw $s3,0x24($s0)` - a provably DEAD read of
// pTarget->pVramData (the only read of s3 in the function, and nothing stores +0x24 anywhere), so
// the pair exists exactly because that dead load ended up in a call-crossing pseudo's register.
// LOAD-BEARING SOURCE FACTS (each measured): the field_0x7C pair must be written with the NEGATED
// test (`if (!(pSpriteData->flags_0xA8 & 1))` copy, else NULL) - the plain form is 84.31 and lays
// the arms out the other way; inside the flagBit block field_0x3A is stored BEFORE field_0x40; and
// the field_0x40/0x3C stores stay in the interleaved order below. The self-store
// `pTarget->pVramData = pVramData;` is deleted by cse as redundant while the load survives - that is
// the only way to get the load into the body at all.
// MEASURED DEAD, all against the 85.47 / 1100 baseline unless noted: routing the parameter through
// the local (`pVramData = pAnimPackage;` at the top, pVramData passed to AnimTaskCreate) 85.47 /
// 1100 - gcc coalesces the copy and the allocno merge is unchanged; `pAnimPackage =
// pTarget->pVramData;` alone 85.35 / 1096 / 274 (delete_trivially_dead_insns removes the load
// outright); the parameter as scratch WITH the self-store back 85.35 / 1096 / 274 and gcc keeps the
// STORE and drops the load; load kept and the store moved after func_80024730 85.96 / 1104 / 276 -
// the best score any shape has reached, but WRONG CODE (gcc drops the load and emits `sw
// $s2,0x24($s0)`, storing numDirections' register), so it is not a body to ship; loading through
// pAnimTask->spriteData.pVramData before pTarget exists 84.61 / 1104 / 276; the pair at the tail
// before func_80023538 84.40; -G0 84.35; gcc-2.6.0 compile-fails; and all nine declaration
// permutations of the eight locals 85.47, so the hand-out is not allocno-number driven.
// The masks an earlier note blamed are NOT the cause: this body materialises both field_0x40 masks
// (0xFFFE1FFF, 0xFFFBFFFF) at the head of the post-call block exactly as retail does.
// Verdict: GIV_TIE, not a spelling problem - lever a-dead-load-emitted-but's prescription ("find the
// call that consumed it") has no candidate here, and a source use of the loaded value reintroduces
// an instruction retail does not have. The three externs the body needs are already live above it.
/*
// Fills a fresh AnimTask's sprite from an existing one; the work-list counter is saved and
// restored around the body because AnimTaskCreate adds work-list entries.
SpriteData* func_80023B84(SpriteData* pSpriteData, u16* pAnimEntry, void* pAnimPackage) {
    AnimTask* pAnimTask;
    SpriteData* pTarget;
    void* pVramData;
    u8 savedWorkListAdds;
    s32 numDirections;
    s32 type;
    u32 flags;
    u32 flagBit;

    savedWorkListAdds = g_FlagTimerWorkListAdds;
    pSpriteData->flags_0xB0.flags |= 0x800;
    if (((u32)pSpriteData->flags_0xB0.flags >> 8) & 1) {
        g_FlagTimerWorkListAdds = 0;
    }
    numDirections = AnimEntryGetNumDirections(pAnimEntry);
    if (numDirections == 3) {
        numDirections = ((u32)pSpriteData->field_0x40 >> 13) & 0xF;
    }
    type = func_80023468(numDirections);
    pAnimTask = AnimTaskCreate(numDirections, type, pAnimPackage, 0, pSpriteData->field_0x6C);
    pAnimTask->task1.unk14_1 = 1;
    pTarget = &pAnimTask->spriteData;
    pVramData = pTarget->pVramData;
    pTarget->pVramData = pVramData;
    pTarget->field_0x40 = (pTarget->field_0x40 & 0xFFFE1FFF) | ((numDirections & 0xF) << 13);
    g_SpriteFlags0x3C(pTarget).flags = (g_SpriteFlags0x3C(pTarget).flags & ~4) | (type & 3);
    pTarget->field_0x40 = (pTarget->field_0x40 & ~0x1F00) | (pSpriteData->field_0x40 & 0x1F00);
    g_SpriteFlags0x3C(pTarget).flags =
        (g_SpriteFlags0x3C(pTarget).flags & ~8) | (g_SpriteFlags0x3C(pSpriteData).flags & 8);
    g_SpriteFlags0x3C(pTarget).flags =
        (g_SpriteFlags0x3C(pTarget).flags & ~0x10) | (g_SpriteFlags0x3C(pSpriteData).flags & 0x10);
    g_SpriteFlags0x3C(pTarget).bytes[1] = g_SpriteFlags0x3C(pSpriteData).bytes[1];
    pTarget->field_0x40 = (pTarget->field_0x40 & ~0x40000) | (pSpriteData->field_0x40 & 0x40000);
    g_SpriteFlags0x3C(pTarget).flags = (g_SpriteFlags0x3C(pTarget).flags | 0x4000000) & ~4;
    pTarget->moveSpeed = pSpriteData->moveSpeed;
    pTarget->direction = pSpriteData->direction;
    pTarget->scale = pSpriteData->scale;
    pTarget->curSpriteFrame = pSpriteData->curSpriteFrame;
    flagBit = ((u32)pSpriteData->flags_0xB0.flags >> 9) & 1;
    pTarget->flags_0xB0.flags = (pTarget->flags_0xB0.flags & ~0x200) | (flagBit << 9);
    if (flagBit) {
        pTarget->field_0x3A = pSpriteData->field_0x3A;
        pTarget->field_0x40 = (pTarget->field_0x40 & ~0x1F00) | 0x300;
    }
    flags = ((u32)pSpriteData->flags_0xA8 >> 30) | ((pSpriteData->flags_0xAC.flags & 3) << 2);
    pTarget->flags_0xA8 = (pTarget->flags_0xA8 & 0x3FFFFFFF) | (flags << 30);
    pTarget->flags_0xAC.flags = (pTarget->flags_0xAC.flags & ~3) | (flags >> 2);
    pTarget->flags_0xB0.flags = (pTarget->flags_0xB0.flags & ~0x100) | (pSpriteData->flags_0xB0.flags & 0x100);
    pTarget->flags_0xAC.flags = (pTarget->flags_0xAC.flags & ~0x40) | (pSpriteData->flags_0xAC.flags & 0x40);
    pTarget->flags_0xAC.flags =
        (pTarget->flags_0xAC.flags & 0xFFF8007F) | (pSpriteData->flags_0xAC.flags & 0x7FF80);
    pTarget->flags_0xA8 &= ~1;
    pTarget->flags_0xAC.flags = (pTarget->flags_0xAC.flags & ~4) | (pSpriteData->flags_0xAC.flags & 4);
    if (!(pSpriteData->flags_0xA8 & 1)) {
        pTarget->field_0x7C = pSpriteData->field_0x7C;
    } else {
        pTarget->field_0x7C = NULL;
    }
    pTarget->field_0x70 = pSpriteData;
    pTarget->pCurAnimFile = pSpriteData->pCurAnimFile;
    pTarget->pDefaultAnimFile = pSpriteData->pDefaultAnimFile;
    pTarget->pTargetEntitySprite = pSpriteData->pTargetEntitySprite;
    pTarget->field_0x82 = pSpriteData->field_0x82;
    pTarget->field_0x50 = pSpriteData->field_0x50;
    pTarget->field_0x8D = pSpriteData->flags_0xAC.bytes[3];
    pTarget->field_0x78 = pSpriteData->field_0x78;
    pTarget->position.x = pSpriteData->position.x;
    pTarget->position.y = pSpriteData->position.y;
    pTarget->position.z = pSpriteData->position.z;
    pTarget->step.x = pSpriteData->step.x;
    pTarget->step.y = pSpriteData->step.y;
    pTarget->step.z = pSpriteData->step.z;
    if (type != 0) {
        pTarget->pBase->rotation.x = pSpriteData->pBase->rotation.x;
        pTarget->pBase->rotation.y = pSpriteData->pBase->rotation.y;
        pTarget->pBase->rotation.z = pSpriteData->pBase->rotation.z;
        pTarget->pBase->scale.x = pSpriteData->pBase->scale.x;
        pTarget->pBase->scale.y = pSpriteData->pBase->scale.y;
        pTarget->pBase->scale.z = pSpriteData->pBase->scale.z;
    }
    func_80023538(pTarget, pAnimEntry);
    func_80024730(pAnimTask);
    g_FlagTimerWorkListAdds = savedWorkListAdds;
    return pTarget;
}*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80023FD8);
/* Best attempt: 85.83% (704 bytes / 176 insns; the original is 700 / 175). The body is
   structurally right -- every load, store, mask and call is in place and in order -- and folding
   the register names out of the two listings leaves 32 differing rows of 208: the residual is
   register allocation plus the reload interleave it drags in, in three parts.
   (1) pAnimFile wants s1 and jointTaskSize s0 (this C: the other way round), which renames s0/s1
   in every later instruction and swaps the first and last prologue save. Both are live across
   the same two calls and both enter through an arg register, so it is a global-alloc allocno
   priority tie; a `taskSize` local for the 4th argument does not move it.
   (2) retail keeps pSpriteData in t0 from `addiu t0,s0,0x38` to the end and reuses the dead s1
   for pVramData (`lw s1,0x24(t0)`); this C uses a3 for the sprite pointer and t1 for pVramData,
   and the pVramData load/store pair at 0x24 is the ONE extra instruction (704 vs 700 bytes).
   (3) the 0x3C/0x40/0xA8/0xAC read-modify-write block: retail materialises 0x7FF80 and 0xFFFF in
   the delay slot of the first beqz, this C loads them inline, and the two independent loads of
   each pair are interleaved differently -- a consequence of (1)+(2)'s pressure, not of statement
   order (the C statement order is already retail's).
   Also: the 0x3D/0x8D byte copies, `flags` fold (0xA8 bits 30-31 <-> 0xAC bits 0-1) and the two
   `field_0x7C` stores all match as written.
   Tried earlier: indexed vs pointer table access, `(offset + base)` spelled as an int sum (this
   is what fixed the addu operand order, 77.05 -> 85.83) and a `taskSize` local. The call to
   func_80023468 passes ONE argument in retail (no `move a1,...` before the jal), which is why
   func_80023468 is defined K&R above; with a prototype the call costs an extra `move`.
   No permuter (85.83 is far below the 99% gate). Unparking needs `extern SpriteData*
   D_800C3E1C;` (the g_IsBattle / g_GfxWorkListCapacity externs are already live above).
   Re-measured 85.83% / 704 bytes / 176 insns this round after unpark - unchanged. New facts,
   both measured here: (a) the INSTRUCTION MULTISET is identical - folding registers and
   immediates leaves exactly ONE difference, ours has one extra `nop`, at the point where retail's
   second mask materialisation (`ori a3,a3,(0x7FF80 & 0xFFFF)`) sits in the `lw v0,0x48(t1)`
   load-delay slot and ours has a nop there with the `ori` three instructions later; every other
   row is a register rename (our s0<->s1, a3 vs t0 for the pSpriteData pointer, t1 for pVramData
   where retail reuses the dead s1, and the two 0x3C/0x40 read-modify-write chains emitted in the
   other order). So the whole residual is callee-saved hand-out plus one slot - nothing is missing
   from the source. (b) Preset ruled out with numbers on THIS body: cdk -O2 -G0 85.23, cdk -O3 78.83, gcc-2.6.0 compile-fails, so the unit's XenoGraphics cdk -O2 -G8 is best. The unit runs
   with -fschedule-insns and -fschedule-insns2, so the two chain orders are the scheduler's.
   Verdict: GIV_TIE, same class as func_80023804 / func_8002435C above.
   Re-measured 85.83% / 704 bytes / 176 insns this round, unchanged. One more spelling measured dead:
   `s32 size; size = jointTaskSize;` then passing `size` to AnimTaskCreate is byte-identical, so a
   copy does not add the reference that would move the hand-out. Root cause narrowed by hand from the
   -dl priority formula ((floor_log2(refs)+1)*refs*10000/(live*size)): the 2nd parameter has TWO RTL
   references here (the offset-table load and AnimTaskCreate's 3rd argument), so it outranks the 4th
   parameter's single reference and takes the first callee-saved register itself; retail's order is
   what you get when the 2nd parameter reaches the RTL with ONE reference, and no C spelling consumes
   pAnimFile once while still passing it to AnimTaskCreate. Same wall as
   a-qiparam-size-one-pins-the-saved-pair. Also stale above: D_800C3E1C, g_IsBattle and
   g_GfxWorkListCapacity are all declared live in this file.

AnimTask* func_80023FD8(s32 index, void* pAnimFile, ActorRotation* pOffset, s32 jointTaskSize) {
    u8* pAnimationOffsets;
    void* pAnimEntry;
    AnimTask* pAnimTask;
    SpriteData* pSpriteData;
    SpriteData* pTemplate;
    void* pVramData;
    s32 numDirections;
    s32 type;
    u32 flags;

    pAnimationOffsets = *(u8**)((u8*)pAnimFile + 0x10);
    pAnimEntry = (void*)(((u16*)pAnimationOffsets)[index + 1] + (mem_addr)pAnimationOffsets);
    numDirections = AnimEntryGetNumDirections((u16*)pAnimEntry);
    type = func_80023468(numDirections);
    pAnimTask = AnimTaskCreate(numDirections, type, pAnimFile, jointTaskSize, NULL);
    *(u32*)((u8*)pAnimTask + 0x14) |= 0x20000000;
    pSpriteData = &pAnimTask->spriteData;
    pVramData = pSpriteData->pVramData;
    pSpriteData->field_0x70 = NULL;
    pSpriteData->pTargetEntitySprite = NULL;
    if (g_IsBattle) {
        pTemplate = D_800C3E1C;
        if (pTemplate) {
            pSpriteData->pCurAnimFile = pTemplate->pCurAnimFile;
            pSpriteData->pDefaultAnimFile = pTemplate->pDefaultAnimFile;
            pSpriteData->pTargetEntitySprite = pTemplate->pTargetEntitySprite;
            pSpriteData->moveSpeed = pTemplate->moveSpeed;
            pSpriteData->direction = pTemplate->direction;
            pSpriteData->field_0x40 = (pSpriteData->field_0x40 & ~0x1F00) | (pTemplate->field_0x40 & 0x1F00);
            (*(u32*)((u8*)pSpriteData + 0x3C)) = ((*(u32*)((u8*)pSpriteData + 0x3C)) & ~8) | ((*(u32*)((u8*)pTemplate + 0x3C)) & 8);
            (*(u32*)((u8*)pSpriteData + 0x3C)) = ((*(u32*)((u8*)pSpriteData + 0x3C)) & ~0x10) | ((*(u32*)((u8*)pTemplate + 0x3C)) & 0x10);
            *(u8*)((u8*)pSpriteData + 0x3D) = *(u8*)((u8*)pTemplate + 0x3D);
            pSpriteData->scale = pTemplate->scale;
            (*(u32*)((u8*)pSpriteData + 0x3C)) = ((*(u32*)((u8*)pSpriteData + 0x3C)) | 0x4000000) & ~4;
            pSpriteData->flags_0xAC.flags = (pSpriteData->flags_0xAC.flags & ~4) | (pTemplate->flags_0xAC.flags & 4);
            pSpriteData->flags_0xAC.flags =
                (pSpriteData->flags_0xAC.flags & 0xFFF8007F) | (pTemplate->flags_0xAC.flags & 0x7FF80);
            pSpriteData->field_0x7C = pTemplate->field_0x7C;
            pSpriteData->field_0x7C = pTemplate->field_0x7C;
            flags = ((u32)pTemplate->flags_0xA8 >> 30) | ((pTemplate->flags_0xAC.flags & 3) << 2);
            pSpriteData->flags_0xA8 = (pSpriteData->flags_0xA8 & 0x3FFFFFFF) | (flags << 30);
            pSpriteData->flags_0xAC.flags = (pSpriteData->flags_0xAC.flags & ~3) | (flags >> 2);
            pSpriteData->field_0x50 = pTemplate->field_0x50;
            *(u8*)((u8*)pSpriteData + 0x8D) = *(u8*)((u8*)pTemplate + 0xAF);
        }
    }
    pSpriteData->pCurAnimFile = NULL;
    pSpriteData->pDefaultAnimFile = NULL;
    pSpriteData->curSpriteFrame = 0;
    pSpriteData->pVramData = pVramData;
    pSpriteData->field_0x40 = (pSpriteData->field_0x40 & 0xFFFE1FFF) | ((numDirections & 0xF) << 13);
    (*(u32*)((u8*)pSpriteData + 0x3C)) = ((*(u32*)((u8*)pSpriteData + 0x3C)) & ~3) | (type & 3);
    pSpriteData->field_0x82 = *(u16*)&g_GfxWorkListCapacity;
    pSpriteData->position.x = pOffset->vx << 16;
    pSpriteData->position.y = pOffset->vy << 16;
    pSpriteData->position.z = pOffset->vz << 16;
    func_80023538(pSpriteData, pAnimEntry);
    func_80024730(pAnimTask);
    return pAnimTask;
}
*/

extern void SpriteAllocFromAnimFile(void* pAnimFile, s16 arg1, s16 arg2, s16 arg3, s16 arg4, s16 arg5);

void SpriteAllocFromAnimFileWithClut(void* pAnimFile, s16 arg1, s16 arg2, s16 arg3, s16 arg4, s16 arg5, s32 arg6) {
    g_SpritePendingClut = arg6;
    SpriteAllocFromAnimFile(pAnimFile, arg1, arg2, arg3, arg4, arg5);
    g_SpritePendingClut = 0;
}

extern SpriteData* func_8002435C(SpriteData* pSpriteData, void* pAnimFile, s16 arg2, s16 arg3, s16 arg4,
                                 s16 arg5, s16 arg6);

void SpriteInitFromAnimFileWithClut(SpriteData* pSpriteData, void* pAnimFile, s16 arg2, s16 arg3, s16 arg4, s16 arg5,
                   s16 arg6, s32 arg7) {
    g_SpritePendingClut = arg7;
    func_8002435C(pSpriteData, pAnimFile, arg2, arg3, arg4, arg5, arg6);
    g_SpritePendingClut = 0;
}

// ROUND 548: 99.63% / 456 bytes / 114 insns / 8 differing lines (was 99.19 / 20 lines). The permuter (4 min, 1223 iters, base 90 best 40) closed residual (a) - the mask constant in v0 and the 0x3C word in v1, retail pair - with ONE edit: name the AND result into a local that is dead but defined again later, i.e.
//        *(u32*)((u8*)pSpriteData + 0x3C) = (pAnimHeader = (*(u32*)((u8*)pSpriteData + 0x3C)) & ~3) | 1;
//    The longer live range of the AND-defined pseudo is what pushes the constant out of v1. Semantics are unchanged (pAnimHeader is overwritten at the 0x48 header read before any use), but the spelling is not source a 1998 programmer would write: a realistic sink is still owed.
//    Residuals now: (b) the prologue emits "sw s0,16(sp) / move s0,a0" before the s6 pair where retail has the s6 pair first (sched1 is ON for this unit: a -fno-schedule-insns/-insns2 compile of this body is an unscheduled prologue, so retail is scheduled and the swap is sched1 order, not an allocation); (c) NEW - the anim header load lands in v1 with "addu a0,v1,s1" where retail has "lw a0,8(s1)" + "addu a0,a0,s1": the sink above extends a pseudo that costs the header value its coalescing into a0. Measured dead this round at 99.63 or worse, all 456 bytes / 114 insns: sink = flags instead of pAnimHeader (98.45, the whole later 0x3C block rotates), the header pointer built in two statements then passed alone (identical stream, 99.63), and the pre-round body at 99.19. Next lever: choose a sink that dies before the header read, so (a) stays closed and (c) returns.
// Two source facts from the earlier rounds are load-bearing, keep them: (1) pAnimFileData = pAnimFile as the
// FIRST statement of the body - a local assigned where it is used is copy-propagated away, so the function
// saves six callee-saved registers instead of seven; with the copy first gcc keeps pAnimFile in s6 for the
// tail (the 0x48 store and the SpriteSetAnimFile call) and a second copy in s1 for the 0x8 offset load
// (89.07 -> 92.17). (2) isBattle = g_IsBattle into a u8 local before the 0x3C read-modify-write; read
// inline, gcc emits the flag block as straight-line statement order instead of retail's interleave with the
// branch last (92.17 -> 99.19). Dead ends already measured (all 456 bytes / 114 insns): the word in a local
// before the store 98.45, a u32 pointer local to 0x3C 99.19, the bitfield SpriteFlagsWord view 92.17, the
// two statements swapped 91.73, s32 isBattle 99.19, u8* for pAnimFile 99.19, "&= ~3" then "|= 1" 99.19.
// Presets: gcc-2.7.2-psx 94.97, this unit's cdk 99.19 (and 99.63 with the sink), 2.6.0 fails to compile;
// the unit cannot move to 2.7.2 (matched neighbours AnimScriptTick 94.75, AnimTaskAlloc 79.92,
// AnimTaskCreate 88.14, func_80023468 86.41). Three earlier permuter runs: base 90 best 90.
/*
SpriteData* func_8002435C(SpriteData* pSpriteData, void* pAnimFile, s16 arg2, s16 arg3, s16 arg4,
                          s16 arg5, s16 arg6) {
    SpriteDataI1* pBase;
    SpriteDataI1* pSpriteBase;
    void* pFramesData;
    u8* pAnimations;
    u8* pAnimFileData;
    mem_addr pAnimHeader;
    u8 isBattle;
    u32 flags;

    pAnimFileData = pAnimFile;
    func_80023804(pSpriteData);
    SpriteInitBaseWithInternalStorage(pSpriteData);
    SpriteSetScale(pSpriteData, 0x1000);
    isBattle = g_IsBattle;
    *(u32*)((u8*)pSpriteData + 0x3C) = (pAnimHeader = (*(u32*)((u8*)pSpriteData + 0x3C)) & ~3) | 1;
    pSpriteData->field_0x40 &= 0xFFFE1FFF;
    if (isBattle) {
        pSpriteData->flags_0xA8 &= ~1;
        ((SpriteDataI3*)pSpriteData->field_0x7C)->field_0x8 = 0;
        ((SpriteDataI3*)pSpriteData->field_0x7C)->field_0xC = 0;
    } else {
        pSpriteData->flags_0xA8 |= 1;
        ((SpriteDataI3*)pSpriteData->field_0x7C)->field_0x18 = NULL;
    }
    flags = *(u32*)((u8*)pSpriteData + 0x3C);
    pSpriteData->field_0x6C = pSpriteData;
    flags &= 0xFF0FFFFF;
    flags |= (g_SpritePendingClut & 0xF) << 20;
    flags &= 0xFFF0FFFF;
    flags |= (g_SpritePendingClut & 0xF) << 16;
    *(u32*)((u8*)pSpriteData + 0x3C) = flags;
    pAnimHeader = *(u32*)(pAnimFileData + 8);
    pFramesData =
        HeapAlloc(SpriteAnimPackageGetFrameCount((u16*)(pAnimHeader + (mem_addr)pAnimFileData)) * sizeof(SpriteFrameData), 0);
    pBase = pSpriteData->pBase;
    pBase->pFramesData = pFramesData;
    pBase->pCurRenderFramesData = pFramesData;
    ((SpriteAnimPackage*)pSpriteData->pVramData)->tex.x = arg4;
    ((SpriteAnimPackage*)pSpriteData->pVramData)->tex.y = arg5;
    ((SpriteAnimPackage*)pSpriteData->pVramData)->clut.x = arg2;
    ((SpriteAnimPackage*)pSpriteData->pVramData)->clut.y = arg3;
    pSpriteData->pDefaultAnimFile = pAnimFile;
    SpriteSetAnimFile(pSpriteData, pAnimFile);
    pAnimations = ((SpriteAnimPackage*)pSpriteData->pVramData)->pAnimations;
    pSpriteData->field_0x60 = (u32)pAnimations + ((*(u16*)pAnimations & 0x3F) + 1) * 2;
    SpritePlayAnimation(pSpriteData, 0);
    return pSpriteData;
}*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_8002435C);


void SpriteAllocFromAnimFile(void* pAnimFile, s16 arg1, s16 arg2, s16 arg3, s16 arg4, s16 arg5) {
    SpriteData* pSpriteData;

    pSpriteData = HeapAlloc(sizeof(SpriteData), 0);
    pSpriteData->allocatedDataSize = sizeof(SpriteData);
    func_8002435C(pSpriteData, pAnimFile, arg1, arg2, arg3, arg4, arg5);
}

extern void SpriteSetAnimFile(SpriteData* pSpriteData, void* pAnimFile);
extern s32 SpriteAnimPackageTestFlag8000(void* pFrames);
extern void func_800223B0(SpriteData* pSpriteData, s16 arg1);
extern void func_80023538(SpriteData* pSpriteData, u16* pAnimEntry);
extern u8 g_IsBattle;

void SpritePlayAnimation(SpriteData* pSpriteData, s32 animId) {
    u8* pAnimations;
    u16 animOffset;

    if (pSpriteData->pDefaultAnimFile == NULL) {
        pSpriteData->pSpriteBytecode = NULL;
        return;
    }
    if (pSpriteData->pCurAnimFile == pSpriteData->pDefaultAnimFile) {
        pSpriteData->flags_0xB0.flags &= ~0x400;
    } else {
        pSpriteData->flags_0xB0.flags |= 0x400;
    }
    if (animId < 0) {
        SpriteSetAnimFile(pSpriteData, pSpriteData->pSpecialAnimFile);
        if (g_IsBattle && SpriteAnimPackageTestFlag8000(((SpriteAnimPackage*)pSpriteData->pVramData)->pFrames) == 0) {
            ((SpriteAnimPackage*)pSpriteData->pVramData)->tex.y = 0x100;
            ((SpriteAnimPackage*)pSpriteData->pVramData)->tex.x = 0x300;
        }
    } else {
        SpriteSetAnimFile(pSpriteData, pSpriteData->pDefaultAnimFile);
        if (g_IsBattle) {
            ((SpriteAnimPackage*)pSpriteData->pVramData)->tex =
                *(SVEC2*)&((SpriteDataI3*)pSpriteData->field_0x7C)->texX;
        }
    }
    pSpriteData->flags_0xAC.bytes[3] = (u8)animId;
    if (animId < 0) {
        animId = ~animId;
    }
    pAnimations = ((SpriteAnimPackage*)pSpriteData->pVramData)->pAnimations;
    animOffset = ((u16*)pAnimations)[animId + 1];
    pSpriteData->field_0x40 |= 0x100000;
    pSpriteData->pCurAnimation = pAnimations + animOffset;
    func_80023538(pSpriteData, pSpriteData->pCurAnimation);
    func_800223B0(pSpriteData, pSpriteData->field_0x80);
}

// Best attempt: 82.67% (400 bytes / 100 insns; retail is 420 / 105).
// Body shape is retail's: switch on (field_0x40 >> 13) & 0xF with the empty 0-6/14 cases
// spelt out (that is what makes retail's `sltiu v0,s1,0xF` bound check), arms in source
// order 12, 10, 13, 11, 7, 8, 9, the groupA position copy share-merged by the cross-jumper,
// and three scalar position assignments per arm (a struct assignment emits load-all then
// store-all through three registers instead of retail's one-register interleave).
// Residual is all allocator, not shape:
// 1) Retail's parameter pseudo lives in $a2 - `move a2,a0` at entry plus `move a0,a2`
//    before each of the four func_800BC158 calls, while the two `addiu s0/s2,a0` address
//    computations still read $a0. That copy is 5 of the 5 missing instructions. A second
//    local alias at the top puts it in $a1 AND routes the addius through it (416 bytes);
//    see lever a-a-surviving-copy-of-a-live-pointer-re-allocates-the-whole-function.
// 2) Retail runs the arm type chain in $v0 (srl v0,v0,13 ... andi v0,v0,0xf) and copies it
//    into $s1 last (`move s1,v0` after the `or`); this body masks straight into $s1 and
//    shifts from $s1. A `newType` local plus `animType = newType` buys the copy back
//    (408 bytes / 102 insns) but the scheduler hoists it above the `or`: 80.86%.
// Header: spelling it as two statements (`animType = field >> 13; animType &= 0xF;`) is
// what produces retail's `srl s1,v0,13 / andi s1,s1,0xf`.
/*
extern void func_800BC158(AnimTask* pAnimTask);
extern void SpriteSetTaskHandler(WorkListEntry* pTask, s32 animType);
extern IVEC3 g_BattleAnimPositionGroupA;
extern IVEC3 g_BattleAnimPositionGroupB;

#define ANIM_FIELD_TYPE_SHIFT 13

void func_80024730(AnimTask* pAnimTask) {
    SpriteData* pSpriteData = &pAnimTask->spriteData;
    WorkListEntry* pTask = &pAnimTask->task2;
    s32 animType;

    animType = pSpriteData->field_0x40 >> ANIM_FIELD_TYPE_SHIFT;
    animType &= 0xF;
    switch (animType) {
    case 12:
        pSpriteData->curSpriteFrame = 1;
        animType = (((pSpriteData->field_0x40 >> ANIM_FIELD_TYPE_SHIFT) & 0xF) - 2) & 0xF;
        pSpriteData->field_0x40 = (pSpriteData->field_0x40 & 0xFFFE1FFF) | (animType << ANIM_FIELD_TYPE_SHIFT);
        func_800BC158(pAnimTask);
        pSpriteData->position.x = g_BattleAnimPositionGroupA.x;
        pSpriteData->position.y = g_BattleAnimPositionGroupA.y;
        pSpriteData->position.z = g_BattleAnimPositionGroupA.z;
        break;
    case 10:
        func_800BC158(pAnimTask);
        pSpriteData->curSpriteFrame = 0;
        pSpriteData->position.x = g_BattleAnimPositionGroupA.x;
        pSpriteData->position.y = g_BattleAnimPositionGroupA.y;
        pSpriteData->position.z = g_BattleAnimPositionGroupA.z;
        break;
    case 13:
        pSpriteData->curSpriteFrame = 1;
        animType = (((pSpriteData->field_0x40 >> ANIM_FIELD_TYPE_SHIFT) & 0xF) - 2) & 0xF;
        pSpriteData->field_0x40 = (pSpriteData->field_0x40 & 0xFFFE1FFF) | (animType << ANIM_FIELD_TYPE_SHIFT);
        func_800BC158(pAnimTask);
        pSpriteData->position.x = g_BattleAnimPositionGroupA.x;
        pSpriteData->position.y = g_BattleAnimPositionGroupA.y;
        pSpriteData->position.z = g_BattleAnimPositionGroupA.z;
        break;
    case 11:
        func_800BC158(pAnimTask);
        pSpriteData->curSpriteFrame = 0;
        pSpriteData->position.x = g_BattleAnimPositionGroupB.x;
        pSpriteData->position.y = g_BattleAnimPositionGroupB.y;
        pSpriteData->position.z = g_BattleAnimPositionGroupB.z;
        break;
    case 7:
        TimerWorkListSetTaskCallback(pAnimTask, &AnimTaskCountedTimerTick);
        break;
    case 8:
        pSpriteData->prim = 0x68;
        pSpriteData->curSpriteFrame = 1;
        break;
    case 9:
        pSpriteData->field_0x36 = 3;
        pSpriteData->prim = 0x60;
        pSpriteData->curSpriteFrame = 1;
        break;
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 14:
        break;
    }
    SpriteSetTaskHandler(pTask, animType);
}*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp1", func_80024730);

