#include "common.h"
#include "main/game.h"

extern void func_800848B4(int, int);
extern u8 D_8009BD60;
extern s16 D_8009BD24;
extern u8 D_8009D738;
extern void *D_8009D7D8;
extern u32 D_8009B6C4[3];
extern u16 D_8009B6D0;
extern u32 D_8009B6D4[3];
extern u16 D_8009B6E0;
extern u16 D_8009CD4C;
extern s32 D_8009CEC0;
extern s32 D_8009C7E8;
extern s32 D_8009BD34;
extern s32 D_8009BE0C;
extern s32 D_8009BE10;
extern s32 D_8009D3F0;
extern s16 D_8009BD38;
extern s16 D_8009BD3A;
extern s16 D_8009BD3C;
extern u16 D_8009D52C;
extern u32 D_8009D55C[];
extern u32 D_8009B224[];
extern u32 D_8009B22C[];
extern u32 D_8009B234[];
extern u32 D_8009B23C[];

typedef struct {
    /* 0x00 */ u8 unk0[0x20];
    /* 0x20 */ u16 unk20;
    /* 0x22 */ u8 unk22[0x6];
    /* 0x28 */ u32 unk28;
    /* 0x2C */ u32 unk2C;
    /* 0x30 */ u32 unk30;
    /* 0x34 */ u32 unk34;
    /* 0x38 */ u8 unk38[0x18];
    /* 0x50 */ u32 unk50;
    /* 0x54 */ u8 unk54[0x4];
    /* 0x58 */ u32 unk58;
    /* 0x5C */ u8 unk5C[0x8];
    /* 0x64 */ void *unk64;
    /* 0x68 */ void *unk68;
    /* 0x6C */ u8 unk6C[0x14];
} WorldMapPlayer; // Size: 0x80

extern WorldMapPlayer *D_8009BE24;

typedef struct {
    /* 0x0 */ u32 w[4];
} Quad; // Size: 0x10

void func_8008DFF4(s32 *pOut) {
    pOut[0] = g_GameState.unk182C << 12;
    pOut[1] = g_GameState.unk182E << 12;
    pOut[2] = g_GameState.unk1830 << 12;
}

void func_8008E034(s32 *pIn) {
    g_GameState.unk182C = pIn[0] >> 12;
    g_GameState.unk182E = pIn[1] >> 12;
    g_GameState.unk1830 = pIn[2] >> 12;
}

void func_8008E078(void) {
    u16 nEntry;
    void *pRect;

    if (D_8009D738 == 0) {
        return;
    }
    switch (D_8009BD60) {
    case 0xF:
        nEntry = D_8009B6D0;
        pRect = D_8009B6C4;
        break;
    case 0x10:
        nEntry = D_8009B6E0;
        pRect = D_8009B6D4;
        break;
    default:
        return;
    }
    D_8009D7D8 = pRect;
    D_8009BD24 = nEntry;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_8008E0F0);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_8008E190);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_8008E4F4);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_8008E680);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_8008E76C);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_800906E0);

int func_800907C4(void) {
    func_800848B4(0, 2);
    func_800848B4(0, 3);
    return 1;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_800907F4);
void func_80090A18(void) {
    u16 keys;
    s32 state;

    keys = D_8009CD4C;
    if ((keys & 1) && (keys & 2)) {
        D_8009CEC0 = 1;
    } else {
        D_8009CEC0 = 0;
    }
    state = D_8009CEC0;
    D_8009BD34 = (state ^ D_8009C7E8) & state;
    D_8009C7E8 = state;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_80090A84);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_80090C68);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_80090E14);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_80090FB4);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_80091430);
/* best: 88.22%. The 4-word copy at 0x28 and the tail match; retail re-reads
   D_8009BD3A with lh (and keeps its address in v1 across the unk50 store) where
   gcc forwards the value it just stored there, so the address is used once and
   is emitted as lui at. Also the D_8009BE10 load sits one statement later.

s32 func_80091430(s32 idx) {
    WorldMapPlayer *pPlayer;
    s32 nMode;

    pPlayer = D_8009BE24 + idx;
    *(Quad *)&pPlayer->unk28 = *(Quad *)D_8009D55C;
    D_8009BD3C = 0;
    D_8009BD3A = D_8009D52C;
    pPlayer->unk50 = (s16)D_8009D52C;
    pPlayer->unk58 = D_8009BD3A << 12;
    nMode = D_8009BE10;
    if (nMode < 8) {
        if (nMode >= 6) {
            pPlayer->unk20 = 3;
        }
    }
    return 1;
}
*/
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_800914D0);
s32 func_80091B54(s32 idx) {
    WorldMapPlayer *pPlayer;
    s32 nMode;

    pPlayer = D_8009BE24 + idx;
    nMode = D_8009BE10;
    switch (nMode) {
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
        pPlayer->unk50 = 3;
        D_8009D3F0 = 0x460000;
        D_8009BD38 = -0x260;
        pPlayer->unk64 = D_8009B224;
        pPlayer->unk68 = D_8009B234;
        break;
    case 6:
        break;
    case 7:
        pPlayer->unk50 = 3;
        D_8009D3F0 = 0x280000;
        D_8009BD38 = -0x260;
        pPlayer->unk64 = D_8009B22C;
        pPlayer->unk68 = D_8009B23C;
        break;
    }
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_80091C18);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_80091FF8);
s32 func_80092234(s32 idx) {
    WorldMapPlayer *pPlayer;
    s32 nMode;

    pPlayer = D_8009BE24 + idx;
    nMode = D_8009BE10;
    if (nMode > 0) {
        if (nMode < 6) {
            D_8009BE0C = 0x8C;
        } else if (nMode < 8) {
            D_8009BE0C = 0x78;
            pPlayer->unk20 = 1;
        }
    }
    pPlayer->unk50 = D_8009BE0C << 12;
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_800922AC);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_800923A8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_800925A0);
