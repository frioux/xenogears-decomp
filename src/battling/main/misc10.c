#include "common.h"
#include "psyq/libgpu.h"
#include "system/memory.h"

// .sdata/.bss of the overlay (supplied by splat's asm segments)
extern u8 D_800928A0;
extern u8 D_80092920;
extern u8 D_80099D9D;
extern u8 D_80099D9E;
extern s32 D_80059488;
extern u8* D_80092938;
extern u8* D_80092784;


extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);
extern void func_8008E120(void);

typedef struct {
    /* 0x0 */ u16 unk0;
    /* 0x2 */ u16 pad2;
    /* 0x4 */ s32 unk4;
} Misc10Resource; // Size: 0x8

typedef struct {
    /* 0x00 */ u8 unk00[0x310];
} Misc10Prim; // Size: 0x310

extern Misc10Prim D_8009A300[];

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_80084A64);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_80084AE0);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_80084B48);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_80084BEC);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_80084C88);

void func_80084FD0(void) {
    if (D_80092784 != NULL && (D_80059488 & 0x1)) {
        func_8008E120();
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_80085014);

void func_80085070(void) {
    u8 flags = D_80092920;

    D_80099D9E = 0;
    D_80099D9D = 0;
    D_80092920 = flags | 0x1;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_8008509C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_80085134);

void func_8008518C(Misc10Resource* pResource, u_int allocFlags) {
    pResource->unk4 = (s32)HeapAlloc(ArchiveDecodeAlignedSize(pResource->unk0), allocFlags);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_800851D4);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_80085264);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_800852C4);

void func_80085E34(u16* pSrc, u16* pDst) {
    pDst[0] = pSrc[0] + 0x18;
    pDst[1] = pSrc[1] + 0x6;
    pDst[0] += 0x4F;
}

void func_80085E60(u16* pSrc, u16* pDst) {
    pDst[0] = 0x8B - pSrc[0];
    pDst[1] = 0x20 - pSrc[1];
    pDst[0] += 0x4F;
}

void func_80085E90(s32 flag, u16* pDst, s32 value) {
    if (flag != 0) {
        *pDst = 0xDA - value;
    } else {
        *pDst = 0x67 + value;
    }
}

void func_80085EAC(s32 flag, u16* pDst, s32 value) {
    if (flag != 0) {
        *pDst = 0x20 - value;
    } else {
        *pDst = 0x6 + value;
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_80085EC8);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_800864B4);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_800866D4);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_800868E0);

void func_80086E24(void) {
    AddPrim(D_80092938, &D_8009A300[D_800928A0]);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_80086E70);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_80086FF8);
