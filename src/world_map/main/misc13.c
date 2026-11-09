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

extern u8 D_8009D498[];
extern u8 D_8009BD64[];
extern u8 D_8009BAC8[];

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
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc13", func_80093354);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc13", func_800933EC);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc13", func_80093484);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc13", func_80093534);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc13", func_800935DC);
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
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc13", func_800941C4);
