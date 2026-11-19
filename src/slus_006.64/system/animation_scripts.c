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

/* Best: 73.54% (668 vs 668 bytes, retail's exact size). Structure is right: the dispatch
   is the case TREE, the case-1 <3 and case-2 <5 arms are ONE shared block (gcc's tail merge,
   so writing both arms identically is correct), and every arm's instruction sequence is
   present. What is left is pure register naming, and it cascades from the PROLOGUE: retail
   copies the raw animId out of $a1 (move a3,a1) and computes (s16)animId + 0x400 IN PLACE in
   $a1, so $a1 is free and the arms pick $a2/$a0; ours keeps the raw id in $a1, computes the
   cast into $v0 ($a3 then holds prevGroup, where retail uses $t0) and every arm's registers
   shift. Measured dead ends for that copy: the same body with the uint cast only (64.66%,
   652 bytes), prevGroup assigned after the if (69.41%, 648), a permuter run from the 64.66%
   body (4 min, 1308 iterations) found only the u16-load split below (64.66 -> 73.54, size
   652 -> 668). Kept: (a) the switch index is a u8 local - as an s32 gcc emits a comparison
   CHAIN (beq 1 / beqz 0 / beq 2) instead of retail's slti-based tree, so this is the lever
   that closes the dispatch; (b) field_0x54 needs the u16 table entry in its OWN variable,
   the inlined expression is 8 bytes short; (c) the 0x3C word is a whole-word RMW of BIT 3
   (li v0,-9 + or/sll), not of a named bitfield - the header's field_0x3C_6 is bit 28.
   Facts: arg is s32 (field_0x80 = arg is sh of the raw, and case 0 uses arg + 0x400 without
   a sign-extend); (s16)animId + 0x400 & 1 sets/clears flags_0xAC bit 2; savedGroup =
   (flags_0xA8 >> 17) & 7 is compared again after the switch; case 0 = raw id, index 0 in
   the u16 table at anim + 4; case 1 index = ((s16)animId + 0x600) >> 10 & 3, index 3 uses
   entry 1 (anim + 6) and sets bit 2; case 2 index = ((s16)animId + 0x500) >> 9 & 7, 5..7
   remapped as (index - 5) ^ 3; cases 1/2 write flags_0xA8 = (flags & 0xFFF1FFFF) |
   ((index & 7) << 17); the group-changed tail saves animScriptWaitTimer around
   func_80022660(pSpriteData, old pSpriteBytecode, (u32)flags_0xA8 >> 22 & 0x3F) and the
   function ends by mirroring flags_0xAC bit 3 ^ bit 2 into bit 3 of the word at 0x3C.

extern void func_80022660(SpriteData* pSpriteData, void* pSpriteBytecode, s32 scriptIndex);

// Select the animation group for an animation id and restart the script: the id's
// low bits pick an entry in the current animation's offset table, whose target
// becomes the frame list, and a group change re-runs the VM from the new script.
void func_800223B0(SpriteData* pSpriteData, s32 animId) {
    s32 prevGroup;
    s32 index;
    u8 animGroup;
    u32 field0x3C;
    s32 newVar;
    u8* pAnimation;
    s16 waitTimer;
    s32 scriptIndex;
    void* pOldBytecode;

    pSpriteData->field_0x80 = animId;
    prevGroup = ((u32)pSpriteData->flags_0xA8 >> 17) & 0x7;
    if (((s16)animId + 0x400) & 1) {
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
        if (((animId + 0x400) & 0xFFF) >= 0x801) {
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
                (u32)(pAnimation + index * 2 + 4) + *(u16*)(pAnimation + index * 2 + 4);
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
                (u32)(pAnimation + index * 2 + 4) + *(u16*)(pAnimation + index * 2 + 4);
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
}
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/animation_scripts", func_800223B0);

// Run Sprite Animation VM
// Description: while (pSpriteBytecode != pEnd || ((u32)flags_0xA8 >> 22 & 0x3F) != scriptIndex)
// the command byte (*pSpriteBytecode, advanced by 1) is handled: command < 0x80 is a timing /
// frame command (0x00-0x0F: func_8001D2B0(frame + 1); 0x10-0x1F: bump the anim index in
// flags_0xA8 bits 11..16 and SpriteSetFrameFromAnimIndex; 0x20-0x2F: func_8001D2B0(frame - 1);
// 0x30-0x3F: no call) and every command < 0x40 adds (command & 0xF) + 1 to animScriptWaitTimer;
// commands >= 0x80 are the script control ops and the default path advances
// pSpriteBytecode by D_8004FC40[command] (a byte length table). The loop then bumps the script
// index in flags_0xA8 bits 22..27 and, when that wraps to 0, decrements it once more. The
// dispatch over >= 0x80 is one comparison tree in retail, so it wants a switch whose index is
// a u8 local.
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
