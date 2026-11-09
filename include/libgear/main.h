#ifndef __LIBGEAR_MAIN_H__
#define __LIBGEAR_MAIN_H__

#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "system/math.h"
#include "system/model.h"

struct _ModelJoint;
typedef struct _ModelJoint ModelJoint;
struct _ModelJoint {
    /* 0x00 */ ModelJoint* pParent;
    /* 0x04 */ u_char doTransform;
    /* 0x05 */ u_char doRotate;
    /* 0x06 */ u_char rotDirection;
    /* 0x07 */ u_char unk7;
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
    /* 0x70 */ u32 unk70;
    /* 0x74 */ u32 unk74;
    /* 0x78 */ u32 unk78;
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
    /* 0xE  */ short unkE;
    
    /* 0x10 */ u_short x2;
    /* 0x12 */ u_short y2;
    /* 0x14 */ u_short z2;
    /* 0x16 */ short unk16;
    
    /* 0x18 */ u_short x3;
    /* 0x1A */ u_short y3;
    /* 0x1C */ u_short z3;
    /* 0x1E */ short unk1E;
    
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

/* Unknown list */
typedef struct {
    /* 0x0  */ u8 unk0;
    /* 0x1  */ u8 unk1;
    /* 0x2  */ u8 unk2;
    /* 0x3  */ u8 unk3;
    /* 0x4  */ s16 unk4;
    /* 0x6  */ s16 unk6;
    /* 0x8  */ s16 unk8;
    /* 0xA  */ s16 unkA;
    /* 0xC  */ s16 unkC;
    /* 0xE  */ s16 unkE;
    /* 0x10 */ s16 unk10;
    /* 0x12 */ s16 unk12;
} Temp3; // Size: 0x14

typedef struct {
    /* 0x0 */ Temp3* pObjects;
    /* 0x4 */ u16 unk4;
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
    /* 0x6  */ s16 unk6;
    /* 0x8  */ u_short x1;
    /* 0xA  */ u_short y1;
    /* 0xC  */ u_short z1;
    /* 0xE  */ s16 unkE;
    /* 0x10 */ s16 modelIndex;
    /* 0x12 */ s16 jointIndex;
} Temp6; // Size: 0x14

// Per-model texture upload state, Model::unk118 entries
struct _Temp8;
typedef struct _Temp8 {
    /* 0x0  */ struct _Temp8* pSource;
    /* 0x4  */ u16* pUnk4;
    /* 0x8  */ void* pUnk8;
    /* 0xC  */ void* pUnkC;
    /* 0x10 */ u8 unk10;
    /* 0x11 */ u8 pad11;
    /* 0x12 */ s16 unk12;
    /* 0x14 */ u16 unk14;
    /* 0x16 */ u16 unk16;
    /* 0x18 */ u16 unk18;
    /* 0x1A */ u16 unk1A;
    /* 0x1C */ void* pUnk1C;
    /* 0x20 */ s16 unk20;
    /* 0x22 */ s16 unk22;
    /* 0x24 */ s16 (*unk24)(u_short);
    /* 0x28 */ RECT rect;
} Temp8; // Size: 0x30

// Per-model list entry, Model::unk114 entries
typedef struct {
    /* 0x0  */ u8 unk0[0x14];
    /* 0x14 */ void* unk14;
    /* 0x18 */ void* unk18;
    /* 0x1C */ void** unk1C;
    /* 0x20 */ void* unk20;
} Temp9; // Size: 0x24

typedef struct {
    /* 0x0  */ s16 unk0;
    /* 0x2  */ u8 unk2;
    /* 0x3  */ u8 unk3;
    /* 0x4  */ u32 unk4;
    /* 0x8  */ u32 unk8;
    /* 0xC  */ s16 unkC;
    /* 0xE  */ s16 unkE;
    /* 0x10 */ s16 unk10;
    /* 0x12 */ s16 unk12;
    /* 0x14 */ s16 unk14;
    /* 0x16 */ s16 unk16;
    /* 0x18 */ s16 unk18;
    /* 0x1A */ u8 unk1A[0x42];
    /* 0x5C */ s16 unk5C;
    /* 0x5E */ s16 unk5E;
    /* 0x60 */ s16 unk60;
    /* 0x62 */ s16 unk62;
    /* 0x64 */ s16 unk64;
    /* 0x66 */ s16 unk66;
    /* 0x68 */ s16 unk68;
    /* 0x6A */ s16 unk6A;
    /* 0x6C */ s16 unk6C;
    /* 0x6E */ s16 unk6E;
} UnkInner1; // Size: 0x70

// Entry of a per-model pointer list (Model::unk14 / Model::unk18)
typedef struct {
    /* 0x0  */ u8 unk0[0x2];
    /* 0x2  */ u16 unk2;
    /* 0x4  */ u8 unk4[0xE];
    /* 0x12 */ u16 unk12;
    /* 0x14 */ u32 unk14; // offset from the start of this entry
} Temp7; // Size: 0x18

typedef struct {
    /* 0x0   */ ModelMesh* pMesh;
    /* 0x4   */ ModelJoint* pSkeleton; // Array of joints
    /* 0x8   */ u32 unk8;
    /* 0xC   */ u32 unkC;
    /* 0x10  */ u32 unk10;
    /* 0x14  */ Temp7** unk14;
    /* 0x18  */ Temp7** unk18;
    /* 0x1C  */ s16 scale;
    /* 0x1E  */ s16 unk1E;
    /* 0x20  */ u8 unk20;
    /* 0x21  */ u8 unk21;
    /* 0x22  */ u8 unk22[0x1];
    /* 0x23  */ u8 unk23;
    /* 0x24  */ s16 unk24;
    /* 0x26  */ s16 unk26;
    /* 0x28  */ s16 unk28;
    /* 0x2A  */ u8 unk2A;
    /* 0x2B  */ u8 unk2B;
    /* 0x2C  */ u8 unk2C[0x8];
    /* 0x34  */ u8 unk34;
    /* 0x35  */ u8 unk35;
    /* 0x36  */ u8 unk36;
    /* 0x37  */ u8 unk37;
    /* 0x38  */ u8 unk38;
    /* 0x39  */ u8 unk39;
    /* 0x3A  */ s16 unk3A;
    /* 0x3C  */ u16 unk3C;
    /* 0x3E  */ u16 unk3E;
    /* 0x40  */ s16 unk40;
    /* 0x42  */ s16 unk42;
    /* 0x44  */ u8 unk44[0x6];
    /* 0x4A  */ u16 unk4A;
    /* 0x4C  */ u32 unk4C;
    /* 0x50  */ u32 unk50;
    /* 0x54  */ u32 unk54;
    /* 0x58  */ s16 unk58;
    /* 0x5A  */ s16 unk5A;
    /* 0x5C  */ u8 unk5C;
    /* 0x5D  */ u8 unk5D[0x3];
    /* 0x60  */ s16 ground; // seems to be some kind of offset? maybe "below this point model will clip through the ground"?
    /* 0x62  */ u8 unk62[0xE];
    /* 0x70  */ s16 unk70[0x10];
    /* 0x90  */ u8 unk90[0x8];
    /* 0x98  */ s16 unk98;
    /* 0x9A  */ s16 unk9A;
    /* 0x9C  */ s16 unk9C;
    /* 0x9E  */ s16 unk9E;
    /* 0xA0  */ u8* unkA0;
    /* 0xA4  */ u8* unkA4;
    /* 0xA8  */ void* unkA8;
    /* 0xAC  */ void* unkAC;
    /* 0xB0  */ void* unkB0;
    /* 0xB4  */ void* unkB4;
    /* 0xB8  */ POLY_FT4 polys[2];
    /* 0x108 */ u8 unk108[0x2];
    /* 0x10A */ u16 unk10A;
    /* 0x10C */ u8 unk10C;
    /* 0x10D */ u8 unk10D;
    /* 0x10E */ u8 unk10E;
    /* 0x10F */ u8 unk10F;
    /* 0x110 */ UnkInner1* unk110; // 0x70 sized array, unk10C entries
    /* 0x114 */ Temp9* unk114; // unk10D entries
    /* 0x118 */ Temp8* unk118; // unk10E entries
    /* 0x11C */ s32 unk11C; // joint translation snapshot, then displaced by the live one
    /* 0x120 */ s32 unk120;
    /* 0x124 */ s32 unk124;
    /* 0x128 */ s32 unk128;
    /* 0x12C */ s32 unk12C;
    /* 0x130 */ s32 unk130;
} Model; // Size: 0x134

// Keyframe block consumed by the joint update loops: a 0x18 byte header
// followed by per-frame (rotation, translation) s16 triples.
typedef struct {
    /* 0x0  */ u16 unk0;
    /* 0x2  */ u16 unk2;
    /* 0x4  */ u16 unk4; // bit 0 skips the rotation channel, bit 1 the translation channel
    /* 0x6  */ s16 unk6;
    /* 0x8  */ u16 unk8;
    /* 0xA  */ u16 unkA;
    /* 0xC  */ u16 unkC;
    /* 0xE  */ u16 unkE;
    /* 0x10 */ u16 unk10;
    /* 0x12 */ u16 unk12;
    /* 0x14 */ u16 unk14;
    /* 0x16 */ u16 unk16;
    /* 0x18 */ s16 keyframes[0];
} KeyframeBlock; // Size: 0x18

#define MAX_MODEL_MESHES 8
#define MAX_MODELS 10

extern ModelMesh g_LibGearMeshes[MAX_MODEL_MESHES];
extern Temp6 D_801E8648[2];
extern Model* g_LibGearModels[MAX_MODELS];
extern s16* D_801E8644;
extern Temp4 D_801E86A8;

void LibGearInitialize(int);

#endif