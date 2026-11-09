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
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_800914D0);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_80091B54);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_80091C18);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_80091FF8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_80092234);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_800922AC);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_800923A8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_800925A0);
