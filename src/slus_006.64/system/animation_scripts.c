#include "common.h"
#include "field/actor.h"

/*
Matches on GCC 2.7.2-970404, ASPSX 2.67
Co-Authored-By: eagleflo <eagleflo@users.noreply.github.com>
*/
void* AnimScriptResolveVariablePointer(SpriteData* pSpriteData, u8* pIndex) {
    u8 index = *pIndex;
    if (!(index & 0x80)) {
        s32 signedIndex = (s8)index;
        return &pSpriteData->stack[(s8)pSpriteData->stackIndex + signedIndex];
    }
    return &pSpriteData->field_0x88[index & 0x7F];
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/animation_scripts", func_8001FBE4);

s32 SpriteColorAddClamp(s32 color, s32 value) {
    color += value;
    if (color >= 0x100) {
        color = 0xFF;
    } else if (color < 0) {
        color = 0;
    }
    return color;
}

void SpriteSetSVec(SVEC* pVector, s16 x, s16 y, s16 z) {
    pVector->x = x;
    pVector->y = y;
    pVector->z = z;
}

void SpriteSetIVec3(IVEC3* pVector, s32 x, s32 y, s32 z) {
    pVector->x = x;
    pVector->y = y;
    pVector->z = z;
}

void SpriteCopySVec(SVEC* pDest, SVEC* pSrc) {
    pDest->x = pSrc->x;
    pDest->y = pSrc->y;
    pDest->z = pSrc->z;
}

void SpriteCopyIVec3(IVEC3* pDest, IVEC3* pSrc) {
    pDest->x = pSrc->x;
    pDest->y = pSrc->y;
    pDest->z = pSrc->z;
}

void SpriteSetSemiTransparent(SpriteData* pSpriteData) {
    pSpriteData->prim |= 1;
    func_8001F6B0(pSpriteData);
}

void SpriteSetColor(SpriteData *pSpriteData, u8 red, u8 green, u8 blue) {
    pSpriteData->red = red;
    pSpriteData->green = green;
    pSpriteData->blue = blue;
    pSpriteData->prim &= 0xFE;
    func_8001F6B0(pSpriteData);
}

void SpriteSetSpeedScale(SpriteData* pSpriteData, u32 animIndex) {
    pSpriteData->flags_0xAC.flags =
        (pSpriteData->flags_0xAC.flags & 0xFFF8007F) | ((animIndex & 0xFFF) << 7);
}

void SpriteSetSpecialAnimFile(SpriteData* pSpriteData, void* pAnimFile) {
    pSpriteData->pSpecialAnimFile = pAnimFile;
}

void SpriteSetCallback(SpriteData* pSpriteData, void* pCallback) {
    pSpriteData->field_0x68 = pCallback;
}

void SpriteSetCoordShift(SpriteData* pSpriteData, u32 value) {
    pSpriteData->field_0x40 = (pSpriteData->field_0x40 & 0xFFFFE0FF) | ((value & 0x1F) << 8);
}

// stackIndex is a signed offset into stack[] (pushes move it below zero), even
// though the field itself is a u8.
/*
Matches on GCC 2.7.2-970404, ASPSX 2.67
Co-Authored-By: Mc-muffin <Mc-muffin@users.noreply.github.com>
*/
u_char AnimScriptStackPopU8(SpriteData* pSpriteData) {
    u_char nStackValue;

    nStackValue = pSpriteData->stack[(s8)pSpriteData->stackIndex];
    pSpriteData->stackIndex++;
    return nStackValue;
}

s16 AnimScriptStackPopU16(SpriteData* pSpriteData) {
    s8 stackIndex = (s8)pSpriteData->stackIndex;
    s16 value = pSpriteData->stack[stackIndex] + (pSpriteData->stack[stackIndex + 1] << 8);

    pSpriteData->stackIndex += 2;
    return value;
}

s32 AnimScriptStackPopU24(SpriteData* pSpriteData) {
    s8 stackIndex = (s8)pSpriteData->stackIndex;
    s32 value = pSpriteData->stack[stackIndex] + (pSpriteData->stack[stackIndex + 1] << 8) +
                (pSpriteData->stack[stackIndex + 2] << 16);

    pSpriteData->stackIndex += 3;
    return value;
}

void AnimScriptStackPushU8(SpriteData* pSpriteData, u8 value) {
    pSpriteData->stackIndex -= 1;
    pSpriteData->stack[(s8)pSpriteData->stackIndex] = value;
}

void AnimScriptStackPushU16(SpriteData* pSpriteData, u16 value) {
    pSpriteData->stackIndex -= 2;
    pSpriteData->stack[(s8)pSpriteData->stackIndex] = value;
    pSpriteData->stack[(s8)pSpriteData->stackIndex + 1] = value >> 8;
}

void AnimScriptStackPushU24(SpriteData* pSpriteData, s32 value) {
    pSpriteData->stackIndex -= 3;
    pSpriteData->stack[(s8)pSpriteData->stackIndex] = value;
    pSpriteData->stack[(s8)pSpriteData->stackIndex + 1] = value >> 8;
    pSpriteData->stack[(s8)pSpriteData->stackIndex + 2] = value >> 16;
}

void SpriteSetPositionXZ(SpriteData* pSpriteData, s32 x, s32 z) {
    pSpriteData->position.z = z << 16;
    pSpriteData->position.x = x << 16;
}

// Animation state copied in and out of a script suspension record (see SpriteRestoreAnimState).
// The offsets are the record's own, not SpriteData's.
typedef struct {
    /* 0x00 */ IVEC3 position; // SpriteData.position
    /* 0x0C */ s32 pad0C;
    /* 0x10 */ u16 field_0x80; // SpriteData.field_0x80
    /* 0x12 */ u16 animIndex; // flags_0xA8 bits 11..16
    /* 0x14 */ s16 animId; // top byte of flags_0xAC
    /* 0x16 */ s16 animFlags; // low byte of flags_0xB0
    /* 0x18 */ s16 scriptIndex; // flags_0xA8 bits 22..27
    /* 0x1C */ s32 field_0x1C; // field_0x7C->field_0x0
    /* 0x20 */ s32 field_0x20; // field_0x7C->field_0x4
    /* 0x24 */ u16 scaleX; // SpriteData.pBase->scale
    /* 0x26 */ u16 scaleY;
    /* 0x28 */ u16 scaleZ;
    /* 0x2A */ u16 scale; // SpriteData.scale
    /* 0x2C */ u16 field_0x82; // SpriteData.field_0x82
} SpriteAnimState; // Size: 0x30

extern s32 g_AnimScriptExtraTicks;
extern void AnimScriptTick(SpriteData* pSpriteData);
extern void SpritePlayAnimation(SpriteData* pSpriteData, s32 animId);

// flags_0xAC keeps the animation id in its top byte and flags_0xB0 the animation
// flag byte in its low byte.
void SpriteRestoreAnimState(SpriteData* pSpriteData, SpriteAnimState* pState) {
    s32 savedRenderContext = g_AnimScriptExtraTicks;

    g_AnimScriptExtraTicks = 0;
    pSpriteData->field_0x80 = pState->field_0x80;
    pSpriteData->flags_0xAC.bytes[3] = (u8)pState->animId;
    pSpriteData->flags_0xB0.bytes[0] = (u8)pState->animFlags;
    pSpriteData->pBase->scale.x = pState->scaleX;
    pSpriteData->pBase->scale.y = pState->scaleY;
    pSpriteData->pBase->scale.z = pState->scaleZ;
    pSpriteData->field_0x82 = pState->field_0x82;
    pSpriteData->scale = pState->scale;
    SpritePlayAnimation(pSpriteData, (s8)pSpriteData->flags_0xAC.bytes[3]);
    while (((u32)pSpriteData->flags_0xA8 >> 22 & 0x3F) != pState->scriptIndex) {
        AnimScriptTick(pSpriteData);
        pSpriteData->position.x += pSpriteData->step.x;
        pSpriteData->position.z += pSpriteData->step.z;
        pSpriteData->position.y += pSpriteData->step.y;
        pSpriteData->step.y += pSpriteData->gravity;
    }
    pSpriteData->position.x = pState->position.x;
    pSpriteData->position.y = pState->position.y;
    pSpriteData->position.z = pState->position.z;
    ((SpriteDataI3*)pSpriteData->field_0x7C)->field_0x0 = pState->field_0x1C;
    ((SpriteDataI3*)pSpriteData->field_0x7C)->field_0x4 = pState->field_0x20;
    g_AnimScriptExtraTicks = savedRenderContext;
}

void SpriteSaveAnimState(SpriteData* pSpriteData, SpriteAnimState* pState) {
    pState->position.x = pSpriteData->position.x;
    pState->position.y = pSpriteData->position.y;
    pState->position.z = pSpriteData->position.z;
    pState->field_0x80 = (u16)pSpriteData->field_0x80;
    pState->animIndex = (u32)pSpriteData->flags_0xA8 >> 11 & 0x3F;
    pState->animId = (s8)pSpriteData->flags_0xAC.bytes[3];
    pState->animFlags = (s8)pSpriteData->flags_0xB0.bytes[0];
    pState->scriptIndex = (u32)pSpriteData->flags_0xA8 >> 22 & 0x3F;
    pState->field_0x1C = ((SpriteDataI3*)pSpriteData->field_0x7C)->field_0x0;
    pState->field_0x20 = ((SpriteDataI3*)pSpriteData->field_0x7C)->field_0x4;
    pState->scaleX = (u16)pSpriteData->pBase->scale.x;
    pState->scaleY = (u16)pSpriteData->pBase->scale.y;
    pState->scaleZ = (u16)pSpriteData->pBase->scale.z;
    pState->field_0x82 = (u16)pSpriteData->field_0x82;
    pState->scale = (u16)pSpriteData->scale;
}

void SpriteSetFieldB0(SpriteData* pSpriteData, u8 value) {
    pSpriteData->flags_0xB0.bytes[0] = value;
}

void SpriteSetMoveSpeed(SpriteData* pSpriteData, s32 moveSpeed) {
    pSpriteData->moveSpeed = moveSpeed;
    SpriteUpdateMoveStep(pSpriteData);
}

void SpriteSetDirection(SpriteData* pSpriteData, s16 direction) {
    pSpriteData->direction = direction;
    SpriteUpdateMoveStep(pSpriteData);
}

void SpriteSetScale(SpriteData* pSpriteData, short scale) {
    SpriteDataI1* pBase = pSpriteData->pBase;
    if (pBase) {
        pSpriteData->scale = scale;
        pBase->scale.z = scale;
        pBase->scale.y = scale;
        pBase->scale.x = scale;
        pSpriteData->field_0x3C_6 = 0x1;
    }
}

// Recompute transform matrix if flag is set
void SpriteUpdateTransformMatrix(SpriteData* pSpriteData) {
    if (pSpriteData->field_0x3C_6 & 0x1) {
        SpriteComputeTransformMatrix(pSpriteData);
        pSpriteData->field_0x3C_6 = 0x0;
    }
}

extern MATRIX D_80018644;

void SpriteComputeTransformMatrix(SpriteData* pSpriteData) {
    VECTOR unused;
    VECTOR scale;
    VECTOR scale2;
    MATRIX rotationMatrix;
    MATRIX transformMatrix;
    
    if (!(pSpriteData->field_0x40 & 1)) {
        scale.vx = pSpriteData->pBase->scale.x;
        scale.vy = pSpriteData->pBase->scale.y;
        scale.vz = pSpriteData->pBase->scale.z;
        RotMatrix(&pSpriteData->pBase->rotation, &pSpriteData->pBase->transformMatrix);
        ScaleMatrixL(&pSpriteData->pBase->transformMatrix, &scale);
    } else {
        transformMatrix = D_80018644;
        scale2.vx = pSpriteData->pBase->scale.x;
        scale2.vy = pSpriteData->pBase->scale.y;
        scale2.vz = pSpriteData->pBase->scale.z;
        ScaleMatrixL(&transformMatrix, &scale2);
        RotMatrix(&pSpriteData->pBase->rotation, &rotationMatrix);
        MulMatrix0(&rotationMatrix, &transformMatrix, &pSpriteData->pBase->transformMatrix);
    }
    if (pSpriteData->field_0x3A) {
        scale.vx = pSpriteData->field_0x3A >> 1;
        scale.vy = pSpriteData->field_0x3A >> 1;
        scale.vz = pSpriteData->field_0x3A >> 1;
        ScaleMatrix(&pSpriteData->pBase->transformMatrix, &scale);
    }
}

// Set animation package
extern u8 g_IsBattle;
extern u8 g_BattleStageFileLoadDone;
extern u8 g_BattleRequestedStageFileId;

void SpriteSetupAnimPackage(SpriteAnimPackage* pAnimPackage, SpriteAnimPackageFileHeader* pAnimPackageFile, SVEC2 tex,
                   SVEC2 clut, u32 field_0x3C_1) {
    s32 numAnimations;

    pAnimPackage->tex = tex;
    pAnimPackage->clut = clut;
    pAnimPackage->pPalette = (void*)(pAnimPackageFile->paletteOffset + (mem_addr)pAnimPackageFile);
    pAnimPackage->pFrames = (void*)(pAnimPackageFile->framesOffset + (mem_addr)pAnimPackageFile);
    pAnimPackage->pAnimations = (void*)(pAnimPackageFile->animationsOffset + (mem_addr)pAnimPackageFile);
    g_BattleStageFileLoadDone = 0;

    if (g_IsBattle) {
        numAnimations = (*(u16*)pAnimPackage->pAnimations >> 6) & 0x3F;

        if (numAnimations != 0) {
            g_BattleRequestedStageFileId = numAnimations;
        }
    }
}

extern s32 SpriteAnimPackageTestFlag8000(void*);
extern void SpriteSetupAnimPackage(SpriteAnimPackage*, SpriteAnimPackageFileHeader*, SVEC2, SVEC2, u32);

void SpriteSetAnimFile(SpriteData* pSprite, SpriteAnimPackageFileHeader* pAnimPackageFile) {
    SpriteAnimPackage* pAnimPackage;
    SVEC2 unused[2];

    pAnimPackage = pSprite->pVramData;
    if (pAnimPackageFile) {
        if (pAnimPackageFile != pSprite->pCurAnimFile) {
            SpriteSetupAnimPackage(pAnimPackage, pAnimPackageFile, pAnimPackage->tex, pAnimPackage->clut, pSprite->field_0x3C_1);
            pSprite->pCurAnimFile = pAnimPackageFile;
            pSprite->field_0x3C_3 = 1;
        }

        // Is battle?
        if (g_IsBattle) {
            // Is not VRAM Pre-backed?
            if (!SpriteAnimPackageTestFlag8000(pAnimPackage->pFrames)) {
                pAnimPackage->tex.y = 0x100;
                pAnimPackage->tex.x = 0x300;
                return;
            }
            pAnimPackage->tex = *(SVEC2*)(pSprite->field_0x7C + 0xE);
        }
    }
}

// Select the animation group for an animation id and restart the script: the id's
// low bits pick an entry in the current animation's offset table, whose target
// becomes the frame list, and a group change re-runs the VM from the new script.
//
/* Best: 74.09% (668 bytes, 167 insns; retail is 688 bytes, 172 insns - five `move`s
   short, all of them register shuffles). Structure is right: the switch is the case TREE
   (beq 1 / slti 2 / beqz 0 - so the switch index must be a u8 local, an s32 one makes a
   comparison CHAIN), the case-1 <3 and case-2 <5 arms are ONE shared block (gcc tail merge,
   so writing both arms identically is correct), and every arm's sequence is present.
   The residual is ONE register: retail keeps the raw animId in $a3 for the whole switch and
   therefore frees $a1 to hold the parity computation IN PLACE (sll/sra/addiu/andi a1), while
   every compile here gives the copy $a0 (copy declared first) or $a2 (any other order) and
   the parity temp $v0, so all four argument registers shift and the arms take different
   temporaries - the 5 missing instructions are the `move`s that follow from that.
   MEASURED: (a) the entry copy `s32 index = animId` must exist and must be REASSIGNED in
   case 1/2 (that is what stops copy-propagation) - without it there is no `move a3,a1` at
   all (73.54%, and 73.54% again when the case-1/2 index expressions are written off `index`
   instead of `animId`, which lets gcc coalesce the copy away, 664 bytes); (b) the parity as
   its own local `s32 parity = ((s16)animId + 0x400) & 1` is worth +0.55 (73.54 -> 74.09,
   same 668 bytes); (c) declaration ORDER of those two locals only moves the copy between
   $a0 and $a2 (both 74.09%), never to retail's $a3; (d) a named `s16 sAnimId = animId` for
   the parity instead of the cast is 73.51%. Facts: arg is s32 (field_0x80 = arg is `sh` of
   the raw, and case 0 uses arg + 0x400 with no sign-extend); `(s16)animId + 0x400 & 1` sets
   bit 2 of flags_0xAC; prevGroup = (flags_0xA8 >> 17) & 7 is recomputed and compared after
   the switch; case 0 = raw id, entry 0 of the u16 table at anim + 4; case 1 index =
   ((s16)animId + 0x600) >> 10 & 3, index 3 uses entry 1 (anim + 6) and sets bit 2; case 2
   index = ((s16)animId + 0x500) >> 9 & 7, 5..7 remapped as (index - 5) ^ 3; cases 1/2 write
   flags_0xA8 = (flags & 0xFFF1FFFF) | ((index & 7) << 17); the group-changed tail saves
   animScriptWaitTimer around func_80022660(pSpriteData, old pSpriteBytecode, flags >> 22 & 0x3F)
   and the function ends by mirroring flags_0xAC bit 3 ^ bit 2 into bit 3 of the word at 0x3C. */

// REDO (round 413): un-parked, re-measured 74.09% (668 bytes, 167 insns); retail is 688/172,
// so it is still five `move`s short. Measured this round, each on its own:
//  (e) the arm offset GROUPED - pAnimation + (index * 2 + 4) in both the load and the add
//      base - is worth 74.09 -> 74.33 at the same 668/167: retail adds the 4 to the OFFSET
//      register (addiu a0,a0,0x4) where every ungrouped spelling adds it to the loaded u16.
//  (f) computing parity and the id copy in STATEMENT order (parity first, index = animId
//      after it) only moves the copy from a0 to a2; the copy is still emitted first and
//      parity still lands in v0, never retail's in-place a1.
//  (g) a second local armIndex = index for the arms is coalesced away: 74.09, 167 insns.
//  (h) preset screen: gcc-2.6.0 -O2 scores 71.56 and gcc-2.7.2-cdk -O3 is rejected by cc1,
//      so the unit's cdk -O2 is not the cause.
// WHY THE FIVE MOVES ARE NOT WRITABLE: retail's copies (a3 = id at entry, v1 = pAnim in case
// 0, a3 + a0 = index in case 1, a0 + a3 = index in case 2) all exist because retail's arms
// run at higher register pressure - its index has to survive the arm in a3 while a0/a1 hold
// the arm's temps. Our compile keeps the same live value in the already-dead a1 and needs no
// second copy, so five coalescings happen and no source statement adds a live value back.
// The permuter was not run: at 74% with five allocator-pressure moves (not a shuffle) it
// cannot help.
// ROUND 426 (redo), re-measured 74.33% (668 bytes, 167 insns) - the parked body already
// carries the grouped arm offset (e). The entry block below is the whole remaining gap:
// retail does `addu a3,a1,zero` (raw id copy) and then the parity chain IN PLACE in $a1
// (`sll a1,a1,16 / sra / addiu 0x400 / andi 0x1`), while every spelling here computes the
// chain into a fresh $v0 and keeps the raw id in the argument register, so $v0/$v1 swap and
// the three case-arm `move`s (a3/a0 in case 1, a0/a3 in case 2) never appear: 688-668 = 5.
// Measured this round, each on its own:
//  (i) parity as an s16 INTERMEDIATE, `parity = (s16)animId + 0x400; if (parity & 1)` with
//      prevGroup between the two: 73.77 / 664 (the copy `index = animId` disappears and the
//      chain is still $v0).
//  (j) `index = animId; ... parity = animId; parity = (s16)parity + 0x400;` (a copy that the
//      chain then destroys, which should make $a3 necessary): 73.77 / 664, identical code -
//      copy-prop folds the two initialisations into one `(s16)animId + 0x400` and drops the
//      copy, so the copy still does not exist.
//  (k) splitting the chain (`parity = (s16)animId; parity += 0x400;`) and even assigning the
//      result back INTO the parameter (`animId = parity; if (animId & 1)`) 73.77 / 664: gcc
//      2.7.2-cdk never makes the parameter's own register self-modifying here, it always
//      materialises (s16)x + K in a new $v0.
//  (l) the m2c draft shape (flag value through a temp variable instead of |= / &= ~, the two
//      arm tests inverted to `>=`, the whole body wrapped in `if (pDefaultAnimFile != NULL)`):
//      60.69 / 596, 149 insns - it loses the tail-merged case-1/2 arm block, so it is a
//      different structure, not a closer allocation.
// Conclusion: the copy and the two case-arm moves are not reachable from source with this
// compiler; the park is the honest ceiling. Do not re-try (i)-(l).
// ROUND 439 (redo), re-measured 74.33% (668 bytes, 167 insns; retail 688/172). New measurements this round, each on its own:
//   (m) entry statement order index-first (retail's move a3,a1 precedes sh a1,0x80), i.e.
//       index = animId; field_0x80 = animId; parity = ...; prevGroup = ...; if (parity):
//       74.33 / 668 / 167 - the copy just moves from a2 to a0 and the entry gap is unchanged.
//   (n) the arm-copy shape m2c prints (a separate arm index local, the arm reading it and the
//       flags tail reading the surviving index, so both are live at the slti): gcc coalesces the
//       pair away, 74.33 / 668 / 167, i.e. the same as (g).
//   (o) retail's tail loads animScriptWaitTimer with lh where every compile here emits lhu (the
//       field is s16 in include/field/actor.h and the loaded value only feeds the sh back); a
//       (s16) cast cannot force lh. One instruction, same size, not the gap.
// A fresh m2c_draft.sh body was read: its only shape not already in the body below is (n), and
// its flag-word-through-a-temp form is the 60.69 (l). No new best after five scored runs, so the
// park stands: the five missing instructions are the entry copy into a3 and the two case-arm
// moves, which need retail's a1-in-place parity chain.
/*
void func_800223B0(SpriteData* pSpriteData, s32 animId) {
    s32 prevGroup;
    s32 parity;
    s32 index;
    u8 animGroup;
    u32 field0x3C;
    s32 newVar;
    u8* pAnimation;
    s16 waitTimer;
    s32 scriptIndex;
    void* pOldBytecode;

    pSpriteData->field_0x80 = animId;
    parity = ((s16)animId + 0x400) & 1;
    index = animId;
    prevGroup = ((u32)pSpriteData->flags_0xA8 >> 17) & 0x7;
    if (parity) {
        pSpriteData->flags_0xAC.flags |= 4;
    } else {
        pSpriteData->flags_0xAC.flags &= ~4;
    }
    if (pSpriteData->pDefaultAnimFile == NULL) {
        return;
    }
    animGroup = ((u32)pSpriteData->flags_0xA8 >> 20) & 0x3;
    switch (animGroup) {
    case 0:
        if (((index + 0x400) & 0xFFF) >= 0x801) {
            pSpriteData->flags_0xAC.flags |= 4;
        } else {
            pSpriteData->flags_0xAC.flags &= ~4;
        }
        pAnimation = pSpriteData->pCurAnimation;
        pSpriteData->flags_0xA8 &= 0xFFF1FFFF;
        pSpriteData->field_0x5C = (u32)(pAnimation + 6);
        pSpriteData->field_0x54 = (u32)(pAnimation + 4) + *(u16*)(pAnimation + 4);
        break;
    case 1:
        index = (((s16)animId + 0x600) >> 10) & 0x3;
        if (index < 3) {
            pAnimation = pSpriteData->pCurAnimation;
            pSpriteData->flags_0xAC.flags &= ~4;
            pSpriteData->field_0x54 =
                (u32)(pAnimation + (index * 2 + 4)) + *(u16*)(pAnimation + (index * 2 + 4));
        } else {
            pAnimation = pSpriteData->pCurAnimation;
            pSpriteData->flags_0xAC.flags |= 4;
            pSpriteData->field_0x54 = (u32)(pAnimation + 6) + *(u16*)(pAnimation + 6);
        }
        pSpriteData->flags_0xA8 = (pSpriteData->flags_0xA8 & 0xFFF1FFFF) | ((index & 0x7) << 17);
        break;
    case 2:
        index = (((s16)animId + 0x500) >> 9) & 0x7;
        if (index < 5) {
            pAnimation = pSpriteData->pCurAnimation;
            pSpriteData->flags_0xAC.flags &= ~4;
            pSpriteData->field_0x54 =
                (u32)(pAnimation + (index * 2 + 4)) + *(u16*)(pAnimation + (index * 2 + 4));
        } else {
            index = (index - 5) ^ 0x3;
            pAnimation = pSpriteData->pCurAnimation;
            newVar = *(u16*)(pAnimation + index * 2 + 4);
            pSpriteData->flags_0xAC.flags |= 4;
            pSpriteData->field_0x54 = (u32)(pAnimation + index * 2 + 4) + newVar;
        }
        pSpriteData->flags_0xA8 = (pSpriteData->flags_0xA8 & 0xFFF1FFFF) | ((index & 0x7) << 17);
        break;
    }
    if (prevGroup != (((u32)pSpriteData->flags_0xA8 >> 17) & 0x7)) {
        waitTimer = pSpriteData->animScriptWaitTimer;
        pAnimation = pSpriteData->pCurAnimation;
        pOldBytecode = pSpriteData->pSpriteBytecode;
        scriptIndex = ((u32)pSpriteData->flags_0xA8 >> 22) & 0x3F;
        pSpriteData->flags_0xA8 = (pSpriteData->flags_0xA8 | 0x1F800) & 0xF03FFFFF;
        pSpriteData->pSpriteBytecode = (u32)(pAnimation + 2) + *(u16*)(pAnimation + 2);
        func_80022660(pSpriteData, pOldBytecode, scriptIndex);
        pSpriteData->animScriptWaitTimer = waitTimer;
    }
    field0x3C = (*(u32*)((u8*)pSpriteData + 0x3C) & ~8) |
                (((((u32)pSpriteData->flags_0xAC.flags >> 3) & 1) ^
                  (((u32)pSpriteData->flags_0xAC.flags >> 2) & 1))
                 << 3);
    *(u32*)((u8*)pSpriteData + 0x3C) = field0x3C;
}*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/animation_scripts", func_800223B0);



// Run Sprite Animation VM
// Commands below 0x80 are timing/frame commands and every command below 0x40 adds
// (command & 0xF) + 1 to animScriptWaitTimer; the rest are control opcodes, whose default
// path advances pSpriteBytecode by g_AnimScriptCommandLengths[command]. The loop then bumps
// the script index and, when that wraps to 0, decrements it once more.
//
// Best: 42.73% (808 bytes; retail is 788). The structure is right and the dispatch, the
// three cross-jumped frame-command tails, the duplicated `ticks = (code & 0xF) + 1` and the
// wrap-to-zero index decrement all come out as retail has them. The whole residual is ONE
// thing: gcc hoists the two loop-invariant constants (0xF03FFFFF for the index and the -9
// of the 0xBE flip-bit clear) into callee-saved registers, so the saved set is s0-s8 against
// retail's s0-s5 and the frame is 0x38 instead of 0x30; every register then shifts.
// Retail re-materialises 0xF03FFFFF with lui/ori inside the iteration and keeps it in a
// call-clobbered register, and reaches it from the *stored* word in the count == 0 arm
// (srl $v0,$a1,22), which is the "re-read the field after the store" idiom. Measured:
// the plain whole-word mask form is 40.32% (808 bytes) and writing the 0xBE flip through a
// union bitfield (as system/temp1.c does for the same word) is 42.73% at the same size, so
// the axis is register pressure, not shape. Untried: a spelling that stops loop.c hoisting
// a const_int -- none is known in this repo (AGENTS.md records the same wall for hoisted
// symbol_refs).
//
// Facts for the next attempt: 197 instructions, frame 0x30, saves s0-s5+ra. The dispatch
// over >= 0x80 is a comparison tree, so it wants `switch (code)` on a u8 local; the case set
// is 0x80/0x81/0x82 -> return, 0x86/0x87/0x97 -> shared arm that returns when
// pBytecode == pEnd and otherwise advances by the length table, 0xB3 -> index RMW from
// (s8)pBytecode[1] & 0x3F, 0xBE -> frame/flip/wait command (see below), 0xE2 -> a subroutine
// CALL: it pushes the return address (pBytecode + 3, a 24-bit pointer, which is why the
// callee is AnimScriptStackPushU24) and jumps by the signed 16-bit offset made of
// pBytecode[1] + ((s8)pBytecode[2] << 8). 0xBE reads a u16 operand pBytecode[1] |
// pBytecode[2] << 8: bit 9 -> flags_0xAC bit 3 (flip), bits 0..8 compared against
// curSpriteFrame (unsigned load) to decide the SpriteAnimFrameListSetFrame call, bits 11..14 add to
// animScriptWaitTimer. The wait-timer increment is in a local the frame commands assign, so
// the control-opcode path adds the previous command's value, as retail does; a
// `(s16)value >> 11` spelling is needed for retail's sra in the 0xBE timer term.
// Best: 72.61% (820 bytes; retail is 788, frame 0x38 against retail's 0x30). What closed the
// gap from the round-319 park's 42.73, each measured on its own:
//  - the two advance statements must read pSpriteData->pSpriteBytecode (the field) and not the
//    pBytecode local. The local is live across the frame commands' calls, so it takes a
//    callee-saved register; reading the field lets gcc keep it in the call-clobbered a3 and
//    reload it after each call, which is retail's shape. Worth most of the gain on its own.
//  - the index-bump tail belongs INSIDE the `if (code < 0x80)` block, ending in `continue`:
//    retail's block order is loop head, frame commands, tail (with its `j` back to the head),
//    and only then the switch, which is what the `continue` produces.
//  - 0x86, 0x87 and 0x97 need SEPARATE case bodies. Retail's decision tree has 0x87 as its root
//    node and 0x86 as a separate node, which only happens when the case labels differ (gcc
//    merges adjacent values that share one label into a range node); the three identical blocks
//    are then cross-jumped into retail's single .L80022920.
//  - the 0xE2 offset is the signed 16-bit word: (s16)(pBytecode[2] << 8) gives retail's
//    lbu/sll 24/sra 16, where (s8)pBytecode[2] << 8 gives lb/sll 8.
// WHAT IS LEFT IS ONE THING: we save 8 registers (s0-s7 + ra, frame 0x38) against retail's 6
// (s0-s5 + ra, frame 0x30), and the two extra are the loop-invariant constants -9 (the two
// `and`s of the 0xBE arm) and 0xF03FFFFF (the tail and the count == 0 block). loop.c hoists a
// constant load whose pseudo has more than one use in the loop, and it goes to a callee-saved
// register; no source spelling stops it: cc1_probe.sh on a 20-line replica keeps the `li` in the
// preheader under 2.7.2-cdk, 2.7.2 and 2.6.0, and the preset sweep ranks the unit's own cdk -O2
// first (plain 2.7.2 -O2 70.45, cdk -O3 67.92, 2.6.0 -O3 66.37, -O1 about 50). The one observed
// exception is the point to work from: 0xFFFE07FF is NOT hoisted, because its two uses sit in
// blocks that do not dominate each other, so cse gives each block its own single-use load - and
// retail materialises it twice, separately, for the same reason. A spelling that gives the -9
// and 0xF03FFFFF pseudos one use each while still emitting retail's single materialisation (or a
// preset that skips loop.c invariant motion) is the whole remaining job.
// Best: 91.20% (788 bytes = retail's exact size, retail's exact register set: s0-s5 + ra,
// frame 0x30). Round 330 closed 72.61 -> 91.20 and found the two levers that decide whether
// loop.c hoists a loop-invariant constant (it hoists iff
// (loop_has_call ? 1 : 2) * (1 + n_non_fixed_regs) * savings * m->lifetime >= insn_count,
// with savings = the number of loads of that value, lifetime = the luid span between its first
// and last use, insn_count = the loop's RTL insn count - 187 here; cc1 -dL prints the numbers
// as "Insn N: regno R (life L), move-insn savings S moved/not desirable", see the loop dump
// recipe in the file history):
//  - shorten the SPAN between a constant's first and last use. -9 (the two `& ~8`s) was hoisted
//    at life 14-16 through the s16 value path and at life 7 with its two ANDs 6 insns apart;
//    reading the 0x3C word into a local before the 0xAC update drops it to life 6 = not hoisted.
//  - 0xF03FFFFF was hoisted at life 7 with `(flags & MASK) | (count << 22)` as one expression;
//    hoisting `count << 22` into its own local first puts the mask's use later, life 6, not
//    hoisted. That is the ONLY reason the tail reads that way - it costs retail's `and`-first
//    order in the tail block (the last ordering difference besides the 0xBE arm's).
// Also settled this round: `value` is s32 (an s16/u16 local is hoisted again: 88.61%, 796);
// the case-body source order must be 0xB3 AFTER 0xE2 and 0x86/0x87/0x97 LAST (retail's block
// order is BE, E2, B3, merged-86/87/97-check, length advance; any other order costs 4-8 bytes
// and 10-14 points); the flip bit must be an unfolded `((f >> 3) & 1) ^ ((f >> 2) & 1)` - gcc
// folds it to `(f << 1 & 8 ^ newBit3)` (3 insns) if the 0x3C clear is written early enough to
// let it see that bit 3 of the new word is the flip value; `%hi/%lo` pairs for
// g_AnimScriptCommandLengths stay unhoisted in both compiles.
// Best: 94.23% (788 bytes = retail's exact size). Round 331 took 91.20 -> 94.23 with six
// permuter finds, each ported and re-measured; they are the five odd-looking statements below
// (all load-bearing, do not tidy):
//  - `pArg = &pBytecode[1]` at the head of the loop and `*pArg` in the 0xE2 arm: retail reads
//    pBytecode[1] twice through TWO address registers (`lbu v0,1(a1)` and `lbu s0,1(a3)`), which
//    only a second address value produces (91.20 -> 92.50).
//  - `((value >> 11) & 0xF)`, no `(s16)` cast: retail's `sra` is NOT reachable - every spelling
//    that makes value signed costs a sign-extend pair (s16 local 796 bytes, `(s16)` cast 796);
//    gcc also folds `(value >> 11) & 0xF` back to `srl`+`andi` whatever the mask is written as.
//    Retail's sra comes from a source shape this compiler does not have (92.50 -> 92.83).
//  - `transformFlags = count; transformFlags <<= 22;` as two statements (92.83 -> 93.02).
//  - `((flags >> 2) >> 1) & 1` for the flip XOR's bit 3 (93.02 -> 93.29).
//  - `value = (flags & MASK) | transformFlags;` then `pSpriteData->flags_0xA8 = value;` - the
//    tail's result reuses the 0xBE arm's dead `value` local (93.29 -> 94.23). A NEW local here
//    costs a frame slot and does not match.
// WHAT IS LEFT is register naming only, except one ordering difference: retail's tail block
// computes `and a1,v0,a2` (flags & 0xF03FFFFF) as the SECOND instruction after the load, ours
// puts the same `and` after the `sll`. Retail's source therefore ANDed the mask EARLY
// (`transformFlags = flags & MASK;` first) and updated the wait timer afterwards - but that
// spelling lengthens the mask's luid span to ~12 and loop.c hoists it into a sixth saved
// register (89.30%, 796 bytes). So the two levers conflict; the anti-hoist shape is kept.
// Everything else at 94.23 is the same instructions with different register names: head
// `addiu a1,a3,1` vs ours a0, tail mask a2 vs a3, 0xBE arm -9 in a2 vs a0, and the delay-slot
// position of `sw pSpriteBytecode` (retail one instruction later, in the second beqz's slot).
// Tried and dead: swapping the two 0xBE stores (776 bytes, worse), `(*pSpriteData).f` spelling
// (identical), reusing a `mem_addr`/`u8*` local for pBytecode+3 before the switch (the
// permuter scores it 740 vs 865 but it does NOT reproduce in the real build - 94.23 either way).
// Five permuter runs on this body found only the six edits above; nothing better since.

// SpriteData's 0x3C flag word: the shared header only splits bits 20-31 into fields,
// so the flip bit this VM mirrors the animation flags into sits inside its 20-bit
// field_0x3C_0 (the same view system/temp1.c declares for the word).
typedef union {
    /* 0x0 */ u32 flags;
    /* 0x0 */ u8 bytes[4];
    struct {
        /* 0x0 */ u32 unk0 : 3;
        /* 0x3 */ u32 flipX : 1;
    } fields;
} SpriteTransformFlags;

typedef struct {
    /* 0x00 */ u8 unk0[0x3C];
    /* 0x3C */ SpriteTransformFlags flags_0x3C;
} SpriteTransformFlagsView;

// flags_0xA8's frame counter (bits 11..16) and script index (bits 22..27) are read and
// written whole, so both live in masks rather than bitfields.
#define ANIM_SCRIPT_FRAME_COUNTER_MASK 0xFFFE07FF
#define ANIM_SCRIPT_FRAME_COUNTER_SHIFT 11
#define ANIM_SCRIPT_SCRIPT_INDEX_MASK 0xF03FFFFF
#define ANIM_SCRIPT_SCRIPT_INDEX_SHIFT 22
#define ANIM_SCRIPT_COUNTER_MASK 0x3F
#define ANIM_SCRIPT_COUNTER(word, shift) (((u32)(word) >> (shift)) & ANIM_SCRIPT_COUNTER_MASK)
// The transform's flip bit mirrors the two animation flip bits: set when they differ.
#define ANIM_SCRIPT_FLIP_BIT 8

extern u8 g_AnimScriptCommandLengths[]; // instruction length per opcode
extern void SpriteAnimFrameListSetFrame(SpriteData* pSpriteData, s32 frame);
extern void SpriteSetFrameFromAnimIndex(SpriteData* pSpriteData);

// ROUND 482: re-measured 95.81% (788 bytes = retail's exact size, 197 insns, saves s0-s5 + ra,
// frame 0x30). The prelude (typedefs, ANIM_SCRIPT_* defines, externs) is now OUTSIDE this block on
// purpose: a park.py round-trip eats everything in the block except the body, and the round-469 note
// was measured against a body that had lost it and therefore never compiled.
// Two new measurements this round, both dead ends - the residual is unchanged and is exactly the
// round-469 one (retail's tail-block AND of 0xF03FFFFF sits 3rd, ours 10th after the sll; everything
// else is register naming):
//  (p) moving the 0xBE arm's 0x3C load AFTER the flags_0xAC store (to try to reproduce retail's
//      lbu/lbu/lw a1,60 order): 93.50 / 796 / 199 - it lengthens the FLIP mask -9's life and loop.c
//      then hoists -9 into a sixth saved register (li s5,-9 in the prologue, and a1,a1,s5 /
//      a0,a0,s5 in the arm). So the round-469 note's 'the extra register is 0xF03FFFFF' is only half
//      the story: -9 is the other loop-invariant, and its life is decided by where the 0x3C read sits.
//  (q) the early-AND tail with -9's life left exactly as in the 95.81 body ('newFlags = flags & MASK'
//      as the block's first statement after the flags load, 'newFlags |= transformFlags' after the
//      sll): 796 / 199, this time with lui s5,0xf03f / ori s5,s5,0xffff in the PROLOGUE. So both
//      constants hoist when their own first use moves early enough, and the late-AND shape in the
//      parked body is the only 788-byte one. Confirms the round-469 conclusion from the other side.
// UNTRIED and probably the only remaining angle: retail materialises BOTH constants inside the loop
// with two uses each and hoists neither, while our compile hoists whichever one gets a long enough
// life - i.e. retail's source had some shape that keeps the loop's RTL insn_count above the loop.c
// threshold (see the round-330 formula note above) even with an early AND. Nothing in the body has
// been found that raises insn_count without changing an instruction.
// ROUND 495: re-measured 95.81% (788 bytes = retail's exact size, 197 insns, saves s0-s5 + ra, frame
// 0x30) - unchanged from round 482. The residual is still only the tail block: retail computes
// 'and a1,v0,a2' (flags & 0xF03FFFFF) as the 3rd instruction after the loads, ours as the 10th
// (after the sll); everything else in the function is register naming downstream of that one.
// NEW HARD EVIDENCE that it is a compiler wall, not a spelling (cc1 -dL on the retail-shaped tail,
// i.e. the mask AND before the srl): the loop dump says 'Loop from 14 to 454: 180 real insns' and
// 'Insn 158: regno 129 (life 15), move-insn savings 1 moved to 465'. loop.c hoists iff
// 25 * savings * life >= insn_count, so with insn_count = 180 any constant with life >= 8 is
// hoisted. Retail's instruction order puts the mask's two uses ~15 insns apart (tail AND ->
// count == 0 AND), so it is always hoisted for us; the parked body only escapes because its AND
// sits after the sll (life <= 7). Measured AND positions: after the flags load 93.42 (796, mask
// in s5 and s6 taken from the scriptIndex argument), between the count and the sll 93.42 (796,
// same hoist), after the sll (this body) 95.81 (788, in-loop). Preset sweep on the retail-shaped
// body: unit cdk -O2 93.42, plain 2.7.2 87.78, 2.6.0 87.60, and the unit's already-matched
// functions break under both alternatives, so the preset is not the cause. Also measured:
// '((flags >> 3) & 1) ^ ((flags >> 2) & 1)' for the flip XOR (retail's interleaved
// srl/andi/srl/andi) 95.66, worse than the parked nested '(flags >> 2) >> 1' form which gcc CSEs
// into the same two shifts but pairs; the 0xBE 0x3C load after the two lbus 93.50 / 796 = note (p).
// 100.00% needs retail's tail order with a mask life of 15 and a loop.c that does not hoist it.
/*
void func_80022660(SpriteData* pSpriteData, u8* pEnd, s32 scriptIndex) {
    u8* pBytecode;
    u8* pArg;
    u8 code;
    s32 ticks;
    s32 count;
    s32 offset;
    u32 flags;
    s32 counterMask;
    u32 newFlags;
    u32 transformFlags;
    s32 value;

    while (1) {
        pBytecode = pSpriteData->pSpriteBytecode;
        if (pBytecode == pEnd &&
            ANIM_SCRIPT_COUNTER(pSpriteData->flags_0xA8, ANIM_SCRIPT_SCRIPT_INDEX_SHIFT) == scriptIndex) {
            return;
        }
        code = *pBytecode;
        pArg = &pBytecode[1];
        if (code < 0x80) {
            pSpriteData->pSpriteBytecode = pArg;
            if (code < 0x10) {
                SpriteAnimFrameListSetFrame(pSpriteData, (u16)pSpriteData->curSpriteFrame + 1);
                ticks = (code & 0xF) + 1;
            } else if (code < 0x20) {
                pSpriteData->flags_0xA8 =
                    (pSpriteData->flags_0xA8 & ANIM_SCRIPT_FRAME_COUNTER_MASK) |
                    (((ANIM_SCRIPT_COUNTER(pSpriteData->flags_0xA8, ANIM_SCRIPT_FRAME_COUNTER_SHIFT) + 1) &
                      ANIM_SCRIPT_COUNTER_MASK)
                     << ANIM_SCRIPT_FRAME_COUNTER_SHIFT);
                SpriteSetFrameFromAnimIndex(pSpriteData);
                ticks = (code & 0xF) + 1;
            } else if (code < 0x30) {
                SpriteAnimFrameListSetFrame(pSpriteData, (u16)pSpriteData->curSpriteFrame - 1);
                ticks = (code & 0xF) + 1;
            }
            if (code < 0x40) {
                ticks = (code & 0xF) + 1;
            }
            flags = pSpriteData->flags_0xA8;
            count = (ANIM_SCRIPT_COUNTER(flags, ANIM_SCRIPT_SCRIPT_INDEX_SHIFT) + 1) &
                    ANIM_SCRIPT_COUNTER_MASK;
            pSpriteData->animScriptWaitTimer += ticks;
            transformFlags = count;
            transformFlags = transformFlags << ANIM_SCRIPT_SCRIPT_INDEX_SHIFT;
            newFlags = flags & ANIM_SCRIPT_SCRIPT_INDEX_MASK;
            newFlags |= transformFlags;
            pSpriteData->flags_0xA8 = newFlags;
            if (count == 0) {
                pSpriteData->flags_0xA8 =
                    (pSpriteData->flags_0xA8 & ANIM_SCRIPT_SCRIPT_INDEX_MASK) |
                    (((ANIM_SCRIPT_COUNTER(pSpriteData->flags_0xA8, ANIM_SCRIPT_SCRIPT_INDEX_SHIFT) - 1) &
                      ANIM_SCRIPT_COUNTER_MASK)
                     << ANIM_SCRIPT_SCRIPT_INDEX_SHIFT);
            }
            continue;
        }
        {
            switch (code) {
            case 0x80:
            case 0x81:
            case 0x82:
                return;
            case 0xBE:
                flags = pSpriteData->flags_0xAC.flags;
                value = pBytecode[1] | (pBytecode[2] << 8);
                transformFlags = ((SpriteTransformFlagsView*)pSpriteData)->flags_0x3C.flags;
                flags &= ~ANIM_SCRIPT_FLIP_BIT;
                flags |= ((u32)value >> 6) & ANIM_SCRIPT_FLIP_BIT;
                pSpriteData->flags_0xAC.flags = flags;
                transformFlags = (transformFlags & ~ANIM_SCRIPT_FLIP_BIT) |
                                 (((((flags >> 2) >> 1) & 1) ^ ((flags >> 2) & 1)) << 3);
                ((SpriteTransformFlagsView*)pSpriteData)->flags_0x3C.flags = transformFlags;
                if ((u16)pSpriteData->curSpriteFrame != (value & 0x1FF)) {
                    SpriteAnimFrameListSetFrame(pSpriteData, value & 0x1FF);
                }
                pSpriteData->animScriptWaitTimer += ((value >> 11) & 0xF) + 1;
                break;
            case 0xE2:
                // subroutine call: push the return address, jump by the signed 16-bit offset
                offset = *pArg + (s16)(pArg[1] << 8);
                AnimScriptStackPushU24(pSpriteData, (mem_addr)(pBytecode + 3));
                pSpriteData->pSpriteBytecode = (u8*)pSpriteData->pSpriteBytecode + (s16)offset;
                continue;
            case 0xB3:
                counterMask = ANIM_SCRIPT_COUNTER_MASK;
                pSpriteData->flags_0xA8 = (pSpriteData->flags_0xA8 & ANIM_SCRIPT_FRAME_COUNTER_MASK) |
                                          ((((s8)pBytecode[1]) & counterMask)
                                           << ANIM_SCRIPT_FRAME_COUNTER_SHIFT);
                break;
            case 0x86:
                if (pBytecode == pEnd) {
                    return;
                }
                break;
            case 0x87:
                if (pBytecode == pEnd) {
                    return;
                }
                break;
            case 0x97:
                if (pBytecode == pEnd) {
                    return;
                }
                break;
            }
            pSpriteData->pSpriteBytecode =
                (u8*)pSpriteData->pSpriteBytecode + g_AnimScriptCommandLengths[code];
            continue;
        }
    }
}*/
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/animation_scripts", func_80022660);






// Recompute animation speed
void SpriteUpdateMoveStep(SpriteData* pSpriteData) {
    s32 speed;
    s32 scale;

    speed = (pSpriteData->moveSpeed >> 4) << 8;
    scale = speed / (s32)(((u32)pSpriteData->flags_0xAC.flags >> 7) & 0xFFF);
    pSpriteData->step.x = ((rsin(pSpriteData->direction) >> 2) * scale) >> 6;
    pSpriteData->step.z = -((rcos(pSpriteData->direction) >> 2) * scale) >> 6;
}

s32 SpriteGetPositionX(SpriteData* pSpriteData) {
    return pSpriteData->position.x;
}
