#include "common.h"
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
extern s32 g_BattleCameraPos;
extern s32 g_BattleCameraTarget;
extern s32 D_80098720;
extern s32 D_80098724;
extern s32 g_BattleCameraOrbitRadius;
extern s32 g_BattleCameraYOffset;
extern u8 g_BattleCameraLerpFrames;
extern s32 g_BattleCameraAngleStep;

extern s32 g_BattleScreenMode;
extern s32 g_BattleCameraSpinStep;
extern s32 g_BattleTransitionFrames;
extern s16 D_80097102;
extern s16 D_8009881E;

extern int rand(void);

extern s32 D_80092944;
extern char D_800701B0[];
extern void BattleDrawTextLeft(char* p);

typedef struct {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ s32 unk4;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s16 unkC;
    /* 0x0E */ s16 unkE;
    /* 0x10 */ s16 unk10;
} Poly9; // Size: unknown

typedef struct {
    /* 0x0 */ u32 unk0;
    /* 0x4 */ Poly9* unk4;
} PolyOwner;

typedef struct {
    /* 0x0000 */ s32 pos[3];
    /* 0x000C */ u8 pad0C[0x15C8];
    /* 0x15D4 */ u8 unk15D4;
    /* 0x15D5 */ u8 unk15D5;
    /* 0x15D6 */ u8 unk15D6;
} BattlerData;

typedef struct {
    /* 0x000 */ s32 unk0;
    /* 0x004 */ s32 unk4;
    /* 0x008 */ s32 unk8;
    /* 0x00C */ u8 pad0C[0xC8];
    /* 0x0D4 */ u32 unkD4;
    /* 0x0D8 */ BattlerData* unkD8;
    /* 0x0DC */ u8 padDC[0x83C];
    /* 0x918 */ u8 unk918;
} Battler9;

typedef struct {
    /* 0x0 */ u8 pad0[0x8];
    /* 0x8 */ PolyOwner* unk8[3];
} PolyTable;

extern u8* g_BattleStageLightRow;

extern s32 BattleClearOrderingTable();
extern void func_80030A30(s32 index, Poly9* pPoly);
extern s32 func_80083310();
extern void BattleSaveActorVectors(s32);
extern void func_8007AC3C(void);
extern void BattleInitRenderScene(s16* p);
extern void BattleSetScreenTileBlack(void);
extern void BattleDrawTextPrims(void* p);
extern void BattleAttachActorPrims(u8* p);
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
/* PARK 98.96% (0x3F8 bytes, the original's exact size - only the prologue's
   register choice for the mask/index and one addu operand order differ).
   The floor of a grid cell is two triangles cut by the diagonal from the near
   corner to the far one; this returns the height of that plane under pPos.
   Four stack tricks were needed to reproduce the frame: the cell corners are
   four SVECTORs at 0x18, the triangle is a second SVECTOR[3] at 0x40, and the
   three pad arrays are addressable-but-unused locals that the original has
   (0x10 and 0x58 are never written) - gcc 2.7.2 only allocates space for an
   unused aggregate, not for an unused scalar.  The 12-byte VECTOR view of the
   long dot product is taken at &v3, the dead fourth corner, which is where the
   original stores it too.
   The height patch is a two-case switch with fall-through: retail's arms are
   "h += 0xC0" falling into "h += 0x40" (cross-jumped if/else-if instead folds
   the two adds into one +0x100 and shrinks the body), and each check re-forms
   g_BattleGridCells[idx] from scratch, so they are written as full array
   accesses rather than through the pCell pointer.
   Remaining diff: retail holds pPos->vx in t2/pPos->vz in t1 and the 0xFFFFFF00
   mask in a0 (ours: v0/a0/a3) and forms the cell address as addu idx,base
   (ours: base,idx).  Readable as: mx/mz = coordinate & 0xFFFFFF00,
   col/row = coordinate >> 8, side = the 3x3 determinant that says which side of
   the diagonal the point is on, then dot products with g_BattlePlaneNormal. */
/*
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
    s32 side;

    mx = pPos->vx & 0xFFFFFF00;
    col = pPos->vx >> 8;
    mz = pPos->vz & 0xFFFFFF00;
    row = pPos->vz >> 8;
    pCell = &g_BattleGridCells[(row << 7) + col];

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
s16 BattleGetGridValueAtPosition(u8* p) {
    s32 tmp[12];
    s32 a;
    s32 b;
    s32 lo;
    s32 hi;

    a = *(u16*)p << 16;
    b = *(u16*)(p + 4) << 16;
    lo = a >> 24;
    hi = (b >> 24) << 7;

    return *(s16*)&g_BattleGridCells[hi + lo];
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
extern u8 D_80095584;
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
    D_80095584 = nearR;
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
     blocker; and retail keeps &D_80092788 in a callee-saved register across
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
    D_80092788 = HeapAlloc(0xA00, 0);
    D_8009278C = HeapAlloc(0xA00, 0);
    p = (POLY_FT4*)D_80092788;
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
    func_800732AC(D_8009278C, D_80092788, 0xA00);
}
*/

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_80082E60);

void BattleRandomizeCameraPosition(void) {
    s32 ang;
    s32 scale;

    scale = (rand() & 0x1FFF) + 0x800;
    ang = rand() % 0x600 + 0x500;
    g_BattleCameraPos = ((rcos(ang) * scale) >> 12) + 0x4000;
    D_80098724 = ((rsin(ang) * scale) >> 12) + 0x4000;
    D_80098720 = -((rand() & 0x7FF) + 0x400);
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

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_80083310);

void BattleRandomizeCameraSpin(void) {
    g_BattleCameraAngle = rand();
    g_BattleCameraSpinStep = rand() % 12 + 4;
    if (rand() & 1) {
        g_BattleCameraSpinStep = -g_BattleCameraSpinStep;
    }
    func_80083310(0);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_80083738);

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
        pA = &D_8009881E;
        pB = &D_80097102;
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

    n = D_80092944;
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

    alpha = pActor->unk918;
    a = alpha;
    pOwner = pTable->unk8[idx];
    if (a != 0) {
        next = alpha - 0x18;
        pActor->unk918 = next;
        if (a < next) {
            pActor->unk918 = 0;
        }
    }
    if (pActor->unkD4 & 0x20) {
        pOwner->unk4->unkC = (pActor->unkD8->unk15D4 * a) / 16;
        pOwner->unk4->unkE = (pActor->unkD8->unk15D5 * a) / 16;
        pOwner->unk4->unk10 = (pActor->unkD8->unk15D6 * a) / 16;
    } else {
        pOwner->unk4->unkC = a << 4;
        pOwner->unk4->unkE = a << 3;
        pOwner->unk4->unk10 = 0;
    }
    if (g_BattleStageLightRow[0x10] != 0) {
        pOwner->unk4->unkC /= 2;
        pOwner->unk4->unkE /= 2;
        pOwner->unk4->unk10 /= 2;
    }
    pOwner->unk4->unk0 = pActor->unk0;
    pOwner->unk4->unk4 = pActor->unk4;
    pOwner->unk4->unk8 = pActor->unk8;
    pOwner->unk4->unk0 -= pActor->unkD8->pos[0];
    pOwner->unk4->unk4 -= pActor->unkD8->pos[1];
    pOwner->unk4->unk8 -= pActor->unkD8->pos[2];
    func_80030A30(idx, pOwner->unk4);
    pOwner = pTable->unk8[2];
    pOwner->unk4->unkC = pOwner->unk4->unkE = pOwner->unk4->unk10 = 0;
    pOwner->unk4->unk0 = pActor->unk0;
    pOwner->unk4->unk4 = pActor->unk4;
    pOwner->unk4->unk8 = pActor->unk8;
    pOwner->unk4->unk0 -= g_BattleCameraPos;
    pOwner->unk4->unk4 -= D_80098720;
    pOwner->unk4->unk8 -= D_80098724;
    func_80030A30(2, pOwner->unk4);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_800840CC);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc9", func_800846A0);

s32 BattleClearAndQueueActorPrims(u8* p) {
    u8* p2;
    void* p3;

    p2 = *(u8**)(p + 0x284);
    BattleClearOrderingTable(p2);
    p3 = *(void**)(p2 + g_BattleDrawBufferIndex * 4 + 4);
    BattleDrawTextPrims(p3 + 4);
    BattleAttachActorPrims(p2);
    BattleAddSharedPrim();
    return 0;
}

s32 func_80084A40(u8* p) {
    return BattleClearOrderingTable(*(s32*)(p + 0x284));
}
