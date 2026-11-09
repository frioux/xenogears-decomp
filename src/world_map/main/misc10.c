#include "common.h"

extern void* D_8009BDF4;
extern void* D_8009BE1C;
extern void* D_8009BE20;
extern u8* D_8009BE24;
extern u8* D_8009C620;

extern void HeapFree(void* pMem);
extern void func_8008BFD4(s32 arg0, u32* pVec, s32 arg2, s32 arg3);

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_80088B40);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_80088C90);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_80088D00);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_80088D64);

s32 func_80088DE4(s32 arg0) {
    func_8008BFD4(arg0, (u32*)(D_8009BE24 + (arg0 << 7) + 0x28), 0x68, 0x60);
    return 1;
}

s32 func_80088E1C(s32 arg0) {
    u8* pDst;
    s32* pSrc;

    pDst = D_8009BE24 + (arg0 << 7);
    pSrc = (s32*)(D_8009C620 + 0x18A4);
    *(s32*)(pDst + 0x28) = pSrc[0] << 12;
    *(s32*)(pDst + 0x2C) = pSrc[1] << 12;
    *(s32*)(pDst + 0x30) = pSrc[2] << 12;
    return 1;
}

s32 func_80088E68(s32 arg0) {
    func_8008BFD4(arg0, (u32*)(D_8009BE24 + (arg0 << 7) + 0x28), 0x10C, 0x1A6);
    return 1;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_80088EA0);

s32 func_80088F1C(s32 arg0) {
    func_8008BFD4(arg0, (u32*)(D_8009BE24 + (arg0 << 7) + 0x28), 0xC9, 0x392);
    return 1;
}

s32 func_80088F54(void) {
    return 3;
}

s32 func_80088F5C(void) {
    return 3;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_80088F64);

void func_80088FF4(void) {
    HeapFree(D_8009BDF4);
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_8008901C);

void func_80089128(void) {
    HeapFree(D_8009BE1C);
    HeapFree(D_8009BE20);
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_80089160);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_800893E0);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_800894C8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_80089514);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_80089580);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_80089748);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_80089C78);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_8008A2C8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_8008A52C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_8008A5B8);
