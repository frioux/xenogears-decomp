#include "common.h"
#include "main/game.h"

extern void* D_8009BCC0;
extern void* D_8009BDF4;
extern s32 D_8009C610;
extern u16 D_8009B69C[];
extern u16 D_8009B6B0[];
extern void* D_8009BE1C;
extern void* D_8009BE20;
extern u8* D_8009BE24;
extern s16 D_8009AFDC[];
extern u8* D_8009C620;

extern void* D_8009CD34;

extern void HeapFree(void* pMem);
extern void* func_80024524(void* pArg0, s16 arg1, s16 arg2, s16 arg3, s16 arg4, s16 arg5);
extern void func_800245D8(void* pSprite, s32 arg1);
extern void SpriteSetScale(void* pSprite, s16 scale);
extern void func_8008BFD4(s32 arg0, u32* pVec, s32 arg2, s32 arg3);

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_80088B40);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_80088C90);
/* 38.96%: retail has no stack frame, keeps D_8009C620 in a1 across the store
   and loads the id with lh; gcc 2.7.2 wants a 16-byte frame, reloads
   D_8009C620 every iteration and emits lhu + sll/sra for the multiply.
s32 func_80088C90(void) {
    s16* pIds;
    s16 id;

    *(s16*)(D_8009C620 + 0x16A4) = 1;
    pIds = D_8009AFDC;
    while (*pIds != -1) {
        id = *pIds;
        *(s16*)(D_8009C620 + id * 0x54) = 1;
        pIds++;
    }
    return 3;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_80088D00);
/* 84.68%: with pSrc computed before the if (the only shape that keeps the
   D_8009C620 load above the branch, as retail does) gcc folds +0x16A4 into the
   three store displacements (0x16AC/0x16B0/0x16B4) instead of retail's
   addiu a1,a1,0x16A4 in the bne delay slot; the compare also lands in
   v0/v1 swapped. Needs a shape where the base+0x16A4 pointer is materialised.
s32 func_80088D00(s32 arg0) {
    u8* pDst;
    s32* pSrc;

    pDst = D_8009BE24 + (arg0 << 7);
    pSrc = (s32*)(D_8009C620 + 0x16A4);
    if (g_GameState.unk1930[0] == 0x99) {
        pSrc[2] = *(s32*)(pDst + 0x28) >> 12;
        pSrc[3] = *(s32*)(pDst + 0x2C) >> 12;
        pSrc[4] = *(s32*)(pDst + 0x30) >> 12;
    }
    return 1;
}
*/
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

s32 func_80088D64(s32 arg0) {
    u8* pDst;
    u8* pSrc;

    pDst = D_8009BE24 + (arg0 << 7);
    pSrc = D_8009C620 + D_8009B69C[D_8009C610] * 0x54;
    *(s32*)(pDst + 0x28) = *(s32*)(pSrc + 0x8) << 12;
    *(s32*)(pDst + 0x2C) = (*(s32*)(pSrc + 0xC) << 12) + 0x40000;
    *(s32*)(pDst + 0x30) = *(s32*)(pSrc + 0x10) << 12;
    return 1;
}

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

s32 func_80088EA0(s32 arg0) {
    u8* pDst;
    u8* pSrc;

    pSrc = D_8009C620 + D_8009B6B0[D_8009C610] * 0x54;
    pDst = D_8009BE24 + (arg0 << 7);
    *(s32*)(pDst + 0x28) = *(s32*)(pSrc + 0x8) << 12;
    *(s32*)(pDst + 0x2C) = *(s32*)(pSrc + 0xC) << 12;
    *(s32*)(pDst + 0x30) = *(s32*)(pSrc + 0x10) << 12;
    return 1;
}

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

void func_80088F64(void) {
    s32 i;
    u8* p;

    p = (u8*)D_8009BCC0;
    for (i = 0x1FF; i != -1; i--) {
        *(s32*)(p + 0x4) = 0;
        *(s16*)(p + 0xA) = 0;
        *(s16*)(p + 0x12) = 0;
        *(s16*)(p + 0x18) = 0;
        *(s16*)(p + 0x16) = 0;
        *(s16*)(p + 0x14) = 0;
        *(s16*)(p + 0x20) = 0;
        *(s16*)(p + 0x1E) = 0;
        *(s16*)(p + 0x1C) = 0;
        p += 0x54;
    }
    D_8009BDF4 = HeapAlloc(0x4C00, 0);
    p = (u8*)D_8009BDF4;
    for (i = 0xFF; i != -1; i--) {
        *(s16*)(p + 0x6) = 0;
        *(s16*)(p + 0x4) = 0;
        p += 0x4C;
    }
}

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

void func_800894C8(s32 arg0) {
    u8* p;
    s32 i;

    p = (u8*)D_8009BCC0 + arg0 * 0x2A0 + 0x4F;
    for (i = 7; i != -1; i--) {
        *p &= 0x7F;
        p += 0x54;
    }
}

void func_80089514(s32 arg0) {
    s16* pEntry;
    s32 i;
    s32 j;

    pEntry = D_8009BDF4;
    for (i = 0xFF; i != -1; i--) {
        for (j = 0; j < 8; j++) {
            if (pEntry[0] == (arg0 << 3) + j && pEntry[3] != 0) {
                pEntry[2] = 0;
                break;
            }
        }
        pEntry += 0x26;
    }
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_80089580);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_80089748);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_80089C78);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_8008A2C8);
s32 func_8008A52C(s32 arg0) {
    u8* pActor;
    u8* pSprite;

    pActor = D_8009BE24 + (arg0 << 7);
    pSprite = func_80024524(D_8009CD34, 0x100, 0x1E0, 0x140, 0x100, 0x40);
    *(u8**)(pActor + 0x4C) = pSprite;
    func_800245D8(pSprite, 0);
    SpriteSetScale(*(u8**)(pActor + 0x4C), 0x1800);
    *(u32*)(*(u8**)(pActor + 0x4C) + 0x3C) &= ~4;
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc10", func_8008A5B8);
