#include "common.h"

// 12.4 fixed point world map position; only x (0x0) and z (0x8) are read here.
typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 unk4;
    /* 0x8 */ s32 z;
} WorldMapPos; // Size: 0xC

extern void func_800346D4(void* pString);
extern s32 func_80093E8C(WorldMapPos* p);
extern u8* func_80093660(s32 x, s32 z);
extern long SquareRoot0(long a);
extern long ratan2(long y, long x);
extern int rcos(int a);
extern int rsin(int a);

extern u8 D_8009D498[];
extern u8 D_8009BD64[];
extern u8 D_8009BAC8[];
extern s32 D_8009B264[];
extern s32 D_8009B26C[];
extern s32 D_8009C184[];
extern s32 D_8009B364[];
extern s32 D_8009B36C[];
extern s32 D_8009B464[];
extern s32 D_8009B46C[];
extern s32 D_8009D160;
extern s32 D_8009D2B4;
extern s16 D_8009BD24;
extern u16 D_8009D4A8;
extern u8 D_8009D500;
extern s32 func_80032F54(void* pString, s16 a, s16 b, s16 c, s16 d, s16 e, s16 f);
extern void func_80093740(WorldMapPos* p, s32 x, s32 z);
extern s32 func_800935DC(WorldMapPos* p0, WorldMapPos* p1, WorldMapPos* p2);

s32 func_80092BE4(void) {
    u8* pBuffer;

    pBuffer = D_8009D498;
    D_8009BD24 = -1;
    func_80032F54(pBuffer, 0x3C0, 0x180, 0xA0, 0x78, 0x20, 1);
    D_8009D500 = 8;
    D_8009D4A8 |= 2;
    func_80034614(pBuffer);
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc13", func_80092C70);

void func_80092DD0(void) {
    func_800346D4(&D_8009D498[0]);
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc13", func_80092DF8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc13", func_80092FD8);

void func_800931B0(void) {
    func_800346D4(&D_8009BD64[0]);
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc13", func_800931D8);
void func_80093354(WorldMapPos* p) {
    if (p->x >= D_8009D160 * 0x800000) {
        p->x -= D_8009D160 * 0x800000;
    }
    if (p->x < 0) {
        p->x += D_8009D160 * 0x800000;
    }
    if (p->z >= D_8009D2B4 * 0x800000) {
        p->z -= D_8009D2B4 * 0x800000;
    }
    if (p->z < 0) {
        p->z += D_8009D2B4 * 0x800000;
    }
}

void func_800933EC(WorldMapPos* p) {
    if (p->x >= D_8009D160 * 2048) {
        p->x -= D_8009D160 * 2048;
    }
    if (p->x < 0) {
        p->x += D_8009D160 * 2048;
    }
    if (p->z >= D_8009D2B4 * 2048) {
        p->z -= D_8009D2B4 * 2048;
    }
    if (p->z < 0) {
        p->z += D_8009D2B4 * 2048;
    }
}

void func_80093484(WorldMapPos* p) {
    if (p->x < -0x4000000) {
        p->x += D_8009D160 * 0x800000;
    } else if (p->x > 0x4000000) {
        p->x -= D_8009D160 * 0x800000;
    }
    if (p->z < -0x4000000) {
        p->z += D_8009D2B4 * 0x800000;
    } else if (p->z > 0x4000000) {
        p->z -= D_8009D2B4 * 0x800000;
    }
}

void func_80093534(WorldMapPos* p) {
    if (p->x < -0x4000) {
        p->x += D_8009D160 * 2048;
    } else if (p->x > 0x4000) {
        p->x -= D_8009D160 * 2048;
    }
    if (p->z < -0x4000) {
        p->z += D_8009D2B4 * 2048;
    } else if (p->z > 0x4000) {
        p->z -= D_8009D2B4 * 2048;
    }
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc13", func_800935DC);
/* best: 72.73%. Every instruction up to the division matches; the original has
   the zero-division and overflow trap guards (break 7 / break 6) that maspsx
   only emits with --expand-div, which the XenoWorldMap preset does not pass.
   Blocked on the preset, not on the C.

s32 func_800935DC(WorldMapPos* p0, WorldMapPos* p1, WorldMapPos* p2) {
    p0->unk4 = (-(p2->x * (p0->x - p1->x)) - p2->z * (p0->z - p1->z)) / p2->unk4;
    p0->unk4 += p1->unk4;
    return p0->unk4;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc13", func_80093660);
/* best: 92.59% (220 bytes vs 224). Instruction multiset is identical; only the
   register assignment of the four temps differs (the original keeps hx in a0 and
   hz in a1 by reusing the dying x/z argument registers, gcc gives hx a3 and hz
   v1 and then puts zone in a0), and gcc fills the hz bgez delay slot with the
   hx >> 4 where the original has a nop. Seven local declaration orders all give
   exactly 92.59%, so this is an allocator tie, not a source-order problem.

u8* func_80093660(s32 x, s32 z) {
    s32 hx;
    s32 hz;
    s16 idx;
    s16 zone;
    s16 sub;

    idx = ((z >> 20) / 8) * D_8009D160 + (x >> 20) / 8;
    hx = (x >> 12) & 0x7FF;
    hz = (z >> 12) & 0x7FF;
    zone = 0;
    if (hx >= 0x400) {
        zone = 1;
        hx -= 0x400;
    }
    if (hz >= 0x400) {
        zone |= 2;
        hz -= 0x400;
    }
    sub = ((hz >> 4) / 8) * 9 + (hx >> 4) / 8;
    return (u8*)D_8009C184[idx] + zone * 81 * 4 + sub * 4;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc13", func_80093740);
s32 func_80093978(s32 x, s32 z) {
    WorldMapPos a;
    WorldMapPos b;
    WorldMapPos c;
    u8* pTile;

    pTile = func_80093660(x, z);
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
    func_800935DC(&a, &b, &c);
    return a.unk4 << 3;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc13", func_80093A5C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc13", func_80093E8C);
/* best: 99.43% (140 bytes, same size). Identical instruction multiset; the only
   delta is the final address add: the original keeps the table entry in v1 and
   emits addu v1,v1,v0 then lh v0,0x510(v1), while gcc aims the add at v0 and
   loads into v0. Tried the address as base+sub*2+0x510, as an s16 index form, as
   (s16*)pBase[0x288+sub] and as a struct member array; all give exactly 99.43%,
   so this is the allocator tie-break class, not a source-shape problem.

s32 func_80093E8C(WorldMapPos* p) {
    s16 idx;
    s32 sub;
    s32 pBase;

    idx = ((p->z >> 20) / 8) * D_8009D160 + (p->x >> 20) / 8;
    pBase = D_8009C184[idx];
    sub = (((p->z & 0x7FF000) >> 19) << 4) | ((p->x & 0x7FF000) >> 19);
    return ((s16*)((u8*)pBase + 0x510))[sub];
}
*/

s32 func_80093F18(WorldMapPos* p) {
    s32 n;
    s32 k;
    s32 dx;
    s32 dz;
    s32 sum;

    n = func_80093E8C(p);
    k = (n & 0xF) * 4;
    dx = (u16)(p->x / 8) - D_8009B464[k];
    dz = (u16)(p->z / 8) - D_8009B46C[k];
    sum = (dx * D_8009B364[k] >> 12) + (dz * D_8009B36C[k] >> 12);
    if (sum > 0) {
        return ((u32)n >> 4) & 7;
    }
    return ((u32)n >> 7) & 7;
}

s32 func_80093FE4(WorldMapPos* p) {
    return func_80093E8C(p) & 0xF;
}

s32 func_80094004(WorldMapPos* p) {
    return ((u32)func_80093E8C(p) << 16) >> 26;
}

s32 func_80094028(WorldMapPos* p) {
    return (func_80093660(p->x, p->z)[3] >> 2) & 0xF;
}

s16 func_80094060(s16 row, s16 col) {
    return *(s16*)(D_8009BAC8 + row * 16 + col * 2);
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc13", func_80094088);
/* best: 79.96%. Yield/dot-product of the position against the row k of
   D_8009B264/D_8009B26C (stride 0x10). The sum, the branches and the reloads
   are right; deltas are the callee-saved order (s0/s1) and that gcc forms the
   second table address as lui+addiu+addu instead of the original's
   lui+addu+lw %lo(at).

s32 func_80094088(WorldMapPos* p, WorldMapPos* pPos, WorldMapPos* pOut) {
    s16 k;
    s32 sum;

    k = func_80093FE4(p);
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
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc13", func_80094154);
/* best: 92.50%. Instruction sequence, abs/shift/mult order and the mult/mflo
   split are identical; the only delta is that gcc copies p0 from a0 into a2 at
   entry and then uses a0 for the dx temp, where the original keeps p0 in a0 and
   uses v0/v1 for the temps. Declaration orders tried: dx,dz,dx2,dz2 (91.07),
   dz,dx,dx2,dz2 (92.50); single-expression, accumulator and shift-in-place
   shapes are all worse (72-91%).

s32 func_80094154(WorldMapPos* p0, WorldMapPos* p1) {
    s32 dz;
    s32 dx;
    s32 dx2;
    s32 dz2;

    dx = p0->x - p1->x;
    if (dx < 0) {
        dx = -dx;
    }
    dx = dx >> 12;
    dx2 = dx * dx;
    dz = p0->z - p1->z;
    if (dz < 0) {
        dz = -dz;
    }
    return SquareRoot0(dx2 + (dz >> 12) * (dz >> 12));
}
*/

void func_800941C4(WorldMapPos* p0, WorldMapPos* p1, s32* pVector, s16* pAngle) {
    s32 dx;
    s32 dz;

    dx = p1->x - p0->x;
    dz = p1->z - p0->z;
    *pAngle = (ratan2(dz, dx) + 0x400) & 0xFFF;
    pVector[0] = rcos(*pAngle);
    pVector[2] = -rsin(*pAngle);
}
