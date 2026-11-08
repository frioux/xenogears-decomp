#include "common.h"
#include "field/actor.h"
#include "system/memory.h"

extern void func_8001E148(SpriteData* pSpriteData);
extern void func_8001E3D8(SpriteData* pSpriteData, void* pArg);
extern void func_8001E9BC(SpriteData* pSpriteData, void* pArg);
extern void func_8001EE88(SpriteData* pSpriteData, void* pArg1, void* pArg2);
extern void func_8001F1D4(SpriteData* pSpriteData, void* pArg1, void* pArg2);
extern void func_800234AC(SpriteData* pSpriteData);

// Rendering-related stuff

// Allocate directions array
void func_8001D4E8(SpriteData* pSpriteData) {
    if (pSpriteData->pBase->pDirTransforms == 0) {
        pSpriteData->pBase->pDirTransforms = HeapAlloc(sizeof(SpriteDirectionTransforms), 0);
        func_800234AC(pSpriteData);
    }
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering", func_8001D53C);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering", func_8001DAE8);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering", func_8001E148);
/*
Matches on  GCC 2.7.2-970404, ASPSX 2.67

extern MATRIX D_8004FBB8;
extern u8 D_800591AD;
extern u8 D_800591AE;

void func_8001E148(SpriteData* pSpriteData) {
    SVECTOR position;
    VECTOR vec;
    MATRIX* pMatrix;
    s32 var_n;
    s32 var_v0;
    int xOffset;
    int yOffset;
    
    if ((D_800591AD != 0) || (D_800591AE != 0)) {
        func_80022038(pSpriteData);
    }

    var_n = pSpriteData->field_0x40_0x8 & 0x1F;
    yOffset = pSpriteData->pBase->offsetY;
    yOffset <<= var_n;
    xOffset = pSpriteData->pBase->offsetX;
    xOffset <<= var_n;

    // Flip X?
    if (pSpriteData->flags_0xAC_0x2) {
        xOffset = -xOffset;
    }
    
    yOffset = yOffset * pSpriteData->scale / 0x1000;
    xOffset = xOffset * pSpriteData->scale / 0x1000;
    
    position.vx = pSpriteData->position.x >> 0x10;
    position.vy = pSpriteData->position.y >> 0x10;
    position.vz = pSpriteData->position.z >> 0x10;
    ApplyMatrix(&D_8004FBB8, &position, &vec);
    pMatrix = &pSpriteData->pBase->transformMatrix;
    pMatrix->t[0] = D_8004FBB8.t[0] + vec.vx + xOffset;
    pMatrix->t[1] = D_8004FBB8.t[1] + vec.vy + yOffset;
    pMatrix->t[2] = D_8004FBB8.t[2] + vec.vz;
    SetRotMatrix(pMatrix);
    SetTransMatrix(pMatrix);
}
*/

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

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering", func_8001EE88);

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

void func_8001F5BC(SpriteData* pSpriteData, s32 unusedIndex, s32* pOutX, s32* pOutY, s32* pOutZ) {
    SpriteAnimPackage* pPackage;
    u8* pAnimations;
    u16* pFrames;
    u8* pFrame;
    s32 index;

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

void func_8001F6B0(SpriteData* pSpriteData) {
    s32 i;
    SpriteFrameData* pFrameData;
    u32 flags;
    u32 color;
    u32 texPageBits;
    u16 texPage;
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

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering", func_8001F750);

// Animation frame and tile data stuff
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering", func_8001F8E8);



INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering", GraphicsDrawPauseLetters);
/* Best attempt: 98.81%, only the callee-saved registers differ (original keeps
   x in s1 and the image pointer in s0, this keeps them in s0 and s1).

extern void func_8002DDE4(void* pImgData, int arg1, int x, int y, int arg4, int arg5, int arg6);
extern void* LZSSHeapDecompress(void* pCompressedData, int flags);
extern void* g_GfxPauseLettersCompressed[];

void GraphicsDrawPauseLetters(int x, int y) {
    void* pPauseImgData = LZSSHeapDecompress(&g_GfxPauseLettersCompressed, 0x0);
    func_8002DDE4(pPauseImgData, 1, x, y, 0, 0, 0);
    DrawSync(0);
    HeapFree(pPauseImgData);
}
*/

// 800592E4 -> EA is bss local
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering", func_8001FB30);

