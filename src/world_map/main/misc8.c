#include "common.h"

typedef struct {
    /* 0x00 */ u8 unk0[0x50];
    /* 0x50 */ u32 unk50;
    /* 0x54 */ u8 unk54[0x2C];
} WorldMapActor; // Size: 0x80

extern WorldMapActor *D_8009BE24;
extern u8 D_8009AC60[];

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_80082F64);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_80083108);
void func_800831D8(u8 *pData, s32 count, u8 r, u8 g, s32 b) {
    s32 i;
    for (i = count - 1; i != -1; i--) {
        pData[4] = r; pData[5] = g; pData[6] = b;
        pData += 0x20;
    }
}INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_80083214);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_80083264);
s32 func_800834D0(void) {
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_800834D8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_8008355C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_800837DC);
s32 func_800838E8(s32 arg0) {
    D_8009BE24[arg0].unk50 = (u32)D_8009AC60;
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_8008390C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_80083A00);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_80083FE4);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_80084068);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_8008440C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_80084580);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_80084818);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_800848B4);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_800848F4);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_80084D00);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_80084DB8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_80085158);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_80085418);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_80085760);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_80085CDC);
