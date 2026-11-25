#include "common.h"
#define BATTLE_SPOKE_COUNT 32 // spokes of the ring BattleDrawViewOriginRing draws
#define BATTLE_SPOKE_ANGLE_STEP 0x10 // angle increment per spoke, 4096-step turn
#define BATTLE_SPOKE_RADIUS 0x3F80 // ring radius, also the offset subtracted from the view origin
#define BATTLE_OT_DEPTH_MAX 0x1C00 // depth limit before the prim is queued
#define BATTLE_ANGLE_QUARTER_TURN 0x400 // a quarter turn in the 4096-step angle space
#define BATTLE_CAMERA_LERP_FRAMES 100 // frames the camera lerp restarts with
#define BATTLE_CLUT_ENTRY_COUNT 0x100 // palette entries the backdrop TIM's CLUT holds
#define BATTLE_SPRITE_PAIR_COUNT 0x40 // quads in the backdrop sprite pool
#define BATTLE_GRID_STATE_MASK 0x3000000 // the two-bit corner-state field of a grid vertex word
#define BATTLE_GRID_SPACING 0x100 // one grid vertex to the next, in position units
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "psyq/inline_c.h"
#include "system/memory.h"

extern s32 g_BattleSavedScreenMode;
extern s32 g_BattleCameraAngle;
extern s32* g_BattleGridCells;
extern u8 g_BattleDrawBufferIndex;

typedef struct {
    /* 0x0 */ s32 unk0[4];
} Unk16; // Size: 0x10

extern SVECTOR g_BattlePlaneNormal;
extern VECTOR g_BattleViewOrigin;
extern VECTOR g_BattleViewOriginZ;
extern POLY_FT4* g_BattleDrawPrimPools[2];
extern s32 g_BattleCameraPos;
extern s32 g_BattleCameraTarget;
extern s32 g_BattleCameraPosY;
extern s32 g_BattleCameraPosZ;
extern s32 g_BattleCameraOrbitRadius;
extern s32 g_BattleCameraYOffset;
extern s32 g_BattleViewOriginY;
extern s32 g_BattleMode;
extern s32 g_BattleCameraDebugEnabled;
extern s32 g_BattleFloorOutlineArc;
extern u8 g_BattleCameraLerpFrames;
extern s32 g_BattleCameraAngleStep;

extern s32 g_BattleScreenMode;
extern s32 g_BattleCameraSpinStep;
extern s32 g_BattleTransitionFrames;
extern s16 g_BattleCombatant0DownCount;
extern s16 g_BattleCombatant1DownCount;

extern int rand(void);

extern s32 g_BattleElapsedFrames;
extern char g_StrBattleElapsedTimeFormat[];
extern void BattleDrawTextLeft(char* p);

// The light payload func_80030A30 normalizes into the light vector table: a
// direction and its colour (the same 0x14-byte record as misc13's Misc13Sprite).
typedef struct {
    /* 0x00 */ s32 dirX;
    /* 0x04 */ s32 dirY;
    /* 0x08 */ s32 dirZ;
    /* 0x0C */ s16 r;
    /* 0x0E */ s16 g;
    /* 0x10 */ s16 b;
} Poly9; // Size: unknown

// A model node of a light group (src/battling/main/misc13.c's Misc13Node): the
// state tag, the light payload BattleSetNodeLight gives the lit nodes and the
// local matrix the group copies into the renderer.
typedef struct Misc9Node {
    /* 0x00 */ s32 state;
    /* 0x04 */ Poly9* pLight;
    /* 0x08 */ void (*pUpdate)(void);
    /* 0x0C */ MATRIX localMatrix;
} Misc9Node; // Size: unknown (the whole node is 0x9C)

typedef struct {
    /* 0x0000 */ s32 pos[3];
    /* 0x000C */ u8 pad0C[0x15C8];
    /* 0x15D4 */ u8 effectColorR;
    /* 0x15D5 */ u8 effectColorG;
    /* 0x15D6 */ u8 effectColorB;
} BattlerData;

// The per-frame display work an actor group owns (the pointer it carries at
// +0x284): a double-buffered ordering table, one per draw buffer. misc14.c
// declares the whole block; this unit only reaches the two table pointers.
typedef struct {
    /* 0x0 */ s32 unk0;
    /* 0x4 */ u_long* pOt[2];
} Misc9DisplayWork; // Size: 0xC

typedef struct {
    /* 0x000 */ s32 x;
    /* 0x004 */ s32 y;
    /* 0x008 */ s32 z;
    /* 0x00C */ u8 pad0C[0xC8];
    /* 0x0D4 */ u32 stateFlags;
    /* 0x0D8 */ BattlerData* pTarget;
    /* 0x0DC */ u8 padDC[0x1A8];
    /* 0x284 */ Misc9DisplayWork* pDisplayWork;
    /* 0x288 */ u8 pad288[0x690];
    /* 0x918 */ u8 lightAlpha;
} Battler9;

// The 0x28C-byte container BattleCreateLightGroup builds: four nodes, the first
// of them the group's own, the three lit ones the light table follows, then the
// ordering-table pair it was created for.
typedef struct {
    /* 0x000 */ s32 unk000;
    /* 0x004 */ Misc9Node* pNode0;
    /* 0x008 */ Misc9Node* pNodes[3];
    /* 0x014 */ u8 pad014[0x270];
    /* 0x284 */ Misc9DisplayWork* pWork;
} Misc9LightGroup; // Size: unknown (0x28C; the display work is at 0x284)

extern u8* g_BattleStageLightRow;

// The two full-screen gradient quads the battle tile is drawn with. misc8's
// BattleSetScreenTileGrey writes their vertices and code byte; this unit writes
// their corner colours (corner 0/1 near, corner 2/3 far).
extern POLY_G4 D_80095580[2];

extern s32 BattleClearOrderingTable();
extern void func_80030A30(s32 index, Poly9* pPoly);
extern void func_80083310(s32 arg0);
extern void BattleSaveActorVectors(s32);
extern void func_8007AC3C(void);
extern void BattleInitRenderScene(s16* p);
extern void BattleSetScreenTileBlack(void);
extern void BattleDrawTextPrims(void* p);
extern void BattleAttachActorPrims(void* p);
extern void BattleAddSharedPrim(void);
extern void BattleAccumulateSpanColumns(s32 x0, s32 y0, s32 x1, s32 y1);
extern void AddPrim(void* pOt, void* pPrim);

extern u8* g_BattleRootOrderingTable;
// One draw-buffer pair of sprites and tpage prims, set up by misc8's
// func_80081ECC; misc8.c declares the same 0x14-byte block as Misc8DimSprites.
// the sprite block's fifth element: the name the original object uses for the
// base of the three SPRT AddPrim calls (D_800955F8 + 5 sprites)
extern SPRT D_80095648[];
extern DR_TPAGE g_BattleDimTpagePrims[];
// element 2 of g_BattleDimTpagePrims; the original object names the middle pair
extern DR_TPAGE D_800955D8[];
// The node pointers the light group's matrices are copied into, and the two
// effect node trees walked after them.
extern Misc9Node* g_BattleEffectNode;
extern Misc9Node* g_BattleActor0Node;
extern Misc9Node* g_BattleActor1Node;
extern Misc9Node* g_BattleEffectPool;

extern void BattleSetMatrixFromPoints(Misc9LightGroup* pGroup, VECTOR* pFrom, VECTOR* pTo);
extern void BattleResetTextPrimQueue(void);
extern void BattleDisableBackColor(void);
extern void BattleEffectQueuePrims(s32 pOt);
extern void BattleEffectQueueMatrixPrims(MATRIX* pMatrix, s32 pOt);
extern void BattleEffectSetMatrix(s32 pOt, MATRIX* pMatrix);
extern void BattleUpdateNodeTree(Misc9Node* pNode);
extern void BattleUpdateSoundSlotPositions(void);
extern void BattleSetStageFogColors(void);
extern void BattleClearSpanTables(void);
extern void BattleRenderShadedSpans(s32 pOt, s32 x, s32 z);
extern void BattleDrawActorShadow(void* pActor, u_long* pOt, MATRIX* pMatrix);
extern void BattleDrawFrameHud(void* pActor);
extern void BattleDrawTimedSpritePrims(s32 pOt);
extern void BattleQueueGaugePrims(void* pA, void* pB);
extern void func_80031678(void* pOt, void* pPrim);
extern MATRIX g_BattleBaseMatrix;

void BattleDrawHexagonOutline(s32 arg0, s32 arg1, s32 arg2) {
    s32 x0;
    s32 y0;
    s32 x;
    s32 y;
    s32 bx;
    s32 by;
    s32 ang;
    s32 px;
    s32 py;
    s32 i;

    bx = arg0;
    bx >>= 8;
    by = arg1 >> 8;
    x = bx + ((rcos(arg2 + 0x280) * 10) >> 12);
    x0 = x;
    y = by + ((rsin(arg2 + 0x280) * 10) >> 12);
    y0 = y;
    ang = arg2 + 0x580;
    for (i = 0; i < 6; i++) {
        px = bx + ((rcos(ang) * 24) >> 12);
        py = by + ((rsin(ang) * 24) >> 12);
        BattleAccumulateSpanColumns(x, y, px, py);
        x = px;
        y = py;
        ang += 0x100;
    }
    px = bx + ((rcos(arg2 - 0x280) * 10) >> 12);
    py = by + ((rsin(arg2 - 0x280) * 10) >> 12);
    BattleAccumulateSpanColumns(x, y, px, py);
    BattleAccumulateSpanColumns(x0, y0, px, py);
}

void BattleDrawArcOutline(s32 arg0, s32 arg1, s32 arg2) {
    s32 x0;
    s32 y0;
    s32 x;
    s32 y;
    s32 bx;
    s32 by;
    s32 px;
    s32 py;
    s32 ang;
    s32 i;

    bx = arg0 >> 8;
    by = arg1 >> 8;
    x = bx + ((rcos(arg2 + 0x100) << 4) >> 12);
    x0 = x;
    y = by + ((rsin(arg2 + 0x100) << 4) >> 12);
    y0 = y;
    ang = arg2 + 0x78A;
    for (i = 0; i < 3; i++) {
        px = bx + ((rcos(ang) << 5) >> 12);
        py = by + ((rsin(ang) << 5) >> 12);
        BattleAccumulateSpanColumns(x, y, px, py);
        x = px;
        y = py;
        ang += 0x75;
    }
    px = bx + ((rcos(arg2 - 0x100) << 4) >> 12);
    bx = rsin(arg2 - 0x100);
    py = by + ((bx << 4) >> 12);
    BattleAccumulateSpanColumns(x, y, px, py);
    BattleAccumulateSpanColumns(x0, y0, px, py);
}

void BattleCopyPlaneNormal(SVECTOR* p) {
    *p = g_BattlePlaneNormal;
}

extern void MathTriangleNormal(SVECTOR* pVertex1, SVECTOR* pVertex2, SVECTOR* pVertex3, SVECTOR* pNormal);

// A vertex of the battle floor grid: the floor height in the low halfword, and
// the corner-state field the height patch tests in the high one.
typedef union {
    /* 0x0 */ s32 word;
    /* 0x0 */ u16 height;
} BattleGridVertex;

// The floor height under pPos: the height of the battle grid cell's plane at
// that point, taking whichever of the cell's two triangles contains it. The
// corners are the four grid vertices around pPos, each pulled in by its
// corner-state field first.
// diff_func.sh cannot print MATCH for this body: retail's object names
// g_BattlePlaneNormal + 2 / + 4 as D_8009276A / D_8009276C, this spelling
// relocates g_BattlePlaneNormal with addend 2 / 4. The linked bytes and the
// 1016-byte size are identical, so judge it with tools/scripts/check_bin.sh.
// The cast view of the dot-product terms at &v3 and the three unused SVECTOR
// locals are load-bearing for the 0x70 frame: a VECTOR local there makes it
// 0x80 and dropping the unused ones shrinks it to 0x58.
s32 BattleGetFloorHeightAtPosition(VECTOR* pPos, s32 fixHeights) {
    SVECTOR padTop[1];
    SVECTOR v0;
    SVECTOR v1;
    SVECTOR v2;
    SVECTOR v3;
    SVECTOR padMid[1];
    SVECTOR tri[3];
    SVECTOR padBottom[1];
    VECTOR* pRef;
    BattleGridVertex* pCell;
    s32 mx;
    s32 mz;
    s32 x;
    s32 z;
    s32 side;

    x = pPos->vx;
    z = pPos->vz;
    mx = x & 0xFFFFFF00;
    mz = z & 0xFFFFFF00;
    x = x >> 8;
    z = z >> 8;
    pCell = (BattleGridVertex*)((u32)((z << 7) + x) * 4 + (u32)g_BattleGridCells);

    v0.vx = mx;
    v0.vy = pCell[0].height;
    v0.vz = mz;
    v1.vx = mx + BATTLE_GRID_SPACING;
    v1.vy = pCell[0x81].height;
    v1.vz = mz + BATTLE_GRID_SPACING;
    v2.vx = mx + BATTLE_GRID_SPACING;
    v2.vy = pCell[1].height;
    v2.vz = mz;
    v3.vx = mx;
    v3.vy = pCell[0x80].height;
    v3.vz = mz + BATTLE_GRID_SPACING;

    if (fixHeights) {
        switch (g_BattleGridCells[(z << 7) + x] & BATTLE_GRID_STATE_MASK) {
        case 0x1000000:
            v0.vy = v0.vy + 0xC0;
        case 0x3000000:
            v0.vy = v0.vy + 0x40;
            break;
        }
        switch (g_BattleGridCells[(z << 7) + x + 0x81] & BATTLE_GRID_STATE_MASK) {
        case 0x1000000:
            v1.vy = v1.vy + 0xC0;
        case 0x3000000:
            v1.vy = v1.vy + 0x40;
            break;
        }
        switch (g_BattleGridCells[(z << 7) + x + 1] & BATTLE_GRID_STATE_MASK) {
        case 0x1000000:
            v2.vy = v2.vy + 0xC0;
        case 0x3000000:
            v2.vy = v2.vy + 0x40;
            break;
        }
        switch (g_BattleGridCells[(z << 7) + x + 0x80] & BATTLE_GRID_STATE_MASK) {
        case 0x1000000:
            v3.vy = v3.vy + 0xC0;
        case 0x3000000:
            v3.vy = v3.vy + 0x40;
            break;
        }
    }

    side = (v0.vz - v1.vz) * pPos->vx + (v1.vx - v0.vx) * pPos->vz +
           v0.vx * v1.vz - v1.vx * v0.vz;
    if (side < 0) {
        tri[0] = v0;
        tri[1] = v1;
        tri[2] = v2;
    } else {
        tri[0] = v0;
        tri[1] = v3;
        tri[2] = v1;
    }

    MathTriangleNormal(&tri[0], &tri[1], &tri[2], &g_BattlePlaneNormal);

    pRef = (VECTOR*)&v3;
    pRef->vx = tri[0].vx;
    pRef->vy = tri[0].vy;
    pRef->vz = tri[0].vz;
    return pPos->vy + ((pRef->vx * g_BattlePlaneNormal.vx + pRef->vy * g_BattlePlaneNormal.vy +
                        pRef->vz * g_BattlePlaneNormal.vz) -
                       (pPos->vx * g_BattlePlaneNormal.vx + pPos->vy * g_BattlePlaneNormal.vy +
                        pPos->vz * g_BattlePlaneNormal.vz)) /
                          g_BattlePlaneNormal.vy;
}

// A battle-grid position in half-word form, as the cell lookups read it: the
// high byte of each component is the cell index (BattleGetGridCellAtPosition
// shifts a 32-bit position the same way).
typedef struct {
    /* 0x0 */ u16 vx;
    /* 0x2 */ u16 unk2;
    /* 0x4 */ u16 vz;
    /* 0x6 */ u16 unk6;
} BattleGridPosition; // Size: 0x8

s16 BattleGetGridValueAtPosition(BattleGridPosition* p) {
    s32 tmp[12]; // unused: an aggregate local is what gives the original's 0x30 frame
    s32 a;
    s32 b;
    s32 col;
    s32 row;

    a = p->vx << 16;
    b = p->vz << 16;
    col = a >> 24;
    row = (b >> 24) << 7;

    return *(s16*)&g_BattleGridCells[row + col];
}

u32 BattleGetGridCellAtPosition(s32* p) {
    s32 a;
    s32 b;

    a = p[0] >> 8;
    b = p[2] >> 8;
    return (u32)g_BattleGridCells[(b << 7) + a];
}

void BattleMovePointInsideRadius(VECTOR* p0, VECTOR* p1, s32 radius) {
    VECTOR v;
    VECTOR v2;
    VECTOR vSq;
    MATRIX m;
    MATRIX mT;
    SVECTOR n;
    s32 r;
    s32 ofs;

    v.vx = p0->vx + p1->vx - 0x3F80;
    v.vz = p0->vz + p1->vz - 0x3F80;
    Square0(&v, &vSq);
    if (radius < SquareRoot0(vSq.vx + vSq.vz)) {
        VectorNormalS(&v, &n);
        m.m[2][1] = 0;
        m.m[1][2] = 0;
        m.m[1][0] = 0;
        m.m[0][1] = 0;
        m.m[1][1] = 0x1000;
        m.m[2][2] = n.vz;
        m.m[0][0] = n.vz;
        m.m[0][2] = -n.vx;
        m.m[2][0] = n.vx;
        ApplyMatrixLV(&m, p1, &v);
        TransposeMatrix(&m, &mT);
        SetRotMatrix(&mT);
        v.vz = 0;
        while (1) {
            ApplyRotMatrixLV(&v, p1);
            v2.vx = p0->vx + p1->vx - 0x3F80;
            v2.vz = p0->vz + p1->vz - 0x3F80;
            Square0(&v2, &vSq);
            r = SquareRoot0(vSq.vx + vSq.vz);
            if (radius >= r) {
                break;
            }
            ofs = r - radius;
            v.vz += 8;
            v.vz -= ofs;
        }
    }
}

// A plain r,g,b triple: the tile-colour bases misc8's BattleSetScreenTileGrey
// and BattleSetScreenTileBlack also write.
typedef struct {
    /* 0x0 */ u8 r;
    /* 0x1 */ u8 g;
    /* 0x2 */ u8 b;
} BattleRgb; // Size: 0x3

extern u8 g_BattleStageLightIndex;
extern u8 g_BattleStageLightTable[];
extern s32 g_BattleBackColorA;
extern s32 g_BattleBackColorG;
extern s32 g_BattleBackColorB;
extern u8 g_BattleFogColors; // D_80095580[0].r0; retail's object names this byte itself
extern BattleRgb D_8009A1C4;
extern BattleRgb D_8009A2BC;
extern u32 g_NearColor;

void BattleSetStageFogColors(void) {
    u8* p;
    s32 farR;
    s32 farG;
    s32 farB;
    s32 nearR;
    s32 nearG;
    s32 nearB;
    s32 nearRG;
    s32 farRG;

    p = &g_BattleStageLightTable[g_BattleStageLightIndex * 17];
    g_BattleStageLightRow = p;
    nearR = p[0];
    nearG = p[1];
    nearB = p[2];
    g_BattleBackColorA = p[4];
    g_BattleBackColorG = p[5];
    g_BattleBackColorB = p[6];
    farR = p[8];
    farG = p[9];
    farB = p[10];
    SetNearColor(p[0xC], p[0xD], p[0xE]);
    SetFarColor(farR, farG, farB);
    D_80095580[0].g0 = nearG;
    D_80095580[1].g0 = nearG;
    nearRG = nearR | (nearG << 8);
    g_BattleFogColors = nearR;
    D_80095580[1].r0 = nearR;
    farRG = farR | (farG << 8);
    D_80095580[0].b0 = nearB;
    D_80095580[1].b0 = nearB;
    // each later corner's r,g is stored as one packed halfword
    *(u16*)&D_80095580[0].r1 = nearRG;
    D_80095580[0].b1 = nearB;
    *(u16*)&D_80095580[1].r1 = nearRG;
    D_80095580[1].b1 = nearB;
    *(u16*)&D_80095580[0].r2 = farRG;
    D_80095580[0].b2 = farB;
    *(u16*)&D_80095580[1].r2 = farRG;
    D_80095580[1].b2 = farB;
    *(u16*)&D_80095580[0].r3 = farRG;
    D_80095580[0].b3 = farB;
    *(u16*)&D_80095580[1].r3 = farRG;
    D_80095580[1].b3 = farB;
    D_8009A1C4.r = farR;
    D_8009A1C4.g = farG;
    D_8009A1C4.b = farB;
    D_8009A2BC.r = farR;
    D_8009A2BC.g = farG;
    D_8009A2BC.b = farB;
    SetFogNearFar(0x800, 0x1800, 0xC0);
    g_NearColor = (g_NearColor & 0xFFFFFF) | 0x28000000;
    gte_ldrgb(&g_NearColor);
}

extern u16 D_800927A0;
extern u16 D_800927A4;
extern u16 D_800927A8;
extern POLY_FT4* D_8009278C;
extern void func_800732AC(void* pDst, void* pSrc, s32 n);

// best 94.35% (0x218 bytes against the original 0x214; 134 insns vs 133); re-measured this round after an unpark, unchanged.
//    The body is instruction-identical to retail in every other respect - same frame,
//    same four setPolyFT4 tag bytes, same store order in both quads, same CLUT loop,
//    same two HeapAlloc calls with g_BattleDrawPrimPools[0] stored in the second
//    jal slot. The ONE difference is the register that holds the biased quad pointer
//    p+0x25: retail keeps it in $a1 (its preheader is addiu a1,v1,0x25, i.e. a FRESH
//    pseudo, so the just-freed $v1 then takes the first clut lhu), while this body
//    gets the in-place addiu v1,v1,37 because gcc folded p (dead at that instruction)
//    into the biased iv, and then the clut lhu takes $a1. That single flip is all 63
//    differing lines, and it also costs the 4 excess bytes: retail fills the
//    lbu D_800927A8 delay slot with the u3 store, this body hoisted u3 one slot
//    earlier and left a nop. objdiff exact alignment is not monotone here: the
//    p[0]/p[1] form is 93.62% at retail exact 0x214/133 while this one is 94.35% at
//    0x218/134.
//    Measured this round, no new best (4 scored runs, stop rule):
//    clut/tpage loads moved above the two setPolyFT4 calls (so the loads precede the
//    tag stores) - 87.92%, 75 lines, same 0x218/134, so the tag stores must stay
//    first; clut/tpage declared before POLY_FT4* p - byte-identical 94.35%, 63 lines,
//    so this hand-out is not declaration-order sensitive; quad 2 written in quad 1
//    store order (tpage postponed to the clut position) - 94.10%, 71 lines.
//    Older rounds, all at the same 94.35 unless noted: both quads in one shared
//    statement order, the pointer assigned in the for-init comma expression, the
//    pointer declared last among the locals, p[0]/p[1] with p += 2 (93.62%, exact
//    0x214/133 but the base bias moves to p+0x4D, so the p/p++ form is the right
//    source), 2.7.2 -O3 (94.35). Preset screen: cdk 71.26, 2.6.0 85.30 - the unit
//    2.7.2 -O2 -G0 is right. A 3-minute permuter run on this body did nothing
//    (base=935 best=905); it is below the 99 percent permuter bar now anyway.
//    Load-bearing finds (older rounds): D_800927A8 must stay a plain u16; a union or
//    a cast pointer makes gcc hoist its address into a callee-saved register and the
//    frame grows to 0x38. clut/tpage need locals re-read at the top of each quad or
//    the two lhu come out next to their stores. 68.73 -> 94.35 over five hand shapes.
//    When this body is un-parked, D_800927A0, D_800927A4, D_800927A8 and
//    D_8009278C need symbol_addrs entries (0x800927A0, 0x800927A4, 0x800927A8,
//    0x8009278C).
/*
void func_80082C4C(u8* pArg) {
    TIM_IMAGE tim;
    POLY_FT4* p;
    u_short* pClut;
    u16 clut;
    u16 tpage;
    s32 mask;
    s32 i;

    OpenTIM(*(u_long**)(pArg + 0x6C));
    ReadTIM(&tim);
    pClut = (u_short*)tim.caddr;
    for (i = 0, mask = -0x8000; i < BATTLE_CLUT_ENTRY_COUNT; i++) {
        *pClut++ |= mask;
    }
    LoadImage(tim.crect, tim.caddr);
    LoadImage(tim.prect, tim.paddr);
    D_800927A0 = GetClut(tim.crect->x, tim.crect->y);
    D_800927A4 = GetTPage(1, 0, tim.prect->x, tim.prect->y);
    D_800927A8 = (u8)tim.prect->y;
    g_BattleDrawPrimPools[0] = HeapAlloc(0xA00, 0);
    D_8009278C = HeapAlloc(0xA00, 0);
    p = g_BattleDrawPrimPools[0];
    for (i = 0; i < BATTLE_SPRITE_PAIR_COUNT; i += 2) {
        setPolyFT4(p);
        setPolyFT4(p + 1);
        clut = D_800927A0;
        tpage = D_800927A4;
        p->u0 = 0x7F;
        p->u1 = 0x7F;
        p->v0 = D_800927A8 + 0x3F;
        p->u2 = 0x3F;
        p->v1 = D_800927A8;
        p->u3 = 0x3F;
        p->v2 = D_800927A8 + 0x3F;
        p->clut = clut;
        p->tpage = tpage;
        p->v3 = D_800927A8;
        p++;
        clut = D_800927A0;
        tpage = D_800927A4;
        p->u0 = 0x3F;
        p->u1 = 0x3F;
        p->v0 = D_800927A8 + 0x3F;
        p->tpage = tpage;
        p->u2 = 0;
        p->v1 = D_800927A8;
        p->u3 = 0;
        p->v2 = D_800927A8 + 0x3F;
        p->clut = clut;
        p->v3 = D_800927A8;
        p++;
    }
    func_800732AC(D_8009278C, g_BattleDrawPrimPools[0], 0xA00);
}*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_80082C4C);

/* What this body cost to recover, all load-bearing:
   - `cam.vx -= 0x3F80; cam.vz -= 0x3F80;` are separate statements: retail
     stores the adjusted values back into the local VECTOR and calls
     ratan2(cam.vx, cam.vz).
   - every view-origin subtraction is an embedded assignment
     (`x -= (originX = g_BattleViewOrigin.vx - 0x3F80)`); the plain
     `X - (O - 0x3F80)` makes fold reassociate to `(X + 0x3F80) - O`.
   - the X origin is g_BattleViewOrigin (whose address retail hoists into a
     saved register) but the loop's Z origin is that same VECTOR's .vz, while
     the first column's Z origin is the separate symbol g_BattleViewOriginZ,
     which is at g_BattleViewOrigin + 8 (hence retail's `lw 8(s7)`).
   - `ang = ratan2(...) & 0xFFF0; ang -= 0x100;` as two statements: the
     one-expression form keeps the masked value alive and folds the later
     `ang += 0x10` into `addiu s3,s0,-240`.
   - a s32 copy of each x/y vertex pair (retail does `lw`/`sw`): the two-short
     form is four instructions, and a DVECTOR copy is unaligned lwl/lwr.
   - `pNext = pPrim + 1` is assigned before the depth compares so reorg puts
     the addiu in the `beqz` delay slot, and the OT index is a local assigned
     first inside the `if` so the `sra` lands in its delay slot.
   - gte_dpcs() had to be fixed to emit the canonical COP2 word: see the note
     in include/psyq/inline_c.h.
*/
void BattleDrawViewOriginRing(u_long* pOt, VECTOR* pCam) {
    VECTOR cam;
    SVECTOR vec[6];
    POLY_FT4* pPrim;
    POLY_FT4* pNext;
    s32 sz[4];
    s32 ang;
    s32 depth;
    s32 i;
    s32 originX;
    s32 originZ;
    s32 otIndex;
    s32 x;
    s32 preOriginX; // dead: see the note above vec[0].vx
    s32 preOriginZ;
    s32 z;

    cam = *pCam;
    pPrim = g_BattleDrawPrimPools[g_BattleDrawBufferIndex];
    cam.vx -= BATTLE_SPOKE_RADIUS;
    cam.vz -= BATTLE_SPOKE_RADIUS;
    ang = ratan2(cam.vx, cam.vz) & 0xFFF0;
    ang -= 0x100;
    vec[0].vy = vec[1].vy = 0;
    vec[2].vy = vec[3].vy = -0x290;
    vec[4].vy = vec[5].vy = -0x520;
    // The first spoke keeps its own origin copies: with the subtraction inlined
    // gcc 2.7.2 folds it to `x + 0x3F80` and emits the addiu on the wrong
    // operand; sharing originX/originZ with the loop hoists both loads.
    vec[0].vx = ((rcos(ang) * BATTLE_SPOKE_RADIUS) >> 12) - (preOriginX = g_BattleViewOrigin.vx - BATTLE_SPOKE_RADIUS);
    vec[0].vz = ((rsin(ang) * BATTLE_SPOKE_RADIUS) >> 12) - (preOriginZ = g_BattleViewOriginZ.vx - BATTLE_SPOKE_RADIUS);
    ang += BATTLE_SPOKE_ANGLE_STEP;
    for (i = 0; i < BATTLE_SPOKE_COUNT; i++) {
        vec[2].vx = vec[0].vx;
        vec[4].vx = vec[0].vx;
        vec[2].vz = vec[0].vz;
        vec[4].vz = vec[0].vz;
        x = ((rcos(ang) * BATTLE_SPOKE_RADIUS) >> 12) - (originX = g_BattleViewOrigin.vx - BATTLE_SPOKE_RADIUS);
        vec[1].vx = x;
        vec[3].vx = x;
        vec[5].vx = x;
        z = ((rsin(ang) * BATTLE_SPOKE_RADIUS) >> 12) - (originZ = g_BattleViewOrigin.vz - BATTLE_SPOKE_RADIUS);
        vec[1].vz = z;
        vec[3].vz = z;
        vec[5].vz = z;
        gte_ldv3(&vec[0], &vec[1], &vec[2]);
        gte_rtpt();
        gte_dpcs();
        gte_stsxy3(&pPrim->x0, &pPrim->x1, &pPrim->x2);
        gte_stsz3(&sz[0], &sz[1], &sz[2]);
        gte_ldv3(&vec[3], &vec[4], &vec[5]);
        gte_rtpt();
        pNext = pPrim + 1;
        depth = sz[0];
        if (depth < sz[1]) {
            depth = sz[1];
        }
        if (depth <= sz[2]) {
            depth = sz[2];
        }
        *(s32*)&pNext->x0 = *(s32*)&pPrim->x2;
        gte_stsxy(&pPrim->x3);
        gte_stsxy3(&pPrim->x3, &pNext->x2, &pNext->x3);
        gte_stsz(&sz[3]);
        *(s32*)&pNext->x1 = *(s32*)&pPrim->x3;
        if (depth <= sz[3]) {
            depth = sz[3];
        }
        if (depth < BATTLE_OT_DEPTH_MAX) {
            otIndex = depth >> 4;
            gte_strgb(&pPrim->r0);
            gte_strgb(&pNext->r0);
            setPolyFT4(pPrim);
            setPolyFT4(pNext);
            AddPrim(pOt + otIndex, pPrim);
            AddPrim(pOt + otIndex, pNext);
        }
        pPrim += 2;
        ang += BATTLE_SPOKE_ANGLE_STEP;
        vec[0].vx = vec[1].vx;
        vec[0].vz = vec[1].vz;
    }
}


void BattleRandomizeCameraPosition(void) {
    s32 ang;
    s32 scale;

    scale = (rand() & 0x1FFF) + 0x800;
    ang = rand() % 0x600 + 0x500;
    g_BattleCameraPos = ((rcos(ang) * scale) >> 12) + 0x4000;
    g_BattleCameraPosZ = ((rsin(ang) * scale) >> 12) + 0x4000;
    g_BattleCameraPosY = -((rand() & 0x7FF) + 0x400);
    g_BattleCameraOrbitRadius = 0x100;
    g_BattleCameraYOffset = 0x40;
    g_BattleCameraLerpFrames = 0x40;
    g_BattleCameraAngleStep = 0x400;
}

void BattleRotateCamera(s32 flags) {
    if (flags & 0x8000) {
        g_BattleCameraAngle += 0x20;
    }
    if (flags & 0x2000) {
        g_BattleCameraAngle -= 0x20;
    }
}

extern s32 g_BattleResultSide;
extern s32 g_BattleTransitionCountdown;
extern Battler9 g_BattleActor0;
extern Battler9 g_BattleActor1;
extern s32 BattleGetFloorHeightAtPosition(VECTOR* pPos, s32 fixHeights);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_80083310);
/* best 75.14% (0x3A4 bytes against the original's 0x38C). The body is
   instruction-for-instruction the original's apart from two things.
   (a) The camera target: retail reaches g_BattleCameraTarget through ONE callee-
   saved base register for all nine accesses (lw 0x0/0x4/0x8(s1), sw 0x0/0x4/0x8(s1)),
   i.e. gcc did NOT fold the +4/+8 displacements into the symbol. Every spelling
   tried here folds two of the three (pTarget->vy becomes lui %hi(g_BattleCameraTarget+4)
   plus lw %lo, +4 bytes each): a VECTOR* local, the global declared as a VECTOR with
   member access, and an unbounded array extern all measure identically under
   tools/scripts/cc1_probe.sh, so the original's base cannot have been a source
   pointer holding a bare symbol address. That is 6 instructions of the 24-byte
   excess.
   (b) Register choice: retail keeps the two combatant pointers in s2/s1 and the
   interpolation scale in s0; this body gets s1/s0 and a3 for the scale, which
   shifts every later s-register by one (camPos lands in s0 in both).
   Load-bearing shapes found while writing it: the interpolation target stores a raw
   copy first and then updates it, so each component is FOUR statements
   (target.vx = pB->x; then target.vx -= pA->x; then target.vx = target.vx * scale;
   then target.vx = target.vx / 256;) - gcc 2.7.2 has no dead-store pass, and
   merging the multiply and the divide into one expression reorders the three
   divides into the middle of the products (71.22% at the same size). After
   BattleMovePointInsideRadius the camera position is advanced by the clamped
   delta, and x goes through the pos pointer while z goes through the
   g_BattleCameraPosZ symbol, exactly as retail does.
// Advance the opening battle camera by one frame. The transition countdown picks
// the point on the segment between the two combatants the camera aims at; the
// target moves a third of the way there per frame unless arg0 asks for the whole
// step, and the camera then orbits that target at the ring radius.
void func_80083310(s32 arg0) {
    VECTOR pos;
    VECTOR delta;
    VECTOR target;
    Battler9* pA;
    Battler9* pB;
    VECTOR* pTarget;
    VECTOR* pCamPos;
    s32 scale;
    s32 angle;
    s32 floorY;
    SVECTOR padEnd[3];

    if (g_BattleResultSide) {
        pA = &g_BattleActor0;
        pB = &g_BattleActor1;
    } else {
        pA = &g_BattleActor1;
        pB = &g_BattleActor0;
    }
    angle = ratan2(pA->x - pB->x, pA->z - pB->z);
    if (g_BattleTransitionCountdown >= 0xB1) {
        scale = 0x100;
    } else if (g_BattleTransitionCountdown >= 0xA1) {
        scale = (g_BattleTransitionCountdown - 0xA0) * 16;
    } else {
        scale = 0;
    }
    pTarget = (VECTOR*)&g_BattleCameraTarget;
    target.vx = pB->x;
    target.vy = pB->y;
    target.vz = pB->z;
    target.vx -= pA->x;
    target.vy -= pA->y;
    target.vz -= pA->z;
    target.vx = target.vx * scale;
    target.vy = target.vy * scale;
    target.vz = target.vz * scale;
    target.vx = target.vx / 256;
    target.vy = target.vy / 256;
    target.vz = target.vz / 256;
    target.vx += pA->x;
    target.vy += pA->y;
    target.vz += pA->z;
    target.vy -= 0xA0;
    target.vx -= pTarget->vx;
    target.vy -= pTarget->vy;
    target.vz -= pTarget->vz;
    if (arg0) {
        target.vx /= 3;
        target.vy /= 3;
        target.vz /= 3;
    }
    g_BattleCameraOrbitRadius = 0xC00;
    pTarget->vy += target.vy;
    pTarget->vx += target.vx;
    pTarget->vz += target.vz;
    angle = g_BattleCameraAngle + g_BattleTransitionCountdown * g_BattleCameraSpinStep;
    pCamPos = (VECTOR*)&g_BattleCameraPos;
    pos.vx = (rcos(angle) * g_BattleCameraOrbitRadius) >> 12;
    pos.vy = -(g_BattleTransitionCountdown * 6 + 0x200);
    pos.vz = (rsin(angle) * g_BattleCameraOrbitRadius) >> 12;
    pos.vx += pTarget->vx;
    pos.vy += pTarget->vy;
    delta.vx = pos.vx - pCamPos->vx;
    pos.vz += pTarget->vz;
    delta.vz = pos.vz - pCamPos->vz;
    BattleMovePointInsideRadius(pCamPos, &delta, 0x3D00);
    pCamPos->vx += delta.vx;
    g_BattleCameraPosZ += delta.vz;
    floorY = BattleGetFloorHeightAtPosition(pCamPos, 0);
    if (floorY < pos.vy) {
        pos.vy = floorY;
    }
    g_BattleCameraPosY = pos.vy;
}

*/

void BattleRandomizeCameraSpin(void) {
    g_BattleCameraAngle = rand();
    g_BattleCameraSpinStep = rand() % 12 + 4;
    if (rand() & 1) {
        g_BattleCameraSpinStep = -g_BattleCameraSpinStep;
    }
    func_80083310(0);
}

extern s32 g_BattleCameraTarget;
extern s32 g_BattleCameraTargetY;
extern s32 g_BattleCameraTargetZ;
extern s32 g_BattleCameraOrbitAngle;
extern u8 g_BattleCameraSpinDirection;
extern s32 BattleVectorDistance(VECTOR* pA, VECTOR* pB);
extern s32 BattleVectorLength2D(VECTOR* p);

// Mirrors the camera on the combatants' midpoint and orbits it to whichever of
// the two candidate points on the ring is nearer, moving a fraction of the way
// there each frame (the ring shrinks as the pair closes).
void BattleAimCameraAtCombatants(VECTOR* pA, VECTOR* pB) {
    VECTOR pos;
    VECTOR alt;
    VECTOR pad0[2]; // unused: the original's frame carries two more 0x10 slots
    VECTOR* pCamPos;
    long* pBvz; // load-bearing: pB->vz read inline loses the original's load pairing
    s32* pTargetX;
    s32 angle;
    s32 dist;
    s32 radius;
    s32 lenPos;
    s32 lenAlt;

    angle = ratan2(pA->vx - pB->vx, pA->vz - pB->vz);
    dist = BattleVectorDistance(pA, pB);
    pTargetX = &g_BattleCameraTarget;
    *pTargetX = (pA->vx + pB->vx) / 2;
    radius = dist * 2 / 3 + 0xC0;
    pBvz = &pB->vz;
    g_BattleCameraTargetY = (pA->vy + pB->vy) / 2 - 0xA0;
    g_BattleCameraTargetZ = (pA->vz + *pBvz) / 2;
    g_BattleCameraOrbitRadius = radius;
    pos.vx = *pTargetX + ((rcos(angle - BATTLE_ANGLE_QUARTER_TURN) * g_BattleCameraOrbitRadius) >> 12);
    pos.vz = g_BattleCameraTargetZ + ((rsin(angle - BATTLE_ANGLE_QUARTER_TURN) * g_BattleCameraOrbitRadius) >> 12);
    alt.vx = *pTargetX + ((rcos(angle + BATTLE_ANGLE_QUARTER_TURN) * g_BattleCameraOrbitRadius) >> 12);
    alt.vz = g_BattleCameraTargetZ + ((rsin(angle + BATTLE_ANGLE_QUARTER_TURN) * g_BattleCameraOrbitRadius) >> 12);
    pos.vx -= g_BattleCameraPos;
    pos.vy -= g_BattleCameraPosY;
    pos.vz -= g_BattleCameraPosZ;
    alt.vx -= g_BattleCameraPos;
    alt.vy -= g_BattleCameraPosY;
    alt.vz -= g_BattleCameraPosZ;
    lenPos = BattleVectorLength2D(&pos);
    lenAlt = BattleVectorLength2D(&alt);
    if (lenAlt < lenPos) {
        g_BattleCameraAngleStep = BATTLE_ANGLE_QUARTER_TURN;
        g_BattleCameraSpinDirection = 0;
    } else {
        g_BattleCameraAngleStep = -BATTLE_ANGLE_QUARTER_TURN;
        g_BattleCameraSpinDirection = 1;
    }
    dist /= 4;
    if (dist > 0x300) {
        dist = 0x300;
    }
    pos.vy = g_BattleCameraTargetY - g_BattleCameraYOffset - dist;
    pos.vx = g_BattleCameraTarget +
            ((rcos(angle + g_BattleCameraAngleStep) * g_BattleCameraOrbitRadius) >> 12);
    pos.vz = g_BattleCameraTargetZ + ((rsin(angle + g_BattleCameraAngleStep) * g_BattleCameraOrbitRadius) >> 12);
    // the ground height reuses lenPos: a fresh local puts it in the wrong register
    lenPos = BattleGetFloorHeightAtPosition(&pos, 0) - 0x100;
    if (lenPos < pos.vy) {
        pos.vy = lenPos;
    }
    pCamPos = (VECTOR*)&g_BattleCameraPos;
    pos.vx = (pos.vx - pCamPos->vx) / g_BattleCameraLerpFrames;
    pos.vy = (pos.vy - g_BattleCameraPosY) / g_BattleCameraLerpFrames;
    pos.vz = (pos.vz - g_BattleCameraPosZ) / g_BattleCameraLerpFrames;
    g_BattleCameraOrbitAngle = angle;
    BattleMovePointInsideRadius(pCamPos, &pos, 0x3D00);
    g_BattleCameraLerpFrames = BATTLE_CAMERA_LERP_FRAMES;
    // volatile: without it sched2 raises the g_BattleCameraPosY load above this store
    *(volatile s32*)&pCamPos->vx += pos.vx;
    g_BattleCameraPosY += pos.vy;
    g_BattleCameraPosZ += pos.vz;
}

void BattleGetCameraPoints(Unk16* p0, Unk16* p1) {
    *p0 = *(Unk16*)&g_BattleCameraPos;
    *p1 = *(Unk16*)&g_BattleCameraTarget;
}

void BattleClearScreen(s32 arg) {
    RECT rect;

    rect.x = 0;
    rect.y = 0;
    rect.w = arg ? 0x280 : 0x140;
    rect.h = 0x1E0;
    ClearImage(&rect, 0, 0, 0);
    DrawSync(0);
}

void BattleSetScreenMode(s32 mode) {
    s16* pA;
    s16* pB;
    s16 vA;
    s16 vB;

    g_BattleScreenMode = mode;
    switch (mode) {
    case 3:
        BattleSetScreenTileBlack();
        break;
    case 4:
        BattleSaveActorVectors(g_BattleTransitionFrames);
        break;
    case 8:
        func_8007AC3C();
        break;
    case 6:
        pA = &g_BattleCombatant1DownCount;
        pB = &g_BattleCombatant0DownCount;
        vA = *pA;
        vB = *pB;
        BattleInitRenderScene(vA < vB ? pB - 0x79 : pA - 0x79);
        break;
    }
}

s32 BattleGetScreenMode(void) {
    return g_BattleSavedScreenMode;
}

// Writes the battle timer's minutes:seconds:hundredths into buf and draws it;
// the two remainders are taken off the frame count once each.
void BattleDrawElapsedTime(void) {
    char buf[0x20];
    s32 n;
    s32 rem;

    n = g_BattleElapsedFrames;
    rem = n % 1800;
    Sprintf(buf, g_StrBattleElapsedTimeFormat, n / 1800, rem / 30, (n % 30) * 99 / 30);
    BattleDrawTextLeft(buf);
}

void BattleFadeActorLights(Misc9LightGroup* pTable, Battler9* pActor, s32 idx) {
    Misc9Node* pOwner;
    u8 alpha;
    u8 next;
    s32 a;
    s32 tmp[8]; // unused: an aggregate local is what gives the original's frame size

    alpha = pActor->lightAlpha;
    a = alpha;
    pOwner = pTable->pNodes[idx];
    if (a != 0) {
        next = alpha - 0x18;
        pActor->lightAlpha = next;
        if (a < next) {
            pActor->lightAlpha = 0;
        }
    }
    if (pActor->stateFlags & 0x20) {
        pOwner->pLight->r = (pActor->pTarget->effectColorR * a) / 16;
        pOwner->pLight->g = (pActor->pTarget->effectColorG * a) / 16;
        pOwner->pLight->b = (pActor->pTarget->effectColorB * a) / 16;
    } else {
        pOwner->pLight->r = a << 4;
        pOwner->pLight->g = a << 3;
        pOwner->pLight->b = 0;
    }
    if (g_BattleStageLightRow[0x10] != 0) {
        pOwner->pLight->r /= 2;
        pOwner->pLight->g /= 2;
        pOwner->pLight->b /= 2;
    }
    pOwner->pLight->dirX = pActor->x;
    pOwner->pLight->dirY = pActor->y;
    pOwner->pLight->dirZ = pActor->z;
    pOwner->pLight->dirX -= pActor->pTarget->pos[0];
    pOwner->pLight->dirY -= pActor->pTarget->pos[1];
    pOwner->pLight->dirZ -= pActor->pTarget->pos[2];
    func_80030A30(idx, pOwner->pLight);
    pOwner = pTable->pNodes[2];
    pOwner->pLight->r = pOwner->pLight->g = pOwner->pLight->b = 0;
    pOwner->pLight->dirX = pActor->x;
    pOwner->pLight->dirY = pActor->y;
    pOwner->pLight->dirZ = pActor->z;
    pOwner->pLight->dirX -= g_BattleCameraPos;
    pOwner->pLight->dirY -= g_BattleCameraPosY;
    pOwner->pLight->dirZ -= g_BattleCameraPosZ;
    func_80030A30(2, pOwner->pLight);
}

// The camera/floor phase of the battle frame: the camera matrix and the two HUD
// frames, the light group's node matrices, the reference matrix the shadows and
// the effect prims are drawn with, the floor outline and the shaded spans.
s32 BattleRenderCameraFrame(Misc9LightGroup* pGroup) {
    MATRIX mtx; // the base matrix translated to the view origin
    MATRIX mat; // the light group's own node matrix
    VECTOR pad; // unused: the original's frame carries this slot after the two matrices
    Misc9DisplayWork* pWork;
    Misc9Node* pNodeA;
    Misc9Node* pNodeB;
    Poly9* pLight;
    VECTOR* pCamPos;

    pWork = pGroup->pWork;
    BattleClearOrderingTable(pWork);
    BattleSetMatrixFromPoints(pGroup, (VECTOR*)&g_BattleCameraPos, (VECTOR*)&g_BattleCameraTarget);
    // The test chain jumps forward to the two shared bodies; as an if/else chain gcc
    // inlines the first arm and the block comes out in another order.
    if (g_BattleSavedScreenMode == 4) {
        goto checkMode;
    }
    if (g_BattleMode == 4) {
        goto drawHud;
    }
    if (g_BattleSavedScreenMode == 8) {
        goto checkMode;
    }
    BattleQueueGaugePrims(&g_BattleActor1, &g_BattleActor0);
    goto done;
checkMode:
    if (g_BattleMode != 4) {
        goto done;
    }
drawHud:
    BattleDrawFrameHud(&g_BattleActor1);
    if (g_BattleCameraDebugEnabled != 0) {
        BattleDrawFrameHud(&g_BattleActor0);
    }
done:
    BattleDrawTimedSpritePrims(g_BattleRootOrderingTable);
    BattleDrawTextPrims(g_BattleRootOrderingTable);
    mat = pGroup->pNode0->localMatrix;
    pNodeA = g_BattleActor0Node;
    pNodeB = g_BattleActor1Node;
    pNodeA->localMatrix = mat;
    pNodeB->localMatrix = pNodeA->localMatrix;
    pLight = pGroup->pNodes[0]->pLight;
    pLight->b = 0x800;
    pLight->g = 0x800;
    pLight->r = 0x800;
    func_80030A30(0, pGroup->pNodes[0]->pLight);
    gte_SetBackColor(g_BattleBackColorA, g_BattleBackColorG, g_BattleBackColorB);
    BattleFadeActorLights(pGroup, &g_BattleActor1, 1);
    BattleUpdateNodeTree(g_BattleEffectPool);
    BattleFadeActorLights(pGroup, &g_BattleActor0, 1);
    BattleUpdateNodeTree(g_BattleEffectNode);
    gte_SetRotTransMatrix_gpr(&mat);
    BattleUpdateSoundSlotPositions();
    BattleEffectQueueMatrixPrims(&mat, pWork->pOt[g_BattleDrawBufferIndex]);
    gte_SetRotTransMatrix_gpr(&mat);
    BattleEffectSetMatrix(pWork->pOt[g_BattleDrawBufferIndex], &pGroup->pNode0->localMatrix);
    mtx = g_BattleBaseMatrix;
    mtx.t[1] = -g_BattleViewOriginY;
    CompMatrix(&mat, &mtx, &mtx);
    gte_SetRotTransMatrix_gpr(&mtx);
    BattleSetStageFogColors();
    pCamPos = (VECTOR*)&g_BattleCameraPos;
    BattleDrawViewOriginRing(pWork->pOt[g_BattleDrawBufferIndex], pCamPos);
    BattleDrawActorShadow(&g_BattleActor1, pWork->pOt[g_BattleDrawBufferIndex], &mtx);
    BattleDrawActorShadow(&g_BattleActor0, pWork->pOt[g_BattleDrawBufferIndex], &mtx);
    gte_SetRotTransMatrix_gpr(&mtx);
    BattleClearSpanTables();
    if (g_BattleFloorOutlineArc != 0) {
        BattleDrawArcOutline(pCamPos->vx, g_BattleCameraPosZ,
                             ratan2(pCamPos->vx - g_BattleCameraTarget, g_BattleCameraPosZ - g_BattleCameraTargetZ));
    } else {
        BattleDrawHexagonOutline(pCamPos->vx, g_BattleCameraPosZ,
                                 ratan2(pCamPos->vx - g_BattleCameraTarget, g_BattleCameraPosZ - g_BattleCameraTargetZ));
    }
    BattleRenderShadedSpans(pWork->pOt[g_BattleDrawBufferIndex], g_BattleCameraTarget, g_BattleCameraTargetZ);
    BattleAttachActorPrims(pWork);
    BattleAddSharedPrim();
    func_80031678(g_BattleRootOrderingTable, &D_80095580[g_BattleDrawBufferIndex]);
    return 0;
}

s32 BattleRenderLightGroupFrame(Misc9LightGroup* pGroup) {
    VECTOR padA[2]; // unused: the original's frame carries this slot before the matrix
    Misc9DisplayWork* pWork;
    MATRIX mat;
    s32 padB[4]; // unused: the original's frame carries this slot after the matrix
    Misc9Node* pNodeA;
    Misc9Node* pNodeB;
    Poly9* pLight;

    pWork = pGroup->pWork;
    BattleClearOrderingTable(pWork);
    BattleSetMatrixFromPoints(pGroup, (VECTOR*)&g_BattleCameraPos, (VECTOR*)&g_BattleCameraTarget);
    BattleResetTextPrimQueue();
    mat = pGroup->pNode0->localMatrix;
    pNodeA = g_BattleActor0Node;
    pNodeB = g_BattleActor1Node;
    pNodeA->localMatrix = mat;
    pNodeB->localMatrix = pNodeA->localMatrix;
    BattleDisableBackColor();
    pLight = pGroup->pNodes[0]->pLight;
    pLight->b = 0x800;
    pLight->g = 0x800;
    pLight->r = 0x800;
    func_80030A30(0, pGroup->pNodes[0]->pLight);
    gte_SetBackColor(g_BattleBackColorA, g_BattleBackColorG, g_BattleBackColorB);
    BattleFadeActorLights(pGroup, &g_BattleActor1, 1);
    BattleUpdateNodeTree(g_BattleEffectPool);
    BattleFadeActorLights(pGroup, &g_BattleActor0, 1);
    BattleUpdateNodeTree(g_BattleEffectNode);
    gte_SetRotMatrix_gpr(&mat);
    BattleEffectSetMatrix(pWork->pOt[g_BattleDrawBufferIndex], &pGroup->pNode0->localMatrix);
    // load-bearing shape: two identical arms that gcc cross-jumps into retail's
    // single copy. Written as one straight-line pair the same two calls get pWork
    // and &mat the other way round (99.66%), so the arms must stay duplicated.
    do {
        if (pGroup) {
            BattleEffectQueuePrims(pWork->pOt[g_BattleDrawBufferIndex]);
            BattleAttachActorPrims(pWork);
        } else {
            BattleEffectQueuePrims(pWork->pOt[g_BattleDrawBufferIndex]);
            BattleAttachActorPrims(pWork);
        }
    } while (0);
    BattleAddSharedPrim();
    AddPrim(g_BattleRootOrderingTable, &D_80095648[g_BattleDrawBufferIndex]);
    AddPrim(g_BattleRootOrderingTable, &D_800955D8[g_BattleDrawBufferIndex]);
    AddPrim(g_BattleRootOrderingTable, &D_80095648[g_BattleDrawBufferIndex - 2]);
    AddPrim(g_BattleRootOrderingTable, &D_80095648[g_BattleDrawBufferIndex - 4]);
    AddPrim(g_BattleRootOrderingTable, &D_800955D8[g_BattleDrawBufferIndex - 2]);
    return 0;
}

s32 BattleClearAndQueueActorPrims(Battler9* p) {
    Misc9DisplayWork* pWork;

    pWork = p->pDisplayWork;
    BattleClearOrderingTable(pWork);
    // the table's first word is its terminator, the prims start after it
    BattleDrawTextPrims(&pWork->pOt[g_BattleDrawBufferIndex][1]);
    BattleAttachActorPrims(pWork);
    BattleAddSharedPrim();
    return 0;
}

s32 func_80084A40(Battler9* p) {
    return BattleClearOrderingTable(p->pDisplayWork);
}
