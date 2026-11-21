#ifndef _XENO_3D_MODEL_H
#define _XENO_3D_MODEL_H

#include "psyq/libgpu.h"
#include "psyq/libgte.h"

// Model part flags
#define MODEL_FLAG_HAS_LIGHT_DATA 0x1

#define PRIM_HAS_TEX_DATA 0x0
#define PRIM_NO_TEX_DATA 0x1

// All primitive data sent to renderer has a code
typedef struct {
    /* 0x0 */ u16 texpageAndClut;
    /* 0x2 */ u8 unk2;
    /* 0x3 */ u8 code;
} PrimitiveHeader;

typedef struct {
    /* 0x0 */ u8 r;
    /* 0x1 */ u8 g;
    /* 0x2 */ u8 b;
    /* 0x3 */ u8 code;
} UntexturedPrimitive;

typedef struct {
    /* 0x0 */ u8 u2;
    /* 0x1 */ u8 v2;
    /* 0x2 */ u8 unk2;
    /* 0x3 */ u8 code;
    /* 0x4 */ u8 u0;
    /* 0x5 */ u8 v0;
    /* 0x6 */ u8 u1;
    /* 0x7 */ u8 v1;
} TexturedTrianglePrimitive;

typedef struct {
    /* 0x0 */ u8 r;
    /* 0x1 */ u8 g;
    /* 0x2 */ u8 b;
    /* 0x3 */ u8 code;
    /* 0x4 */ u8 u0;
    /* 0x5 */ u8 v0;
    /* 0x6 */ u8 u1;
    /* 0x7 */ u8 v1;
    /* 0x8 */ u8 u2;
    /* 0x9 */ u8 v2;
    /* 0xA */ u8 u3;
    /* 0xB */ u8 v3;
} TexturedQuadPrimitive;




typedef struct {
    u32 unk0;
    u32 lightDataSize;
    u32 modelPacketSize;
} ModelPacketInfo; // Size: 0xC

typedef struct {
    /* 0x0  */ u_short flags;
    /* 0x2  */ u8 unk2[0x2];
    /* 0x4  */ u_short polyCount;
    /* 0x6  */ u16 nodeCount;
    /* 0x8  */ SVECTOR* pVertices;
    /* 0xC  */ u32 pNormals;
    /* 0x10 */ u32 pNodes;
    /* 0x14 */ CVECTOR* pColors;
    /* 0x18 */ void* pLightData;
    /* 0x1C */ u32* pLodData;
    /* 0x20 */ ModelPacketInfo unk20[2];
} ModelPart; // Size: 0x38

typedef struct {
    /* 0x0  */ int numModelParts;
    /* 0x4  */ u_int flags;
    /* 0x8  */ u8 unk8[0x8];
    /* 0x10 */ ModelPart modelParts[0]; // numModelParts sized
} ModelFileHeader; // Size: 0x10


#endif