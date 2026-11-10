#include "common.h"
#include "psyq/libetc.h"
#include "psyq/libgpu.h"
#include "psyq/memory.h"

typedef struct {
    /* 0x00 */ u32 unk0;
    /* 0x04 */ s16 unk4;
    /* 0x06 */ u8 unk6[0x1A];
    /* 0x20 */ s16 unk20;
    /* 0x22 */ u16 pad22;
    /* 0x24 */ u32 unk24;
    /* 0x28 */ s32 unk28;
    /* 0x2C */ s32 unk2C;
    /* 0x30 */ s32 unk30;
    /* 0x34 */ u8 unk34[0x1C];
    /* 0x50 */ u32 unk50;
    /* 0x54 */ u8 unk54[0x2C];
} WorldMapActor; // Size: 0x80

typedef struct {
    /* 0x0 */ u8 r;
    /* 0x1 */ u8 g;
    /* 0x2 */ u8 b;
    /* 0x3 */ u8 a;
} FadeParams; // Size: 0x4, alignment 1

typedef struct WorldMapEntry {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ u16 pad2;
    /* 0x04 */ u16 unk4;
    /* 0x06 */ u16 pad6;
    /* 0x08 */ s32 unk8;
    /* 0x0C */ s32 unkC;
    /* 0x10 */ s32 unk10;
    /* 0x14 */ u8 unk14[0x2C];
    /* 0x40 */ u8 *unk40;
    /* 0x44 */ u32 unk44;
    /* 0x48 */ u8 *unk48;
    /* 0x4C */ u8 *unk4C;
    /* 0x50 */ struct WorldMapEntry *unk50;
} WorldMapEntry; // Size: 0x54

extern WorldMapActor *D_8009BE24; // actors, 0x80 apart
extern WorldMapEntry *D_8009C620; // sprite sets, 0x54 apart
extern s16 D_8009D7E0; // D_8009C620 entry count
extern u8 D_8009AC60[];
extern u8 D_8009C5AC[];

extern void ModelPartFreeLightData(void *pModelPart);
extern int Vsync(int mode);
extern void func_80097D64(void);
extern void func_800967E4(void);
extern void func_80097BC0(void *p);
extern s32 func_80096668(void);
extern s16 func_80084DB8(void *pArg, s16 arg1);
extern void func_80083108(WorldMapEntry *pEntry, u8 *pDst, s32 count, s32 arg3);
extern s32 D_8009C5B0;
extern s32 D_8009C5B4;
extern s32 D_8009BE28;
extern s32 D_8009BE2C;
extern s32 D_8009BE30;
extern s32 D_8009D55C;
extern s32 D_8009D560;
extern s32 D_8009D564;
extern s16 D_8009BD38;
extern s16 D_8009BD3A;
extern s16 D_8009BD3C;
extern s32 D_8009D144;
extern s32 D_8009D3F0;
extern s32 D_8009BE0C;
extern void HeapFree(void *pMem);
extern void *HeapAlloc(u_int allocSize, u_int allocFlags);
extern void *D_8006259C;
extern s32 D_8009BC38;
extern s32 D_8009BCB0;
extern s32 D_8009BC3C;
extern s32 D_8009BCB4;
extern s32 D_8009C180;
extern s16 D_8006F94E;
extern s16 D_8006F950;
extern s16 D_8006F954;
extern s32 D_8009BBC4;
extern void func_80039FF8(void);
extern void func_8003852C(void *p);
extern void func_80086568(void);
extern void func_800866C8(void);
extern void func_80074F04(void);
extern void func_800750DC(void);
extern void func_80088FF4(void);
extern void func_80089128(void);
extern void func_800976A0(void);
extern void func_80084818(void);
extern void func_8002DD20(void *pBlock);
extern void func_800931D8(u8 *pSrc, u8 *pDst, s32 count, u8 *pFactors);
extern void *D_8009BD20;
extern u16 D_8009BCE0[];
extern FadeParams D_800704DC;

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_80082F64);

void func_80083108(WorldMapEntry *pEntry, u8 *pDst, s32 count, s32 arg3) {
    s32 i;

    for (i = count - 1; i != -1; i--) {
        pDst[3] = 7;
        pDst[7] = 0x24;
        *(u16 *)(pDst + 0x16) = GetTPage(0, arg3, 0x2C0, 0x100);
        pDst[4] = 0;
        pDst[5] = 0;
        pDst[6] = 0;
        pDst[7] |= 2;
        pDst += 0x20;
    }
    memcpy(pEntry->unk4C, pEntry->unk48, count << 5);
}

void func_800831D8(u8 *pData, s32 count, u8 r, u8 g, s32 b) {
    s32 i;

    for (i = count - 1; i != -1; i--) {
        pData[4] = r;
        pData[5] = g;
        pData[6] = b;
        pData += 0x20;
    }
}

s32 func_80083214(s32 arg0) {
    WorldMapEntry *pEntry = &D_8009C620[arg0 + 78];

    func_80083108(pEntry, pEntry->unk48, *(u16 *)(pEntry->unk40 + 4), 3);
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_80083264);

s32 func_800834D0(void) {
    return 1;
}

s32 func_800834D8(s32 arg0) {
    WorldMapActor *pActor = &D_8009BE24[arg0];

    if (pActor->unk4 == 1) {
        pActor->unk4 = 0;
        DrawSync(0);
        Vsync(0);
        func_80097D64();
        func_80097BC0(D_8009C5AC);
        do {
            func_800967E4();
            Vsync(0);
        } while (func_80096668() > 0);
    }
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_8008355C);
s32 func_800837DC(void) {
    u16 v;

    func_80039FF8();
    func_8003852C(D_8006259C);
    HeapFree(D_8006259C);
    func_80084818();
    func_80086568();
    func_800866C8();
    func_80074F04();
    func_800750DC();
    func_80088FF4();
    func_80089128();
    func_80097D64();
    HeapFree(D_8009BC38);
    HeapFree(D_8009BCB0);
    HeapFree(D_8009BC3C);
    HeapFree(D_8009BCB4);
    HeapFree(D_8009C180);
    func_800976A0();
    v = *(u16 *)&D_8009BD3A;
    D_8006F94E = 0x269;
    D_8006F954 = 4;
    D_8009BBC4 = 1;
    D_8006F950 = v;
    return 1;
}

s32 func_800838E8(s32 arg0) {
    D_8009BE24[arg0].unk50 = (u32)D_8009AC60;
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_8008390C);
// Best: 84.75%. Every instruction and relocation matches except that gcc
// folds the stores to D_8009BD3A (0x400) and D_8009BD3C (0) into the later
// reads of them (lui $v0,0x40 instead of lui/lh), so retail's compiler did
// not forward store to load here. -O1 scores 67.46.
/*
s32 func_8008390C(s32 arg0) {
    WorldMapActor *pActor = &D_8009BE24[arg0];
    u8 *p = (u8 *)pActor;
    s32 v;

    *(s32 *)(p + 0x7C) = 0x1000;
    v = *(s32 *)D_8009C5AC;
    D_8009BE28 = v;
    D_8009D55C = v;
    pActor->unk28 = v;
    v = D_8009C5B0;
    D_8009BE2C = v;
    D_8009D560 = v;
    pActor->unk2C = v;
    v = D_8009C5B4;
    D_8009BE30 = v;
    D_8009D564 = v;
    pActor->unk30 = v;
    pActor->unk20 = 0;
    pActor->unk4 = 0;
    *(s32 *)(p + 0x5C) = 0x960000;
    D_8009BD38 = -0x20;
    D_8009BD3A = 0x400;
    D_8009BD3C = 0;
    *(s32 *)(p + 0x50) = -0x20000;
    *(s32 *)(p + 0x38) = -0x20000;
    v = D_8009BD3A;
    D_8009D144 = 0;
    D_8009D3F0 = 0x960000;
    *(s32 *)(p + 0x54) = v << 12;
    *(s32 *)(p + 0x3C) = v << 12;
    v = D_8009BD3C;
    D_8009BE0C = 0x78;
    *(s32 *)(p + 0x58) = v << 12;
    *(s32 *)(p + 0x40) = v << 12;
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_80083A00);

s32 func_80083FE4(s32 arg0) {
    WorldMapActor *pActor = &D_8009BE24[arg0];
    s32 x;
    s32 y;
    s32 z;

    pActor->unk28 = 0x1800000;
    pActor->unk2C = 0x80000;
    pActor->unk30 = 0x1A00000;
    pActor->unk20 = 0;
    D_8009C620[8].unk0 = 1;
    D_8009C620[7].unk0 = 1;
    x = pActor->unk28 >> 12;
    D_8009C620[8].unk8 = x;
    D_8009C620[7].unk8 = x;
    y = pActor->unk2C >> 12;
    D_8009C620[8].unkC = y;
    D_8009C620[7].unkC = y;
    z = pActor->unk30 >> 12;
    D_8009C620[8].unk10 = z;
    D_8009C620[7].unk10 = z;
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_80084068);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_8008440C);
// Best: 90.19%. Structure is right (the unaligned 4-byte copy of the
// alignment-1 FadeParams at D_800704DC into sp+0x18, the RECT at sp+0x10,
// 0x200/0x2000 HeapAllocs, func_800931D8 factors argument, the 16 GetClut
// loop). Remaining diff is allocation/schedule: retail hoists `i = 0`
// into the LZSS delay slot and computes &D_8009BCE0 late, ours does the
// reverse; retail ends with i in s5 and pClut in s2. Declaration order had
// no effect (six permutations, all 90.19).
/*
void func_8008440C(void) {
    RECT rect;
    FadeParams factors = D_800704DC;
    u8 *pData;
    u8 *pDst;
    u16 *pClut;
    s32 i;

    pData = LZSSHeapDecompress(D_8009BD20, 1);
    func_8002DD20(pData);
    DrawSync(0);
    HeapFree(pData);
    HeapFree(D_8009BD20);
    pData = HeapAlloc(0x200, 1);
    pDst = HeapAlloc(0x2000, 1);
    rect.x = 0;
    rect.y = 0x1F0;
    rect.w = 0x100;
    rect.h = 1;
    StoreImage(&rect, (u_long *)pData);
    DrawSync(0);
    func_800931D8(pData, pDst, 0x10, (u8 *)&factors);
    rect.x = 0;
    rect.y = 0x1F0;
    rect.w = 0x100;
    rect.h = 0xF;
    LoadImage(&rect, (u_long *)pDst);
    DrawSync(0);
    pClut = D_8009BCE0;
    for (i = 0; i < 0x10; i++, pClut++) {
        *pClut = GetClut(rect.x, rect.y);
        rect.y++;
    }
    HeapFree(pDst);
    HeapFree(pData);
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_80084580);

void func_80084818(void) {
    s32 i;

    for (i = 0; i < D_8009D7E0; i++) {
        HeapFree(D_8009C620[i].unk48);
        ModelPartFreeLightData(D_8009C620[i].unk40);
    }
    HeapFree(D_8009C620);
}

void func_800848B4(s32 arg0, s32 arg1) {
    D_8009C620[arg1].unk50 = &D_8009C620[arg0];
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_800848F4);

s32 func_80084D00(void *pArg, s16 *pOut) {
    WorldMapEntry *pEntry = D_8009C620;
    s16 result;
    s16 i;

    for (i = 0; i < D_8009D7E0; i++) {
        if ((pEntry->unk4 & 1) && (result = func_80084DB8(pArg, i)) != 0) {
            *pOut = i;
            return result;
        }
        pEntry++;
    }
    return 0;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_80084DB8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_80085158);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_80085418);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_80085760);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_80085CDC);
