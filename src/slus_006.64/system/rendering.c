#include "common.h"
#include "field/actor.h"
#include "system/memory.h"

extern void func_8001E148(SpriteData* pSpriteData);
extern void func_8001E3D8(SpriteData* pSpriteData, void* pArg);
extern void func_8001E9BC(SpriteData* pSpriteData, void* pArg);
extern void func_8001EE88(SpriteData* pSpriteData, void* pArg1, void* pArg2);
extern void func_8001F1D4(SpriteData* pSpriteData, void* pArg1, void* pArg2);
extern void func_8001F750(SpriteData* pSpriteData, s32 index, SpriteAnimPackage* pAnimPackage);
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

extern MATRIX D_8004FBB8;
extern u8 D_800591AD;
extern u8 D_800591AE;

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering", func_8001E148);
/* Best attempt: 90.89% (332 bytes, the original is 336). Every field, cast and
   instruction shape in the body matches -- the `lb` (not lbu) loads want the
   (s8) offsets, the flags read wants the (u32) cast to stay srl, and the
   `pMat = &D_8004FBB8` local is required or gcc re-forms lui/addiu for each
   t[] access after the ApplyMatrix call. What is left is callee-saved
   allocation: the original holds the matrix address in s1, yOffset in s3 and
   xOffset in s2 with pSpriteData in s0; this C always gives the address the
   lowest-priority register (s3) because it has the fewest references, and
   moving its declaration to the top of the list just moves pSpriteData out of
   s0 instead. Also 86.76% under gcc-2.7.2-cdk-psx, so it is not the preset.

void func_8001E148(SpriteData* pSpriteData) {
    MATRIX* pMat = &D_8004FBB8;
    SVECTOR position;
    VECTOR vec;
    MATRIX* pMatrix;
    s32 n;
    s32 yOffset;
    s32 xOffset;

    if ((D_800591AD != 0) || (D_800591AE != 0)) {
        func_80022038(pSpriteData);
    }

    n = (pSpriteData->field_0x40 >> 8) & 0x1F;
    yOffset = (s8)pSpriteData->pBase->offsetY;
    yOffset <<= n;
    xOffset = (s8)pSpriteData->pBase->offsetX;
    xOffset <<= n;

    if (((u32)pSpriteData->flags_0xAC >> 2) & 1) {
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

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering", func_8001F5BC);
/* Best attempt: 95.16%. UPDATE: this C scores 98.52% under gcc-2.7.2-cdk-psx
   too, so the preset is not the answer here; the miss is the allocator.
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

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering", func_8001F6B0);
/* Best attempt: 96.50%. UPDATE: this C scores 97.12% under gcc-2.7.2-cdk-psx,
   so the preset is not the answer; the miss is the allocator.
   Instruction stream is identical; only the temp
   assignment differs (original: pSpriteData->t1, texPageBits->a3, color->a1,
   pFrameData->a0, i->t0; this C: a3, t0, a2, v1, a0). Reordering the locals has
   no effect at all (re-confirmed). The sprite reaching t1 rather than a3 is the
   whole difference: a0 goes to the flags word, so the parameter has to be copied
   out of its incoming register.

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
*/
/* Best attempt: 96.50%, instruction stream identical, only the register
   allocation differs (the original gets the sprite data into t1, freeing a0 for
   the frame pointer; this keeps the sprite in a3 and puts the counter in a0).
   The 4-byte copy of ((SpriteAnimPackage*)pVramData)->tex into a local is dead
   code that the original object really contains.

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
*/

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering", func_8001F750);
/* Best attempt: 89.74% (404 bytes, the original's size). The whole structure is
   right -- flags/0x3F count split, the 4 + 2*count entry stride, the
   SpriteDirectionTransform stores, the loop tail (i++, break, pEntry += 3, which
   only reproduces the original's delay slot with a for(;;) + break) -- and every
   callee-saved register lands where the original has it. What is left is pure
   instruction scheduling: the original loads the entry flags byte before the
   pBase/pDirTransforms loads in the 0x10 rotationZ arm, keeps `pEntry++` in the
   delay slot of the `beqz (flags & 0x40)`, and sinks the `pEntry = pBlock + ...`
   add into the delay slot of the entry test. Address arithmetic is written with
   mem_addr because that is the only way it emits `addu offset,base` rather than
   `addu base,offset`.

void func_8001F750(SpriteData* pSpriteData, s32 index, SpriteAnimPackage* pAnimPackage) {
    u8* pFrames;
    u8* pBlock;
    u8* pEntry;
    u32 flags;
    u32 unkFlag;
    u32 count;
    u8 entryFlags;
    s32 dir;
    s32 i;

    pFrames = pAnimPackage->pFrames;
    pBlock = pFrames + ((u16*)pFrames)[index];
    flags = *pBlock;
    unkFlag = flags & 0x80;
    count = flags & 0x3F;
    pEntry = pBlock + (count * 2 + 4);
    i = 0;
    if (count != 0) {
        for (;;) {
        entryFlags = *pEntry;
        if (entryFlags & 0x80) {
            if (entryFlags & 0x40) {
                pEntry++;
                dir = entryFlags & 7;
                if (pSpriteData->pBase->pDirTransforms == 0) {
                    pSpriteData->pBase->pDirTransforms = HeapAlloc(sizeof(SpriteDirectionTransforms), 0);
                    func_800234AC(pSpriteData);
                }
                if (entryFlags & 0x20) {
                    *(u8*)((mem_addr)pSpriteData->pBase->pDirTransforms + dir * 8) = *pEntry++;
                    *(u8*)((mem_addr)pSpriteData->pBase->pDirTransforms + (dir * 8 + 1)) = *pEntry++;
                }
                if (entryFlags & 0x10) {
                    *(s16*)((mem_addr)pSpriteData->pBase->pDirTransforms + (dir * 8 + 6)) = *pEntry++ << 4;
                } else {
                    *(s16*)((mem_addr)pSpriteData->pBase->pDirTransforms + (dir * 8 + 6)) = 0;
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

*/

// Animation frame and tile data stuff
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/rendering", func_8001F8E8);
/* Best attempt: 88.26%. Byte-stream parser; every branch, delay slot and store
   matches. Only difference: `dir` lands in s3 (callee-saved) here but in a1 in
   the original, which pushes i/count/unkFlag up one register and grows the frame
   to 64 bytes (plus one duplicated `andi v0,s1,0x10`).

void func_8001F8E8(SpriteData* pSpriteData, s32 index, SpriteAnimPackage* pAnimPackage) {
    u8* pFrames;
    u8* pBlock;
    u8* pEntry;
    u16 header;
    u8 flags;
    u32 unkFlag;
    u32 count;
    u8 entryFlags;
    s32 dir;
    s32 i;

    pFrames = pAnimPackage->pFrames;
    header = *(u16*)pFrames;
    if (index < (header & 0x1FF) + 1) {
        if (header & 0x8000) {
            func_8001F750(pSpriteData, index, pAnimPackage);
            return;
        }
        pBlock = pFrames + ((u16*)pFrames)[index];
        flags = *pBlock;
        i = 0;
        unkFlag = flags & 0x80;
        count = flags & 0x3F;
        pEntry = pBlock + (count * 4 + 6);
        if (count != 0) {
            for (;;) {
                entryFlags = *pEntry;
                if (entryFlags & 0x80) {
                    if (entryFlags & 0x40) {
                        pEntry++;
                        if (pSpriteData->pBase->pDirTransforms == 0) {
                            pSpriteData->pBase->pDirTransforms = HeapAlloc(sizeof(SpriteDirectionTransforms), 0);
                            func_800234AC(pSpriteData);
                        }
                        if (entryFlags & 0x20) {
                            dir = entryFlags & 7;
                            *(u8*)((mem_addr)pSpriteData->pBase->pDirTransforms + dir * 8) = *pEntry++;
                            *(u8*)((mem_addr)pSpriteData->pBase->pDirTransforms + (dir * 8 + 1)) = *pEntry++;
                        }
                        if (entryFlags & 0x10) {
                            *(s16*)((mem_addr)pSpriteData->pBase->pDirTransforms + (dir * 8 + 6)) = *pEntry++ << 4;
                        } else {
                            *(s16*)((mem_addr)pSpriteData->pBase->pDirTransforms + (dir * 8 + 6)) = 0;
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
*/

// GraphicsDrawPauseLetters and func_8001FB30 were built by a cdk-compiler TU;
// they live in system/rendering_pause_text.c, which carries that preset.
