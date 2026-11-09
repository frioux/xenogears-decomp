#include "common.h"
#include "psyq/libetc.h"
#include "psyq/libgpu.h"
#include "psyq/memory.h"
#include "system/memory.h"

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
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_800837DC);

s32 func_800838E8(s32 arg0) {
    D_8009BE24[arg0].unk50 = (u32)D_8009AC60;
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc8", func_8008390C);
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
