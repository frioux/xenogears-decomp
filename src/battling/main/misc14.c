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

extern void func_80030A30(s32 index, void *pLight);
extern void func_8008AF6C(void *pData);
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

/* Best attempt: objdiff 83.44%, same 96 bytes. The original addresses the three
   s16 stores off a second induction pointer ($v1 = entry + 0x10, offsets -4/-2/0)
   and the word copy off the entry pointer; gcc merges them into one base
   (entry + 4). Reordering the body drops it to 69.80%. */
/*
void func_8008B0D8(u8 *pActor) {
    u8 *pModel = *(u8 **)pActor;
    Misc14Entry *pEntry = *(Misc14Entry **)(pActor + 8);
    s32 i;

    *(s16 *)(pActor + 0x10) = 0;
    *(s16 *)(pActor + 0x12) = 0;
    for (i = 0; i < *(s16 *)(pModel + 6); i++) {
        pEntry->unkC = 0;
        pEntry->unkE = 0;
        pEntry->unk10 = 0;
        pEntry->unk4 = pEntry->unk0;
        pEntry++;
    }
}
*/

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008B13C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008B38C);

void func_8008B5DC(void *pA, s32 arg1) {
    func_8008B730(pA, arg1, 1);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008B5FC);

/* Best attempt: objdiff 96.19% and 88 bytes against the original's 84, so it is
   not linkable either. Identical apart from which pseudo holds `value`: the
   original keeps it in $v0 (`move v0,a0` in the beqz delay slot, `mflo v0`,
   `subu v0,a0,v0`), gcc in $a0, which dies at the same point. `return value;`
   at the end and three early `return`s both give the $a0 form. */
/*
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
*/

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

/* Best attempt: objdiff 94.29%. Only the temporaries differ: the original loads
   into $t0/$t1, gcc into $v0/$v1; the instruction sequence and both symbols are
   identical. Two locals, direct field stores and a struct pointer all give
   $v0/$v1. */
/*
void func_8008BB00(Misc14Entry *pDst) {
    pDst->unk0 = D_80096D88;
    pDst->unk4 = D_80096D8C;
}
*/

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008BB1C);

/* Best attempt: objdiff 73.12%. The original stores each field before loading
   the next (`lw t0 / sw t0 / lw t0 / sw t0`, one temporary); gcc hoists the
   second load above the first store (`lw v0 / lw v1 / sw / sw`). */
/*
void func_8008BB1C(Misc14Entry *pSrc) {
    D_80096D88 = pSrc->unk0;
    D_80096D8C = pSrc->unk4;
}
*/

INCLUDE_ASM("asm/battling/nonmatchings/main/misc14", func_8008BB3C);
