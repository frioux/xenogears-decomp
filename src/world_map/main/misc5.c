#include "common.h"
#include "psyq/libgte.h"

typedef struct {
    /* 0x00 */ u8 unk0[0x20];
    /* 0x20 */ u16 unk20;
    /* 0x22 */ u16 unk22;
    /* 0x24 */ u8 unk24[0x4];
    /* 0x28 */ u32 unk28;
    /* 0x2C */ u32 unk2C;
    /* 0x30 */ u32 unk30;
    /* 0x34 */ u8 unk34[0x4];
    /* 0x38 */ u32 unk38;
    /* 0x3C */ u32 unk3C;
    /* 0x40 */ u32 unk40;
    /* 0x44 */ u8 unk44[0xC];
    /* 0x50 */ u32 unk50;
    /* 0x54 */ u32 unk54;
    /* 0x58 */ u32 unk58;
    /* 0x5C */ u8 unk5C[0x24];
} WorldMapPlayer; // Size: 0x80

typedef struct {
    /* 0x000 */ u8 unk0[0x18];
    /* 0x018 */ SVECTOR unk18;
    /* 0x020 */ MATRIX unk20;
    /* 0x040 */ u8 unk40[0x308];
    /* 0x348 */ s16 unk348;
    /* 0x34A */ u8 unk34A[0x16];
    /* 0x360 */ SVECTOR unk360;
    /* 0x368 */ MATRIX unk368;
} WorldMapWork;

extern WorldMapPlayer *D_8009BE24;
extern WorldMapWork *D_8009C620;
extern u16 D_8009A4D8;
extern u16 D_8009A4E8[];
extern void func_800848B4(int, int);

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007A9F8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007AD34);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007ADD4);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007B200);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007B394);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007B604);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007B798);

int func_8007BA08(void) {
    return 3;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007BA10);

int func_8007BB60(int idx) {
    WorldMapWork *pWork;
    WorldMapPlayer *pPlayer;

    func_800848B4(0, 1);
    func_800848B4(0, 2);
    func_800848B4(0, 3);
    pWork = D_8009C620;
    pPlayer = D_8009BE24 + idx;
    pPlayer->unk50 = 0;
    pPlayer->unk54 = 0x100;
    pPlayer->unk58 = 0x100;
    pWork->unk18.vx = 0;
    pWork->unk18.vy = 0;
    pWork->unk18.vz = 0;
    RotMatrixYXZ(&pWork->unk18, &pWork->unk20);
    return 3;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007BBEC);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007BF50);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007C260);

int func_8007C36C(int idx) {
    WorldMapPlayer *pBase = D_8009BE24;
    WorldMapPlayer *pPlayer = pBase + idx;

    pPlayer->unk50 = 0;
    pPlayer->unk20 = D_8009A4D8;
    pPlayer->unk22 = D_8009A4E8[pPlayer->unk50];
    pPlayer->unk50++;
    return 1;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007C3B8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007C724);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007C7D8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007CC6C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007CD20);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007CE84);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007CF18);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007D078);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007D110);

int func_8007D228(int idx) {
    WorldMapWork *pWork;
    WorldMapPlayer *pPlayer;

    func_800848B4(0xA, 0xB);
    pWork = D_8009C620;
    pWork->unk348 = 0;
    pWork->unk360.vx = pWork->unk360.vy = pWork->unk360.vz = 0;
    RotMatrixYXZ(&pWork->unk360, &pWork->unk368);
    pPlayer = D_8009BE24 + idx;
    pPlayer->unk40 = -0x4000;
    pPlayer->unk28 = 0xE00000;
    pPlayer->unk3C = 0;
    pPlayer->unk38 = 0;
    pPlayer->unk2C = 0;
    pPlayer->unk30 = 0x100000;
    return 1;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007D2B8);
