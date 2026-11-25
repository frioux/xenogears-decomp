#ifndef __LIBGEAR_MAIN_H__
#define __LIBGEAR_MAIN_H__

#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "system/math.h"
#include "system/model.h"

// Sentinel values for the fields below that carry a state rather than a quantity
#define LIBGEAR_VERTEX_AGE_FREE (-1)        // Vertex::age: the slot is free
#define LIBGEAR_COLOR_ENTRY_JOINT_NONE (-1) // UnkInner1::jointIndex: the entry is free
#define LIBGEAR_SCRIPT_FRAME_STOPPED (-1)   // Model::frame: nothing to run
// Temp3::opcode, the value LibGearSetJointRotation / LibGearMoveJointToPoint queue
#define LIBGEAR_JOINT_OPCODE_INTERPOLATE 3
#define LIBGEAR_JOINT_OPCODE_MOVE_BASE 7
// ModelJoint::doRender / LibGearSetJointFlag's other bits
#define LIBGEAR_JOINT_FLAG_RENDER 0x1
#define LIBGEAR_JOINT_FLAG_ADD 0x20    // add to the joint's value instead of setting it
#define LIBGEAR_JOINT_FLAG_RECURSE 0x80 // apply to the joint's children too

// Queued joint object (LibGearAllocJointObject); defined with the pool below
struct _Temp3;
typedef struct _Temp3 Temp3;

struct _ModelJoint;
typedef struct _ModelJoint ModelJoint;
struct _ModelJoint {
    /* 0x00 */ ModelJoint* pParent;
    /* 0x04 */ u_char doTransform;
    /* 0x05 */ u_char doRotate;
    /* 0x06 */ u_char rotDirection;
    /* 0x07 */ u_char doRender; // non-zero: the joint's packets are drawn (LibGearSetJointFlag's bit 0)
    /* 0x08 */ u_short jointIndex;
    /* 0x0A */ u_short numChildren;
    /* 0x0c */ MATRIX localTransform;

    // Global transform matrix, where we've taken into account the
    // local transform matrices of the parent joints
    /* 0x2c */ MATRIX globalTransform;

    // Local transform matrix is computed based on these values
    /* 0x4C */ SVECTOR scale;
    /* 0x54 */ SVECTOR rotation;
    /* 0x5C */ LVEC3 translation;
    
    /* 0x68 */ void* pModelPacketBuffer;
    /* 0x6c */ void* pCurModelPacket;

    // Queued joint objects (LibGearAllocJointObject) that interpolate this
    // joint, one per transform channel; NULL = none queued
    /* 0x70 */ Temp3* pRotationObject;
    /* 0x74 */ Temp3* pTranslationObject;
    /* 0x78 */ Temp3* pScaleObject;
}; // Size: 0x7C

// Joint entry
typedef struct {
    /* 0x0 */ u_short jointIndex;
    /* 0x2 */ u_short parentIndex;
} ModelJointEntry; // Size: 0x4

typedef struct {
    // NOTE: These first four blocks are sometimes used as SVECTORs, but
    // they can't be defined as such since the padding field is actually used
    
    /* 0x0  */ u_short x0;
    /* 0x2  */ u_short y0;
    /* 0x4  */ u_short z0;
    /* 0x6  */ u16 unk6;
    
    /* 0x8  */ u_short x1;
    /* 0xA  */ u_short y1;
    /* 0xC  */ u_short z1;
    /* 0xE  */ short is3D; // 0: x0..x3/y0..y3 are screen coordinates, otherwise 3D points to project
    
    /* 0x10 */ u_short x2;
    /* 0x12 */ u_short y2;
    /* 0x14 */ u_short z2;
    /* 0x16 */ short age; // frames survived, -1 = free entry
    
    /* 0x18 */ u_short x3;
    /* 0x1A */ u_short y3;
    /* 0x1C */ u_short z3;
    /* 0x1E */ short lifetime; // freed once age reaches it
    
    /* 0x20 */ u_short red;
    /* 0x22 */ u_short green;
    /* 0x24 */ u_short blue;
    /* 0x26 */ short redDelta;
    /* 0x28 */ short greenDelta;
    /* 0x2A */ short blueDelta;
    /* 0x2C */ POLY_FT4 polys[2];
} Vertex;

typedef struct {
    /* 0x0 */ Vertex* pVertices;
    /* 0x4 */ short capacity;
    /* 0x6 */ short curIndex;
} VertexBuffer;

// Temp3::type of a joint object with no queued command: LibGearReleaseSkeletonObjects
// releases a component only when its type is not this, and LibGearFreeModel's by-type
// sweep passes it.
#define LIBGEAR_JOINT_OBJECT_TYPE_NONE 0xFF

/* Unknown list */
struct _Temp3 {
    /* 0x0  */ u8 inUse;
    /* 0x1  */ u8 loops; // non-zero: the object restarts when its interpolation ends instead of being released
    /* 0x2  */ u8 opcode; // command code plus 3 (plus 7 for a move); the joint update dispatches on it & 0xF
    /* 0x3  */ u8 type;
    /* 0x4  */ s16 startX; // the component's value when the interpolation was queued
    /* 0x6  */ s16 startY;
    /* 0x8  */ s16 startZ;
    /* 0xA  */ s16 targetX; // where the interpolation ends, or the per-step delta
    /* 0xC  */ s16 targetY;
    /* 0xE  */ s16 targetZ;
    /* 0x10 */ s16 curFrame; // counted up to endFrame by the joint update
    /* 0x12 */ s16 endFrame; // frame the interpolation runs to
}; // Size: 0x14

typedef struct {
    /* 0x0 */ Temp3* pObjects;
    /* 0x4 */ u16 freeIndex; // lowest index that may be free; the alloc scan starts here
    /* 0x6 */ u16 numObjects;
} Temp4;
/* -------------- */

typedef struct {
    /* 0x0 */ ModelPart** pParts;
    /* 0x4 */ u_int numParts;
} ModelMesh; // Size: 0x8



// Per-model transformed position; x1/y1/z1 is rotated into x0/y0/z0
typedef struct {
    /* 0x0  */ u_short x0;
    /* 0x2  */ u_short y0;
    /* 0x4  */ u_short z0;
    /* 0x6  */ s16 inUse;
    /* 0x8  */ u_short x1;
    /* 0xA  */ u_short y1;
    /* 0xC  */ u_short z1;
    /* 0xE  */ s16 unkE;
    /* 0x10 */ s16 modelIndex;
    /* 0x12 */ s16 jointIndex;
} Temp6; // Size: 0x14

// Per-model texture upload state, Model::unk118 entries. A battle sound-bank record
// is also read through this type, where 0x8 holds its SEDS entry and 0x14 its base id.
struct _Temp8;
typedef struct _Temp8 {
    /* 0x0  */ struct _Temp8* pSource;
    /* 0x4  */ u16* pPixels; // the region's own pixels: what a fade or blend reads, and what the upload writes back
    /* 0x8  */ void* pStartPixels; // the colours a blend starts from; the fourth argument of func_80026FE8
    /* 0xC  */ void* pResultPixels; // where a fade or blend writes: the third argument of func_80026F44/80026FE8, and what LoadImage uploads
    /* 0x10 */ u8 mode; // 0/1 raw copy, 4 fade, 5 blend
    /* 0x11 */ u8 unk11;
    /* 0x12 */ s16 pixelCount; // pixels the upload writes; the GPF helpers loop this many times
    /* 0x14 */ u16 angle; // advanced by angleStep per frame, first argument of pInterpolate; a sound-bank record read through this type keeps its base id here
    /* 0x16 */ u16 angleStep;
    /* 0x18 */ u16 prevFrame; // the interpolated frame last applied; the same value again skips the upload
    /* 0x1A */ u16 isLoaded; // buffers below are held; cleared by LibGearFlushTexture
    /* 0x1C */ s16* pRows; // region rows of 3 pixels, indexed by rect.y + row
    /* 0x20 */ s16 divisor; // second argument of pInterpolate
    /* 0x22 */ s16 offset; // third argument of pInterpolate
    /* 0x24 */ s32 (*pInterpolate)(u_short frame, s16 x, s16 y); // maps unk14 to the blend value; called with (unk14, divisor, offset)
    /* 0x28 */ RECT rect;
} Temp8; // Size: 0x30

// Point record a Temp9::pPoints entry addresses (stride 0x18); the model-list loop
// of LibGearRenderFrame writes the projected x/y/z into it and func_801E22F8 walks
// the entries as the trail each section leaves behind.
typedef struct {
    /* 0x0  */ s16 unk0;
    /* 0x2  */ s16 unk2;
    /* 0x4  */ u16 x;
    /* 0x6  */ u16 y;
    /* 0x8  */ u16 z;
    /* 0xA  */ u16 unkA[7];
} Temp9Point; // Size: 0x18

// One projected section of a model-list entry (stride 0x10): the source SVECTOR,
// whose pad slot holds the joint the section rides, then the projected x/y/z.
typedef struct {
    /* 0x0 */ SVECTOR vector;
    /* 0x8 */ u16 x;
    /* 0xA */ u16 y;
    /* 0xC */ u16 z;
    /* 0xE */ u16 unkE;
} Temp9Section; // Size: 0x10

// Per-model list entry, Model::unk114 entries
typedef struct {
    /* 0x0  */ s16 jointIndex; // negative: the entry is free
    /* 0x2  */ s16 unk2;
    /* 0x4  */ s16 numVectors; // SVECTORs pVectors holds
    /* 0x6  */ s16 numSegments; // records pSegments holds, and the outer loop count of func_801E22F8
    /* 0x8  */ s16 numPoints; // point records pPoints[0] holds; the count its smoothing loops run to
    /* 0xA  */ s16 numSections; // sections pSections holds
    // Two RGB colours; func_801E22F8 feeds one of them to NCCS as the light colour
    /* 0xC  */ u8 red0;
    /* 0xD  */ u8 green0;
    /* 0xE  */ u8 blue0;
    /* 0xF  */ u8 red1;
    /* 0x10 */ u8 green1;
    /* 0x11 */ u8 blue1;
    /* 0x12 */ u8 pad12[0x2];
    /* 0x14 */ SVECTOR* pVectors;
    /* 0x18 */ Temp9Section* pSections;
    /* 0x1C */ Temp9Point** pPoints;
    /* 0x20 */ void* pSegments; // numSegments 0x18 byte records; the first two s16 are indices into pPoints[0]
} Temp9; // Size: 0x24

typedef struct {
    /* 0x0  */ s16 jointIndex; // joint whose matrix projects the two sources; negative = free
    /* 0x2  */ u8 is3D; // non-zero: the sources are projected in 3D and pass through the GTE
    /* 0x3  */ u8 isSemiTrans; // handed to LibGearAllocVertex
    /* 0x4  */ u32 pVertexBuffer; // the VertexBuffer the vertex below is allocated from
    /* 0x8  */ u32 pVertex; // the Vertex this entry drives; 0 = none allocated yet
    /* 0xC  */ SVECTOR sourceA; // the two points the joint's matrix projects
    /* 0x14 */ SVECTOR sourceB;
    /* 0x1C */ u16 screenA[16]; // projected x/y, two 0x10-byte slots of four SXY quads
    /* 0x3C */ u16 screenB[16];
    /* 0x5C */ s16 slot; // the quad the next projection writes; counts down and wraps at 8
    /* 0x5E */ s16 age; // frames drawn; 0 after the wrap retires the entry
    /* 0x60 */ s16 numFrames;
    /* 0x62 */ s16 divisor;
    /* 0x64 */ s16 red;
    /* 0x66 */ s16 green;
    /* 0x68 */ s16 blue;
    /* 0x6A */ s16 redDelta;
    /* 0x6C */ s16 greenDelta;
    /* 0x6E */ s16 blueDelta;
} UnkInner1; // Size: 0x70

// Entry of a per-model pointer list (Model::unk14 / Model::unk18)
typedef struct {
    /* 0x0  */ u8 unk0[0x2];
    /* 0x2  */ u16 frameCount; // frames the script runs before it restarts
    /* 0x4  */ u8 unk4[0xE];
    /* 0x12 */ u16 numCommands; // commands the script holds
    /* 0x14 */ u32 dataOffset; // offset from the start of this entry
} Temp7; // Size: 0x18

typedef struct {
    /* 0x0   */ ModelMesh* pMesh;
    /* 0x4   */ ModelJoint* pSkeleton; // Array of joints
    /* 0x8   */ u32* pAnimTable; // address of the entry table a queued animation id indexes
    /* 0xC   */ Temp8* unkC;
    /* 0x10  */ u32 animValue; // the table entry that id selects
    /* 0x14  */ Temp7** pEntryTableLow; // entries for animation ids below 0x40 (indexed id + 1)
    /* 0x18  */ Temp7** pEntryTableHigh; // and from 0x40 up (indexed id - 0x3F)
    /* 0x1C  */ s16 scale;
    /* 0x1E  */ s16 unk1E;
    /* 0x20  */ u8 modelSlot; // this model's index in g_LibGearModels
    /* 0x21  */ u8 linkedModelSlot; // the g_LibGearModels slot sourceModelSlot 0xFC resolves to
    /* 0x22  */ u8 unk22[0x1];
    /* 0x23  */ u8 unk23;
    /* 0x24  */ s16 height;
    /* 0x26  */ s16 width;
    /* 0x28  */ s16 depth;
    /* 0x2A  */ u8 commands[0x1]; // the queued command: low 7 bits index the entry tables, bit 7 is a flag
    /* 0x2B  */ u8 commandCount; // appended model/animation changes, capped at 5
    /* 0x2C  */ u8 unk2C[0x2]; // the rest of the 5-byte queue commands starts
    /* 0x2E  */ u8 commandArgs[0x6]; // the argument queued alongside the command's model slot
    /* 0x34  */ u8 doUpdate; // gates the per-frame animation update; inherited from the parent model
    /* 0x35  */ u8 unk35;
    /* 0x36  */ u8 skipGroundSnap; // non-zero: LibGearApplyGroundHeight leaves the root joint's y alone
    /* 0x37  */ u8 useScaledMatrices; // non-zero: the step uses LibGearComputeScaledJointMatrices
    /* 0x38  */ u8 unk38;
    /* 0x39  */ u8 depthShade; // written every frame from the model's OT depth (0x6B - otz / 8)
    /* 0x3A  */ s16 animationBitMask; // g_LibGearModelBitMask the current animation was started with; -1 = none
    /* 0x3C  */ u16 objectType; // joint objects whose type matches it set a bit of the step result
    /* 0x3E  */ u16 unk3E;
    /* 0x40  */ s16 frameTimer; // the wait opcode's frame counter: advanced by the frame count, reset at the operand
    /* 0x42  */ s16 repeatCount; // the repeat opcode's counter: advanced per entry, reset at the operand
    /* 0x44  */ u16 waitTimer; // frames elapsed of the wait that resumes at pWaitScript
    /* 0x46  */ u16 waitFrames; // that wait's length
    /* 0x48  */ s16 arrivalDistance; // pArrivalScript resumes once LibGearGetModelDistance is within it
    /* 0x4A  */ u16 flags;
    /* 0x4C  */ u32 pArrivalScript; // script offset resumed when the model is within arrivalDistance
    /* 0x50  */ u32 pWaitScript; // script offset resumed once waitTimer reaches waitFrames
    /* 0x54  */ u32 pGroundScript; // script offset resumed when the root joint rises above ground
    /* 0x58  */ s16 sourceModelSlot; // g_LibGearModels slot whose transform places this one; 0xFF..0xFA pick it indirectly, 1..0x7F is slot + 1
    /* 0x5A  */ s16 sourceJointIndex; // joint of the model sourceModelSlot selects, whose transform places this one
    /* 0x5C  */ u8 parentModelSlot; // g_LibGearModels slot whose transform this one inherits; 0xFF = none
    /* 0x5D  */ u8 isAttached;
    /* 0x5E  */ s16 parentJointIndex;
    /* 0x60  */ s16 ground; // seems to be some kind of offset? maybe "below this point model will clip through the ground"?
    /* 0x62  */ u8 hasSedsEntry; // a SEDS entry is open for this model; LibGearFreeModel removes it
    /* 0x63  */ u8 pad63[0x1];
    /* 0x64  */ u16 localX; // the parent-relative position the parent matrix rotates into posX/posY/posZ
    /* 0x66  */ u16 localY;
    /* 0x68  */ u16 localZ;
    /* 0x6A  */ u16 attachOffsetX;
    /* 0x6C  */ u16 attachOffsetY;
    /* 0x6E  */ u16 attachOffsetZ;
    /* 0x70  */ u16 rotOffset[3]; // added to the root joint's rotation, >>3
    /* 0x76  */ u16 rotStep[3]; // added to rotOffset every frame
    /* 0x7C  */ s16 posOffset[3]; // scaled by the root joint's scale and rotated into its translation
    /* 0x82  */ u16 posStep[3]; // added to posOffset every frame
    /* 0x88  */ s16 posX;
    /* 0x8A  */ s16 posY;
    /* 0x8C  */ s16 posZ;
    /* 0x8E  */ s16 unk8E;
    /* 0x90  */ s16 textureOffsetX; // added to the material records' texture x; negative = no offset
    /* 0x92  */ u16 textureOffsetY; // and to their y
    /* 0x94  */ u16 blendOffsetX; // added to the blend target's x
    /* 0x96  */ u16 blendOffsetY; // and to its y
    /* 0x98  */ s16 frame; // frame of the animation script; -1 = stopped
    /* 0x9A  */ s16 frameCount; // frame the script restarts at; -1 = never
    /* 0x9C  */ s16 commandIndex; // commands of the current frame already run
    /* 0x9E  */ s16 numCommands; // commands the script holds
    /* 0xA0  */ u8* pScriptCursor; // the command being run
    /* 0xA4  */ u8* pScriptStart; // where pScriptCursor restarts each loop
    /* 0xA8  */ void* pModelFile;
    /* 0xAC  */ void* pModelSource; // the model's source record; LibGearFreeModel releases it
    /* 0xB0  */ void* pSoundBank1; // the record whose +8 is the SEDS entry of battle sound bank 1
    /* 0xB4  */ void* pSoundBank2; // and bank 2
    /* 0xB8  */ POLY_FT4 polys[2];
    /* 0x108 */ u8 unk108[0x2];
    /* 0x10A */ u16 modelBitMask;
    /* 0x10C */ u8 numColorEntries;
    /* 0x10D */ u8 numModelListEntries;
    /* 0x10E */ u8 numTextures;
    /* 0x10F */ u8 unk10F;
    /* 0x110 */ UnkInner1* pColorEntries; // 0x70 sized array, numColorEntries entries
    /* 0x114 */ Temp9* pModelListEntries; // numModelListEntries entries
    /* 0x118 */ Temp8* pTextures; // numTextures entries
    /* 0x11C */ s32 prevPosX; // root joint translation before the step
    /* 0x120 */ s32 prevPosY;
    /* 0x124 */ s32 prevPosZ;
    /* 0x128 */ s32 posDeltaX; // prevPos - the translation after the step
    /* 0x12C */ s32 posDeltaY;
    /* 0x130 */ s32 posDeltaZ;
} Model; // Size: 0x134

// Keyframe block consumed by the joint update loops: a 0x18 byte header
// followed by per-frame (rotation, translation) s16 triples.
typedef struct {
    /* 0x0  */ u16 unk0;
    /* 0x2  */ u16 endFrame; // frame the keyframes run to
    /* 0x4  */ u16 flags; // bit 0 skips the rotation channel, bit 1 the translation channel
    /* 0x6  */ u16 type; // 0: the keyframes below are interpolated; otherwise they are applied as they are
    /* 0x8  */ u16 unk8;
    /* 0xA  */ u16 unkA;
    /* 0xC  */ u16 numRotFrames; // rotation keyframes that follow the header
    /* 0xE  */ u16 numTransFrames; // translation keyframes that follow them
    /* 0x10 */ u16 unk10;
    /* 0x12 */ u16 unk12;
    /* 0x14 */ u16 unk14;
    /* 0x16 */ u16 unk16;
    /* 0x18 */ s16 keyframes[0];
} KeyframeBlock; // Size: 0x18

#define MAX_MODEL_MESHES 8
#define MAX_MODELS 10

extern ModelMesh g_LibGearMeshes[MAX_MODEL_MESHES];
extern Temp6 g_LibGearModelOffsets[2];
extern Model* g_LibGearModels[MAX_MODELS];
extern s16* D_801E8644;
extern Temp4 g_LibGearJointObjectPool;

void LibGearInitialize(int);

#endif