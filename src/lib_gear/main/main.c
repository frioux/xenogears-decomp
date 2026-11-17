#include "common.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"
#include "system/memory.h"
#include "libgear/main.h"
#include "system/animation.h"

// Animation ids below LIBGEAR_ANIM_ID_HIGH_BASE are in Model::pEntryTableLow
// (indexed id + 1), and from it up in pEntryTableHigh (indexed id - 0x3F)
#define LIBGEAR_ANIM_ID_HIGH_BASE 0x40
#define LIBGEAR_ANIM_ID_HIGH_INDEX_OFFSET (LIBGEAR_ANIM_ID_HIGH_BASE - 1)

// From this id up the model's queued command names the animation
#define LIBGEAR_ANIM_ID_QUEUED_BASE 0xFE

// A byte of Model::commands: low 7 bits index the entry tables, bit 7 is a flag
#define LIBGEAR_MODEL_COMMAND_ID_MASK 0x7F
#define LIBGEAR_MODEL_COMMAND_FLAG 0x80

// Appended model/animation changes the queue holds
#define LIBGEAR_MODEL_COMMAND_QUEUE_MAX 5

// Model::flags bits
#define LIBGEAR_MODEL_FLAG_USE_WIDTH 0x8 // the width runs along the joint's x, else the depth along z
#define LIBGEAR_MODEL_FLAG_OWN_UPDATE 0x10 // keep doUpdate instead of inheriting the parent's

// ModelJointEntry index sentinels
#define LIBGEAR_JOINT_INDEX_NONE 0xFFFF // the joint has no model part
#define LIBGEAR_PARENT_INDEX_NONE 0xFFFF // the joint is the root

// LibGearReleaseJointObjects flags: which queued object to release
#define LIBGEAR_JOINT_RELEASE_ROTATION 0x1
#define LIBGEAR_JOINT_RELEASE_TRANSLATION 0x2
#define LIBGEAR_JOINT_RELEASE_SCALE 0x4

// LibGearSetJointTransform / LibGearSetJointFlag flags
#define LIBGEAR_JOINT_FLAG_ADD 0x20 // add to the joint's value instead of setting it
#define LIBGEAR_JOINT_FLAG_RECURSE 0x80 // apply to the joint's children too

// Interpolation curves LibGearGetInterpolationFunc picks by type; 0 (the default) is the sine
#define LIBGEAR_INTERPOLATE_RISE 1
#define LIBGEAR_INTERPOLATE_FALL 2
#define LIBGEAR_INTERPOLATE_FALL_CLAMPED 3

// Frames UnkInner1::numFrames holds at most
#define LIBGEAR_COLOR_ENTRY_FRAME_MAX 7

// Vertex entries LibGearInitialize creates the shared vertex buffer with
#define LIBGEAR_VERTEX_BUFFER_CAPACITY 16

// Degrees g_LibGearSwingAngle advances per stepped frame
#define LIBGEAR_SWING_ANGLE_STEP 56

// Value g_LibGearPendingFrameCount is clamped to
#define LIBGEAR_PENDING_FRAME_MAX 6

extern s32 g_GfxOTDepthShift;

// Interpolation helpers; LibGearGetInterpolationFunc picks one for a type
s16 LibGearInterpolateSine(s16 angle, s16 divisor, s16 offset);
s16 LibGearInterpolateRise(s16 a, s16 b, s16 c);
s16 LibGearInterpolateFall(s16 a, s16 b, s16 c);
s16 LibGearInterpolateFallClamped(s16 a, s16 b, s16 c);

extern Temp8* g_BattleSoundBankMain;
extern u32 g_LibGearDirectKeyframes;
extern u16 g_LibGearModelBitMask;
extern s16 g_LibGearCurrentModelIndex;
extern s32 g_LibGearPendingFrameCount;
extern s16 g_LibGearSwingAngle;
extern s16 g_LibGearSwingScale;

void func_801DCEC8(Model* pModel, MATRIX* pMatrix, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 arg6);
void func_80026F44(s32 count, s32 level, void* pDst, void* pSrc);
void func_80026FE8(s32 count, s32 level, void* pDst, void* pSrc, void* pSrc2);
void LibGearUpdateModelAttachment(Model* pModel);
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
    while (jointIndex < pModel->numParts || jointIndex == LIBGEAR_JOINT_INDEX_NONE) {
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
    pSkeleton[0].jointIndex = LIBGEAR_JOINT_INDEX_NONE;
    pSkeleton[0].numChildren = numJoints;
    pSkeleton[0].pModelPacketBuffer = NULL;
    pSkeleton[0].pCurModelPacket = NULL;
    pSkeleton[0].rotation.vx = 0;
    pSkeleton[0].rotation.vy = 0;
    pSkeleton[0].rotation.vz = 0;
    pSkeleton[0].translation.x = 0;
    pSkeleton[0].translation.y = 0;
    pSkeleton[0].translation.z = 0;
    pSkeleton[0].pRotationObject = 0;
    pSkeleton[0].pTranslationObject = 0;
    pSkeleton[0].pScaleObject = 0;
  
    while (jointIndex < pModel->numParts || jointIndex == LIBGEAR_JOINT_INDEX_NONE) {
        if (parentIndex == LIBGEAR_PARENT_INDEX_NONE) {
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
    
        if (jointIndex != LIBGEAR_JOINT_INDEX_NONE) {
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
        pCurJoint->pRotationObject = 0;
        pCurJoint->pTranslationObject = 0;
        pCurJoint->pScaleObject = 0;
        
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

unsigned LibGearComputeScaledJointMatrices(ModelJoint* pJoint, int degrees) {
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

        if (pJoint->pParent != NULL) {
            if (pJoint->pParent->doRotate == TRUE) {
                pJoint->doRotate = TRUE;
            }
            if (pJoint->pParent->doTransform == TRUE) {
                pJoint->doTransform = TRUE;
            }
        }

        if (pJoint->doRotate) {
            MATRIX* pScratch = (MATRIX*)PSX_SCRATCH;

            if (pJoint->rotDirection != 0) {
                RotMatrixYXZ(&pJoint->rotation, &pJoint->localTransform);
            } else {
                RotMatrix(&pJoint->rotation, &pJoint->localTransform);
            }

            pScratch->m[0][0] = pJoint->scale.vx;
            pScratch->m[0][1] = 0;
            pScratch->m[0][2] = 0;
            pScratch->m[1][0] = 0;
            pScratch->m[1][1] = pJoint->scale.vy;
            pScratch->m[1][2] = 0;
            pScratch->m[2][0] = 0;
            pScratch->m[2][1] = 0;
            pScratch->m[2][2] = pJoint->scale.vz;

            MulMatrix0(&pJoint->localTransform, pScratch, &pJoint->localTransform);

            if (pJoint->pParent != NULL) {
                pScratch->m[0][0] = 0x1000000 / pJoint->pParent->scale.vx;
                pScratch->m[0][1] = 0;
                pScratch->m[0][2] = 0;
                pScratch->m[1][0] = 0;
                pScratch->m[1][1] = 0x1000000 / pJoint->pParent->scale.vy;
                pScratch->m[1][2] = 0;
                pScratch->m[2][0] = 0;
                pScratch->m[2][1] = 0;
                pScratch->m[2][2] = 0x1000000 / pJoint->pParent->scale.vz;

                MulMatrix0(pScratch, &pJoint->localTransform, &pJoint->localTransform);
            }
        }

        if (pJoint->doTransform) {
            pJoint->localTransform.t[0] = pJoint->translation.x;
            pJoint->localTransform.t[1] = pJoint->translation.y;
            pJoint->localTransform.t[2] = pJoint->translation.z;

            if (pJoint->pParent != NULL) {
                CompMatrix(&pJoint->pParent->globalTransform, &pJoint->localTransform, &pJoint->globalTransform);
            } else {
                pJoint->globalTransform = pJoint->localTransform;
            }
        }
    }

    for (i = 1; i < numChildren; i++) {
        pSkeleton++;
        pSkeleton->doRotate = FALSE;
        pSkeleton->doTransform = FALSE;
    }

    return numChildren;
}
void LibGearNoOp(void) {}

void LibGearRenderModel(ModelMesh* pMesh, ModelJoint* pSkeleton, MATRIX* pLightMatrix, MATRIX* pViewMatrix, s32 arg4, s32 arg5, s32 packetIndex) {
    MATRIX* scratch = (MATRIX*)PSX_SCRATCH;
    MATRIX* pRotMatrix = &scratch[1];
    MATRIX* pTransMatrix = &scratch[2];
    MATRIX* pMatrix = scratch;
    u_int numChildren;
    u_int i;

    MulMatrix0(pViewMatrix, &pSkeleton->globalTransform, pRotMatrix);
    CompMatrix(pLightMatrix, &pSkeleton->localTransform, pTransMatrix);

    numChildren = pSkeleton->numChildren;
    pSkeleton++;

    for (i = 1; i < numChildren; i++) {
        if (pSkeleton->jointIndex != LIBGEAR_JOINT_INDEX_NONE) {
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
// differences remain, both allocation: the original loads pBlock->type first
// into t0 and keeps pKeyframe in a1 (the in-place `addiu a1,a1,0x18` reorg
// pulls into the bnez delay slot), where gcc 2.6 loads type last (v0) and
// copies pBlock to a2 for pKeyframe; and the back edge loses the original's
// dead `andi v0,count` delay slot insn. Load order 0x6,0xC,0xE,0x4 (fixed by
// declaring numTransFrames before flags) still allocates flags to a1 and
// pKeyframe to a2. Also tried: s16 x/y/z locals (74.79%), gcc 2.7.2 (94.95%).
// Screened: no compiler harvests it - 94.95 best (2.6.0 and 2.7.2 -O2 alike), 92.43 under 2.7.2-cdk,
// 89.90 cdk + maspsx aspsx 2.67. The delta is the one un-reproducible register pair noted above.
/*
void func_801DEF10(ModelJoint* pSkeleton, KeyframeBlock* pBlock) {
    u16 rotFrame = 0;
    u16 transFrame = 0;
    s16 blockType = pBlock->type;
    u16 numRotFrames = pBlock->numRotFrames;
    u16 numTransFrames = pBlock->numTransFrames;
    u16 flags = pBlock->flags;
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
                if (pJoint->pRotationObject == 0 || ((Temp3*)pJoint->pRotationObject)->type != 0xFF) {
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
                if (pJoint->pTranslationObject == 0 || ((Temp3*)pJoint->pTranslationObject)->type != 0xFF) {
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
INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801DEF10);

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801DF0B4);

void LibGearReleaseJointObjects(Temp4* pList, ModelJoint* pSkeleton, s32 index, s32 flags) {
    if (index < pSkeleton->numChildren) {
        pSkeleton += index;

        if (pSkeleton->pRotationObject != NULL && (flags & LIBGEAR_JOINT_RELEASE_ROTATION)) {
            LibGearFreeJointObject(pList, (s8*)pSkeleton->pRotationObject);
            pSkeleton->pRotationObject = 0;
        }

        if (pSkeleton->pTranslationObject != NULL && (flags & LIBGEAR_JOINT_RELEASE_TRANSLATION)) {
            LibGearFreeJointObject(pList, (s8*)pSkeleton->pTranslationObject);
            pSkeleton->pTranslationObject = 0;
        }

        if (pSkeleton->pScaleObject != NULL && (flags & LIBGEAR_JOINT_RELEASE_SCALE)) {
            LibGearFreeJointObject(pList, (s8*)pSkeleton->pScaleObject);
            pSkeleton->pScaleObject = 0;
        }
    }
}

void* LibGearInitializeJointObjectPool(Temp4* pUnk, int numObjects) {
    if (numObjects <= 0) {
        return NULL;
    }

    pUnk->numObjects = numObjects;
    HeapChangeCurrentUser(HEAP_USER_MASA, HEAP_CONTENT_NULL);
    pUnk->pObjects = HeapAlloc(numObjects * sizeof(Temp3), 0x0);
    if (pUnk->pObjects) {
        LibGearResetJointObjectPool(pUnk);
        return pUnk;
    }
    
    return NULL;
}

void LibGearFreeJointObjectPool(Temp4* pUnk) {
    pUnk->freeIndex = 0;
    if (pUnk->pObjects != NULL) {
        HeapFree(pUnk->pObjects);
    }
    pUnk->pObjects = NULL;
}

void LibGearResetJointObjectPool(Temp4* pUnk) {
    int i;
    Temp3* pCurObject;

    if (pUnk->pObjects == NULL) {
        return;
    }

    pCurObject = pUnk->pObjects;
    pUnk->freeIndex = 0;

    for (i = 0; i < pUnk->numObjects; i++) {
        pCurObject->inUse = 0;
        pCurObject++;
    }
}

Temp3* LibGearAllocJointObject(Temp4* pList) {
    Temp3* pObject;

    if (pList->freeIndex < pList->numObjects) {
        pObject = &pList->pObjects[pList->freeIndex];
        if (pObject->inUse == 0) {
            for (
                pList->freeIndex++;
                pList->freeIndex < pList->numObjects;
                pList->freeIndex++
            ) {
                if (pList->pObjects[pList->freeIndex].inUse == 0) {
                    break;
                }
            }
            return pObject;
        }
    }

    return NULL;
}

int LibGearFreeJointObject(Temp4* pUnk, s8* arg1) {
    int index;

    if (arg1 == NULL) {
        return -1;
    }
    
    index = ((mem_addr)arg1 - (mem_addr)pUnk->pObjects) / sizeof(Temp3);
    if (index < pUnk->freeIndex) {
        pUnk->freeIndex = index;
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
//     pObject->type test compares against a register loaded inside the loop.
//   * the original reads pBlock->type with `lhu` (unsigned); u16, s16 and a u16
//     cast on the s16 header field all still emit `lh`.
// Also tried: `pJoint = pSkeleton` as a declaration initialiser (68.76%), using
// pSkeleton as the joint cursor instead of a separate pJoint (68.76%), no
// `flag1` local with `flags & 0x1` inline (70.81%), declaration-order variations.
// Screened: 71.21 best (2.6.0), 70.49 under 2.7.2-cdk - not a preset problem.
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

    if (pBlock->type != 0) {
        LibGearReleaseSkeletonObjects(pList, pSkeleton);
        func_801DEF10(pSkeleton, pBlock);
        return 1;
    }

    numRotFrames = pBlock->numRotFrames;
    numChildren = pSkeleton->numChildren;
    count = numChildren;
    numTransFrames = pBlock->numTransFrames;
    blockFlags = pBlock->flags;
    flag1 = flags & 0x1;

    if (numRotFrames + 1 < numChildren) {
        count = numRotFrames + 1;
    }

    frame = pBlock->endFrame;
    if (!flag1) {
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
            pObject = (Temp3*)pJoint->pRotationObject;
            if (pObject == NULL) {
                pObject = LibGearAllocJointObject(pList);
            } else if (pObject->type == 0xFF) {
                goto next1;
            }
            if (pObject != NULL) {
                pObject->inUse = 1;
                pObject->unk1 = flag1;
                pObject->type = type;
                pObject->opcode = ((u8*)pKeyframe)[4];
                pObject->curFrame = 0;
                pObject->endFrame = frame;
                *(u32*)&pObject->startX = *(u32*)&pObject->startZ = (u32)(pVertexData + pKeyframe[0]);
                pJoint->pRotationObject = (u32)pObject;
            }
        } else {
            pObject = (Temp3*)pJoint->pRotationObject;
            if (pObject != NULL && i != 0 && pObject->type != 0xFF) {
                LibGearFreeJointObject(pList, (s8*)pObject);
                pJoint->pRotationObject = 0;
            }
        }
    next1:

        pKeyframe++;

        if (pKeyframe[0] != 0xFFFF) {
            pObject = (Temp3*)pJoint->pTranslationObject;
            if (pObject == NULL) {
                pObject = LibGearAllocJointObject(pList);
            } else if (pObject->type == 0xFF) {
                goto next2;
            }
            if (pObject != NULL) {
                pObject->inUse = 1;
                pObject->unk1 = flag1;
                pObject->type = type;
                pObject->opcode = ((u8*)pKeyframe)[4];
                pObject->curFrame = 0;
                pObject->endFrame = frame;
                *(u32*)&pObject->startX = *(u32*)&pObject->startZ = (u32)(pVertexData + pKeyframe[0]);
                pJoint->pTranslationObject = (u32)pObject;
            }
        } else {
            pObject = (Temp3*)pJoint->pTranslationObject;
            if (pObject != NULL && i != 0 && pObject->type != 0xFF) {
                LibGearFreeJointObject(pList, (s8*)pObject);
                pJoint->pTranslationObject = 0;
            }
        }
    next2:

        pKeyframe += 2;
        pJoint++;
    }

    return 0;
}
*/
INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801DF7F4);

// objdiff 79.40%, 996 bytes against retail's 968. Every one of retail's 242
// instructions is reproduced in order -- the two 6-byte-stride channels, the
// 0xFFFF merge into the else arm (the unk3==0xFF goto lands on the same code),
// the blockFlags&1 / i!=0 / rotFrame<numRotFrames guards, the pObject fill order
// (unk0,unk1,type,opcode,unk10,endFrame,unk4,unk8 with one pVertex temp so the two
// stores keep retail's order), the 6-byte pKeyframe stride and the joint+0x74
// base. What is left is ONE allocator decision that cascades:
//   retail: s0=pData s1=pJoint s2=pKeyframe s3=i s4=transFrame s5=rotFrame
//           s6=blockFlags s7=pVertexBase fp=flag1, the hoisted constant 1 in
//           a1 with a caller-save (sw/lw 0x48(sp)) around both
//           LibGearAllocJointObject calls, u8 `type` read straight from its
//           home slot 0x88(sp), and a per-iteration u8* pFrameData spilled to
//           0x18(sp) (addiu s2+4 / sw / two lw).
//   gcc 2.6.0 here: puts the hoisted 1 in a callee-saved reg (s7, no saves),
//           pVertexBase in fp, and spills flag1 (u8, 0x40(sp)) and `type`
//           (0x18(sp), one lbu home + one sb). Same frame size 0x78, same slot
//           map, but 8 registers instead of 9 long-lived and the stores pay a
//           load each.
// Tried and measured, none of it changes the assignment: flag1 s32 (75.14) /
// u8 (79.38) / u32, flag1 = flags; flag1 &= 1;  (gcc folds it back to one
// andi), if (!flag1), flag1 declared first, a pFrameData local at the loop top
// (frame grows to 0x80), the chained unk4=unk8 assignment (stores 8 before 4,
// so the pVertex temp is required). A 4-minute permuter run (1754 iterations,
// base 5790) reached 5260 and found only new_var splits -- no structural win.
// Screened earlier: 74.89 under 2.7.2-cdk, so not a preset problem.
/*
s32 func_801DFAC4(Temp4* pList, ModelJoint* pSkeleton, KeyframeBlock* pBlock, s32 flags, u8 type) {
    u16 numRotFrames;
    u16 numTransFrames;
    u16 count;
    u16 frame;
    u16 rotFrame;
    u16 transFrame;
    u16 blockFlags;
    u8 flag1;
    u16* pKeyframe;
    u16* pData;
    u8* pVertexBase;
    u32 pVertex;
    Temp3* pObject;
    ModelJoint* pJoint;
    s32 i;

    if (pBlock->type != 0) {
        LibGearReleaseSkeletonObjects(pList, pSkeleton);
        func_801DEF10(pSkeleton, pBlock);
        return 1;
    }

    flag1 = flags & 0x1;
    numRotFrames = pBlock->numRotFrames;
    rotFrame = 0;
    count = pSkeleton->numChildren;
    if (numRotFrames + 1 < count) {
        count = numRotFrames + 1;
    }
    numTransFrames = pBlock->numTransFrames;
    transFrame = 0;

    frame = pBlock->endFrame;
    if (flag1 == 0) {
        frame--;
    }

    blockFlags = pBlock->flags;
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
            pObject = (Temp3*)pJoint->pRotationObject;
            if (pObject != NULL) {
                if (pObject->type == 0xFF) {
                    goto skipRot;
                }
            } else {
                pObject = LibGearAllocJointObject(pList);
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
                pObject->inUse = 1;
                pObject->unk1 = flag1;
                pObject->type = type;
                pObject->opcode = ((u8*)&pKeyframe[2])[0];
                pObject->curFrame = 0;
                pObject->endFrame = frame;
                pVertex = (u32)(pVertexBase + pKeyframe[0]);
                *(u32*)&pObject->startX = pVertex;
                *(u32*)&pObject->startZ = pVertex;
                pJoint->pRotationObject = (u32)pObject;
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
            pObject = (Temp3*)pJoint->pTranslationObject;
            if (pObject != NULL) {
                if (pObject->type == 0xFF) {
                    goto skipTrans;
                }
            } else {
                pObject = LibGearAllocJointObject(pList);
            }

            if (!(blockFlags & 0x2) && i != 0 && (u16)transFrame < numTransFrames) {
                pJoint->translation.x = *(s16*)pData++;
                pJoint->translation.y = *(s16*)pData++;
                pJoint->translation.z = *(s16*)pData++;
                transFrame++;
                pJoint->doTransform = 1;
            }

            if (pObject != NULL) {
                pObject->inUse = 1;
                pObject->unk1 = flag1;
                pObject->type = type;
                pObject->opcode = ((u8*)&pKeyframe[2])[1];
                pObject->curFrame = 0;
                pObject->endFrame = frame;
                pVertex = (u32)(pVertexBase + pKeyframe[0]);
                *(u32*)&pObject->startX = pVertex;
                *(u32*)&pObject->startZ = pVertex;
                pJoint->pTranslationObject = (u32)pObject;
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
INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801DFAC4);

void LibGearReleaseSkeletonObjects(Temp4* pList, ModelJoint* pSkeleton) {
    int i;
    u16 numChildren = pSkeleton->numChildren;

    for (i = 0; i < numChildren; i++) {
        if (pSkeleton->pRotationObject != 0 && ((Temp3*)pSkeleton->pRotationObject)->type != 0xFF) {
            LibGearFreeJointObject(pList, (s8*)pSkeleton->pRotationObject);
            pSkeleton->pRotationObject = 0;
        }

        if (pSkeleton->pTranslationObject != 0 && ((Temp3*)pSkeleton->pTranslationObject)->type != 0xFF) {
            LibGearFreeJointObject(pList, (s8*)pSkeleton->pTranslationObject);
            pSkeleton->pTranslationObject = 0;
        }

        if (pSkeleton->pScaleObject != 0 && ((Temp3*)pSkeleton->pScaleObject)->type != 0xFF) {
            LibGearFreeJointObject(pList, (s8*)pSkeleton->pScaleObject);
            pSkeleton->pScaleObject = 0;
        }

        pSkeleton++;
    }
}

void LibGearReleaseSkeletonObjectsByType(Temp4* pList, ModelJoint* pSkeleton, u8 type) {
    int i;
    u16 numChildren = pSkeleton->numChildren;

    for (i = 0; i < numChildren; i++) {
        if (pSkeleton->pRotationObject != 0 && ((Temp3*)pSkeleton->pRotationObject)->type == type) {
            LibGearFreeJointObject(pList, (s8*)pSkeleton->pRotationObject);
            pSkeleton->pRotationObject = 0;
        }

        if (pSkeleton->pTranslationObject != 0 && ((Temp3*)pSkeleton->pTranslationObject)->type == type) {
            LibGearFreeJointObject(pList, (s8*)pSkeleton->pTranslationObject);
            pSkeleton->pTranslationObject = 0;
        }

        if (pSkeleton->pScaleObject != 0 && ((Temp3*)pSkeleton->pScaleObject)->type == type) {
            LibGearFreeJointObject(pList, (s8*)pSkeleton->pScaleObject);
            pSkeleton->pScaleObject = 0;
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
        pVertex->age = -1;
        pVertex->lifetime = 0;
        
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

void* LibGearAllocVertex(VertexBuffer* pVertexBuffer, short isSemiTrans) {
    Vertex* pVertex;
    
    if (pVertexBuffer->curIndex < pVertexBuffer->capacity) {
        pVertex = &pVertexBuffer->pVertices[pVertexBuffer->curIndex];
        if (pVertex->age == -1) {
            for (
                pVertexBuffer->curIndex++; 
                pVertexBuffer->curIndex < pVertexBuffer->capacity; 
                pVertexBuffer->curIndex++
            ) {
                if (pVertexBuffer->pVertices[pVertexBuffer->curIndex].age == -1) {
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

int LibGearFreeVertex(VertexBuffer* pVertexBuffer, Vertex* pObject) {
    int index;

    index = ((mem_addr)pObject - (mem_addr)pVertexBuffer->pVertices) / sizeof(Vertex);
    if (pVertexBuffer->curIndex >= index) {
        pVertexBuffer->curIndex = index;
    }
    
    pObject->age = -1;
    return index;
}

void LibGearRenderVertices(VertexBuffer* pVertexBuffer, MATRIX* pMatrix, s32 arg2, s32* pOT, s32 renderCtx) {
    long otz;
    int i;
    Vertex* pVertex;

    SetRotMatrix(pMatrix);
    SetTransMatrix(pMatrix);
    
    pVertex = pVertexBuffer->pVertices;
    for (i = 0; i < pVertexBuffer->capacity; i++, pVertex++) {
        if (pVertex->age == -1) {
            continue;
        }
        
        if (pVertex->age >= pVertex->lifetime) {
            LibGearFreeVertex(pVertexBuffer, pVertex);
            continue;
        }

        // Does not match w/ setRGB0 macro
        pVertex->polys[renderCtx].r0 = pVertex->red >> 6;
        pVertex->polys[renderCtx].g0 = pVertex->green >> 6;
        pVertex->polys[renderCtx].b0 = pVertex->blue >> 6;
        
        if (pVertex->is3D == 0) {
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
            otz >>= g_GfxOTDepthShift;

            // RotTransPers
            gte_ldv0((SVECTOR*) &pVertex->x3);
            gte_rtps();
            gte_stsxy((long*) &pVertex->polys[renderCtx].x3);

            addPrim(&pOT[otz], &pVertex->polys[renderCtx]);
        }
        
        pVertex->age += arg2;
        pVertex->red -= pVertex->redDelta * arg2;
        pVertex->green -= pVertex->greenDelta * arg2;
        pVertex->blue -= pVertex->blueDelta * arg2;
    }
}

s32 LibGearInitializeColorEntry(UnkInner1* pEntry, u32 arg1, u16 arg2, u8 arg3, s16 numFrames, u16 divisor, u8 arg6, u8 arg7, u8 arg8, u8 arg9, u8 arg10, u8 arg11, u16 arg12, u16 arg13, u16 arg14, u16 arg15, u16 arg16, u16 arg17, u16 arg18) {
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
        if (numFrames < LIBGEAR_COLOR_ENTRY_FRAME_MAX) {
            pEntry->numFrames = numFrames;
        } else {
            pEntry->numFrames = LIBGEAR_COLOR_ENTRY_FRAME_MAX;
        }

        pEntry->red = arg6 << 6;
        pEntry->green = arg7 << 6;
        pEntry->blue = arg8 << 6;
        div = (s16)divisor;
        pEntry->redDelta = (pEntry->red - (arg9 << 6)) / div;
        pEntry->greenDelta = (pEntry->green - (arg10 << 6)) / div;
        pEntry->blueDelta = (pEntry->blue - (arg11 << 6)) / div;

        pEntry->divisor = divisor;
        pEntry->unk8 = 0;

        return 0;
    }
}

void LibGearClearColorEntry(short* pValue, s32 arg1) {
    *pValue = -1;
}

s16 LibGearInterpolateSine(s16 angle, s16 divisor, s16 offset) {
    return offset + (rsin(angle) + 0x1000) / divisor;
}

s16 LibGearInterpolateRise(s16 a, s16 b, s16 c) {
    s16 result = c + a / b;

    if (result > 0x20) {
        return -1;
    } else {
        return result;
    }
}

s16 LibGearInterpolateFall(s16 a, s16 b, s16 c) {
    return c - a / b;
}

s16 LibGearInterpolateFallClamped(s16 a, s16 b, s16 c) {
    s16 result = 0x20 - a / b;

    if (result < c) {
        result = c;
    }

    return result;
}

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E0A00);

// objdiff 88.98%, 1048 bytes against retail's 1028. The whole structure is
// reproduced: the unk14/angleStep advance with the pInterpolate callback, the
// flush-on-negative path, the prevFrame latch, the four-way unk10 dispatch (0,1,4,5
// as a compare tree - matches a switch), the shared LoadImage tail of cases 0/1,
// the clipped copy rectangle (retail really does keep it in two RECT locals -
// that is what makes the sh/lh off sp 0x18..0x26 and the oversized 0x78 frame
// match exactly), the unk11=1 flag, the unk10<4 two-loop split and the 6-byte
// row pitch of the pUnk1C path. pDstBase/pSrcBase locals cost the two hoisted
// `lw 4(t1)` / `lw 12(s0)`.
// FINDINGS from this round:
// 1. The old body's second loop was WRONG: retail applies the destination row
//    product there too (`lh v0,0x22(sp); lh v1,0x2c(t1); mult`), so both loops
//    store to pDstBase + (dstY+y)*pSource->rect.w + dstX + x.
// 2. The addu operand order IS controllable, in the mirror direction of the
//    documented embedded-assignment lever: `dstX*2 + (u32)base` keeps
//    retail's offset-first `addu a0,a0,t3`, while every pointer expression
//    (`base + x`, `x + base`, `p[i]`, `((u8*)base + i)`) collapses to the
//    same base-first `addu v1,t3,dstX<<1` however it is spelled. The pointer
//    form also has to be a per-term scaled sum plus the row product last, so
//    the u32 locals below are what reproduces retail's five addu sites.
// WHAT IS LEFT: the prevFrame early-exit costs 3 instructions more than retail's
// single `beq s1,v0,.epilogue` (ours: `bne` + `j` + `addu v0,s1,zero`), and
// retail's tail return truncates the raw callback result (`sll v0,s2,16` /
// `sra`) at four exits while its negative exit is a plain `addu v0,s1,zero` -
// so the returned expression at those sites is NOT the s16 local. Retail also
// sign-extends the callback result explicitly, i.e. Temp8::pInterpolate returns
// something wider than the s16 the header declares. Register allocation after
// that (t1/t2 vs retail's t2/t3) is what is left of the loops.
// Tried: `base + row*w + dstX + x` (row first, 85.01), the col-pointer +
// `pDst[(dstY+y)*w]` pointer/subscript forms (85.5-87.9), s16 loop counters
// (much worse - the counters must be int), a nested `if (frame != prevFrame)`
// single-exit body (85.5).
// A 3-minute permuter run on the previous body (676 iterations, base 2411)
// reached 2131 only by deleting the `return frame` of the prevFrame early-exit,
// which is not a fix.
// Round 165 (park repair): this body did NOT compile against the current headers
// -- it compared pSource->unk10, renamed to mode -- so its recorded 88.98% came
// from an older variant. Fixed to pSource->mode, un-parked and re-measured:
// 88.98%, 1048 bytes against retail's 1028 (retail 0x404, ours 0x418). The
// deltas above are unchanged, so the note stands; re-parked.
INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E1258);
/*
s16 func_801E1258(Temp8* pTexture, s32 frameStep) {
    Temp8* pSource;
    s16 frame;
    RECT srcRect;
    RECT dstRect;
    int x;
    int y;
    u16* pDstBase;
    u16* pSrcBase;
    u16* pDst;
    u16* pSrc;
    u32 dstAddr;
    u32 srcAddr;

    if (pTexture->isLoaded == 0) {
        return -1;
    }

    pTexture->unk14 += pTexture->angleStep * (frameStep + 1);
    frame = pTexture->pInterpolate(pTexture->unk14, pTexture->divisor, pTexture->offset);
    if (frame < 0) {
        LibGearFlushTexture(pTexture);
        return frame;
    }

    if (frame == pTexture->prevFrame) {
        return frame;
    }
    pTexture->prevFrame = frame;

    switch (pTexture->mode) {
        case 0:
            func_80026F44(pTexture->pixelCount, frame, pTexture->pUnkC, pTexture->pUnk4);
            if (pTexture->pSource == NULL) {
                LoadImage(&pTexture->rect, (u_long*)pTexture->pUnkC);
            }
            break;
        case 1:
            func_80026FE8(pTexture->pixelCount, frame, pTexture->pUnkC, pTexture->pUnk8, pTexture->pUnk4);
            if (pTexture->pSource == NULL) {
                LoadImage(&pTexture->rect, (u_long*)pTexture->pUnkC);
            }
            break;
        case 4:
            LibGearFadeTexturePixels(pTexture, frame);
            break;
        case 5:
            LibGearBlendTexturePixels(pTexture, frame);
            break;
    }

    pSource = pTexture->pSource;
    if (pSource == NULL || pSource->isLoaded == 0) {
        return frame;
    }

    // Clip the copy to the overlap of the two rectangles
    if (pSource->rect.x < pTexture->rect.x) {
        dstRect.x = pTexture->rect.x - pSource->rect.x;
        srcRect.x = 0;
        dstRect.w = pSource->rect.x + pSource->rect.w - pTexture->rect.x;
    } else {
        dstRect.x = 0;
        srcRect.x = pSource->rect.x - pTexture->rect.x;
        dstRect.w = pTexture->rect.x + pTexture->rect.w - pSource->rect.x;
    }

    if (pSource->rect.y < pTexture->rect.y) {
        dstRect.y = pTexture->rect.y - pSource->rect.y;
        srcRect.y = 0;
        dstRect.h = pSource->rect.y + pSource->rect.h - pTexture->rect.y;
    } else {
        dstRect.y = 0;
        srcRect.y = pSource->rect.y - pTexture->rect.y;
        dstRect.h = pTexture->rect.y + pTexture->rect.h - pSource->rect.y;
    }

    if (dstRect.w > 0 && dstRect.h > 0) {
        pSource->unk11 = 1;
        pDstBase = pSource->pUnk4;
        pSrcBase = (u16*)pTexture->pUnkC;

        if (pSource->mode < 4) {
            for (y = 0; y < dstRect.h; y++) {
                for (x = 0; x < dstRect.w; x++) {
                    dstAddr = dstRect.x * 2 + (u32)pDstBase;
                    srcAddr = srcRect.x * 2 + (u32)pSrcBase;
                    dstAddr = x * 2 + dstAddr;
                    srcAddr = x * 2 + srcAddr;
                    *(u16*)((dstRect.y + y) * pSource->rect.w * 2 + dstAddr) =
                        *(u16*)((srcRect.y + y) * pTexture->rect.w * 2 + srcAddr);
                }
            }
        } else {
            for (y = 0; y < dstRect.h; y++) {
                for (x = 0; x < dstRect.w; x++) {
                    pDst = pDstBase + dstRect.x + x;
                    pSrc = (u16*)pTexture->pRows + 3 * (srcRect.y + y) + (srcRect.x + x);
                    pDst[(dstRect.y + y) * pSource->rect.w] = *pSrc;
                }
            }
        }
    }

    return frame;
}
*/

void LibGearFlushTexture(Temp8* pTexture) {
    if (pTexture->isLoaded != 0) {
        if (pTexture->pUnk4 != NULL) {
            if (pTexture->mode < 4) {
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

        pTexture->isLoaded = 0;
    }
}

void LibGearFadeTexturePixels(Temp8* pTexture, s16 scale) {
    u16* pSrc = pTexture->pUnk4;
    s16* pDst;
    int y;
    int x;

    for (y = 0; y < pTexture->rect.h; y++) {
        for (x = 0; x < pTexture->rect.w; x++) {
            pDst = (s16*)pTexture->pRows + 3 * (pTexture->rect.y + y);
            pDst[pTexture->rect.x + x] = (*pSrc * scale) / 0x20;
            pSrc++;
        }
    }
}

void LibGearBlendTexturePixels(Temp8* pTexture, s16 scale) {
    u16* pSrc = pTexture->pUnk4;
    u16* pDelta = pTexture->pUnk8;
    s16* pDst;
    int y;
    int x;

    for (y = 0; y < pTexture->rect.h; y++) {
        for (x = 0; x < pTexture->rect.w; x++) {
            pDst = (s16*)pTexture->pRows + 3 * (pTexture->rect.y + y);
            pDst[pTexture->rect.x + x] = *pDelta + (*pSrc - *pDelta) * scale / 0x20;
            pSrc++;
            pDelta++;
        }
    }
}

// Rotates each entry's source position by the model's joint matrix
void LibGearRotateModelOffsets(Model** pModels) {
    MATRIX* pMatrix = (MATRIX*)PSX_SCRATCH;
    Model* pModel;
    u_short value[6];
    int i;

    for (i = 0; i < 2; i++) {
        if (g_LibGearModelOffsets[i].inUse != 0) {
            if (g_LibGearModelOffsets[i].modelIndex >= 0 && (pModel = pModels[g_LibGearModelOffsets[i].modelIndex]) != NULL) {
                CompMatrix(
                    &pModel->pSkeleton[0].localTransform,
                    &pModel->pSkeleton[g_LibGearModelOffsets[i].jointIndex + 1].globalTransform,
                    pMatrix
                );
                SetRotMatrix(pMatrix);
                SetTransMatrix(pMatrix);

                gte_ldv0((SVECTOR*)&g_LibGearModelOffsets[i].x1);
                gte_mvmva(1, 0, 0, 0, 0);
                gte_stlvnl(value);

                g_LibGearModelOffsets[i].x0 = value[0];
                g_LibGearModelOffsets[i].y0 = value[2];
                g_LibGearModelOffsets[i].z0 = value[4];
            } else {
                g_LibGearModelOffsets[i].x0 = g_LibGearModelOffsets[i].x1;
                g_LibGearModelOffsets[i].y0 = g_LibGearModelOffsets[i].y1;
                g_LibGearModelOffsets[i].z0 = g_LibGearModelOffsets[i].z1;
            }
        }
    }
}


INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E1A14);

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E22F8);

void LibGearFreeModelEntry(Temp9* pEntry) {
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

s16 (*LibGearGetInterpolationFunc(s32 type))(s16, s16, s16) {
    switch (type) {
        case LIBGEAR_INTERPOLATE_RISE:
            return LibGearInterpolateRise;
        case LIBGEAR_INTERPOLATE_FALL:
            return LibGearInterpolateFall;
        case LIBGEAR_INTERPOLATE_FALL_CLAMPED:
            return LibGearInterpolateFallClamped;
        default:
            return LibGearInterpolateSine;
    }
}

// objdiff 94.87%: 39 insns, exact instruction multiset, one store slot apart.
// Both `li v0,1` (for unk70[15], insn 7) and the whole body match, so the locals
// negOne = -1 and one = 1 are confirmed. The only delta is scheduling: the
// original keeps `li v1,-1` at insn 0 and its first user `sh v1,0x98` at insn 14.
// With `pModel->frame = negOne;` first the li sits at 0 but the store is deferred
// to 8; with the store in its logical place (after unk2B) the store lands at 14
// but the li is hoisted to 6 (74.36%). Declaration order of the two locals, a
// mid-block `s32 one = 1;` and bare/s16/u32 -1 forms all stay at 74.36%.
// Round 14: the two remaining deltas are the -1 constant's slot and the 0x98
// store's slot, and they are coupled by sched2: with `frame = negOne` as the
// first statement the addiu lands at slot 0 (correct) but the store at 8 (target
// 14); with it as the 10th statement the store lands at 14 (correct) but the
// addiu at 5; 6th statement gives (5, 9). Store order = source order in every
// shape, so the (0, 14) split needs a scheduling decision this gcc will not make.
// Screened: 94.87 identical under 2.6.0 and 2.7.2-cdk - not a compiler problem.
// Later: three bare `-1` literals (no negOne local) also give 74.36 - cse still
// merges them into one register and sched2 parks that li 9 insns early (slot 5),
// so the constant is not a source-shape question either.
// Round 62: permuter, 783 iterations from this body (base 60), found nothing
// better - the (li slot 0, store slot 14) split is confirmed out of reach here.
INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E3534);
/*
void func_801E3534(Model* pModel, s32 arg1, u32 arg2, u32 arg3) {
    s16 negOne = -1;
    s16 one = 1;

    pModel->frame = negOne;
    pModel->objectType = 0xFFFF;
    pModel->parentModelSlot = 0xFF;
    pModel->unk39 = 0x6B;
    pModel->pAnimTable = arg2;
    pModel->unkC = 0;
    pModel->animValue = 0;
    pModel->pEntryTableLow = (Temp7**)arg3;
    pModel->pEntryTableHigh = NULL;
    pModel->commandCount = 0;
    pModel->sourceModelSlot = 0;
    pModel->unk35 = 0;
    pModel->useScaledMatrices = 0;
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
    pModel->posX = 0;
    pModel->posY = 0;
    pModel->posZ = 0;
    pModel->unk8E = one;
    pModel->skipGroundSnap = 0;
    pModel->unk1E = negOne;
}
*/
void LibGearSetAnimation(Model* pModel, Model* pSource, s32 arg2, s32 arg3) {
    if ((pModel != NULL) && (pSource != NULL)) {
        if (pModel->commandCount != 0) {
            if (pModel->commandCount < LIBGEAR_MODEL_COMMAND_QUEUE_MAX) {
                pModel->commandCount++;
            }

            pModel->commands[pModel->commandCount] = pSource->modelSlot;
            pModel->commandArgs[pModel->commandCount] = arg3;
        } else {
            if (arg3 < 0x50) {
                pModel->animValue = ((u32*)pSource->pAnimTable)[arg3];
            } else {
                pModel->animValue = ((u32*)((Temp8*)pSource->unkC)->pUnk4)[arg3 - 0x4E];
            }

            pModel->unk42 = 0;
            pModel->unk40 = 0;
            pModel->unk50 = 0;
            pModel->unk54 = 0;
            pModel->unk4C = 0;
            pModel->modelBitMask = g_LibGearModelBitMask;
            pModel->unk23 = 0;
            func_801E39F0(pModel, arg2, -1, 1, 0);
        }
    }
}

s32 LibGearStepModelAnimation(Model* pModel, s32 arg1, s32 count, s32 arg3, s32 arg4) {
    int i;
    s32 result;

    if (pModel->pMesh != NULL) {
        result = 0;

        if (pModel->doUpdate) {
            LibGearApplyGroundHeight(pModel);

            if (pModel->useScaledMatrices) {
                LibGearComputeScaledJointMatrices(pModel->pSkeleton, pModel->scale);
            } else {
                LibGearComputeJointMatrices(pModel->pSkeleton, pModel->scale);
            }

            for (i = 0; i < count; i++) {
                result |= func_801DDBF8(arg1, pModel->pSkeleton, pModel->objectType, pModel->scale);
                func_801E5D44(pModel, arg1, arg3);
            }
        }

        func_801E39F0(pModel, arg1, result, count, arg4);
    }

    return result;
}

void LibGearUpdateModelAttachment(Model* pModel) {
    MATRIX* pMatrix = (MATRIX*)PSX_SCRATCH;
    SVECTOR vec;

    if (g_LibGearModels[pModel->parentModelSlot] != NULL) {
        if (!(pModel->flags & LIBGEAR_MODEL_FLAG_OWN_UPDATE)) {
            pModel->doUpdate = g_LibGearModels[pModel->parentModelSlot]->doUpdate;
        }

        if (pModel->doUpdate == 0) {
            return;
        }

        if (pModel->isAttached != 0) {
            if (pModel->parentJointIndex != 0) {
                MulMatrix0(
                    &g_LibGearModels[pModel->parentModelSlot]->pSkeleton[0].globalTransform,
                    &g_LibGearModels[pModel->parentModelSlot]->pSkeleton[pModel->parentJointIndex].globalTransform,
                    (MATRIX*)PSX_SCRATCH
                );
            } else {
                pMatrix = &g_LibGearModels[pModel->parentModelSlot]->pSkeleton[0].globalTransform;
            }

            MulMatrix2(pMatrix, &pModel->pSkeleton[0].localTransform);
            MulMatrix2(pMatrix, &pModel->pSkeleton[0].globalTransform);
        }

        if (pModel->parentJointIndex != 0) {
            CompMatrix(
                &g_LibGearModels[pModel->parentModelSlot]->pSkeleton[0].localTransform,
                &g_LibGearModels[pModel->parentModelSlot]->pSkeleton[pModel->parentJointIndex].globalTransform,
                pMatrix
            );
        } else {
            pMatrix = &g_LibGearModels[pModel->parentModelSlot]->pSkeleton[0].localTransform;
        }

        SetRotMatrix(pMatrix);
        SetTransMatrix(pMatrix);

        vec.vx = pModel->attachOffsetX;
        vec.vy = pModel->attachOffsetY;
        vec.vz = pModel->attachOffsetZ;

        gte_ldv0(&vec);
        gte_mvmva(1, 0, 0, 0, 0);
        gte_stlvnl(&pModel->pSkeleton[0].localTransform.t[0]);
        gte_stlvnl(&pModel->pSkeleton[0].translation.x);
    } else {
        pModel->parentModelSlot = 0xFF;
    }
}

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E39F0);

void LibGearSetJointRotation(Temp4* pList, ModelJoint* pJoint, s32 numFrames, s32 x, s32 y, s32 z) {
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

    if (pJoint->pRotationObject == NULL) {
        pObject = LibGearAllocJointObject(pList);
    } else {
        pObject = (Temp3*)pJoint->pRotationObject;
    }

    if (pObject != NULL) {
        pObject->inUse = 1;
        pObject->unk1 = 0;
        pObject->opcode = 3;
        pObject->type = 0xFE;

        pObject->startX = pJoint->rotation.vx;
        pObject->startY = pJoint->rotation.vy;
        pObject->startZ = pJoint->rotation.vz;

        x = (x - pJoint->rotation.vx) & 0xFFF;
        if (x >= 0x800) {
            x -= 0x1000;
        }
        pObject->targetX = x;

        y = (y - pJoint->rotation.vy) & 0xFFF;
        if (y >= 0x800) {
            y -= 0x1000;
        }
        pObject->targetY = y;

        z = (z - pJoint->rotation.vz) & 0xFFF;
        if (z >= 0x800) {
            z -= 0x1000;
        }
        pObject->targetZ = z;

        pObject->curFrame = 0;
        pObject->endFrame = numFrames;
        pJoint->pRotationObject = (u32)pObject;
    }
}

void LibGearMoveJointToPoint(Temp4* pList, ModelJoint* pJoint, s32 arg2, s32 arg3, s32 arg4, s32 arg5, s32 x, s32 y, s32 z) {
    Temp3* pObject;
    int dx;
    int dy;
    int dz;

    if (pJoint->pRotationObject == NULL) {
        pObject = LibGearAllocJointObject(pList);
    } else {
        pObject = (Temp3*)pJoint->pRotationObject;
    }

    if (pObject != NULL) {
        pObject->inUse = 1;
        pObject->unk1 = 0;
        pObject->opcode = arg2 + 7;
        pObject->type = 0xFE;

        dx = x - pJoint->translation.x;
        dy = y - pJoint->translation.y;
        dz = z - pJoint->translation.z;

        pObject->startX = SquareRoot0(dx * dx + dy * dy + dz * dz) + 1;
        pObject->startY = arg3;
        pObject->startZ = arg4;
        pObject->targetX = x;
        pObject->targetY = y;
        pObject->targetZ = z;
        pObject->curFrame = 0;
        pObject->endFrame = arg5;

        pJoint->pRotationObject = (u32)pObject;
    }
}

void LibGearPlayAnimationScript(Model* pModel, Temp7* pEntry, s32 arg2) {
    if (pEntry->numCommands != 0) {
        pModel->frame = 0;
        if (arg2 != 0) {
            pModel->frameCount = pEntry->frameCount;
        } else {
            pModel->frameCount = -1;
        }
        pModel->commandIndex = 0;
        pModel->numCommands = pEntry->numCommands;
        pModel->pScriptCursor = (u8*)pEntry + pEntry->dataOffset;
        pModel->pScriptStart = pModel->pScriptCursor;
    } else {
        pModel->frame = -1;
    }
}

// Round 14: three separate `return`s with no shared local also cross-jump the
// same wrong pair (61.93%, 88 bytes): gcc keeps type==1 inline at the beq target.
// Best attempt, 71.67%. The three temp8 hops are confirmed. What the original
// does and this does not: type==1 gets its OWN copy of the `lhu 0x14 / sll 16`
// tail (out of line, last, falling into the epilogue) while type==0 and type==2
// share one copy. Giving the type==1 arm its own `return` produces the second
// copy but gcc then emits type==1 inline (61.93%); with the `if (type != 1) ...
// else return ...` spelling below gcc shares a *deeper* tail instead
// (`lw 8(v0) / lhu 20 / sll` shared by type==1 and type==2, 88 vs 108 bytes), so
// the split point and the block placement are one decision. A `switch` on type
// is far off (19.74%, case bodies emitted in source order with break jumps).
// Later round: the plain `if/else if` chain that assigns a single `pTexture`
// local and then has ONE `return pTexture->unk14 << 16` is also 61.93%
// (88 bytes vs the original's 108): gcc cross-jumps the type==1 and type==2
// tails and hoists them, so the pair that merges is (1,2) instead of the
// original's (0,2). Which of the three identical `lhu 0x14 / sll 16` tails
// survives is the whole function.
// Screened: 71.67% is also what 2.7.2-cdk gives (identical), so the tail
// split is a C-shape problem, not a compiler one.
/*
s32 func_801E5CD8(Model* pModel, s32 type) {
    Temp8* pTexture;

    if (type == 0) {
        pTexture = g_BattleSoundBankMain;
    } else if (type != 1) {
        if (type != 2) {
            return 2;
        }
        pTexture = ((Temp8*)pModel->pSoundBank2)->pUnk8;
    } else {
        return ((Temp8*)((Temp8*)pModel->pSoundBank1)->pUnk8)->unk14 << 16;
    }

    return pTexture->unk14 << 16;
}
*/
INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E5CD8);

// One command of the per-frame animation script that Model::pScriptCursor walks.
// The frame index at 0x0 is compared against Model::frame, the id at 0x2 selects the
// handler and also decides the command's length (0x14, 0x12, 0x1C, 0xA, 8, 6, 4).
typedef struct {
    /* 0x0  */ s16 frame;
    /* 0x2  */ u8 type;
    /* 0x3  */ u8 index;
    /* 0x4  */ u8 unk4;
    /* 0x5  */ u8 unk5;
    /* 0x6  */ u8 flags;
    union {
        // id 2: three byte scale factors and the joint's target position
        struct {
            /* 0x7  */ u8 unk7;
            /* 0x8  */ u8 unk8;
            /* 0x9  */ u8 unk9;
            /* 0xA  */ u16 unkA;
            /* 0xC  */ u16 unkC;
            /* 0xE  */ u16 unkE;
            /* 0x10 */ u16 unk10;
        } joint;
        // id 9: 16-bit component targets for the texture blend
        struct {
            /* 0x7  */ u8 unk7;
            /* 0x8  */ u16 x;
            /* 0xA  */ u16 y;
            /* 0xC  */ u16 z;
            /* 0xE  */ u16 w;
            /* 0x10 */ u16 unk10;
        } blend;
    } payload; // 0x7
    /* 0x12 */ u8 unk12;
    /* 0x13 */ u8 unk13;
    /* 0x14 */ u8 unk14;
    /* 0x15 */ u8 unk15;
    /* 0x16 */ s16 unk16;
    /* 0x18 */ s16 unk18;
    /* 0x1A */ s16 unk1A;
} ModelScriptCommand; // Size: 0x1C

void func_801E0A00(Temp8* pTexture, Temp8* pSource, u16 flags, u16 arg3, s16* pColorMatrix, s16 x, s16 y,
                   s16 arg7, s16 z, s16 w, u16 unk10, s16 arg11, s16 arg12, u32 unk13, u32 unk14, s16 unk16,
                   s16 unk18, s16 unk1A, s16 (*pInterpolate)(s16, s16, s16));

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E5D44);


// objdiff 88.82%, body complete (1488 bytes against retail's 1512). Every case
// of the 9-entry jump table is decoded and the shape is retail's: the
// `while (commandIndex < numCommands)` loop with its duplicated exit test, `break` on
// `frame != cmd->frame`, one `pScriptCursor += K` per arm (gcc merges the stores),
// case 2's 7 recomputed `g_LibGearModelOffsets[cmd->index]` sites, case 8's
// 8-model loop with both LibGearSetModelAnimation calls, case 9's if/else with
// the big arm inline, and the frame advance with the shared exit.
// ALREADY SOLVED (do not re-derive):
// - the 0x88 frame and the `lhu 0x50(sp)` that retail reads before ever writing
//   it: both come from ONE 16-byte uninitialized local (unkFlag[8] in the parked
//   body). Retail has 16 bytes of local area at 0x50..0x5C and 0x50 is the only
//   reference, so shorten it and the frame drops to 0x80 and nothing matches.
// - case 2 and case 9 do NOT share a payload view: id 2 reads 0x7/0x8/0x9 as
//   bytes and 0xA..0x10 as u16, id 9 reads 0x8..0x10 as five u16 - hence the
//   union in ModelScriptCommand. A single flat field list gives `lbu` where
//   retail has `lhu`.
// - g_LibGearCurrentModelIndex is an s16: retail loads it with `lh`.
// - LibGearClearColorEntry really takes a second argument: retail copies a2
//   into a1 before the jal, so the definition below now takes (short*, s32).
// WHAT IS LEFT (all register/scheduling, in case 8 and case 9):
// - case 8: retail loads cmd->unk5 (`lbu a3,5(cmd)`) BEFORE the NULL test and
//   compares it with `blez`; ours loads it after and uses `beqz`. Retail also
//   keeps `andi a1,<i>,0xFFFF` for the index argument where ours passes a bare
//   `move` (the explicit (u16)i cast did not bring it back).
// - case 9: retail's stack-argument stores go 0x20,0x24,0x28 (z,w,unk10) and
//   only then 0x10,0x14,0x18,0x1C; ours stores in argument order, and retail
//   loads all five u16 payload words back to back where ours interleaves the
//   flags test.
// - position of the merged `pScriptCursor += 6` block: retail puts it after case 9 (all
//   four users jump to it), ours keeps it inline in case 3/4 and jumps to the
//   +=0x1C arm instead, so the branch polarities of cases 3/4 and 9 are mirrored.
//   That is the biggest single diff and it may be a layout consequence of the
//   above. Best hand percentage 88.82%; no permuter run yet.
/*
PLACEHOLDER
    Temp8* pSource;
    s16* pColorMatrix;
    s16 (*pInterpolate)(s16, s16, s16);
    u16 x;
    u16 y;
    u16 z;
    u16 w;
    u16 modelBit;
    volatile u16 unkFlag[8]; // retail reads the first word of this slot inside case 8 before writing it
    u16 currentModelIndex;
    u16 modelBitMask;
    s32 i;

    if (pModel->frame < 0) {
        return;
    }

    while (pModel->commandIndex < pModel->numCommands) {
        pCmd = (ModelScriptCommand*)pModel->pScriptCursor;
        if (pModel->frame != pCmd->frame) {
            break;
        }

        switch (pCmd->type) {
            case 1:
                pModel->pScriptCursor += 0x14;
                break;
            case 2:
                if (pCmd->unk4 != 0) {
                    if (pCmd->index < 2) {
                        s16 modelIndex;

                        if (pCmd->unk5 != 0) {
                            modelIndex = -1;
                        } else {
                            modelIndex = pModel->modelSlot;
                        }
                        g_LibGearModelOffsets[pCmd->index].modelIndex = modelIndex;
                        g_LibGearModelOffsets[pCmd->index].jointIndex = pCmd->flags;
                        D_801E8644[pCmd->index + 1] = pCmd->payload.joint.unk7 * 16;
                        D_801E8644[pCmd->index + 4] = pCmd->payload.joint.unk8 * 16;
                        D_801E8644[pCmd->index + 7] = pCmd->payload.joint.unk9 * 16;
                        g_LibGearModelOffsets[pCmd->index].x1 = pCmd->payload.joint.unkA;
                        g_LibGearModelOffsets[pCmd->index].y1 = pCmd->payload.joint.unkC;
                        g_LibGearModelOffsets[pCmd->index].z1 = pCmd->payload.joint.unkE;
                        g_LibGearModelOffsets[pCmd->index].inUse = pCmd->payload.joint.unk10;
                    }
                    pModel->pScriptCursor += 0x12;
                } else {
                    g_LibGearModelOffsets[pCmd->index].inUse = 0;
                    pModel->pScriptCursor += 6;
                }
                break;
            case 3:
            case 4:
                LibGearClearColorEntry((short*)&pModel->pColorEntries[pCmd->index], arg2);
                if (pCmd->unk4 != 0) {
                    pModel->pScriptCursor += 0x1C;
                } else {
                    pModel->pScriptCursor += 6;
                }
                break;
            case 5:
                pModel->pScriptCursor += 8;
                break;
            case 6:
                pModel->pScriptCursor += 4;
                break;
            case 7:
                pModel->pSkeleton[pCmd->unk4].unk7 = pCmd->unk5 & 1;
                pModel->pScriptCursor += 6;
                break;
            case 8: {
                s16 flag;
                u16 savedIndex;

                flag = unkFlag[0];
                currentModelIndex = g_LibGearCurrentModelIndex;
                modelBitMask = g_LibGearModelBitMask;
                savedIndex = currentModelIndex;
                modelBit = 1 << currentModelIndex;
                for (i = 0; i < 8; i++) {
                    if (((pModel->modelBitMask >> i) & 1) && g_LibGearModels[i] != NULL && pCmd->unk5 != 0) {
                        if (flag != 0) {
                            LibGearSetModelAnimationFrom(pModel, (u16)i, (u16)modelBit, pCmd->unk5);
                        } else {
                            LibGearSetModelAnimation((u16)i, (u16)modelBit, pCmd->unk5);
                        }
                    }
                }
                g_LibGearCurrentModelIndex = savedIndex;
                g_LibGearModelBitMask = modelBitMask;
                pModel->pScriptCursor += 0xA;
                break;
            }
            case 9:
                if (pCmd->unk4 != 0) {
                    if (pCmd->index < pModel->numTextures) {
                        pSource = NULL;
                        if (pCmd->unk5 != 0xFF && pCmd->unk5 < pModel->numTextures) {
                            pSource = &pModel->pTextures[pCmd->unk5];
                        }
                        pColorMatrix = NULL;
                        if ((pCmd->flags & 0x7F) >= 4) {
                            pColorMatrix = D_801E8644;
                        }
                        pInterpolate = LibGearGetInterpolationFunc(pCmd->payload.blend.unk7);
                        x = pCmd->payload.blend.x;
                        y = pCmd->payload.blend.y;
                        z = pCmd->payload.blend.z;
                        w = pCmd->payload.blend.w;
                        if (pCmd->flags & 0x80) {
                            if (pModel->unk90 >= 0) {
                                x += pModel->blendOffsetX;
                                y += pModel->blendOffsetY;
                                if ((pCmd->unk12 >> 4) == 1) {
                                    z += pModel->blendOffsetX;
                                    w += pModel->blendOffsetY;
                                }
                            }
                        }
                        func_801E0A00(&pModel->pTextures[pCmd->index], pSource, pCmd->flags & 0x7F,
                                      pCmd->unk12 | 0x700, pColorMatrix, x, y, 0, z, w, pCmd->payload.blend.unk10, x, y,
                                      pCmd->unk13, pCmd->unk14, pCmd->unk16, pCmd->unk18, pCmd->unk1A,
                                      pInterpolate);
                    }
                    pModel->pScriptCursor += 0x1C;
                } else {
                    LibGearFlushTexture(&pModel->pTextures[pCmd->index]);
                    pModel->pScriptCursor += 6;
                }
                break;
        }

        pModel->commandIndex++;
    }

    pModel->frame++;
    if (pModel->frameCount < 0) {
        return;
    }
    if (pModel->frame < pModel->frameCount) {
        return;
    }

    pModel->frame = 0;
    pModel->commandIndex = 0;
    pModel->pScriptCursor = pModel->pScriptStart;
}
*/
void LibGearStopAnimationScript(Model* pModel) {
    pModel->frame = -1;
}

int LibGearGetModelDistance(Model* pModel) {
    int dx = pModel->posX - pModel->pSkeleton->translation.x;
    int dy = pModel->posY - pModel->pSkeleton->translation.y;
    int dz = pModel->posZ - pModel->pSkeleton->translation.z;

    return SquareRoot0(dx * dx + dy * dy + dz * dz);
}

// 95.39% with the header's Model::sourceJointIndex now s16 (it scored 93.15% while it was
// u8[2], which made the attempt below test the field's address instead of loading
// it). What already reproduces exactly: the reload of
// sourceModelSlot after the bit-scan loop, the sequential (not else-if) test chain on
// 0xFE/0xFD/0xFC/0xFA, the CompMatrix / SetRotMatrix / gte_ldv0 + gte_mvmva body
// and the 0x88 tail, including the 0x38 frame (which needs a 24-byte GTE result
// buffer, hence value[12], not value[6]).
// Remaining delta: the original computes `type - 1` TWICE from two different
// registers -- `move a1,v1` early, then `addiu v0,a1,-1` for the range test and
// `addiu a0,v1,-1` for the stored index -- so its two operands are distinct
// pseudos. Every single-local spelling tried (s16/s32/u32 local, repeated
// `pModel->sourceModelSlot` reads, a separate `rawType` copy) makes gcc 2.6 CSE them into
// one addiu, which costs one instruction and shifts every later label.
// Screened: 93.15 attempt above; 95.39 best (2.6.0), 92.13 under 2.7.2-cdk - not a preset problem.
// Round 62: 99.18% (464 bytes, the original's size). The last instruction is
// `addiu a0,v1,-1` in the original where this compiles to `move a0,v1`: gcc CSEs
// the `sourceModelSlot - 1` of the range test into the stored index. What got the body from
// 95.39 to 99.18 is the `pModel2` copy of the pointer in the 0xFD arm, which makes
// the sourceModelSlot copy land in the register retail uses (retail `lh v1` + `move a1,v1`,
// here `lh a1` + `move v1,a1`); it is an allocator workaround, not plausible
// source, and it is the only remaining delta besides that one addiu. Two permuter
// runs (790 + 30 iterations, base 231) found nothing past it, and screening the
// FD arm against a cast or against `pModel->modelSlot` directly brings it back to
// 95.39. Making the store read through pModel2 instead of pModel is the same
// 99.18 (the test's subtract and the store's are still the same pseudo).
// Round 76: 99.87%, same 464 bytes, instruction multiset identical to retail.
// The permuter put the range test through a boolean temporary (`inRange`) and
// that removed the CSE'd `sourceModelSlot - 1`; a second block around the 0xFA store
// (which jump-optimisation deletes again, but only after cse has seen it) is
// what moves the four type tests onto the load register. Left in the diff: the
// 0xFD and 0xFC tests read `a1` here and `v1` in retail, and the range subtract
// is `addiu v0,v1,-1` here against `addiu v0,a1,-1` -- registers only, no
// instruction or size delta, so this is the cse reg_eqv tie-break. Without the
// `pModel2` copy the body is 460 bytes and 98.58%. Permuter: 1389 iterations
// found base 26 -> 15 (this body), 2062 more from it found nothing better.
// Round 110 (near-miss park round): three more shapes measured, all re-park at
// 99.87%/464 bytes. (1) `pModel2 = pModel;` moved above the 0xFE test drops the
// a1 copy and the body shrinks to 460 bytes/98.58%, so the copy is born from the
// pModel2->sourceModelSlot read in the 0xFD arm and that read must stay first. (2) reading
// the range test through pModel2 (either with the FD arm on pModel or with the
// pointer kept only for `pModel2->modelSlot`) makes cse emit a SECOND `lh v1,88(s0)`
// -> 468 bytes: gcc only folds the pModel2 read into the pModel load when it is
// the first post-loop read. (3) an `s16 type = pModel->sourceModelSlot;` local, in place of
// the pModel2 read, is cse'd away entirely (no copy, 460 bytes). A fourth
// permuter run (854 iters, -j 2) from this body found nothing (base=best=15).
// Conclusion: retail's `a1` is used exactly once, by the range subtract, and its
// def sits in the 0xFE delay slot; ours puts the equivalent copy on the 0xFD/0xFC
// compares instead. Same instruction multiset, same size, only reg_eqv's choice of
// which use takes a1 differs - not reachable from source without a reload.
INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E63A8);
/*
void func_801E63A8(Model* pModel) {
    Model* pModel2;
    Model* pSource;
    MATRIX* pMatrix;
    u_short value[12];
    s32 index;
    int inRange;
    s32 jointIndex;
    Temp3* pObject;

    index = 0;
    if (pModel->sourceModelSlot == 0xFF) {
        for (index = 0; index < 8; index++) {
            if ((pModel->modelBitMask >> index) & 0x1) {
                break;
            }
        }
    }

    if (pModel->sourceModelSlot == 0xFE) {
        index = g_LibGearCurrentModelIndex;
    }
    pModel2 = pModel;
    if (pModel2->sourceModelSlot == 0xFD) {
        index = pModel2->modelSlot;
    }
    if (pModel->sourceModelSlot == 0xFC) {
        index = pModel->unk21;
    }
    if (pModel->sourceModelSlot == 0xFA) {
        if (pModel2) {
            index = 0xA;
        } else {
            index = 0xA;
        }
    }
    inRange = ((u32)(pModel->sourceModelSlot - 1)) < 0x7F;
    if (inRange) {
        index = pModel->sourceModelSlot - 1;
    }

    pSource = g_LibGearModels[index];
    if (pSource == NULL) {
        return;
    }

    if (index == pModel->modelSlot) {
        return;
    }

    jointIndex = pModel->sourceJointIndex;
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

    gte_ldv0((SVECTOR*)&pModel->localX);
    gte_mvmva(1, 0, 0, 0, 0);
    gte_stlvnl(value);

    pModel->posX = value[0];
    pModel->posY = value[2];
    pModel->posZ = value[4];

    pObject = (Temp3*)pModel->pSkeleton->pRotationObject;
    if (pObject != NULL) {
        if ((u32)(pObject->opcode - 7) < 2) {
            pObject->targetX = value[0];
            pObject->targetY = value[2];
            pObject->targetZ = value[4];
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
// the three LibGearFreeJointObject calls) reproduces the original's instruction sequence
// exactly -- i++ in the bne delay slot, pChild++ in the loop-back delay slot, and
// `li` in the first jal's delay slot -- so the loop shape is settled; it scores
// 96.10% because the allocator then swaps BOTH pairs (pJoint s1/i s0 and pSkeleton
// s5/pTargetSkeleton s4). pTarget as `pTargetSkeleton + jointIndex`, no pTarget
// local at all, and gcc 2.7.2 (try_flags.sh) all leave that map unchanged, so the
// remaining gap looks like an allocno tie-break (allocno_compare returns 0 and the
// original host's qsort ordered ties differently), not a source-level detail.
// Also tried since (all still 98.63%, same s0/s1 swap): pChild's initialiser
// moved above pJoint's, and `pChild = pChild + 1` in the loop tail.
// Screened: 98.63 best (2.6.0 and 2.7.2 -O2 alike), 93.73 under 2.7.2-cdk. Closest park in the
// unit, but not a preset problem.
// Round 165 (park repair): re-measured live at 98.63%, 240 bytes against retail's
// 240, same single s0/s1 (pJoint/i) swap and nothing else. `i = 1` hoisted by hand
// into the declarations plus `for (; i < numChildren; i++)` changes nothing.
// decomp-permuter could NOT be used on this body: its base fails to compile with
// "conversion to non-scalar type requested" at the two `(ModelJoint*)` casts
// (permuter/func_801E6578/log.txt), so the 95-99.6% class tool has not run here.
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

    LibGearFreeJointObject(pList, (s8*)pJoint->pRotationObject);
    pJoint->pRotationObject = 0;
    LibGearFreeJointObject(pList, (s8*)pJoint->pTranslationObject);
    pJoint->pTranslationObject = 0;
    LibGearFreeJointObject(pList, (s8*)pJoint->pScaleObject);
    pJoint->pScaleObject = 0;

    for (i = 1; i < numChildren; i++) {
        pChild++;

        if (pChild->pParent == pJoint) {
            func_801E6578(pList, pChild->numChildren, pSkeleton, pTargetSkeleton);
        }
    }
}
*/
INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E6578);

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

s16 LibGearComputeSignedAngle(VECTOR* pNormal, VECTOR* pVec0, VECTOR* pVec1, s32 divisor) {
    VECTOR outer;
    s32 dot;
    s32 length;

    OuterProduct12(pVec0, pVec1, &outer);
    dot = outer.vx * pNormal->vx + outer.vy * pNormal->vy + outer.vz * pNormal->vz;
    length = SquareRoot0(outer.vx * outer.vx + outer.vy * outer.vy + outer.vz * outer.vz) + 1;
    return ((dot << 4) / length << 8) / divisor;
}

int LibGearGetFirstModelIndex(void) {
    u_short bits = g_LibGearModelBitMask;
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
// Round 14: an explicit `1 << (index & 0xFF)` in the store is folded away by gcc
// exactly like the u8 conversion (92.95%): the four per-path `andi v0,a1,0xff`
// turn into copies of `li v1,1` in the jump delay slots instead.
// Screened: 92.95 identical under 2.6.0 and 2.7.2-cdk - not a compiler problem.
/*
void func_801E6830(Model* pModel, int type, s16* pMask) {
    u8 index = type;

    if (index == 0xFF) {
        index = LibGearGetFirstModelIndex();
    } else if (index == 0xFE) {
        index = *(u8*)&g_LibGearCurrentModelIndex;
    } else if (index == 0xFD || index == 0xF9) {
        index = pModel->modelSlot;
    } else if (index == 0xFC) {
        index = pModel->unk21;
    } else if (index == 0xFA) {
        index = 0xA;
    } else if (index == 0xF8) {
        index = pModel->modelSlot * 2 + 8;
    } else if (index == 0xF7) {
        index = pModel->modelSlot * 2 + 9;
    }

    *pMask = 1 << index;
}
*/
INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E6830);

void* LibGearGetAnimationEntry(Model* pModel, u8 index, s32* pFlags) {
    void* pResult;

    *pFlags = 0;
    if (index >= LIBGEAR_ANIM_ID_QUEUED_BASE) {
        u8 entry = pModel->commands[0];

        index = entry & LIBGEAR_MODEL_COMMAND_ID_MASK;
        *pFlags = entry & LIBGEAR_MODEL_COMMAND_FLAG;
    }

    if (index >= LIBGEAR_ANIM_ID_HIGH_BASE) {
        pResult = pModel->pEntryTableHigh[index - LIBGEAR_ANIM_ID_HIGH_INDEX_OFFSET];
    } else {
        pResult = pModel->pEntryTableLow[index + 1];
    }

    return pResult;
}

// objdiff 98.63%, every instruction present in the original order and the 0xA0
// frame reproduced exactly. Body: three channels selected by flags & 0x7 (0 =
// rotation 0x54, 1 = translation 0x5C, 2 = scale 0x4C), an optional base offset
// (flags & 0x20) and delta base (flags & 0x40) added to the 7 stacked position
// args, the arg4 == 0 path storing arg4/6/8 as deltas from obj->unk4/6/8, the
// arg4 < 2 write-back of the object position into the joint vector, and the
// flags & 0x80 recursion into every child whose pParent is this joint.
// Remaining delta is register identity only, four sites: gcc allocates the
// `flags` copy to s7 one instruction before the original's `andi s1,a3,0x7`
// (the original derives the channel straight from a3), the channel mask lands in
// v1 instead of a3, and the loop reload of pModel->pSkeleton lands in v1 instead
// of s1 (so its numChildren load is `lhu v0,10(v1)`).
// Tried: `u8 channel` first/last in the declarations, channel computed inline at
// each use, baseX/Y/Z declared s16 with `= 0` initialisers (that one costs 32
// bytes of frame: the early zeroing extends all six live ranges), and
// (s16)/(u16) casts on the stacked args.
/*
void func_801E6974(Model* pModel, Temp4* pList, ModelJoint* pJoint, s32 flags,
                   u8 arg4, u8 arg5, u8 arg6, s16 posX, s16 posY, s16 posZ,
                   s16 offX, s16 offY, s16 offZ, s16 arg13) {
    ModelJoint* pChild;
    Temp3* pObject;
    u16 baseX;
    u16 baseY;
    u16 baseZ;
    u16 offBaseX;
    u16 offBaseY;
    u16 offBaseZ;
    u8 channel = flags & 0x7;
    s32 i;

    if (channel == 0) {
        pObject = (Temp3*)pJoint->pRotationObject;
    } else if (channel == 1) {
        pObject = (Temp3*)pJoint->pTranslationObject;
    } else {
        pObject = (Temp3*)pJoint->pScaleObject;
    }

    if (pObject == NULL) {
        pObject = LibGearAllocJointObject(pList);
        if (pObject == NULL) {
            return;
        }
    }

    pObject->inUse = 1;
    pObject->unk1 = arg6;
    pObject->opcode = arg4 + 3;
    pObject->type = arg5;

    if (flags & 0x20) {
        if (channel == 0) {
            baseX = pJoint->rotation.vx;
            baseY = pJoint->rotation.vy;
            baseZ = pJoint->rotation.vz;
        } else if (channel == 1) {
            baseX = pJoint->translation.x;
            baseY = pJoint->translation.y;
            baseZ = pJoint->translation.z;
        } else {
            baseX = pJoint->scale.vx;
            baseY = pJoint->scale.vy;
            baseZ = pJoint->scale.vz;
        }
    } else {
        baseX = 0;
        baseY = 0;
        baseZ = 0;
    }

    if (flags & 0x40) {
        if (channel == 0) {
            offBaseX = pJoint->rotation.vx;
            offBaseY = pJoint->rotation.vy;
            offBaseZ = pJoint->rotation.vz;
        } else if (channel == 1) {
            offBaseX = pJoint->translation.x;
            offBaseY = pJoint->translation.y;
            offBaseZ = pJoint->translation.z;
        } else {
            offBaseX = pJoint->scale.vx;
            offBaseY = pJoint->scale.vy;
            offBaseZ = pJoint->scale.vz;
        }
    } else {
        offBaseX = 0;
        offBaseY = 0;
        offBaseZ = 0;
    }

    pObject->startX = posX + baseX;
    pObject->startY = posY + baseY;
    pObject->startZ = posZ + baseZ;

    if (arg4 == 0) {
        pObject->targetX = offX + offBaseX - pObject->startX;
        pObject->targetY = offY + offBaseY - pObject->startY;
        pObject->targetZ = offZ + offBaseZ - pObject->startZ;
    } else {
        pObject->targetX = offX + offBaseX;
        pObject->targetY = offY + offBaseY;
        pObject->targetZ = offZ + offBaseZ;
    }

    pObject->curFrame = 0;
    pObject->endFrame = arg13;

    if (channel == 0) {
        if (arg4 < 2) {
            pJoint->rotation.vx = pObject->startX;
            pJoint->rotation.vy = pObject->startY;
            pJoint->rotation.vz = pObject->startZ;
        }
        pJoint->pRotationObject = (u32)pObject;
    } else if (channel == 1) {
        if (arg4 < 2) {
            pJoint->translation.x = pObject->startX;
            pJoint->translation.y = pObject->startY;
            pJoint->translation.z = pObject->startZ;
        }
        pJoint->pTranslationObject = (u32)pObject;
    } else {
        if (arg4 < 2) {
            pJoint->scale.vx = pObject->startX;
            pJoint->scale.vy = pObject->startY;
            pJoint->scale.vz = pObject->startZ;
        }
        pJoint->pScaleObject = (u32)pObject;
    }

    if (flags & 0x80) {
        pChild = pModel->pSkeleton + 1;

        for (i = 1; i < pModel->pSkeleton->numChildren; i++) {
            if (pChild->pParent == pJoint) {
                func_801E6974(pModel, pList, pChild, flags & 0xFF, arg4, arg5, arg6,
                              posX, posY, posZ, offX, offY, offZ, arg13);
            }
            pChild++;
        }
    }
}
*/
INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E6974);

void LibGearSetJointFlag(Model* pModel, ModelJoint* pJoint, s32 flags) {
    ModelJoint* pChild;
    int i;

    pJoint->unk7 = flags & 0x1;

    if (flags & LIBGEAR_JOINT_FLAG_RECURSE) {
        pChild = pModel->pSkeleton;
        for (i = 1; i < pModel->pSkeleton->numChildren; i++) {
            pChild++;
            if (pChild->pParent == pJoint) {
                LibGearSetJointFlag(pModel, pChild, flags);
            }
        }
    }
}

// Appended block of the per-joint task func_80023FD8 allocates. Its byte offset
// is the s16 at 0xBE of the task (spriteData.allocatedDataSize).
typedef struct {
    /* 0x0  */ u8 unk0[0x4];
    /* 0x4  */ WorkListCallback_t pChainCallback; // replaced by LibGearJointTaskCallback
    /* 0x8  */ Model* pModel;
    /* 0xC  */ s16 jointIndex;
    /* 0xE  */ s16 useGroundY; // non-zero: the model's ground replaces the rotated offset's y
    /* 0x10 */ SVECTOR offset;
} LibGearJointTask; // Size: 0x18

// Per-joint data LibGearCreateJointTask copies into the LibGearJointTask
typedef struct {
    /* 0x0  */ u8 unk0[0x5];
    /* 0x5  */ u8 jointIndex;
    /* 0x6  */ u16 offsetX;
    /* 0x8  */ u16 offsetY;
    /* 0xA  */ u16 offsetZ;
    /* 0xC  */ u8 useGroundY;
    /* 0xD  */ u8 padD[0x6];
    /* 0x13 */ u8 followJoint; // non-zero: LibGearJointTaskCallback drives the sprite position
} LibGearJointInfo; // Size: 0x14

// Allocates a work list task plus a LibGearJointTask; returns the task
extern void* func_80023FD8(s32 index, void* pAnimFile, s32 arg2, s32 jointTaskSize);
extern void SpriteSetDirection(SpriteData* pSpriteData, s16 direction);
extern void func_800223B0(SpriteData* pSpriteData, s16 direction);
extern void SpriteSetScale(SpriteData* pSpriteData, s16 scale);
extern WorkListCallback_t WorkListTaskGetTaskCallback(WorkListEntry* pTask);
extern void TimerWorkListSetTaskCallback(WorkListEntry* pTask, WorkListCallback_t callback);
extern void LibGearJointTaskCallback(void* pTask);

void LibGearCreateJointTask(void* pAnimFile, s32 animIndex, s32 arg2, s16 direction, s16 scale, LibGearJointInfo* pInfo, Model* pModel) {
    AnimTask* pTask;
    SpriteData* pSpriteData;
    LibGearJointTask* pJointTask;

    pTask = func_80023FD8(animIndex, pAnimFile, arg2, sizeof(LibGearJointTask));
    pSpriteData = &pTask->spriteData;
    SpriteSetDirection(pSpriteData, direction);
    func_800223B0(pSpriteData, direction);
    SpriteSetScale(pSpriteData, scale);

    pJointTask = (LibGearJointTask*)((u8*)pTask + pSpriteData->allocatedDataSize);
    pJointTask->pModel = pModel;
    pJointTask->jointIndex = pInfo->jointIndex;

    if (pInfo->followJoint != 0) {
        pJointTask->pChainCallback = WorkListTaskGetTaskCallback(&pTask->task1);
        TimerWorkListSetTaskCallback(&pTask->task1, &LibGearJointTaskCallback);
        pJointTask->offset.vx = pInfo->offsetX;
        pJointTask->offset.vy = pInfo->offsetY;
        pJointTask->offset.vz = pInfo->offsetZ;
        pJointTask->useGroundY = pInfo->useGroundY;
    }
}

void LibGearJointTaskCallback(void* pTaskRaw) {
    AnimTask* pTask = pTaskRaw;
    LibGearJointTask* pJointTask;
    LVEC3 vec;
    MATRIX* pMatrix;

    pJointTask = (LibGearJointTask*)((u8*)pTask + pTask->spriteData.allocatedDataSize);
    pMatrix = (MATRIX*)PSX_SCRATCH;

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

    if (pJointTask->useGroundY != 0) {
        vec.y = pJointTask->pModel->ground;
    }

    pTask->spriteData.position.x = CONV_FROM_GTE(vec.x);
    pTask->spriteData.position.y = CONV_FROM_GTE(vec.y);
    pTask->spriteData.position.z = CONV_FROM_GTE(vec.z);

    pJointTask->pChainCallback(pTask);
}

void LibGearSetJointTransform(Model* pModel, ModelJoint* pJoint, u8 flags, s16 x, s16 y, s16 z) {
    ModelJoint* pChild;
    int i;

    if ((flags & 0x7) == 0) {
        if (flags & LIBGEAR_JOINT_FLAG_ADD) {
            pJoint->rotation.vx += x;
            pJoint->rotation.vy += y;
            pJoint->rotation.vz += z;
        } else {
            pJoint->rotation.vx = x;
            pJoint->rotation.vy = y;
            pJoint->rotation.vz = z;
        }
    } else if ((flags & 0x7) == 1) {
        if (flags & LIBGEAR_JOINT_FLAG_ADD) {
            pJoint->translation.x += x;
            pJoint->translation.y += y;
            pJoint->translation.z += z;
        } else {
            pJoint->translation.x = x;
            pJoint->translation.y = y;
            pJoint->translation.z = z;
        }
    } else {
        if (flags & LIBGEAR_JOINT_FLAG_ADD) {
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

    if (flags & LIBGEAR_JOINT_FLAG_RECURSE) {
        pChild = pModel->pSkeleton;
        for (i = 1; i < pModel->pSkeleton->numChildren; i++) {
            pChild++;
            if (pChild->pParent == pJoint) {
                LibGearSetJointTransform(pModel, pChild, flags, x, y, z);
            }
        }
    }
}

void LibGearApplyGroundHeight(Model* pModel) {
    VECTOR unk;
    SVECTOR offset;

    offset.vy = pModel->ground;
    if (pModel->skipGroundSnap == 0) {
        pModel->pSkeleton->translation.y = offset.vy;
    }
}

// Computes the global transform matrix for a certain joint?
void LibGearGetJointMatrix(MATRIX* pMatrix, void* _unk, int index, int jointIndex) {
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

void LibGearSetDirectKeyframes(u32 shouldRender) {
    g_LibGearDirectKeyframes = shouldRender & 0x1;
}

void LibGearInitialize(int capacity) {
    int i;

    g_LibGearPendingFrameCount = 0;
    g_LibGearSwingAngle = 0;
    
    LibGearInitializeJointObjectPool(&g_LibGearJointObjectPool, capacity);
    LibGearInitializeVertexBuffer(&g_LibGearVertexBuffer, LIBGEAR_VERTEX_BUFFER_CAPACITY);

    for (i = 0; i < MAX_MODELS; i++) {
        g_LibGearModels[i] = NULL;
    }

    for (i = 0; i < MAX_MODEL_MESHES; i++) {
        g_LibGearMeshes[i].pParts = NULL;
    }

    for (i = 0; i < 2; i++) {
        g_LibGearModelOffsets[i].inUse = 0;
    }
}

INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E742C);

void LibGearRenderFrame(MATRIX* pMatrix, s32 arg1, s32* pOT, s32 arg3, s32 arg4) {
    int i;
    int count;
    u16 angle;
    RECT rect;

    count = 0;
    g_LibGearPendingFrameCount += arg4 + 1;
    if (g_LibGearPendingFrameCount >= LIBGEAR_PENDING_FRAME_MAX + 1) {
        g_LibGearPendingFrameCount = LIBGEAR_PENDING_FRAME_MAX;
    }

    while (g_LibGearPendingFrameCount >= 2) {
        g_LibGearPendingFrameCount -= 2;
        count++;
    }

    angle = g_LibGearSwingAngle + count * LIBGEAR_SWING_ANGLE_STEP;
    g_LibGearSwingAngle = angle;
    g_LibGearSwingScale = (rsin((s16)angle) + 0x1000) / 800 + 4;

    for (i = 0; i < MAX_MODELS; i++) {
        if (g_LibGearModels[i] != NULL) {
            g_LibGearModels[i]->prevPosX = g_LibGearModels[i]->pSkeleton->translation.x;
            g_LibGearModels[i]->prevPosY = g_LibGearModels[i]->pSkeleton->translation.y;
            g_LibGearModels[i]->prevPosZ = g_LibGearModels[i]->pSkeleton->translation.z;
            LibGearStepModelAnimation(g_LibGearModels[i], (s32)&g_LibGearJointObjectPool, count, arg3, 1);
        }
    }

    for (i = 0; i < MAX_MODELS; i++) {
        if (g_LibGearModels[i] != NULL && g_LibGearModels[i]->parentModelSlot < 0xFF) {
            LibGearUpdateModelAttachment(g_LibGearModels[i]);
        }
    }

    LibGearRotateModelOffsets(g_LibGearModels);
    SetColorMatrix(D_801E8644);

    for (i = 0; i < MAX_MODELS; i++) {
        if (g_LibGearModels[i] != NULL) {
            g_LibGearModels[i]->posDeltaX = g_LibGearModels[i]->prevPosX - g_LibGearModels[i]->pSkeleton->translation.x;
            g_LibGearModels[i]->posDeltaY = g_LibGearModels[i]->prevPosY - g_LibGearModels[i]->pSkeleton->translation.y;
            g_LibGearModels[i]->posDeltaZ = g_LibGearModels[i]->prevPosZ - g_LibGearModels[i]->pSkeleton->translation.z;
            func_801DCEC8(g_LibGearModels[i], pMatrix, arg1, 1, 1, pOT, arg3);
        }
    }

    LibGearRenderVertices(&g_LibGearVertexBuffer, pMatrix, count, pOT, arg3);
}

void LibGearShutdown(void) {
    int i;

    for (i = 0; i < MAX_MODELS; i++) {
        LibGearFreeModel(i);
    }

    LibGearFreeJointObjectPool(&g_LibGearJointObjectPool);
    LibGearFreeVertexBuffer(&g_LibGearVertexBuffer);
}

// Frees every resource a model owns and clears its slot. The texture and list
// entries are arrays of Temp8 / Temp9 structs, not byte blobs.
void LibGearFreeModel(s32 modelIndex) {
    s32 i;

    if (g_LibGearModels[modelIndex] == NULL) {
        return;
    }

    if (g_LibGearModels[modelIndex]->pModelFile != NULL) {
        HeapFree(g_LibGearModels[modelIndex]->pModelFile);
        LibGearFreeModelMesh(g_LibGearModels[modelIndex]->pMesh, 1);
    }

    if (g_LibGearModels[modelIndex]->hasSedsEntry != 0) {
        SoundRemoveSedsEntry(((Temp8*)g_LibGearModels[modelIndex]->pSoundBank1)->pUnk8);
    }

    if (g_LibGearModels[modelIndex]->unkAC != NULL) {
        HeapFree(g_LibGearModels[modelIndex]->unkAC);
    }

    if ((u32)(modelIndex - MAX_MODEL_MESHES) >= 2) {
        if (g_LibGearModels[modelIndex]->pSkeleton != NULL) {
            Temp4* pPool = &g_LibGearJointObjectPool;

            LibGearReleaseSkeletonObjects(pPool, g_LibGearModels[modelIndex]->pSkeleton);
            LibGearReleaseSkeletonObjectsByType(pPool, g_LibGearModels[modelIndex]->pSkeleton, 0xFF);
            LibGearFreeModelSkeleton(g_LibGearModels[modelIndex]->pSkeleton);
            g_LibGearModels[modelIndex]->pMesh = NULL;
            g_LibGearModels[modelIndex]->pSkeleton = NULL;
        }
    } else {
        if (g_LibGearModels[modelIndex]->pSkeleton != NULL) {
            Temp4* pPool = &g_LibGearJointObjectPool;

            LibGearReleaseSkeletonObjects(pPool, g_LibGearModels[modelIndex]->pSkeleton);
            LibGearReleaseSkeletonObjectsByType(pPool, g_LibGearModels[modelIndex]->pSkeleton, 0xFF);
            HeapFree(g_LibGearModels[modelIndex]->pSkeleton);
        }
    }

    if (g_LibGearModels[modelIndex]->numColorEntries != 0) {
        HeapFree(g_LibGearModels[modelIndex]->pColorEntries);
    }

    if (g_LibGearModels[modelIndex]->numTextures != 0) {
        for (i = 0; i < g_LibGearModels[modelIndex]->numTextures; i++) {
            LibGearFlushTexture(&g_LibGearModels[modelIndex]->pTextures[i]);
        }
        HeapFree(g_LibGearModels[modelIndex]->pTextures);
    }

    if (g_LibGearModels[modelIndex]->numModelListEntries != 0) {
        for (i = 0; i < g_LibGearModels[modelIndex]->numModelListEntries; i++) {
            LibGearFreeModelEntry(&g_LibGearModels[modelIndex]->pModelListEntries[i]);
        }
        HeapFree(g_LibGearModels[modelIndex]->pModelListEntries);
    }

    HeapFree(g_LibGearModels[modelIndex]);
    g_LibGearModels[modelIndex] = NULL;
}

void LibGearSetModelAnimation(u16 modelIndex, u16 bits, s32 arg2) {
    Model* pModel = g_LibGearModels[modelIndex];

    g_LibGearCurrentModelIndex = modelIndex;
    g_LibGearModelBitMask = bits;
    pModel->unk35 = 0;

    if (g_LibGearModels[modelIndex] != NULL) {
        LibGearSetAnimation(g_LibGearModels[modelIndex], g_LibGearModels[modelIndex], &g_LibGearJointObjectPool, arg2);
    }
}

void LibGearSetModelAnimationFrom(s32 arg0, u16 modelIndex, u16 arg2, s32 arg3) {
    g_LibGearCurrentModelIndex = modelIndex;
    g_LibGearModelBitMask = arg2;
    g_LibGearModels[modelIndex]->unk35 = 0;

    if (g_LibGearModels[modelIndex] != NULL) {
        if (modelIndex != LibGearGetFirstModelIndex()) {
            LibGearSetAnimation(g_LibGearModels[modelIndex], arg0, &g_LibGearJointObjectPool, arg3);
        }
    }
}

int LibGearGetModelHeight(int modelIndex) {
    Model* pModel = g_LibGearModels[modelIndex];
    int result = 0;

    if (pModel != NULL) {
        result = (pModel->height * ((pModel->scale * pModel->pSkeleton->scale.vy) >> 0xC)) >> 0xC;
    }

    return result;
}

int LibGearGetModelWidth(int modelIndex) {
    int factor;
    int jointScale;
    int result = 0;

    if (g_LibGearModels[modelIndex] != NULL) {
        if (g_LibGearModels[modelIndex]->flags & LIBGEAR_MODEL_FLAG_USE_WIDTH) {
            jointScale = g_LibGearModels[modelIndex]->pSkeleton->scale.vx;
            factor = g_LibGearModels[modelIndex]->width;
        } else {
            jointScale = g_LibGearModels[modelIndex]->pSkeleton->scale.vz;
            factor = g_LibGearModels[modelIndex]->depth;
        }

        result = factor * ((g_LibGearModels[modelIndex]->scale * jointScale) >> 0xC) >> 0xC;
    }

    return result;
}

void LibGearAllocateColorEntries(Model* pModel) {
    int i;
    UnkInner1* pObjects;

    if (pModel->numColorEntries) {
        pObjects = HeapAlloc(pModel->numColorEntries * sizeof(UnkInner1), 0x0);
        for (i = 0; i < pModel->numColorEntries; i++) {
            pObjects[i].unk0 = -1;
            pObjects[i].unk8 = 0;
        }
        pModel->pColorEntries = pObjects;
    }
}
