#ifndef _XENO_FIELD_MAIN_H
#define _XENO_FIELD_MAIN_H

#include "psyq/libgpu.h"
#include "psyq/libgte.h"

#define setRGB(obj, _color) obj.r0 = obj.g0 = obj.b0 = _color

#define SCREEN_WIDTH 320U
#define SCREEN_HEIGHT 224U

#define FIELD_OT_MAX_SIZE 4096

// FieldScene.unk48 flags
#define FIELD_SCENE_INTERPOLATING_SCREEN_Z 0x10

typedef struct {
    // There could potentially be more fields to the struct here

    /* 0x0   */ MATRIX viewMatrix;
    /* 0x20  */ MATRIX camRotationMatrix;
    /* 0x40  */ SVECTOR camRotation;
    /* 0x48  */ s32 unk48; // Flags related to random encounters?
    /* 0x4C  */ undefined8 unk4C[0x8];
    /* 0x54  */ SVECTOR sceneAngle;
    /* 0x5C  */ s32 unk5C; // Scene angle interpolation step
    /* 0x60  */ s32 unk60;
    /* 0x64  */ u_char dollySet;
    /* 0x65  */ u_char dollyStop;
    /* 0x66  */ s16 unk66; // Scene angle interpolation frames left
    /* 0x68  */ u_int sceneScrZ;
    /* 0x6C  */ s16 sceneDIP;
    /* 0x6E  */ s16 sceneScale;
    /* 0x70  */ s16 unk70; // Scene DIP interpolation duration
    /* 0x72  */ undefined16 unk72;
    /* 0x74  */ s32 unk74; // Scene DIP interpolation start, fixed point
    /* 0x78  */ s32 unk78; // Scene DIP interpolation delta per frame
    /* 0x7C  */ u_long unk7C;
    /* 0x80  */ s16 unk80; // Scene Z interpolation duration
    /* 0x82  */ undefined16 unk82;
    /* 0x84  */ s32 unk84; // Scene Z interpolation start, fixed point
    /* 0x88  */ s32 unk88; // Scene Z interpolation delta per frame
    /* 0x8C  */ s16 unk8C; // Scene scale interpolation duration
    /* 0x8E  */ undefined16 unk8E;
    /* 0x90  */ s32 unk90; // Scene scale interpolation start, fixed point
    /* 0x94  */ s32 unk94; // Scene scale interpolation delta per frame
    /* 0x98  */ undefined8 unk98[0x20];
    /* 0xB8  */ s16 unkB8; // Camera direction when the interpolation started
    /* 0xBA  */ undefined16 unkBA;
    /* 0xBC  */ s32 unkBC; // sceneScrZ when the interpolation started
    /* 0xC0  */ s16 unkC0; // sceneDIP when the interpolation started
    /* 0xC2  */ undefined16 unkC2;
    /* 0xC4  */ SVECTOR worldRotation;
    /* 0xCC  */ SVECTOR worldTranslation;
    /* 0xD4  */ MATRIX worldToScreenMatrix;
    /* 0xF4  */ MATRIX unkF4;
    /* 0x114 */ MATRIX worldRotationMatrix;
} FieldScene; // Size: 0x134

typedef struct {
    /* 0x0  */ s16 x0;
    /* 0x2  */ s16 y0; // Height?
    /* 0x4  */ s16 z0;
    /* 0x6  */ s16 x1;
    /* 0x8  */ s16 y1; // Unused?
    /* 0xA  */ s16 z1;
    /* 0xC  */ s16 x2;
    /* 0xE  */ s16 y2; // Unused?
    /* 0x10 */ s16 z2;
    /* 0x12 */ s16 x3;
    /* 0x14 */ s16 y3; // Unused?
    /* 0x16 */ s16 z3;
} FieldTriggerZone; // Size: 0x18

extern FieldTriggerZone* g_pFieldTriggerZones;

extern FieldScene g_Scene;
extern int g_WorldScale;


typedef struct {
    DRAWENV drawEnvs[2];
    DISPENV dispEnv;
    u_long ot1[0x1000];
    int unk1;
    u_long ot2[0x1000];
    int unk2;
    u_long ot3[0x8];
} RenderContext;

typedef struct {
    SVECTOR vertices[4];
    POLY_FT4 polys[2];
} Quad;

typedef struct {
    short x;
    short y;
    short z;
    short w;
} SVec4;

typedef struct {
    // 0x800b11ac
    DR_MODE drawModes[10];
    
    // 0x800b1224
    RECT rects[10];

    // 0x800b1274
    POLY_FT4 polygons[10];

    // 0x800b1404
    SVECTOR vectors[20]; // Vectors for RotAverage4
} ZoomFadeEffect;

// Random assortment of control-related things
typedef struct {
    short isRandomEncountersEnabled;
    short unkAngle;
    short controllerBtnMask;
} FieldControl;

extern FieldControl g_FieldControl;

extern ZoomFadeEffect g_FieldZoomFadeEffect;

extern int D_80059198; // Has initialized render contexts?

#define SYSTEM_MODE_PC_HDD 0x0
#define SYSTEM_MODE_CD_ROM 0x1

extern int g_FieldSystemMode;

extern int g_FieldCurRenderContextIndex;
extern int g_FieldRenderContextUseOT2;
extern RenderContext* g_FieldCurRenderContext;
extern RenderContext g_FieldRenderContexts[2];

#endif
