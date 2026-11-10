#include "common.h"
#include "main/game.h"
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
extern u8 D_800D2DD7[];
extern u8 D_800D2DD8[];
extern u8 D_800C204C;
extern u8 D_800C3202[];
extern u8 *D_800C3EA4;
extern volatile s32 D_800CCB34;

extern s32 func_8008ABB8(s32, s32);
extern void func_8008AB94(void);
extern void func_8007FB70(u8);
extern void func_8008AC50(void);
extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);
extern void ArchiveReadFileToBuffer(s32 index, void *pBuffer, u32 arg2, u32 flags);
extern void HeapFree(void *pMem);
extern void bzero(void *p, s32 n);
extern u8 *D_800C3E24;
extern u8 D_800D2C34;
extern u8 func_80084854(u8 arg0, u8 arg1);
extern void func_80093B08(u8 arg0);
extern u16 func_80089C08(u8 arg0);
extern void func_800BC404(u16 arg0);
extern void func_800BCD98(u16 arg0);
extern void func_80088490(u8 arg0);
extern void func_8009AEFC(u8 arg0);
extern void func_800BAF48(u8 arg0);
extern void func_800883AC(u8 arg0);
extern u8 D_800CCE42[];
extern u8 D_800C3EB8[];
extern u8 D_800D2D24[];
extern u8 D_80059179;

/* 74.02%: with `extern volatile s32 D_800CCB34` the loop body is exact (four
   reloads of the counter between the four stores, and the two separate
   `lbu 0(a1)` of pSrc[i]: the stores are plain, the counter is what reloads).
   The remaining gap is that the original hoists `lw a3,D_800C3EA4` into the
   preheader and hoists only the 0xC constant (`li t2,0xC`), leaving `li v1,0x12`
   in the body; our gcc keeps the D_800C3EA4 load in the loop (CSE'd across the
   four stores) and hoists both constants. Also allocation: original i=a2,
   pSrc=a1, base=a3. A pDst local hoists the load but drops to 71.89.
   Prologue is exact: (u8 arg0, u8 arg1), u8 count, compared as (u8)count. */
/*
void func_8007FBE0(u8 arg0, u8 arg1) {
    u8 count;
    s32 i;
    u8 *pSrc;

    count = arg1;
    if (arg0 == arg1) {
        count = arg1 - 1;
    }
    pSrc = D_800C3202 + arg0 * 6;
    for (i = 0; i < arg0 - 1; i++) {
        *(u16 *)(D_800C3EA4 + (D_800CCB34 + i * 2) * 0x10 + 0x910) = 0xC;
        *(u16 *)(D_800C3EA4 + (D_800CCB34 + i * 2) * 0x10 + 0x912) = pSrc[i] + 0x5E;
        *(u16 *)(D_800C3EA4 + (D_800CCB34 + i * 2) * 0x10 + 0x914) = 0x12;
        *(u16 *)(D_800C3EA4 + (D_800CCB34 + i * 2) * 0x10 + 0x916) = pSrc[i] + 0x5E;
    }
    D_800D2D28[0x97] = count;
    D_800D2D28[0x98] = *(u8 *)&D_800CCB34;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_8007FBE0);

void func_8007FCE8(void) {
    if (D_800D2D28[0xAE] != 0) {
        HeapFree(D_800D367C);
        D_800D2D28[0xAE] = 0;
    }
}

void func_8007FD38(u8 arg0) {
    s32 type;

    if (D_800D2D28[0xAE] != 0) {
        return;
    }
    func_8008AC50();
    if (D_800D32A1[arg0 * 8] == 0) {
        type = 1;
    } else {
        type = 2;
    }
    func_8008AB94();
    D_800D367C = (void *)func_8008ABB8(ArchiveDecodeAlignedSize(type), 0);
    ArchiveReadFileToBuffer(type, D_800D367C, 0, 0x80);
    func_8008AC50();
    D_800D2D28[0xAE] = 1;
}

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
    u8 *pBuffer;

    pBuffer = (u8 *)func_8008ABB8(0x5DA4, 0);
    D_800D2DB4 = pBuffer;
    bzero(pBuffer, 0x5DA4);
    *(volatile u16 *)(D_800D2DB4 + 0x5D9C) = 0xA0;
    *(volatile u16 *)(D_800D2DB4 + 0x5D9E) = 0x64;
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
s32 func_80080AE4(u8 arg0) {
    s32 best = 0xFF;
    s32 i = D_800D2DD7[0];
    s32 sel;
    s32 idx;
    s16 *pSlots = (s16 *)(D_800D2DD7 + 0x2F);

    do {
        idx = D_800D2DD8[i];
        i++;
        if (*(pSlots + idx) < best && idx != arg0) {
            sel = idx;
            best = *(u8 *)(pSlots + idx);
        }
        if (i == 0xB) {
            i = 0;
        }
    } while (i != D_800D2DD7[0]);
    return sel;
}

void func_80080B64(u8 color) {
    D_800C402F[D_800C3EAC[0x2DA] * 0x48] = 0xFE;
    D_800C400B[D_800C3EAC[0x2DA] * 0x48] = color;
    D_800D366C = 0;
}

void func_80080BD0(void) {
    D_800C3EAC[0x2DB] = 1;
    if (D_800C3EAC[0x2D3] < 3 && D_800C204C == 0) {
        func_8007FCE8();
        func_8007FDEC();
        func_800800E8(D_800C3EAC[0x2D3]);
        func_8007FB70(D_800C3EAC[0x2D3]);
    }
}

void func_80080C6C(u8 index) {
    u8 *pEntry = D_800D3278 + index * 0x38;

    pEntry[0x34] = 1;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_80080C94);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_8008115C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_80081318);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_80081504);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_800816F8);
void func_8008189C(s32 arg0) {
    s32 i;

    if (D_800C3EAC[0x2E9] != 0) {
        return;
    }
    func_800BC404(func_80089C08(D_800C3EAC[0x2E8]));
    func_800BCD98(func_80089C08(D_800C3EAC[0x2E8]));
    i = 0;
    do {
        if (func_80084854(D_800C3EAC[0x2E8], i) != D_800C3EAC[0x2E8]) {
            (D_800C3E24 + i)[0xE6] = 1;
        } else {
            (D_800C3E24 + i)[0xE6] = 0;
        }
        i++;
    } while (i < 4);
    if (D_800D2C34 != 4) {
        func_80093B08(arg0);
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_800819A4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_80081B58);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_800820A4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_800822C4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_80082504);
void func_800826CC(u8 arg0) {
    s32 i;

    func_80088490(arg0);
    func_8009AEFC(arg0);
    func_800BAF48(arg0);
    *(u8 *)(D_800CCE42 + arg0 * 0x170) |= 0x80;
    func_800883AC(arg0);
    D_800D32A1[arg0 * 8] = 2;
    if (D_800D2D24[arg0] != 7) {
        *(u8 *)(arg0 * 0x1E4 + (u32)D_800C3EA4 + 0x853D) = 2;
    }
    D_800C3EB8[arg0 * 0x1C] = 1;
    D_800C3EAC[arg0 + 0x2EB] = 1;
    for (i = 0; i < 3; i++) {
        if (g_GameState.partyMembers[i] == D_800D2D24[arg0] && D_80059179 == 0) {
            g_GameState.gearRide[i] = 1;
        }
    }
}
