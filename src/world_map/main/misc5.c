#include "common.h"
#include "psyq/libgte.h"

typedef struct {
    /* 0x00 */ u8 unk0[0x4];
    /* 0x04 */ s16 unk4;
    /* 0x06 */ u8 unk6[0x1A];
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
    /* 0x5C */ u8 unk5C[0x20];
    /* 0x7C */ u32 unk7C;
} WorldMapPlayer; // Size: 0x80

typedef struct {
    /* 0x000 */ u8 unk0[0x18];
    /* 0x018 */ SVECTOR unk18;
    /* 0x020 */ MATRIX unk20;
    /* 0x040 */ u8 unk40[0x110];
    /* 0x150 */ u16 unk150;
    /* 0x152 */ u8 unk152[0x16];
    /* 0x168 */ SVECTOR unk168;
    /* 0x170 */ MATRIX unk170;
    /* 0x190 */ u8 unk190[0x14];
    /* 0x1A4 */ u16 unk1A4;
    /* 0x1A6 */ u8 unk1A6[0x16];
    /* 0x1BC */ SVECTOR unk1BC;
    /* 0x1C4 */ MATRIX unk1C4;
    /* 0x1E4 */ u8 unk1E4[0x110];
    /* 0x2F4 */ u16 unk2F4;
    /* 0x2F6 */ u8 unk2F6[0x16];
    /* 0x30C */ SVECTOR unk30C;
    /* 0x314 */ MATRIX unk314;
    /* 0x334 */ u8 unk334[0x14];
    /* 0x348 */ s16 unk348;
    /* 0x34A */ u8 unk34A[0x16];
    /* 0x360 */ SVECTOR unk360;
    /* 0x368 */ MATRIX unk368;
} WorldMapWork; // Size: 0x388

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

/* best 95.26%: instruction multiset and registers identical to the original,
   only the emission order of the independent loads differs (gcc hoists
   lhu D_8009A4D8 and defers sll $4,$4,1). A pointer local instead gives
   65.26% with the same multiset.

int func_8007C36C(int idx) {
    D_8009BE24[idx].unk50 = 0;
    D_8009BE24[idx].unk20 = D_8009A4D8;
    D_8009BE24[idx].unk22 = D_8009A4E8[D_8009BE24[idx].unk50];
    D_8009BE24[idx].unk50++;
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007C36C);

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007C3B8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007C724);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007C7D8);
int func_8007CC6C(int idx) {
    WorldMapWork *pWork;
    WorldMapPlayer *pPlayer;

    func_800848B4(4, 0);
    func_800848B4(4, 2);
    func_800848B4(4, 1);
    func_800848B4(4, 3);
    pWork = D_8009C620;
    pWork->unk150 = 0;
    pWork->unk168.vx = pWork->unk168.vy = pWork->unk168.vz = 0;
    RotMatrixYXZ(&pWork->unk168, &pWork->unk170);
    pPlayer = D_8009BE24 + idx;
    pPlayer->unk40 = -0x4000;
    pPlayer->unk28 = 0xD00000;
    pPlayer->unk3C = 0;
    pPlayer->unk38 = 0;
    pPlayer->unk2C = 0;
    pPlayer->unk30 = 0x400000;
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007CD20);
int func_8007CE84(int idx) {
    WorldMapWork *pWork;
    WorldMapPlayer *pPlayer;

    func_800848B4(5, 6);
    pWork = D_8009C620;
    pWork->unk1A4 = 0;
    pWork->unk1BC.vx = pWork->unk1BC.vy = pWork->unk1BC.vz = 0;
    RotMatrixYXZ(&pWork->unk1BC, &pWork->unk1C4);
    pPlayer = D_8009BE24 + idx;
    pPlayer->unk40 = -0x4000;
    pPlayer->unk28 = 0xB00000;
    pPlayer->unk20 = 0;
    pPlayer->unk3C = 0;
    pPlayer->unk38 = 0;
    pPlayer->unk2C = 0;
    pPlayer->unk30 = 0x200000;
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc5", func_8007CF18);
int func_8007D078(int idx) {
    WorldMapWork *pWork;
    WorldMapPlayer *pPlayer;

    func_800848B4(9, 7);
    func_800848B4(9, 8);
    pWork = D_8009C620;
    pWork->unk2F4 = 0;
    pWork->unk30C.vx = pWork->unk30C.vy = pWork->unk30C.vz = 0;
    RotMatrixYXZ(&pWork->unk30C, &pWork->unk314);
    pPlayer = D_8009BE24 + idx;
    pPlayer->unk40 = -0x4000;
    pPlayer->unk3C = 0;
    pPlayer->unk38 = 0;
    pPlayer->unk28 = 0xC00000;
    pPlayer->unk2C = 0;
    pPlayer->unk30 = 0;
    return 1;
}
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
