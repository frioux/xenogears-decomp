#include "common.h"
#include "psyq/libgpu.h"
#include "system/memory.h"
#include "system/archive.h"

// .bss/.sdata of the overlay (supplied by splat's asm segments)
extern u8 D_800928A0;
extern u32 D_80092818[];
extern s32 D_80092810;
extern s32 D_80092820;
extern u32 D_800928E4;
extern s32 D_80050100;
extern s32 D_80091C30;
extern s32 D_80096D88;
extern s32 D_80096D8C;
extern s32 D_80092914;
extern u8 *D_80092938;
extern void *D_80092868;

extern void func_80030A30(s32 index, void *pLight);
extern void func_8008B730(void *pA, s32 arg1, s32 arg2);
extern u32 func_800405E4(void);
extern void func_8008BB3C(void *pCtx);

typedef struct {
    /* 0x00 */ u32 unk0;
    /* 0x04 */ u32 unk4;
    /* 0x08 */ u8 unk8[4];
    /* 0x0C */ s16 unkC;
    /* 0x0E */ s16 unkE;
    /* 0x10 */ s16 unk10;
    /* 0x12 */ u16 pad12;
} Misc14Entry; // Size: 0x14

void func_8008A618(void) {
    D_80092810 = 1;
}

void func_8008A62C(void) {
    D_80092810 = 0;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008A63C);

typedef struct {
    u8 b[4];
} Misc14Quad; // Size: 0x4

void func_8008A6F8(u8 *pSrc) {
    u8 *pDst = (u8 *)&D_80092818[D_800928A0];

    if (pSrc[0] == pDst[0] && pSrc[1] == pDst[1] && pSrc[2] == pDst[2]) {
        D_80092914 = 0;
    } else {
        D_80092914 = 1;
        *(Misc14Quad *)pDst = *(Misc14Quad *)pSrc;
        pDst[3] = 0x20;
    }
}

void func_8008A78C(u8 *pActor) {
    s32 index = D_800928A0;
    u8 *pModel = *(u8 **)(pActor + 4);
    u8 *pData = *(u8 **)(pModel + 0xC);
    s32 count = *(s16 *)(pData + 2);
    Misc14Entry *pEntry = *(Misc14Entry **)(pData + index * 4 + 0x10);
    u32 value = D_80092818[index];

    while (--count != -1) {
        pEntry->unk4 = value;
        pEntry++;
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
    D_80092820 = GetRCnt(0xF2000001);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008ACB8);

extern void func_800453E8(void *pDrmode, void *pRect);
extern void func_8004546C(void *pDrmode, void *pRect);

void func_8008AE1C(u8 *pActor) {
    func_8008AC7C((s32)pActor);
    AddPrims(D_80092938, *(void **)(pActor + D_800928A0 * 4 + 0xC), *(void **)(pActor + D_800928A0 * 4 + 4));
    if ((*(u8 *)(pActor + 0x16) & 4) == 0) {
        func_800453E8(pActor + (D_800928A0 * 12 + 0x18), D_80092868);
    }
    if ((*(u8 *)(pActor + 0x16) & 8) == 0) {
        func_8004546C(pActor + (D_800928A0 * 12 + 0x30), (u8 *)D_80092868 + 8);
    }
    if (*(u8 *)(pActor + 0x16) & 0x10) {
        AddPrim(D_80092938, pActor + (D_800928A0 * 16 + 0x48));
    }
    AddPrim(D_80092938, pActor + (D_800928A0 * 12 + 0x30));
    AddPrim(D_80092938, pActor + (D_800928A0 * 12 + 0x18));
}

typedef struct {
    /* 0x00 */ u32 unk0;
    /* 0x04 */ u32 unk4;
    /* 0x08 */ u32 unk8;
    /* 0x0C */ u32 unkC;
    /* 0x10 */ u32 unk10;
    /* 0x14 */ u32 unk14;
    /* 0x18 */ u32 unk18;
    /* 0x1C */ u32 unk1C;
    /* 0x20 */ u32 unk20;
    /* 0x24 */ u32 unk24;
} Misc14Reloc; // Size: 0x28

extern void func_8008A040(void *pImg);

Misc14Reloc *func_8008AF6C(Misc14Reloc *pData) {
    s32 delta = (s32)pData - (s32)pData->unk1C;
    u32 *pSlot;
    u32 i;

    pData->unk1C = (u32)pData;
    pData->unk0 += delta;
    pData->unk4 += delta;
    pData->unk10 += delta;
    pData->unk14 += delta;
    pData->unk18 += delta;
    pData->unk20 += delta;
    pData->unk24 += delta;
    if (pData->unkC != 0) {
        pData->unkC += delta;
        func_8008A040((void *)pData->unkC);
    }
    if (pData->unk8 != 0) {
        pData->unk8 += delta;
        for (i = 1; i < ((u32 *)pData->unk8)[0] + 1; i++) {
            pSlot = (u32 *)(i * 4 + pData->unk8);
            if (*pSlot != 0) {
                *pSlot += delta;
            }
        }
    }
    return pData;
}

void func_8008B070(s32 entryIndex) {
    void *pBuffer;

    HeapSetCurrentContentType(0xA);
    pBuffer = HeapAlloc(ArchiveDecodeSize(entryIndex), 0);
    ArchiveReadFileToBuffer(entryIndex, pBuffer, 0, 0);
    ArchiveCdDataSync(0);
    func_8008AF6C(pBuffer);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008B0D8);

/* Best attempt: objdiff 99.80%, same 100 bytes. The instruction order and the
   registers ($a1 = entry, $v1 = entry+0x10, $v0 = value, $a2 = i) all match;
   the only difference is which derived giv gcc re-bases onto: the original uses
   entry+0x10 (sh at -4/-2/0, sw at -0xC), gcc entry+4 (sh at 8/10/12, sw at 0).
   Both serve exactly four refs, so this is a loop.c combine_givs tie. Confirmed
   rule: the base is the address of the body's LAST memory reference, so putting
   `unk4 = unk0` first flips the base to entry+0x10 (correct!) but moves its lw/sw
   to the top of the body, and putting `unk4 = value` last flips it back to
   entry+4. A separate s16 *pField = entry+0x10 pointer gives 75.84% and 112
   bytes (two strength-reduced bivs). An indexed pEntry[i] loop gives 88.24% and
   92 bytes (single base, no derived giv). */
/*
void func_8008B0D8(u8 *pActor) {
    Misc14Entry *pEntry = *(Misc14Entry **)(pActor + 8);
    s32 i;
    s32 value;

    *(s16 *)(pActor + 0x10) = 0;
    *(s16 *)(pActor + 0x12) = 0;
    for (i = 0; i < *(s16 *)(*(u8 **)pActor + 6); i++) {
        pEntry->unkC = 0;
        pEntry->unkE = 0;
        value = pEntry->unk0;
        pEntry->unk10 = 0;
        pEntry->unk4 = value;
        pEntry++;
    }
}
*/

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
    if (diff != 0) {
        if (diff < 0x800) {
            value = arg0 - diff / arg2;
        } else {
            value = arg0 + (0x1000 - diff) / arg2;
        }
    } else {
        value = arg0;
    }
    return value;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008B650);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008B730);

typedef struct {
    /* 0x00 */ u32 unk0[0x20];
    /* 0x80 */ u32 unk80;
} Misc14Work; // Size: 0x84

Misc14Work *func_8008BA2C(u32 arg0, u32 arg1, u32 arg2, s32 arg3) {
    Misc14Work *p;
    s32 i;
    u32 value;

    HeapSetCurrentContentType(3);
    p = HeapAlloc(0x84, 2);
    for (i = 0x1F; i >= 0; i--) {
        p->unk0[i] = 0;
    }
    p->unk80 = arg2;
    p->unk0[0x1C] = func_800405E4();
    value = p->unk80 + arg3 * 4;
    p->unk0[0x1F] = arg0;
    p->unk0[4] = arg1;
    p->unk0[0x1D] = value;
    p->unk0[0x1E] = value;
    func_8008BB3C(p);
    return p;
}

void func_8008BAE0(void *pMem) {
    HeapFree(pMem);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008BB00);

/* Best attempt: objdiff 94.29% (also 94.29 under gcc-2.7.2-cdk). Only the
   temporaries differ: the original loads into $t0/$t1, gcc into $v0/$v1; the
   instruction sequence and both symbols are identical. Two locals, direct field
   stores, a struct pointer and an 8-byte struct copy (70.71%, it also bases the
   second load off one symbol) all give $v0/$v1. */
/*
void func_8008BB00(Misc14Entry *pDst) {
    pDst->unk0 = D_80096D88;
    pDst->unk4 = D_80096D8C;
}
*/

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008BB1C);

/* Best attempt: objdiff 97.50% (97.5 under gcc-2.6.0 and -cdk too) - identical
   except that the original keeps the one reused temporary in $t0, gcc in $v0.
   Reusing a single temp is what reproduces the interleaved load/store order;
   two separate statements make gcc hoist the second load (73.12%). */
/*
void func_8008BB1C(Misc14Entry *pSrc) {
    u32 value;

    value = pSrc->unk0;
    D_80096D88 = value;
    value = pSrc->unk4;
    D_80096D8C = value;
}
*/

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008BB3C);
