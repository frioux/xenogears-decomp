#include "common.h"
#include "psyq/libgpu.h"
#include "system/memory.h"
#include "system/archive.h"

// .sdata/.bss of the overlay (supplied by splat's asm segments)
extern u8 D_800928A0;
extern s16 D_80092780;
extern s32 D_80092794;
extern s32 D_800928C8;
extern u8 D_8005061C;
extern u8* D_800927B4[];
extern s32 D_800927C4;
extern s32 D_800917F0;
extern s32 D_80092948;
extern u8 D_80097919;
extern u8 D_80099035;
extern u8 D_80091FB0[];
extern u8 D_80092920;
extern u8 D_80099D9D;
extern u8 D_80099D9E;
extern s32 D_80059488;
extern u8* D_80092938;
extern u8* D_80092784;


extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);
extern void ArchiveCdDataSync(s32 mode);
extern void func_8008E120(void);
extern void func_8007F258(void* pOt, s32 arg);
extern void func_80080D20(void* pOt);
extern void func_8008E3CC(void* pOt, s32 arg1, s32 arg2);
extern void func_8008BC04(void);
extern void func_80083BB4(s32 arg);
extern void func_80083C0C(s32 arg);
extern void func_8003852C(s32 arg);
extern void func_80039C4C(s32 arg);
extern void func_800399D4(s32 arg);
extern void func_80088A40(void);
extern void ChangeGameState(s32 state);
extern void Vsync(s32 mode);
extern void MainLoop(s32 arg);
extern s32 func_80089C54(void);
extern s32 func_80089FC4(void);
extern void func_80089E2C(void* pA, void* pB);
extern void func_8008A184(void* pA, void* pB);
extern void func_80089C88(s32 handle, void* pB);
extern void* func_800891C0(s32 arg);
extern void func_8008AE1C(void* pData);
extern s32 func_8008F530(s32 arg0, s32 arg1);
void func_80086E24(void);

typedef struct {
    /* 0x000 */ u8 unk000[0x284];
    /* 0x284 */ u32* unk284;
} Misc10Actor; // Size: unknown

typedef struct {
    /* 0x00 */ u8 unk00[0x44];
    /* 0x44 */ s16 unk44;
    /* 0x46 */ s16 unk46;
    /* 0x48 */ s16 unk48;
} Misc10Draw; // Size: unknown

typedef struct {
    /* 0x0 */ u16 unk0;
    /* 0x2 */ u16 pad2;
    /* 0x4 */ s32 unk4;
} Misc10Resource; // Size: 0x8

typedef struct {
    /* 0x00 */ u8 unk00[0x310];
} Misc10Prim; // Size: 0x310

extern Misc10Prim D_8009A300[];

s32 func_80084A64(Misc10Actor* pActor) {
    u32* pData = pActor->unk284;

    func_8008E3CC((u8*)pData[D_800928A0 + 1] + 8, 0xC0, 0);
    func_80080D20((u8*)pData[D_800928A0 + 1] + 4);
    func_8008AE1C(pData);
    func_80086E24();
    return 0;
}

void func_80084AE0(void) {
    if (D_80092780 != 0) {
        func_8008E120();
        func_8007F258(D_80092938, 0);
        func_80080D20(D_80092938);
        func_8008E3CC(D_80092938, 0xC0, 1);
        func_8008BC04();
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_80084B48);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_80084BEC);

/* 97.56% - instruction multiset identical, only the s0/s1/s2 assignment is
   rotated (a global-alloc tie-break; three declaration orders tried).
void func_80084BEC(Misc10Actor* pActor) {
    Misc10Draw* pDraw;
    u8* pScript;
    s32 arg0;

    pScript = *(u8**)((u8*)pActor + 0x5C);
    pScript = *(u8**)(pScript + 4);
    pScript = *(u8**)(pScript + 4);
    arg0 = *(s32*)(pScript + 0x30);

    pDraw = (Misc10Draw*)func_80089C54();
    pScript = (u8*)func_80089FC4();
    func_80089E2C(pDraw, pScript);
    func_8008A184(pScript, D_80091FB0);
    func_80089C88(arg0, pDraw);
    pDraw->unk46 = 0xC00;
    pDraw->unk44 = 0;
    pDraw->unk48 = 0x400;
}
*/

INCLUDE_ASM("asm/battling/nonmatchings/main/misc10", func_80084C88);

void func_80084FD0(void) {
    if (D_80092784 != NULL && (D_80059488 & 0x1)) {
        func_8008E120();
    }
}

void func_80085014(void) {
    D_80092920 &= 0xFE;
    func_80083BB4(0);
    func_80083C0C(5);
    D_80099D9E = 1;
    D_80099D9D = 1;
    D_800928C8 = 6;
}

void func_80085070(void) {
    u8 flags = D_80092920;

    D_80099D9E = 0;
    D_80099D9D = 0;
    D_80092920 = flags | 0x1;
}

void func_8008509C(s32 index, s32 value) {
    if (index != 0) {
        D_80097919 = value;
    } else {
        D_80099035 = value;
    }
    if (D_800927B4[index] != NULL) {
        HeapFree(D_800927B4[index]);
        D_800927B4[index] = NULL;
    }
    ArchiveSetIndex(0x30, 1);
    D_800927B4[index] = func_800891C0(value + 2);
    ArchiveSetIndex(0x30, 0);
}

void func_80085134(s32 index) {
    ArchiveCdDataSync(0);
    if (D_800927B4[index] != NULL) {
        HeapFree(D_800927B4[index]);
        D_800927B4[index] = NULL;
    }
}

void func_8008518C(Misc10Resource* pResource, u_int allocFlags) {
    pResource->unk4 = (s32)HeapAlloc(ArchiveDecodeAlignedSize(pResource->unk0), allocFlags);
}

void func_800851D4(void) {
    func_8003852C(D_800927C4);
    if (D_800917F0 != 0) {
        func_80039C4C(D_80092948);
        func_800399D4(D_80092948);
    }
    func_80088A40();
    ChangeGameState(1);
    DrawSync(0);
    Vsync(2);
    D_8005061C = 1;
    MainLoop(0);
}

s32 func_80085264(void) {
    s32 result = 0;

    if (D_80092794 == 5 || D_80092794 == 7) {
        result = 1;
    } else if (D_80092794 == 1 && D_800928C8 == 4 && D_8005061C == 0) {
        result = 1;
    }
    return result;
}

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

/* 84.25% - the two value branches each re-materialise `li 0x28000000`; the
   original shares one lui across the branch and keeps the 1 return in v0.
s32 func_80086FF8(s32 arg0, u8* pOut) {
    s32 value;

    if (func_8008F530(arg0, 0) != 0) {
        return 0;
    }
    value = 0x28000000;
    value |= func_8008F530(arg0, 1) ? 0xFFFF : 0xFF;
    pOut[3] = 5;
    *(s32*)(pOut + 4) = value;
    return 1;
}
*/
