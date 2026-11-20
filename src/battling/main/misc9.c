#include "common.h"
#define BATTLE_SPOKE_COUNT 32 // spokes of the ring BattleDrawViewOriginRing draws
#define BATTLE_SPOKE_ANGLE_STEP 0x10 // angle increment per spoke, 4096-step turn
#define BATTLE_SPOKE_RADIUS 0x3F80 // ring radius, also the offset subtracted from the view origin
#define BATTLE_OT_DEPTH_MAX 0x1C00 // depth limit before the prim is queued
#define BATTLE_ANGLE_QUARTER_TURN 0x400 // a quarter turn in the 4096-step angle space
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
extern u8 g_BattleCameraLerpFrames;
extern s32 g_BattleCameraAngleStep;

extern s32 g_BattleScreenMode;
extern s32 g_BattleCameraSpinStep;
extern s32 g_BattleTransitionFrames;
extern s16 g_BattleCombatant0DownCount;
extern s16 g_BattleCombatant1DownCount;

extern int rand(void);

extern s32 g_BattleElapsedFrames;
extern char D_800701B0[];
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

typedef struct {
    /* 0x0 */ u32 unk0;
    /* 0x4 */ Poly9* pLight;
} PolyOwner;

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

typedef struct {
    /* 0x0 */ u8 pad0[0x8];
    /* 0x8 */ PolyOwner* pLightOwners[3];
} PolyTable;

extern u8* g_BattleStageLightRow;

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
extern void func_80087698(s32 x0, s32 y0, s32 x1, s32 y1);

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
        func_80087698(x, y, px, py);
        x = px;
        y = py;
        ang += 0x100;
    }
    px = bx + ((rcos(arg2 - 0x280) * 10) >> 12);
    py = by + ((rsin(arg2 - 0x280) * 10) >> 12);
    func_80087698(x, y, px, py);
    func_80087698(x0, y0, px, py);
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
        func_80087698(x, y, px, py);
        x = px;
        y = py;
        ang += 0x75;
    }
    px = bx + ((rcos(arg2 - 0x100) << 4) >> 12);
    bx = rsin(arg2 - 0x100);
    py = by + ((bx << 4) >> 12);
    func_80087698(x, y, px, py);
    func_80087698(x0, y0, px, py);
}

void BattleCopyPlaneNormal(SVECTOR* p) {
    *p = g_BattlePlaneNormal;
}

extern void MathTriangleNormal(SVECTOR* pVertex1, SVECTOR* pVertex2, SVECTOR* pVertex3, SVECTOR* pNormal);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_80082488);
/* PARK 99.00% (1016 bytes, the original's exact size). Two things differ.
   1. The mask/index registers: retail materialises the 0xFFFFFF00 mask in a0
   (addiu a0,zero,-256 hoisted above the loads), keeps pPos->vx in t2 and
   pPos->vz in t1 and lets the mz AND overwrite the mask register; this body gets
   the mask in a3 and the coordinates in v0/a0 (the same six instructions, four
   register names). Six source shapes all measure identically here, including
   dropping the mask local (which is what a permuter run's best candidate did, but
   it also made the mz unmasked, so it is not shippable), so this is an allocator
   tie, not a spelling question.
   2. g_BattlePlaneNormal.vy/.vz: retail's object names the addresses D_8009276A /
   D_8009276C (0x80092768 + 2 and + 4, fields of the SVECTOR at that address) while
   the typed spelling relocates the whole symbol with an addend. The linked bytes
   are identical - this is the second exception in the brief, and it is why
   diff_func.sh can never print MATCH here.
   The addu that forms the cell pointer is index-first in retail
   (addu t0,scaledIdx,base): the array form and 'idx + g_BattleGridCells' both come
   out base-first, and only the explicit
   (s32 *)((u32)((row << 7) + col) * 4 + (u32)g_BattleGridCells) spelling
   reproduces retail - worth 1 instruction of operand order.
   The floor of a grid cell is two triangles cut by the diagonal from the near
   corner to the far one; this returns the height of that plane under pPos. Four
   stack tricks reproduce the frame: the cell corners are four SVECTORs at 0x18,
   the triangle is a second SVECTOR[3] at 0x40, and the pad arrays are
   addressable-but-unused locals the original has (0x10 and 0x58 are never
   written) - gcc 2.7.2 only allocates space for an unused aggregate, not for an
   unused scalar. The 12-byte VECTOR view of the long dot product is taken at &v3,
   the dead fourth corner, which is where the original stores it too.
   The height patch is a two-case switch with fall-through: retail's arms are
   'h += 0xC0' falling into 'h += 0x40' (an if/else-if is cross-jumped and folds
   the two adds into one +0x100, which shrinks the body), and each check re-forms
   g_BattleGridCells[idx] from scratch, so they are written as full array accesses
   rather than through the pCell pointer.

// The floor of a grid cell is two triangles cut by the diagonal from the
// near corner to the far one; this returns the height of that plane under pPos.
s32 func_80082488(VECTOR* pPos, s32 fixHeights) {
    SVECTOR padTop[1];
    SVECTOR v0;
    SVECTOR v1;
    SVECTOR v2;
    SVECTOR v3;
    SVECTOR padMid[1];
    SVECTOR tri[3];
    SVECTOR padBottom[1];
    VECTOR* pRef;
    s32* pCell;
    s32 row;
    s32 col;
    s32 mx;
    s32 mz;
    s32 x;
    s32 z;
    s32 side;

    x = pPos->vx;
    z = pPos->vz;
    mx = x & 0xFFFFFF00;
    col = x >> 8;
    mz = z & 0xFFFFFF00;
    row = z >> 8;
    pCell = (s32 *)((u32)((row << 7) + col) * 4 + (u32)g_BattleGridCells);

    v0.vx = mx;
    v0.vy = *(u16*)&pCell[0];
    v0.vz = mz;
    v1.vx = mx + 0x100;
    v1.vy = *(u16*)&pCell[0x81];
    v1.vz = mz + 0x100;
    v2.vx = mx + 0x100;
    v2.vy = *(u16*)&pCell[1];
    v2.vz = mz;
    v3.vx = mx;
    v3.vy = *(u16*)&pCell[0x80];
    v3.vz = mz + 0x100;

    if (fixHeights) {
        switch (g_BattleGridCells[(row << 7) + col] & 0x3000000) {
        case 0x1000000:
            v0.vy = v0.vy + 0xC0;
        case 0x3000000:
            v0.vy = v0.vy + 0x40;
            break;
        }
        switch (g_BattleGridCells[(row << 7) + col + 0x81] & 0x3000000) {
        case 0x1000000:
            v1.vy = v1.vy + 0xC0;
        case 0x3000000:
            v1.vy = v1.vy + 0x40;
            break;
        }
        switch (g_BattleGridCells[(row << 7) + col + 1] & 0x3000000) {
        case 0x1000000:
            v2.vy = v2.vy + 0xC0;
        case 0x3000000:
            v2.vy = v2.vy + 0x40;
            break;
        }
        switch (g_BattleGridCells[(row << 7) + col + 0x80] & 0x3000000) {
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
*/

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
    s32 tmp[12];
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

extern u8 g_BattleStageLightIndex;
extern u8 g_BattleStageLightTable[];
extern s32 g_BattleBackColorA;
extern s32 g_BattleBackColorG;
extern s32 g_BattleBackColorB;
extern u8 g_BattleFogColors;
extern u8 D_80095585;
extern u8 D_80095586;
extern u16 D_8009558C;
extern u8 D_8009558E;
extern u16 D_80095594;
extern u8 D_80095596;
extern u16 D_8009559C;
extern u8 D_8009559E;
extern u8 D_800955A8;
extern u8 D_800955A9;
extern u8 D_800955AA;
extern u16 D_800955B0;
extern u8 D_800955B2;
extern u16 D_800955B8;
extern u8 D_800955BA;
extern u16 D_800955C0;
extern u8 D_800955C2;
extern u8 D_8009A1C4;
extern u8 D_8009A1C5;
extern u8 D_8009A1C6;
extern u8 D_8009A2BC;
extern u8 D_8009A2BD;
extern u8 D_8009A2BE;
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
    D_80095585 = nearG;
    D_800955A9 = nearG;
    nearRG = nearR | (nearG << 8);
    g_BattleFogColors = nearR;
    D_800955A8 = nearR;
    farRG = farR | (farG << 8);
    D_80095586 = nearB;
    D_800955AA = nearB;
    D_8009558C = nearRG;
    D_8009558E = nearB;
    D_800955B0 = nearRG;
    D_800955B2 = nearB;
    D_80095594 = farRG;
    D_80095596 = farB;
    D_800955B8 = farRG;
    D_800955BA = farB;
    D_8009559C = farRG;
    D_8009559E = farB;
    D_800955C0 = farRG;
    D_800955C2 = farB;
    D_8009A1C4 = farR;
    D_8009A1C5 = farG;
    D_8009A1C6 = farB;
    D_8009A2BC = farR;
    D_8009A2BD = farG;
    D_8009A2BE = farB;
    SetFogNearFar(0x800, 0x1800, 0xC0);
    g_NearColor = (g_NearColor & 0xFFFFFF) | 0x28000000;
    gte_ldrgb(&g_NearColor);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_80082C4C);

/* best 68.73%: structure recovered but three allocator/CSE differences remain.
   - retail uses setPolyFT4 (tag length byte at offset 3, code 0x2C at 7) and
     the 64 quads of 0x28 bytes are POLY_FT4; the store order u0,u1,v0,u2,v1,
     u3,v2,clut,tpage,v3 is the original's.
   - D_800927A8 is read both as a byte (lbu, for v0/v2) and a halfword (lhu,
     for v1/v3) at the same address; the setup store into it is a sh. A union
     of u16/u8 members reproduces both loads without hoisting an address.
   - retail reloads the D_800927A8 address with a fresh lui per access, ours
     CSEs the eight uses into one callee-saved register (s0) - that is the
     blocker; and retail keeps &g_BattleDrawPrimPools in a callee-saved register across
     the two HeapAlloc calls and stores through it in the jal delay slot.
   - the CLUT loop wants an s32 -0x8000 local (or v0,v0,a0) and drop its
     counter in a2, ours picks a1; D_800927A8 needs a union of u16/u8
     members to get both loads.

/*
void func_80082C4C(u8* pArg) {
    TIM_IMAGE tim;
    POLY_FT4* p;
    u_short* pClut;
    s32 i;
    s32 mask;

    OpenTIM(*(s32*)(pArg + 0x6C));
    ReadTIM(&tim);
    mask = -0x8000;
    pClut = (u_short*)tim.caddr;
    for (i = 0; i < 0x100; i++) {
        *pClut++ |= mask;
    }
    LoadImage(tim.crect, tim.caddr);
    LoadImage(tim.prect, tim.paddr);
    D_800927A0 = GetClut(tim.crect->x, tim.crect->y);
    D_800927A4 = GetTPage(1, 0, tim.prect->x, tim.prect->y);
    D_800927A8.w = *(u8*)&tim.prect->y;
    g_BattleDrawPrimPools = HeapAlloc(0xA00, 0);
    D_8009278C = HeapAlloc(0xA00, 0);
    p = (POLY_FT4*)g_BattleDrawPrimPools;
    for (i = 0; i < 0x40; i += 2) {
        setPolyFT4(p);
        p->u0 = 0x7F;
        p->u1 = 0x7F;
        p->v0 = D_800927A8.b + 0x3F;
        p->u2 = 0x3F;
        p->v1 = D_800927A8.w;
        p->u3 = 0x3F;
        p->v2 = D_800927A8.b + 0x3F;
        p->clut = D_800927A0;
        p->tpage = D_800927A4;
        p->v3 = D_800927A8.w;
        p++;
        setPolyFT4(p);
        p->u0 = 0x3F;
        p->u1 = 0x3F;
        p->v0 = D_800927A8.b + 0x3F;
        p->u2 = 0;
        p->v1 = D_800927A8.w;
        p->u3 = 0;
        p->v2 = D_800927A8.b + 0x3F;
        p->clut = D_800927A0;
        p->tpage = D_800927A4;
        p->v3 = D_800927A8.w;
        p++;
    }
    func_800732AC(D_8009278C, g_BattleDrawPrimPools, 0xA00);
}
*/

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
extern s32 func_80082488(VECTOR* pPos, s32 fixHeights);

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
    floorY = func_80082488(pCamPos, 0);
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
extern s32 D_80098680;
extern s32 D_80098684;
extern s32 D_8009277C;
extern u8 g_BattleCameraSpinDirection;
extern s32 BattleVectorDistance(s32* pA, s32* pB);
extern s32 BattleVectorLength2D(s32* p);

// Mirrors the camera on the combatants' midpoint and orbits it to whichever of
// the two candidate points on the ring is nearer, moving a fraction of the way
// there each frame (the ring shrinks as the pair closes).
void func_80083738(VECTOR* pA, VECTOR* pB) {
    VECTOR pos;
    s32 radius;
    VECTOR alt;
    VECTOR pad0[2]; // unused: the original's frame carries two more 0x10 slots
    VECTOR* pCamPos;
    long* pBvz;
    s32* pTargetX;
    s32 angle;
    s32 dist;
    s32 lenPos;
    s32 lenAlt;

    angle = ratan2(pA->vx - pB->vx, pA->vz - pB->vz);
    dist = BattleVectorDistance((s32*)pA, (s32*)pB);
    pTargetX = &g_BattleCameraTarget;
    *pTargetX = (pA->vx + pB->vx) / 2;
    radius = dist * 2 / 3 + 0xC0;
    pBvz = &pB->vz;
    D_80098680 = (pA->vy + pB->vy) / 2 - 0xA0;
    D_80098684 = (pA->vz + *pBvz) / 2;
    g_BattleCameraOrbitRadius = radius;
    pos.vx = *pTargetX + ((rcos(angle - BATTLE_ANGLE_QUARTER_TURN) * g_BattleCameraOrbitRadius) >> 12);
    pos.vz = D_80098684 + ((rsin(angle - BATTLE_ANGLE_QUARTER_TURN) * g_BattleCameraOrbitRadius) >> 12);
    alt.vx = *pTargetX + ((rcos(angle + BATTLE_ANGLE_QUARTER_TURN) * g_BattleCameraOrbitRadius) >> 12);
    alt.vz = D_80098684 + ((rsin(angle + BATTLE_ANGLE_QUARTER_TURN) * g_BattleCameraOrbitRadius) >> 12);
    pos.vx -= g_BattleCameraPos;
    pos.vy -= g_BattleCameraPosY;
    pos.vz -= g_BattleCameraPosZ;
    alt.vx -= g_BattleCameraPos;
    alt.vy -= g_BattleCameraPosY;
    alt.vz -= g_BattleCameraPosZ;
    lenPos = BattleVectorLength2D((s32*)&pos);
    lenAlt = BattleVectorLength2D((s32*)&alt);
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
    pos.vy = D_80098680 - g_BattleCameraYOffset - dist;
    pos.vx = g_BattleCameraTarget +
            ((rcos(angle + g_BattleCameraAngleStep) * g_BattleCameraOrbitRadius) >> 12);
    pos.vz = D_80098684 + ((rsin(angle + g_BattleCameraAngleStep) * g_BattleCameraOrbitRadius) >> 12);
    lenPos = func_80082488(&pos, 0) - 0x100;
    if (lenPos < pos.vy) {
        pos.vy = lenPos;
    }
    pCamPos = (VECTOR*)&g_BattleCameraPos;
    pos.vx = (pos.vx - pCamPos->vx) / g_BattleCameraLerpFrames;
    pos.vy = (pos.vy - g_BattleCameraPosY) / g_BattleCameraLerpFrames;
    pos.vz = (pos.vz - g_BattleCameraPosZ) / g_BattleCameraLerpFrames;
    D_8009277C = angle;
    BattleMovePointInsideRadius(pCamPos, &pos, 0x3D00);
    g_BattleCameraLerpFrames = 0x64;
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

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_80083CE8);

/* best 85.00%: instruction stream is insn-for-insn identical; gcc puts n in
   t0 and the 0x88888889 magic in a0 where the original has n in a0, the magic
   in a1 and an extra `move a2,t0` for n/1800, and gcc hoists the format
   address. Splitting the three values into locals gives byte-identical C
   output (also 85.00%).
void func_80083CE8(void) {
    char buf[0x20];
    s32 n;

    n = g_BattleElapsedFrames;
    Sprintf(buf, D_800701B0, n / 1800, (n % 1800) / 30, (n % 30) * 99 / 30);
    BattleDrawTextLeft(buf);
}
*/

void BattleFadeActorLights(PolyTable* pTable, Battler9* pActor, s32 idx) {
    PolyOwner* pOwner;
    u8 alpha;
    u8 next;
    s32 a;
    s32 tmp[8];

    alpha = pActor->lightAlpha;
    a = alpha;
    pOwner = pTable->pLightOwners[idx];
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
    pOwner = pTable->pLightOwners[2];
    pOwner->pLight->r = pOwner->pLight->g = pOwner->pLight->b = 0;
    pOwner->pLight->dirX = pActor->x;
    pOwner->pLight->dirY = pActor->y;
    pOwner->pLight->dirZ = pActor->z;
    pOwner->pLight->dirX -= g_BattleCameraPos;
    pOwner->pLight->dirY -= g_BattleCameraPosY;
    pOwner->pLight->dirZ -= g_BattleCameraPosZ;
    func_80030A30(2, pOwner->pLight);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_800840CC);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_800846A0);

s32 BattleClearAndQueueActorPrims(Battler9* p) {
    Misc9DisplayWork* pWork;

    pWork = p->pDisplayWork;
    BattleClearOrderingTable(pWork);
    BattleDrawTextPrims((u8*)pWork->pOt[g_BattleDrawBufferIndex] + 4);
    BattleAttachActorPrims(pWork);
    BattleAddSharedPrim();
    return 0;
}

s32 func_80084A40(Battler9* p) {
    return BattleClearOrderingTable(p->pDisplayWork);
}
