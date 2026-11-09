#include "common.h"

// 12.4 fixed point world map position; only x (0x0) and z (0x8) are read here.
typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 unk4;
    /* 0x8 */ s32 z;
} WorldMapPos; // Size: 0xC

extern void func_800346D4(void* pString);
extern s16 func_80093E8C(WorldMapPos* p);
extern u8* func_80093660(s32 x, s32 z);
extern long SquareRoot0(long a);
extern long ratan2(long y, long x);
extern int rcos(int a);
extern int rsin(int a);

extern u8 D_8009D498[];
extern u8 D_8009BD64[];
extern u8 D_8009BAC8[];
extern s32 D_8009D160;
extern s32 D_8009D2B4;

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc13", func_80092BE4);
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
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc13", func_80093484);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc13", func_80093534);
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
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc13", func_80093740);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc13", func_80093978);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc13", func_80093A5C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc13", func_80093E8C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc13", func_80093F18);

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
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc13", func_80094154);
/* best: 91.07%. Instruction sequence and mult/mflo placement identical; the
   only delta is that gcc copies p0 from a0 into a2 at entry (and permutes the
   temps) where the original keeps p0 in a0 and uses v0/v1. Tried the squares
   in the final expression and ternary negation: both worse.

s32 func_80094154(WorldMapPos* p0, WorldMapPos* p1) {
    s32 dx;
    s32 dz;
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
    dz = dz >> 12;
    dz2 = dz * dz;
    return SquareRoot0(dx2 + dz2);
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
