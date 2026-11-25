#include "common.h"
#include "system/menu.h"
#include "system/animation.h"
#include "psyq/inline_c.h"

typedef void (*SpriteScriptCallback)(SpriteData* pSpriteData);

// SpriteData's 0x3C flag word: the shared header only splits bits 20-31 into
// fields, so the two-bit mode and the flip bit this interpreter maintains sit
// inside its 20-bit field_0x3C_0 (the same view system/temp1.c declares).
typedef union {
    /* 0x0 */ u32 flags;
    struct {
        /* 0x0 */ u32 mode: 2;
        /* 0x2 */ u32 unk2: 1;
        /* 0x3 */ u32 flipX: 1;
        /* 0x4 */ u32 flipY: 1;
        /* 0x5 */ u32 unk5: 27;
    } fields;
} SpriteStateFlags;

typedef struct {
    /* 0x00 */ u8 unk0[0x3C];
    /* 0x3C */ SpriteStateFlags flags_0x3C;
} SpriteStateFlagsView;

#define g_SpriteStateFlags(pSpriteData) (((SpriteStateFlagsView*)(pSpriteData))->flags_0x3C)

// flags_0xA8 packs the frame counter (bits 11..16) and the script index (bits
// 22..27); both are read and written whole, so they live in masks.
#define ANIM_SCRIPT_FRAME_COUNTER_MASK 0xFFFE07FF
#define ANIM_SCRIPT_FRAME_COUNTER_SHIFT 11
#define ANIM_SCRIPT_SCRIPT_INDEX_MASK 0xF03FFFFF
#define ANIM_SCRIPT_SCRIPT_INDEX_SHIFT 22
#define ANIM_SCRIPT_COUNTER_MASK 0x3F
#define ANIM_SCRIPT_COUNTER(word, shift) (((u32)(word) >> (shift)) & ANIM_SCRIPT_COUNTER_MASK)
// Bits 28-29 of flags_0xA8 are the slot mode; 0x10000000 arms it.
#define ANIM_SCRIPT_MODE_MASK 0xCFFFFFFF
#define ANIM_SCRIPT_MODE_SET 0x10000000
// The 12-bit speed field of flags_0xAC, the 8-bit divisor a frame count is
// scaled by, and the animation id byte beside it.
#define ANIM_SCRIPT_SPEED_SHIFT 7
#define ANIM_SCRIPT_SPEED_MASK 0xFFF
#define ANIM_SCRIPT_SPEED_SCALE 256
#define ANIM_SCRIPT_ANIM_ID_DEFAULT 0x3F
#define ANIM_SCRIPT_FLIP_BIT 8
// The jump displacement is the 16-bit word the opcode carries, signed from its
// second byte; 0xFA builds the same word from its third byte.
#define SCRIPT_JUMP_AT(p) ((s32)((s16)((p)[1] << 8) | (p)[0]))
#define SCRIPT_JUMP_SHORT_AT(p) (((s16)((p)[2] << 8)) | (p)[1])

extern u8 g_IsBattle;
extern u8 g_AnimScriptCommandLengths[];  // per-opcode instruction length, indexed by the opcode byte
extern s32 g_WorkListCurTimer;
extern void func_800C11CC(SpriteData* pSpriteData);
extern void SpriteAnimFrameListSetFrame(SpriteData* pSpriteData, s32 frame);
extern void SpriteSetFrameFromAnimIndex(SpriteData* pSpriteData);
extern void SpritePlayAnimation(SpriteData* pSpriteData, s32 animId);
extern void func_8001FBE4(SpriteData* pSpriteData, s32 code, u8* pArgs);
extern void* AnimScriptResolveVariablePointer(SpriteData* pSpriteData, u8* pIndex);
extern void AnimScriptStackPushU24(SpriteData* pSpriteData, s32 value);
extern s32 AnimScriptStackPopU24(SpriteData* pSpriteData);
extern u_char AnimScriptStackPopU8(SpriteData* pSpriteData);
extern void AnimScriptStackPushU8(SpriteData* pSpriteData, u8 value);

// Ported from the func_800C11CC park in src/battle/main/misc35.c (the battle twin of
// this interpreter, same arm order) with the field SpriteData spelling of
// src/slus_006.64/system/temp1.c / animation_scripts.c.
// Measured levers of the older 71.83% body, still true:
//   - the dispatch MUST be a goto loop (label + `goto dispatch`), not while/for:
//     a switch inside a loop construct gives loop.c the outer LOOP notes and it hoists
//     both `li 1` and the &g_AnimScriptCommandLengths lui/addiu into the function
//     preheader as s5/s6 -> 61.25% and a 0x30 frame. goto: 67.82%, frame 0x28 = retail's.
//   - 0x82's `step.y` needs its OWN local (stepY): as part of `count` it is the one
//     live range that spans calls, so `count` got a callee-saved register and every
//     other register shifted one up (67.82 -> 69.15). With it, pSpriteData=s1, code=s2
//     (u8), frames=s3, pArgs=s0 exactly as retail.
//   - the script-index tail wants the masked flags in a local FIRST (mask = flags &
//     0xF03FFFFF, then mask |= count << 22) and the count == 0 arm re-derives from that
//     local, not from the stored field: 71.78. The signed `(count >> 6) & 8` and
//     `(flags >> 3 & 1) ^ (flags >> 2 & 1)` coped 71.83.
// WHAT IS LEFT (16 bytes over, 2 instructions in the head, the rest in 0x8E/0xE2):
//   - head tail: retail emits `addu v1,v1,s3; sh v1,158(s1)` BEFORE `sw a1,168(s1)`;
//     gcc emits the flags store first whatever the statement order (tried both).
//   - retail cross-jumps the head's count == 0 tail's `or; sw` with 0x81's
//     `| 0x10000000` store (:L80024DBC); ours emits its own pair.
//   - 0x8E: retail's `j` puts the pSpriteBytecode = NULL store in its frames slot, ours
//     does not.
//   - 0xE2: retail computes SCRIPT_JUMP_AT(pArgs) before the PushU24 call and re-signs
//     it after; with an `offset` local we get that, but the arm then also needs the
//     post-call add re-formed (ours cross-jumps into 0xE1's tail instead).
//   - 0xBE: retail loads flags_0xAC then the 0x3C word; gcc loads them the other way up.
// Best 90.37% (1596 bytes against retail's 1612, 399 vs 403 insns): the whole arm set is
// instruction-for-instruction right; what is left is register naming ($a0/$a2 and
// $s0/$s3 swapped all through the head and the 0xBE arm) and 4 instructions.
// MEASURED, in the order it paid:
//   - case 0x81's anim-id == DEFAULT path is a plain `goto` to a label on case 0x80's
//     body, not a second copy of it: retail's 0x81 is five instructions and its equal
//     path branches into 0x80's body at the `ori` after the `lui`. Duplicating the
//     body (what the previous park did) is 71.83%; sharing it is 90.37% and -8 insns.
//     This is the single find for this function: the interpreter's default-mode tail is
//     one piece of source, and gcc's own cross-jumping is what made it look inlined.
//   - the rest of the previous note still holds (goto dispatch loop, separate stepY
//     local, masked-script-index local first).
// WHAT IS STILL WRONG (register allocation, not structure):
//   - head: retail keeps the 0xF03FFFFF mask in $a2 and the flags word in $v0; gcc here
//     picks $a0 for the mask and $a2 for the word. The head's two stores are also
//     emitted in the other order (retail: addu, sh waitTimer, bnez, sw flags in the delay
//     slot; ours: sw flags, addu, bnez, sh).
//   - 0xBE arm: retail $a2 = script word, $s0 = delay, $a3 = offset, $a1 = ~8,
//     $a0/$v1 = the two flag words; ours $a1/$s3/$s2/$a0/$a2/$s0. Because of that
//     shift gcc also emits one extra `sra s3,v0,8` (delay slot) in the /256 round.
//   - 0xE2: retail computes pArgs[0] + ((s8)pArgs[1] << 8) with `addu` before the
//     PushU24 call and re-signs it as s16 after (`sll 16; sra 16`); ours does the `or`
//     form of SCRIPT_JUMP_AT and no re-sign.
// TRIED AND REJECTED (all measured, all worse than 90.37):
//   - 0xBE arm with its own block locals (count/delay/offset/flags/mask): 68.61 then
//     89.57 after the 0x81 find - the shared locals allocate better here.
//   - `offset = count & 0x1FF` moved before the frame-count multiply: 68.80, and it
//     pushes the script word into $t0.
//   - storing the 0x3C word twice (flags = mask; then .fields.flipY = (count >> 10) & 1,
//     which is what retail's unconditional `sw a1,0x3C` plus branch store really are):
//     68.17 alone, 71.58 with the stepY move - correct-looking, but it costs the
//     allocation 2% here.
//   - loading step.y after the callback in 0x82: 71.58, +1 insn.
// Not tried: the permuter (bar is 85% and this is at 90.37 -> one run is allowed by the
// STOP RULE exception only if the residual is named by a lever; it is regalloc, see
// lever.py 'regalloc callee-saved swapped tie').
// Best 97.22% (1592 bytes against retail's 1612, 398 vs 403 insns): the whole arm set
// matches instruction for instruction except one register rotation in the 0xBE arm and
// two tail merges (below).
// PORTED FROM the func_800C11CC park in src/battle/main/misc35.c with the earlier
// findings kept - the dispatch MUST be a goto loop (a switch inside a while/for gives
// loop.c the outer LOOP notes and hoists both `li 1` and the &g_AnimScriptCommandLengths
// lui/addiu into the preheader -> 61.25%), and case 0x81's anim-id == DEFAULT path is a
// goto onto case 0x80's body, not a second copy of it.
// MEASURED, in the order it paid (round 479):
//   - EVERY ARM MUST OWN ITS LOCALS. A local shared between arms that crosses a call
//     becomes a global allocno and takes a callee-saved register: giving the head block
//     its own headFlags/headMask/headCount moved the head from $s0/$a2/$a0 onto retail's
//     $a1/$a2/$v0 (90.37 -> 91.96), and giving the 0xBE arm its own beFrames did the
//     same there. Splitting the whole 0xBE arm at once is WORSE (91.07: pSpriteData
//     falls to $s2) and beOffset alone is worse too (91.19) - beFrames alone is the win.
//   - 0x80/0x81 must use the ANIM_SCRIPT_MODE_MASK macro INLINE (retail rematerialises
//     the constant twice per arm) and 0x82 must load step.y AFTER the callback, not
//     before its test: 93.11 -> 95.61.
//   - 0x98 wants a typed pLinked local for field_0x70 (retail keeps it in $a0) and the
//     anim-id compare as (s8)pSpriteData->field_0x8D: retail loads BOTH operands with
//     lb, so the header's u8 field_0x8D is really signed there: 91.96 -> 93.11.
//   - 0xE2 is `pArgs[0] + (s16)(pArgs[1] << 8)` (an unsigned lbu + sll 24/sra 16, an
//     add), truncated again at the pointer add (sll 16/sra 16 AFTER the PushU24 call) -
//     not the or-based SCRIPT_JUMP_AT: 96.45 -> 97.22.
// WHAT IS LEFT:
//   - 0xBE arm, one register rotation: retail count $a2 / offset $a3 / ~8 mask $a1,
//     ours $a1 / $t0 / $a0. Retail also re-emits both of its delay-slot instructions
//     (li a1,-9, sra v0,a2,6) on the fall-through into .L80024B1C.
//   - tail merges: retail cross-jumps the head's count == 0 tail into 0x81's or/j/sw
//     (.L80024DBC, with the j in its release and sll in the delay slot) where ours emits
//     a private copy, and ours merges 0x8E/0xE1 into the 0xBE arm's `j exit` where retail
//     keeps them separate. Net -5 insns.
// Measured and rejected: `offset = count & 0x1FF` before the frame multiply, storing the
// 0x3C word twice, the 0xBE arm with block locals, and per-arm locals for count/flags/mask.
/*
void func_800248D4(SpriteData* pSpriteData) {
    u8* pArgs;
    u8 code;
    s32 frames;
    s32 count;
    s32 offset;
    s32 stepY;
    u32 flags;
    u32 mask;
    u8* pBuffer;

    if (g_IsBattle) {
        func_800C11CC(pSpriteData);
        return;
    }
dispatch:
    if (pSpriteData->animScriptWaitTimer != 0) {
        return;
    }
    {
        code = *(u8*)pSpriteData->pSpriteBytecode;
        pArgs = (u8*)pSpriteData->pSpriteBytecode + 1;
        if (code < 0x80) {
            u32 headFlags;
            u32 headMask;
            s32 headCount;

            pSpriteData->pSpriteBytecode = pArgs;
            if (code < 0x10) {
                SpriteAnimFrameListSetFrame(pSpriteData, (u16)pSpriteData->curSpriteFrame + 1);
                frames = (code & 0xF) + 1;
            } else if (code < 0x20) {
                pSpriteData->flags_0xA8 =
                    (pSpriteData->flags_0xA8 & ANIM_SCRIPT_FRAME_COUNTER_MASK) |
                    (((ANIM_SCRIPT_COUNTER(pSpriteData->flags_0xA8, ANIM_SCRIPT_FRAME_COUNTER_SHIFT) + 1) &
                      ANIM_SCRIPT_COUNTER_MASK)
                     << ANIM_SCRIPT_FRAME_COUNTER_SHIFT);
                SpriteSetFrameFromAnimIndex(pSpriteData);
                frames = (code & 0xF) + 1;
            } else if (code < 0x30) {
                SpriteAnimFrameListSetFrame(pSpriteData, (u16)pSpriteData->curSpriteFrame - 1);
                frames = (code & 0xF) + 1;
            }
            if (code < 0x40) {
                frames = (code & 0xF) + 1;
            }
            frames = frames * (s32)(((u32)pSpriteData->flags_0xAC.flags >> ANIM_SCRIPT_SPEED_SHIFT) & ANIM_SCRIPT_SPEED_MASK) /
                     ANIM_SCRIPT_SPEED_SCALE;
            if (frames == 0) {
                frames = 1;
            }
            headFlags = pSpriteData->flags_0xA8;
            headMask = headFlags & ANIM_SCRIPT_SCRIPT_INDEX_MASK;
            headCount = (ANIM_SCRIPT_COUNTER(headFlags, ANIM_SCRIPT_SCRIPT_INDEX_SHIFT) + 1) & ANIM_SCRIPT_COUNTER_MASK;
            headMask |= headCount << ANIM_SCRIPT_SCRIPT_INDEX_SHIFT;
            pSpriteData->animScriptWaitTimer += frames;
            pSpriteData->flags_0xA8 = headMask;
            if (headCount == 0) {
                pSpriteData->flags_0xA8 =
                    (headMask & ANIM_SCRIPT_SCRIPT_INDEX_MASK) |
                    (((ANIM_SCRIPT_COUNTER(headMask, ANIM_SCRIPT_SCRIPT_INDEX_SHIFT) - 1) & ANIM_SCRIPT_COUNTER_MASK)
                     << ANIM_SCRIPT_SCRIPT_INDEX_SHIFT);
            }
            return;
        }
        switch (code) {
        case 0xBE: {
            s32 beFrames;

            count = pArgs[0] | ((s8)pArgs[1] << 8);
            beFrames = ((count >> 11) & 0xF) + 1;
            beFrames = beFrames *
                       (s32)(((u32)pSpriteData->flags_0xAC.flags >> ANIM_SCRIPT_SPEED_SHIFT) & ANIM_SCRIPT_SPEED_MASK) /
                       ANIM_SCRIPT_SPEED_SCALE;
            if (beFrames == 0) {
                beFrames = 1;
            }
            offset = count & 0x1FF;
            if (g_SpriteStateFlags(pSpriteData).fields.mode != 1) {
                pSpriteData->curSpriteFrame = offset;
            } else {
                if (count < 0) {
                    if (offset != 0) {
                        offset = *(u8*)(pSpriteData->field_0x60 + offset - 1);
                    }
                }
                flags = pSpriteData->flags_0xAC.flags;
                mask = g_SpriteStateFlags(pSpriteData).flags;
                flags = (flags & ~ANIM_SCRIPT_FLIP_BIT) | ((count >> 6) & ANIM_SCRIPT_FLIP_BIT);
                pSpriteData->flags_0xAC.flags = flags;
                mask = (mask & ~ANIM_SCRIPT_FLIP_BIT) | ((((flags >> 3) & 1) ^ ((flags >> 2) & 1)) << 3);
                if ((count >> 10) & 1) {
                    g_SpriteStateFlags(pSpriteData).flags = mask | 0x10;
                } else {
                    g_SpriteStateFlags(pSpriteData).flags = mask & ~0x10;
                }
                SpriteAnimFrameListSetFrame(pSpriteData, offset);
            }
            pSpriteData->animScriptWaitTimer += beFrames;
            pSpriteData->pSpriteBytecode = (u8*)pSpriteData->pSpriteBytecode + 3;
            return;
        }
        case 0x8E:
            pSpriteData->pSpriteBytecode = NULL;
            return;
        case 0xE2: {
            s32 jumpOffset;

            jumpOffset = pArgs[0] + (s16)(pArgs[1] << 8);
            AnimScriptStackPushU24(pSpriteData, (s32)pSpriteData->pSpriteBytecode + 3);
            pSpriteData->pSpriteBytecode = (u8*)pSpriteData->pSpriteBytecode + (s16)jumpOffset;
            goto dispatch;
        }
        case 0x85:
            pSpriteData->pSpriteBytecode =
                (void*)(((u32)pSpriteData->pSpriteBytecode & 0xFF000000) | AnimScriptStackPopU24(pSpriteData));
            goto dispatch;
        case 0xFA:
            if (*(u8*)AnimScriptResolveVariablePointer(pSpriteData, pArgs) != 0) {
                pSpriteData->pSpriteBytecode = (u8*)pSpriteData->pSpriteBytecode + SCRIPT_JUMP_SHORT_AT(pArgs);
                goto dispatch;
            }
            break;
        case 0xD4:
            pSpriteData->pSpriteBytecode = (u8*)pSpriteData->pSpriteBytecode + SCRIPT_JUMP_AT(pArgs);
            if (pSpriteData->field_0x68 != NULL) {
                ((SpriteScriptCallback)pSpriteData->field_0x68)(pSpriteData);
            }
            goto dispatch;
        case 0x86:
            if (pSpriteData->step.y < 0) {
                pSpriteData->animScriptWaitTimer = 1;
                return;
            }
            break;
        case 0x87:
            if ((s16)(pSpriteData->position.y >> 16) < pSpriteData->field_0x84) {
                pSpriteData->animScriptWaitTimer = 1;
                return;
            }
            break;
        case 0x80:
        animScriptDefaultMode:
            pSpriteData->flags_0xA8 &= ANIM_SCRIPT_MODE_MASK;
            if (pSpriteData->field_0x68 != NULL) {
                ((SpriteScriptCallback)pSpriteData->field_0x68)(pSpriteData);
                return;
            }
            if ((s8)pSpriteData->flags_0xB0.bytes[0] >= 0) {
                SpritePlayAnimation(pSpriteData, (s8)pSpriteData->flags_0xB0.bytes[0]);
            }
            pSpriteData->flags_0xA8 &= ANIM_SCRIPT_MODE_MASK;
            return;
        case 0x98: {
            SpriteData* pLinked;

            pLinked = pSpriteData->field_0x70;
            if (pLinked == NULL) {
                break;
            }
            pSpriteData->animScriptWaitTimer = 1;
            if ((s8)pLinked->flags_0xAC.bytes[3] != (s8)pSpriteData->field_0x8D) {
                break;
            }
            if ((((u32)pLinked->flags_0xA8 >> 28) & 3) == 2) {
                return;
            }
            break;
        }
        case 0x82:
            if (pSpriteData->field_0x68 != NULL) {
                ((SpriteScriptCallback)pSpriteData->field_0x68)(pSpriteData);
            }
            stepY = pSpriteData->step.y;
            SpritePlayAnimation(pSpriteData, (s8)pSpriteData->flags_0xAC.bytes[3]);
            pSpriteData->step.y = stepY;
            pSpriteData->animScriptWaitTimer = 0;
            func_800248D4(pSpriteData);
            return;
        case 0x81:
            if ((s8)pSpriteData->flags_0xAC.bytes[3] == ANIM_SCRIPT_ANIM_ID_DEFAULT) {
                goto animScriptDefaultMode;
            }
            pSpriteData->animScriptWaitTimer = 0;
            if (pSpriteData->field_0x68 != NULL) {
                ((SpriteScriptCallback)pSpriteData->field_0x68)(pSpriteData);
            }
            pSpriteData->flags_0xA8 = (pSpriteData->flags_0xA8 & ANIM_SCRIPT_MODE_MASK) | ANIM_SCRIPT_MODE_SET;
            return;
        case 0xE4:
            count = AnimScriptStackPopU8(pSpriteData);
            if (count != 0) {
                AnimScriptStackPushU8(pSpriteData, count - 1);
                pSpriteData->pSpriteBytecode = (u8*)pSpriteData->pSpriteBytecode + SCRIPT_JUMP_AT(pArgs);
                goto dispatch;
            }
            break;
        case 0xE1:
            pSpriteData->pSpriteBytecode = (u8*)pSpriteData->pSpriteBytecode + SCRIPT_JUMP_AT(pArgs);
            goto dispatch;
        case 0xA7:
            pSpriteData->pSpriteBytecode = (u8*)pSpriteData->pSpriteBytecode + g_AnimScriptCommandLengths[code];
            if (pArgs[0] & 0x80) {
                g_WorkListCurTimer = (pArgs[0] & 0x7F) + 1;
                pSpriteData->animScriptWaitTimer++;
                return;
            }
            count = (pArgs[0] + 2) *
                    (s32)(((u32)pSpriteData->flags_0xAC.flags >> ANIM_SCRIPT_SPEED_SHIFT) & ANIM_SCRIPT_SPEED_MASK) /
                    ANIM_SCRIPT_SPEED_SCALE;
            if (count == 0) {
                count = 1;
            }
            pSpriteData->animScriptWaitTimer += count;
            return;
        case 0xC8:
            func_8001FBE4(pSpriteData, pArgs[0], AnimScriptResolveVariablePointer(pSpriteData, pArgs + 1));
            break;
        default:
            func_8001FBE4(pSpriteData, code, pArgs);
            break;
        }
        pSpriteData->pSpriteBytecode = (u8*)pSpriteData->pSpriteBytecode + g_AnimScriptCommandLengths[code];
    }
}*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_800248D4);




// Work list and image-list setup; g_GfxWorkListCapacity is the work list entry budget.
extern u32 g_GfxWorkListCapacity;
extern u8 g_IsBattle;
extern u8 g_GfxSpritesUseBaseMatrix;
extern void WorkListsReset(void);

void GfxResetWorkLists(void) {
    g_IsBattle = 0;
    g_GfxSpritesUseBaseMatrix = 0;
    g_GfxWorkListCapacity = 0x2000;
    WorkListsReset();
    SpriteAnimFrameListReset();
}



extern void SpriteAnimFrameListReset(void);
extern void SpriteAnimFrameListClear(void);

// Defined here so that the compiler places it in .sbss (%gp_rel)
Image* g_GfxImageList[2];

void GfxAllocateWorkBuffers(int workBufferSize, unsigned int allocFlag) {
    void* pWorkBuffers;

    g_GfxWorkBufferSize = workBufferSize;
    pWorkBuffers = HeapAlloc(workBufferSize * NUM_RENDER_CONTEXTS, allocFlag);
    g_GfxWorkBuffers = pWorkBuffers;
    g_GfxWorkBuffer2 = pWorkBuffers + workBufferSize;
    D_80059304 = 0;
    g_GfxSpriteTileListHeads = 0;
    g_GfxImageList[0] = NULL;
    SpriteAnimFrameListReset();
}

void GfxFreeWorkBuffers(void) {
    HeapFree(g_GfxWorkBuffers);
    SpriteAnimFrameListClear();
}

void GfxSetCurrentOT(u_long* ot) {
    g_GfxCurOT = ot;
}








// Maybe start of a TU
// ===========================================================


// Set g_GfxBaseMatrix matrix
void GfxSetBaseMatrix(MATRIX* matrix) {
    g_GfxBaseMatrix = *matrix;
}




// Maybe start of a TU
// ===========================================================

// Loops over the linked list given by g_GfxImageList[g_GfxCurContext].
// LoadImage on entries with an address, ClearImage otherwise.
// Set the list to NULL afterwards.
// GfxShapeTransfer
void GfxFlushImageList(void) {
    Image* pListHead;
    Image* pImage;

    pListHead = g_GfxImageList[g_GfxCurContext];
    for (pImage = pListHead; pImage != NULL; pImage = pImage->pNext) {
        if (pImage->addr) {
            LoadImage(&pImage->rect, pImage->addr);
        } else {
            ClearImage(&pImage->rect, 0x0, 0x0, 0x0);
        }
    }

    g_GfxImageList[g_GfxCurContext] = NULL;
}

typedef struct {
    void* pData; // Sprite Tile data stuff
    struct LinkedListEntry* pNext;
} LinkedListEntry;

// include/system/animation.h already types g_GfxSpriteTileListHeads as a scalar; the real
// object is the two per-context list heads, so address it through a cast.
#define g_GfxSpriteTileLists ((LinkedListEntry**)&g_GfxSpriteTileListHeads)

// g_GfxWorkBuffers and the g_GfxWorkBuffer2 that follows it are the two
// per-context buffer bases, reached as one array through the symbol's address:
// indexing the symbol is what makes maspsx emit %hi/%lo rather than %gp_rel.
// animation.h names them singly (GfxAllocateWorkBuffers sets each), so the
// pair is read here through a view.
typedef struct {
    /* 0x0 */ void* pBuffers[NUM_RENDER_CONTEXTS];
} GfxWorkBufferBases;

#define g_GfxWorkBufferBases ((GfxWorkBufferBases*)&g_GfxWorkBuffers)

// g_GfxCurWorkBufferBase is a tentative definition so maspsx emits %gp_rel for it.
s32 g_GfxCurWorkBufferBase;

void GfxSetRenderContext(int context) {
    // The tile-list heads are the two words at g_GfxSpriteTileListHeads. The pointer local
    // with the embedded assignment is what keeps retail's single context*4
    // before the symbol address: a plain index or a typed head array moves the
    // sll after the lui and stalls at 95.00%. The s32* element type is part of
    // that (a LinkedListEntry** local gives the same 95.00%).
    s32* pLists;
    LinkedListEntry* pCurEntry;
    void* pPrimBuffer;

    pCurEntry = ((LinkedListEntry**)(pLists = &g_GfxSpriteTileListHeads))[context];
    pPrimBuffer = g_GfxWorkBufferBases->pBuffers[context];
    g_GfxCurContext = context;
    g_GfxCurWorkBuffer = pPrimBuffer;
    g_GfxCurWorkBufferBase = pPrimBuffer;
    g_GfxCurWorkBufferEnd = pPrimBuffer + g_GfxWorkBufferSize;
    while (pCurEntry != NULL) {
        HeapFree(pCurEntry->pData);
        pCurEntry = pCurEntry->pNext;
    }
    g_GfxSpriteTileLists[context] = NULL;
}
// GfxQueueSpriteTileData
void GfxQueueSpriteTileData(void* pData) {
    LinkedListEntry* pNewEntry;

    pNewEntry = g_GfxCurWorkBuffer;
    g_GfxCurWorkBuffer = pNewEntry + 1;
    if (pNewEntry) {
        pNewEntry->pData = pData;
        pNewEntry->pNext = g_GfxSpriteTileLists[g_GfxCurContext];
        g_GfxSpriteTileLists[g_GfxCurContext] = pNewEntry;
    }
}

// GfxQueueShapeTransfer
// Add Image to current g_GfxImageList[g_GfxCurContext] linked list
//
// height is declared s16 so that cdk's gcc loads the incoming stack word
// (lw v1,0x10(sp)) before the sltu instead of inside the branch.
void GfxQueueImageTransfer(u_long* addr, int x, int y, int width, s16 height) {
    Image* pCurrent;
    Image* pNext;

    pCurrent = g_GfxCurWorkBuffer;
    pNext = pCurrent + 1;
    if (pNext < (Image*) g_GfxCurWorkBufferEnd) {
        pCurrent->rect.h = height;
        pCurrent->rect.x = x;
        pCurrent->rect.y = y;
        pCurrent->rect.w = width;
        pCurrent->addr = addr;
        g_GfxCurWorkBuffer = pNext;
        pCurrent->pNext = g_GfxImageList[g_GfxCurContext];
        g_GfxImageList[g_GfxCurContext] = pCurrent;
    }
}

// Handler table of the sprite work task, indexed by its handler id:
// SpriteDrawProjected, SpriteTaskNoOp, SpriteDrawModelParts, NULL, NULL, SpriteDrawProjected,
// SpriteDrawProjected, SpriteDrawModelParts, SpriteDrawTile, func_80025544, NULL, NULL,
// NULL, NULL, SpriteDrawProjected, SpriteDrawLitModelParts.
extern WorkListCallback_t g_SpriteTaskHandlers[];

// Sets pStruct->unk8 callback
void SpriteSetTaskHandler(WorkListEntry* pTask, int handlerIndex) {
    WorkListSetTaskCallback(pTask, g_SpriteTaskHandlers[handlerIndex]);
}

extern s32 g_GfxOTDepthShift;
extern u8 D_800C3664;
extern void func_8001E3D8(SpriteData* pSpriteData, void* pArg);
extern void SpriteDrawFrame(SpriteData* pSpriteData, void* pArg);

// SpriteData as this unit reads it: the per-context frames-data pair at 0x2C
// (two scalars in the shared header, because other units assign them singly),
// curSpriteFrame as an unsigned halfword, the colour word at 0x28 (copied
// whole into a primitive), the 0x3C flag word (bits 25-27 are read as a whole
// word here, where the header's bitfields over them narrow to lbu 0x3F) and the
// upper halfword of field_0x40, which is read with lhu.
typedef struct {
    /* 0x00 */ u8 unk0[0x28];
    /* 0x28 */ u32 color; // red, green, blue and the primitive code as one word
    /* 0x2C */ void* pFramesData[NUM_RENDER_CONTEXTS];
    /* 0x34 */ u16 curSpriteFrame; // unsigned here: -1 is a valid frame
    /* 0x36 */ u16 xOffset; // horizontal offset added to the projected x
    /* 0x38 */ u8 unk38[0x4];
    /* 0x3C */ SpriteFlagWord flags_0x3C;
    /* 0x40 */ u16 flags0x40Low;
    /* 0x42 */ u16 flags0x40High;
} SpriteDataView;

#define g_SpriteFramesData(pSpriteData) \
    (((SpriteDataView*)(pSpriteData))->pFramesData)
#define g_SpriteFlags0x3C(pSpriteData) \
    (((SpriteDataView*)(pSpriteData))->flags_0x3C)
#define g_SpriteFlags0x40High(pSpriteData) \
    (((SpriteDataView*)(pSpriteData))->flags0x40High)

// Projects the sprite's position and queues it into the OT at its own depth
// offset, either straight (bit 24) or through the model transform (bit 29).
void SpriteDrawProjected(WorkListEntry* pTask) {
    SVECTOR vec;
    long vecTrans[3];
    long sxy;
    long flag;
    SpriteData* pSpriteData;
    MATRIX* pMatrix;
    s32 nOTOffset;

    pSpriteData = pTask->unk4;
    if ((((u32)pSpriteData->flags_0xB0.flags >> 8) & 1) && D_800C3664) {
        return;
    }

    vec.vx = pSpriteData->position.x >> 16;
    vec.vy = pSpriteData->position.y >> 16;
    vec.vz = pSpriteData->position.z >> 16;
    pMatrix = &g_GfxBaseMatrix;
    SetRotMatrix(pMatrix);
    SetTransMatrix(pMatrix);
    nOTOffset = (RotTransPers(&vec, &sxy, (u_long*)&sxy, &flag) >> g_GfxOTDepthShift) +
               pSpriteData->field_0x30;
    if (flag & 0x8000) {
        nOTOffset = 0;
    }
    pSpriteData->field_0x2e = nOTOffset;

    if (pSpriteData->field_0x3C_2 & 1) {
        SpriteUpdateTransformMatrix(pSpriteData);
        vecTrans[0] = pSpriteData->position.x >> 16;
        vecTrans[1] = pSpriteData->position.y >> 16;
        vecTrans[2] = pSpriteData->position.z >> 16;
        TransMatrix(&pSpriteData->pBase->transformMatrix, (VECTOR*)vecTrans);
        SetRotMatrix(&pSpriteData->pBase->transformMatrix);
        SetTransMatrix(&pSpriteData->pBase->transformMatrix);
        // Re-read rather than keep a copy: the GTE calls above clobber the
        // register the word was in.
        if (((u32)g_SpriteFlags0x3C(pSpriteData).flags >> 25) & 1) {
            nOTOffset = 0xFFF;
        } else {
            nOTOffset = pSpriteData->field_0x30;
        }
        if ((u32)(nOTOffset - 1) < 0xFFF) {
            func_8001E3D8(pSpriteData, &g_GfxCurOT[nOTOffset]);
        }
    } else {
        if (pSpriteData->field_0x3C_5 & 1) {
            nOTOffset = ((SpriteData*)pSpriteData->field_0x70)->field_0x2e;
        }
        if ((u32)(nOTOffset - 1) < 0xFFF) {
            SpriteDrawFrame(pSpriteData, &g_GfxCurOT[nOTOffset]);
        }
    }
}

void SpriteDrawTile(WorkListEntry* pTask) {
    SVECTOR vec;
    int nFlag;
    int nOTOffset;
    DR_TPAGE* pPrimTPage;
    TILE_1* pPrim;
    TILE_1* pNewBufferHead;
    DR_TPAGE* pNewBufferHead_2;
    SpriteData* pSpriteData;
    MATRIX* pMatrix;

    pSpriteData = pTask->unk4;
    if (((SpriteDataView*)pSpriteData)->curSpriteFrame == 0) {
        pPrim = g_GfxCurWorkBuffer;
        pNewBufferHead = pPrim + 1;
        if (pNewBufferHead < (TILE_1*)g_GfxCurWorkBufferEnd) {
            pMatrix = &g_GfxBaseMatrix;
            vec.vx = pSpriteData->position.x >> 16;
            vec.vy = pSpriteData->position.y >> 16;
            vec.vz = pSpriteData->position.z >> 16;
            g_GfxCurWorkBuffer = pNewBufferHead;
            SetRotMatrix(pMatrix);
            SetTransMatrix(pMatrix);
            nOTOffset = RotTransPers(&vec, &pPrim->x0, &nFlag, &nFlag) >> g_GfxOTDepthShift;
            pSpriteData->field_0x2e = nOTOffset;

            // SetTile1
            setlen(pPrim, 2);
            *((u32*)&pPrim->r0) = ((SpriteDataView*)pSpriteData)->color;

            AddPrim(&g_GfxCurOT[nOTOffset], pPrim);

            pPrimTPage = g_GfxCurWorkBuffer;
            pNewBufferHead_2 = pPrimTPage + 1;
            if (pNewBufferHead_2 < (DR_TPAGE*)g_GfxCurWorkBufferEnd) {
                g_GfxCurWorkBuffer = pNewBufferHead_2;

                // Only the tpage bits of flags_0x3C make it into the primitive.
                setDrawTPage(pPrimTPage, 0, 0, pSpriteData->field_0x3C_0 & 0x60);
                AddPrim(&g_GfxCurOT[nOTOffset], pPrimTPage);
            }
        }
    }
}

/*
Un-park screen: cdk 89.57%, gcc-2.6.0 88.37%, gcc-2.7.2 88.24% - the park
stays, and cdk is still the closest of the three.
Best 89.57% (464 bytes against 460). cdk is the right compiler here: the same C
scores 87.24% under gcc-2.7.2-psx, so this one is source-level. Two gaps: gcc
sinks the `lhu` of pPrim->x0 - and the `lhu` of xOffset, which the original
issues before the g_GfxCurWorkBuffer load - so the original fills a load-frames
slot that gcc leaves as a nop, and xOffset/prim end up in s3/s1 instead of
s1/s3. Ruled out (all produced byte-identical output, so neither statement
order nor copies are the lever): assigning prim before xOffset, a pNext local
with a nested if, a second TILE pointer for the tail, aliasing the tail through
a u16 pointer, and folding nHalf into the field updates. gcc coalesces every
copy; the s1/s3 swap and the missing move-a1-tail copy must come from elsewhere
in the source (prim is the higher-priority pseudo here, lower there). Both x0 reads (signed for the width, unsigned for the centre) have to
stay in the source.
Also tried and byte-identical: a u32 pNewBufferHead local with the neighbour's
`pNewBufferHead < g_GfxCurWorkBufferEnd` idiom, and reading x0 (u16) before
nWidth. The reorder is gcc moving the xOffset lhu two insns down, after the two
gp loads; that reorder is what puts pPrim in s1 instead of s3.
*/
// Best 89.57% (464 bytes against retail's 460, 116 insns against 115). Two residuals, both allocation order:
// - the callee-saved pair is swapped: retail s1 = xOffset, s3 = pPrim; ours s1 = pPrim, s3 = xOffset, so gcc loads the gp globals before the lhu of xOffset (retail loads xOffset first) and makes its copy of pPrim to a1 late (retail's 'addu a1,s3,zero' sits right after the negu).
// - the x0 reads: retail emits lh v0,8(s3) then lhu a0,8(s3) then subu, so the load-delay slot is filled; ours emits lh, nop, subu, lhu - the extra nop is the whole 1-instruction/4-byte gap.
// Tried this round, all byte-identical at 89.57: x0 read before vs after nWidth; xOffset declared first in the list; 'pPrim->y0 - nHalf' without the u16* cast; x0 = pPrim->x0 (implicit s16->u16) instead of *(u16*)&pPrim->x0. Typed the read as SpriteDataView->xOffset (new field at 0x36, unk38[0x4] after it) with no byte change, so the body is realistic now.
/*
void func_80025544(WorkListEntry* pTask) {
    SpriteData* pSpriteData;
    MATRIX* pMatrix;
    TILE* pPrim;
    DR_TPAGE* pPrimTPage;
    SVECTOR vec;
    SVECTOR vecOffset;
    SVECTOR sxy1;
    long sxy2;
    long flag;
    u16 xOffset;
    u16 x0;
    s32 nWidth;
    s32 nHalf;
    s32 nOTOffset;
    u32 pNewBufferHead;

    pSpriteData = pTask->unk4;
    if (((SpriteDataView*)pSpriteData)->curSpriteFrame != 0) {
        return;
    }
    xOffset = ((SpriteDataView*)pSpriteData)->xOffset;
    pPrim = g_GfxCurWorkBuffer;
    pNewBufferHead = pPrim + 1;
    if (!(pNewBufferHead < g_GfxCurWorkBufferEnd)) {
        return;
    }

    vec.vx = pSpriteData->position.x >> 16;
    vec.vy = pSpriteData->position.y >> 16;
    vec.vz = pSpriteData->position.z >> 16;
    pMatrix = &g_GfxBaseMatrix;
    g_GfxCurWorkBuffer = pPrim + 1;
    SetRotMatrix(pMatrix);
    SetTransMatrix(pMatrix);

    vecOffset = vec;
    *(u16*)&vecOffset.vx += xOffset;
    nOTOffset = RotTransPers3(&vec, &vecOffset, &vec, (long*)&pPrim->x0,
                              (long*)&sxy1, &sxy2, &flag, &flag) >> g_GfxOTDepthShift;
    pSpriteData->field_0x2e = nOTOffset;

    x0 = pPrim->x0;
    nWidth = sxy1.vx - pPrim->x0;
    if (nWidth == 0) {
        nWidth = 1;
    }
    if (nWidth < 0) {
        nWidth = -nWidth;
    }
    nHalf = nWidth / 2;
    pPrim->x0 = x0 - nHalf;
    pPrim->y0 = pPrim->y0 - nHalf;

    setlen(pPrim, 3);
    *((u32*)&pPrim->r0) = *((u32*)&pSpriteData->red);
    pPrim->h = nWidth;
    pPrim->w = nWidth;
    AddPrim(&g_GfxCurOT[nOTOffset], pPrim);

    pPrimTPage = g_GfxCurWorkBuffer;
    if (!((u8*)(pPrimTPage + 1) < (u8*)g_GfxCurWorkBufferEnd)) {
        return;
    }
    g_GfxCurWorkBuffer = pPrimTPage + 1;
    setDrawTPage(pPrimTPage, 0, 0, pSpriteData->field_0x3C_0 & 0x60);
    AddPrim(&g_GfxCurOT[nOTOffset], pPrimTPage);
}*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80025544);



void SpriteTaskNoOp(void) {}

extern void SpriteUpdateTransformMatrix(SpriteData* pSpriteData);
extern void ModelRenderPacket(void* pDirTransforms, void* pFramesData, u_long* pOT, int flags);

// Draws the sprite through its transform matrix, then its model parts.
void SpriteDrawModelParts(WorkListEntry* pTask) {
    MATRIX matrix;
    VECTOR vec;
    SpriteData* pSpriteData;

    pSpriteData = pTask->unk4;
    SpriteUpdateTransformMatrix(pSpriteData);
    if (pSpriteData->pBase->pDirTransforms) {
        vec.vx = pSpriteData->position.x >> 16;
        vec.vy = pSpriteData->position.y >> 16;
        vec.vz = pSpriteData->position.z >> 16;
        TransMatrix(&pSpriteData->pBase->transformMatrix, &vec);
        if (!(pSpriteData->field_0x3C_2 & 1)) {
            CompMatrix(&g_GfxBaseMatrix, &pSpriteData->pBase->transformMatrix, &matrix);
        }
        SetRotMatrix(&matrix);
        SetTransMatrix(&matrix);
        ModelRenderPacket(pSpriteData->pBase->pDirTransforms,
                          g_SpriteFramesData(pSpriteData->pBase)[g_GfxCurContext],
                          g_GfxCurOT, g_SpriteFlags0x40High(pSpriteData) & 4);
    }
}

extern void func_800B1F6C(void* pDirTransforms, void* pFramesData, u_long* pOT,
                          int flags, s32 arg4, s32 arg5);
extern MATRIX g_GfxColorMatrix;
extern MATRIX g_GfxLightSourceMatrix;

// The standalone base object pBase points at carries a second rotation/scale
// pair past the 0x40 SpriteDataI1 covers; SVECTOR's 8-byte size puts the scale
// exactly where g_GfxColorMatrix keeps its 0xC00 identity diagonal. The cast is
// written at each read on purpose: one pointer local across the g_GfxColorMatrix
// stores turns retail's four pBase reloads into one and costs the match (93%).
typedef struct {
    /* 0x00 */ SpriteDataI1 base;
    /* 0x40 */ u32 unk40;
    /* 0x44 */ SVECTOR rotation;
    /* 0x4C */ SVECTOR scale;
} SpriteBaseExt; // Size: 0x54

// Builds the sprite's color and light matrices from its base scale/rotation,
// then draws its model parts; a negative flag word saves and restores the
// geometry offset around the draw.
void SpriteDrawLitModelParts(WorkListEntry* pTask) {
    MATRIX matrix;
    VECTOR vec;
    MATRIX colorMatrix;
    MATRIX lightMatrix;
    long geomOffsetX;
    long geomOffsetY;
    SpriteData* pSpriteData;
    s32 oldShift;

    pSpriteData = pTask->unk4;
    SpriteUpdateTransformMatrix(pSpriteData);
    if (pSpriteData->pBase->pDirTransforms) {
        if ((pSpriteData->field_0x40 >> 1) & 1) {
            PushMatrix();
            // The scale values go in the m[i][0] slots (bytes 0, 6, 0xC), which
            // is where the static initialiser keeps its 0xC00 identity values.
            g_GfxColorMatrix.m[0][0] = ((SpriteBaseExt*)pSpriteData->pBase)->scale.vx;
            g_GfxColorMatrix.m[1][0] = ((SpriteBaseExt*)pSpriteData->pBase)->scale.vy;
            g_GfxColorMatrix.m[2][0] = ((SpriteBaseExt*)pSpriteData->pBase)->scale.vz;
            RotMatrix(&((SpriteBaseExt*)pSpriteData->pBase)->rotation,
                      &colorMatrix);
            MulMatrix0(&colorMatrix, &pSpriteData->pBase->transformMatrix,
                       &colorMatrix);
            MulMatrix0(&g_GfxLightSourceMatrix, &colorMatrix, &lightMatrix);
            SetBackColor(0x20, 0x20, 0x20);
            SetColorMatrix(&g_GfxColorMatrix);
            SetLightMatrix(&lightMatrix);
            PopMatrix();
        }

        vec.vx = pSpriteData->position.x >> 16;
        vec.vy = pSpriteData->position.y >> 16;
        vec.vz = pSpriteData->position.z >> 16;
        TransMatrix(&pSpriteData->pBase->transformMatrix, &vec);
        if (!(pSpriteData->field_0x3C_2 & 1)) {
            CompMatrix(&g_GfxBaseMatrix, &pSpriteData->pBase->transformMatrix,
                       &matrix);
            SetRotMatrix(&matrix);
            SetTransMatrix(&matrix);
        } else {
            SetRotMatrix(&pSpriteData->pBase->transformMatrix);
            SetTransMatrix(&pSpriteData->pBase->transformMatrix);
        }

        if (pSpriteData->field_0x3C_4 & 1) {
            ReadGeomOffset(&geomOffsetX, &geomOffsetY);
            SetGeomOffset(0xA0, 0x70);
        }
        if (((u32)g_SpriteFlags0x3C(pSpriteData).flags >> 25) & 1) {
            oldShift = g_GfxOTDepthShift;
            g_GfxOTDepthShift = 0x10;
            func_800B1F6C(pSpriteData->pBase->pDirTransforms,
                          g_SpriteFramesData(pSpriteData->pBase)[g_GfxCurContext],
                          g_GfxCurOT, 0, 0xFEC,
                          g_SpriteFlags0x3C(pSpriteData).bytes[0] >> 5);
            g_GfxOTDepthShift = oldShift;
        } else {
            func_800B1F6C(pSpriteData->pBase->pDirTransforms,
                          g_SpriteFramesData(pSpriteData->pBase)[g_GfxCurContext],
                          g_GfxCurOT, 0, pSpriteData->field_0x30,
                          g_SpriteFlags0x3C(pSpriteData).bytes[0] >> 5);
        }
        if (pSpriteData->field_0x3C_4 & 1) {
            SetGeomOffset(geomOffsetX, geomOffsetY);
        }
    }
}

// Draws the sprite through the matrix g_GfxBaseMatrix composed with its own, then
// hands the model parts to func_800B1F6C.
void SpriteDrawModelPartsInBaseMatrix(WorkListEntry* pTask) {
    MATRIX matrix;
    VECTOR vecResult;
    SVECTOR vec;
    SpriteData* pSpriteData;
    s32 oldShift;

    pSpriteData = pTask->unk4;
    SpriteUpdateTransformMatrix(pSpriteData);
    if (pSpriteData->pBase->pDirTransforms) {
        vec.vx = pSpriteData->position.x >> 16;
        vec.vy = pSpriteData->position.y >> 16;
        vec.vz = pSpriteData->position.z >> 16;
        ApplyMatrix(&g_GfxBaseMatrix, &vec, &vecResult);
        pSpriteData->pBase->transformMatrix.t[0] = g_GfxBaseMatrix.t[0] + vecResult.vx;
        pSpriteData->pBase->transformMatrix.t[1] = g_GfxBaseMatrix.t[1] + vecResult.vy;
        pSpriteData->pBase->transformMatrix.t[2] = g_GfxBaseMatrix.t[2] + vecResult.vz;
        SetRotMatrix(&pSpriteData->pBase->transformMatrix);
        SetTransMatrix(&pSpriteData->pBase->transformMatrix);
        if (((u32)g_SpriteFlags0x3C(pSpriteData).flags >> 25) & 1) {
            oldShift = g_GfxOTDepthShift;
            g_GfxOTDepthShift = 0x10;
            func_800B1F6C(pSpriteData->pBase->pDirTransforms,
                          g_SpriteFramesData(pSpriteData->pBase)[g_GfxCurContext],
                          g_GfxCurOT, 0, 0xFEC,
                          g_SpriteFlags0x3C(pSpriteData).bytes[0] >> 5);
            g_GfxOTDepthShift = oldShift;
        } else {
            func_800B1F6C(pSpriteData->pBase->pDirTransforms,
                          g_SpriteFramesData(pSpriteData->pBase)[g_GfxCurContext],
                          g_GfxCurOT, 0, pSpriteData->field_0x30,
                          g_SpriteFlags0x3C(pSpriteData).bytes[0] >> 5);
        }
    }
}

// splat marks 80025C04, 80025D4C, 80026F44 and 80026FE8 "Handwritten
// function", but only two of them really are: 80026F44 and 80026FE8 contain
// trapping-immediate ALU ops that no gcc emits (80026F44 0x20050020 addi,
// 80026FE8 three addi and three add). 80025C04 and 80025D4C are ordinary GTE
// code and their mtc2/lwc2/swc2 all have macros in psyq/inline_c.h.
// 80025C04 best attempt below: 87.30% at the original's exact 316 bytes.
// Remaining, all allocator/scheduling, no structure left:
//  - retail's scratchpad base is $t0 and holds it across the loop; ours parks
//    the 0x1F800000 lui in $t1 and copies it to $t0 (one extra move).
//  - the accumulator at 0x1F800000: retail stores the clamped red with sh and
//    then RE-READS it in both arms of the green/blue ifs; gcc 2.7.2 keeps the
//    stored value in a register (v1) and defers the sh past the loads. Making
//    the u16 view volatile is the untried lever.
// NOT reachable, whatever the C: splat renders the two GTE buffer addresses as
// addiu $v0,$t0,%lo(D_1F800004) / %lo(D_1F800014), so the expected object
// carries an R_MIPS_LO16 reloc there with a literal-zero immediate, while any
// C spelling that keeps the base a runtime register emits addiu $v0,$t0,4 with
// no reloc. Same linked bytes (retail's lui has no %hi reloc at all, splat
// printed it as a raw (0x1F800000 >> 16)), but objdiff can never score them
// equal. Naming the symbols instead (extern s32 D_1F800004; gte_ldlvl(&D_...))
// does emit the %lo form, but then gcc gives the hi its own %hi reloc and a
// separate register, and the score falls to 80.05% with 320 bytes.
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80025C04);
/*
// Interpolates 16-bit colours of pSrc into pDst through GPF: the source colour
// is staged in the scratchpad as IR1-IR3, GPF blends it against the running
// colour at 0x1F800000 and the result is clamped and packed back.
void func_80025C04(s32 count, s32 intensity, u16 *pDst, u16 *pSrc) {
    s32 *pScratch = (s32 *)PSX_SCRATCH;
    u16 *pAcc = (u16 *)PSX_SCRATCH;
    u16 value;

    gte_lddp(intensity << 7);
    while (--count != -1) {
        pScratch[1] = *pSrc & 0x1F;
        pScratch[2] = *pSrc & 0x3E0;
        pScratch[3] = *pSrc & 0x7C00;
        gte_ldlvl(pScratch + 1);
        gte_gpf12();
        gte_stlvl(pScratch + 5);
        pAcc[0] = (pScratch[5] < 0x20) ? (pAcc[10] & 0x1F) : 0x1F;
        if (pScratch[6] >= 0x3E1) {
            pAcc[0] |= 0x3E0;
        } else {
            pAcc[0] |= pAcc[12] & 0x3E0;
        }
        if (pScratch[7] >= 0x7C01) {
            pAcc[0] |= 0x7C00;
        } else {
            pAcc[0] |= pAcc[14] & 0x7C00;
        }
        value = pAcc[0] | (*pSrc++ & 0x8000);
        pAcc[0] = value;
        *pDst++ = value;
    }
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80025D4C);
/* Best parked version 87.89% (604 bytes, retail's exact size).
Re-parked after a re-measure under this unit's preset (XenoGraphicsDiv, gcc
2.7.2-cdk): the parked body below still scores 87.89% at retail's exact 604
bytes, identical to the gcc-2.6.0 number, so the preset is not the difference.
The permuter cannot be pointed at this stub: its base source is the unit's
preprocessed file and the GTE macros are inline asm, which the permuter's C
parser rejects ('Syntax error in base.c ... mtc2').

Un-park screen: best 87.89% (604 bytes, the original's exact size). This stub
is NOT one of the unreachable "handwritten function" ones: gte_ldlvl (lwc2
$9/$10/$11 off the long[3] scratch) and gte_stlvl_gpr (swc2 back into the same
two words) from include/psyq/inline_c.h reproduce the GTE traffic exactly, and
gte_gpf12 supplies the two nops plus GPF, so the instruction multiset is right.
What is left is one allocator permutation plus the frame. Retail keeps redBias
in s0, the shifted green/blue biases in t9/t8, mode in t6, and does the
mode-3 mfhi into s1 (frame 0x18, two saved registers); this body needs a third
saved register (s0 = greenBias, s1 = redBias, s2 = mfhi), hence a 0x20 frame,
ships greenIn/blueIn in t6/t5 where retail has t3/t2, and puts the clamped red
in t3 where retail has a0. Tried and no move: shifts inline in the body vs
mutating the bias parameters, *pSrc++ vs a tail pSrc++, folding the average
local into redIn, hoisted redValue/greenValue/blueValue locals (81.21%),
a pDelta u16* local instead of the (u16*)d casts (81.65%, 624 bytes).
Signature from the only caller, func_800A6AE8 in battle/main/misc23:
(count = w * h, pSrc, pSrc2, pDst, redBias, greenBias, blueBias, mode, depth),
src/src2/dst the same buffer there, mode 0..3, depth from the same record.
Return: the blended colour is written only when the source colour is non-zero,
and always with bit 15 set.
// Shades count 16-bit colours of pSrc into pDst: the source colour is divided
// down (mode 0 keeps it, 1 halves it, 2 quarters it, 3 replaces it with the
// average of R, G and B) and biased per channel, then the GPF interpolates the
// difference against pSrc2's colour by depth and the result is added back.

Frame is the last structural gap, found this round: retail is 0x18 with TWO
saved registers (s0 = redBias, s1 = the mode-3 average) and keeps greenBias<<5
in t9 / blueBias<<10 in t8; this body needs THREE (s0 = greenBias, s1 =
redBias, s2 = average), hence 0x20. Swapping the two bias shift statements
(blueBias<<=10 first) changes nothing - 87.89 again - so retail's
`j` frames slot holding the green shift while ours holds the blue one is
allocator freedom, not statement order. Retail also builds the output word as
(red-part | 0x8000) then green then blue with an `ori 0x8000`, where gcc 2.6.0
hoists `li v1,-32768` into the beqz frames slot and ORs it at the end; that is
a reassociation tie, not a source shape.
void func_80025D4C(s32 count, u16* pSrc, u16* pSrc2, u16* pDst,
                       s32 redBias, s32 greenBias, s32 blueBias, s32 mode,
                       s32 depth) {
    long d[3];
    s16 red;
    s16 green;
    s16 blue;
    s32 redIn;
    s32 greenIn;
    s32 blueIn;
    s32 average;
    u16 color;

    if (depth >= 0x21) {
        depth = 0x20;
    }
    gte_lddp(depth << 7);
    greenBias <<= 5;
    blueBias <<= 10;
    while (--count != -1) {
        color = *pSrc++;
        switch (mode) {
        case 0:
            redIn = color & 0x1F;
            greenIn = color & 0x3E0;
            blueIn = color & 0x7C00;
            break;
        case 1:
            redIn = (color & 0x1E) >> 1;
            greenIn = (color & 0x3C0) >> 1;
            blueIn = (color & 0x7800) >> 1;
            break;
        case 2:
            redIn = (color & 0x1C) >> 2;
            greenIn = (color & 0x380) >> 2;
            blueIn = (color & 0x7000) >> 2;
            break;
        case 3:
            average = ((color & 0x1F) + ((color & 0x3E0) >> 5) +
                       ((color & 0x7C00) >> 10)) / 3;
            redIn = average;
            greenIn = average << 5;
            blueIn = average << 10;
            break;
        }
        red = redIn + redBias;
        if (red < 0) {
            red = 0;
        }
        green = greenIn + greenBias;
        if (green < 0) {
            green = 0;
        }
        blue = blueIn + blueBias;
        if (blue < 0) {
            blue = 0;
        }
        if (red >= 0x20) {
            red = 0x1F;
        }
        if (green >= 0x3E1) {
            green = 0x3E0;
        }
        if (blue >= 0x7C01) {
            blue = 0x7C00;
        }
        d[0] = (*pSrc2 & 0x1F) - red;
        d[1] = (*pSrc2 & 0x3E0) - green;
        d[2] = (*pSrc2 & 0x7C00) - blue;
        gte_ldlvl(d);
        gte_gpf12();
        gte_stlvl_gpr(d);
        if (color != 0) {
            *pDst = 0x8000 | (red + (((u16*)d)[0] & 0x1F)) |
                    (green + (((u16*)d)[2] & 0x3E0)) |
                    (blue + (((u16*)d)[4] & 0x7C00));
        }
        pSrc2++;
        pDst++;
    }
}
*/

// The identity scale MATRIX the rotated quad helper starts from and the four
// corner SVECTORs it projects (their z stays at the 0x1000 (1.0) the table is
// initialised with).
extern MATRIX g_GfxIdentityMatrix;
extern SVECTOR g_GfxQuadCorners[];

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_80025FA8);

// Parked at 97.85% (912 bytes = the original's exact size) under this unit's
// preset (XenoGraphicsDiv, gcc 2.7.2-cdk). Everything but the allocator matches:
// the frame (0xA0), every stack slot (geom offset 0x58/0x5C, p/flag 0x60/0x64,
// angle 0x68, old geom screen 0x70), both clamp chains and the uv stores are
// instruction for instruction. What is left:
//  - the saved registers are rotated: retail has the data offset in s5, the loop
//    index in s6, pDirent in s7 and g_GfxQuadCorners's %hi in s4; this body has them
//    one slot lower (s4/s5/s6) with %hi in s7. A cyclic rotation, i.e. an
//    allocator tie, not a source shape.
//  - retail's `i = 0` lands BEFORE the loop's entry branch and its angle store
//    fills that branch's frames slot; here the store fills the count load's frames
//    slot and `i = 0` fills the branch slot.
// Measured and no move: `i = 0` as an explicit statement before the for, the
// for-init moved in/out of the init clause, the angle store before/inside/after
// the if-count guard wrapper, nCount as a separate local for the guard, 2*i vs
// i*2, and a 3-minute permuter run (best 324 vs base 490, no zero). The
// u16 localAngle[2] array and the geom/p/flag pairs declared as long[2] arrays
// are LOAD-BEARING: an 8-byte slot for each is what reproduces retail's frame
// (a plain u16 local is hoisted into a register by loop.c and the function drops
// to 93.45%, and scalars instead of arrays give a 0x98 frame). The `if (count !=
// 0)` wrapper must NOT be there: without it gcc inverts the loop and the entry
// test/init order changes.
/*
// Draws each record of the dirent as a rotated, scaled textured quad: builds a
// rotation/scale matrix, projects the record rectangle's corners through it with
// the geometry offset swapped for (x, y) for the duration, then writes the uv
// rectangle. A quad that ends up mirrored loses its first texel column (and row)
// so the neighbouring texel is not sampled, and a quarter turn loses one column
// unconditionally.
s32 func_80025FA8(MenuResourceDirectory* dir, u32 index, POLY_FT4* pPrims,
                  s32 context, s16 x, s16 y, s16 scaleX, s16 scaleY,
                  s16 angle) {
    MATRIX matrix;
    long scale[3];
    long oldGeomOffset[2];
    long pFlag[2];
    long oldGeomScreen;
    u16 localAngle[2];
    MenuResourceDirentTextureMetadata* pDirent;
    MenuResourceTextureMetadata* pData;
    POLY_FT4* pPrim;
    s32 nAngle;
    s32 nLeft;
    s32 nTop;
    s32 nRight;
    s32 nBottom;
    u16 u;
    u16 v;
    s32 width;
    s32 height;
    s32 i;

    matrix = g_GfxIdentityMatrix;
    scale[0] = scaleX;
    scale[1] = scaleY;
    scale[2] = 0x1000;
    PushMatrix();
    ScaleMatrixL(&matrix, (VECTOR*)scale);
    RotMatrixZ(angle, &matrix);
    ReadGeomOffset(&oldGeomOffset[0], &oldGeomOffset[1]);
    oldGeomScreen = ReadGeomScreen();
    SetGeomOffset(x, y);
    SetGeomScreen(0x1000);
    SetRotMatrix(&matrix);
    SetTransMatrix(&matrix);

    pDirent = (MenuResourceDirentTextureMetadata*)(dir->offsets[index] + (unsigned)dir);
    i = 0;
    localAngle[0] = angle;
    for (; i != pDirent->count; i++) {
            pPrim = pPrims + i * 2 + context;
            pData = &pDirent->data[i];

            SetPolyFT4(pPrim);
            SetSemiTrans(pPrim, 0);
            SetShadeTex(pPrim, 1);
            pPrim->tpage = GetTPage(pData->texPageId, 0, (s16)pData->texPageX,
                                    pData->texPageY);
            pPrim->clut = GetClut(pData->clutX, pData->clutY);

            nRight = pData->width;
            nBottom = pData->height;
            nLeft = pData->x;
            nTop = (u16)pData->y;
            if (!pData->flipX) {
                g_GfxQuadCorners[0].vx = nLeft;
                g_GfxQuadCorners[1].vx = nLeft + nRight;
                g_GfxQuadCorners[2].vx = nLeft + nRight;
                g_GfxQuadCorners[3].vx = nLeft;
            } else {
                g_GfxQuadCorners[0].vx = nLeft + nRight;
                g_GfxQuadCorners[1].vx = nLeft;
                g_GfxQuadCorners[2].vx = nLeft;
                g_GfxQuadCorners[3].vx = nLeft + nRight;
            }
            if (!pData->flipY) {
                g_GfxQuadCorners[2].vy = nTop + nBottom;
                g_GfxQuadCorners[3].vy = nTop + nBottom;
                g_GfxQuadCorners[0].vy = nTop;
                g_GfxQuadCorners[1].vy = nTop;
            } else {
                g_GfxQuadCorners[2].vy = nTop;
                g_GfxQuadCorners[3].vy = nTop;
                g_GfxQuadCorners[0].vy = nTop + nBottom;
                g_GfxQuadCorners[1].vy = nTop + nBottom;
            }
            RotTransPers4(&g_GfxQuadCorners[0], &g_GfxQuadCorners[1], &g_GfxQuadCorners[2],
                          &g_GfxQuadCorners[3], (long*)&pPrim->x0, (long*)&pPrim->x1,
                          (long*)&pPrim->x3, (long*)&pPrim->x2, &pFlag[0], &pFlag[1]);

            u = pData->u;
            v = (u16)pData->v;
            width = pData->width;
            height = pData->height;
            nAngle = localAngle[0] & 0xFFF;
            if (nAngle == 0xC00) {
                u = u - 1;
            }
            if (nAngle == 0) {
                if (pPrim->x3 < pPrim->x0) {
                    u = u - 1;
                    if ((s16)u < 0) {
                        u = 0;
                        width = width - 1;
                    }
                }
                if (pPrim->y3 < pPrim->y0) {
                    v = v - 1;
                    if ((s16)v < 0) {
                        v = 0;
                        height = height - 1;
                    }
                }
            }

            pPrim->u0 = u;
            pPrim->v0 = v;
            pPrim->u1 = u + width;
            pPrim->v1 = v;
            pPrim->u2 = u;
            pPrim->v2 = v + height;
            pPrim->u3 = u + width;
            pPrim->v3 = v + height;
    }

    SetGeomOffset(oldGeomOffset[0], oldGeomOffset[1]);
    SetGeomScreen(oldGeomScreen);
    PopMatrix();
    return pDirent->count;
}
*/

void ResourceHelperGetTexCoords(MenuResourceDirectory *dir, u32 index,
                   s32* outDirentCount,
                   s32* outtexPageId,
                   s32* outClutX, s32* outClutY, s32* outTexPageX, s32* outTexPageY) {
    int texPageX_lsb, u_shifted;
    MenuResourceDirentTextureMetadata *dirent;
    MenuResourceTextureMetadata *data;
    dirent = (MenuResourceDirentTextureMetadata *)(dir->offsets[index] + (unsigned) dir);
    data = &dirent->data[0];

    *outDirentCount = dirent->count;

    u_shifted = data->u << 0x10;
    if (data->texPageId != 0) {
        texPageX_lsb = u_shifted >> 0x12;
    } else {
        texPageX_lsb = u_shifted >> 0x14;
    }
    *outtexPageId = data->texPageId;
    *outClutX = data->clutX;
    *outClutY = data->clutY;
    *outTexPageX = (s16) (data->texPageX & 0xFFC0) + texPageX_lsb;
    *outTexPageY = (s16) (data->texPageY & 0xFF00) + data->v;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_800263E4);

// Un-park screen: 81.59% under cdk (82.30% was an earlier attempt of this body),
// 75.89% under gcc-2.6.0, 76.39% under gcc-2.7.2, unchanged by -O3/-G0/aspsx.
// Parked: best 82.30% (816 bytes against 888). The corner fills, both clamps,
// the uv stores and the negation blocks already come out instruction for
// instruction; what does not match is register pressure. The original spills
// four more loop-carried values than this shape does (scale at 0x38, the x
// quotient at 0x3C, the record offset at 0x40 and pPrims at 0x44, plus
// `addu s3,s6,zero`-style eager copies before each conditional negation), so
// its source holds about four more long-lived variables than this one. What
// this attempt does reproduce: the 0x70 frame with localX at 0x10, localY at
// 0x18, u8 copies of the two mirror flags at 0x20/0x28, pDirent at 0x30 and i
// at 0x34. Adding a masked scaleVal local or (u16) truncations of the
// quotients did not move it.
// Signature is fixed by the three callers (shop_menu, member_change_menu,
// gear_shop_menu): (dir, index, pPrims, context, x, y, scale, mirrorX,
// mirrorY), mirrorY=1 for the bottom scroll-bar ornament.
/*
// Builds one textured quad per record of the dirent, scaled by `scale` and
// mirrored on either axis when asked to; u/v shrink by one texel when the quad
// comes out flipped so the last column/row is not sampled.
s32 func_800263E4(MenuResourceDirectory* dir, u32 index, POLY_FT4* pPrims,
                  s32 context, u16 x, u16 y, u16 scale, u8 mirrorX, u8 mirrorY) {
    u16 localX = x;
    u16 localY = y;
    u8 mirrorLocalX = mirrorX;
    u8 mirrorLocalY = mirrorY;
    s32 scaleVal = scale & 0xFFFF;
    MenuResourceDirentTextureMetadata* pDirent;
    MenuResourceTextureMetadata* pData;
    POLY_FT4* pPrim;
    s32 nX;
    s32 nY;
    s32 nWidth;
    s32 nHeight;
    u16 u;
    u16 v;
    u16 width;
    u16 height;
    u16 x0;
    u16 x1;
    u16 y0;
    u16 y1;
    s32 i = 0;

    pDirent = (MenuResourceDirentTextureMetadata*)(dir->offsets[index] + (unsigned)dir);
    if (pDirent->count != 0) {
        for (; i != pDirent->count; i++) {
            pData = &pDirent->data[i];
            pPrim = pPrims + i * 2 + context;

            nX = (s16)pData->x * scaleVal / 4096;
            nY = (s16)pData->y * scaleVal / 4096;
            nWidth = (s16)pData->width * scaleVal / 4096;
            nHeight = (s16)pData->height * scaleVal / 4096;

            SetPolyFT4(pPrim);
            SetSemiTrans(pPrim, 0);
            SetShadeTex(pPrim, 1);
            pPrim->tpage = GetTPage(pData->texPageId, 0, (s16)pData->texPageX,
                                    pData->texPageY);
            pPrim->clut = GetClut(pData->clutX, pData->clutY);

            if (mirrorLocalX) {
                nWidth = -nWidth;
                nX = -nX;
            }
            if (mirrorLocalY) {
                nY = -nY;
                nHeight = -nHeight;
            }

            u = pData->u;
            v = pData->v;
            width = pData->width;
            height = pData->height;

            if (!pData->flipX) {
                x0 = localX + nX;
                x1 = x0 + nWidth;
                pPrim->x0 = x0;
                pPrim->x1 = x1;
                pPrim->x2 = x0;
                pPrim->x3 = x1;
            } else {
                x1 = localX + nX;
                x0 = x1 + nWidth;
                pPrim->x0 = x0;
                pPrim->x1 = x1;
                pPrim->x2 = x0;
                pPrim->x3 = x1;
            }

            if (!pData->flipY) {
                y0 = localY + nY;
                y1 = y0 + nHeight;
                pPrim->y0 = y0;
                pPrim->y1 = y0;
                pPrim->y2 = y1;
                pPrim->y3 = y1;
            } else {
                y1 = localY + nY;
                y0 = y1 + nHeight;
                pPrim->y0 = y0;
                pPrim->y1 = y0;
                pPrim->y2 = y1;
                pPrim->y3 = y1;
            }

            if (pPrim->x3 < pPrim->x0) {
                u = u - 1;
                if ((s16)u < 0) {
                    u = 0;
                    width = width - 1;
                }
            }
            if (pPrim->y3 < pPrim->y0) {
                v = v - 1;
                if ((s16)v < 0) {
                    v = 0;
                    height = height - 1;
                }
            }

            pPrim->u0 = u;
            pPrim->v0 = v;
            pPrim->u1 = u + width;
            pPrim->v1 = v;
            pPrim->u2 = u;
            pPrim->v2 = v + height;
            pPrim->u3 = u + width;
            pPrim->v3 = v + height;
        }
    }
    return pDirent->count;
}
*/

// Best 97.55% (688 bytes = retail's, 172 insns = retail's). Residual is allocator + scheduler only:
// - head: gcc hoists the lhu of scale to the top of the prologue and puts the localY store (sh t1,24(sp)) in the beqz delay slot where retail has a nop (retail loads scale just before the beqz, after the dirent count lh).
// - the four finite-division temporaries: retail mflo lands in v1/a1/v1/v0, ours in a0/v1/a0/v0 (same sra/srl choice, so the types are right: nX s32, nY u16, nWidth s32, nHeight u16).
// - addu operand order on x1/x0 only: retail addu v1,s2,v0 (nWidth first), ours (v0,s2), even with the C written nWidth + x0. The y pair is exact after writing nHeight + y0.
// Fixed this round (92.96 -> 97.55): v-- instead of v = pData->v - 1 (the s16 reload cost 2 insns; in-place keeps the lhu), and nWidth/nHeight as the LEFT operand of x1/y1/x0/y0, which fixed both y adds.
// Tried and worse: s32 x0/x1 locals (97.26, 54 diff lines); x0 + nWidth (no change).
// Permuter, 3 min, 1317 iterations from this body: base 420 best 325, all candidates are declaration noise (new_var2 = (s16)pData->height, new_var = pPrim) - no lead.
/*
s32 func_8002675C(MenuResourceDirectory* dir, u32 index, POLY_FT4* pPrims,
                  s32 context, u16 x, u16 y, u16 scale) {
    u16 localX = x;
    u16 localY = y;
    MenuResourceDirentTextureMetadata* pDirent;
    MenuResourceTextureMetadata* pData;
    POLY_FT4* pPrim;
    s32 nX;
    u16 nY;
    s32 nWidth;
    u16 nHeight;
    u16 u;
    u16 v;
    u16 width;
    u16 height;
    u16 x0;
    u16 x1;
    u16 y0;
    u16 y1;
    s32 i = 0;

    pDirent = (MenuResourceDirentTextureMetadata*)(dir->offsets[index] + (unsigned)dir);
    if (pDirent->count != 0) {
        for (; i != pDirent->count; i++) {
            pPrim = pPrims + i * 2 + context;
            pData = &pDirent->data[i];

            nX = (s16)pData->x * scale / 4096;
            nY = (s16)pData->y * scale / 4096;
            nWidth = (s16)pData->width * scale / 4096;
            nHeight = (s16)pData->height * scale / 4096;

            SetPolyFT4(pPrim);
            SetSemiTrans(pPrim, 0);
            SetShadeTex(pPrim, 1);
            pPrim->tpage = GetTPage(pData->texPageId, 0, (s16)pData->texPageX,
                                    pData->texPageY);
            pPrim->clut = GetClut(pData->clutX, pData->clutY);

            u = pData->u;
            v = (u16)pData->v;
            width = pData->width;
            height = pData->height;
            if (!pData->flipX) {
                x0 = localX + nX;
                x1 = nWidth + x0;
                pPrim->x0 = x0;
                pPrim->x1 = x1;
                pPrim->x2 = x0;
                pPrim->x3 = x1;
            } else {
                u = pData->u - 1;
                x1 = localX + nX;
                x0 = nWidth + x1;
                pPrim->x0 = x0;
                pPrim->x1 = x1;
                pPrim->x2 = x0;
                pPrim->x3 = x1;
                if ((s16)u < 0) {
                    u = 0;
                    width = width - 1;
                }
            }

            if (!pData->flipY) {
                y0 = localY + nY;
                y1 = nHeight + y0;
                pPrim->y0 = y0;
                pPrim->y1 = y0;
                pPrim->y2 = y1;
                pPrim->y3 = y1;
            } else {
                v--;
                y1 = localY + nY;
                y0 = nHeight + y1;
                pPrim->y0 = y0;
                pPrim->y1 = y0;
                pPrim->y2 = y1;
                pPrim->y3 = y1;
                if ((s16)v < 0) {
                    v = 0;
                    height = height - 1;
                }
            }

            pPrim->u0 = u;
            pPrim->v0 = v;
            pPrim->u1 = u + width;
            pPrim->v1 = v;
            pPrim->u2 = u;
            pPrim->v2 = v + height;
            pPrim->u3 = u + width;
            pPrim->v3 = v + height;
        }
    }
    return pDirent->count;
}*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/temp2", func_8002675C);



// Builds one SPRT per record of the dirent into pSprites, two sprites per
// record (one per render context), and the DR_MODE that follows them.
// Returns the record count plus one, the number of 2-sprite slots written.
// i must be initialised at its declaration and the tail indexes with `i << 1`:
// both are what make gcc 2.7.2-cdk emit the original's block order.
s32 ResourceHelperBuildSprites(MenuResourceDirectory* dir, u32 index, SPRT* pSprites,
                  int context, u16 x, u16 y) {
    MenuResourceDirentTextureMetadata* pDirent;
    SPRT* pPrim;
    MenuResourceTextureMetadata* pData;
    u16 x0;
    u16 y0;
    u16 u0;
    u16 v0;
    u16 w;
    u16 h;
    int i = 0;

    pDirent = (MenuResourceDirentTextureMetadata*)(dir->offsets[index] + (unsigned)dir);
    if (pDirent->count) {
        for (; i != pDirent->count; i++) {
            pPrim = pSprites + i * 2 + context;
            pData = &pDirent->data[i];
            SetSprt(pPrim);
            pPrim->clut = GetClut(pData->clutX, pData->clutY);
            SetSemiTrans(pPrim, 0);
            SetShadeTex(pPrim, 1);
            x0 = x + pData->x;
            y0 = y + pData->y;
            u0 = pData->u;
            v0 = pData->v;
            w = pData->width;
            h = pData->height;
            pPrim->x0 = x0;
            pPrim->y0 = y0;
            pPrim->u0 = u0;
            pPrim->v0 = v0;
            pPrim->w = w;
            pPrim->h = h;
        }
    }
    pData = &pDirent->data[0];
    SetDrawMode((DR_MODE*)(pSprites + (i << 1) + context), 0, 0,
                GetTPage(pData->texPageId, 0, (s16)pData->texPageX, pData->texPageY) & 0xFFFF,
                NULL);
    return pDirent->count + 1;
}

void ResourceHelperNoOp(void) {
}
