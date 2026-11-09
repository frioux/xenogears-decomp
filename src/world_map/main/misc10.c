#include "common.h"
#include "main/game.h"

extern void* D_8009BCC0;
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
/* 84.68%: retail loads the D_8009C620 pointer before the branch and keeps
   addiu a1,a1,0x16A4 in the bne delay slot, and compares with the id in v1
   against li v0,0x99; gcc 2.7.2 sinks the global load below the branch.
s32 func_80088D00(s32 arg0) {
    u8* pDst;
    u8* pBase;
    s32* pSrc;

    pDst = D_8009BE24 + (arg0 << 7);
    pBase = D_8009C620;
    if (g_GameState.unk1930[0] == 0x99) {
        pSrc = (s32*)(pBase + 0x16A4);
        pSrc[2] = *(s32*)(pDst + 0x28) >> 12;
        pSrc[3] = *(s32*)(pDst + 0x2C) >> 12;
        pSrc[4] = *(s32*)(pDst + 0x30) >> 12;
    }
    return 1;
}
*/

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
/* 83.11%: retail counts 7 down to -1 with bne against -1 instead of gcc's
   slti loop bound, and the base is D_8009BCC0 + arg0 * 0x2A0 + 0x4F with a
   0x54 stride.
void func_800894C8(s32 arg0) {
    u8* p;
    s32 i;

    p = (u8*)D_8009BCC0 + arg0 * 0x2A0 + 0x4F;
    for (i = 0; i < 8; i++) {
        *p &= 0x7F;
        p += 0x54;
    }
}
*/

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_80089514);
/* 88.11%: the 256-entry outer loop counts 0xFF down to -1 (bne against -1);
   the inner 8-entry loop is an ordinary slti loop over pEntry[0] == arg0 * 8 + j
   with pEntry[3] != 0, then pEntry[2] = 0.
void func_80089514(s32 arg0) {
    s16* pEntry;
    s32 i;
    s32 j;

    pEntry = D_8009BDF4;
    arg0 <<= 3;
    for (i = 0; i < 256; i++) {
        for (j = 0; j < 8; j++) {
            if (pEntry[0] == arg0 + j && pEntry[3] != 0) {
                pEntry[2] = 0;
                break;
            }
        }
        pEntry += 0x26;
    }
}
*/

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_80089580);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_80089748);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_80089C78);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_8008A2C8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_8008A52C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_8008A5B8);
