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

// Animation ids below this one index Model::pAnimTable directly; from it up they come from
// the secondary wad, at index id - LIBGEAR_ANIM_WAD_INDEX_OFFSET
#define LIBGEAR_ANIM_ID_WAD_BASE 0x50
#define LIBGEAR_ANIM_WAD_INDEX_OFFSET 0x4E

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

// Model::parentModelSlot value meaning the model inherits no parent transform
#define LIBGEAR_MODEL_SLOT_NONE 0xFF

// LibGearReleaseJointObjects flags: which queued object to release
#define LIBGEAR_JOINT_RELEASE_ROTATION 0x1
#define LIBGEAR_JOINT_RELEASE_TRANSLATION 0x2
#define LIBGEAR_JOINT_RELEASE_SCALE 0x4

// LibGearSetJointTransform / LibGearSetJointFlag flags
#define LIBGEAR_JOINT_FLAG_ADD 0x20 // add to the joint's value instead of setting it
#define LIBGEAR_JOINT_FLAG_RECURSE 0x80 // apply to the joint's children too

// Which of the joint's vectors LibGearSetJointTransform drives; anything else is the scale
#define LIBGEAR_JOINT_CHANNEL_MASK 0x7
#define LIBGEAR_JOINT_CHANNEL_ROTATION 0
#define LIBGEAR_JOINT_CHANNEL_TRANSLATION 1

// Interpolation curves LibGearGetInterpolationFunc picks by type; 0 (the default) is the sine
#define LIBGEAR_INTERPOLATE_RISE 1
#define LIBGEAR_INTERPOLATE_FALL 2
#define LIBGEAR_INTERPOLATE_FALL_CLAMPED 3

// Top of the frame range LibGearInterpolateRise and LibGearInterpolateFallClamped work in
#define LIBGEAR_INTERPOLATE_FRAME_MAX 32

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
extern u16 g_LibGearCurrentModelIndex;
extern s32 g_LibGearPendingFrameCount;
extern s16 g_LibGearSwingAngle;
extern s16 g_LibGearSwingScale;

void func_801DCEC8(Model* pModel, MATRIX* pLightMatrix, MATRIX* pViewMatrix, s32 renderMode, s32 frameStep, u_long* pOt, s32 packetIndex);
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
    pSkeleton[0].doRender = 0;
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
        pCurJoint->doRender = 1;
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

// Model::pColorEntries entry (0x70 stride). UnkInner1 in libgear/main.h has the
// same layout under its own field names; this view exists for the parked body
// below: the joint it follows, the vertex it drives and the projected
// coordinates the previous frame left behind.
typedef struct {
    /* 0x00 */ s16 jointIndex; // joint whose transform projects the two sources; negative = free
    /* 0x02 */ u8 is3D; // non-zero: the sources are projected in 3D and pass through the GTE
    /* 0x03 */ u8 isSemiTrans; // handed to LibGearAllocVertex
    /* 0x04 */ VertexBuffer* pVertexBuffer; // buffer the vertex is allocated from
    /* 0x08 */ Vertex* pVertex; // the vertex this entry drives; 0 = none allocated yet
    /* 0x0C */ SVECTOR sourceA;
    /* 0x14 */ SVECTOR sourceB;
    /* 0x1C */ u16 screenA[16]; // projected coordinates, two 0x10 byte slots of 4 quads
    /* 0x3C */ u16 screenB[16];
    /* 0x5C */ s16 slot; // the quad the next projection writes; counts down and wraps at 8
    /* 0x5E */ u16 age; // frames drawn; 0 after the wrap retires the entry
    /* 0x60 */ s16 numFrames; // age the entry is driven to before the vertex is recycled
    /* 0x62 */ u16 lifetime; // copied into the vertex below
    /* 0x64 */ u16 red;
    /* 0x66 */ u16 green;
    /* 0x68 */ u16 blue;
    /* 0x6A */ u16 redDelta;
    /* 0x6C */ u16 greenDelta;
    /* 0x6E */ u16 blueDelta;
} ColorSlot; // Size: 0x70

extern s16 func_801E1258(Temp8* pTexture, s32 frameStep);
extern void func_801E22F8(Temp9* pEntry, SVECTOR* pVector, MATRIX* pLightMatrix, u_long* pOt, s32 packetIndex, s32 scale, s16 ground);

// PARK, round 281: objdiff 96.88%, 3388 bytes against retail's 3376 (+3 instructions:
// one pSkeleton reload in the entry block, the `otz` memory spill, and the
// model-list loop's cursor order). 158 of 860 rows still differ, and the great
// majority of those are saved-register renames between two equivalent maps.
// CLOSED THIS ROUND, each measured on its own:
//  - the 2D/3D quad index wants a NARROW local, not `s32 slot`: `u16 quadSlot` (the
//    `s32` version left it in a callee-saved register). 94.35 -> 94.96%.
//  - the declaration order of the scratch pointers sets the spilled-slot order:
//    declaring `pScratch/pViewScratch/pLightScratch` AFTER `modelScale` moves
//    pSkeleton's home to 0x78, retail's, instead of 0x80.
//  - retail reads `pModel->pSkeleton` DIRECTLY (a fresh `lw 0x4(s5)`) at two phase-1
//    sites - `&pModel->pSkeleton[1].globalTransform` and the
//    `pModel->pSkeleton->translation.y` of the depth clamp - while the joint-loop and
//    phase-3/5 uses go through the local. Writing the local at those two sites is
//    what kept it alive across the whole of phase 1. 95.49 -> 96.46% together with
//    the t[] order below.
//  - `pViewScratch->t[]`'s source order is t[0], t[1], t[2] (trans.vx, ground,
//    trans.vz); the "obvious" t[0], t[2], t[1] order is 95.81.
//  - the 3D colour path holds TWO bank pointers (retail `addiu a0,v1,0x1c` and
//    `addiu v1,v1,0x3c` off one `8*(1-slot)` scale): `pBankA = &pEntry->screenA[8*(1-slot)]`
//    and `pBankB = &pEntry->screenB[8*(1-slot)]`, not six indexed reads off one base.
//    94.96 -> 95.47%.
//  - `pMesh = pModel->pMesh;` assigned just before the joint loop (not in the entry
//    block) frees a register there: 96.68 -> 96.88%. Moving `numChildren` the same
//    way is 96.12 (worse) and swapping scale/numChildren is 96.46.
//  - joint/section loops: `for (...; i++, p++)` - the increments in the for-clause,
//    not `p++` at the end of the body - is retail's latch order.
//  - the light-colour else-arm is m[0][1]=0; m[1][1]=scale; m[2][1]=0 in that order
//    (retail stores m[2][1] into the branch delay slot). 96.49 -> 96.68%.
// CLOSED IN ROUND 280, kept for the record:
//  - the corner depth read-back is `gte_stszotz(&depthBuf[0/1])`, NOT a `gte_stsz3_val`
//    plus a shift: gte_stszotz already carries mfc2 $19 / nop / sra $12,$12,2 / sw
//    through the address operand, which is retail's exact four-instruction shape and
//    why retail materialises sp+0x48 and sp+0x4C once each. 87.94 -> 90.05%, and the
//    six missing instructions (four `mfc2` nops plus two) come from it. The round-265
//    note's "a second, nop-bearing SZ3 macro is needed" is WRONG - no header change.
//  - the OT index is its own register value, not a `depthBuf[0] >>=` write-back:
//    `otz = depthBuf[0] >> g_GfxOTDepthShift;` and then `pOt[otz]` twice. That keeps
//    &pOt[otz] in one register across both pokes (retail's a1) instead of re-forming
//    the address from a memory reload. 90.05 -> 91.57%.
//  - `pPoly = &pModel->polys[packetIndex];` is assigned AFTER the first
//    `gte_ldv0/gte_rtps`, immediately before the first `gte_stsxy2`: retail computes
//    the packet pointer in the RTPS shadow (its `lw t2,0xF8(sp)` sits right after
//    RTPS) and a hoisted assignment costs a 12-row block. 91.57 -> 93.30%.
//  - the 3D path's `pSection` was a `Temp9Section*`, so `pSection + 4` scaled by 0x10:
//    retail's `lhu 0x4(a0)` is `screenA[8 * (1 - quadSlot) + 2]`. Write the six reads
//    through the u16 arrays. 93.30 -> 94.07%.
//  - the root-joint CompMatrix is IN PLACE: retail materialises 0x1F800020 in BOTH a1
//    and a2 (two independent lui/ori pairs, at 0x1034 and in 0x1068's delay slot), so
//    the call is `CompMatrix(pLightMatrix, pViewScratch, pViewScratch)`, not
//    `...&pSkeleton->globalTransform, pViewScratch`. 94.07 -> 94.30%.
//  - ONE loop variable serves all three loops (retail resets `$s4` with
//    `addu s4,zero,zero` at each head and increments `s4` at the joint loop's latch);
//    dropping the separate `jointIndex` local is 94.30 -> 94.35, and its only other
//    effect is to move the permutation around.
// STILL OPEN, in the order the measurements point (all three are ALLOCATOR calls, and
// the saved set and the 0xE0 frame match retail exactly, so none of them is
// over-allocation):
//  - pSkeleton has no callee-saved home in the entry block: ours reloads
//    `lw t0,0x78(sp)` right after storing it, where retail keeps it in $s2 for the
//    whole of phase 1 and only reloads in phase 3/5. `pModel->pSkeleton` is already
//    used at the two sites where retail loads it fresh (see above), so what is left is
//    the pseudo's live range against 9 saved registers. Measured dead: moving
//    numChildren out of the entry block (96.12), swapping scale/numChildren (96.46),
//    every declaration order of the pointer locals.
//  - the OT tail keeps `otz` in a register where retail spills it to 0x48(sp) (the
//    same slot as depthBuf[0], so retail's source may have shifted depthBuf[0] in
//    place - but `depthBuf[0] >>= shift` and every other write-back spelling costs
//    28 bytes and 0.6 points, so the register form below is the best measured).
//  - the model-list loop's two `addiu ..,0x24` cursors and the packet-buffer `addu`
//    operand order (`addu v0,v0,s2` in retail, `addu v0,s2,v0` here) are the last
//    ordering differences: 4 rows.
// The permuter DOES parse this body now (`&vec`-style macro args; base=5173) and found
// the 2D `u16 quadSlot` edit in 5 minutes, so run it again from this body rather than
// hand-iterating. Un-park by deleting the INCLUDE_ASM line and the enclosing comment.
void func_801DCEC8(Model* pModel, MATRIX* pLightMatrix, MATRIX* pViewMatrix, s32 renderMode, s32 frameStep, u_long* pOt, s32 packetIndex) {
    ModelJoint* pSkeleton;
    ModelMesh* pMesh;
    ModelJoint* pJoint;
    Temp9* pListEntry;
    Temp8* pTexture;
    ColorSlot* pEntry;
    Temp9Section* pSection;
    u8* pQuadA;
    u8* pQuadB;
    u16* pBankA;
    u16* pBankB;
    POLY_FT4* pPoly;
    SVECTOR svec;
    VECTOR vec;
    s32 quadIndex;
    VECTOR trans;
    s32 depthBuf[2]; // [0] shallowest corner depth, [1] the corner just projected
    VECTOR proj;
    SVECTOR swingVec;
    ModelMesh* pMeshLoaded;
    u16 modelScale;
    MATRIX* pScratch = (MATRIX*)PSX_SCRATCH;
    MATRIX* pViewScratch = &pScratch[1];
    MATRIX* pLightScratch = &pScratch[2];
    s32 depth;
    s32 otz;
    u16 quadSlot; // the quad bank the 2D/3D paths pick from the entry's slot
    u_int numChildren;
    s32 i;
    s32 j;

    if (pModel->doUpdate == 0) {
        return;
    }

    pSkeleton = pModel->pSkeleton;
    pMeshLoaded = pModel->pMesh;
    modelScale = pModel->scale;
    numChildren = pSkeleton->numChildren;

    CompMatrix(pLightMatrix, &pSkeleton->localTransform, pLightScratch);

    if ((pModel->flags & 1) == 0) {
        CompMatrix(&pSkeleton->localTransform, &pModel->pSkeleton[1].globalTransform, pScratch);
        SetRotMatrix(pScratch);
        SetTransMatrix(pScratch);

        svec.vx = 0;
        svec.vy = 0;
        svec.vz = 0x1000;
        gte_ldv0(&svec);
        gte_mvmva(1, 0, 0, 0, 0);
        gte_stlvnl(&vec);

        svec.vx = 0;
        svec.vy = 0;
        svec.vz = 0;
        gte_ldv0(&svec);
        gte_mvmva(1, 0, 0, 0, 0);
        gte_stlvnl(&trans);

        svec.vy = -ratan2(vec.vz - trans.vz, vec.vx - trans.vx);
        svec.vx = 0;
        svec.vz = 0;
        RotMatrix(&svec, pViewScratch);

        pViewScratch->t[0] = trans.vx;
        pViewScratch->t[1] = pModel->ground;
        pViewScratch->t[2] = trans.vz;

        // Retail composes the light matrix with the view scratch in place, not
        // with the root joint's globalTransform: the call site carries two
        // materialised 0x1F800020 immediates (a1 and a2).
        CompMatrix(pLightMatrix, pViewScratch, pViewScratch);

        depth = pModel->scale - (pModel->ground - pModel->pSkeleton->translation.y) / 4;
        if (depth < 0) {
            depth = 0;
        }

        pScratch->m[0][0] = depth;
        pScratch->m[0][1] = 0;
        pScratch->m[0][2] = 0;
        pScratch->m[1][0] = 0;
        pScratch->m[1][1] = depth;
        pScratch->m[1][2] = 0;
        pScratch->m[2][0] = 0;
        pScratch->m[2][1] = 0;
        pScratch->m[2][2] = depth;

        MulMatrix(pViewScratch, pScratch);
        SetRotMatrix(pViewScratch);
        SetTransMatrix(pViewScratch);

        svec.vx = pModel->width;
        svec.vy = 0;
        svec.vz = pModel->depth;
        gte_ldv0(&svec);
        gte_rtps();
        pPoly = &pModel->polys[packetIndex];
        gte_stsxy2((u_long*)&pPoly->x0);
        gte_stszotz(&depthBuf[0]);

        svec.vx = -pModel->width;
        gte_ldv0(&svec);
        gte_rtps();
        gte_stsxy2((u_long*)&pPoly->x1);
        gte_stszotz(&depthBuf[1]);
        if (depthBuf[1] < depthBuf[0]) {
            depthBuf[0] = depthBuf[1];
        }

        svec.vx = pModel->width;
        svec.vz = -pModel->depth;
        gte_ldv0(&svec);
        gte_rtps();
        gte_stsxy2((u_long*)&pPoly->x2);
        gte_stszotz(&depthBuf[1]);
        if (depthBuf[1] < depthBuf[0]) {
            depthBuf[0] = depthBuf[1];
        }

        svec.vx = -pModel->width;
        gte_ldv0(&svec);
        gte_rtps();
        gte_stsxy2((u_long*)&pPoly->x3);
        gte_stszotz(&depthBuf[1]);
        if (depthBuf[1] < depthBuf[0]) {
            depthBuf[0] = depthBuf[1];
        }

        otz = depthBuf[0];
        otz >>= g_GfxOTDepthShift;
        pModel->polys[packetIndex].tag = (pModel->polys[packetIndex].tag & 0xFF000000) | (pOt[otz] & 0xFFFFFF);
        pOt[otz] = pOt[otz] | ((u32)pPoly & 0xFFFFFF);
        if (otz >= 0x2D9) {
            otz = 0x2D8;
        }
        pModel->unk39 = 0x6B - otz / 8;
    }

    MulMatrix0(pViewMatrix, &pSkeleton->globalTransform, pViewScratch);

    pMesh = pMeshLoaded;
    pJoint = pSkeleton + 1;
    for (i = 1; i < numChildren; i++, pJoint++) {
        if (pJoint->jointIndex != LIBGEAR_JOINT_INDEX_NONE && pJoint->doRender != 0) {
            MulMatrix0(pViewScratch, &pJoint->globalTransform, pScratch);
            SetLightMatrix(pScratch);
            CompMatrix(pLightScratch, &pJoint->globalTransform, pScratch);

            if (pJoint->scale.pad > 0) {
                pScratch->m[0][0] = pModel->scale;
                pScratch->m[0][2] = 0;
                pScratch->m[1][0] = 0;
                pScratch->m[1][2] = 0;
                pScratch->m[2][0] = 0;
                pScratch->m[2][2] = pModel->scale;

                if (pJoint->scale.pad == 1) {
                    pScratch->m[0][1] = pLightScratch->m[0][1];
                    pScratch->m[1][1] = pLightScratch->m[1][1];
                    pScratch->m[2][1] = pLightScratch->m[2][1];
                } else {
                    pScratch->m[0][1] = 0;
                    pScratch->m[1][1] = pModel->scale;
                    pScratch->m[2][1] = 0;
                }
            }

            SetRotMatrix(pScratch);
            SetTransMatrix(pScratch);

            ModelRenderPacket(
                pMesh->pParts[pJoint->jointIndex],
                ((void**)&pJoint->pModelPacketBuffer)[packetIndex],
                pOt,
                renderMode
            );
        }
    }

    pListEntry = pModel->pModelListEntries;
    for (i = 0; i < pModel->numModelListEntries; pListEntry++, i++) {
        if (pListEntry->jointIndex < 0) {
            continue;
        }

        swingVec.vx = -g_LibGearSwingScale * rsin(pSkeleton->rotation.vy + 0x400) / 0x1000;
        swingVec.vz = g_LibGearSwingScale * rcos(pSkeleton->rotation.vy + 0x400) / 0x1000;
        swingVec.vy = pModel->unk3E;

        CompMatrix(&pSkeleton->localTransform, &pSkeleton[pListEntry->jointIndex].globalTransform, pScratch);
        SetRotMatrix(pScratch);
        SetTransMatrix(pScratch);

        for (j = 0; j < pListEntry->numVectors; j++) {
            gte_ldv0(&pListEntry->pVectors[j]);
            gte_mvmva(1, 0, 0, 0, 0);
            gte_stlvnl(&proj);

            pListEntry->pPoints[j]->x = proj.vx;
            pListEntry->pPoints[j]->y = proj.vy;
            pListEntry->pPoints[j]->z = proj.vz;
        }

        pSection = pListEntry->pSections;
        for (j = 0; j < pListEntry->numSections; j++, pSection++) {
            CompMatrix(&pSkeleton->localTransform, &pSkeleton[pSection->vector.pad].globalTransform, pScratch);
            SetRotMatrix(pScratch);
            SetTransMatrix(pScratch);

            gte_ldv0(&pSection->vector);
            gte_mvmva(1, 0, 0, 0, 0);
            gte_stlvnl(&proj);

            pSection->x = proj.vx;
            pSection->y = proj.vy;
            pSection->z = proj.vz;
        }

        func_801E22F8(pListEntry, &swingVec, pLightMatrix, pOt, packetIndex, modelScale, pModel->ground);
    }

    pTexture = pModel->pTextures;
    for (i = 0; i < pModel->numTextures; i++, pTexture++) {
        func_801E1258(pTexture, frameStep);
    }

    pEntry = (ColorSlot*)pModel->pColorEntries;
    for (i = 0; i < pModel->numColorEntries; i++) {
        if (pEntry->jointIndex < 0) {
            pEntry++;
            continue;
        }

        pEntry->slot = (pEntry->slot - 1) & 7;

        if (pEntry->is3D == 0) {
            CompMatrix(pLightScratch, &pSkeleton[pEntry->jointIndex].globalTransform, pScratch);
            SetRotMatrix(pScratch);
            SetTransMatrix(pScratch);

            gte_ldv0(&pEntry->sourceA);
            gte_rtps();
            gte_stsxy2((u_long*)&pEntry->screenA[2 * pEntry->slot]);

            gte_ldv0(&pEntry->sourceB);
            gte_rtps();
            gte_stsxy2((u_long*)&pEntry->screenB[2 * pEntry->slot]);

            pEntry->age++;
            if ((s16)pEntry->age == 0) {
                continue;
            }

            if (pEntry->numFrames < (s16)pEntry->age || pEntry->pVertex == NULL) {
                pEntry->age = 1;
                pEntry->pVertex = LibGearAllocVertex(pEntry->pVertexBuffer, pEntry->isSemiTrans);
                pEntry->pVertex->is3D = 0;
                pEntry->pVertex->age = 0;
                pEntry->pVertex->lifetime = pEntry->lifetime;
                pEntry->pVertex->red = pEntry->red;
                pEntry->pVertex->green = pEntry->green;
                pEntry->pVertex->blue = pEntry->blue;
                pEntry->pVertex->redDelta = pEntry->redDelta;
                pEntry->pVertex->greenDelta = pEntry->greenDelta;
                pEntry->pVertex->blueDelta = pEntry->blueDelta;

                quadIndex = (pEntry->slot + pEntry->age) & 7;
        quadSlot = quadIndex;
                pEntry->pVertex->x0 = pEntry->screenA[2 * quadSlot];
                pEntry->pVertex->y0 = pEntry->screenA[2 * quadSlot + 1];
                pEntry->pVertex->x2 = pEntry->screenB[2 * quadSlot];
                pEntry->pVertex->y2 = pEntry->screenB[2 * quadSlot + 1];
            }

            pEntry->pVertex->x1 = pEntry->screenA[2 * pEntry->slot];
            pEntry->pVertex->y1 = pEntry->screenA[2 * pEntry->slot + 1];
            pEntry->pVertex->x3 = pEntry->screenB[2 * pEntry->slot];
            pEntry->pVertex->y3 = pEntry->screenB[2 * pEntry->slot + 1];
        } else {
            otz = pEntry->jointIndex;
            CompMatrix(&pSkeleton->localTransform, &pSkeleton[otz].globalTransform, pScratch);
            SetRotMatrix(pScratch);
            SetTransMatrix(pScratch);

            quadSlot = pEntry->slot & 1;
            pQuadA = (u8*)pEntry + 0x1C + 0x10 * quadSlot;
            pQuadB = (u8*)pEntry + 0x3C + 0x10 * quadSlot;

            gte_ldv0(&pEntry->sourceA);
            gte_mvmva(1, 0, 0, 0, 0);
            gte_stlvnl(pQuadA);

            gte_ldv0(&pEntry->sourceB);
            gte_mvmva(1, 0, 0, 0, 0);
            gte_stlvnl(pQuadB);

            pEntry->age++;
            if ((s16)pEntry->age == 0) {
                continue;
            }

            if (pEntry->numFrames < (s16)pEntry->age || pEntry->pVertex == NULL) {
                pEntry->age = 1;
                pEntry->pVertex = LibGearAllocVertex(pEntry->pVertexBuffer, pEntry->isSemiTrans);
                pEntry->pVertex->is3D = 1;
                pEntry->pVertex->age = 0;
                pEntry->pVertex->lifetime = pEntry->lifetime;
                pEntry->pVertex->red = pEntry->red;
                pEntry->pVertex->green = pEntry->green;
                pEntry->pVertex->blue = pEntry->blue;
                pEntry->pVertex->redDelta = pEntry->redDelta;
                pEntry->pVertex->greenDelta = pEntry->greenDelta;
                pEntry->pVertex->blueDelta = pEntry->blueDelta;

                pBankA = &pEntry->screenA[8 * (1 - quadSlot)];
                pBankB = &pEntry->screenB[8 * (1 - quadSlot)];

                pEntry->pVertex->x0 = pBankA[0];
                pEntry->pVertex->y0 = pBankA[2];
                pEntry->pVertex->z0 = pBankA[4];
                pEntry->pVertex->x2 = pBankB[0];
                pEntry->pVertex->y2 = pBankB[2];
                pEntry->pVertex->z2 = pBankB[4];
            }

            pEntry->pVertex->x1 = *(u16*)(pQuadA + 0);
            pEntry->pVertex->y1 = *(u16*)(pQuadA + 4);
            pEntry->pVertex->z1 = *(u16*)(pQuadA + 8);
            pEntry->pVertex->x3 = *(u16*)(pQuadB + 0);
            pEntry->pVertex->y3 = *(u16*)(pQuadB + 4);
            pEntry->pVertex->z3 = *(u16*)(pQuadB + 8);
        }

        pEntry++;
    }
}


// Decoded from the .s, verified against the near-twin function LibGearRenderModel
// in this file (its joint loop is already matched - start from it):
//
// PARK_NOTE 1 — signature (7 params, frame 0xE0, no gp use): 'void func_801DCEC8(Model* pModel,
// MATRIX* pLightMatrix, MATRIX* pViewMatrix, s32 renderMode, s16 frameStep, u_long* pOt, s32
// packetIndex)'.
// PARK_NOTE 2 — early exit: 'if (pModel->doUpdate == 0) return;' ('lbu 0x34' / beqz to the
// epilogue at 0x1BC4).
// PARK_NOTE 3 — scratch matrices: 0x1F800000, 0x1F800020, 0x1F800040 are MATRIX* bases (use the
// existing 'PSX_SCRATCH' idiom as 'LibGearRenderModel' at main.c:390 does), plus a fourth
// scratch whose base register is the literal constant 0x1F80000E, used for SetLightMatrix and
// CompMatrix output inside the joint loop — that odd base is retail's, not a typo.
// PARK_NOTE 4 — phase 1 (0xEC8-0x1310), root/packet setup, no known C yet:
// CompMatrix(pLightMatrix, &pSkeleton->localTransform, scratch40); if '!(pModel->flags & 1)'
// then CompMatrix(&pSkeleton->localTransform, &pSkeleton[1].globalTransform, scratch00) +
// SetRotMatrix/SetTransMatrix(scratch00), then a GTE projection block: two MVMVA with swc2
// $25-$27 stores (the 3.x macro family, 'gte_stlvnl'), 'ratan2' of the projected deltas, 'neg'
// into an SVECTOR at sp+0x20, 'RotMatrix(&svec, scratch20)', writes to 0x1F800034/38/3C,
// 'CompMatrix(pLightMatrix, &pSkeleton->globalTransform, scratch20)', then a signed divide-by-4
// clamp: 'v = pModel->scale - ((pModel->ground - pSkeleton->translation.y) / 4); if (v < 0) v =
// 0;' stored as the diagonal of a scratch matrix, then 'MulMatrix(scratch20, scratch00)',
// SetRotMatrix/SetTransMatrix(scratch20).
// PARK_NOTE 5 — phase 1b (0x1110-0x1310), bounding-box corners + OT link: four 'RTPS' calls load
// SVECTORs at sp+0x20 built from 'pModel->width'/'pModel->depth' (offsets 0x26/0x28,
// sign-flipped per corner), store SXY2 with 'swc2 $14' into 'pModel->polys[packetIndex]' at
// +8/+0x10/+0x18/+0x20, read SZ3 with 'mfc2 $19' + 'sra 2' (⇒ the 'gte_stsz3_val'-style
// read-back) and keep the max in a stack local at 0x48(sp). Then the OT link: 'word = (prim.tag
// & 0xFF000000) | (otEntry & 0x00FFFFFF)' and the mirror poke on the OT slot, 'if (otz >= 0x2D8)
// otz = 0x2D8;', and 'pModel->unk39 = 0x6B - (otz / 8)' (0x2D8, 0x6B and the '+7' before 'sra 3'
// are all visible here — a constant round will want names).
// PARK_NOTE 6 — phase 2 (0x1314): 'MulMatrix0(pViewMatrix, &pSkeleton->globalTransform,
// scratch20);' then the joint loop 'for (i = 1; i < pSkeleton->numChildren; i++)' with 'if
// (pJoint->jointIndex != 0xFFFF)': MulMatrix0(scratch20, &pJoint->globalTransform,
// lightScratch); SetLightMatrix(lightScratch); CompMatrix(scratch ?, &pJoint->globalTransform,
// lightScratch); 'if (pJoint->rotation.w? (lh 0x4A) > 0)' writes the light colour halfwords at
// 0x1F800002/8/E from scratch+2/+8/+0xE else zeros, then SetRotMatrix/SetTransMatrix and
// 'ModelRenderPacket(pMesh->pParts[pJoint->jointIndex],
// ((void**)&pJoint->pModelPacketBuffer)[packetIndex], pOt, renderMode)'. **This loop is the
// near-twin of the already-matched 'LibGearRenderModel' (main.c:390) — start the body from that
// function.**
// PARK_NOTE 7 — phase 3 (0x1450-0x16B4), model-list loop: 'for (i = 0; i <
// pModel->numModelListEntries; i++)' over 'Temp9 pModel->pModelListEntries' (stride 0x24),
// skipped when 'entry->unk0 < 0'. Per entry: an SVECTOR at sp+0x60 = '{
// (s16)((-g_LibGearSwingScale * rsin(*(s16*)(entry+0x56) + 0x400)) >> 12), pModel->unk3E,
// (s16)((g_LibGearSwingScale * rcos(...)) >> 12) }' (note the '+0xFFF' before 'sra 12'
// signed-divide fixup); 'CompMatrix(&pSkeleton->localTransform,
// &pSkeleton[entry->unk0].globalTransform, lightScratch)'; MVMVA+swc2 $25-$27 into a stack
// SVECTOR at 0x50(sp); a loop 'for (j = 0; j < entry->unk0x0E? count; j++)' writing three 'lhu'
// from the projected vector into each of 'entry->pUnk1C[j]''s +4/+6/+8 (two offsets must come
// from the Temp9 view, which is still opaque at 0x0-0x13 and 0x14-0x20); a second loop over
// 'entry->unk0E' writing the projected xyz into 'entry'+0xC+16*j; then 'func_801E22F8(entry,
// &svec60, pLightMatrix, pOt, frameStep, packetIndex, pModel->ground)'.
// PARK_NOTE 8 — phase 4 (0x16B8): 'for (i = 0; i < pModel->numTextures; i++)
// func_801E1258(&pModel->pTextures[i], frameStep);' (stride 0x30).
// PARK_NOTE 9 — phase 5 (0x16EC-0x1BC0), colour-entry loop over 'UnkInner1
// pModel->pColorEntries' (stride 0x70), skipped when 'entry->unk0 < 0': 'entry->unk54 =
// (entry->unk54 - 1) & 7;' then 'if (entry->unk56 == 0)' the one-quad path ('RTPS' on
// 'entry+0xC', 'swc2 $14' to 'entry + 0x1C + 4*entry->unk54' then 'entry+0x14' → '+0x3C +
// 4*unk54', 'LibGearAllocVertex(g_LibGearVertexBufferPtr, entry->unk... / * lbu at -0x5(s1) *
// /)', 'vertex->unkE = 0', lifetime/colour stores from entry 0x5A..0x66, then the 4 corners from
// 'entry[0x1C + 4*((unk54+unk56)&7)]' etc.) else the two-quad path with 'MVMVA'+'swc2 $25-$27'
// into two vertices selected by 'unk54 & 1' and 'LibGearAllocVertex'. The 'lbu -0x5($s1)' / 'lw
// -0x4($s1)' reads are 'entry->unk?' fields just before +0x70 — the loop pointer is already
// advanced, so those are 'pColorEntries[i].…' at 0x6B/0x6C of the previous-entry layout; that is
// where the 0x70 stride must be nailed down.
// PARK_NOTE 10 — globals used: 'g_GfxOTDepthShift' (s32, OT index) and 'g_LibGearSwingScale'
// (s16, swing amplitude); scratchpad bases 0x1F800000/0x20/0x40/0x0E. 'Model', 'Temp8', 'Temp9',
// 'UnkInner1', 'Vertex', 'VertexBuffer' all already exist in 'include/libgear/main.h' and look
// sufficient — no header change expected.
//
// CORRECTIONS after writing the body (the full body is parked above, 87.92%):
// - PARK_NOTE 3 is wrong about the fourth base: $s7 is loaded with a bare
//   'lui $s7,0x1F80' and no 'ori', so it is 0x1F800000 (PSX_SCRATCH), not 0x1F80000E.
//   splat prints (0x1F80000E>>16) only because some symbol access used that hi part.
//   The 'light colour' halfwords at 0x1F800000/2/8/E are plain fields of the scratch
//   MATRIX (m[0][0], m[0][1], m[1][1], m[2][1]), and the joint loop's
//   SetLightMatrix/SetRotMatrix/SetTransMatrix all take PSX_SCRATCH.
// - PARK_NOTE 9's offsets are all 8 too low: the phase-5 cursor is entry+8, so the
//   quad counter is entry+0x5C, the age 0x5E, the frame cap 0x60, and the seven u16
//   copied into the Vertex (0x1E..0x2A: lifetime, rgb, rgb deltas) are 0x62..0x6E.
//   The entry head (not covered by UnkInner1, so this file declares a local view
//   'ColorSlot'): 0x0 s16 joint index (-1 = free), 0x2 u8 3D flag, 0x3 u8 semi-trans,
//   0x4 VertexBuffer*, 0x8 Vertex*, 0xC/0x14 SVECTORs projected with the joint matrix,
//   0x1C and 0x3C two banks of 16 u16 = two 0x10-byte slots of four SXY quads each.
// - Temp9 (0x24 stride) is nailed down: +0x0 s16 (negative = free), +0x4 s16 vector
//   count for the SVECTOR array at +0x14, +0xA s16 record count for the stride-0x10
//   records at +0x18 (SVECTOR at +0, s16 joint index at +6, u16 x/y/z at +8/+A/+C).
// - Phase 4's callee takes frameStep in $a1 and func_801E22F8 takes
//   (entry, &swingVec, pLightMatrix, pOt, packetIndex, scale, ground) - the last three
//   are pushed at 0x10/0x14/0x18(sp).

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
                if (pJoint->pRotationObject == 0 || pJoint->pRotationObject->type != 0xFF) {
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
                if (pJoint->pTranslationObject == 0 || pJoint->pTranslationObject->type != 0xFF) {
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
            pObject = pJoint->pRotationObject;
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
                pJoint->pRotationObject = pObject;
            }
        } else {
            pObject = pJoint->pRotationObject;
            if (pObject != NULL && i != 0 && pObject->type != 0xFF) {
                LibGearFreeJointObject(pList, (s8*)pObject);
                pJoint->pRotationObject = 0;
            }
        }
    next1:

        pKeyframe++;

        if (pKeyframe[0] != 0xFFFF) {
            pObject = pJoint->pTranslationObject;
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
                pJoint->pTranslationObject = pObject;
            }
        } else {
            pObject = pJoint->pTranslationObject;
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
            pObject = pJoint->pRotationObject;
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
                pJoint->pRotationObject = pObject;
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
            pObject = pJoint->pTranslationObject;
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
                pJoint->pTranslationObject = pObject;
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
        if (pSkeleton->pRotationObject != 0 && pSkeleton->pRotationObject->type != 0xFF) {
            LibGearFreeJointObject(pList, (s8*)pSkeleton->pRotationObject);
            pSkeleton->pRotationObject = 0;
        }

        if (pSkeleton->pTranslationObject != 0 && pSkeleton->pTranslationObject->type != 0xFF) {
            LibGearFreeJointObject(pList, (s8*)pSkeleton->pTranslationObject);
            pSkeleton->pTranslationObject = 0;
        }

        if (pSkeleton->pScaleObject != 0 && pSkeleton->pScaleObject->type != 0xFF) {
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
        if (pSkeleton->pRotationObject != 0 && pSkeleton->pRotationObject->type == type) {
            LibGearFreeJointObject(pList, (s8*)pSkeleton->pRotationObject);
            pSkeleton->pRotationObject = 0;
        }

        if (pSkeleton->pTranslationObject != 0 && pSkeleton->pTranslationObject->type == type) {
            LibGearFreeJointObject(pList, (s8*)pSkeleton->pTranslationObject);
            pSkeleton->pTranslationObject = 0;
        }

        if (pSkeleton->pScaleObject != 0 && pSkeleton->pScaleObject->type == type) {
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
        pEntry->isSemiTrans = arg18;
        pEntry->age = -1;
        pEntry->jointIndex = arg2;
        pEntry->is3D = arg3;
        pEntry->unk4 = arg1;
        pEntry->sourceA.vx = arg12;
        pEntry->sourceA.vy = arg13;
        pEntry->sourceA.vz = arg14;
        pEntry->sourceB.vx = arg15;
        pEntry->sourceB.vy = arg16;
        pEntry->sourceB.vz = arg17;
        pEntry->slot = 0;
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

    if (result > LIBGEAR_INTERPOLATE_FRAME_MAX) {
        return -1;
    } else {
        return result;
    }
}

s16 LibGearInterpolateFall(s16 a, s16 b, s16 c) {
    return c - a / b;
}

s16 LibGearInterpolateFallClamped(s16 a, s16 b, s16 c) {
    s16 result = LIBGEAR_INTERPOLATE_FRAME_MAX - a / b;

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
    if (pEntry->pVectors != NULL) {
        HeapFree(pEntry->pVectors);
        HeapFree(pEntry->pPoints[0]);
        HeapFree(pEntry->pPoints);
        HeapFree(pEntry->unk20);

        if (pEntry->pSections != NULL) {
            HeapFree(pEntry->pSections);
        }

        pEntry->pVectors = NULL;
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
    pModel->rotOffset[0] = 0;
    pModel->rotOffset[1] = 0;
    pModel->rotOffset[2] = 0;
    pModel->rotStep[0] = 0;
    pModel->rotStep[1] = 0;
    pModel->rotStep[2] = 0;
    pModel->posOffset[0] = 0;
    pModel->posOffset[1] = 0;
    pModel->posOffset[2] = 0;
    pModel->posStep[0] = 0;
    pModel->posStep[1] = 0;
    pModel->posStep[2] = 0;
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
            if (arg3 < LIBGEAR_ANIM_ID_WAD_BASE) {
                pModel->animValue = ((u32*)pSource->pAnimTable)[arg3];
            } else {
                pModel->animValue = ((u32*)((Temp8*)pSource->unkC)->pUnk4)[arg3 - LIBGEAR_ANIM_WAD_INDEX_OFFSET];
            }

            pModel->repeatCount = 0;
            pModel->frameTimer = 0;
            pModel->pWaitScript = 0;
            pModel->pGroundScript = 0;
            pModel->pArrivalScript = 0;
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
        pModel->parentModelSlot = LIBGEAR_MODEL_SLOT_NONE;
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
        pObject = pJoint->pRotationObject;
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
        pJoint->pRotationObject = pObject;
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
        pObject = pJoint->pRotationObject;
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

        pJoint->pRotationObject = pObject;
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
                pModel->pSkeleton[pCmd->unk4].doRender = pCmd->unk5 & 1;
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
// objdiff 99.87%, 464 bytes = retail's size (permuter score 15; it was 215, and
// objdiff 99.35%, while the global was still declared s16). The ONE fix the permuter
// found is the global's type: g_LibGearCurrentModelIndex is a u16 in retail, which
// loads it with `lhu %lo(...)(at)` here - func_801E5D44's note above (`lh`) is the
// only contrary evidence and is a different, still-parked body.
// WHAT IS LEFT, one reg_eqv tie: the compare chain (0xFD/0xFC/0xFA) reads the
// sourceModelSlot value from a1 in our build and from v1 in retail, and the range
// subtract `addiu v0,?, -1` takes the other register of the pair. Same instruction
// multiset, same 464 bytes, the `move a1,v1` copy sits in the 0xFE bne delay slot on
// both sides - only which of the two equivalent pseudos each use reads differs.
// Tried on top of the u16 type (all re-parked, none better): a separate
// `s16 slot = pModel2->sourceModelSlot` feeding the range test (98.02%, 468 bytes),
// dropping the inRange temporary for an inline `if ((u32)(pModel->sourceModelSlot - 1) < 0x7F)`
// (97.97%, 468 bytes), and a fresh 4-minute permuter run (1810 iters, -j 2) which
// returned NONE (base=best=15). The earlier shapes listed above were all measured
// with the s16 declaration, so they are worth one more pass only if the allocator
// question is ever reopened.
// Round 217: permuter-out's only edit (the u16 global) was already in this body;
// re-measured live at 99.87%/464 bytes, and another 4-minute run (-j 2, 1616
// iterations) returned NONE, base=best=15, as the two earlier runs did. One new
// hand shape: swapping the 0xFD and 0xFC arms (mutually exclusive, so semantics
// unchanged) scores 99.84% - worse - so the remaining reg_eqv tie stands.
// Round 245: permuter-out handed this park back again (its diff is the u16 global,
// already here); re-measured live at 99.87%/464 bytes, and two more 4-minute runs
// (-j 2, 1711 and 688 iterations) both returned NONE, base=best=15 - five such runs
// now. One new hand shape: a long-lived `s16 slot = pModel->sourceModelSlot` set
// just before the 0xFD arm and read only by the range test is simply cse'd into
// the same pseudo, leaving the identical three-line residual. Measured dry; do not
// re-open unless the allocator question is.
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
        index = pModel->linkedModelSlot;
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

    pObject = pModel->pSkeleton->pRotationObject;
    if (pObject != NULL) {
        if ((u32)(pObject->opcode - 7) < 2) {
            pObject->targetX = value[0];
            pObject->targetY = value[2];
            pObject->targetZ = value[4];
        }
    }
}*/
INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E63A8);


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
    pJoint->doRender = 0;
    pTarget->doRender = 1;

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

void LibGearTransferJointRenderFlags(ModelJoint* pSkeleton, ModelJoint* pTargetSkeleton) {
    int i;
    u_short numChildren = pSkeleton->numChildren;

    for (i = 1; i < numChildren; i++) {
        pSkeleton++;
        pTargetSkeleton++;

        if (pSkeleton->doRender != 0) {
            pSkeleton->doRender = 0;
            pTargetSkeleton->doRender = 1;
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
        index = pModel->linkedModelSlot;
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
        pObject = pJoint->pRotationObject;
    } else if (channel == 1) {
        pObject = pJoint->pTranslationObject;
    } else {
        pObject = pJoint->pScaleObject;
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
        pJoint->pRotationObject = pObject;
    } else if (channel == 1) {
        if (arg4 < 2) {
            pJoint->translation.x = pObject->startX;
            pJoint->translation.y = pObject->startY;
            pJoint->translation.z = pObject->startZ;
        }
        pJoint->pTranslationObject = pObject;
    } else {
        if (arg4 < 2) {
            pJoint->scale.vx = pObject->startX;
            pJoint->scale.vy = pObject->startY;
            pJoint->scale.vz = pObject->startZ;
        }
        pJoint->pScaleObject = pObject;
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

    pJoint->doRender = flags & 0x1;

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

    if ((flags & LIBGEAR_JOINT_CHANNEL_MASK) == LIBGEAR_JOINT_CHANNEL_ROTATION) {
        if (flags & LIBGEAR_JOINT_FLAG_ADD) {
            pJoint->rotation.vx += x;
            pJoint->rotation.vy += y;
            pJoint->rotation.vz += z;
        } else {
            pJoint->rotation.vx = x;
            pJoint->rotation.vy = y;
            pJoint->rotation.vz = z;
        }
    } else if ((flags & LIBGEAR_JOINT_CHANNEL_MASK) == LIBGEAR_JOINT_CHANNEL_TRANSLATION) {
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

// Parked at 83.19% (objdiff: 2332 bytes against retail 2280). The frame is 200
// against retail 208; pArchive takes s1 where retail has s4; the four offset
// parameters are kept in registers instead of being reloaded from their
// 0x68/0x70/0x78/0x80 slots; the prim/texture/material loops were not diffed.
INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E742C);
/*
// A gear model's resource record: a 0x14 byte header that describes the model,
// followed by numModelListEntries 0x24 byte material records.
typedef struct {
    u16 unk0; // 0x00
    u16 height; // 0x02
    u16 width; // 0x04
    u16 depth; // 0x06
    u16 scale; // 0x08
    u8 command; // 0x0A
    u8 unkB; // 0x0B
    u16 flags; // 0x0C
    u8 numColorEntries; // 0x0E
    u8 unkF; // 0x0F
    u8 numTextures; // 0x10
    u8 unk11; // 0x11
    u8 numModelListEntries; // 0x12
    u8 unk13; // 0x13
} ModelResource; // Size: 0x14

// The animation tables and the sound bank record a gear model is built against
typedef struct {
    u32 unk0; // 0x0
    u32* pAnimTables; // pAnimTables[1] is the entry table, pAnimTables[2] the first wad // 0x4
    Temp8* pSoundBank; // 0x8
} ModelSource; // Size: 0xC

// The archive entry a gear model is built from
typedef struct {
    u32 unk0; // 0x0
    void* pMeshSource; // 0x4
    ModelFileHeader* pModelFile; // 0x8
    ModelJointEntry* pJointEntries; // 0xC
    u32* pWadTable; // pWadTable[i + 2] is the vertex wad of material i // 0x10
} ModelArchive; // Size: 0x14

void LibGearAllocateColorEntries(Model* pModel);
void func_8002DDE4(void* pMeshSource, s16 a1, s16 a2, s16 a3, s16 a4, s16 a5, s16 a6);
void func_80030988(s16 a0, s16 a1, s32 a2, s32 a3);
void func_801E1A14(Temp9* pEntry, void* pWad, s16 a2, s16 a3, s16 a4, s16 a5, s16 a6, s16 a7, s16 a8, s16 a9, s16 a10, s16 a11, s16 a12, s16 a13, u8 a14, u8 a15, u8 a16, u8 a17, u8 a18, u8 a19);
void ResolveArchiveEntryPointers(void* pTable);
s32 SoundFindSedsEntry(void* pEntry, s16 bank);
void SoundAddSedsEntry(void* pEntry);

// The heap copy of the model file being created, and the mesh slot it takes
extern ModelFileHeader* g_LibGearModelFile;
extern s32 g_LibGearMeshSlot;

// Builds a gear model from its archive entry into g_LibGearModels[modelIndex].
// offsetX..offsetW shift the material records' texture coordinates, and
// pPosition, when given, places the root joint.
void func_801E742C(s32 modelIndex, u16 flags, ModelSource* pSource, ModelArchive* pArchive, u16 offsetX, u16 offsetY, u16 offsetZ, u16 offsetW, SVECTOR* pPosition) {
    Model* pModel;
    ModelResource* pResource;
    ModelFileHeader* pModelFile;
    Temp8* pSoundBank;
    u32* pAnimTables;
    void* pMeshSource;
    u8* pDataStart;
    ModelJointEntry* pJointEntries;
    u32* pWadTable;
    u16* pCur;
    s32 size;
    s32 i;
    s32 j;
    s32 count;
    s32 isFlat;
    u16 resourceFlags;

    HeapChangeCurrentUser(HEAP_USER_MASA, HEAP_CONTENT_NULL);

    if (modelIndex >= MAX_MODELS || g_LibGearModels[modelIndex] != NULL) {
        return;
    }

    pModel = HeapAlloc(sizeof(Model), 0x0);

    if (!(flags & 1)) {
        ResolveArchiveEntryPointers(pArchive);
        ResolveArchiveEntryPointers(pArchive->pWadTable);
    }

    pModel->hasSedsEntry = 0;
    pModel->pad63[0] = 0;

    if (!(flags & 4)) {
        ResolveArchiveEntryPointers(pSource);
        ResolveArchiveEntryPointers(pSource->pSoundBank);
        pAnimTables = pSource->pAnimTables;
        ResolveArchiveEntryPointers(pAnimTables);
        ResolveArchiveEntryPointers((void*)pAnimTables[1]);
        pSoundBank = pSource->pSoundBank;
        if (pSoundBank->pUnkC != pSoundBank->pUnk8) {
            if (SoundFindSedsEntry(pSoundBank->pUnk8, 0) == 0) {
                SoundAddSedsEntry(pSoundBank->pUnk8);
                pModel->hasSedsEntry = 1;
            }
        }
    }

    pWadTable = pArchive->pWadTable;
    pResource = (ModelResource*)pWadTable[1];
    pMeshSource = pArchive->pMeshSource;
    pDataStart = (u8*)pArchive->pModelFile;
    pJointEntries = pArchive->pJointEntries;
    size = (u8*)pJointEntries - pDataStart;
    g_LibGearModels[modelIndex] = pModel;

    pModel->height = pResource->height;
    pModel->width = pResource->width;
    pModel->depth = pResource->depth;
    pModel->commands[0] = pResource->command;
    resourceFlags = pResource->flags;
    pModel->flags = resourceFlags;
    if (resourceFlags & 0x200) {
        func_80030988(2, 2, 0x40, 0x40);
    }

    if (!(flags & 1)) {
        isFlat = 0;
        if (!(flags & 0x40)) {
            isFlat = (pModel->flags & 0x4) == 0;
        }
        func_8002DDE4(pMeshSource, (s16)isFlat, (s16)offsetX, (s16)offsetY, (s16)isFlat, (s16)offsetZ, (s16)offsetW);

        pModelFile = HeapAlloc(size, 1);
        g_LibGearModelFile = pModelFile;
        memcpy(pModelFile, pDataStart, size);

        g_LibGearMeshSlot = 0;
        while (g_LibGearMeshSlot < MAX_MODEL_MESHES) {
            if (g_LibGearMeshes[g_LibGearMeshSlot].pParts == NULL) {
                break;
            }
            g_LibGearMeshSlot++;
        }

        LibGearInitializeMesh(g_LibGearModelFile, &g_LibGearMeshes[g_LibGearMeshSlot]);
    }

    pModel->pMesh = &g_LibGearMeshes[g_LibGearMeshSlot];

    if (flags & 0x40) {
        pModel->pSkeleton = LibGearInitializeSkeleton(pModel->pMesh, pJointEntries, 0, 0, 0, 0, 0, 0);
    } else if (pModel->flags & 0x4) {
        pModel->pSkeleton = LibGearInitializeSkeleton(pModel->pMesh, pJointEntries, 2, 0, 0, 0, 0, 0);
    } else {
        pModel->pSkeleton = LibGearInitializeSkeleton(pModel->pMesh, pJointEntries, 2, 1, (s16)offsetX, (s16)offsetY, (s16)offsetZ, (s16)offsetW);
    }

    if (pPosition != NULL) {
        pModel->pSkeleton->translation.x = pPosition->vx;
        pModel->pSkeleton->translation.y = pPosition->vy;
        pModel->pSkeleton->translation.z = pPosition->vz;
    }

    if (flags & 1) {
        pModel->unk90 = -1;
    } else {
        pModel->unk90 = offsetX;
        pModel->unk92 = offsetY;
        pModel->blendOffsetX = offsetZ;
        pModel->blendOffsetY = offsetW;
    }

    if (!(flags & 4) && !(flags & 0x80)) {
        pModel->unkAC = pSource;
    } else {
        pModel->unkAC = NULL;
    }

    for (i = 0; i < 2; i++) {
        SetPolyFT4(&pModel->polys[i]);
        SetSemiTrans(&pModel->polys[i], 1);

        pModel->polys[i].r0 = 0x40;
        pModel->polys[i].g0 = 0x40;
        pModel->polys[i].b0 = 0x40;
        pModel->polys[i].clut = GetClut(0x100, 0xF3);
        pModel->polys[i].tpage = GetTPage(0, 2, 0x280, 0x100);

        setUV4(
            &pModel->polys[i],
            0x0, 0xE0,
            0xF, 0xE0,
            0x0, 0xEF,
            0xF, 0xEF
        );
    }

    pModel->scale = pResource->scale;
    pModel->numColorEntries = pResource->numColorEntries;
    LibGearAllocateColorEntries(pModel);
    pModel->numTextures = pResource->numTextures;

    if (pModel->numTextures != 0) {
        pModel->pTextures = HeapAlloc(pModel->numTextures * sizeof(Temp8), 0x0);
        for (i = 0; i < pModel->numTextures; i++) {
            pModel->pTextures[i].isLoaded = 0;
            pModel->pTextures[i].pUnk4 = NULL;
            pModel->pTextures[i].pUnk8 = NULL;
            pModel->pTextures[i].pUnkC = NULL;
        }
    }

    pModel->numModelListEntries = pResource->numModelListEntries;
    pCur = (u16*)(pResource + 1);

    if (pModel->numModelListEntries != 0) {
        pModel->pModelListEntries = HeapAlloc(pModel->numModelListEntries * sizeof(Temp9), 0x0);

        for (i = 0; i < pModel->numModelListEntries; i++) {
            count = (s16)pCur[0x11];
            *(u16*)pModel->pModelListEntries[i].unk0 = *pCur++;

            func_801E1A14(
                &pModel->pModelListEntries[i],
                (void*)pWadTable[i + 2],
                *(s16*)pCur++,
                *(s16*)pCur++,
                *(s16*)pCur++,
                *(s16*)pCur++,
                *(s16*)pCur++,
                count,
                (s16)(offsetX + *pCur++),
                (s16)(offsetY + *pCur++),
                *(s16*)pCur++,
                *(s16*)pCur++,
                (s16)(offsetZ + *pCur++),
                (s16)offsetY,
                *(u8*)pCur++,
                *(u8*)pCur++,
                *(u8*)pCur++,
                *(u8*)pCur++,
                *(u8*)pCur++,
                *(u8*)pCur++
            );
            pCur++;

            for (j = 0; j < count; j++) {
                *(u16*)((u8*)pModel->pModelListEntries[i].unk18 + j * 0x10 + 0x6) = *pCur++;
                *(u16*)((u8*)pModel->pModelListEntries[i].unk18 + j * 0x10 + 0xE) = *pCur++;
                *(u16*)((u8*)pModel->pModelListEntries[i].unk18 + j * 0x10 + 0x0) = *pCur++;
                *(u16*)((u8*)pModel->pModelListEntries[i].unk18 + j * 0x10 + 0x2) = *pCur++;
                *(u16*)((u8*)pModel->pModelListEntries[i].unk18 + j * 0x10 + 0x4) = *pCur++;
            }
        }
    }

    pModel->unk22[0] = 0;
    pModel->doUpdate = 1;
    pModel->modelSlot = modelIndex;

    if (!(flags & 0x40)) {
        pModel->pSoundBank1 = pSource->pSoundBank;
        func_801E3534(pModel, (s32)&g_LibGearJointObjectPool, (u32)&pSource->pAnimTables[2], pSource->pAnimTables[1]);
        LibGearSetAnimation(pModel, pModel, (s32)&g_LibGearJointObjectPool, 0);
    }

    if (flags & 2) {
        pModel->pModelFile = NULL;
    } else {
        ModelFileTrimHeapBlock(g_LibGearModelFile);
        ModelUnresolvePointers(g_LibGearModelFile);
        size = (s32)HeapGetNextBlockHeader((HeapBlock*)g_LibGearModelFile);
        pModelFile = HeapAlloc(size, 0x0);
        memcpy(pModelFile, g_LibGearModelFile, size);
        HeapFree(g_LibGearModelFile);
        LibGearFreeModelMesh(pModel->pMesh, 0);
        LibGearInitializeMesh(pModelFile, pModel->pMesh);
        pModel->pModelFile = pModelFile;
    }
}
*/

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
        if (g_LibGearModels[i] != NULL && g_LibGearModels[i]->parentModelSlot < LIBGEAR_MODEL_SLOT_NONE) {
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
            pObjects[i].jointIndex = -1;
            pObjects[i].unk8 = 0;
        }
        pModel->pColorEntries = pObjects;
    }
}
