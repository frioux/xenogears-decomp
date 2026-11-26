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

// Psy-Q angles wrap at LIBGEAR_ANGLE_FULL_TURN, so this offset into rsin/rcos is a quarter turn
#define LIBGEAR_QUARTER_TURN 0x400
// A turn and its half: a difference of two angles is masked to a turn and folded into -half..+half
#define LIBGEAR_ANGLE_MASK 0xFFF
#define LIBGEAR_ANGLE_HALF_TURN 0x800
#define LIBGEAR_ANGLE_FULL_TURN 0x1000

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

// Which of the joint's vectors LibGearSetJointTransform drives; anything else is the scale
#define LIBGEAR_JOINT_CHANNEL_MASK 0x7
#define LIBGEAR_JOINT_CHANNEL_ROTATION 0
#define LIBGEAR_JOINT_CHANNEL_TRANSLATION 1

// Temp3::opcode values LibGearSetJointRotation and LibGearMoveJointToPoint queue

// Temp3::opcode & this mask is the command the joint update dispatches on
#define LIBGEAR_JOINT_OPCODE_MASK 0xF
// Temp3::opcode below the interpolation walks a byte script through the value at 0x4
#define LIBGEAR_JOINT_OBJECT_SET 0
#define LIBGEAR_JOINT_OBJECT_RELATIVE 1
#define LIBGEAR_JOINT_OBJECT_ADD 2
// ...and from the interpolation up interpolates that value
#define LIBGEAR_JOINT_OBJECT_STEP 4
#define LIBGEAR_JOINT_OBJECT_ACCUMULATE 5

// Temp3::opcode bits that say the command leaves that channel alone
#define LIBGEAR_JOINT_OBJECT_NO_X 0x10
#define LIBGEAR_JOINT_OBJECT_NO_Y 0x20
#define LIBGEAR_JOINT_OBJECT_NO_Z 0x40

// A script byte that introduces a 16-bit value instead of a relative step
#define LIBGEAR_SCRIPT_LITERAL -0x80

// LibGearStepModelAnimation's result bits: low bits are the state of the objects
// whose type is the model's, the 0x100 block the same for any object
#define LIBGEAR_JOINT_UPDATE_ADVANCED 0x1
#define LIBGEAR_JOINT_UPDATE_FINISHED 0x2
#define LIBGEAR_JOINT_UPDATE_LOOPED 0x4
#define LIBGEAR_JOINT_UPDATE_ADVANCED_ANY 0x100
#define LIBGEAR_JOINT_UPDATE_FINISHED_ANY 0x200
#define LIBGEAR_JOINT_UPDATE_LOOPED_ANY 0x400

// Temp3::curFrame stops accumulating before it reaches this
#define LIBGEAR_JOINT_FRAME_ACCUMULATE_MAX 0x7D00

// Temp3::type value meaning the object holds no queued command

// Interpolation curves LibGearGetInterpolationFunc picks by type; 0 (the default) is the sine
#define LIBGEAR_INTERPOLATE_RISE 1
#define LIBGEAR_INTERPOLATE_FALL 2
#define LIBGEAR_INTERPOLATE_FALL_CLAMPED 3

// Top of the frame range LibGearInterpolateRise and LibGearInterpolateFallClamped work in
#define LIBGEAR_INTERPOLATE_FRAME_MAX 32

// Frames an interpolation needs; fewer are written into the joint instead of queued
#define LIBGEAR_INTERPOLATE_MIN_FRAMES 2

// Frames UnkInner1::numFrames holds at most
#define LIBGEAR_COLOR_ENTRY_FRAME_MAX 7

// UnkInner1::jointIndex meaning the entry projects through no joint

// Model::frame meaning the model's animation script is not running

// Vertex entries LibGearInitialize creates the shared vertex buffer with
#define LIBGEAR_VERTEX_BUFFER_CAPACITY 16

// Vertex::age meaning the vertex buffer slot holds no vertex

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
    /* 0x5E */ s16 age; // frames drawn; 0 after the wrap retires the entry
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

// PARK, round 284: objdiff 98.91% at retail's EXACT 3376 bytes. The OT tail below is
// SOLVED; what is left is two swapped saved-register pairs.
//  - the 4 missing instructions were TWO independent things. (1) both OT link pokes
//    mask BOTH sides of the OR: retail's `and v1,v1,t0` (0xFF000000) on the packet tag
//    is mirrored by `and v0,v0,t0` on the OT slot, so the slot poke is
//    `pOt[otz] = (pOt[otz] & 0xFF000000) | ((u32)pPoly & 0xFFFFFF);` and not a bare OR
//    (round 282's `| `-only form was 8 bytes short by itself, 97.13). (2) retail's
//    `otz` IS memory-homed in depthBuf[0]'s slot: `depthBuf[0] = otz;` as a separate
//    statement AFTER the two pokes, then the clamp on `depthBuf[0]` and the `0x6B -
//    depthBuf[0] / 8` through the array. That store sits before the `slti`, and the
//    pointer store inside the poke invalidates memory CSE, so gcc reloads it - which
//    is exactly retail's 3-insn `bgez/addiu 7/sra 3` division. Together: 3360 -> 3376.
//  - `ColorSlot.age` is **s16**, not u16: retail reads it with `lh` in the 2D path's
//    `(slot + age) & 7` and with `lhu` only for its own `age++`. s16 also lets the
//    `(s16)` casts on the age compares go away at no cost.
//  - still open: the saved-register map. s0=localT, s3=numChildren, s4=i,
//    s7=pScratch(0x1F800000), fp=pMesh are retail's, but retail has s1=pViewMatrix and
//    s2=pSkeleton where this body has s1=pSkeleton, s2=pViewMatrix, and retail has
//    s5=pModel, s6=pViewScratch(0x1F800020) where this body has s5=pViewScratch,
//    s6=pModel. Both pairs are ADJACENT in gcc's allocation order, i.e. their
//    n_refs/live_length priorities are within noise of each other and the two
//    parameter pseudos rank just below their local counterpart here. Every remaining
//    row (93 of 846) is one of those four register names, the delay-slot placement it
//    implies, and 2 `addu v0,v0,s2` / `addu v0,s2,v0` operand orders in the 2D colour
//    path. Measured dead in this round: `pScratch[1].t[0..2]` for the three
//    pViewScratch t[] writes (identical code), `numChildren = pRoot->numChildren` plus
//    a reordered pRoot assignment (98.09/3380), `&pRoot->localTransform` for the first
//    CompMatrix (pRoot and pSkeleton coalesce, no change).
// Superseded round-283 header, kept for its history:
// PARK, round 283: objdiff 98.79%, 3360 bytes. Round 282's 97.71 was an ENTRY-BLOCK
// REGISTER-MAP artefact, not a missing instruction: retail's pSkeleton value dies at
// the joint loop's head (its $s2 becomes the cursor) and every later use reads the
// SPILLED copy at 0x78(sp), so the source carries TWO locals for the same field -
// `pRoot = pModel->pSkeleton;` for the list/colour phases and `pSkeleton` for phase 1
// and the loop head - and the separate `pMeshLoaded` copy is not needed (one `pMesh`
// local). Measured on its own: the split alone 98.42/3360, the mesh-copy drop alone
// 97.59/3372, both 98.79/3360. The entry block's reload of pSkeleton disappears and
// the prologue's saved-register SET becomes retail's ten.
// WHAT IS LEFT, all measured from this body:
//  - TWO saved registers are swapped: s0=localT, s3=numChildren, s5=pModel,
//    s6=viewScratch(0x1F800020), s7=pScratch(0x1F800000) and fp=pMesh are retail's,
//    but pSkeleton takes s1 where retail has pViewMatrix there and pViewMatrix takes
//    s2 (retail's pSkeleton). gcc ranks pSkeleton's 4 refs above pViewMatrix's 2, so
//    it gets the lower number; the visible rows are the prologue's two swapped
//    save/restore pairs. Measured dead: `pJoint = pRoot + 1` (98.67/3364, takes the
//    loop head off pSkeleton), `numChildren = pModel->pSkeleton->numChildren`
//    (neutral), a live copy `pView = pViewMatrix` for the MulMatrix0 (98.02).
//  - (OT tail, since solved by round 284 - see the header.) Measured dead: `depthBuf[0]
//    >>= shift` write-back 97.46/3396, one-statement `otz = depthBuf[0] >> shift`
//    98.18, ternary clamp and `> 0x2D8` clamp both neutral.
//  - `addiu v0,i,0x84` at the joint loop head where retail keeps the coalesced cursor
//    (`addiu s2,s2,0x7C` then `addiu s1,s2,8`); now that pSkeleton dies there,
//    `pJoint = pSkeleton; pJoint++;` was re-measured and is worse (98.69).
//  - (the 0x5C/0x5E `lh`/`lhu` pair was `ColorSlot.age` being s16 - fixed in round 284.)
// PERMUTER: a fourth round from THIS body found nothing (base=1336, best=1336).
// Superseded round-282 header, kept for its history:
// PARK, round 282: objdiff 97.71%, 3376 bytes - retail's EXACT size, so every
// remaining row is a register name or a statement order, not a missing instruction.
// 31 diff blocks, and most of them are the same thing: the two builds assigned the
// nine saved registers differently (retail s1=pViewMatrix, s2=pSkeleton,
// s3=numChildren, s6=viewScratch, s8=pMesh; this body s1=pMeshLoaded,
// s2=pViewMatrix, s6=numChildren, s8=viewScratch). That map is the wall: objdiff
// charges a row for every one of those renames.
// CLOSED IN ROUND 282, each measured on its own from the round-281 body (96.88):
//  - both OT link pokes OR into the entry instead of masking it off first:
//    `pOt[otz] = pOt[otz] | ((u32)pPoly & 0xFFFFFF);` 96.88 -> 97.13, -8 bytes.
//  - the 3D colour path takes the joint index through a local first
//    (`otz = pEntry->jointIndex; ... &pSkeleton[otz].globalTransform`): 97.08 AT
//    RETAIL'S 3376 BYTES, and it stays exact from here on.
//  - the OT shift is two statements (`otz = depthBuf[0]; otz >>= shift;`), not one
//    expression: 97.18.
//  - the joint loop's mesh pointer comes from a SECOND copy of `pModel->pMesh` that
//    is loaded in the entry block (`pMeshLoaded`), assigned to `pMesh` just before
//    the loop: 97.44. Removing the assignment and using pMeshLoaded directly is
//    3372 bytes / 97.59 - the copy is load-bearing.
//  - the model-list latch is `pListEntry++, i++` (pointer first): 97.64. This is the
//    block whose `nop` delay slot was the last extra instruction.
//  - the 2D quad index goes through its own s32 temp (`quadIndex`, then
//    `quadSlot = quadIndex;`): 97.71.
//  - `LIBGEAR_QUARTER_TURN` names the `+ 0x400` rsin/rcos offset, held in a local
//    assigned in the entry block (a literal folds back per block); measured neutral.
// THE PERMUTER IS DONE WITH THIS BODY: three rounds from 96.88 (score 3215 -> 2378)
// found the `|=` poke, the jointIndex local, the quadSlot split and the 0x400 hoist,
// and the last round produced only a bogus `numChildren = (pModel->doUpdate == 0)`.
// CLOSED IN ROUND 281, kept for the record:
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
// STILL OPEN (all ALLOCATOR calls: the saved set and the 0xE0 frame match retail
// exactly, so none of them is over-allocation):
//  - the saved-register map above.
//  - pSkeleton has no callee-saved home in the entry block: this body reloads
//    `lw t2,0x78(sp)` right after storing it, where retail keeps it in $s2 up to
//    0x1478 (its pJoint cursor then takes s2 over) and only reloads in phase 3/5.
//    Measured dead: moving numChildren out of the entry block (96.12), swapping
//    scale/numChildren (96.46), using the local at the two phase-1 sites where retail
//    loads `0x4(s5)` fresh (96.68), every declaration order of the pointer locals.
//  - the OT tail keeps `otz` in a register where retail spills it to 0x48(sp) (the
//    depthBuf[0] slot) and reloads it for the /8, which also gives retail's 3-insn
//    `bgez / addiu 7 / sra 3` division against 4 here. `depthBuf[0] >>= shift` and
//    every other write-back spelling is much worse (3416 bytes, 95.99), so the
//    register form below is the best measured.
//  - `(pEntry->slot + pEntry->age) & 7` reads both fields with `lh` in retail and
//    `lhu` here (2 rows; behaviour is identical after the mask). `s16 age` on the
//    ColorSlot field changes nothing.
//  - the 2D colour path's two `screenA[2*slot + 1]` / `screenB[2*slot + 1]` reads:
//    retail `addu v0,v0,s2`, this body `addu v0,s2,v0` (2 rows).
// Un-park by deleting the INCLUDE_ASM line and the enclosing comment.


INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801DCEC8);

/*

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
    ModelJoint* pRoot;
    s32 quarterTurn;
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
    pMesh = pModel->pMesh;
    quarterTurn = LIBGEAR_QUARTER_TURN;
    modelScale = pModel->scale;
    numChildren = pSkeleton->numChildren;
    pRoot = pModel->pSkeleton;

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
        pOt[otz] = (pOt[otz] & 0xFF000000) | ((u32)pPoly & 0xFFFFFF);
        depthBuf[0] = otz;
        if (depthBuf[0] >= 0x2D9) {
            depthBuf[0] = 0x2D8;
        }
        pModel->depthShade = 0x6B - depthBuf[0] / 8;
    }

    MulMatrix0(pViewMatrix, &pSkeleton->globalTransform, pViewScratch);

    pMesh = pMesh;
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

        swingVec.vx = -g_LibGearSwingScale * rsin(pRoot->rotation.vy + quarterTurn) / 0x1000;
        swingVec.vz = g_LibGearSwingScale * rcos(pRoot->rotation.vy + quarterTurn) / 0x1000;
        swingVec.vy = pModel->unk3E;

        CompMatrix(&pRoot->localTransform, &pRoot[pListEntry->jointIndex].globalTransform, pScratch);
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
            CompMatrix(&pRoot->localTransform, &pRoot[pSection->vector.pad].globalTransform, pScratch);
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
            CompMatrix(pLightScratch, &pRoot[pEntry->jointIndex].globalTransform, pScratch);
            SetRotMatrix(pScratch);
            SetTransMatrix(pScratch);

            gte_ldv0(&pEntry->sourceA);
            gte_rtps();
            gte_stsxy2((u_long*)&pEntry->screenA[2 * pEntry->slot]);

            gte_ldv0(&pEntry->sourceB);
            gte_rtps();
            gte_stsxy2((u_long*)&pEntry->screenB[2 * pEntry->slot]);

            pEntry->age++;
            if (pEntry->age == 0) {
                continue;
            }

            if (pEntry->numFrames < pEntry->age || pEntry->pVertex == NULL) {
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
            CompMatrix(&pRoot->localTransform, &pRoot[otz].globalTransform, pScratch);
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
            if (pEntry->age == 0) {
                continue;
            }

            if (pEntry->numFrames < pEntry->age || pEntry->pVertex == NULL) {
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

*/



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
// otz = 0x2D8;', and 'pModel->depthShade = 0x6B - (otz / 8)' (0x2D8, 0x6B and the '+7' before 'sra 3'
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

// A joint's queued object (Temp3) as this update reads it. Bytes 0x4..0xE are a
// byte script while opcode & 0xF is below LIBGEAR_JOINT_OBJECT_STEP and an
// interpolation from LIBGEAR_JOINT_OPCODE_INTERPOLATE up, so they share one
// union: the script holds its start and its cursor, the interpolation its start
// and target values.
typedef struct {
    /* 0x0 */ u8* pScript;
    /* 0x4 */ u8* pCursor;
} JointObjectStep;
typedef struct {
    /* 0x0 */ s16 startX;
    /* 0x2 */ s16 startY;
    /* 0x4 */ s16 startZ;
    /* 0x6 */ s16 targetX;
    /* 0x8 */ s16 targetY;
    /* 0xA */ s16 targetZ;
} JointObjectLerp;
typedef union {
    JointObjectStep step;
    JointObjectLerp lerp;
} JointObjectValue;
typedef struct {
    /* 0x0  */ u8 inUse;
    /* 0x1  */ u8 loops;
    /* 0x2  */ u8 opcode;
    /* 0x3  */ u8 type;
    /* 0x4  */ JointObjectValue value;
    /* 0x10 */ s16 curFrame;
    /* 0x12 */ s16 endFrame;
} JointObject; // Size: 0x14

// REDO lane round 435. Best body 82.84% / 5092 bytes / 1273 insns against
// retail's 4888 / 1222 - 51 instructions over, all of it register identity plus
// arm shape; the prologue, the loop tail and the ApplyMatrix site already carry
// retail's instruction sequence apart from register names. This is the same
// function as battle/main/misc21's func_800A0838 (twin_for.py ratio 1.000; that
// unit parks the identical body at 82.84% / 5092 too), so anything learned here
// ports there and back.
//
// What this round established, each measured with diff_func.sh:
// - `u32 i;` declared BEFORE `u32 numChildren;` and `u32 numChildren` (retail
//   stores it with `sw` and tests the entry with a bare `beqz count`): i lands at
//   0x48 and count at 0x50, retail's slots. u16 gives `sh` + an sltu/beqz pair.
// - the single-walker loop `for (i = 0; i < numChildren; i++, pJoint++)` with NO
//   second pointer is what makes gcc produce retail's register copy biased by +4
//   (retail's `addiu s1,a1,4`, hence the 0x6C/0x48/0x58(s1) displacements) and
//   the +0x7C step on the spilled home as well.
// - the script cursor is written per component (`pObject->value.step.pCursor +=
//   2;` after each store) AND kept in a `pCursor` local advanced in the same
//   component; see the R500 note below, which closed this class (a pCursor
//   stored once at the end of the arm is 4888 bytes but 3 instructions short per
//   arm, so only the double advance is retail's).
// - the RELATIVE arm's literal branch reads the high byte with `lb` (signed) and
//   the low byte with `lbu` (retail `lbu $a0,0x1($v1)` / `lb $v0,0x2($v1)`), so
//   the source's high byte is a signed s8; the cursor advances before each read.
//   Six sites in the two RELATIVE arms; the rotation channel's three still come
//   out `lbu` + `sll 24`/`sra 16` (see residual 2 below).
// - the frame is 0xA0 against retail's 0xB8: this body spills a0@0x28, a3@0x30
//   and a1@0x38 and keeps a2 in $s8, where retail spills a0..a3 at
//   0x28/0x30/0x38/0x40 and reloads a2 from 0x38(sp) nine times. Declaration
//   order of the locals, u16 vs u32 count and the two walker forms were all
//   measured and none of them moves the spill order.


// R500 REDO lane: best 95.66% / 4876 bytes / 1218 insns against retail's 4888 /
// 1222 (4 instructions and 12 bytes SHORT). Every remaining difference is
// allocation plus the three items at the end; the arm shapes are retail's.
// Closed this round, each measured with diff_func.sh:
// - the four SET/ADD arms each open with `pCursor = pObject->value.step.pCursor;`
//   and then do BOTH `pCursor += 2;` and `pObject->value.step.pCursor += 2;` per
//   component. That double advance is retail's `lhu 0(a0)` / `addiu a0,a0,2`
//   next to `lw v0,8(s0)` / `addiu v0,v0,2` / `sw` (82.84 -> 86.02).
// - the six RELATIVE literal paths advance the field BEFORE each byte read
//   (`field++; low = pCursor[0]; field++; delta = *(s8*)(pCursor+1);`) and
//   combine through a u8 `low` local (86.02 -> 90.82).
// - the STEP arms want 16-bit dx/dy/dz (retail's all-zero test is `sll 16` /
//   `bnez`, i.e. it compares the low half only) while MOVE_BASE wants 32-bit
//   ones (`subu` then `mult dx,dx` with no truncation): split into s16 dx/dy/dz
//   plus s32 moveX/moveY/moveZ (90.82 -> 93.78).
// - MOVE_BASE computes ratan2 BEFORE the maxStep division (93.78 -> 95.01), and
//   `angle`/`maxStep` are s32, not s16 (95.01 -> 95.45).
// - the TRANSLATION channel block sets only pJoint->doTransform; rotation and
//   scale set doRotate as well (retail has one `sb v0,0x0(s1)` at 0x2ACC but two
//   at 0x23E4 and 0x2EA8) (95.45 -> 95.66).
// Residual, all measured:
// 1) retail spills objectType (a2) to 0x38(sp) and reloads it as t3/t4 in every
//    channel tail (nine sites); this body keeps it in $s8 instead. Retail needs
//    the register because it also copies pObject to $s6 and runs maxStep in $s0
//    in the MOVE_BASE arm. Re-ordering the local declarations does not move it.
// 2) three rotation RELATIVE literal paths still read the high byte as `lbu` +
//    `sll 24`/`sra 16` where retail has `lb` + `sll 8`; the translation
//    channel's three (32-bit LVEC3 target) do match. The s16 rotation target is
//    what lets gcc drop the sign-extending load.
// 3) two extra `nop` + `negu` pairs at the `(angle < 0 ? -angle : angle) >=
//    maxStep` clamp.
/*
s32 func_801DDBF8(Temp4* pList, ModelJoint* pSkeleton, u32 objectType, s32 scale) {
    s32 result;
    JointObject* pObject;
    ModelJoint* pJoint;
    u32 i;
    u32 numChildren;
    s16 frame;
    u8* pCursor;
    u8 opcode;
    s8 delta;
    u8 low;
    SVECTOR vec;
    VECTOR out;
    s16 dx;
    s16 dy;
    s16 dz;
    s32 dist;
    s32 angle;
    s32 maxStep;
    s32 moveX;
    s32 moveY;
    s32 moveZ;

    result = 0;
    numChildren = pSkeleton->numChildren;
    pJoint = pSkeleton;

    for (i = 0; i < numChildren; i++, pJoint++) {

        // Rotation channel
        pObject = (JointObject*)pJoint->pRotationObject;
        if (pObject != NULL) {
            opcode = pObject->opcode;

            switch (opcode & LIBGEAR_JOINT_OPCODE_MASK) {
            case LIBGEAR_JOINT_OBJECT_SET:
                pCursor = pObject->value.step.pCursor;
                if (!(opcode & LIBGEAR_JOINT_OBJECT_NO_X)) {
                    pJoint->rotation.vx = *(u16*)pCursor;
                    pCursor += 2;
                    pObject->value.step.pCursor += 2;
                }
                if (!(opcode & LIBGEAR_JOINT_OBJECT_NO_Y)) {
                    pJoint->rotation.vy = *(u16*)pCursor;
                    pCursor += 2;
                    pObject->value.step.pCursor += 2;
                }
                if (!(opcode & LIBGEAR_JOINT_OBJECT_NO_Z)) {
                    pJoint->rotation.vz = *(u16*)pCursor;
                    pCursor += 2;
                    pObject->value.step.pCursor += 2;
                }
                break;

            case LIBGEAR_JOINT_OBJECT_RELATIVE:
                if (!(opcode & LIBGEAR_JOINT_OBJECT_NO_X)) {
                    delta = *(s8*)pObject->value.step.pCursor++;
                    if (delta != LIBGEAR_SCRIPT_LITERAL) {
                        pJoint->rotation.vx += delta;
                    } else {
                        pCursor = pObject->value.step.pCursor;
                        pObject->value.step.pCursor++;
                        low = pCursor[0];
                        pObject->value.step.pCursor++;
                        delta = *(s8*)(pCursor + 1);
                        pJoint->rotation.vx = low | (delta << 8);
                    }
                }
                if (!(opcode & LIBGEAR_JOINT_OBJECT_NO_Y)) {
                    delta = *(s8*)pObject->value.step.pCursor++;
                    if (delta != LIBGEAR_SCRIPT_LITERAL) {
                        pJoint->rotation.vy += delta;
                    } else {
                        pCursor = pObject->value.step.pCursor;
                        pObject->value.step.pCursor++;
                        low = pCursor[0];
                        pObject->value.step.pCursor++;
                        delta = *(s8*)(pCursor + 1);
                        pJoint->rotation.vy = low | (delta << 8);
                    }
                }
                if (!(opcode & LIBGEAR_JOINT_OBJECT_NO_Z)) {
                    delta = *(s8*)pObject->value.step.pCursor++;
                    if (delta != LIBGEAR_SCRIPT_LITERAL) {
                        pJoint->rotation.vz += delta;
                    } else {
                        pCursor = pObject->value.step.pCursor;
                        pObject->value.step.pCursor++;
                        low = pCursor[0];
                        pObject->value.step.pCursor++;
                        delta = *(s8*)(pCursor + 1);
                        pJoint->rotation.vz = low | (delta << 8);
                    }
                }
                break;

            case LIBGEAR_JOINT_OBJECT_ADD:
                pCursor = pObject->value.step.pCursor;
                if (!(opcode & LIBGEAR_JOINT_OBJECT_NO_X)) {
                    pJoint->rotation.vx += *(u16*)pCursor;
                    pCursor += 2;
                    pObject->value.step.pCursor += 2;
                }
                if (!(opcode & LIBGEAR_JOINT_OBJECT_NO_Y)) {
                    pJoint->rotation.vy += *(u16*)pCursor;
                    pCursor += 2;
                    pObject->value.step.pCursor += 2;
                }
                if (!(opcode & LIBGEAR_JOINT_OBJECT_NO_Z)) {
                    pJoint->rotation.vz += *(u16*)pCursor;
                    pCursor += 2;
                    pObject->value.step.pCursor += 2;
                }
                break;

            case LIBGEAR_JOINT_OPCODE_INTERPOLATE:
                frame = pObject->curFrame + 1;
                pJoint->rotation.vx = pObject->value.lerp.startX +
                    (pObject->value.lerp.targetX * frame) / pObject->endFrame;
                pJoint->rotation.vy = pObject->value.lerp.startY +
                    (pObject->value.lerp.targetY * frame) / pObject->endFrame;
                pJoint->rotation.vz = pObject->value.lerp.startZ +
                    (pObject->value.lerp.targetZ * frame) / pObject->endFrame;
                break;

            case LIBGEAR_JOINT_OBJECT_STEP:
                dx = (pObject->value.lerp.targetX - pJoint->rotation.vx) / pObject->endFrame;
                dy = (pObject->value.lerp.targetY - pJoint->rotation.vy) / pObject->endFrame;
                dz = (pObject->value.lerp.targetZ - pJoint->rotation.vz) / pObject->endFrame;
                if ((dx == 0) && (dy == 0) && (dz == 0)) {
                    pObject->curFrame = pObject->endFrame;
                    pJoint->rotation.vx = pObject->value.lerp.targetX;
                    pJoint->rotation.vy = pObject->value.lerp.targetY;
                    pJoint->rotation.vz = pObject->value.lerp.targetZ;
                } else {
                    pJoint->rotation.vx += dx;
                    pJoint->rotation.vy += dy;
                    pJoint->rotation.vz += dz;
                    pObject->curFrame = 0;
                }
                break;

            case LIBGEAR_JOINT_OBJECT_ACCUMULATE:
                pObject->value.lerp.startX += pObject->value.lerp.targetX;
                pJoint->rotation.vx += pObject->value.lerp.startX;
                pObject->value.lerp.startY += pObject->value.lerp.targetY;
                pJoint->rotation.vy += pObject->value.lerp.startY;
                pObject->value.lerp.startZ += pObject->value.lerp.targetZ;
                pJoint->rotation.vz += pObject->value.lerp.startZ;
                break;

            case LIBGEAR_JOINT_OPCODE_MOVE_BASE:
            case LIBGEAR_JOINT_OPCODE_MOVE_BASE + 1:
                moveX = pObject->value.lerp.targetX - pJoint->translation.x;
                moveY = pObject->value.lerp.targetY - pJoint->translation.y;
                moveZ = pObject->value.lerp.targetZ - pJoint->translation.z;
                dist = SquareRoot0((moveX * moveX) + (moveY * moveY) + (moveZ * moveZ)) + 1;
                angle = (ratan2(-moveX, -moveZ) - pJoint->rotation.vy) & LIBGEAR_ANGLE_MASK;
                if (angle >= LIBGEAR_ANGLE_HALF_TURN) {
                    angle -= LIBGEAR_ANGLE_FULL_TURN;
                }
                maxStep = pObject->value.lerp.startY +
                    ((dist + pObject->curFrame) * pObject->value.lerp.startZ) /
                        pObject->value.lerp.startX;
                if ((angle < 0 ? -angle : angle) >= maxStep) {
                    angle = (angle < 0) ? -maxStep : maxStep;
                }
                pJoint->rotation.vy += angle;
                if ((opcode & LIBGEAR_JOINT_OPCODE_MASK) == LIBGEAR_JOINT_OPCODE_MOVE_BASE) {
                    angle = (ratan2(moveY, SquareRoot0((moveX * moveX) + (moveZ * moveZ))) -
                                pJoint->rotation.vx) &
                        LIBGEAR_ANGLE_MASK;
                    if (angle >= LIBGEAR_ANGLE_HALF_TURN) {
                        angle -= LIBGEAR_ANGLE_FULL_TURN;
                    }
                    if ((angle < 0 ? -angle : angle) >= maxStep) {
                        angle = (angle < 0) ? -maxStep : maxStep;
                    }
                    pJoint->rotation.vx += angle;
                }
                if (pObject->curFrame < LIBGEAR_JOINT_FRAME_ACCUMULATE_MAX) {
                    pObject->curFrame += pObject->endFrame;
                }
                break;
            }

            pObject->curFrame++;
            if (pObject->curFrame >= pObject->endFrame) {
                if (pObject->loops == 0) {
                    if (pObject->type == objectType) {
                        result |= LIBGEAR_JOINT_UPDATE_FINISHED;
                    }
                    result |= LIBGEAR_JOINT_UPDATE_FINISHED_ANY;
                    LibGearFreeJointObject(pList, (Temp3*)pObject);
                    pJoint->pRotationObject = NULL;
                } else {
                    if (pObject->type == objectType) {
                        result |= LIBGEAR_JOINT_UPDATE_LOOPED;
                    }
                    result |= LIBGEAR_JOINT_UPDATE_LOOPED_ANY;
                    if ((opcode & LIBGEAR_JOINT_OPCODE_MASK) < LIBGEAR_JOINT_OPCODE_INTERPOLATE) {
                        pObject->curFrame = 0;
                        pObject->value.step.pCursor = pObject->value.step.pScript;
                    } else {
                        pObject->curFrame = -1;
                        if ((opcode & LIBGEAR_JOINT_OPCODE_MASK) == LIBGEAR_JOINT_OBJECT_ACCUMULATE) {
                            pObject->value.lerp.targetX = 0;
                            pObject->value.lerp.targetY = 0;
                            pObject->value.lerp.targetZ = 0;
                        }
                    }
                }
            } else {
                if (pObject->type == objectType) {
                    result |= LIBGEAR_JOINT_UPDATE_ADVANCED;
                }
                result |= LIBGEAR_JOINT_UPDATE_ADVANCED_ANY;
            }

            pJoint->doRotate = TRUE;
            pJoint->doTransform = TRUE;
        }

        // Translation channel
        pObject = (JointObject*)pJoint->pTranslationObject;
        if (pObject != NULL) {
            opcode = pObject->opcode;

            switch (opcode & LIBGEAR_JOINT_OPCODE_MASK) {
            case LIBGEAR_JOINT_OBJECT_SET:
                pCursor = pObject->value.step.pCursor;
                if (!(opcode & LIBGEAR_JOINT_OBJECT_NO_X)) {
                    pJoint->translation.x = *(s16*)pCursor;
                    pCursor += 2;
                    pObject->value.step.pCursor += 2;
                }
                if (!(opcode & LIBGEAR_JOINT_OBJECT_NO_Y)) {
                    pJoint->translation.y = *(s16*)pCursor;
                    pCursor += 2;
                    pObject->value.step.pCursor += 2;
                }
                if (!(opcode & LIBGEAR_JOINT_OBJECT_NO_Z)) {
                    pJoint->translation.z = *(s16*)pCursor;
                    pCursor += 2;
                    pObject->value.step.pCursor += 2;
                }
                break;

            case LIBGEAR_JOINT_OBJECT_RELATIVE:
                if (!(opcode & LIBGEAR_JOINT_OBJECT_NO_X)) {
                    delta = *(s8*)pObject->value.step.pCursor++;
                    if (delta != LIBGEAR_SCRIPT_LITERAL) {
                        pJoint->translation.x += delta;
                    } else {
                        pCursor = pObject->value.step.pCursor;
                        pObject->value.step.pCursor++;
                        low = pCursor[0];
                        pObject->value.step.pCursor++;
                        delta = *(s8*)(pCursor + 1);
                        pJoint->translation.x = low | (delta << 8);
                    }
                }
                if (!(opcode & LIBGEAR_JOINT_OBJECT_NO_Y)) {
                    delta = *(s8*)pObject->value.step.pCursor++;
                    if (delta != LIBGEAR_SCRIPT_LITERAL) {
                        pJoint->translation.y += delta;
                    } else {
                        pCursor = pObject->value.step.pCursor;
                        pObject->value.step.pCursor++;
                        low = pCursor[0];
                        pObject->value.step.pCursor++;
                        delta = *(s8*)(pCursor + 1);
                        pJoint->translation.y = low | (delta << 8);
                    }
                }
                if (!(opcode & LIBGEAR_JOINT_OBJECT_NO_Z)) {
                    delta = *(s8*)pObject->value.step.pCursor++;
                    if (delta != LIBGEAR_SCRIPT_LITERAL) {
                        pJoint->translation.z += delta;
                    } else {
                        pCursor = pObject->value.step.pCursor;
                        pObject->value.step.pCursor++;
                        low = pCursor[0];
                        pObject->value.step.pCursor++;
                        delta = *(s8*)(pCursor + 1);
                        pJoint->translation.z = low | (delta << 8);
                    }
                }
                break;

            case LIBGEAR_JOINT_OBJECT_ADD:
                pCursor = pObject->value.step.pCursor;
                if (!(opcode & LIBGEAR_JOINT_OBJECT_NO_X)) {
                    vec.vx = *(u16*)pCursor;
                    pCursor += 2;
                    pObject->value.step.pCursor += 2;
                } else {
                    vec.vx = 0;
                }
                if (!(opcode & LIBGEAR_JOINT_OBJECT_NO_Y)) {
                    vec.vy = *(u16*)pCursor;
                    pCursor += 2;
                    pObject->value.step.pCursor += 2;
                } else {
                    vec.vy = 0;
                }
                if (!(opcode & LIBGEAR_JOINT_OBJECT_NO_Z)) {
                    vec.vz = *(u16*)pCursor;
                    pCursor += 2;
                    pObject->value.step.pCursor += 2;
                } else {
                    vec.vz = 0;
                }
                vec.vx = (vec.vx * pJoint->scale.vx) >> 12;
                vec.vy = (vec.vy * pJoint->scale.vy) >> 12;
                vec.vz = (vec.vz * pJoint->scale.vz) >> 12;
                ApplyMatrix(&pJoint->globalTransform, &vec, &out);
                pJoint->translation.x += (scale * out.vx) >> 12;
                pJoint->translation.y += (scale * out.vy) >> 12;
                pJoint->translation.z += (scale * out.vz) >> 12;
                break;

            case LIBGEAR_JOINT_OPCODE_INTERPOLATE:
                frame = pObject->curFrame + 1;
                pJoint->translation.x = pObject->value.lerp.startX +
                    (pObject->value.lerp.targetX * frame) / pObject->endFrame;
                pJoint->translation.y = pObject->value.lerp.startY +
                    (pObject->value.lerp.targetY * frame) / pObject->endFrame;
                pJoint->translation.z = pObject->value.lerp.startZ +
                    (pObject->value.lerp.targetZ * frame) / pObject->endFrame;
                break;

            case LIBGEAR_JOINT_OBJECT_STEP:
                dx = (pObject->value.lerp.targetX - pJoint->translation.x) / pObject->endFrame;
                dy = (pObject->value.lerp.targetY - pJoint->translation.y) / pObject->endFrame;
                dz = (pObject->value.lerp.targetZ - pJoint->translation.z) / pObject->endFrame;
                if ((dx == 0) && (dy == 0) && (dz == 0)) {
                    pObject->curFrame = pObject->endFrame;
                    pJoint->translation.x = pObject->value.lerp.targetX;
                    pJoint->translation.y = pObject->value.lerp.targetY;
                    pJoint->translation.z = pObject->value.lerp.targetZ;
                } else {
                    pJoint->translation.x += dx;
                    pJoint->translation.y += dy;
                    pJoint->translation.z += dz;
                    pObject->curFrame = 0;
                }
                break;

            case LIBGEAR_JOINT_OBJECT_ACCUMULATE:
                pObject->value.lerp.startX += pObject->value.lerp.targetX;
                pJoint->translation.x += pObject->value.lerp.startX;
                pObject->value.lerp.startY += pObject->value.lerp.targetY;
                pJoint->translation.y += pObject->value.lerp.startY;
                pObject->value.lerp.startZ += pObject->value.lerp.targetZ;
                pJoint->translation.z += pObject->value.lerp.startZ;
                break;
            }

            pObject->curFrame++;
            if (pObject->curFrame >= pObject->endFrame) {
                if (pObject->loops == 0) {
                    if (pObject->type == objectType) {
                        result |= LIBGEAR_JOINT_UPDATE_FINISHED;
                    }
                    result |= LIBGEAR_JOINT_UPDATE_FINISHED_ANY;
                    LibGearFreeJointObject(pList, (Temp3*)pObject);
                    pJoint->pTranslationObject = NULL;
                } else {
                    if (pObject->type == objectType) {
                        result |= LIBGEAR_JOINT_UPDATE_LOOPED;
                    }
                    result |= LIBGEAR_JOINT_UPDATE_LOOPED_ANY;
                    if ((opcode & LIBGEAR_JOINT_OPCODE_MASK) < LIBGEAR_JOINT_OPCODE_INTERPOLATE) {
                        pObject->curFrame = 0;
                        pObject->value.step.pCursor = pObject->value.step.pScript;
                    } else {
                        pObject->curFrame = -1;
                        if ((opcode & LIBGEAR_JOINT_OPCODE_MASK) == LIBGEAR_JOINT_OBJECT_ACCUMULATE) {
                            pObject->value.lerp.targetX = 0;
                            pObject->value.lerp.targetY = 0;
                            pObject->value.lerp.targetZ = 0;
                        }
                    }
                }
            } else {
                if (pObject->type == objectType) {
                    result |= LIBGEAR_JOINT_UPDATE_ADVANCED;
                }
                result |= LIBGEAR_JOINT_UPDATE_ADVANCED_ANY;
            }

            pJoint->doTransform = TRUE;
        }

        // Scale channel; it has no script form, so its opcode starts at the interpolation
        pObject = (JointObject*)pJoint->pScaleObject;
        if (pObject != NULL) {
            switch (pObject->opcode & LIBGEAR_JOINT_OPCODE_MASK) {
            case LIBGEAR_JOINT_OPCODE_INTERPOLATE:
                frame = pObject->curFrame + 1;
                pJoint->scale.vx = pObject->value.lerp.startX +
                    (pObject->value.lerp.targetX * frame) / pObject->endFrame;
                pJoint->scale.vy = pObject->value.lerp.startY +
                    (pObject->value.lerp.targetY * frame) / pObject->endFrame;
                pJoint->scale.vz = pObject->value.lerp.startZ +
                    (pObject->value.lerp.targetZ * frame) / pObject->endFrame;
                break;

            case LIBGEAR_JOINT_OBJECT_STEP:
                dx = (pObject->value.lerp.targetX - pObject->value.lerp.startX) /
                    pObject->endFrame;
                dy = (pObject->value.lerp.targetY - pObject->value.lerp.startY) /
                    pObject->endFrame;
                dz = (pObject->value.lerp.targetZ - pObject->value.lerp.startZ) /
                    pObject->endFrame;
                if ((dx == 0) && (dy == 0) && (dz == 0)) {
                    pObject->curFrame = pObject->endFrame;
                    pJoint->scale.vx = pObject->value.lerp.targetX;
                    pJoint->scale.vy = pObject->value.lerp.targetY;
                    pJoint->scale.vz = pObject->value.lerp.targetZ;
                } else {
                    pObject->value.lerp.startX += dx;
                    pObject->value.lerp.startY += dy;
                    pObject->value.lerp.startZ += dz;
                    pJoint->scale.vx = pObject->value.lerp.startX;
                    pJoint->scale.vy = pObject->value.lerp.startY;
                    pJoint->scale.vz = pObject->value.lerp.startZ;
                    pObject->curFrame = 0;
                }
                break;

            case LIBGEAR_JOINT_OBJECT_ACCUMULATE:
                pObject->value.lerp.startX += pObject->value.lerp.targetX;
                pJoint->scale.vx += pObject->value.lerp.startX;
                pObject->value.lerp.startY += pObject->value.lerp.targetY;
                pJoint->scale.vy += pObject->value.lerp.startY;
                pObject->value.lerp.startZ += pObject->value.lerp.targetZ;
                pJoint->scale.vz += pObject->value.lerp.startZ;
                break;
            }

            pObject->curFrame++;
            if (pObject->curFrame >= pObject->endFrame) {
                if (pObject->loops == 0) {
                    if (pObject->type == objectType) {
                        result |= LIBGEAR_JOINT_UPDATE_FINISHED;
                    }
                    result |= LIBGEAR_JOINT_UPDATE_FINISHED_ANY;
                    LibGearFreeJointObject(pList, (Temp3*)pObject);
                    pJoint->pScaleObject = NULL;
                } else {
                    if (pObject->type == objectType) {
                        result |= LIBGEAR_JOINT_UPDATE_LOOPED;
                    }
                    result |= LIBGEAR_JOINT_UPDATE_LOOPED_ANY;
                    pObject->curFrame = -1;
                    if ((pObject->opcode & LIBGEAR_JOINT_OPCODE_MASK) ==
                        LIBGEAR_JOINT_OBJECT_ACCUMULATE) {
                        pObject->value.lerp.targetX = 0;
                        pObject->value.lerp.targetY = 0;
                        pObject->value.lerp.targetZ = 0;
                    }
                }
            } else {
                if (pObject->type == objectType) {
                    result |= LIBGEAR_JOINT_UPDATE_ADVANCED;
                }
                result |= LIBGEAR_JOINT_UPDATE_ADVANCED_ANY;
            }

            pJoint->doRotate = TRUE;
            pJoint->doTransform = TRUE;
        }
    }

    return result;
}*/
INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801DDBF8);




// objdiff 96.81%, 420 bytes / 105 insns = retail's exact size; the loop body,
// both arm guards and the cursor (a0 = &joint->doTransform, a0 += 0x7C) are
// instruction-identical. Up from 95.98 by twin-porting battle/main/misc21
// func_800A1B50's parked body (same 105-insn function in the battle binary,
// 97.90% there): the loads in source order type,numRot,numTrans,flags, a s32
// skipFirst = blockType == 0 local deciding the if, and rotFrame/transFrame
// assigned as statements instead of initialisers (that alone fixed the type
// load's position: 94.95 -> 96.81).
// Residual, both in the prologue/epilogue, and both allocation:
// 1) retail holds flags in a2 and pKeyframe in a1 with the add in-place
//    (addiu a1,a1,0x18, reorg then pulls it into the bnez delay slot). Here
//    gcc 2.6 emits addiu a2,a1,24 BEFORE the lhu a1,4(a1) flags load, so pBlock
//    is still live when pKeyframe is allocated and takes a2. Spellings measured
//    identical (all fold back to one add of a fresh pseudo): (s16*)pBlock + 12,
//    'pKeyframe = (s16*)pBlock; pKeyframe += 12;', and the parameter advance
//    'pBlock = (KeyframeBlock*)((s16*)pBlock + 12); pKeyframe = (s16*)pBlock;'
//    which was the diagnostic for whether an in-place update is reachable.
// 2) the back edge's bnez delay slot: retail duplicates andi v0,count there,
//    ours has nop. Blocked on (1) changing which insns reorg can move.
// Also tried and rejected: s16 x/y/z locals (74.79%), blockType == 0 inline
// instead of skipFirst (94.95), gcc 2.7.2 (94.95), 2.7.2-cdk (92.43), cdk +
// aspsx 2.67 (89.90).
// R462 closed residual (1) as a class rather than a spelling, on this function
// and on func_801DF0B4 together (levers.md
// a-parameter-home-register-is-abandoned-when-its-block-sets-it):
// - 'pBlock++; pKeyframe = (s16*)pBlock;' - sets on the parameter - is
//   byte-identical to this body: gcc gives the incremented value a fresh pseudo
//   and leaves a1 live at the add, so retail's in-place add on the parameter's
//   own register is not reachable from source.
// - the two residuals are mutually exclusive: loading flags before type (all
//   four field loads then read the same offsets) puts addiu a1,a1,24 in the
//   bnez delay slot exactly as retail has it, but flags takes a3 and y stays
//   mirrored - 95.93%, 48 differing lines against this body's 50.
// R475: best stays 96.81%, 420 bytes / 105 insns (retail exact size); body
// re-measured from this park. Re-measured the flags-first variant of the
// prologue: 95.93%, 48 differing lines, addiu in the bnez delay slot exactly
// where retail has it but still on a2 with flags in a3 (retail: cursor a1,
// flags a2, blockType t0), so residual (1) is confirmed a register tie and not
// a statement order. No new lever found; do not re-try load order.
/*
void func_801DEF10(ModelJoint* pSkeleton, KeyframeBlock* pBlock) {
    u16 rotFrame;
    u16 transFrame;
    s16 blockType;
    s32 skipFirst;
    u16 numRotFrames;
    u16 numTransFrames;
    u16 flags;
    s16* pKeyframe;
    u16 count;
    int i;
    int x;
    int y;
    int z;
    ModelJoint* pJoint;

    rotFrame = 0;
    transFrame = 0;
    blockType = pBlock->type;
    skipFirst = blockType == 0;
    numRotFrames = pBlock->numRotFrames;
    numTransFrames = pBlock->numTransFrames;
    flags = pBlock->flags;
    pKeyframe = (s16*)(pBlock + 1);
    if (skipFirst) {
        pKeyframe += (numRotFrames + 1) * 3;
    }

    count = pSkeleton->numChildren - 1;
    pJoint = pSkeleton;
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
}*/
INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801DEF10);



// R449 additions to the note below, all measured this round:
// - The keyframe-pointer tie is the SAME mechanism that blocks func_801DEF10 in
//   this unit: retail coalesces the cursor onto the pBlock parameter register
//   (in-place 'addiu s1,s1,0x18'), the compile always allocates a fresh
//   register (here s2) because the add's input is still live at its def. Four
//   spellings on func_801DEF10 measured byte-identical (a fresh a2 there):
//   '(s16*)pBlock + 12', 'p = (s16*)pBlock; p += 12;' (copy-prop folds the two
//   back into one add of a new pseudo), 'pBlock = (KeyframeBlock*)((s16*)pBlock
//   + 12); pKeyframe = pBlock;', and pBlock->keyframes. So gcc 2.6 does not
//   produce the in-place add from any of them; a future round should attack it
//   once for BOTH functions (see levers.md n-a-symbol-plus-a-big-immediate /
//   the two-address addsi3 note in world_map/misc6_b for the tie's mechanism).
// - u16 'frame' instead of s32: 95.57% but 1148 bytes / 287 insns (one over
//   retail's 1144), so the local is s32 with a 16-bit read retail gets from a
//   HImode load of its stack slot ('lhu t0,16(sp)' against our 'lw'): that is
//   the one non-register delta left in the body.
// - try_flags gcc-2.7.2-psx: 95.19, so the unit's preset is not the question.
// - the (s8*) casts on the two LibGearFreeJointObject calls are gone (it takes
//   Temp3*); byte-identical.
// - R462: the entry 'move s1,a2' + in-place 'addiu s1,s1,0x18' is ONE pseudo in
//   retail - the four header loads read through it (lhu t0,12(s1) ...
//   lhu t0,4(s1)) and then it is incremented in place. gcc 2.6.0 cannot produce
//   it: it propagates the parameter copy into those loads and gives the add a
//   fresh pseudo, so the header stays on a2 and the cursor takes s2. Measured
//   byte-identical to the body below (95.40%, 168 differing lines):
//   'pKeyframe = (s16*)pBlock; pKeyframe += 12;', 'pBlock++; pKeyframe =
//   (s16*)pBlock;', and 'pHeader = pBlock;' with all four header loads through
//   pHeader then 'pHeader++; pKeyframe = (s16*)pHeader;'. See levers.md
//   a-parameter-home-register-is-abandoned-when-its-block-sets-it.
// objdiff 95.40%, 1144 bytes (retail's exact size, 286 instructions), every
// branch target and every slot at the same offset as the original. Both
// channels reproduce retail's own structure: the whole-word read of the
// rotation (lh x/y/z, compare, 0xFFF wrap) and of the translation (lw/lhu,
// plain difference), the release path shared by the guard-fail, unchanged and
// alloc-fail exits with the success path jumping over it, the opcode = flag + 3
// fill, and the `if (pObject != NULL) { type test } else { alloc }` spelling of
// the allocate-or-reuse step (with `else if` in the rotation arm instead it is
// 1140 bytes / 92.27%, and in both arms 1128 / 88.26%).
// What is left is ONE allocator tie that cascades: the original has pKeyframe in
// s1 with pBlock coalesced onto it (hence the entry `move s1,a2` and the in-place
// `addiu s1,s1,0x18`), x/y/z in s2/s3/s4, and pObject in a0 (hence the extra
// `addu a0,v0,zero`); here x wins s1, so pKeyframe takes s2 and the whole
// cascade shifts. Declaration order (pKeyframe first, x/y/z first, pObject
// first), s16 instead of int x/y/z, and reusing `i` for blockType (which the
// permuter proposed) all measure 95.40%. Three permuter runs (base 3748 -> 3274
// -> 2776 -> 2514 on its own word metric) produced the two arm spellings above
// and nothing further; the register swap is the remaining 4.6% (14 instructions
// of 286).
// The arguments are s32 on the evidence of the original's in-place `sw` of
// `unk6 & 1`; it then reads both (lbu 0x8C(sp), lbu 0x90(sp)) a byte at a time,
// which s32 cannot reproduce (it emits `lw`), while u8 grows the frame to 0x80
// for `type` alone and 0x88 for both by spilling them into new slots.
// R475: re-measured 95.40%, 1144 bytes (retail exact), 286 insns, 168 differing
// lines; body unchanged. Closed the frame slot access widths (the one non
// register delta): retail stores numFrames slot 0x10(sp) with `sw` twice and
// reads it with `lhu` twice at `endFrame = frame`. s32 `frame` (this body) =
// sw + lw; `u16 frame` + the test on the parameter (`if (numFrames == 0)
// frame = 1;`) = sh + lhu at 95.40 / 1144 / 286 (retail reads, wrong stores);
// `u16 frame` + test on frame = sh + lhu 95.57 / 1148 / 287 (one insn over, so
// unlinkable); `u16 numFrames` in the signature = 93.67 / 1164 / 291, frame
// 0x84. No spelling gives word stores with halfword reads, so the local stays
// s32 and the register tie of the note above is the whole residual.
/*
s32 func_801DF0B4(Temp4* pList, ModelJoint* pSkeleton, KeyframeBlock* pBlock, s32 numFrames,
                  s32 absolute, s32 unk6, s32 type) {
    Temp3* pObject;
    s32 frame;
    u16 numRotFrames;
    u16 rotFrame;
    u16 numTransFrames;
    u16 transFrame;
    u16 count;
    u16 blockFlags;
    s16 blockType;
    s16* pKeyframe;
    ModelJoint* pJoint;
    int i;
    int x;
    int y;
    int z;

    frame = numFrames;
    if (frame == 0) {
        frame = 1;
    }
    rotFrame = 0;
    transFrame = 0;

    unk6 &= 0x1;

    numRotFrames = pBlock->numRotFrames;
    blockType = pBlock->type;
    numTransFrames = pBlock->numTransFrames;
    absolute &= 0x1;
    blockFlags = pBlock->flags;
    pKeyframe = pBlock->keyframes;

    if (blockType == 0) {
        pKeyframe += (numRotFrames + 1) * 3;
    }

    count = pSkeleton->numChildren - 1;

    pJoint = pSkeleton;
    for (i = 0; i < count; i++) {
        pJoint++;

        if (!(blockFlags & 0x1) && rotFrame < numRotFrames) {
            x = *pKeyframe++;
            y = *pKeyframe++;
            z = *pKeyframe++;
            rotFrame++;

            if (pJoint->rotation.vx != x || pJoint->rotation.vy != y || pJoint->rotation.vz != z) {
                pObject = pJoint->pRotationObject;
                if (pObject != NULL) {
                    if (pObject->type == 0xFF) {
                        goto skipRotation;
                    }
                } else {
                    pObject = LibGearAllocJointObject(pList);
                }

                if (pObject != NULL) {
                    pObject->inUse = 1;
                    pObject->loops = unk6;
                    pObject->opcode = absolute + 3;
                    pObject->type = type;

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

                    if (absolute) {
                        pObject->targetX += pJoint->rotation.vx;
                        pObject->targetY += pJoint->rotation.vy;
                        pObject->targetZ += pJoint->rotation.vz;
                    }

                    pObject->curFrame = 0;
                    pObject->endFrame = frame;
                    pJoint->pRotationObject = pObject;
                    goto skipRotation;
                }
            }
        }

        pObject = pJoint->pRotationObject;
        if (pObject != NULL && pObject->type != 0xFF) {
            LibGearFreeJointObject(pList, pObject);
            pJoint->pRotationObject = NULL;
        }

    skipRotation:

        if (!(blockFlags & 0x2) && transFrame < numTransFrames) {
            x = *pKeyframe++;
            y = *pKeyframe++;
            z = *pKeyframe++;
            transFrame++;

            if (pJoint->translation.x != x || pJoint->translation.y != y || pJoint->translation.z != z) {
                pObject = pJoint->pTranslationObject;
                if (pObject != NULL) {
                    if (pObject->type == 0xFF) {
                        goto skipTranslation;
                    }
                } else {
                    pObject = LibGearAllocJointObject(pList);
                }

                if (pObject != NULL) {
                    pObject->inUse = 1;
                    pObject->loops = unk6;
                    pObject->opcode = absolute + 3;
                    pObject->type = type;

                    pObject->startX = pJoint->translation.x;
                    pObject->startY = pJoint->translation.y;
                    pObject->startZ = pJoint->translation.z;

                    if (absolute) {
                        pObject->targetX = x;
                        pObject->targetY = y;
                        pObject->targetZ = z;
                    } else {
                        pObject->targetX = x - pJoint->translation.x;
                        pObject->targetY = y - pJoint->translation.y;
                        pObject->targetZ = z - pJoint->translation.z;
                    }

                    pObject->curFrame = 0;
                    pObject->endFrame = frame;
                    pJoint->pTranslationObject = pObject;
                    goto skipTranslation;
                }
            }
        }

        pObject = pJoint->pTranslationObject;
        if (pObject != NULL && pObject->type != 0xFF) {
            LibGearFreeJointObject(pList, pObject);
            pJoint->pTranslationObject = NULL;
        }

    skipTranslation:;
    }

    return count;
}*/
INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801DF0B4);




void LibGearReleaseJointObjects(Temp4* pList, ModelJoint* pSkeleton, s32 index, s32 flags) {
    if (index < pSkeleton->numChildren) {
        pSkeleton += index;

        if (pSkeleton->pRotationObject != NULL && (flags & LIBGEAR_JOINT_RELEASE_ROTATION)) {
            LibGearFreeJointObject(pList, pSkeleton->pRotationObject);
            pSkeleton->pRotationObject = 0;
        }

        if (pSkeleton->pTranslationObject != NULL && (flags & LIBGEAR_JOINT_RELEASE_TRANSLATION)) {
            LibGearFreeJointObject(pList, pSkeleton->pTranslationObject);
            pSkeleton->pTranslationObject = 0;
        }

        if (pSkeleton->pScaleObject != NULL && (flags & LIBGEAR_JOINT_RELEASE_SCALE)) {
            LibGearFreeJointObject(pList, pSkeleton->pScaleObject);
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

int LibGearFreeJointObject(Temp4* pList, Temp3* pObject) {
    int index;

    if (pObject == NULL) {
        return -1;
    }

    index = ((mem_addr)pObject - (mem_addr)pList->pObjects) / sizeof(Temp3);
    if (index < pList->freeIndex) {
        pList->freeIndex = index;
    }
    pObject->inUse = 0;
    return index;
}

// objdiff 74.39%, 720 bytes and 180 instructions: retail's exact size and count.
// Every instruction of the loop is present; what is left is a callee-saved
// permutation: retail keeps pSkeleton in $s0 (advanced as the joint cursor) and
// pBlock/$s1 (advanced to the keyframe cursor), ours gives pBlock $s0 and
// pSkeleton $s5, with frame/flag1 mirrored between $s6/$s8.
// Shapes that got here and that the next attempt should keep: `s32 type` (a u8
// parameter makes gcc copy it to a fresh 0x10(sp) slot and the frame grows to
// 0x50; s32 keeps retail's 0x48 frame and count at 0x10(sp)), no pJoint local
// (advance pSkeleton itself), a pOpcode local = (u8*)&pKeyframe[2] live across
// both calls (this is what takes a callee-saved register in retail), and
// JointObject::value.step.pScript/pCursor as two statements for retail's
// sw 0x4 / sw 0x8 order.
// Tried: u8 type (frame 0x50, 73.57), a separate pJoint cursor (74.31), 0xFFFF
// and 0xFF compared against rematerialised constants, declaration-order and
// (u16)/s16 cast variations on endFrame/numRotFrames.
/*
s32 func_801DF7F4(Temp4* pList, ModelJoint* pSkeleton, KeyframeBlock* pBlock, s32 flags, s32 type) {
    u16 numRotFrames;
    u16 numTransFrames;
    u16 blockFlags;
    u16 count;
    u8 flag1;
    u16 frame;
    u16* pKeyframe;
    u8* pOpcode;
    u8* pVertexData;
    u8* pVertex;
    JointObject* pObject;
    int i;

    if (pBlock->type != 0) {
        LibGearReleaseSkeletonObjects(pList, pSkeleton);
        func_801DEF10(pSkeleton, pBlock);
        return 1;
    }

    numRotFrames = pBlock->numRotFrames;
    count = pSkeleton->numChildren;
    if (numRotFrames + 1 < count) {
        count = numRotFrames + 1;
    }
    numTransFrames = pBlock->numTransFrames;
    flag1 = flags & 0x1;
    frame = pBlock->endFrame;
    if (flag1 == 0) {
        frame--;
    }
    blockFlags = pBlock->flags;

    pKeyframe = (u16*)pBlock->keyframes;
    pVertexData = (u8*)pKeyframe + (numRotFrames + 1) * 6;
    if (!(blockFlags & 0x1)) {
        pVertexData += numRotFrames * 6;
    }
    if (!(blockFlags & 0x2)) {
        pVertexData += numTransFrames * 6;
    }

    for (i = 0; i < count; i++) {
        pOpcode = (u8*)&pKeyframe[2];
        if (pKeyframe[0] != LIBGEAR_JOINT_INDEX_NONE) {
            pObject = (JointObject*)pSkeleton->pRotationObject;
            if (pObject == NULL) {
                pObject = (JointObject*)LibGearAllocJointObject(pList);
            } else if (pObject->type == 0xFF) {
                goto next;
            }
            if (pObject != NULL) {
                pObject->inUse = 1;
                pObject->loops = flag1;
                pObject->type = type;
                pObject->opcode = pOpcode[0];
                pObject->curFrame = 0;
                pObject->endFrame = frame;
                pVertex = pVertexData + pKeyframe[0];
                pObject->value.step.pScript = pVertex;
                pObject->value.step.pCursor = pVertex;
                pSkeleton->pRotationObject = (Temp3*)pObject;
            }
        } else {
            pObject = (JointObject*)pSkeleton->pRotationObject;
            if (pObject != NULL && i != 0 && pObject->type != 0xFF) {
                LibGearFreeJointObject(pList, (Temp3*)pObject);
                pSkeleton->pRotationObject = NULL;
            }
        }
    next:
        pKeyframe++;

        if (pKeyframe[0] != LIBGEAR_JOINT_INDEX_NONE) {
            pObject = (JointObject*)pSkeleton->pTranslationObject;
            if (pObject == NULL) {
                pObject = (JointObject*)LibGearAllocJointObject(pList);
            } else if (pObject->type == 0xFF) {
                goto next2;
            }
            if (pObject != NULL) {
                pObject->inUse = 1;
                pObject->loops = flag1;
                pObject->type = type;
                pObject->opcode = pOpcode[1];
                pObject->curFrame = 0;
                pObject->endFrame = frame;
                pVertex = pVertexData + pKeyframe[0];
                pObject->value.step.pScript = pVertex;
                pObject->value.step.pCursor = pVertex;
                pSkeleton->pTranslationObject = (Temp3*)pObject;
            }
        } else {
            pObject = (JointObject*)pSkeleton->pTranslationObject;
            if (pObject != NULL && i != 0 && pObject->type != 0xFF) {
                LibGearFreeJointObject(pList, (Temp3*)pObject);
                pSkeleton->pTranslationObject = NULL;
            }
        }
    next2:
        pKeyframe += 2;
        pSkeleton++;
    }

    return 0;
}*/
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
// Re-measured round 400 with the same two changes that helped func_801DF7F4:
// a pOpcode local = (u8*)&pKeyframe[2] used for both opcode bytes (retail
// spills exactly that to 0x18(sp)) plus `s32 type` measured 80.21% / 1016
// bytes / 254 insns, and with u8 type 79.33% / 1020 / 255; both are worse
// than the 996 / 249 above, and s32 type also spills flag1 to 56(sp) where
// retail keeps it in $fp, so the body below stays as measured.
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
                pObject->loops = flag1;
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
                pObject->loops = flag1;
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
        if (pSkeleton->pRotationObject != 0 && pSkeleton->pRotationObject->type != LIBGEAR_JOINT_OBJECT_TYPE_NONE) {
            LibGearFreeJointObject(pList, pSkeleton->pRotationObject);
            pSkeleton->pRotationObject = 0;
        }

        if (pSkeleton->pTranslationObject != 0 && pSkeleton->pTranslationObject->type != LIBGEAR_JOINT_OBJECT_TYPE_NONE) {
            LibGearFreeJointObject(pList, pSkeleton->pTranslationObject);
            pSkeleton->pTranslationObject = 0;
        }

        if (pSkeleton->pScaleObject != 0 && pSkeleton->pScaleObject->type != LIBGEAR_JOINT_OBJECT_TYPE_NONE) {
            LibGearFreeJointObject(pList, pSkeleton->pScaleObject);
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
            LibGearFreeJointObject(pList, pSkeleton->pRotationObject);
            pSkeleton->pRotationObject = 0;
        }

        if (pSkeleton->pTranslationObject != 0 && pSkeleton->pTranslationObject->type == type) {
            LibGearFreeJointObject(pList, pSkeleton->pTranslationObject);
            pSkeleton->pTranslationObject = 0;
        }

        if (pSkeleton->pScaleObject != 0 && pSkeleton->pScaleObject->type == type) {
            LibGearFreeJointObject(pList, pSkeleton->pScaleObject);
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
        pVertex->age = LIBGEAR_VERTEX_AGE_FREE;
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
        if (pVertex->age == LIBGEAR_VERTEX_AGE_FREE) {
            for (
                pVertexBuffer->curIndex++; 
                pVertexBuffer->curIndex < pVertexBuffer->capacity; 
                pVertexBuffer->curIndex++
            ) {
                if (pVertexBuffer->pVertices[pVertexBuffer->curIndex].age == LIBGEAR_VERTEX_AGE_FREE) {
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
    
    pObject->age = LIBGEAR_VERTEX_AGE_FREE;
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
        if (pVertex->age == LIBGEAR_VERTEX_AGE_FREE) {
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
        pEntry->pVertexBuffer = arg1;
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
        pEntry->pVertex = 0;

        return 0;
    }
}

void LibGearClearColorEntry(short* pValue, s32 arg1) {
    *pValue = LIBGEAR_COLOR_ENTRY_JOINT_NONE;
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

// objdiff 87.43%, 2120 bytes / 530 insns against retail's 2136 (0x858) / 534. Every
// block below is decoded from the .s and reproduced; the whole residual is register
// pressure plus the frame, and the frame is now pinned to ONE cause.
// THE FRAME, measured (round 493): retail is 0x108 (vars 0xD0), this body 0xF8
// (vars 0xC0), and the 16 bytes are EXACTLY the WIDTH and HEIGHT word spills. Both
// sides spill the same low area at the same offsets - 0x10 RECT, 0x18 pSource,
// 0x20 mode (sh), 0x28 flags, 0x30 pRows, 0x38..0x70 the eight u16 area args, then
// retail's pInterpolate at 0x88; this body has pInterpolate at 0x78 because retail
// puts `sw 0x78(sp)` = width and `sw 0x80(sp)` = height in front of it (0x84 unused).
// So no dummy local can close it: everything below 0x78 already matches offset for
// offset, and adding 16 bytes of dead locals at the top only pushes the spills.
// WHY RETAIL SPILLS THEM AND WE DO NOT (the actual blocker): the wide width/height
// have exactly ONE wide use each - the `== 0` default tests, which retail does with
// `lw 0x78(sp)` + `sll 16` + `bnez` (a NARROW test of the word) - and every other use
// is narrow, so gcc 2.6 has a free choice: retail kept the NARROW values in $s3/$s2
// and left the words in memory, this body keeps the WIDE values in $s2/$s3 and
// re-forms the narrow with `sll 16`/`sra 16` at every use. That is the entire 534 vs
// 530 delta and most of the 394 differing rows: retail's `mult s3,s2` and its three
// HeapAlloc size multiplications have no sll/sra pair, ours all do.
// Also allocation-only, not structure: retail reads the mask narrow in its prologue
// (`sw a3,0x28(sp)` + `lhu s5,0x28(sp)`) and stores `pTexture->mode` with
// `lbu 0x20(sp)`, where this body does `lw 0x28(sp)` + `move s5,t6` and
// `lhu 0x20(sp)`+`sb` (2 instructions); retail keeps the flat fill colour in $a0
// while this body keeps it in $s1 ($s1 holds the DITHER colour in retail too, so the
// flat colour there is a block-local that dies with the loop); in both copy loops
// retail re-loads pRows from 0x30(sp) inside the row prologue (`lw t6,0x30(sp)` then
// `addu t0,v0,t6` last) where this body hoists it into $t3 and adds it early.
// Measured this round, all worse or byte-identical: `if ((u16)width == 0)` for the
// two default tests (87.23, 2120); a block-local `u16 flat` inside each flat-fill
// case instead of the function-scope `colour` (BYTE-IDENTICAL - the colour stays in
// $s1, so it is not the live range that puts it there); gcc 2.7.2 (84.42) and
// 2.7.2-cdk (80.00) via try_flags.sh. Not tried, and the next thing worth a lane:
// explicit narrow locals (`s16 w = width; h = height;` refreshed after
// `width = half * 2`) so the VALUES IN $s2/$s3 are narrow the way retail's are and
// the wide ones are left live only to the two tests, which is the shape that makes
// gcc spill them (lever j-a-spilled-scalar-slot-width-names-its-type, inverse).
// WHAT THE CODE DOES (for the next round): fills a Temp8's three buffers from the
// flags mask - 0x100 allocates pResultPixels, 0x200 pStartPixels, 0x400 pPixels, each
// (memoWidth * memoHeight * 2) bytes - then per 4-bit op nibble either uploads the
// buffer with StoreImage at the triple's (x,y) or fills it (flat 15-bit colour, or
// a 3-colour stripe picked by `(y + row) % 3`), or copies rows out of pRows.
// Mode 0/1 rounds the width up to even ((width + 1) / 2 * 2); mode 4/5 uses it raw
// and also handles the second buffer.
// SOLVED, DO NOT RE-DERIVE:
// - the mode dispatch: a `switch (mode)` with `case 0: case 1:` and
//   `case 4: case 5:` grouped plus `default: return pTexture;` and a trailing
//   `return pTexture;` reproduces retail's `bltz / slti 2 / slti 6 / slti 4` tree
//   AND its two separate routes to the epilogue (an empty case group for 2/3 does
//   not).
// - the mask adjustments are `if ((mode & 1) == 0) mask &= 0xFD0F;` but
//   `if (mode & 4) mask &= 0xFEFF;` - the second one's polarity is retail's
//   (the AND is in the fall-through).
// - the flat colour is `(((b & 0x3F) << 10) + ((g & 0x1F) << 5)) | (r & 0x1F)`:
//   `+` for the first two fields then `|` (all `|` gives or/or, all `+` differs).
// - `half` must be an int: `half = ((s16)width + 1) / 2;` gives retail's real
//   signed /2 (srl 31 / addu / sra 1); an `s16 half` gives sll 15 / sra 16.
// - the copy loop index needs the cast: `pPixels[row * (s16)width + col]` keeps the
//   `sra / mult / mflo` inside the inner loop as retail has it, while
//   `row * width + col` makes gcc hoist the product into the row preheader.
// - pRows is read as `pRows[3 * ((s16)y0 + row) + (s16)x0 + col]` (the casts are
//   hoisted to one sra each, retail's t4/t2), and the pattern fill is
//   `switch (((s16)y + row) % 3)` with the modulo hoisted once per row (retail's
//   `lui/ori 0x55555556` magic-multiply).
// - each buffer's three parameters serve as its region (x, y), its StoreImage rect
//   origin AND its 15-bit fill colour (r = x, g = y, b = the third) - that is what
//   retail does, not a transcription slip.
// - the prototype is 19 parameters (the caller pushes 15 words at 0x10..0x48) and
//   width/height are the two 32-bit stack args at words 9/10: retail reads them with
//   `lw`, so they are NOT u16 parameters.
// Tried and worse: `s16` loop counters (sign-extends every iteration), an
// `int count` local for the fill loops (the permuter's 4-minute run, base 6619 ->
// 6159, found ONLY that the product belongs inline in the loop condition, which is
// in the body below).
/*
Temp8* func_801E0A00(Temp8* pTexture, Temp8* pSource, u16 mode, u32 flags, s16* pRows, u16 x0, u16 y0,
                     u16 colour0, u16 x1, u16 y1, u16 colour1, u16 x, u16 y, u32 argWidth, u32 argHeight,
                     u16 angleStep, u16 divisor, u16 offset, s16 (*pInterpolate)(u_short, s16, s16)) {
    u32 width = argWidth;
    u32 height = argHeight;
    RECT rect;
    u16 mask;
    u16 colour;
    int half;
    int row;
    int col;
    int pixel;
    int pixelB;
    int i;

    if (pTexture->isLoaded != 0) {
        return NULL;
    }

    HeapChangeCurrentUser(HEAP_USER_MASA, HEAP_CONTENT_NULL);

    pTexture->isLoaded = 1;
    pTexture->unk11 = 0;
    pTexture->mode = mode;
    pTexture->pSource = pSource;
    pTexture->angle = 0;
    pTexture->prevFrame = 0xFFFF;
    pTexture->angleStep = angleStep;
    pTexture->divisor = divisor;
    pTexture->offset = offset;
    pTexture->pInterpolate = pInterpolate;
    pTexture->pRows = pRows;

    mask = flags;
    if ((mode & 1) == 0) {
        mask &= 0xFD0F;
    }
    if (mode & 4) {
        mask &= 0xFEFF;
    }
    if (width == 0) {
        width = 0x100;
    }
    if (height == 0) {
        height = 0x100;
    }

    switch (mode) {
        case 0:
        case 1:
            half = ((s16)width + 1) / 2;
            width = half * 2;
            pTexture->rect.x = x;
            pTexture->rect.y = y;
            pTexture->rect.w = width;
            pTexture->rect.h = height;
            pTexture->pixelCount = width * height;
            if (mask & 0x100) {
                pTexture->pResultPixels = HeapAlloc((s16)width * (s16)height * 2, HEAP_CONTENT_NULL);
            }
            if (mask & 0x200) {
                pTexture->pStartPixels = HeapAlloc((s16)width * (s16)height * 2, HEAP_CONTENT_NULL);
            }
            if (mask & 0x400) {
                pTexture->pPixels = HeapAlloc((s16)width * (s16)height * 2, HEAP_CONTENT_NULL);
            }

            switch (mask & 0xF) {
                case 1:
                    rect.x = x0;
                    rect.y = y0;
                    rect.w = width;
                    rect.h = height;
                    StoreImage(&rect, pTexture->pPixels);
                    DrawSync(0);
                    break;
                case 2:
                    colour = (((colour0 & 0x3F) << 10) + ((y0 & 0x1F) << 5)) | (x0 & 0x1F);
                    for (i = 0; i < (s16)width * (s16)height; i++) {
                        pTexture->pPixels[i] = colour;
                    }
                    break;
            }

            switch ((mask >> 4) & 0xF) {
                case 1:
                    rect.x = x1;
                    rect.y = y1;
                    rect.w = width;
                    rect.h = height;
                    StoreImage(&rect, pTexture->pStartPixels);
                    DrawSync(0);
                    break;
                case 2:
                    colour = (((colour1 & 0x3F) << 10) + ((y1 & 0x1F) << 5)) | (x1 & 0x1F);
                    for (i = 0; i < (s16)width * (s16)height; i++) {
                        ((u16*)pTexture->pStartPixels)[i] = colour;
                    }
                    break;
            }
            break;

        case 4:
        case 5:
            pTexture->rect.x = x;
            pTexture->rect.y = y;
            pTexture->rect.w = width;
            pTexture->rect.h = height;
            pTexture->pixelCount = width * height;
            if (mask & 0x100) {
                pTexture->pResultPixels = HeapAlloc((s16)width * (s16)height * 2, HEAP_CONTENT_NULL);
            }
            if (mask & 0x200) {
                pTexture->pStartPixels = HeapAlloc((s16)width * (s16)height * 2, HEAP_CONTENT_NULL);
            }
            if (mask & 0x400) {
                pTexture->pPixels = HeapAlloc((s16)width * (s16)height * 2, HEAP_CONTENT_NULL);
            }

            switch (mask & 0xF) {
                case 1:
                    for (row = 0; row < (s16)height; row++) {
                        for (col = 0; col < (s16)width; col++) {
                            pTexture->pPixels[row * (s16)width + col] = pRows[3 * ((s16)y0 + row) + (s16)x0 + col];
                        }
                    }
                    break;
                case 2:
                    pixel = 0;
                    for (row = 0; row < (s16)height; row++) {
                        for (col = 0; col < (s16)width; col++) {
                            switch (((s16)y + row) % 3) {
                                case 0:
                                    colour = x0;
                                    break;
                                case 1:
                                    colour = y0;
                                    break;
                                case 2:
                                    colour = colour0;
                                    break;
                            }
                            pTexture->pPixels[pixel + col] = colour;
                        }
                        pixel += width;
                    }
                    break;
            }

            switch ((mask >> 4) & 0xF) {
                case 1:
                    for (row = 0; row < (s16)height; row++) {
                        for (col = 0; col < (s16)width; col++) {
                            ((u16*)pTexture->pStartPixels)[row * (s16)width + col] = pRows[3 * ((s16)y1 + row) + (s16)x1 + col];
                        }
                    }
                    break;
                case 2:
                    pixelB = 0;
                    for (row = 0; row < (s16)height; row++) {
                        for (col = 0; col < (s16)width; col++) {
                            switch (((s16)y + row) % 3) {
                                case 0:
                                    colour = x1;
                                    break;
                                case 1:
                                    colour = y1;
                                    break;
                                case 2:
                                    colour = colour1;
                                    break;
                            }
                            ((u16*)pTexture->pStartPixels)[pixelB + col] = colour;
                        }
                        pixelB += width;
                    }
                    break;
            }
            break;
    }

    return pTexture;
}*/
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
// Round 479: best 89.35% / 1036 bytes (retail 1028 = 257 insns; ours 259). Ported the battle twin's SOLVED shape (misc22 func_800A3E98) into this body: u16 prevFrame local, all the work inside 'if (frame != prevFrame)', the two pSource checks returning 'level' early, and 'return prevFrame;' after the block. That buys retail's prevFrame BRANCH SHAPE for free (beq s1,v0,<epilogue>, no move/j) - 1048 -> 1036 bytes, 88.98 -> 89.35. Also measured this round: return type s32 gives the same 1036 bytes but 88.32 (its equal path folds to a move v0,s1), and 'return level;' under an s16 return type is byte-identical to 'return (s16)level;'. WHAT IS LEFT, +2 instructions: (1) the entry - retail keeps the RAW callback result in s2 and the s16 in s1 ('move s2,v0 / sll v0,s2,16 / sra s1,v0,16', so the 0x18 store is a free 'sh s2,0x18'), ours truncates into s2 and copies to s1 and then needs 'move v1,s2' for the store; every spelling of the s32 'level' local coalesces into the truncated value because all its uses (halfword store, s16 returns) truncate. (2) the equal path goes through one truncating exit (+1) - the record_jump_equiv fold the battle note says does not go away. Loop register/interleave deltas (retail t3/t2 vs ours t2/a1) unchanged.
// Round 479: 'pTexture->prevFrame = frame;' measures IDENTICAL to '= level;' (1036 bytes / 259 insns, 89.35), so the store's source is not where the surplus sits.
// Round 492: the header's Temp8.pInterpolate was declared s16-returning, which is why the
// entry could not match: with it the call result is truncated in place ('sll v0,v0,16 / sra
// s2,v0,16 / move s1,s2') where retail copies the raw result first ('move s2,v0 / sll v0,s2,16
// / sra s1,v0,16'). include/libgear/main.h now declares it s32 (the battle twin misc22 declares
// its own struct that way), and 89.35 -> 92.35 / 1032 bytes / 258 insns (retail 1028 / 257):
// the entry, the four tail exits ('sll v0,s2,0x10' in the branch delay slots) and 'sh s2,0x18'
// now match retail. WHAT IS LEFT, one instruction plus the two loops: (1) the equal-path exit
// 'return prevFrame;' (u16 local -> s16 return) emits 'sll v0,v0,16' in the first beq's delay
// slot, where retail leaves a nop and returns the raw lhu value - 's32 prevFrame' frees the
// COMPARE ('beq s1,v0' with no extension, retail's shape) but costs three at that exit (261
// insns / 88.89), 's16 prevFrame' is 261 / 89.75, '(u16)frame != prevFrame' 260 / 90.00, so the
// u16 local is the best of the four; the store also lands one slot early ('sh v1,0x18' after the
// first beq instead of in the mode-dispatch beq's delay slot). (2) the two copy loops: retail
// keeps pSource->pPixels in t3 and pTexture->pResultPixels in t2 (ours t2 / a1) and hoists
// 'sll a2,a3,0x1' into the loop-2 'bnez' delay slot, so ours recomputes the x index (+1) - the
// same allocation residual the battle twin records. Not permuted below 99%.
/*
s16 func_801E1258(Temp8* pTexture, s32 frameStep) {
    Temp8* pSource;
    RECT srcRect;
    RECT dstRect;
    s32 level;
    s16 frame;
    s16 pixelCount;
    u16 prevFrame;
    int x;
    int y;
    u32 dstAddr;
    u32 srcAddr;
    u16* pDstBase;
    u16* pSrcBase;
    u16* pSrcRow;

    if (pTexture->isLoaded == 0) {
        return -1;
    }

    pTexture->angle += pTexture->angleStep * (frameStep + 1);
    level = pTexture->pInterpolate(pTexture->angle, pTexture->divisor, pTexture->offset);
    frame = level;
    if (frame < 0) {
        LibGearFlushTexture(pTexture);
        return frame;
    }

    prevFrame = pTexture->prevFrame;
    if (frame != prevFrame) {
        pTexture->prevFrame = level;

        switch (pTexture->mode) {
            case 0:
                func_80026F44(pTexture->pixelCount, frame, pTexture->pResultPixels, pTexture->pPixels);
                if (pTexture->pSource == NULL) {
                    LoadImage(&pTexture->rect, (u_long*)pTexture->pResultPixels);
                }
                break;
            case 1:
                pixelCount = pTexture->pixelCount;
                func_80026FE8(pixelCount, frame, pTexture->pResultPixels, pTexture->pStartPixels, pTexture->pPixels);
                if (pTexture->pSource == NULL) {
                    LoadImage(&pTexture->rect, (u_long*)pTexture->pResultPixels);
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
        if (pSource == NULL) {
            return level;
        }
        if (pSource->isLoaded == 0) {
            return level;
        }

        // Clip the copy to the overlap of the two regions
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
            pDstBase = pSource->pPixels;

            if (pSource->mode < 4) {
                pSrcBase = (u16*)pTexture->pResultPixels;
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
                        dstAddr = dstRect.x * 2 + (u32)pDstBase;
                        dstAddr = x * 2 + dstAddr;
                        *(u16*)((dstRect.y + y) * pSource->rect.w * 2 + dstAddr) =
                            *(pSrcRow = (u16*)pTexture->pRows + 3 * (srcRect.y + y) + (srcRect.x + x));
                    }
                }
            }
        }

        return level;
    }

    return prevFrame;
}*/
INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E1258);




void LibGearFlushTexture(Temp8* pTexture) {
    if (pTexture->isLoaded != 0) {
        if (pTexture->pPixels != NULL) {
            if (pTexture->mode < 4) {
                LoadImage(&pTexture->rect, (u_long*)pTexture->pPixels);
            }
            HeapFree(pTexture->pPixels);
            pTexture->pPixels = NULL;
        }

        if (pTexture->pStartPixels != NULL) {
            HeapFree(pTexture->pStartPixels);
            pTexture->pStartPixels = NULL;
        }

        if (pTexture->pResultPixels != NULL) {
            HeapFree(pTexture->pResultPixels);
            pTexture->pResultPixels = NULL;
        }

        pTexture->isLoaded = 0;
    }
}

void LibGearFadeTexturePixels(Temp8* pTexture, s16 scale) {
    u16* pSrc = pTexture->pPixels;
    s16* pDst;
    int y;
    int x;

    for (y = 0; y < pTexture->rect.h; y++) {
        for (x = 0; x < pTexture->rect.w; x++) {
            pDst = pTexture->pRows + 3 * (pTexture->rect.y + y);
            pDst[pTexture->rect.x + x] = (*pSrc * scale) / 0x20;
            pSrc++;
        }
    }
}

void LibGearBlendTexturePixels(Temp8* pTexture, s16 scale) {
    u16* pSrc = pTexture->pPixels;
    u16* pDelta = pTexture->pStartPixels;
    s16* pDst;
    int y;
    int x;

    for (y = 0; y < pTexture->rect.h; y++) {
        for (x = 0; x < pTexture->rect.w; x++) {
            pDst = pTexture->pRows + 3 * (pTexture->rect.y + y);
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


// One textured trail segment func_801E1A14 builds (stride 0x58): the texture rows the
// segment covers, then the two Gouraud triangles that span them.
typedef struct {
    /* 0x0 */ s16 unk0;
    /* 0x2 */ s16 unk2;
    /* 0x4 */ s16 unk4;
    /* 0x6 */ s16 unk6;
    /* 0x8 */ POLY_GT3 polys[2];
} LibGearTrailSegment; // Size: 0x58

// objdiff 71.13% (2260 bytes against retail's 2276; 565 of 569 instructions; frame 0x130
// == retail's). This body is battle/main/misc23's func_800A7064 park ported 1:1
// (twin_for 1.000; per that park this function is the same body: texX/texY/texZ are
// 32-BIT parameters narrowed once with sll/sra in the vector loop, and the eight texture
// expressions written inline in the store statements), plus one fix found this round.
// THE FIX: nSegs is a 16-bit UNSIGNED local (u16), not s32. That is what produces
// retail's nSegs pick `sltu v0,v1,a3` (0x1F40) while rowStep stays a signed `div
// zero,v0,a3` (0x1F60): with s32 nSegs the pick compiles to `slt`, and the body loses
// four instructions (561 of 569). Measured chain this round, worst to best:
//   543 insns / 2172 bytes - the previous park here (objdiff 76.11%)
//   562 / 2248 - sibling body ported (objdiff 14.61%)
//   549 / 2196 - point loop read pEntry->pVectors[i]: gcc reloads the field per j
//   561 / 2244 - per-i SVECTOR* pVector = &pVectors[i] local in the point loop
//   565 / 2260 - this body (s32 nSegs -> u16), objdiff 71.13%
//   565 / 2260 - pVector = &pEntry->pVectors[i] per i, objdiff 70.91%
// objdiff's fuzzy percentage is NOT a guide here (the sibling park reads 13.46% at a
// better instruction count than the 76.11% one): judge by insn count, byte count, frame.
// REMAINING 4 instructions / 16 bytes, allocation only, each read off the .s:
// - retail keeps rowBase in s6 and texX/texY/texZ in s0/s2/s3 from the narrowing at
//   0x5B38 to the end of the segment loops; this compile homes rowBase at 0x18 and
//   re-reads texX/texY from their incoming 0x140/0x144 slots for the narrowing.
// - retail re-reads pEntry->pVectors (lw a3,0x14(t5), 0x5CCC) once per i in the point
//   loop and walks it with addiu a3,a3,8; both spellings tried give the same size.
// - retail keeps the pCount cursor in a call-clobbered t1 with sw t1,0xF8(sp) / lw
//   t1,0xF8(sp) around each HeapAlloc/HeapFree; this compile promotes it to s8.
// Everything else agrees instruction for instruction: the five HeapAllocs and their
// sizes, the four failure tails with their exact frees, the part-stream count re-reads,
// the four k-loop store orders (u0 u1 u2 per the two loops), both div expansions, the
// sltu pick, and the tail's eight section clears.
// Permuter not run: 71% is below the STOP RULE exception and the gap is spill and
// address-forming code, not a statement or operand shape.
/*
void func_801E1A14(Temp9* pEntry, u16* pCur, s32 rowBase, s32 scale, s32 texX, s32 texY, s32 texZ,
    s32 numSections, s16 tileX, s16 tileY, s16 uSpan, s16 vSpan, s16 clutX, s16 clutY, u8 red0, u8 green0,
    u8 blue0, u8 red1, u8 green1, u8 blue1) {
    SVECTOR* pVectors;
    Temp9Point** pPoints;
    Temp9Point* pPointData;
    Temp9Point* pPoint;
    LibGearTrailSegment* pSegments;
    LibGearTrailSegment* pSeg;
    POLY_GT3* pPoly;
    u16* pCount;
    u16* pData;
    u8* pIndex;
    s16 pageX;
    s16 pageY;
    u16 tpage;
    u16 clut;
    u16 rowStep;
    s16 uStep;
    u16 nSegs;
    s32 nRaw;
    s32 uStart;
    s32 uEnd;
    s32 uRow;
    s32 nRow;
    s32 i;
    s32 j;
    s32 k;
    Temp9Section* pSection;

    pEntry->numVectors = *pCur++;
    pEntry->numSegments = *pCur++ << 1;
    HeapChangeCurrentUser(HEAP_USER_MASA, HEAP_CONTENT_NULL);

    pVectors = HeapAlloc(pEntry->numVectors * sizeof(SVECTOR), 0x0);
    if (pVectors == NULL) {
        pEntry->pVectors = NULL;
        return;
    }
    pEntry->pVectors = pVectors;

    for (i = 0; i < pEntry->numVectors; i++) {
        pVectors[i].vx = (*pCur++ + (s16)texX) * scale / 4096;
        pVectors[i].vy = (*pCur++ + (s16)texY) * scale / 4096;
        pVectors[i].vz = (*pCur++ + (s16)texZ) * scale / 4096;
    }

    nRaw = pCur[pEntry->numVectors];
    pEntry->numPoints = nRaw + pEntry->numVectors;

    pPoints = HeapAlloc(pEntry->numVectors * sizeof(Temp9Point*), 0x0);
    if (pPoints == NULL) {
        pEntry->pVectors = NULL;
        HeapFree(NULL);
        return;
    }
    pEntry->pPoints = pPoints;

    pData = pCur + pEntry->numVectors + 1;
    pIndex = (u8*)(pData + nRaw);
    pPointData = HeapAlloc(pEntry->numPoints * sizeof(Temp9Point), 0x0);
    if (pPointData == NULL) {
        HeapFree(NULL);
        pEntry->pVectors = NULL;
        HeapFree(pEntry->pPoints);
        HeapFree(pPointData);
        return;
    }

    pPoint = pPointData;
    pCount = pCur;
    for (i = 0; i < pEntry->numVectors; i++) {
        pPoints[i] = pPoint;
        for (j = 0; j < *pCount; j++) {
            pPoint->unk0 = *pData++ * scale / 4096;
            pPoint->unk2 = *pIndex++ + rowBase;
            pPoint->x = pVectors[i].vx;
            pPoint->y = pVectors[i].vy;
            pPoint->z = pVectors[i].vz;
            pPoint++;
        }
        pPoint->unk0 = 0;
        pPoint->unk2 = 0;
        pPoint->x = pVectors[i].vx;
        pPoint->y = pVectors[i].vy;
        pPoint->z = pVectors[i].vz;
        pPoint++;
        pCount++;
    }

    pSegments = HeapAlloc(pEntry->numSegments * sizeof(LibGearTrailSegment), 0x0);
    if (pSegments == NULL) {
        HeapFree(NULL);
        pEntry->pVectors = NULL;
        HeapFree(pEntry->pPoints);
        HeapFree(pPointData);
        return;
    }
    pEntry->pSegments = pSegments;

    pageX = tileX / 64 * 64;
    pageY = tileY / 256 * 256;
    tpage = GetTPage(0, 1, pageX, pageY);
    clut = GetClut(clutX, clutY);
    texX = (tileX - pageX) * 4;
    texY = tileY - pageY;
    uStep = uSpan / (pEntry->numVectors - 1);
    uStart = 0;
    uEnd = uStep;
    uRow = 0;
    pCount = pCur;
    pSeg = pSegments;
    for (i = 0; i < pEntry->numVectors - 1; i++) {
        nSegs = pCount[1];
        if (pCount[0] < nSegs) {
            nSegs = pCount[0];
        }
        rowStep = vSpan / nSegs;
        nRow = uRow;
        for (j = 0; j < nSegs; j++) {
            pSeg->unk0 = nRow;
            pSeg->unk4 = nRow + 1;
            pSeg->unk2 = nRow + *pCount + 1;
            for (k = 0; k < 2; k++) {
                pPoly = &pSeg->polys[k];
                SetPolyGT3(pPoly);
                pPoly->tpage = tpage;
                pPoly->u0 = texX + uStart;
                pPoly->v0 = texY + rowStep * j;
                pPoly->clut = clut;
                pPoly->v1 = texY + rowStep * j;
                pPoly->u2 = texX + uStart;
                pPoly->u1 = texX + uEnd;
                pPoly->v2 = texY + rowStep * (j + 1);
            }
            pSeg++;
            pSeg->unk0 = nRow + *pCount + 1;
            pSeg->unk4 = nRow + 1;
            pSeg->unk2 = nRow + *pCount + 2;
            for (k = 0; k < 2; k++) {
                pPoly = &pSeg->polys[k];
                SetPolyGT3(pPoly);
                pPoly->tpage = tpage;
                pPoly->u0 = texX + uEnd;
                pPoly->u1 = texX + uEnd;
                pPoly->clut = clut;
                pPoly->u2 = texX + uStart;
                pPoly->v0 = texY + rowStep * j;
                pPoly->v1 = texY + rowStep * (j + 1);
                pPoly->v2 = texY + rowStep * (j + 1);
            }
            pSeg++;
            nRow++;
        }
        uStart += uStep;
        uEnd += uStep;
        uRow += pCount[0] + 1;
        pCount++;
    }

    pEntry->red0 = red0;
    pEntry->green0 = green0;
    pEntry->blue0 = blue0;
    pEntry->red1 = red1;
    pEntry->green1 = green1;
    pEntry->blue1 = blue1;
    pEntry->numSections = numSections;
    if (pEntry->numSections > 0) {
        pSection = HeapAlloc(pEntry->numSections * sizeof(Temp9Section), 0x0);

        if (pSection == NULL) {
            pEntry->numSections = 0;
        }
        if (pEntry->numSections > 0) {
            pEntry->pSections = pSection;
            for (i = 0; i < pEntry->numSections; i++) {
                pSection[i].vector.vx = 0;
                pSection[i].vector.vy = 0;
                pSection[i].vector.vz = 0;
                pSection[i].vector.pad = 0;
                pSection[i].x = 0;
                pSection[i].y = 0;
                pSection[i].z = 0;
                pSection[i].unkE = 0;
            }
        }
    } else {
        pEntry->pSections = NULL;
    }
}*/
INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E1A14);






INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E22F8);

void LibGearFreeModelEntry(Temp9* pEntry) {
    if (pEntry->pVectors != NULL) {
        HeapFree(pEntry->pVectors);
        HeapFree(pEntry->pPoints[0]);
        HeapFree(pEntry->pPoints);
        HeapFree(pEntry->pSegments);

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

// objdiff 94.87% (best), 39 insns / 156 bytes - retail length, instruction multiset and
// store order all exact. The ONE delta is the -1 constant's slot: retail has
// li v1,-1 at insn 0 and its first user sh v1,0x98 at insn 14. Store order
// equals source order in every measured shape, so with frame = negOne first the
// li is at 0 and the store sinks to 8 (this body, 94.87); with the store in
// retail's place (10th statement) the store is at 14 but the li is hoisted only
// to 6 (74.36). sched2 couples the constant load with the store's sink, so
// (li@0, store@14) is out of reach. Screened: 2.6.0 and 2.7.2-cdk identical -
// not a compiler question. Permuter: 783 iterations, NONE (round 62).
// Rounds 370/451: negOne = -1 as a statement, one = 1 moved to the 0x39 store
// position, three bare -1 literals, volatile barriers on objectType /
// parentModelSlot, 6th-statement position (5, 9) - all no better.
// Round 466: un-parked and re-measured. Retail's store order with the
// DECLARATION INITIALISER form re-measures at 74.36 (li@6): gcc folds the local
// into its first use, so the load is never emitted at the declaration.
// register s16 on both locals: byte-identical output. 94.87 stands as the best.
// objdiff 94.87%, confirmed round 479 re-measure: 156 bytes / 39 insns = retail's exact size with ONE displaced instruction (sh v1,152 at slot 8 vs retail 14); li v1,-1 is at slot 0 as retail. Levers h-no-way-to-home-a and h-constant-home-hoist-is-capped-at-nine-insns cover it: the -1 home's hoist is capped at ~9 insns, retail's own store slot gives li@5/74.36, the front slot sinks the store to 8/94.87, a duplicate early store is wrong code and still 97.44. Not a preset (2.6.0/2.7.2/cdk swept), permuter 783 iters NONE (round 62).
// Round 492 re-measured 94.87 from the parked body and added two negatives: swapping the
// negOne/one declaration order is byte-identical (94.87), and writing 'pModel->unk1E = negOne;'
// FIRST (so the -1 home is created ahead of the 0x3C/0x5C/0x39 triplet) gives li@0 but puts the
// 0x1E store at slot 8 as well - two displaced stores, and sched2 does not sink that store into
// the jr delay slot. Retail's (li@0, sh 0x98@14) stays out of reach.
/*
void func_801E3534(Model* pModel, s32 arg1, u32 arg2, u32 arg3) {
    s16 negOne = -1;
    s16 one = 1;

    pModel->frame = negOne;
    pModel->objectType = 0xFFFF;
    pModel->parentModelSlot = 0xFF;
    pModel->depthShade = 0x6B;
    pModel->pAnimTable = (u32*)arg2;
    pModel->unkC = 0;
    pModel->animValue = 0;
    pModel->pEntryTableLow = (Temp7**)arg3;
    pModel->pEntryTableHigh = NULL;
    pModel->commandCount = 0;
    pModel->sourceModelSlot = 0;
    pModel->unk35 = 0;
    pModel->useScaledMatrices = 0;
    pModel->unk38 = 0;
    pModel->animationBitMask = negOne;
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
}*/
INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E3534);



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
                pModel->animValue = pSource->pAnimTable[arg3];
            } else {
                pModel->animValue = ((u32*)pSource->unkC->pPixels)[arg3 - LIBGEAR_ANIM_WAD_INDEX_OFFSET];
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

    if (numFrames < LIBGEAR_INTERPOLATE_MIN_FRAMES) {
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
        pObject->loops = 0;
        pObject->opcode = LIBGEAR_JOINT_OPCODE_INTERPOLATE;
        pObject->type = 0xFE;

        pObject->startX = pJoint->rotation.vx;
        pObject->startY = pJoint->rotation.vy;
        pObject->startZ = pJoint->rotation.vz;

        x = (x - pJoint->rotation.vx) & LIBGEAR_ANGLE_MASK;
        if (x >= LIBGEAR_ANGLE_HALF_TURN) {
            x -= LIBGEAR_ANGLE_FULL_TURN;
        }
        pObject->targetX = x;

        y = (y - pJoint->rotation.vy) & LIBGEAR_ANGLE_MASK;
        if (y >= LIBGEAR_ANGLE_HALF_TURN) {
            y -= LIBGEAR_ANGLE_FULL_TURN;
        }
        pObject->targetY = y;

        z = (z - pJoint->rotation.vz) & LIBGEAR_ANGLE_MASK;
        if (z >= LIBGEAR_ANGLE_HALF_TURN) {
            z -= LIBGEAR_ANGLE_FULL_TURN;
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
        pObject->loops = 0;
        pObject->opcode = arg2 + LIBGEAR_JOINT_OPCODE_MOVE_BASE;
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
        pModel->frame = LIBGEAR_SCRIPT_FRAME_STOPPED;
    }
}

// Returns a sound-bank base id (record field 0x14) selected by `type`, or 2 for
// an unknown type. Best 71.67% (88 bytes against retail's 108, 22 insns of 27).
// RESIDUAL, named: retail keeps THREE copies of the `lhu 0x14 / sll 16` tail, in
// the block order [test0][arm 0][test1 beq arm 1][test2 bne exit][arm 2][shared
// tail: lhu/sll, reached by arm 0's `j` AND arm 2's fall-through][arm 1, its own
// full lw/lw/lhu/sll copy][jr ra], and its `bnez $a1` delay slot is a `nop`.
// In every spelling here gcc 2.6.0 cross-jumps arm 0 and arm 2 INTO arm 1's copy
// instead - its merge target is the function's LAST matching block - so the body
// is 5 insns (20 bytes) short and the `bnez` slot takes the next test's
// `li v0,1` in place of the nop. Measured, all 88 bytes unless noted: three
// separate `return`s in an 0,1,2 `if` chain 61.93; one `pTexture` local and one
// return in 0,2,1 order 64.59 (also the wrong test order); `if (type != 0) {
// nested } else {...}` 0.00 at 96; the same nested 0,1,2 form 54.41 at 96; the
// body below 71.67 - the only shape that reproduces retail's test ORDER and
// POLARITY (beq for type==1, bne for type==2, arm 1 emitted last), and what the
// residual above is measured on; duplicating the tail into each arm as well
// measures the same 71.67. A `switch` on type is 19.74 (case bodies emitted in
// source order with break jumps). Screened identical (71.67) under
// gcc-2.7.2-cdk, so the tail split is not a compiler choice. A probe sweep tried
// to make arm 1's tail rtx-different before jump.c runs - `& 0xFFFF` on the
// field, `* 0x10000`, `(x << 8) << 8`, a `u32` id local, a `u16*` alias, and a
// `goto` label placed inside arm 2's then-body that arm 0 jumps to - and every
// one compiles to the same 20 insns with the survivor at arm 1's copy, so cse
// and combine fold them all ahead of the merge.
/*
s32 func_801E5CD8(Model* pModel, s32 type) {
    Temp8* pTexture;

    if (type == 0) {
        pTexture = g_BattleSoundBankMain;
    } else if (type != 1) {
        if (type == 2) {
            pTexture = ((Temp8*)pModel->pSoundBank2)->pStartPixels;
        } else {
            return 2;
        }
    } else {
        pTexture = ((Temp8*)pModel->pSoundBank1)->pStartPixels;
    }
    return pTexture->angle << 16;
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
    /* 0x7  */ u8 unk7; // id 2: scale factor, id 9: interpolation type
    union {
        // id 2: the two byte scale factors that follow unk7
        struct {
            /* 0x8 */ u8 unk8;
            /* 0x9 */ u8 unk9;
        } joint;
        // id 9: x component of the texture blend, over the last two scale factors
        /* 0x8 */ u16 x;
    } payload;
    /* 0xA  */ u16 unkA;
    /* 0xC  */ u16 unkC;
    /* 0xE  */ u16 unkE;
    /* 0x10 */ u16 unk10;
    /* 0x12 */ u8 unk12;
    /* 0x13 */ u8 unk13;
    /* 0x14 */ u8 unk14;
    /* 0x15 */ u8 unk15;
    /* 0x16 */ s16 unk16;
    /* 0x18 */ s16 unk18;
    /* 0x1A */ s16 unk1A;
} ModelScriptCommand; // Size: 0x1C


Temp8* func_801E0A00(Temp8* pTexture, Temp8* pSource, u16 mode, u32 flags, s16* pRows, u16 x0, u16 y0,
                     u16 colour0, u16 x1, u16 y1, u16 colour1, u16 x, u16 y, u32 argWidth, u32 argHeight,
                     u16 angleStep, u16 divisor, u16 offset, s16 (*pInterpolate)(u_short, s16, s16));
// objdiff 97.61%, 378 instructions and 1512 bytes = retail exactly, frame 0x88 =
// retail. The body's shape is settled and nothing below changes an instruction count;
// three named residuals are left:
// (a) case 2 reads every field of the arm through `a1`, which retail reaches with
//     `move a1,s0` in the delay slot of the `unk5 != 0` test: a second pseudo this
//     compiler keeps and cannot be talked into. Writing `pCommand = pCmd` at the top of
//     the arm is coalesced back to s0, and reloading `pModel->pScriptCursor` would emit
//     a `lw` where retail reuses s0. 16 lines of the diff sit here.
// (b) the 2nd and 3rd of the three D_801E8644 stores: retail has `addu v1,a0,v1` and
//     `addu a0,a0,v0` (base first) where this build has both index-first, and the 1st
//     store matches in both, so retail's own two spellings of one expression differ.
// (c) the case-8 header. Retail reads the 0x50(sp) flag FIRST, sign-extends it right
//     after the mask load and puts `li v0,1` last; this build hoists `li v1,1` to the
//     top and sinks the flag load past both global loads. And retail's `lbu a3,5(s6)`
//     (count) sits between `lw g_LibGearModels[i]` and its `beqz`, where this build
//     loads it at the top of the loop body.
// Measured this round, every variant re-parked here at 97.61% / 1512 bytes / 378 insns:
// - preset leg closed: gcc-2.7.2-cdk -O2 -G8 84.88, gcc-2.7.2 -O2 -G8 95.49, -O3
//   compile-failed. 2.6.0 -O2 -G8 is the best of the four, so (c) is not a compiler
//   version.
// - the flag slot must stay `volatile`: as a plain `u16 unkFlag[4]` gcc deletes the load
//   and its sign-extend (376 insns, 1504 bytes, 97.33). With volatile its POSITION is
//   not source-controlled - moving `flag = unkFlag[0]` from the arm's first statement
//   to its last measures byte-identical - so retail's flag-first header is out of reach.
// - `count = pCommand->unk5` moved out of the loop preheader into the then-block as
//   nested ifs: 380 insns, 1520 bytes, 96.87.
// - `modelBit = 1 << savedIndex` instead of currentModelIndex: 377 insns, 1508 bytes,
//   97.10, one instruction short.
// - case-8 declaration order with `flag` declared first: byte-identical.
// - `volatile u16 unkFlag[4]` (not 8) is the right size: retail reads that slot at
//   0x50(sp) before writing it and the frame is 0x88 with this one.
// - (b) is closed: `((s16 *)((u8 *)D_801E8644 + pCmd->index * 2))[4]` and `[7]` (the
//   levers' cast/paren-sum spelling, and the closest thing to the double negation when
//   the base is a pointer MEM loaded once, not a pointer variable) measure byte-identical
//   to the plain subscript at 97.61 / 1512 / 378. Both remaining orders are the
//   legitimiser's choice between three uses of one expression.
// - (a) with `pCommand = pCmd;` and the merged reads through pCommand: as the first
//   statement of the `index < 2` block the copy DOES survive, but it costs 4 instructions
//   (382 insns, 1528 bytes, 95.71) and re-allocates case 9 (its pointer leaves a0 for s2);
//   at the top of case 2's `unk4 != 0` block it is 96.74 / 1512 / 378 with the whole body
//   mirrored ($s0/$s1 swapped at every site), because the extra pseudo takes a
//   callee-saved register. So the copy cannot be added without re-allocating the function;
//   (a) is not reachable by sourcing.
/*
void func_801E5D44(Model* pModel, s32 arg1, s32 arg2) {
    ModelScriptCommand* pCmd;
    ModelScriptCommand* pCommand;
    Temp8* pSource;
    s16* pColorMatrix;
    s16 (*pInterpolate)(s16, s16, s16);
    s16 x;
    s16 y;
    s16 z;
    s16 w;
    s16 colour1;
    volatile u16 unkFlag[4]; // retail reads this slot inside case 8 before ever writing it

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
                        if (pCmd->unk5 != 0) {
                            g_LibGearModelOffsets[pCmd->index].modelIndex = -1;
                        } else {
                            g_LibGearModelOffsets[pCmd->index].modelIndex = pModel->modelSlot;
                        }
                        g_LibGearModelOffsets[pCmd->index].jointIndex = pCmd->flags;
                        D_801E8644[pCmd->index + 1] = pCmd->unk7 * 16;
                        D_801E8644[pCmd->index + 4] = pCmd->payload.joint.unk8 * 16;
                        D_801E8644[pCmd->index + 7] = pCmd->payload.joint.unk9 * 16;
                        g_LibGearModelOffsets[pCmd->index].x1 = pCmd->unkA;
                        g_LibGearModelOffsets[pCmd->index].y1 = pCmd->unkC;
                        g_LibGearModelOffsets[pCmd->index].z1 = pCmd->unkE;
                        g_LibGearModelOffsets[pCmd->index].inUse = pCmd->unk10;
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
                s32 i;
                s32 count;
                s16 currentModelIndex;
                u16 modelBitMask;
                u16 savedIndex;
                s16 flag;
                u16 modelBit;

                flag = unkFlag[0];
                currentModelIndex = g_LibGearCurrentModelIndex;
                modelBitMask = g_LibGearModelBitMask;
                savedIndex = currentModelIndex;
                modelBit = 1 << currentModelIndex;
                pCommand = pCmd;
                for (i = 0; i < 8; i++) {
                    count = pCommand->unk5;
                    if (((pModel->modelBitMask >> i) & 1) && g_LibGearModels[i] != NULL && count > 0) {
                        if (flag != 0) {
                            LibGearSetModelAnimationFrom(pModel, (u16)i, (u16)modelBit, count);
                        } else {
                            LibGearSetModelAnimation((u16)i, (u16)modelBit, count);
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
                        if (pCmd->unk5 != 0xFF && pCmd->unk5 < pModel->numTextures) {
                            pSource = &pModel->pTextures[pCmd->unk5];
                        } else {
                            pSource = NULL;
                        }
                        pColorMatrix = NULL;
                        if ((pCmd->flags & 0x7F) >= 4) {
                            pColorMatrix = D_801E8644;
                        }
                        pInterpolate = LibGearGetInterpolationFunc(pCmd->unk7);
                        x = pCmd->payload.x;
                        y = pCmd->unkA;
                        z = pCmd->unkC;
                        w = pCmd->unkE;
                        colour1 = pCmd->unk10;
                        if (pCmd->flags & 0x80) {
                            if (pModel->textureOffsetX >= 0) {
                                x += pModel->blendOffsetX;
                                y += pModel->blendOffsetY;
                                if ((pCmd->unk12 >> 4) == 1) {
                                    z += pModel->blendOffsetX;
                                    w += pModel->blendOffsetY;
                                }
                            }
                        }
                        func_801E0A00(&pModel->pTextures[pCmd->index], pSource, pCmd->flags & 0x7F,
                                      pCmd->unk12 | 0x700, pColorMatrix, x, y, 0, z, w, colour1, x, y,
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
}*/
INCLUDE_ASM("asm/lib_gear/nonmatchings/main/main", func_801E5D44);

void LibGearStopAnimationScript(Model* pModel) {
    pModel->frame = LIBGEAR_SCRIPT_FRAME_STOPPED;
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
// Round 370: re-measured live at 99.87%/464 bytes (retail size). The three residual
// insns are the documented reg_eqv tie: retail bne v1,v0 x2 + addiu v0,a1,-1, ours
// bne a1,v0 x2 + addiu v0,v1,-1. Copy slot, size and multiset are identical.
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


void LibGearFreeJointAndChildren(Temp4* pList, s32 jointIndex, ModelJoint* pSkeleton, ModelJoint* pTargetSkeleton) {
    ModelJoint* pJoint;
    ModelJoint* pTarget;
    ModelJoint* pChild;
    s32 numChildren;
    s32 i;

    pJoint = (ModelJoint*)(jointIndex * sizeof(ModelJoint) + (s32)pSkeleton);
    pTarget = (ModelJoint*)(jointIndex * sizeof(ModelJoint) + (s32)pTargetSkeleton);
    numChildren = pSkeleton->numChildren;
    pJoint->doRender = 0;
    pTarget->doRender = 1;
    LibGearFreeJointObject(pList, pJoint->pRotationObject);
    do {
        pJoint->pRotationObject = NULL;
        LibGearFreeJointObject(pList, pJoint->pTranslationObject);
        pJoint->pTranslationObject = NULL;
    } while (0);
    LibGearFreeJointObject(pList, pJoint->pScaleObject);
    pJoint->pScaleObject = NULL;
    pChild = pSkeleton;
    for (i = 1; i < numChildren; i++) {
        pChild++;

        if (pChild->pParent == pJoint) {
            LibGearFreeJointAndChildren(pList, pChild->numChildren, pSkeleton, pTargetSkeleton);
        }
    }
}

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

int LibGearResolveModelSlot(Model* pModel, int type, s16* pMask) {
    if ((type & 0xFF) == 0xFF) {
        type = LibGearGetFirstModelIndex();
    } else if ((type & 0xFF) == 0xFE) {
        type = (u8)g_LibGearCurrentModelIndex;
    } else if ((type & 0xFF) == 0xFD || (type & 0xFF) == 0xF9) {
        type = pModel->modelSlot;
    } else if ((type & 0xFF) == 0xFC) {
        type = pModel->linkedModelSlot;
    } else if ((type & 0xFF) == 0xFA) {
        type = 0xA;
    } else if ((type & 0xFF) == 0xF8) {
        type = pModel->modelSlot * 2 + 8;
    } else if ((type & 0xFF) == 0xF7) {
        type = pModel->modelSlot * 2 + 9;
    }

    *pMask = 1 << (type & 0xFF);
    return type & 0xFF;
}


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

void LibGearQueueJointTransformCommand(Model* pModel, Temp4* pList, ModelJoint* pJoint, u8 flags,
                   u8 arg4, u8 arg5, u8 arg6, s16 posX, s16 posY, s16 posZ,
                   s16 offX, s16 offY, s16 offZ, s16 arg13) {
    u8 channel;
    Temp3* pObject;
    ModelJoint* pChild;
    u16 baseX;
    u16 baseY;
    u16 baseZ;
    u16 offBaseX;
    u16 offBaseY;
    u16 offBaseZ;
    s32 i;

    channel = flags & LIBGEAR_JOINT_CHANNEL_MASK;
    if (channel == LIBGEAR_JOINT_CHANNEL_ROTATION) {
        pObject = pJoint->pRotationObject;
    } else if (channel == LIBGEAR_JOINT_CHANNEL_TRANSLATION) {
        pObject = pJoint->pTranslationObject;
    } else {
        pObject = pJoint->pScaleObject;
    }
    if (pObject == NULL) {
        pObject = LibGearAllocJointObject(pList);
    }
    if (pObject != NULL) {
        pObject->inUse = 1;
        pObject->loops = arg6;
        pObject->opcode = arg4 + LIBGEAR_JOINT_OPCODE_INTERPOLATE;
        pObject->type = arg5;
        if (flags & LIBGEAR_JOINT_FLAG_ADD) {
            if (channel == LIBGEAR_JOINT_CHANNEL_ROTATION) {
                baseX = pJoint->rotation.vx;
                baseY = pJoint->rotation.vy;
                baseZ = pJoint->rotation.vz;
            } else if (channel == LIBGEAR_JOINT_CHANNEL_TRANSLATION) {
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
            if (channel == LIBGEAR_JOINT_CHANNEL_ROTATION) {
                offBaseX = pJoint->rotation.vx;
                offBaseY = pJoint->rotation.vy;
                offBaseZ = pJoint->rotation.vz;
            } else if (channel == LIBGEAR_JOINT_CHANNEL_TRANSLATION) {
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
        if (channel == LIBGEAR_JOINT_CHANNEL_ROTATION) {
            if (arg4 < 2) {
                pJoint->rotation.vx = pObject->startX;
                pJoint->rotation.vy = pObject->startY;
                pJoint->rotation.vz = pObject->startZ;
            }
            pJoint->pRotationObject = pObject;
        } else if (channel == LIBGEAR_JOINT_CHANNEL_TRANSLATION) {
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
    }
    if (flags & LIBGEAR_JOINT_FLAG_RECURSE) {
        pChild = pModel->pSkeleton;
        for (i = 1; i < pModel->pSkeleton->numChildren; i++) {
            pChild++;
            if (pChild->pParent == pJoint) {
                LibGearQueueJointTransformCommand(pModel, pList, pChild, flags & 0xFF, arg4, arg5, arg6,
                              posX, posY, posZ, offX, offY, offZ, arg13);
            }
        }
    }
}

void LibGearSetJointFlag(Model* pModel, ModelJoint* pJoint, s32 flags) {
    ModelJoint* pChild;
    int i;

    pJoint->doRender = flags & LIBGEAR_JOINT_FLAG_RENDER;

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
void func_801E1A14(Temp9* pEntry, u16* pCur, s32 rowBase, s32 scale, s32 texX, s32 texY, s32 texZ, s32 numSections, s16 tileX, s16 tileY, s16 uSpan, s16 vSpan, s16 clutX, s16 clutY, u8 red0, u8 green0, u8 blue0, u8 red1, u8 green1, u8 blue1);
void ResolveArchiveEntryPointers(void* pTable);
s32 SoundFindSedsEntry(void* pEntry, s16 bank);
void SoundAddSedsEntry(void* pEntry);

// The heap copy of the model file being created, and the mesh slot it takes
extern ModelFileHeader* g_LibGearModelFile;
extern s32 g_LibGearMeshSlot;

// Builds a gear model from its archive entry into g_LibGearModels[modelIndex].
// offsetX..offsetW shift the material records' texture coordinates, and
// pPosition, when given, places the root joint.
// Parked at 90.47% (objdiff: 2264 bytes against retail 2280, 566 insns
// against 570). Three measured wins this round, all kept: (1) the material loop
// must walk RUNNING POINTERS (pEntry++ / pWad++ / a pSections base advanced by
// 0x24), not index pModel->pModelListEntries[i] - the indexed form reloads the
// array base five times per point and was 83.39 -> 86.21; (2) if/else arms are
// written NEGATED (if (!(flags & 0x40)) { if (pModel->flags & 4) B else C } else
// A) so gcc branches on the true condition and leaves the then-block out of line,
// sharing the two zero-filled LibGearInitializeSkeleton argument tails - that
// closed the whole skeleton region and went 86.21 -> 90.47; (3) same negation
// already applied to the texture-offset if at 0x90/0x92/0x94/0x96.
// RESIDUAL, one named cause: allocator tie. Mine keeps offsetW in $s7 and spills
// only offsetX/Y/Z to 0x68/0x70/0x78, where retail reloads all four from
// 0x68/0x70/0x78/0x80; that also costs the pWadTable slot at 0x88 and the 8-frame
// bytes (200 against 208), and shifts the whole callee-saved set by one
// (mine $s2=pModel, retail $s3=pModel). Tried: flipping the pModelSource
// if/else, declaring the four offsets as locals, indexed vs pointer loop forms.
/*
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
    Temp9* pEntry;
    u32* pWad;
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
        if (pSoundBank->pResultPixels != pSoundBank->pStartPixels) {
            if (SoundFindSedsEntry(pSoundBank->pStartPixels, 0) == 0) {
                SoundAddSedsEntry(pSoundBank->pStartPixels);
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

    if (!(flags & 0x40)) {
        if (pModel->flags & 0x4) {
            pModel->pSkeleton = LibGearInitializeSkeleton(pModel->pMesh, pJointEntries, 2, 0, 0, 0, 0, 0);
        } else {
            pModel->pSkeleton = LibGearInitializeSkeleton(pModel->pMesh, pJointEntries, 2, 1, (s16)offsetX, (s16)offsetY, (s16)offsetZ, (s16)offsetW);
        }
    } else {
        pModel->pSkeleton = LibGearInitializeSkeleton(pModel->pMesh, pJointEntries, 0, 0, 0, 0, 0, 0);
    }

    if (pPosition != NULL) {
        pModel->pSkeleton->translation.x = pPosition->vx;
        pModel->pSkeleton->translation.y = pPosition->vy;
        pModel->pSkeleton->translation.z = pPosition->vz;
    }

    if (!(flags & 1)) {
        pModel->textureOffsetX = offsetX;
        pModel->textureOffsetY = offsetY;
        pModel->blendOffsetX = offsetZ;
        pModel->blendOffsetY = offsetW;
    } else {
        pModel->textureOffsetX = -1;
    }

    if (!(flags & 4) && !(flags & 0x80)) {
        pModel->pModelSource = pSource;
    } else {
        pModel->pModelSource = NULL;
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
            pModel->pTextures[i].pPixels = NULL;
            pModel->pTextures[i].pStartPixels = NULL;
            pModel->pTextures[i].pResultPixels = NULL;
        }
    }

    pModel->numModelListEntries = pResource->numModelListEntries;
    pCur = (u16*)(pResource + 1);

    if (pModel->numModelListEntries != 0) {
        pModel->pModelListEntries = HeapAlloc(pModel->numModelListEntries * sizeof(Temp9), 0x0);

        pEntry = pModel->pModelListEntries;
        pWad = pWadTable + 2;
        for (i = 0; i < pModel->numModelListEntries; i++) {
            count = (s16)pCur[0x11];
            pEntry->jointIndex = *pCur++;

            func_801E1A14(
                pEntry,
                (void*)*pWad,
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
                pEntry->pSections[j].y = *pCur++;
                pEntry->pSections[j].unkE = *pCur++;
                pEntry->pSections[j].vector.vx = *pCur++;
                pEntry->pSections[j].vector.vy = *pCur++;
                pEntry->pSections[j].vector.vz = *pCur++;
            }
            pWad++;
            pEntry++;
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
}*/
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
        SoundRemoveSedsEntry(((Temp8*)g_LibGearModels[modelIndex]->pSoundBank1)->pStartPixels);
    }

    if (g_LibGearModels[modelIndex]->pModelSource != NULL) {
        HeapFree(g_LibGearModels[modelIndex]->pModelSource);
    }

    if ((u32)(modelIndex - MAX_MODEL_MESHES) >= MAX_MODELS - MAX_MODEL_MESHES) {
        if (g_LibGearModels[modelIndex]->pSkeleton != NULL) {
            Temp4* pPool = &g_LibGearJointObjectPool;

            LibGearReleaseSkeletonObjects(pPool, g_LibGearModels[modelIndex]->pSkeleton);
            LibGearReleaseSkeletonObjectsByType(pPool, g_LibGearModels[modelIndex]->pSkeleton, LIBGEAR_JOINT_OBJECT_TYPE_NONE);
            LibGearFreeModelSkeleton(g_LibGearModels[modelIndex]->pSkeleton);
            g_LibGearModels[modelIndex]->pMesh = NULL;
            g_LibGearModels[modelIndex]->pSkeleton = NULL;
        }
    } else {
        if (g_LibGearModels[modelIndex]->pSkeleton != NULL) {
            Temp4* pPool = &g_LibGearJointObjectPool;

            LibGearReleaseSkeletonObjects(pPool, g_LibGearModels[modelIndex]->pSkeleton);
            LibGearReleaseSkeletonObjectsByType(pPool, g_LibGearModels[modelIndex]->pSkeleton, LIBGEAR_JOINT_OBJECT_TYPE_NONE);
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
            pObjects[i].jointIndex = LIBGEAR_COLOR_ENTRY_JOINT_NONE;
            pObjects[i].pVertex = 0;
        }
        pModel->pColorEntries = pObjects;
    }
}
