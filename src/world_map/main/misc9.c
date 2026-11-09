#include "common.h"
#include "system/memory.h"
#include "psyq/libgpu.h"

extern u_char* D_8009BE24;
extern s32 D_8009C610;
extern u16 D_8009B674[];
extern u_char* D_8009C7EC;
extern u_char* D_8009CEB4;
extern u_char* D_8009D150;
extern u16 D_8009D478[];
extern u_char* D_8009D7E8;
extern u_char* D_8009D7EC;
extern u_char* D_8009D7F8;
extern u_char* D_8009D7FC;

void func_800848B4(s32 arg0, s32 arg1);
s32 func_800879E0(s32 arg0);

// 0x80-byte slot in the D_8009BE24 table
typedef struct {
    /* 0x00 */ u8 pad0[0x50];
    /* 0x50 */ s32 unk50;
    /* 0x54 */ s32 unk54;
    /* 0x58 */ u8 pad58[0x28];
} WorldMapSlot; // Size: 0x80

typedef struct {
    /* 0x0 */ s32 offset;
    /* 0x4 */ s32 unk4;
} WorldMapTableEntry; // Size: 0x8

void func_80085F58(void) {
    WorldMapTableEntry* pEntry;
    u_char* pBase;
    s32 i;

    pBase = D_8009C7EC;
    pEntry = (WorldMapTableEntry*)pBase;
    for (i = 0; i < 0x100; i++, pEntry++) {
        if (pEntry->offset != 0) {
            pEntry->offset += (s32)pBase;
        }
    }
    for (i = 0; i < 0x10; i++) {
        D_8009D478[i] = GetClut(0xF0, i + 0x1F0);
    }
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_80085FE0);
void func_80086124(void) {
    HeapFree(D_8009D7EC);
    HeapFree(D_8009D7E8);
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_8008615C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_800863E0);
void func_80086568(void) {
    HeapFree(D_8009CEB4);
    HeapFree(D_8009D150);
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_800865A0);
void func_800866C8(void) {
    HeapFree(D_8009D7FC);
    HeapFree(D_8009D7F8);
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_80086700);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_80086798);
s32 func_80087710(s32 index) {
    WorldMapSlot* pSlot = (WorldMapSlot*)(D_8009BE24 + index * 0x80);

    pSlot->unk50 = 0;
    pSlot->unk54 = 8;
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_80087734);
s32 func_800877E0(s32 index) {
    WorldMapSlot* pSlot = (WorldMapSlot*)(D_8009BE24 + index * 0x80);

    pSlot->unk50 = 0;
    pSlot->unk54 = 8;
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_80087804);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_80087904);
s32 func_800879A8(s32 index) {
    WorldMapSlot* pSlot = (WorldMapSlot*)(D_8009BE24 + index * 0x80);

    pSlot->unk50 = 0;
    pSlot->unk54 = 0x10;
    func_800879E0(index);
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_800879E0);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_80087A8C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_80087B84);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_80087C6C);
s32 func_80087F60(void) {
    s32 value = D_8009B674[D_8009C610];

    func_800848B4(value, value - 4);
    func_800848B4(value, value - 3);
    func_800848B4(value, value - 2);
    func_800848B4(value, value - 1);
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_80087FD0);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_80088570);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_8008868C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc9", func_80088720);
