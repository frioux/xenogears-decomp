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
    pSpriteData->flags_0xAC = (pSpriteData->flags_0xAC & 0xFFF8007F) | ((animIndex & 0xFFF) << 7);
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
typedef struct {
    /* 0x0  */ s32 positionX;
    /* 0x4  */ s32 positionY;
    /* 0x8  */ s32 positionZ;
    /* 0xC  */ s32 padC;
    /* 0x10 */ u16 field_0x80;
    /* 0x12 */ u16 animIndex;
    /* 0x14 */ s16 animId;
    /* 0x16 */ s16 field_0xB0;
    /* 0x18 */ s16 scriptIndex;
    /* 0x1C */ s32 field_0x1C;
    /* 0x20 */ s32 field_0x20;
    /* 0x24 */ u16 scaleX;
    /* 0x26 */ u16 scaleY;
    /* 0x28 */ u16 scaleZ;
    /* 0x2A */ u16 field_0x2C;
    /* 0x2C */ u16 field_0x82;
} SpriteAnimState; // Size: 0x30

extern s32 D_80059198;
extern void AnimScriptTick(SpriteData* pSpriteData);
extern void func_800245D8(SpriteData* pSpriteData, s32 animId);

void SpriteRestoreAnimState(SpriteData* pSpriteData, SpriteAnimState* pState) {
    s32 savedRenderContext = D_80059198;

    D_80059198 = 0;
    pSpriteData->field_0x80 = pState->field_0x80;
    *(u8*)((u8*)pSpriteData + 0xAF) = *(u8*)((u8*)pState + 0x14);
    *(u8*)((u8*)pSpriteData + 0xB0) = *(u8*)((u8*)pState + 0x16);
    pSpriteData->pBase->scale.x = pState->scaleX;
    pSpriteData->pBase->scale.y = pState->scaleY;
    pSpriteData->pBase->scale.z = pState->scaleZ;
    pSpriteData->field_0x82 = pState->field_0x82;
    pSpriteData->scale = pState->field_0x2C;
    func_800245D8(pSpriteData, (s8)*(u8*)((u8*)pSpriteData + 0xAF));
    while (((u32)pSpriteData->flags_0xA8 >> 22 & 0x3F) != pState->scriptIndex) {
        AnimScriptTick(pSpriteData);
        pSpriteData->position.x += pSpriteData->step.x;
        pSpriteData->position.z += pSpriteData->step.z;
        pSpriteData->position.y += pSpriteData->step.y;
        pSpriteData->step.y += pSpriteData->gravity;
    }
    pSpriteData->position.x = pState->positionX;
    pSpriteData->position.y = pState->positionY;
    pSpriteData->position.z = pState->positionZ;
    ((SpriteDataI3*)pSpriteData->field_0x7C)->field_0x0 = pState->field_0x1C;
    ((SpriteDataI3*)pSpriteData->field_0x7C)->field_0x4 = pState->field_0x20;
    D_80059198 = savedRenderContext;
}

void SpriteSaveAnimState(SpriteData* pSpriteData, SpriteAnimState* pState) {
    pState->positionX = pSpriteData->position.x;
    pState->positionY = pSpriteData->position.y;
    pState->positionZ = pSpriteData->position.z;
    pState->field_0x80 = (u16)pSpriteData->field_0x80;
    pState->animIndex = (u32)pSpriteData->flags_0xA8 >> 11 & 0x3F;
    pState->animId = (s8)*(u8*)((u8*)pSpriteData + 0xAF);
    pState->field_0xB0 = (s8)*(u8*)((u8*)pSpriteData + 0xB0);
    pState->scriptIndex = (u32)pSpriteData->flags_0xA8 >> 22 & 0x3F;
    pState->field_0x1C = ((SpriteDataI3*)pSpriteData->field_0x7C)->field_0x0;
    pState->field_0x20 = ((SpriteDataI3*)pSpriteData->field_0x7C)->field_0x4;
    pState->scaleX = (u16)pSpriteData->pBase->scale.x;
    pState->scaleY = (u16)pSpriteData->pBase->scale.y;
    pState->scaleZ = (u16)pSpriteData->pBase->scale.z;
    pState->field_0x82 = (u16)pSpriteData->field_0x82;
    pState->field_0x2C = (u16)pSpriteData->scale;
}

void SpriteSetFieldB0(SpriteData* pSpriteData, u8 value) {
    *(u8*)((u8*)pSpriteData + 0xB0) = value;
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
extern u8 D_800591AD;
extern u8 D_800591B0;
extern u8 D_800591B3;

void SpriteSetupAnimPackage(SpriteAnimPackage* pAnimPackage, SpriteAnimPackageFileHeader* pAnimPackageFile, SVEC2 tex,
                   SVEC2 clut, u32 field_0x3C_1) {
    s32 numAnimations;

    pAnimPackage->tex = tex;
    pAnimPackage->clut = clut;
    pAnimPackage->pPalette = (void*)(pAnimPackageFile->paletteOffset + (mem_addr)pAnimPackageFile);
    pAnimPackage->pFrames = (void*)(pAnimPackageFile->framesOffset + (mem_addr)pAnimPackageFile);
    pAnimPackage->pAnimations = (void*)(pAnimPackageFile->animationsOffset + (mem_addr)pAnimPackageFile);
    D_800591B0 = 0;

    if (D_800591AD) {
        numAnimations = (*(u16*)pAnimPackage->pAnimations >> 6) & 0x3F;

        if (numAnimations != 0) {
            D_800591B3 = numAnimations;
        }
    }
}

extern s32 func_8001EE68(void*);
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
        if (D_800591AD) {
            // Is not VRAM Pre-backed?
            if (!func_8001EE68(pAnimPackage->pFrames)) {
                pAnimPackage->tex.y = 0x100;
                pAnimPackage->tex.x = 0x300;
                return;
            }
            pAnimPackage->tex = *(SVEC2*)(pSprite->field_0x7C + 0xE);
        }
    }
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/animation_scripts", func_800223B0);

// Run Sprite Animation VM
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/animation_scripts", func_80022660);

// Recompute animation speed
void SpriteUpdateMoveStep(SpriteData* pSpriteData) {
    s32 speed;
    s32 scale;

    speed = (pSpriteData->moveSpeed >> 4) << 8;
    scale = speed / (s32)(((u32)pSpriteData->flags_0xAC >> 7) & 0xFFF);
    pSpriteData->step.x = ((rsin(pSpriteData->direction) >> 2) * scale) >> 6;
    pSpriteData->step.z = -((rcos(pSpriteData->direction) >> 2) * scale) >> 6;
}

s32 SpriteGetPositionX(SpriteData* pSpriteData) {
    return pSpriteData->position.x;
}
