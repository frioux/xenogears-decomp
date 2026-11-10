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
