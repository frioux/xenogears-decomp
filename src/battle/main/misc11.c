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
extern u8 D_800D2DD7[];
extern u8 D_800D2DD8[];
extern u8 D_800C204C;

extern s32 func_8008ABB8(s32, s32);
extern void func_8008AB94(void);
extern void func_8007FB70(u8);
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

/* 89.00%: only the tail order differs - the original has sh; lw ra; addiu sp;
   jr ra; nop (20 insns), this gives lw ra; sh; jr ra; addiu sp (19). A pointer
   local for the stores picks up an s0 save (73.40%). */
/*
void func_8007FEC4(void) {
    D_800D2DB4 = (u8 *)func_8008ABB8(0x5DA4, 0);
    bzero(D_800D2DB4, 0x5DA4);
    *(u16 *)(D_800D2DB4 + 0x5D9C) = 0xA0;
    *(u16 *)(D_800D2DB4 + 0x5D9E) = 0x64;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_8007FEC4);

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

/* 84.53%: same insns, but gcc keeps the D_800D2DD7 base in t0 for the
   loop-end `lbu` (the original keeps a copy t1,v0) and leaves the `i++` at
   the end of the body instead of in the first beqz delay slot. Tried: u8 vs
   s32 locals, best=0xFF init first (that is what got 84.53), ++i, separate
   index local. */
/*
s32 func_80080AE4(u8 arg0) {
    s32 best = 0xFF;
    u8 *p = D_800D2DD7;
    s16 *pSlots = (s16 *)(p + 0x2F);
    s32 i = p[0];
    s32 sel;
    u8 idx;

    do {
        idx = D_800D2DD8[i];
        if (pSlots[idx] < best && idx != arg0) {
            sel = idx;
            best = *(u8 *)&pSlots[idx];
        }
        if (++i == 0xB) {
            i = 0;
        }
    } while (i != p[0]);
    return sel;
}
*/
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_8008189C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_800819A4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_80081B58);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_800820A4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_800822C4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_80082504);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc11", func_800826CC);
