#include "common.h"
#include "psyq/libgpu.h"

extern u8 *D_800C3EAC;

extern u8 *D_800C3DE8;
extern u8 *D_800D2D28;
extern u8 *D_800D2DB4;
extern void *D_800D367C;
extern u8 *D_800D3278;
extern u8 D_800C400B[];
extern u8 D_800C402F[];
extern u8 D_800D366C;
extern u8 D_800D32A1[];

extern s32 func_8008ABB8(s32, s32);
extern void func_8008AB94(void);
extern void func_8008AC50(void);
extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);
extern void ArchiveReadFileToBuffer(s32 index, void *pBuffer, u32 arg2, u32 flags);
extern void HeapFree(void *pMem);
extern void bzero(void *p, s32 n);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_8007FBE0);

void func_8007FCE8(void) {
    if (D_800D2D28[0xAE] != 0) {
        HeapFree(D_800D367C);
        D_800D2D28[0xAE] = 0;
    }
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_8007FD38);

void func_8007FDEC(void) {
    if (D_800D2D28[0x96] != 0) {
        HeapFree(D_800C3DE8);
        D_800D2D28[0x96] = 0;
    }
}

void func_8007FE3C(void) {
    s32 pBuffer;

    if (D_800D2D28[0x96] == 0) {
        func_8008AC50();
        func_8008AB94();
        pBuffer = func_8008ABB8(ArchiveDecodeAlignedSize(3), 0);
        D_800C3DE8 = (u8 *)pBuffer;
        ArchiveReadFileToBuffer(3, (void *)pBuffer, 0, 0x80);
        func_8008AC50();
        D_800D2D28[0x96] = 1;
    }
}

void func_8007FEC4(void) {
    D_800D2DB4 = (u8 *)func_8008ABB8(0x5DA4, 0);
    bzero(D_800D2DB4, 0x5DA4);
    *(u16 *)(D_800D2DB4 + 0x5D9C) = 0xA0;
    *(u16 *)(D_800D2DB4 + 0x5D9E) = 0x64;
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_8007FF14);

void func_800800E8(u8 arg0) {
    s32 index;

    D_800D2D28[0xAD] = 0;
    D_800D2D28[0xC7] = 0;
    D_800D2D28[0xA8] = 0;
    index = arg0 * 8;
    if (D_800D32A1[index] == 2) {
        D_800D32A1[index] = 1;
    }
    HeapFree(D_800D2DB4);
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_80080160);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_80080AE4);

void func_80080B64(u8 color) {
    D_800C402F[D_800C3EAC[0x2DA] * 0x48] = 0xFE;
    D_800C400B[D_800C3EAC[0x2DA] * 0x48] = color;
    D_800D366C = 0;
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_80080BD0);

void func_80080C6C(u8 index) {
    u8 *pEntry = D_800D3278 + index * 0x38;

    pEntry[0x34] = 1;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_80080C94);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_8008115C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_80081318);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_80081504);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_800816F8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_8008189C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_800819A4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_80081B58);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_800820A4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_800822C4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_80082504);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_800826CC);
