#include "common.h"
#include "psyq/libgpu.h"
#include "system/memory.h"
#include "system/archive.h"

// .bss/.sdata of the overlay (supplied by splat's asm segments)
extern u8 D_800928A0;
extern u32 D_80092818[];
extern s32 D_80092810;
extern u32 D_800928E4;
extern s32 D_80050100;
extern s32 D_80050104;
extern s32 D_80091C30;
extern s32 D_80096D88;
extern s32 D_80096D8C;

extern void func_80030A30(s32 index, void *pLight);
extern void func_8008AF6C(void *pData);
extern void func_8008B730(void *pA, s32 arg1, s32 arg2);

void func_8008A618(void) {
    D_80092810 = 1;
}

void func_8008A62C(void) {
    D_80092810 = 0;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008A63C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008A6F8);

void func_8008A78C(u8 *pActor) {
    s32 index = D_800928A0;
    u8 *pModel = *(u8 **)(pActor + 4);
    u8 *pData = *(u8 **)(pModel + 0xC);
    s32 count = *(s16 *)(pData + 2) - 1;
    u32 *pList = *(u32 **)(pData + index * 4 + 0x10);
    u32 value = D_80092818[index];

    while (count != -1) {
        pList[1] = value;
        count--;
        pList = (u32 *)((u8 *)pList + 0x14);
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008A7E0);

void func_8008ABAC(u8 *pActor) {
    func_80030A30(0, *(u8 **)(*(u8 **)pActor + 4));
    func_80030A30(1, *(u8 **)(*(u8 **)(pActor + 4) + 4));
    func_80030A30(2, *(u8 **)(*(u8 **)(pActor + 8) + 4));
}

void func_8008AC0C(u8 *pActor) {
    s32 n = *(s16 *)(pActor + 0x14);

    ClearOTagR(*(u_long **)(pActor + D_800928A0 * 4 + 4), n);
    D_800928E4 = *(u_long *)(pActor + D_800928A0 * 4 + 4);
    D_80050100 = *(u8 *)(pActor + 0x17);
}

void func_8008AC7C(s32 arg0) {
    D_80091C30 = arg0;
}

void func_8008AC8C(void) {
    D_80091C30 = GetRCnt(0xF2000001);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008ACB8);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008AE1C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008AF6C);

void func_8008B070(s32 entryIndex) {
    void *pBuffer;

    HeapSetCurrentContentType(0xA);
    pBuffer = HeapAlloc(ArchiveDecodeSize(entryIndex), 0);
    ArchiveReadFileToBuffer(entryIndex, pBuffer, 0, 0);
    ArchiveCdDataSync(0);
    func_8008AF6C(pBuffer);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008B0D8);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008B13C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008B38C);

void func_8008B5DC(void *pA, s32 arg1) {
    func_8008B730(pA, arg1, 1);
}

s16 func_8008B5FC(s32 arg0, s32 arg1, s32 arg2) {
    s32 value;
    s32 diff;

    arg0 &= 0xFFF;
    diff = (arg0 - arg1) & 0xFFF;
    value = arg0;
    if (diff != 0) {
        if (diff < 0x800) {
            value = arg0 - diff / arg2;
        } else {
            value = arg0 + (0x1000 - diff) / arg2;
        }
    }
    return value;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008B650);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008B730);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008BA2C);

void func_8008BAE0(void *pMem) {
    HeapFree(pMem);
}

void func_8008BB00(s32 *pDst) {
    s32 a = D_80096D88;
    s32 b = D_80096D8C;

    pDst[0] = a;
    pDst[1] = b;
}

void func_8008BB1C(s32 *pSrc) {
    D_80096D88 = pSrc[0];
    D_80096D8C = pSrc[1];
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008BB3C);
