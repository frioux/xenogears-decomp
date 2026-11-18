#include "common.h"

// Same sentinel as include/main/game.h's WORLD_MAP_AREA_NONE (this unit does not include
// game.h); duplicated here so the name is used, not the literal.
#define WORLD_MAP_AREA_NONE (-1)


// 12.4 fixed point world map position; only x (0x0) and z (0x8) are read here.
typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 unk4;
    /* 0x8 */ s32 z;
} WorldMapPos; // Size: 0xC

// One slot of the world map menu list; stride 0x80, only 0x20 and 0x50 are read.
typedef struct {
    /* 0x00 */ u8 unk0[0x20];
    /* 0x20 */ s16 unk20;
    /* 0x22 */ u8 unk22[0x2E];
    /* 0x50 */ s32 unk50;
} WorldMapMenuSlot; // Size: 0x80

extern void SystemStringFree(void* pString);
extern s32 WorldMapGetTileValue(WorldMapPos* p);
extern u8* WorldMapGetTileEntry(s32 x, s32 z);
extern long SquareRoot0(long a);
extern long ratan2(long y, long x);
extern int rcos(int a);
extern int rsin(int a);

extern u8 g_WorldMapAreaNameString[];
extern u8 g_WorldMapMode4AreaNameString[];
extern u8 g_WorldMapTileKindTable[];
extern s32 D_8009B264[];
extern s32 D_8009B26C[];
extern s32 g_WorldMapTileCellPointers[];
extern s32 g_WorldMapTileShapeHalfAxis[];
extern s32 D_8009B36C[];
extern s32 g_WorldMapTileShapeHalfOrigin[];
extern s32 D_8009B46C[];
extern s32 g_WorldMapTileColumns;
extern s32 g_WorldMapTileRows;
extern s16 g_WorldMapAreaId;
extern u16 g_WorldMapAreaNamePanelFlags;
extern u8 D_8009D500;
extern s32 func_80032F54(void* pString, s16 a, s16 b, s16 c, s16 d, s16 e, s16 f);
extern void func_80093740(WorldMapPos* p, s32 x, s32 z);
extern s32 WorldMapInterpolatePlaneHeight(WorldMapPos* p0, WorldMapPos* p1, WorldMapPos* p2);
extern void SystemStringResetIfIdle(void* pString);
extern void SystemStringPushBuffer(void* pString, void* pData);
extern void func_80034888(void* pString, u32 unk, u32 arg);
extern u8* GetStringEntry(u8* pData, s32 index);
extern u8* g_WorldMapSlots;
extern u32* g_WorldMapRenderContext;
extern u8* g_WorldMapStringTable;
extern s32 g_WorldMapPrimPacketIndex;
extern s16 g_WorldMapMode4AreaId;
extern u8 D_8009D2B8[];
extern u8 D_8009BDCC;
extern u16 D_8009D2C0;
extern u16 D_8009D2C2;
extern u16 D_8009D2C8;
extern u16 D_8009D2CA;
extern u16 D_8009D2D0;
extern u16 D_8009D2D2;
extern u16 D_8009D2D8;
extern u16 D_8009D2DA;
extern u8 D_8009D2BB;
extern u8 D_8009D2BC;
extern u8 D_8009D2BD;
extern u8 D_8009D2BE;
extern u8 D_8009D2BF;
extern u8 D_8009D2C4;
extern u8 D_8009D2C5;
extern u8 D_8009D2CC;
extern u8 D_8009D2CD;
extern u8 D_8009D2D4;
extern u8 D_8009D2D5;
extern u8 D_8009D2DC;
extern u8 D_8009D2DD;
extern u16 D_8009D2C6;
extern u16 D_8009D2CE;
extern u16 GetTPage(int tp, int abr, int x, int y);
extern u16 GetClut(int x, int y);
extern void SetSemiTrans(void* p, int abe);

// Untyped 0x28-byte primitive blob at D_8009D2B8; only the copy is visible.
typedef struct {
    /* 0x00 */ u32 unk0[0xA];
} WorldMapPrim; // Size: 0x28

s32 WorldMapInitAreaNameString(void) {
    u8* pBuffer;

    pBuffer = g_WorldMapAreaNameString;
    g_WorldMapAreaId = WORLD_MAP_AREA_NONE;
    func_80032F54(pBuffer, 0x3C0, 0x180, 0xA0, 0x78, 0x20, 1);
    D_8009D500 = 8;
    g_WorldMapAreaNamePanelFlags |= 2;
    SystemStringResetIfIdle(pBuffer);
    return 1;
}

s32 WorldMapUpdateAreaNameString(s32 index) {
    WorldMapMenuSlot* pSlot;

    pSlot = (WorldMapMenuSlot*)(g_WorldMapSlots + index * 0x80);
    switch (pSlot->unk20) {
    case 0:
        if (g_WorldMapAreaId != WORLD_MAP_AREA_NONE) {
            SystemStringResetIfIdle(g_WorldMapAreaNameString);
            SystemStringPushBuffer(g_WorldMapAreaNameString, GetStringEntry(g_WorldMapStringTable, g_WorldMapAreaId));
            pSlot->unk20 = 1;
            pSlot->unk50 = g_WorldMapAreaId;
        }
        break;
    case 1:
        if (g_WorldMapAreaId == WORLD_MAP_AREA_NONE) {
            SystemStringResetIfIdle(g_WorldMapAreaNameString);
            pSlot->unk20 = 0;
        } else if (g_WorldMapAreaId != pSlot->unk50) {
            SystemStringResetIfIdle(g_WorldMapAreaNameString);
            SystemStringPushBuffer(g_WorldMapAreaNameString, GetStringEntry(g_WorldMapStringTable, g_WorldMapAreaId));
            pSlot->unk50 = g_WorldMapAreaId;
        }
        break;
    }
    func_80034888(g_WorldMapAreaNameString, g_WorldMapRenderContext[0x1C], (u32)g_WorldMapPrimPacketIndex);
    return 1;
}

void WorldMapFreeAreaNameString(void) {
    SystemStringFree(&g_WorldMapAreaNameString[0]);
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc13", func_80092DF8);
/* best: 91.17% (480 bytes vs 472). Instruction stream and the 0x28-byte copy
   loop are identical; the only delta is that retail hoists each `li` of a
   repeated constant one statement earlier than gcc does here (e.g. `li v1,296`
   lands before the D_8009D2C2 store). Store order below is retail's.

s32 func_80092DF8(void) {
    u16 flags;

    g_WorldMapMode4AreaId = WORLD_MAP_AREA_NONE;
    func_80032F54(g_WorldMapMode4AreaNameString, 0x3C0, 0x18D, 0xA0, 0x78, 0x20, 4);
    flags = g_WorldMapAreaNamePanelFlags;
    D_8009BDCC = 8;
    g_WorldMapAreaNamePanelFlags = flags | 2;
    SystemStringResetIfIdle(g_WorldMapMode4AreaNameString);
    D_8009D2BB = 9;
    D_8009D2BF = 0x2C;
    D_8009D2C2 = 0x70;
    D_8009D2CA = 0x70;
    D_8009D2C8 = 0x128;
    D_8009D2D8 = 0x128;
    D_8009D2C0 = 0x98;
    D_8009D2D0 = 0x98;
    D_8009D2D2 = 0xB4;
    D_8009D2DA = 0xB4;
    D_8009D2C4 = 0x80;
    D_8009D2C5 = 0;
    D_8009D2CC = 0xFF;
    D_8009D2CD = 0;
    D_8009D2D4 = 0x80;
    D_8009D2D5 = 0x3F;
    D_8009D2DC = 0xFF;
    D_8009D2DD = 0x3F;
    D_8009D2BC = 0x80;
    D_8009D2BD = 0x80;
    D_8009D2BE = 0x80;
    D_8009D2CE = GetTPage(0, 0, 0x380, 0x100);
    D_8009D2C6 = GetClut(0x130, 0x1E4);
    SetSemiTrans(D_8009D2B8, 1);
    ((WorldMapPrim*)D_8009D2B8)[1] = ((WorldMapPrim*)D_8009D2B8)[0];
    return 1;
}
*/

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc13", func_80092FD8);
/* best: 85.92% (484 bytes vs 472). Everything up to the display-list linking
   matches; only the constant/address materialisation order inside it differs
   (retail loads `la D_8009D2B8`, the 0xFF000000 mask, g_WorldMapPrimPacketIndex and
   g_WorldMapRenderContext before the *40 multiply, ours interleaves them).

s32 func_80092FD8(s32 index) {
    WorldMapMenuSlot* pSlot;
    u32* pPrim;

    pSlot = (WorldMapMenuSlot*)(g_WorldMapSlots + index * 0x80);
    switch (pSlot->unk20) {
    case 0:
        if (g_WorldMapMode4AreaId != -1) {
            SystemStringResetIfIdle(g_WorldMapMode4AreaNameString);
            SystemStringPushBuffer(g_WorldMapMode4AreaNameString, GetStringEntry(g_WorldMapStringTable, g_WorldMapMode4AreaId));
            pSlot->unk20 = 1;
            pSlot->unk50 = g_WorldMapMode4AreaId;
        }
        break;
    case 1:
        if (g_WorldMapMode4AreaId == -1) {
            SystemStringResetIfIdle(g_WorldMapMode4AreaNameString);
            pSlot->unk20 = 0;
        } else if (g_WorldMapMode4AreaId != pSlot->unk50) {
            SystemStringResetIfIdle(g_WorldMapMode4AreaNameString);
            SystemStringPushBuffer(g_WorldMapMode4AreaNameString, GetStringEntry(g_WorldMapStringTable, g_WorldMapMode4AreaId));
            pSlot->unk50 = g_WorldMapMode4AreaId;
        }
        break;
    }
    func_80034888(g_WorldMapMode4AreaNameString, g_WorldMapRenderContext[0x1C], (u32)g_WorldMapPrimPacketIndex);
    if (pSlot->unk20 == 1) {
        pPrim = (u32*)(D_8009D2B8 + g_WorldMapPrimPacketIndex * 0x28);
        pPrim[0] = (pPrim[0] & 0xFF000000) | (*(u32*)g_WorldMapRenderContext[0x1C] & 0xFFFFFF);
        *(u32*)g_WorldMapRenderContext[0x1C] = (*(u32*)g_WorldMapRenderContext[0x1C] & 0xFF000000) | ((u32)pPrim & 0xFFFFFF);
    }
    return 1;
}
*/

void WorldMapFreeMode4AreaNameString(void) {
    SystemStringFree(&g_WorldMapMode4AreaNameString[0]);
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc13", func_800931D8);
/* best: 69.59% (316 bytes vs 380). 16-row CLUT fade: 2 nested loops,
   t = (i << 12) / count, s = 0x1000 - t, per u16 entry
   out = (v & 0x8000) | ((cr*s + fr*t) >> 15) | (((cg*s + fg*t) >> 15) << 5)
         | (((cb*s + fb*t) >> 15) << 10), cr/cg/cb = (v&0x1F)<<3, (v>>2)&0xF8,
   (v>>7)&0xF8. Retail facts for the next attempt: the three pFactors lbu's are
   hoisted to the entry block (s0/t9/t8) yet the three fr*t products are NOT
   hoisted out of the inner loop (ours are); cr/cg/cb are byte-wide pseudo
   spills to 0(sp)/1(sp)/2(sp) (so they are u8 locals); the >> 15 is srl, so
   the sums are unsigned; the frame also saves s3 with no use in the body.
void func_800931D8(u8* pSrc, u8* pDst, s32 count, u8* pFactors) {
    s32 i;
    s32 j;
    s32 t;
    s32 s;
    u16* pIn;
    u16* pOut;
    u32 v;
    u8 cr;
    u8 cg;
    u8 cb;

    pOut = (u16*)pDst;
    for (i = 0; i < count; i++) {
        t = (i << 12) / count;
        s = 0x1000 - t;
        pIn = (u16*)pSrc;
        for (j = 0; j < 0x100; j++) {
            v = *pIn;
            if (v == 0) {
                *pOut = v;
            } else {
                cr = (v & 0x1F) << 3;
                cg = (v >> 2) & 0xF8;
                cb = (v >> 7) & 0xF8;
                *pOut = (v & 0x8000) | ((u32)(cr * s + pFactors[0] * t) >> 15) |
                        (((u32)(cg * s + pFactors[1] * t) >> 15) << 5) |
                        (((u32)(cb * s + pFactors[2] * t) >> 15) << 10);
            }
            pIn++;
            pOut++;
        }
    }
}
*/
void WorldMapWrapPosition(WorldMapPos* p) {
    if (p->x >= g_WorldMapTileColumns * 0x800000) {
        p->x -= g_WorldMapTileColumns * 0x800000;
    }
    if (p->x < 0) {
        p->x += g_WorldMapTileColumns * 0x800000;
    }
    if (p->z >= g_WorldMapTileRows * 0x800000) {
        p->z -= g_WorldMapTileRows * 0x800000;
    }
    if (p->z < 0) {
        p->z += g_WorldMapTileRows * 0x800000;
    }
}

void WorldMapWrapPositionScaled(WorldMapPos* p) {
    if (p->x >= g_WorldMapTileColumns * 2048) {
        p->x -= g_WorldMapTileColumns * 2048;
    }
    if (p->x < 0) {
        p->x += g_WorldMapTileColumns * 2048;
    }
    if (p->z >= g_WorldMapTileRows * 2048) {
        p->z -= g_WorldMapTileRows * 2048;
    }
    if (p->z < 0) {
        p->z += g_WorldMapTileRows * 2048;
    }
}

void WorldMapWrapDelta(WorldMapPos* p) {
    if (p->x < -0x4000000) {
        p->x += g_WorldMapTileColumns * 0x800000;
    } else if (p->x > 0x4000000) {
        p->x -= g_WorldMapTileColumns * 0x800000;
    }
    if (p->z < -0x4000000) {
        p->z += g_WorldMapTileRows * 0x800000;
    } else if (p->z > 0x4000000) {
        p->z -= g_WorldMapTileRows * 0x800000;
    }
}

void WorldMapWrapDeltaScaled(WorldMapPos* p) {
    if (p->x < -0x4000) {
        p->x += g_WorldMapTileColumns * 2048;
    } else if (p->x > 0x4000) {
        p->x -= g_WorldMapTileColumns * 2048;
    }
    if (p->z < -0x4000) {
        p->z += g_WorldMapTileRows * 2048;
    } else if (p->z > 0x4000) {
        p->z -= g_WorldMapTileRows * 2048;
    }
}

s32 WorldMapInterpolatePlaneHeight(WorldMapPos* p0, WorldMapPos* p1, WorldMapPos* p2) {
    p0->unk4 = (-(p2->x * (p0->x - p1->x)) - p2->z * (p0->z - p1->z)) / p2->unk4;
    p0->unk4 += p1->unk4;
    return p0->unk4;
}
u8* WorldMapGetTileEntry(s32 x, s32 z) {
    s16 idx;
    s16 zone;
    s16 sub;

    idx = (z >> 20) / 8 * g_WorldMapTileColumns + (x >> 20) / 8;
    x = (x >> 12) & 0x7FF;
    z = (z >> 12) & 0x7FF;
    zone = 0;
    if (x >= 0x400) {
        zone = 1;
        x -= 0x400;
    }
    if (z >= 0x400) {
        zone |= 2;
        z -= 0x400;
    }
    sub = (z >> 4) / 8 * 9 + (x >> 4) / 8;
    return (u8*)g_WorldMapTileCellPointers[idx] + zone * 81 * 4 + sub * 4;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc13", func_80093740);
s32 WorldMapGetTilePlaneHeight(s32 x, s32 z) {
    WorldMapPos a;
    WorldMapPos b;
    WorldMapPos c;
    u8* pTile;

    pTile = WorldMapGetTileEntry(x, z);
    a.x = (u16)(x / 8);
    a.z = -(u16)(z / 8);
    if (pTile[1] & 0x80) {
        b.x = 0;
        b.z = 0;
        b.unk4 = (s8)pTile[0] << 12;
    } else {
        b.x = 0x10000;
        b.z = 0;
        b.unk4 = (s8)pTile[4] << 12;
    }
    func_80093740(&c, x, z);
    WorldMapInterpolatePlaneHeight(&a, &b, &c);
    return a.unk4 << 3;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc13", func_80093A5C);
s32 WorldMapGetTileValue(WorldMapPos* p) {
    s16 idx;
    s16 baseOffset;
    s32 sub;
    s32 pBase;

    idx = ((p->z >> 20) / 8) * g_WorldMapTileColumns + (p->x >> 20) / 8;
    pBase = g_WorldMapTileCellPointers[idx];
    baseOffset = 0x510; // a constant local keeps the load at 0x510(v1), not a widened add
    sub = (((p->z & 0x7FF000) >> 19) << 4) | ((p->x & 0x7FF000) >> 19);
    return ((s16*)((u8*)pBase + baseOffset))[sub];
}

s32 WorldMapGetTileHalfIndex(WorldMapPos* p) {
    s32 n;
    s32 k;
    s32 dx;
    s32 dz;
    s32 sum;

    n = WorldMapGetTileValue(p);
    k = (n & 0xF) * 4;
    dx = (u16)(p->x / 8) - g_WorldMapTileShapeHalfOrigin[k];
    dz = (u16)(p->z / 8) - D_8009B46C[k];
    sum = (dx * g_WorldMapTileShapeHalfAxis[k] >> 12) + (dz * D_8009B36C[k] >> 12);
    if (sum > 0) {
        return ((u32)n >> 4) & 7;
    }
    return ((u32)n >> 7) & 7;
}

s32 WorldMapGetTileShape(WorldMapPos* p) {
    return WorldMapGetTileValue(p) & 0xF;
}

s32 WorldMapGetTileAttrBits10To15(WorldMapPos* p) {
    return ((u32)WorldMapGetTileValue(p) << 16) >> 26;
}

s32 WorldMapGetTileGroup(WorldMapPos* p) {
    return (WorldMapGetTileEntry(p->x, p->z)[3] >> 2) & 0xF;
}

s16 WorldMapGetTileKindValue(s16 row, s16 col) {
    return *(s16*)(g_WorldMapTileKindTable + row * 16 + col * 2);
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc13", func_80094088);
/* best: 79.96%. The index really is `k * 4` on the s32 arrays (the original's
   sll 16/sra 12 sign-extends the s16 k and scales by 16; plain D_8009B264[k] is
   sra 14 and is worse). Remaining deltas: gcc swaps the callee-saved pair (pPos
   in s1, pOut in s0; the original has pPos in s0), and forms the second table
   address as lui+addiu+addu instead of the original's lui+addu+lw %lo(at).

s32 func_80094088(WorldMapPos* p, WorldMapPos* pPos, WorldMapPos* pOut) {
    s16 k;
    s32 sum;

    k = WorldMapGetTileShape(p);
    sum = pPos->x * D_8009B264[k * 4] + pPos->z * D_8009B26C[k * 4];
    if (sum == 0) {
        pOut->x = pPos->x;
        pOut->z = pPos->z;
        return 0;
    }
    if (sum < 0) {
        pOut->x = -D_8009B264[k * 4];
        pOut->z = -D_8009B26C[k * 4];
    } else {
        pOut->x = D_8009B264[k * 4];
        pOut->z = D_8009B26C[k * 4];
    }
    return 1;
}
*/INCLUDE_ASM("asm/world_map/nonmatchings/main/misc13", func_80094154);
/* best: 98.21% (112 bytes, original 112). Instruction sequence and sizes are
   identical; the only delta is register choice: gcc swaps the two loads and
   puts the dz difference's operand load in the dying a0, and picks a3/v0 for
   the two mflo results where the original has a2/v1. Six declaration orders of
   {dx,dz,sum} all give exactly 98.21%; split-statement (dx = p0->x; dx -= p1->x)
   forms are 91.79, single-expression 72.86. Allocator tie.

s32 func_80094154(WorldMapPos* p0, WorldMapPos* p1) {
    s32 dx;
    s32 dz;
    s32 sum;

    dx = p0->x - p1->x;
    if (dx < 0) {
        dx = -dx;
    }
    sum = (dx >> 12) * (dx >> 12);
    dz = p0->z - p1->z;
    if (dz < 0) {
        dz = -dz;
    }
    sum += (dz >> 12) * (dz >> 12);
    return SquareRoot0(sum);
}
*/
void WorldMapBuildDirectionFromPositions(WorldMapPos* p0, WorldMapPos* p1, s32* pVector, s16* pAngle) {
    s32 dx;
    s32 dz;

    dx = p1->x - p0->x;
    dz = p1->z - p0->z;
    *pAngle = (ratan2(dz, dx) + 0x400) & 0xFFF;
    pVector[0] = rcos(*pAngle);
    pVector[2] = -rsin(*pAngle);
}
