#include "common.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"
#include "system/memory.h"
#include "libgear/main.h"
#include "system/animation.h"

extern s32 D_80050100;

// Interpolation helpers; func_801E34BC picks one for a type
s16 func_801E0850(s16 angle, s16 divisor, s16 offset);
s16 func_801E08D4(s16 a, s16 b, s16 c);
s16 func_801E0938(s16 a, s16 b, s16 c);
s16 func_801E0988(s16 a, s16 b, s16 c);

extern Temp8* D_8005919C;
extern u32 D_801E85CC;
extern u16 D_801E863C;
extern u16 D_801E86B0;
extern s32 D_801E8640;
extern s16 D_801E869C;
extern s16 D_801E8698;

void func_801DCEC8(Model* pModel, MATRIX* pMatrix, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6);
void func_801E37D0(Model* pModel);
extern VertexBuffer g_LibGearVertexBuffer;

ModelMesh* LibGearInitializeMesh(ModelFileHeader* pModelFile, ModelMesh* pModel) {
    u_int numParts;
    int i;

    HeapChangeCurrentUser(HEAP_USER_MASA, HEAP_CONTENT_NULL);
    numParts = ModelResolvePointers(pModelFile);
    pModel->pParts = HeapAlloc(numParts * sizeof(ModelPart*), 0x0);
    pModel->numParts = numParts;
    if (pModel->pParts != NULL) {
        for (i = 0; i < numParts; i++) {
            pModel->pParts[i] = &pModelFile->modelParts[i];
        }
    }
    return pModel;
}

ModelJoint* LibGearInitializeSkeleton(ModelMesh* pModel, ModelJointEntry* pJointEntries, s32 arg2, s32 hasTexture, s16 texX, s16 texY, s16 clutX, s16 clutY) {
    int i;
    int numJoints;
    u_short jointIndex;
    u_short parentIndex;
    ModelJointEntry* pCurJointEntry;
    ModelJoint* pSkeleton;
    ModelJoint* pCurJoint;

    HeapChangeCurrentUser(HEAP_USER_MASA, HEAP_CONTENT_NULL);
    
    pCurJointEntry = pJointEntries;
    jointIndex = pCurJointEntry->jointIndex;
    numJoints = 0;
    while (jointIndex < pModel->numParts || jointIndex == 0xFFFF) {
        pCurJointEntry++;
        jointIndex = pCurJointEntry->jointIndex;
        numJoints++;
    }

    if (numJoints == 0) {
        return NULL;
    }

    // Always account for the root bone / joint
    numJoints++;
    
    pSkeleton = HeapAlloc(numJoints * sizeof(ModelJoint), 0x0);
    if (pSkeleton == NULL) {
        return NULL;
    }
    
    pCurJointEntry = pJointEntries;
    pCurJoint = pSkeleton + 1;
    i = 1;
    jointIndex = pCurJointEntry->jointIndex;
    parentIndex = pCurJointEntry->parentIndex;

    // Root bone / joint
    pSkeleton[0].doTransform = TRUE;
    pSkeleton[0].doRotate = TRUE;
    pSkeleton[0].rotDirection = 1;
    pSkeleton[0].scale.vx = 0x1000;
    pSkeleton[0].scale.vy = 0x1000;
    pSkeleton[0].scale.vz = 0x1000;
    pSkeleton[0].pParent = NULL;
    pSkeleton[0].unk7 = 0;
    pSkeleton[0].jointIndex = 0xFFFF;
    pSkeleton[0].numChildren = numJoints;
    pSkeleton[0].pModelPacketBuffer = NULL;
    pSkeleton[0].pCurModelPacket = NULL;
    pSkeleton[0].rotation.vx = 0;
    pSkeleton[0].rotation.vy = 0;
    pSkeleton[0].rotation.vz = 0;
    pSkeleton[0].translation.x = 0;
    pSkeleton[0].translation.y = 0;
    pSkeleton[0].translation.z = 0;
    pSkeleton[0].unk70 = 0;
    pSkeleton[0].unk74 = 0;
    pSkeleton[0].unk78 = 0;
  
    while (jointIndex < pModel->numParts || jointIndex == 0xFFFF) {
        if (parentIndex == 0xFFFF) {
            pCurJoint->pParent = NULL;
        } else {
            pCurJoint->pParent = &pSkeleton[parentIndex] + 1;
        }
        
        pCurJoint->numChildren = i++;
        pCurJoint->doTransform = TRUE;
        pCurJoint->doRotate = TRUE;
        pCurJoint->unk7 = 1;
        pCurJoint->scale.vx = 0x1000;
        pCurJoint->scale.vy = 0x1000;
        pCurJoint->scale.vz = 0x1000;
        pCurJoint->rotDirection = 0;
        pCurJoint->scale.pad = 0;
        pCurJoint->jointIndex = jointIndex;
    
        if (jointIndex != 0xFFFF) {
            // Allocate model packets
            ModelAllocatePackets(pModel->pParts[jointIndex], &pCurJoint->pModelPacketBuffer, &pCurJoint->pCurModelPacket);
            if (pCurJoint->pModelPacketBuffer == NULL) {
                LibGearFreeModelSkeleton(pSkeleton);
                return NULL;
            }
    
            if (hasTexture) {
                ModelPacketSetBaseTexPage(texX, texY);
                ModelPacketSetBaseClut(clutX, clutY);
            }
    
            // Initialize model packets
            ModelInitializePacket(pModel->pParts[jointIndex], pCurJoint->pModelPacketBuffer, arg2);
            memcpy(pCurJoint->pCurModelPacket, pCurJoint->pModelPacketBuffer, pModel->pParts[jointIndex]->unk20[1].modelPacketSize);
        } else {
            pCurJoint->pModelPacketBuffer = NULL;
            pCurJoint->pCurModelPacket = NULL;
        }
    
        pCurJoint->rotation.vx = 0;
        pCurJoint->rotation.vy = 0;
        pCurJoint->rotation.vz = 0;
        pCurJoint->translation.x = 0;
        pCurJoint->translation.y = 0;
        pCurJoint->translation.z = 0;
        pCurJoint->unk70 = 0;
        pCurJoint->unk74 = 0;
        pCurJoint->unk78 = 0;
        
        pCurJoint++;
        pCurJointEntry++;
        
        jointIndex = pCurJointEntry->jointIndex;
        parentIndex = pCurJointEntry->parentIndex;
    }
    
    return pSkeleton;
}

// Computes the local and global transform matrices for the joints of a skeleton
unsigned LibGearComputeJointMatrices(ModelJoint* pJoint, int degrees) {
    ModelJoint* pSkeleton = pJoint;
    MATRIX* scratch = (MATRIX*)PSX_SCRATCH;
    unsigned i;
    unsigned numChildren;

    pJoint->globalTransform.t[0] = pJoint->translation.x;
    pJoint->globalTransform.t[1] = pJoint->translation.y;
    pJoint->globalTransform.t[2] = pJoint->translation.z;

    numChildren = pJoint->numChildren;

    if (pJoint->rotDirection != 0) {
        RotMatrixYXZ(&pJoint->rotation, &pJoint->globalTransform);
    } else {
        RotMatrix(&pJoint->rotation, &pJoint->globalTransform);
    }

    scratch->m[0][0] = (degrees * pJoint->scale.vx) >> 0xC;
    scratch->m[0][1] = 0;
    scratch->m[0][2] = 0;
    scratch->m[1][0] = 0;
    scratch->m[1][1] = (degrees * pJoint->scale.vy) >> 0xC;
    scratch->m[1][2] = 0;
    scratch->m[2][0] = 0;
    scratch->m[2][1] = 0;
    scratch->m[2][2] = (degrees * pJoint->scale.vz) >> 0xC;

    MulMatrix0(&pJoint->globalTransform, scratch, &pJoint->localTransform);

    pJoint->localTransform.t[0] = pJoint->globalTransform.t[0];
    pJoint->localTransform.t[1] = pJoint->globalTransform.t[1];
    pJoint->localTransform.t[2] = pJoint->globalTransform.t[2];

    for (i = 1; i < numChildren; i++) {
        pJoint++;

        if (pJoint->doRotate) {
            if (pJoint->rotDirection != 0) {
                RotMatrixYXZ(&pJoint->rotation, &pJoint->localTransform);
                pJoint->doRotate = FALSE;
            } else {
                RotMatrix(&pJoint->rotation, &pJoint->localTransform);
                pJoint->doRotate = FALSE;
            }
        }

        if ((pJoint->pParent != NULL) && (pJoint->pParent->doTransform == TRUE)) {
            pJoint->doTransform = TRUE;
        }

        if(pJoint->doTransform) {
            pJoint->localTransform.t[0] = pJoint->translation.x;
            pJoint->localTransform.t[1] = pJoint->translation.y;
            pJoint->localTransform.t[2] = pJoint->translation.z;

            if(pJoint->pParent != NULL) {
                CompMatrix(&pJoint->pParent->globalTransform, &pJoint->localTransform, &pJoint->globalTransform);
            } else {
                pJoint->globalTransform = pJoint->localTransform;
            }
        }
    }

    for(i = 1; i < numChildren; i++) {
        pSkeleton++;
        pSkeleton->doTransform = FALSE;
    }

    return numChildren;
}


INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801DC848);

void func_801DCC34(void) {}

void LibGearRenderModel(ModelMesh* pMesh, ModelJoint* pSkeleton, MATRIX* pLightMatrix, MATRIX* pViewMatrix, s32 arg4, s32 arg5, s32 packetIndex) {
    MATRIX* pRotMatrix = (MATRIX*)((u8*)PSX_SCRATCH + 0x20);
    MATRIX* pTransMatrix = (MATRIX*)((u8*)PSX_SCRATCH + 0x40);
    MATRIX* pMatrix = (MATRIX*)PSX_SCRATCH;
    u_int numChildren;
    u_int i;

    MulMatrix0(pViewMatrix, &pSkeleton->globalTransform, pRotMatrix);
    CompMatrix(pLightMatrix, &pSkeleton->localTransform, pTransMatrix);

    numChildren = pSkeleton->numChildren;
    pSkeleton++;

    for (i = 1; i < numChildren; i++) {
        if (pSkeleton->jointIndex != 0xFFFF) {
            ModelPart* pPart;
            void* pPacket;

            MulMatrix0(pRotMatrix, &pSkeleton->globalTransform, pMatrix);
            SetLightMatrix(pMatrix);
            CompMatrix(pTransMatrix, &pSkeleton->globalTransform, pMatrix);
            SetRotMatrix(pMatrix);
            SetTransMatrix(pMatrix);

            pPart = pMesh->pParts[pSkeleton->jointIndex];
            pPacket = ((void**)&pSkeleton->pModelPacketBuffer)[packetIndex];
            ModelRenderPacket(pPart, pPacket, arg5, arg4);
        }

        pSkeleton++;
    }
}

void LibGearFreeModelSkeleton(ModelJoint* pSkeleton) {
    int i;
    ModelJoint* pJoint = pSkeleton;

    if (pSkeleton != NULL) {
        for (i = 0; i < pSkeleton->numChildren; i++) {
            if (pJoint->pModelPacketBuffer != NULL) {
                HeapFree(pJoint->pModelPacketBuffer);
                pJoint->pModelPacketBuffer = NULL;
                pJoint->pCurModelPacket = NULL;
            }
            pJoint++;
        }
        pSkeleton->numChildren = 0;
        HeapFree(pSkeleton);
    }
}

void LibGearFreeModelMesh(ModelMesh* pMesh, s32 shouldFreeLightData) {
    int i;

    if (pMesh != NULL) {
        for (i = 0; i < pMesh->numParts; i++) {
            if (pMesh->pParts != NULL) {
                if (pMesh->pParts[i] && shouldFreeLightData) {
                    ModelPartFreeLightData(pMesh->pParts[i]);
                }
            }
        }
        
        if (pMesh->pParts != NULL) {
            HeapFree(pMesh->pParts);
            pMesh->pParts = NULL;
        }
    }
}

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801DCEC8);

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801DDBF8);

// objdiff 95.98%. Loop body and register anchors match the original
// instruction for instruction (a0 is the skeleton+4 cursor, a0 += 0x7C per
// joint, rotation compared with lh, translation with lw). Two prologue
// differences remain, both allocation: the original loads pBlock->unk6 first
// into t0 and keeps pKeyframe in a1 (the in-place `addiu a1,a1,0x18` reorg
// pulls into the bnez delay slot), where gcc 2.6 loads unk6 last (v0) and
// copies pBlock to a2 for pKeyframe; and the back edge loses the original's
// dead `andi v0,count` delay slot insn. Load order 0x6,0xC,0xE,0x4 (fixed by
// declaring numTransFrames before flags) still allocates flags to a1 and
// pKeyframe to a2. Also tried: s16 x/y/z locals (74.79%), gcc 2.7.2 (94.95%).
INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801DEF10);
/*
void func_801DEF10(ModelJoint* pSkeleton, KeyframeBlock* pBlock) {
    u16 rotFrame = 0;
    u16 transFrame = 0;
    s16 blockType = pBlock->unk6;
    u16 numRotFrames = pBlock->unkC;
    u16 numTransFrames = pBlock->unkE;
    u16 flags = pBlock->unk4;
    s16* pKeyframe = (s16*)(pBlock + 1);
    u16 count;
    int i;
    int x;
    int y;
    int z;
    ModelJoint* pJoint = pSkeleton;

    if (blockType == 0) {
        pKeyframe += (numRotFrames + 1) * 3;
    }

    count = pSkeleton->numChildren - 1;
    for (i = 0; i < count; i++) {
        pJoint++;

        if (!(flags & 0x1) && rotFrame < numRotFrames) {
            x = *pKeyframe++;
            y = *pKeyframe++;
            z = *pKeyframe++;
            rotFrame++;
            if (pJoint->rotation.vx != x || pJoint->rotation.vy != y || pJoint->rotation.vz != z) {
                if (pJoint->unk70 == 0 || ((Temp3*)pJoint->unk70)->unk3 != 0xFF) {
                    pJoint->rotation.vx = x;
                    pJoint->rotation.vy = y;
                    pJoint->rotation.vz = z;
                    pJoint->doTransform = 1;
                    pJoint->doRotate = 1;
                }
            }
        }

        if (!(flags & 0x2) && transFrame < numTransFrames) {
            x = *pKeyframe++;
            y = *pKeyframe++;
            z = *pKeyframe++;
            transFrame++;
            if (pJoint->translation.x != x || pJoint->translation.y != y || pJoint->translation.z != z) {
                if (pJoint->unk74 == 0 || ((Temp3*)pJoint->unk74)->unk3 != 0xFF) {
                    pJoint->translation.x = x;
                    pJoint->translation.y = y;
                    pJoint->translation.z = z;
                    pJoint->doTransform = 1;
                }
            }
        }
    }
}
*/

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801DF0B4);

void func_801DF52C(Temp4* pList, ModelJoint* pSkeleton, s32 index, s32 flags) {
    if (index < pSkeleton->numChildren) {
        pSkeleton += index;

        if (pSkeleton->unk70 != NULL && (flags & 0x1)) {
            func_801DF7A8(pList, (s8*)pSkeleton->unk70);
            pSkeleton->unk70 = 0;
        }

        if (pSkeleton->unk74 != NULL && (flags & 0x2)) {
            func_801DF7A8(pList, (s8*)pSkeleton->unk74);
            pSkeleton->unk74 = 0;
        }

        if (pSkeleton->unk78 != NULL && (flags & 0x4)) {
            func_801DF7A8(pList, (s8*)pSkeleton->unk78);
            pSkeleton->unk78 = 0;
        }
    }
}

void* func_801DF5F4(Temp4* pUnk, int numObjects) {
    if (numObjects <= 0) {
        return NULL;
    }

    pUnk->numObjects = numObjects;
    HeapChangeCurrentUser(HEAP_USER_MASA, HEAP_CONTENT_NULL);
    pUnk->pObjects = HeapAlloc(numObjects * sizeof(Temp3), 0x0);
    if (pUnk->pObjects) {
        func_801DF6A8(pUnk);
        return pUnk;
    }
    
    return NULL;
}

void func_801DF668(Temp4* pUnk) {
    pUnk->unk4 = 0;
    if (pUnk->pObjects != NULL) {
        HeapFree(pUnk->pObjects);
    }
    pUnk->pObjects = NULL;
}

void func_801DF6A8(Temp4* pUnk) {
    int i;
    Temp3* pCurObject;

    if (pUnk->pObjects == NULL) {
        return;
    }

    pCurObject = pUnk->pObjects;
    pUnk->unk4 = 0;

    for (i = 0; i < pUnk->numObjects; i++) {
        pCurObject->unk0 = 0;
        pCurObject++;
    }
}

Temp3* func_801DF6F0(Temp4* pList) {
    Temp3* pObject;

    if (pList->unk4 < pList->numObjects) {
        pObject = &pList->pObjects[pList->unk4];
        if (pObject->unk0 == 0) {
            for (
                pList->unk4++;
                pList->unk4 < pList->numObjects;
                pList->unk4++
            ) {
                if (pList->pObjects[pList->unk4].unk0 == 0) {
                    break;
                }
            }
            return pObject;
        }
    }

    return NULL;
}

int func_801DF7A8(Temp4* pUnk, s8* arg1) {
    int index;

    if (arg1 == NULL) {
        return -1;
    }
    
    index = ((mem_addr)arg1 - (mem_addr)pUnk->pObjects) / sizeof(Temp3);
    if (index < pUnk->unk4) {
        pUnk->unk4 = index;
    }
    *arg1 = 0;
    return index;
}

// Best attempt below: objdiff 70.90%, frame 0x48 (same as the original) and the
// whole loop body -- both channels, the NULL/0xFF paths, the release path, the
// 0xFFFF-rematerialised compares and the 6-byte-per-joint keyframe stride --
// emits the same instructions in the same order. What is left is the allocator:
//   * the original keeps pSkeleton/pJoint in s0 and pBlock/pKeyframe coalesced in
//     s1; here pBlock takes s0 (so pJoint reuses s0) and pKeyframe gets its own
//     s1, because the `(u16*)((u8*)pBlock + 0x18)` copy is not combined with the
//     dead parameter. `pBlock->keyframes` coalesces but grows the frame to 0x50.
//   * the original spills `count` to 0x10(sp) and holds 0xFF in s7; here `count`
//     wins s7 and the `type` parameter is copied to 0x10(sp) instead, so the
//     pObject->unk3 test compares against a register loaded inside the loop.
//   * the original reads pBlock->unk6 with `lhu` (unsigned); u16, s16 and a u16
//     cast on the s16 header field all still emit `lh`.
// Also tried: `pJoint = pSkeleton` as a declaration initialiser (68.76%), using
// pSkeleton as the joint cursor instead of a separate pJoint (68.76%), no
// `flag1` local with `flags & 0x1` inline (70.81%), declaration-order variations.
INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801DF7F4);
/*
s32 func_801DF7F4(Temp4* pList, ModelJoint* pSkeleton, KeyframeBlock* pBlock, s32 flags, u8 type) {
    u16 numRotFrames;
    u16 numChildren;
    u16 numTransFrames;
    u16 blockFlags;
    u16 count;
    u8 flag1;
    s16 frame;
    u16* pKeyframe;
    u8* pVertexData;
    Temp3* pObject;
    ModelJoint* pJoint;
    int i;

    if (pBlock->unk6 != 0) {
        func_801DFE8C(pList, pSkeleton);
        func_801DEF10(pSkeleton, pBlock);
        return 1;
    }

    numRotFrames = pBlock->unkC;
    numChildren = pSkeleton->numChildren;
    count = numChildren;
    numTransFrames = pBlock->unkE;
    blockFlags = pBlock->unk4;
    flag1 = flags & 0x1;

    if (numRotFrames + 1 < numChildren) {
        count = numRotFrames + 1;
    }

    frame = pBlock->unk2;
    if (flag1 == 0) {
        frame--;
    }

    pKeyframe = (u16*)((u8*)pBlock + 0x18);
    pVertexData = (u8*)pKeyframe + (numRotFrames + 1) * 6;
    if (!(blockFlags & 0x1)) {
        pVertexData += numRotFrames * 6;
    }
    if (!(blockFlags & 0x2)) {
        pVertexData += numTransFrames * 6;
    }

    pJoint = pSkeleton;
    for (i = 0; i < count; i++) {
        if (pKeyframe[0] != 0xFFFF) {
            pObject = (Temp3*)pJoint->unk70;
            if (pObject == NULL) {
                pObject = func_801DF6F0(pList);
            } else if (pObject->unk3 == 0xFF) {
                goto next1;
            }
            if (pObject != NULL) {
                pObject->unk0 = 1;
                pObject->unk1 = flag1;
                pObject->unk3 = type;
                pObject->unk2 = ((u8*)pKeyframe)[4];
                pObject->unk10 = 0;
                pObject->unk12 = frame;
                *(u32*)&pObject->unk4 = *(u32*)&pObject->unk8 = (u32)(pVertexData + pKeyframe[0]);
                pJoint->unk70 = (u32)pObject;
            }
        } else {
            pObject = (Temp3*)pJoint->unk70;
            if (pObject != NULL && i != 0 && pObject->unk3 != 0xFF) {
                func_801DF7A8(pList, (s8*)pObject);
                pJoint->unk70 = 0;
            }
        }
    next1:

        pKeyframe++;

        if (pKeyframe[0] != 0xFFFF) {
            pObject = (Temp3*)pJoint->unk74;
            if (pObject == NULL) {
                pObject = func_801DF6F0(pList);
            } else if (pObject->unk3 == 0xFF) {
                goto next2;
            }
            if (pObject != NULL) {
                pObject->unk0 = 1;
                pObject->unk1 = flag1;
                pObject->unk3 = type;
                pObject->unk2 = ((u8*)pKeyframe)[4];
                pObject->unk10 = 0;
                pObject->unk12 = frame;
                *(u32*)&pObject->unk4 = *(u32*)&pObject->unk8 = (u32)(pVertexData + pKeyframe[0]);
                pJoint->unk74 = (u32)pObject;
            }
        } else {
            pObject = (Temp3*)pJoint->unk74;
            if (pObject != NULL && i != 0 && pObject->unk3 != 0xFF) {
                func_801DF7A8(pList, (s8*)pObject);
                pJoint->unk74 = 0;
            }
        }
    next2:

        pKeyframe += 2;
        pJoint++;
    }

    return 0;
}
*/

// objdiff 75.14%. The instruction shapes of the whole 242-insn body are right:
// the two 6-byte-stride channels, the 0xFFFF merge into the else arm (the
// unk3==0xFF goto lands on the same code), blockFlags&1 / i!=0 / rotFrame<
// numRotFrames guards, pObject fill order (unk0,unk1,unk3,unk2,unk10,unk12,
// unk4=unk8=pVertexBase+pKeyframe[0]) and the 6-byte pKeyframe stride.
// Remaining delta is allocation only: the original keeps flag1 in $fp and the
// constant 1 in $a1 (spilled to 0x48(sp) around the two func_801DF6F0 calls),
// and reads the u8 `type` argument straight from its home slot 0x88(sp) each
// use; gcc here spills flag1 to a slot and copies `type` into a slot at entry,
// which costs the register that the other value needs. Also unmatched: the
// original spills `(u8*)&pKeyframe[2]` (addiu t0,s2,4 at the loop top, reloaded
// for both channels) where gcc uses 4(s2)/5(s2) immediates; and it allocates
// pSkeleton to s1 and pBlock to s2, ours is swapped. Declaration reordering
// (flag1 first) dropped the score to 75.10%.
INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801DFAC4);
/*
s32 func_801DFAC4(Temp4* pList, ModelJoint* pSkeleton, KeyframeBlock* pBlock, s32 flags, u8 type) {
    u16 numRotFrames;
    u16 numTransFrames;
    u16 count;
    u16 frame;
    u16 rotFrame;
    u16 transFrame;
    u16 blockFlags;
    s32 flag1;
    u16* pKeyframe;
    u16* pData;
    u8* pVertexBase;
    Temp3* pObject;
    ModelJoint* pJoint;
    s32 i;

    if (pBlock->unk6 != 0) {
        func_801DFE8C(pList, pSkeleton);
        func_801DEF10(pSkeleton, pBlock);
        return 1;
    }

    flag1 = flags & 0x1;
    numRotFrames = pBlock->unkC;
    rotFrame = 0;
    count = pSkeleton->numChildren;
    if (numRotFrames + 1 < count) {
        count = numRotFrames + 1;
    }
    numTransFrames = pBlock->unkE;
    transFrame = 0;

    frame = pBlock->unk2;
    if (flag1 == 0) {
        frame--;
    }

    blockFlags = pBlock->unk4;
    pKeyframe = (u16*)pBlock->keyframes;
    pData = pKeyframe + (numRotFrames + 1) * 3;
    pVertexBase = (u8*)pData;
    if (!(blockFlags & 0x1)) {
        pVertexBase += numRotFrames * 6;
    }
    if (!(blockFlags & 0x2)) {
        pVertexBase += numTransFrames * 6;
    }

    pJoint = pSkeleton;
    for (i = 0; i < count; i++) {
        if (pKeyframe[0] != 0xFFFF) {
            pObject = (Temp3*)pJoint->unk70;
            if (pObject != NULL) {
                if (pObject->unk3 == 0xFF) {
                    goto skipRot;
                }
            } else {
                pObject = func_801DF6F0(pList);
            }

            if (!(blockFlags & 0x1) && i != 0 && (u16)rotFrame < numRotFrames) {
                pJoint->rotation.vx = *pData++;
                pJoint->rotation.vy = *pData++;
                pJoint->rotation.vz = *pData++;
                rotFrame++;
                pJoint->doTransform = 1;
                pJoint->doRotate = 1;
            }

            if (pObject != NULL) {
                pObject->unk0 = 1;
                pObject->unk1 = flag1;
                pObject->unk3 = type;
                pObject->unk2 = ((u8*)&pKeyframe[2])[0];
                pObject->unk10 = 0;
                pObject->unk12 = frame;
                *(u32*)&pObject->unk4 = *(u32*)&pObject->unk8 = (u32)(pVertexBase + pKeyframe[0]);
                pJoint->unk70 = (u32)pObject;
            }
        } else {
        skipRot:
            if (!(blockFlags & 0x1) && i != 0 && (u16)rotFrame < numRotFrames) {
                pData += 3;
                rotFrame++;
            }
        }

        pKeyframe++;

        if (pKeyframe[0] != 0xFFFF) {
            pObject = (Temp3*)pJoint->unk74;
            if (pObject != NULL) {
                if (pObject->unk3 == 0xFF) {
                    goto skipTrans;
                }
            } else {
                pObject = func_801DF6F0(pList);
            }

            if (!(blockFlags & 0x2) && i != 0 && (u16)transFrame < numTransFrames) {
                pJoint->translation.x = *(s16*)pData++;
                pJoint->translation.y = *(s16*)pData++;
                pJoint->translation.z = *(s16*)pData++;
                transFrame++;
                pJoint->doTransform = 1;
            }

            if (pObject != NULL) {
                pObject->unk0 = 1;
                pObject->unk1 = flag1;
                pObject->unk3 = type;
                pObject->unk2 = ((u8*)&pKeyframe[2])[1];
                pObject->unk10 = 0;
                pObject->unk12 = frame;
                *(u32*)&pObject->unk4 = *(u32*)&pObject->unk8 = (u32)(pVertexBase + pKeyframe[0]);
                pJoint->unk74 = (u32)pObject;
            }
        } else {
        skipTrans:
            if (!(blockFlags & 0x2) && i != 0 && (u16)transFrame < numTransFrames) {
                pData += 3;
                transFrame++;
            }
        }

        pKeyframe += 2;
        pJoint++;
    }

    return 0;
}
*/

void func_801DFE8C(Temp4* pList, ModelJoint* pSkeleton) {
    int i;
    u16 numChildren = pSkeleton->numChildren;

    for (i = 0; i < numChildren; i++) {
        if (pSkeleton->unk70 != 0 && ((Temp3*)pSkeleton->unk70)->unk3 != 0xFF) {
            func_801DF7A8(pList, (s8*)pSkeleton->unk70);
            pSkeleton->unk70 = 0;
        }

        if (pSkeleton->unk74 != 0 && ((Temp3*)pSkeleton->unk74)->unk3 != 0xFF) {
            func_801DF7A8(pList, (s8*)pSkeleton->unk74);
            pSkeleton->unk74 = 0;
        }

        if (pSkeleton->unk78 != 0 && ((Temp3*)pSkeleton->unk78)->unk3 != 0xFF) {
            func_801DF7A8(pList, (s8*)pSkeleton->unk78);
            pSkeleton->unk78 = 0;
        }

        pSkeleton++;
    }
}

void func_801DFF78(Temp4* pList, ModelJoint* pSkeleton, u8 type) {
    int i;
    u16 numChildren = pSkeleton->numChildren;

    for (i = 0; i < numChildren; i++) {
        if (pSkeleton->unk70 != 0 && ((Temp3*)pSkeleton->unk70)->unk3 == type) {
            func_801DF7A8(pList, (s8*)pSkeleton->unk70);
            pSkeleton->unk70 = 0;
        }

        if (pSkeleton->unk74 != 0 && ((Temp3*)pSkeleton->unk74)->unk3 == type) {
            func_801DF7A8(pList, (s8*)pSkeleton->unk74);
            pSkeleton->unk74 = 0;
        }

        if (pSkeleton->unk78 != 0 && ((Temp3*)pSkeleton->unk78)->unk3 == type) {
            func_801DF7A8(pList, (s8*)pSkeleton->unk78);
            pSkeleton->unk78 = 0;
        }

        pSkeleton++;
    }
}

void* LibGearInitializeVertexBuffer(VertexBuffer* pVertexBuffer, u32 capacity) {
    HeapChangeCurrentUser(HEAP_USER_MASA, HEAP_CONTENT_NULL);
    pVertexBuffer->capacity = capacity;
    pVertexBuffer->curIndex = 0;
    pVertexBuffer->pVertices = HeapAlloc((capacity + 1) * sizeof(Vertex), 0x0);
    
    if (pVertexBuffer->pVertices) {
        LibGearInitializeVertices(pVertexBuffer);
        return pVertexBuffer;
    }
    
    return NULL;
}

void LibGearFreeVertexBuffer(VertexBuffer* pVertexBuffer) {
    pVertexBuffer->capacity = 0;
    pVertexBuffer->curIndex = 0;
    if (pVertexBuffer->pVertices != NULL) {
        HeapFree(pVertexBuffer->pVertices);
    }
    pVertexBuffer->pVertices = NULL;
}

void LibGearInitializeVertices(VertexBuffer* pVertexBuffer) {
    int i;
    int j;
    Vertex* pVertex;

    pVertex = pVertexBuffer->pVertices;
    for (i = 0; i < pVertexBuffer->capacity + 1; i++) {
        pVertex->unk16 = -1;
        pVertex->unk1E = 0;
        
        for (j = 0; j < 2; j++) {
            SetPolyFT4(&pVertex->polys[j]);
            SetSemiTrans(&pVertex->polys[j], 1);
            
            pVertex->polys[j].clut = GetClut(0, 0x1CD);
            pVertex->polys[j].tpage = GetTPage(0, 1, 0x340, 0x100);

            setUV4(
                &pVertex->polys[j],
                0x0, 0xBD,
                0x0, 0xBD,
                0xF, 0xBD,
                0xF, 0xBD
            );
        }

        pVertex++;
    }
}

void* func_801E0248(VertexBuffer* pVertexBuffer, short isSemiTrans) {
    Vertex* pVertex;
    
    if (pVertexBuffer->curIndex < pVertexBuffer->capacity) {
        pVertex = &pVertexBuffer->pVertices[pVertexBuffer->curIndex];
        if (pVertex->unk16 == -1) {
            for (
                pVertexBuffer->curIndex++; 
                pVertexBuffer->curIndex < pVertexBuffer->capacity; 
                pVertexBuffer->curIndex++
            ) {
                if (pVertexBuffer->pVertices[pVertexBuffer->curIndex].unk16 == -1) {
                    break;
                }
            }
            
            SetSemiTrans(&pVertex->polys[0], isSemiTrans);
            SetSemiTrans(&pVertex->polys[1], isSemiTrans);
            return pVertex;
        }
    }
    
    return &pVertexBuffer->pVertices[pVertexBuffer->capacity];
}

int func_801E0354(VertexBuffer* pVertexBuffer, Vertex* pObject) {
    int index;

    index = ((mem_addr)pObject - (mem_addr)pVertexBuffer->pVertices) / sizeof(Vertex);
    if (pVertexBuffer->curIndex >= index) {
        pVertexBuffer->curIndex = index;
    }
    
    pObject->unk16 = -1;
    return index;
}

void func_801E0398(VertexBuffer* pVertexBuffer, MATRIX* pMatrix, s32 arg2, s32* pOT, s32 renderCtx) {
    long otz;
    int i;
    Vertex* pVertex;

    SetRotMatrix(pMatrix);
    SetTransMatrix(pMatrix);
    
    pVertex = pVertexBuffer->pVertices;
    for (i = 0; i < pVertexBuffer->capacity; i++, pVertex++) {
        if (pVertex->unk16 == -1) {
            continue;
        }
        
        if (pVertex->unk16 >= pVertex->unk1E) {
            func_801E0354(pVertexBuffer, pVertex);
            continue;
        }

        // Does not match w/ setRGB0 macro
        pVertex->polys[renderCtx].r0 = pVertex->red >> 6;
        pVertex->polys[renderCtx].g0 = pVertex->green >> 6;
        pVertex->polys[renderCtx].b0 = pVertex->blue >> 6;
        
        if (pVertex->unkE == 0) {
            setXY4(
                &pVertex->polys[renderCtx],
                pVertex->x0, pVertex->y0,
                pVertex->x1, pVertex->y1,
                pVertex->x2, pVertex->y2,
                pVertex->x3, pVertex->y3
            );
            addPrim(pOT, &pVertex->polys[renderCtx]);
        } else {
            // RotTransPers3
            gte_ldv3(
                (SVECTOR*) &pVertex->x0,
                (SVECTOR*) &pVertex->x1,
                (SVECTOR*) &pVertex->x2
            );
            gte_rtpt();
            gte_stsxy3(
                (long*) &pVertex->polys[renderCtx].x0,
                (long*) &pVertex->polys[renderCtx].x1,
                (long*) &pVertex->polys[renderCtx].x2
            );
            gte_stszotz(&otz);
            otz >>= D_80050100;

            // RotTransPers
            gte_ldv0((SVECTOR*) &pVertex->x3);
            gte_rtps();
            gte_stsxy((long*) &pVertex->polys[renderCtx].x3);

            addPrim(&pOT[otz], &pVertex->polys[renderCtx]);
        }
        
        pVertex->unk16 += arg2;
        pVertex->red -= pVertex->redDelta * arg2;
        pVertex->green -= pVertex->greenDelta * arg2;
        pVertex->blue -= pVertex->blueDelta * arg2;
    }
}

s32 func_801E0698(UnkInner1* pEntry, u32 arg1, u16 arg2, u8 arg3, s16 numFrames, u16 divisor, u8 arg6, u8 arg7, u8 arg8, u8 arg9, u8 arg10, u8 arg11, u16 arg12, u16 arg13, u16 arg14, u16 arg15, u16 arg16, u16 arg17, u16 arg18) {
    s16 div;

    if (pEntry != NULL) {
        pEntry->unk3 = arg18;
        pEntry->unk5E = -1;
        pEntry->unk0 = arg2;
        pEntry->unk2 = arg3;
        pEntry->unk4 = arg1;
        pEntry->unkC = arg12;
        pEntry->unkE = arg13;
        pEntry->unk10 = arg14;
        pEntry->unk14 = arg15;
        pEntry->unk16 = arg16;
        pEntry->unk18 = arg17;
        pEntry->unk5C = 0;
        if (numFrames < 7) {
            pEntry->unk60 = numFrames;
        } else {
            pEntry->unk60 = 7;
        }

        pEntry->unk64 = arg6 << 6;
        pEntry->unk66 = arg7 << 6;
        pEntry->unk68 = arg8 << 6;
        div = (s16)divisor;
        pEntry->unk6A = (pEntry->unk64 - (arg9 << 6)) / div;
        pEntry->unk6C = (pEntry->unk66 - (arg10 << 6)) / div;
        pEntry->unk6E = (pEntry->unk68 - (arg11 << 6)) / div;

        pEntry->unk62 = divisor;
        pEntry->unk8 = 0;

        return 0;
    }
}

void func_801E0844(short* pValue) {
    *pValue = -1;
}

s16 func_801E0850(s16 angle, s16 divisor, s16 offset) {
    return offset + (rsin(angle) + 0x1000) / divisor;
}

s16 func_801E08D4(s16 a, s16 b, s16 c) {
    s16 result = c + a / b;

    if (result > 0x20) {
        return -1;
    } else {
        return result;
    }
}

s16 func_801E0938(s16 a, s16 b, s16 c) {
    return c - a / b;
}

s16 func_801E0988(s16 a, s16 b, s16 c) {
    s16 result = 0x20 - a / b;

    if (result < c) {
        result = c;
    }

    return result;
}

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E0A00);

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E1258);

void func_801E165C(Temp8* pTexture) {
    if (pTexture->unk1A != 0) {
        if (pTexture->pUnk4 != NULL) {
            if (pTexture->unk10 < 4) {
                LoadImage(&pTexture->rect, (u_long*)pTexture->pUnk4);
            }
            HeapFree(pTexture->pUnk4);
            pTexture->pUnk4 = NULL;
        }

        if (pTexture->pUnk8 != NULL) {
            HeapFree(pTexture->pUnk8);
            pTexture->pUnk8 = NULL;
        }

        if (pTexture->pUnkC != NULL) {
            HeapFree(pTexture->pUnkC);
            pTexture->pUnkC = NULL;
        }

        pTexture->unk1A = 0;
    }
}

void func_801E1708(Temp8* pTexture, s16 scale) {
    u16* pSrc = pTexture->pUnk4;
    s16* pDst;
    int y;
    int x;

    for (y = 0; y < pTexture->rect.h; y++) {
        for (x = 0; x < pTexture->rect.w; x++) {
            pDst = (s16*)pTexture->pUnk1C + 3 * (pTexture->rect.y + y);
            pDst[pTexture->rect.x + x] = (*pSrc * scale) / 0x20;
            pSrc++;
        }
    }
}

void func_801E17B8(Temp8* pTexture, s16 scale) {
    u16* pSrc = pTexture->pUnk4;
    u16* pDelta = pTexture->pUnk8;
    s16* pDst;
    int y;
    int x;

    for (y = 0; y < pTexture->rect.h; y++) {
        for (x = 0; x < pTexture->rect.w; x++) {
            pDst = (s16*)pTexture->pUnk1C + 3 * (pTexture->rect.y + y);
            pDst[pTexture->rect.x + x] = *pDelta + (*pSrc - *pDelta) * scale / 0x20;
            pSrc++;
            pDelta++;
        }
    }
}

// Rotates each entry's source position by the model's joint matrix
void func_801E1880(Model** pModels) {
    MATRIX* pMatrix = (MATRIX*)PSX_SCRATCH;
    Model* pModel;
    u_short value[6];
    int i;

    for (i = 0; i < 2; i++) {
        if (D_801E8648[i].unk6 != 0) {
            if (D_801E8648[i].modelIndex >= 0 && (pModel = pModels[D_801E8648[i].modelIndex]) != NULL) {
                CompMatrix(
                    &pModel->pSkeleton[0].localTransform,
                    &pModel->pSkeleton[D_801E8648[i].jointIndex + 1].globalTransform,
                    pMatrix
                );
                SetRotMatrix(pMatrix);
                SetTransMatrix(pMatrix);

                gte_ldv0((SVECTOR*)&D_801E8648[i].x1);
                gte_mvmva(1, 0, 0, 0, 0);
                gte_stlvnl(value);

                D_801E8648[i].x0 = value[0];
                D_801E8648[i].y0 = value[2];
                D_801E8648[i].z0 = value[4];
            } else {
                D_801E8648[i].x0 = D_801E8648[i].x1;
                D_801E8648[i].y0 = D_801E8648[i].y1;
                D_801E8648[i].z0 = D_801E8648[i].z1;
            }
        }
    }
}


INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E1A14);

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E22F8);

void func_801E3438(Temp9* pEntry) {
    if (pEntry->unk14 != NULL) {
        HeapFree(pEntry->unk14);
        HeapFree(pEntry->unk1C[0]);
        HeapFree(pEntry->unk1C);
        HeapFree(pEntry->unk20);

        if (pEntry->unk18 != NULL) {
            HeapFree(pEntry->unk18);
        }

        pEntry->unk14 = NULL;
    }
}

s16 (*func_801E34BC(s32 type))(s16, s16, s16) {
    switch (type) {
        case 1:
            return func_801E08D4;
        case 2:
            return func_801E0938;
        case 3:
            return func_801E0988;
        default:
            return func_801E0850;
    }
}

// objdiff 94.87%: 39 insns, exact instruction multiset, one store slot apart.
// Both `li v0,1` (for unk70[15], insn 7) and the whole body match, so the locals
// negOne = -1 and one = 1 are confirmed. The only delta is scheduling: the
// original keeps `li v1,-1` at insn 0 and its first user `sh v1,0x98` at insn 14.
// With `pModel->unk98 = negOne;` first the li sits at 0 but the store is deferred
// to 8; with the store in its logical place (after unk2B) the store lands at 14
// but the li is hoisted to 6 (74.36%). Declaration order of the two locals, a
// mid-block `s32 one = 1;` and bare/s16/u32 -1 forms all stay at 74.36%.
INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E3534);
/*
void func_801E3534(Model* pModel, s32 arg1, u32 arg2, u32 arg3) {
    s32 negOne = -1;
    s32 one = 1;

    pModel->unk98 = negOne;
    pModel->unk3C = 0xFFFF;
    pModel->unk5C = 0xFF;
    pModel->unk39 = 0x6B;

    pModel->unk8 = arg2;
    pModel->unkC = 0;
    pModel->unk10 = 0;
    pModel->unk14 = (Temp7**)arg3;
    pModel->unk18 = NULL;

    pModel->unk2B = 0;
    pModel->unk58 = 0;

    pModel->unk35 = 0;
    pModel->unk37 = 0;
    pModel->unk38 = 0;
    pModel->unk3A = negOne;

    pModel->unk70[0] = 0;
    pModel->unk70[1] = 0;
    pModel->unk70[2] = 0;
    pModel->unk70[3] = 0;
    pModel->unk70[4] = 0;
    pModel->unk70[5] = 0;
    pModel->unk70[6] = 0;
    pModel->unk70[7] = 0;
    pModel->unk70[8] = 0;
    pModel->unk70[9] = 0;
    pModel->unk70[10] = 0;
    pModel->unk70[11] = 0;
    pModel->unk70[12] = 0;
    pModel->unk70[13] = 0;
    pModel->unk70[14] = 0;
    pModel->unk70[15] = one;

    pModel->unk36 = 0;
    pModel->unk1E = negOne;
}
*/

void func_801E35D0(Model* pModel, Model* pSource, s32 arg2, s32 arg3) {
    if ((pModel != NULL) && (pSource != NULL)) {
        if (pModel->unk2B != 0) {
            if (pModel->unk2B < 5) {
                pModel->unk2B++;
            }

            *(u8*)((u32)pModel + pModel->unk2B + 0x2A) = pSource->unk20;
            *(u8*)((u32)pModel + pModel->unk2B + 0x2E) = arg3;
        } else {
            if (arg3 < 0x50) {
                pModel->unk10 = ((u32*)pSource->unk8)[arg3];
            } else {
                pModel->unk10 = ((u32*)((Temp8*)pSource->unkC)->pUnk4)[arg3 - 0x4E];
            }

            pModel->unk42 = 0;
            pModel->unk40 = 0;
            pModel->unk50 = 0;
            pModel->unk54 = 0;
            pModel->unk4C = 0;
            pModel->unk10A = D_801E863C;
            pModel->unk23 = 0;
            func_801E39F0(pModel, arg2, -1, 1, 0);
        }
    }
}

s32 func_801E36BC(Model* pModel, s32 arg1, s32 count, s32 arg3, s32 arg4) {
    int i;
    s32 result;

    if (pModel->pMesh != NULL) {
        result = 0;

        if (pModel->unk34) {
            func_801E7298(pModel);

            if (pModel->unk37) {
                func_801DC848(pModel->pSkeleton, pModel->scale);
            } else {
                LibGearComputeJointMatrices(pModel->pSkeleton, pModel->scale);
            }

            for (i = 0; i < count; i++) {
                result |= func_801DDBF8(arg1, pModel->pSkeleton, pModel->unk3C, pModel->scale);
                func_801E5D44(pModel, arg1, arg3);
            }
        }

        func_801E39F0(pModel, arg1, result, count, arg4);
    }

    return result;
}

void func_801E37D0(Model* pModel) {
    MATRIX* pMatrix = (MATRIX*)PSX_SCRATCH;
    SVECTOR vec;

    if (g_LibGearModels[pModel->unk5C] != NULL) {
        if (!(pModel->unk4A & 0x10)) {
            pModel->unk34 = g_LibGearModels[pModel->unk5C]->unk34;
        }

        if (pModel->unk34 == 0) {
            return;
        }

        if (pModel->unk5D != 0) {
            if (pModel->unk5E != 0) {
                MulMatrix0(
                    &g_LibGearModels[pModel->unk5C]->pSkeleton[0].globalTransform,
                    &g_LibGearModels[pModel->unk5C]->pSkeleton[pModel->unk5E].globalTransform,
                    (MATRIX*)PSX_SCRATCH
                );
            } else {
                pMatrix = &g_LibGearModels[pModel->unk5C]->pSkeleton[0].globalTransform;
            }

            MulMatrix2(pMatrix, &pModel->pSkeleton[0].localTransform);
            MulMatrix2(pMatrix, &pModel->pSkeleton[0].globalTransform);
        }

        if (pModel->unk5E != 0) {
            CompMatrix(
                &g_LibGearModels[pModel->unk5C]->pSkeleton[0].localTransform,
                &g_LibGearModels[pModel->unk5C]->pSkeleton[pModel->unk5E].globalTransform,
                pMatrix
            );
        } else {
            pMatrix = &g_LibGearModels[pModel->unk5C]->pSkeleton[0].localTransform;
        }

        SetRotMatrix(pMatrix);
        SetTransMatrix(pMatrix);

        vec.vx = *(u16*)&pModel->unk62[8];
        vec.vy = *(u16*)&pModel->unk62[10];
        vec.vz = *(u16*)&pModel->unk62[12];

        gte_ldv0(&vec);
        gte_mvmva(1, 0, 0, 0, 0);
        gte_stlvnl(&pModel->pSkeleton[0].localTransform.t[0]);
        gte_stlvnl(&pModel->pSkeleton[0].translation.x);
    } else {
        pModel->unk5C = 0xFF;
    }
}

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E39F0);

void func_801E59D4(Temp4* pList, ModelJoint* pJoint, s32 numFrames, s32 x, s32 y, s32 z) {
    Temp3* pObject;

    if (numFrames < 2) {
        pJoint->rotation.vx = x;
        pJoint->rotation.vy = y;
        pJoint->rotation.vz = z;
        pJoint->doRotate = TRUE;
        return;
    }

    if (pJoint->rotation.vx == x && pJoint->rotation.vy == y && pJoint->rotation.vz == z) {
        return;
    }

    if (pJoint->unk70 == NULL) {
        pObject = func_801DF6F0(pList);
    } else {
        pObject = (Temp3*)pJoint->unk70;
    }

    if (pObject != NULL) {
        pObject->unk0 = 1;
        pObject->unk1 = 0;
        pObject->unk2 = 3;
        pObject->unk3 = 0xFE;

        pObject->unk4 = pJoint->rotation.vx;
        pObject->unk6 = pJoint->rotation.vy;
        pObject->unk8 = pJoint->rotation.vz;

        x = (x - pJoint->rotation.vx) & 0xFFF;
        if (x >= 0x800) {
            x -= 0x1000;
        }
        pObject->unkA = x;

        y = (y - pJoint->rotation.vy) & 0xFFF;
        if (y >= 0x800) {
            y -= 0x1000;
        }
        pObject->unkC = y;

        z = (z - pJoint->rotation.vz) & 0xFFF;
        if (z >= 0x800) {
            z -= 0x1000;
        }
        pObject->unkE = z;

        pObject->unk10 = 0;
        pObject->unk12 = numFrames;
        pJoint->unk70 = (u32)pObject;
    }
}

void func_801E5B50(Temp4* pList, ModelJoint* pJoint, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 x, s32 y, s32 z) {
    Temp3* pObject;
    int dx;
    int dy;
    int dz;

    if (pJoint->unk70 == NULL) {
        pObject = func_801DF6F0(pList);
    } else {
        pObject = (Temp3*)pJoint->unk70;
    }

    if (pObject != NULL) {
        pObject->unk0 = 1;
        pObject->unk1 = 0;
        pObject->unk2 = arg2 + 7;
        pObject->unk3 = 0xFE;

        dx = x - pJoint->translation.x;
        dy = y - pJoint->translation.y;
        dz = z - pJoint->translation.z;

        pObject->unk4 = SquareRoot0(dx * dx + dy * dy + dz * dz) + 1;
        pObject->unk6 = arg3;
        pObject->unk8 = arg4;
        pObject->unkA = x;
        pObject->unkC = y;
        pObject->unkE = z;
        pObject->unk10 = 0;
        pObject->unk12 = arg5;

        pJoint->unk70 = (u32)pObject;
    }
}

void func_801E5C74(Model* pModel, Temp7* pEntry, s32 arg2) {
    if (pEntry->unk12 != 0) {
        pModel->unk98 = 0;
        if (arg2 != 0) {
            pModel->unk9A = pEntry->unk2;
        } else {
            pModel->unk9A = -1;
        }
        pModel->unk9C = 0;
        pModel->unk9E = pEntry->unk12;
        pModel->unkA0 = (u8*)pEntry + pEntry->unk14;
        pModel->unkA4 = pModel->unkA0;
    } else {
        pModel->unk98 = -1;
    }
}

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E5CD8);
/* Best attempt, 71.67%. The three temp8 hops are confirmed. What the original
   does and this does not: type==1 gets its OWN copy of the `lhu 0x14 / sll 16`
   tail (out of line, last, falling into the epilogue) while type==0 and type==2
   share one copy. Giving the type==1 arm its own `return` produces the second
   copy but gcc then emits type==1 inline (61.93%); with the `if (type != 1) ...
   else return ...` spelling below gcc shares a *deeper* tail instead
   (`lw 8(v0) / lhu 20 / sll` shared by type==1 and type==2, 88 vs 108 bytes), so
   the split point and the block placement are one decision. A `switch` on type
   is far off (19.74%, case bodies emitted in source order with break jumps).
   Later round: the plain `if/else if` chain that assigns a single `pTexture`
   local and then has ONE `return pTexture->unk14 << 16` is also 61.93%
   (88 bytes vs the original's 108): gcc cross-jumps the type==1 and type==2
   tails and hoists them, so the pair that merges is (1,2) instead of the
   original's (0,2). Which of the three identical `lhu 0x14 / sll 16` tails
   survives is the whole function. */
/*
s32 func_801E5CD8(Model* pModel, s32 type) {
    Temp8* pTexture;

    if (type == 0) {
        pTexture = D_8005919C;
    } else if (type != 1) {
        if (type != 2) {
            return 2;
        }
        pTexture = ((Temp8*)pModel->unkB4)->pUnk8;
    } else {
        return ((Temp8*)((Temp8*)pModel->unkB0)->pUnk8)->unk14 << 16;
    }

    return pTexture->unk14 << 16;
}
*/

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E5D44);

void func_801E632C(Model* pModel) {
    pModel->unk98 = -1;
}

int func_801E6338(Model* pModel) {
    int dx = pModel->unk70[0xC] - pModel->pSkeleton->translation.x;
    int dy = pModel->unk70[0xD] - pModel->pSkeleton->translation.y;
    int dz = pModel->unk70[0xE] - pModel->pSkeleton->translation.z;

    return SquareRoot0(dx * dx + dy * dy + dz * dz);
}

// 95.39% with the header's Model::unk5A now s16 (it scored 93.15% while unk5A was
// u8[2], which made the attempt below test the field's address instead of loading
// it). What already reproduces exactly: the reload of
// unk58 after the bit-scan loop, the sequential (not else-if) test chain on
// 0xFE/0xFD/0xFC/0xFA, the CompMatrix / SetRotMatrix / gte_ldv0 + gte_mvmva body
// and the 0x88 tail, including the 0x38 frame (which needs a 24-byte GTE result
// buffer, hence value[12], not value[6]).
// Remaining delta: the original computes `type - 1` TWICE from two different
// registers -- `move a1,v1` early, then `addiu v0,a1,-1` for the range test and
// `addiu a0,v1,-1` for the stored index -- so its two operands are distinct
// pseudos. Every single-local spelling tried (s16/s32/u32 local, repeated
// `pModel->unk58` reads, a separate `rawType` copy) makes gcc 2.6 CSE them into
// one addiu, which costs one instruction and shifts every later label.
INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E63A8);
/* Best attempt, 93.15%:
void func_801E63A8(Model* pModel) {
    Model* pSource;
    MATRIX* pMatrix;
    u_short value[12];
    s32 index;
    s32 jointIndex;
    Temp3* pObject;

    index = 0;
    if (pModel->unk58 == 0xFF) {
        for (index = 0; index < 8; index++) {
            if ((pModel->unk10A >> index) & 0x1) {
                break;
            }
        }
    }

    if (pModel->unk58 == 0xFE) {
        index = D_801E86B0;
    }
    if (pModel->unk58 == 0xFD) {
        index = pModel->unk20;
    }
    if (pModel->unk58 == 0xFC) {
        index = pModel->unk21;
    }
    if (pModel->unk58 == 0xFA) {
        index = 0xA;
    }
    if ((u32)(pModel->unk58 - 1) < 0x7F) {
        index = pModel->unk58 - 1;
    }

    pSource = g_LibGearModels[index];
    if (pSource == NULL) {
        return;
    }

    if (index == pModel->unk20) {
        return;
    }

    jointIndex = pModel->unk5A;
    pMatrix = (MATRIX*)0x1F800000;

    if (jointIndex != 0) {
        CompMatrix(
            &pSource->pSkeleton[0].localTransform,
            &pSource->pSkeleton[jointIndex].globalTransform,
            pMatrix
        );
    } else {
        pMatrix = &pSource->pSkeleton[0].localTransform;
    }

    SetRotMatrix(pMatrix);
    SetTransMatrix(pMatrix);

    gte_ldv0((SVECTOR*)&pModel->unk62[2]);
    gte_mvmva(1, 0, 0, 0, 0);
    gte_stlvnl(value);

    pModel->unk70[12] = value[0];
    pModel->unk70[13] = value[2];
    pModel->unk70[14] = value[4];

    pObject = (Temp3*)pModel->pSkeleton->unk70;
    if (pObject != NULL) {
        if ((u32)(pObject->unk2 - 7) < 2) {
            pObject->unkA = value[0];
            pObject->unkC = value[2];
            pObject->unkE = value[4];
        }
    }
}
*/


// objdiff 98.63%: byte-identical except that gcc 2.6 puts pJoint in s1 and the
// loop counter i in s0; the original has pJoint in s0, i in s1 (the other five
// callee-saved assignments -- pList s3, pSkeleton s4, pChild s2, pTargetSkeleton
// s5, numChildren s6 -- do match). Declaration orders, for/while, u_short
// numChildren, moving pChild++ to the loop tail, an explicit `s32 offset` local,
// `u_short i` (adds 8 bytes, order becomes pJoint/pChild/i), hoisting `i = 1`
// into the declarations (same), `u32 i` (96.80%), `register int i` and
// `register ModelJoint* pJoint` (both same) all leave the pair alone or score
// lower. `pSkeleton + jointIndex` instead of the s32 cast gives 98.30%: it
// swaps the addu operands to `addu s1,s4,v0` without touching s0/s1. Since both
// builds emit the same insns in the same order, the choice comes from the
// pseudo order/ties local-alloc resolves, not from anything in the body.
// Also tried since: `int i;` declared first with the pointers after (same swap,
// pJoint still s1), and reversing the pointer declaration order (same).
// A `while (i < numChildren) { i++; if (...) {...} pChild++; }` body (i = 1 before
// the three func_801DF7A8 calls) reproduces the original's instruction sequence
// exactly -- i++ in the bne delay slot, pChild++ in the loop-back delay slot, and
// `li` in the first jal's delay slot -- so the loop shape is settled; it scores
// 96.10% because the allocator then swaps BOTH pairs (pJoint s1/i s0 and pSkeleton
// s5/pTargetSkeleton s4). pTarget as `pTargetSkeleton + jointIndex`, no pTarget
// local at all, and gcc 2.7.2 (try_flags.sh) all leave that map unchanged, so the
// remaining gap looks like an allocno tie-break (allocno_compare returns 0 and the
// original host's qsort ordered ties differently), not a source-level detail.
// Also tried since (all still 98.63%, same s0/s1 swap): pChild's initialiser
// moved above pJoint's, and `pChild = pChild + 1` in the loop tail.
INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E6578);
/*
void func_801E6578(Temp4* pList, s32 jointIndex, ModelJoint* pSkeleton, ModelJoint* pTargetSkeleton) {
    int i;
    int numChildren;
    ModelJoint* pChild = pSkeleton;
    ModelJoint* pJoint = (ModelJoint*)(jointIndex * sizeof(ModelJoint) + (s32)pSkeleton);
    ModelJoint* pTarget = (ModelJoint*)(jointIndex * sizeof(ModelJoint) + (s32)pTargetSkeleton);

    numChildren = pChild->numChildren;
    pJoint->unk7 = 0;
    pTarget->unk7 = 1;

    func_801DF7A8(pList, (s8*)pJoint->unk70);
    pJoint->unk70 = 0;
    func_801DF7A8(pList, (s8*)pJoint->unk74);
    pJoint->unk74 = 0;
    func_801DF7A8(pList, (s8*)pJoint->unk78);
    pJoint->unk78 = 0;

    for (i = 1; i < numChildren; i++) {
        pChild++;

        if (pChild->pParent == pJoint) {
            func_801E6578(pList, pChild->numChildren, pSkeleton, pTargetSkeleton);
        }
    }
}
*/

void func_801E6668(ModelJoint* pSkeleton, ModelJoint* pTargetSkeleton) {
    int i;
    u_short numChildren = pSkeleton->numChildren;

    for (i = 1; i < numChildren; i++) {
        pSkeleton++;
        pTargetSkeleton++;

        if (pSkeleton->unk7 != 0) {
            pSkeleton->unk7 = 0;
            pTargetSkeleton->unk7 = 1;
        }
    }
}

s16 func_801E66BC(VECTOR* pNormal, VECTOR* pVec0, VECTOR* pVec1, s32 divisor) {
    VECTOR outer;
    s32 dot;
    s32 length;

    OuterProduct12(pVec0, pVec1, &outer);
    dot = outer.vx * pNormal->vx + outer.vy * pNormal->vy + outer.vz * pNormal->vz;
    length = SquareRoot0(outer.vx * outer.vx + outer.vy * outer.vy + outer.vz * outer.vz) + 1;
    return ((dot << 4) / length << 8) / divisor;
}

int func_801E67F8(void) {
    u_short bits = D_801E863C;
    int i;

    for (i = 0; i < 8; i++) {
        if ((bits >> i) & 0x1) {
            break;
        }
    }

    return i;
}

// objdiff 92.95%: the only delta is one `andi v0,a1,0xff` narrowing the merged
// index before `li v1,1; sllv` (the original's jump-delay slots then hold that
// andi; ours hold a copy of the `li`). gcc 2.6 drops the mask in every
// formulation tried — u8 index, s32 index with `& 0xFF`, s32 index with a
// `(u8)` cast, and a separate `s32 n = index` temporary — because the shift
// count only needs its low 5 bits, so the narrowing never survives.
// objdiff 92.95%: one `andi v0,a1,0xff` short. gcc 2.6 keeps the merged u8 index in
// SImode (jump-delay slots then hold a copy of `li v1,1`), the original truncates
// it once at the merge. Reassigning a `u8 index = type` local in each branch (this
// shape) reproduces the rest exactly, including the `andi v1,a1,0xff` at the top
// and the index living in a1; u8 param, s32 index with `& 0xFF`, a `(u8)` cast and
// an `s32 n = index` temporary all leave the same delta, because the mask only
// feeds a shift count.
INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E6830);
/*
void func_801E6830(Model* pModel, int type, s16* pMask) {
    u8 index = type;

    if (index == 0xFF) {
        index = func_801E67F8();
    } else if (index == 0xFE) {
        index = *(u8*)&D_801E86B0;
    } else if (index == 0xFD || index == 0xF9) {
        index = pModel->unk20;
    } else if (index == 0xFC) {
        index = pModel->unk21;
    } else if (index == 0xFA) {
        index = 0xA;
    } else if (index == 0xF8) {
        index = pModel->unk20 * 2 + 8;
    } else if (index == 0xF7) {
        index = pModel->unk20 * 2 + 9;
    }

    *pMask = 1 << index;
}

*/

void* func_801E6910(Model* pModel, u8 index, s32* pFlags) {
    void* pResult;

    *pFlags = 0;
    if (index >= 0xFE) {
        u8 entry = pModel->unk2A;

        index = entry & 0x7F;
        *pFlags = entry & 0x80;
    }

    if (index >= 0x40) {
        pResult = pModel->unk18[index - 0x3F];
    } else {
        pResult = pModel->unk14[index + 1];
    }

    return pResult;
}

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E6974);

void func_801E6D94(Model* pModel, ModelJoint* pJoint, s32 flags) {
    ModelJoint* pChild;
    int i;

    pJoint->unk7 = flags & 0x1;

    if (flags & 0x80) {
        pChild = pModel->pSkeleton;
        for (i = 1; i < pModel->pSkeleton->numChildren; i++) {
            pChild++;
            if (pChild->pParent == pJoint) {
                func_801E6D94(pModel, pChild, flags);
            }
        }
    }
}

// Appended block of the per-joint task func_80023FD8 allocates. Its byte offset
// is the s16 at 0xBE of the task (spriteData.allocatedDataSize).
typedef struct {
    /* 0x0  */ u8 unk0[0x4];
    /* 0x4  */ WorkListCallback_t pChainCallback; // replaced by func_801E6F64
    /* 0x8  */ Model* pModel;
    /* 0xC  */ s16 jointIndex;
    /* 0xE  */ s16 unkE; // non-zero: snap the rotated offset to the model's ground
    /* 0x10 */ SVECTOR offset;
} LibGearJointTask; // Size: 0x18

// Per-joint data func_801E6E48 copies into the LibGearJointTask
typedef struct {
    /* 0x0  */ u8 unk0[0x5];
    /* 0x5  */ u8 jointIndex;
    /* 0x6  */ u16 offsetX;
    /* 0x8  */ u16 offsetY;
    /* 0xA  */ u16 offsetZ;
    /* 0xC  */ u8 unkC;
    /* 0xD  */ u8 padD[0x6];
    /* 0x13 */ u8 unk13; // non-zero: install func_801E6F64 as the task callback
} LibGearJointInfo; // Size: 0x14

// Allocates a work list task plus a LibGearJointTask; returns the task
extern void* func_80023FD8(s32 index, void* pAnimFile, s32 arg2, s32 jointTaskSize);
extern void func_80021FE0(SpriteData* pSpriteData, s16 direction);
extern void func_800223B0(SpriteData* pSpriteData, s16 direction);
extern void SpriteSetScale(SpriteData* pSpriteData, s16 scale);
extern WorkListCallback_t WorkListTaskGetTaskCallback(WorkListEntry* pTask);
extern void TimerWorkListSetTaskCallback(WorkListEntry* pTask, WorkListCallback_t callback);
extern void func_801E6F64(void* pTask);

void func_801E6E48(void* pAnimFile, s32 animIndex, s32 arg2, s16 direction, s16 scale, LibGearJointInfo* pInfo, Model* pModel) {
    AnimTask* pTask;
    SpriteData* pSpriteData;
    LibGearJointTask* pJointTask;

    pTask = func_80023FD8(animIndex, pAnimFile, arg2, 0x18);
    pSpriteData = &pTask->spriteData;
    func_80021FE0(pSpriteData, direction);
    func_800223B0(pSpriteData, direction);
    SpriteSetScale(pSpriteData, scale);

    pJointTask = (LibGearJointTask*)((u8*)pTask + pSpriteData->allocatedDataSize);
    pJointTask->pModel = pModel;
    pJointTask->jointIndex = pInfo->jointIndex;

    if (pInfo->unk13 != 0) {
        pJointTask->pChainCallback = WorkListTaskGetTaskCallback(&pTask->task1);
        TimerWorkListSetTaskCallback(&pTask->task1, &func_801E6F64);
        pJointTask->offset.vx = pInfo->offsetX;
        pJointTask->offset.vy = pInfo->offsetY;
        pJointTask->offset.vz = pInfo->offsetZ;
        pJointTask->unkE = pInfo->unkC;
    }
}

void func_801E6F64(void* pTaskRaw) {
    AnimTask* pTask = pTaskRaw;
    LibGearJointTask* pJointTask;
    LVEC3 vec;
    MATRIX* pMatrix;

    pJointTask = (LibGearJointTask*)((u8*)pTask + pTask->spriteData.allocatedDataSize);
    pMatrix = (MATRIX*)0x1F800000;

    if (pJointTask->jointIndex != 0) {
        CompMatrix(
            &pJointTask->pModel->pSkeleton[0].localTransform,
            &pJointTask->pModel->pSkeleton[pJointTask->jointIndex].globalTransform,
            pMatrix
        );
    } else {
        pMatrix = &pJointTask->pModel->pSkeleton[0].localTransform;
    }

    SetRotMatrix(pMatrix);
    SetTransMatrix(pMatrix);

    gte_ldv0(&pJointTask->offset);
    gte_mvmva(1, 0, 0, 0, 0);
    gte_stlvnl(&vec);

    if (pJointTask->unkE != 0) {
        vec.y = pJointTask->pModel->ground;
    }

    pTask->spriteData.position.x = CONV_FROM_GTE(vec.x);
    pTask->spriteData.position.y = CONV_FROM_GTE(vec.y);
    pTask->spriteData.position.z = CONV_FROM_GTE(vec.z);

    pJointTask->pChainCallback(pTask);
}

void func_801E7094(Model* pModel, ModelJoint* pJoint, u8 flags, s16 x, s16 y, s16 z) {
    ModelJoint* pChild;
    int i;

    if ((flags & 0x7) == 0) {
        if (flags & 0x20) {
            pJoint->rotation.vx += x;
            pJoint->rotation.vy += y;
            pJoint->rotation.vz += z;
        } else {
            pJoint->rotation.vx = x;
            pJoint->rotation.vy = y;
            pJoint->rotation.vz = z;
        }
    } else if ((flags & 0x7) == 1) {
        if (flags & 0x20) {
            pJoint->translation.x += x;
            pJoint->translation.y += y;
            pJoint->translation.z += z;
        } else {
            pJoint->translation.x = x;
            pJoint->translation.y = y;
            pJoint->translation.z = z;
        }
    } else {
        if (flags & 0x20) {
            pJoint->scale.vx += x;
            pJoint->scale.vy += y;
            pJoint->scale.vz += z;
        } else {
            pJoint->scale.vx = x;
            pJoint->scale.vy = y;
            pJoint->scale.vz = z;
        }
    }

    pJoint->doTransform = TRUE;
    pJoint->doRotate = TRUE;

    if (flags & 0x80) {
        pChild = pModel->pSkeleton;
        for (i = 1; i < pModel->pSkeleton->numChildren; i++) {
            pChild++;
            if (pChild->pParent == pJoint) {
                func_801E7094(pModel, pChild, flags, x, y, z);
            }
        }
    }
}

void func_801E7298(Model* pModel) {
    VECTOR unk;
    SVECTOR offset;

    offset.vy = pModel->ground;
    if (pModel->unk36 == 0) {
        pModel->pSkeleton->translation.y = offset.vy;
    }
}

// Computes the global transform matrix for a certain joint?
void func_801E72CC(MATRIX* pMatrix, void* _unk, int index, int jointIndex) {
    MATRIX _mat;
    Model* pModel;

    pModel = g_LibGearModels[index];
    if (pModel == NULL) {
        return;
    }
    
    if (jointIndex != 0) {
        CompMatrix(
            &pModel->pSkeleton[0].localTransform,
            &pModel->pSkeleton[jointIndex].globalTransform, 
            pMatrix
        );
        return;
    }
    
    *pMatrix = pModel->pSkeleton[0].localTransform;
}

void func_801E7378(u32 shouldRender) {
    D_801E85CC = shouldRender & 0x1;
}

void LibGearInitialize(int capacity) {
    int i;

    D_801E8640 = 0;
    D_801E869C = 0;
    
    func_801DF5F4(&D_801E86A8, capacity);
    LibGearInitializeVertexBuffer(&g_LibGearVertexBuffer, 0x10);

    for (i = 0; i < MAX_MODELS; i++) {
        g_LibGearModels[i] = NULL;
    }

    for (i = 0; i < MAX_MODEL_MESHES; i++) {
        g_LibGearMeshes[i].pParts = NULL;
    }

    for (i = 0; i < 2; i++) {
        D_801E8648[i].unk6 = 0;
    }
}

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E742C);

void func_801E7D14(MATRIX* pMatrix, s32 arg1, s32* pOT, s32 arg3, s32 arg4) {
    int i;
    int count;
    u16 angle;
    RECT rect;

    count = 0;
    D_801E8640 += arg4 + 1;
    if (D_801E8640 >= 7) {
        D_801E8640 = 6;
    }

    while (D_801E8640 >= 2) {
        D_801E8640 -= 2;
        count++;
    }

    angle = D_801E869C + count * 56;
    D_801E869C = angle;
    D_801E8698 = (rsin((s16)angle) + 0x1000) / 800 + 4;

    for (i = 0; i < MAX_MODELS; i++) {
        if (g_LibGearModels[i] != NULL) {
            g_LibGearModels[i]->unk11C = g_LibGearModels[i]->pSkeleton->translation.x;
            g_LibGearModels[i]->unk120 = g_LibGearModels[i]->pSkeleton->translation.y;
            g_LibGearModels[i]->unk124 = g_LibGearModels[i]->pSkeleton->translation.z;
            func_801E36BC(g_LibGearModels[i], (s32)&D_801E86A8, count, arg3, 1);
        }
    }

    for (i = 0; i < MAX_MODELS; i++) {
        if (g_LibGearModels[i] != NULL && g_LibGearModels[i]->unk5C < 0xFF) {
            func_801E37D0(g_LibGearModels[i]);
        }
    }

    func_801E1880(g_LibGearModels);
    SetColorMatrix(D_801E8644);

    for (i = 0; i < MAX_MODELS; i++) {
        if (g_LibGearModels[i] != NULL) {
            g_LibGearModels[i]->unk128 = g_LibGearModels[i]->unk11C - g_LibGearModels[i]->pSkeleton->translation.x;
            g_LibGearModels[i]->unk12C = g_LibGearModels[i]->unk120 - g_LibGearModels[i]->pSkeleton->translation.y;
            g_LibGearModels[i]->unk130 = g_LibGearModels[i]->unk124 - g_LibGearModels[i]->pSkeleton->translation.z;
            func_801DCEC8(g_LibGearModels[i], pMatrix, arg1, 1, 1, pOT, arg3);
        }
    }

    func_801E0398(&g_LibGearVertexBuffer, pMatrix, count, pOT, arg3);
}

void func_801E7FD4(void) {
    int i;

    for (i = 0; i < MAX_MODELS; i++) {
        func_801E8030(i);
    }

    func_801DF668(&D_801E86A8);
    LibGearFreeVertexBuffer(&g_LibGearVertexBuffer);
}

// 94.08%: the body is right, the reload shape of `g_LibGearModels[modelIndex]`
// (a CSE'd &g_LibGearModels[modelIndex] in s1, the model pointer never hoisted)
// matches, and so do the two 0x30/0x24 teardown loops. Remaining delta is one
// register pair: the original keeps the array address in s1 and &D_801E86A8 in
// s0, ours has them swapped, which also moves `addiu v0,s4,-8` out of the
// `beqz` delay slot. Declaration order and dropping the pList local both leave
// the swap.
INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E8030);
/*
    void func_801E8030(s32 modelIndex) {
        s32 i;
        s32 offset;

        if (g_LibGearModels[modelIndex] == NULL) {
            return;
        }

        if (g_LibGearModels[modelIndex]->unkA8 != NULL) {
            HeapFree(g_LibGearModels[modelIndex]->unkA8);
            LibGearFreeModelMesh(g_LibGearModels[modelIndex]->pMesh, 1);
        }

        if (g_LibGearModels[modelIndex]->unk62[0] != 0) {
            func_8003852C(((Temp8*)g_LibGearModels[modelIndex]->unkB0)->pUnk8);
        }

        if (g_LibGearModels[modelIndex]->unkAC != NULL) {
            HeapFree(g_LibGearModels[modelIndex]->unkAC);
        }

        if ((u32)(modelIndex - 8) >= 2) {
            if (g_LibGearModels[modelIndex]->pSkeleton != NULL) {
                func_801DFE8C(&D_801E86A8, g_LibGearModels[modelIndex]->pSkeleton);
                func_801DFF78(&D_801E86A8, g_LibGearModels[modelIndex]->pSkeleton, 0xFF);
                LibGearFreeModelSkeleton(g_LibGearModels[modelIndex]->pSkeleton);
                g_LibGearModels[modelIndex]->pMesh = NULL;
                g_LibGearModels[modelIndex]->pSkeleton = NULL;
            }
        } else {
            if (g_LibGearModels[modelIndex]->pSkeleton != NULL) {
                func_801DFE8C(&D_801E86A8, g_LibGearModels[modelIndex]->pSkeleton);
                func_801DFF78(&D_801E86A8, g_LibGearModels[modelIndex]->pSkeleton, 0xFF);
                HeapFree(g_LibGearModels[modelIndex]->pSkeleton);
            }
        }

        if (g_LibGearModels[modelIndex]->unk10C != 0) {
            HeapFree(g_LibGearModels[modelIndex]->unk110);
        }

        if (g_LibGearModels[modelIndex]->unk10E != 0) {
            offset = 0;
            for (i = 0; i < g_LibGearModels[modelIndex]->unk10E; i++) {
                func_801E165C((Temp8*)((u8*)g_LibGearModels[modelIndex]->unk118 + offset));
                offset += 0x30;
            }
            HeapFree(g_LibGearModels[modelIndex]->unk118);
        }

        if (g_LibGearModels[modelIndex]->unk10D != 0) {
            offset = 0;
            for (i = 0; i < g_LibGearModels[modelIndex]->unk10D; i++) {
                func_801E3438((Temp9*)((u8*)g_LibGearModels[modelIndex]->unk114 + offset));
                offset += 0x24;
            }
            HeapFree(g_LibGearModels[modelIndex]->unk114);
        }

        HeapFree(g_LibGearModels[modelIndex]);
        g_LibGearModels[modelIndex] = NULL;
    }
*/

void func_801E8330(u16 modelIndex, u16 bits, s32 arg2) {
    Model* pModel = g_LibGearModels[modelIndex];

    D_801E86B0 = modelIndex;
    D_801E863C = bits;
    pModel->unk35 = 0;

    if (g_LibGearModels[modelIndex] != NULL) {
        func_801E35D0(g_LibGearModels[modelIndex], g_LibGearModels[modelIndex], &D_801E86A8, arg2);
    }
}

void func_801E8394(s32 arg0, u16 modelIndex, u16 arg2, s32 arg3) {
    D_801E86B0 = modelIndex;
    D_801E863C = arg2;
    g_LibGearModels[modelIndex]->unk35 = 0;

    if (g_LibGearModels[modelIndex] != NULL) {
        if (modelIndex != func_801E67F8()) {
            func_801E35D0(g_LibGearModels[modelIndex], arg0, &D_801E86A8, arg3);
        }
    }
}

int func_801E8430(int modelIndex) {
    Model* pModel = g_LibGearModels[modelIndex];
    int result = 0;

    if (pModel != NULL) {
        result = (pModel->unk24 * ((pModel->scale * pModel->pSkeleton->scale.vy) >> 0xC)) >> 0xC;
    }

    return result;
}

int func_801E8480(int modelIndex) {
    int factor;
    int jointScale;
    int result = 0;

    if (g_LibGearModels[modelIndex] != NULL) {
        if (g_LibGearModels[modelIndex]->unk4A & 0x8) {
            jointScale = g_LibGearModels[modelIndex]->pSkeleton->scale.vx;
            factor = g_LibGearModels[modelIndex]->unk26;
        } else {
            jointScale = g_LibGearModels[modelIndex]->pSkeleton->scale.vz;
            factor = g_LibGearModels[modelIndex]->unk28;
        }

        result = factor * ((g_LibGearModels[modelIndex]->scale * jointScale) >> 0xC) >> 0xC;
    }

    return result;
}

void func_801E8510(Model* pModel) {
    int i;
    UnkInner1* pObjects;

    if (pModel->unk10C) {
        pObjects = HeapAlloc(pModel->unk10C * sizeof(UnkInner1), 0x0);
        for (i = 0; i < pModel->unk10C; i++) {
            pObjects[i].unk0 = -1;
            pObjects[i].unk8 = 0;
        }
        pModel->unk110 = pObjects;
    }
}
