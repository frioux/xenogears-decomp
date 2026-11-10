#include "common.h"
#include "main/game.h"
#include "psyq/libgte.h"

extern s32 func_80095414(s32 *pPos, s32 *pRot, s32 *pOut, s32 nSpeed, s32 nMode);
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
    /* 0x00 */ u8 unk0[0x4];
    /* 0x04 */ s16 unk4;
    /* 0x06 */ u8 unk6[0x1A];
    /* 0x20 */ s16 unk20;
    /* 0x22 */ u8 unk22[0x2];
    /* 0x24 */ u16 unk24;
    /* 0x26 */ u8 unk26[0x2];
    /* 0x28 */ s32 unk28;
    /* 0x2C */ s32 unk2C;
    /* 0x30 */ s32 unk30;
    /* 0x34 */ s32 unk34;
    /* 0x38 */ u32 unk38;
    /* 0x3C */ u32 unk3C;
    /* 0x40 */ u32 unk40;
    /* 0x44 */ u32 unk44;
    /* 0x48 */ u16 unk48;
    /* 0x4A */ u16 unk4A;
    /* 0x4C */ u8 unk4C[0x4];
    /* 0x50 */ s32 unk50;
    /* 0x54 */ u32 unk54;
    /* 0x58 */ s32 unk58;
    /* 0x5C */ s32 unk5C;
    /* 0x60 */ u32 unk60;
    /* 0x64 */ void *unk64;
    /* 0x68 */ void *unk68;
    /* 0x6C */ u32 unk6C;
    /* 0x70 */ u8 unk70[0x4];
    /* 0x74 */ u32 unk74;
    /* 0x78 */ u8 unk78[0x8];
} WorldMapPlayer; // Size: 0x80

typedef struct {
    /* 0x000 */ u8 unk0[0x8];
    /* 0x008 */ u32 unk8[4];
    /* 0x018 */ u8 unk18[0x98];
    /* 0x0B0 */ u32 unk0B0[4];
    /* 0x0C0 */ u8 unk0C0[0x44];
    /* 0x104 */ u32 unk104[4];
} WorldMapSource; // Size: 0x114

extern WorldMapPlayer *D_8009BE24;
extern WorldMapSource *D_8009C620;

typedef struct {
    /* 0x0 */ u32 w[4];
} Quad; // Size: 0x10

typedef struct {
    /* 0x00 */ u8 unk0[0x1C];
    /* 0x1C */ u16 unk1C;
    /* 0x1E */ u8 unk1E[0x2];
    /* 0x20 */ MATRIX matrix;
    /* 0x40 */ u8 unk40[0x14];
} WorldMapRot; // Size: 0x54

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
s32 func_8008E0F0(s32 *pPos, s32 *pDir, s32 nSpeed) {
    s32 nAngle;
    s32 out[4];
    s32 rot[4];

    for (nAngle = 0; nAngle < 0x1000; nAngle += 0x100) {
        rot[0] = rcos(nAngle);
        rot[2] = -rsin(nAngle);
        if (func_80095414(pPos, rot, out, nSpeed, 2) == 1) {
            return nAngle;
        }
    }
    return -1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_8008E190);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_8008E4F4);
s32 func_8008E680(s32 idx) {
    WorldMapPlayer *pPlayer;
    u16 nAngle;

    pPlayer = D_8009BE24 + idx;
    func_8008DFF4(&pPlayer->unk28);
    pPlayer->unk2C = 0x80000;
    nAngle = g_GameState.unk1832;
    pPlayer->unk4A = 0x20;
    pPlayer->unk74 = 3;
    pPlayer->unk68 = (void *)0xFFD80000;
    pPlayer->unk40 = 0;
    pPlayer->unk3C = 0;
    pPlayer->unk38 = 0;
    pPlayer->unk24 = 0;
    pPlayer->unk60 = 0;
    pPlayer->unk64 = 0;
    pPlayer->unk6C = 0;
    pPlayer->unk20 = 8;
    pPlayer->unk48 = nAngle;
    *(Quad *)D_8009D55C = *(Quad *)&pPlayer->unk28;
    D_8009D52C = pPlayer->unk48;
    *(Quad *)D_8009C620->unk8 = *(Quad *)&pPlayer->unk28;
    return 1;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_8008E76C);
s32 func_800906E0(s32 idx) {
    WorldMapPlayer *pPlayer;
    s32 nMode;

    func_800848B4(0, 2);
    func_800848B4(0, 3);
    pPlayer = D_8009BE24 + idx;
    pPlayer->unk20 = 0;
    *(Quad *)&pPlayer->unk28 = *(Quad *)D_8009C620->unk0B0;
    *(Quad *)&pPlayer->unk38 = *(Quad *)D_8009C620->unk104;
    nMode = D_8009BE10;
    pPlayer->unk50 = 0;
    pPlayer->unk54 = 0x40;
    if (nMode < 8) {
        if (nMode >= 4) {
            pPlayer->unk20 = 3;
            pPlayer->unk5C = 0x80;
            pPlayer->unk58 = 0x80;
        }
    }
    return 1;
}

int func_800907C4(void) {
    func_800848B4(0, 2);
    func_800848B4(0, 3);
    return 1;
}

INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_800907F4);
/* best: 98.27%. Structurally identical (same frame, same registers, two pointers
   at D_8009C620+0xA8/+0xFC, one scratchpad base in $s0 live across both calls).
   The only delta is the value in $s0: retail holds 0x1F800000 and so stores at
   0xA0/0xA2/0xA4/0xA8/0xAA/0xAC and passes the two RotMatrixZYX vectors with
   `ori a0,s0,0xA0` / `ori a0,s0,0xA8`; gcc 2.7.2 rematerialises the one-insn
   constant and derives the vectors as base+0xA0 with `addiu` (98.27), while a
   base of 0x1F8000A0 keeps a register but shifts all eight offsets (98.27 too,
   mirrored). Literal 0x1F8000xx addresses make reload use $at per store (74.29);
   cdk 72.92, so the preset is not the answer.

s32 func_800907F4(s32 idx) {
    WorldMapPlayer *pPlayer;
    WorldMapRot *pRotA;
    WorldMapRot *pRotB;

    pPlayer = D_8009BE24 + idx;
    pRotA = (WorldMapRot *)((u8 *)D_8009C620 + 0xA8);
    pRotB = (WorldMapRot *)((u8 *)D_8009C620 + 0xFC);
    if (pPlayer->unk4 == 9) {
        pPlayer->unk4 = 0;
        pPlayer->unk20 = 1;
    } else if (pPlayer->unk4 == 0xA) {
        pPlayer->unk4 = 0;
        pPlayer->unk20 = 2;
    }
    switch (pPlayer->unk20) {
    case 0:
        pPlayer->unk58 = 0;
        pPlayer->unk5C = 0;
        break;
    case 1:
        pPlayer->unk58 += 4;
        if (pPlayer->unk58 >= 0x11) {
            pPlayer->unk5C += 4;
        }
        if (pPlayer->unk58 >= 0x80) {
            pPlayer->unk58 = 0x80;
        }
        if (pPlayer->unk5C >= 0x80) {
            pPlayer->unk5C = 0x80;
        }
        if ((pPlayer->unk58 >= 0x80) & (pPlayer->unk5C >= 0x80)) {
            pPlayer->unk20 = 3;
        }
        break;
    case 2:
        pPlayer->unk58 -= 4;
        if (pPlayer->unk58 < 0x70) {
            pPlayer->unk5C -= 4;
        }
        if (pPlayer->unk58 < 0) {
            pPlayer->unk58 = 0;
        }
        if (pPlayer->unk5C < 0) {
            pPlayer->unk5C = 0;
        }
        if ((pPlayer->unk58 == 0) & (pPlayer->unk5C == 0)) {
            pPlayer->unk20 = 0;
        }
        break;
    }
    pPlayer->unk50 += pPlayer->unk58;
    pPlayer->unk54 -= pPlayer->unk5C;
    *(u16 *)0x1F8000A8 = 0;
    *(u16 *)0x1F8000A0 = 0;
    *(u16 *)0x1F8000A2 = pPlayer->unk50;
    *(u16 *)0x1F8000AA = pPlayer->unk54;
    *(u16 *)0x1F8000A4 = pRotA->unk1C;
    *(u16 *)0x1F8000AC = pRotB->unk1C;
    RotMatrixZYX((SVECTOR *)0x1F8000A0, &pRotA->matrix);
    RotMatrixZYX((SVECTOR *)0x1F8000A8, &pRotB->matrix);
    return 1;
}*/
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
/* best: 88.22%. Everything but the D_8009BD3A pair matches: retail computes the
   address once into v1 (sh + lh off the same base) and really reloads it for
   unk58, while gcc forwards the value it just stored and so uses the address
   once ($at) and skips the lh. Not a preset problem: cdk 75.97, -O3 85.22,
   -G0 85.22, -O2 88.22. Declaring D_8009BD3A volatile kills the forwarding but
   also splits the address into a second lui/addiu (78.10). Swapping the two
   halfword stores changes nothing (88.22).

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
s32 func_800922AC(s32 idx) {
    WorldMapPlayer *pPlayer;
    s32 nRet;

    pPlayer = D_8009BE24 + idx;
    nRet = 1;
    if (pPlayer->unk4 == 9) {
        pPlayer->unk4 = 0;
        pPlayer->unk20 = 1;
    } else if (pPlayer->unk4 == 0xA) {
        pPlayer->unk4 = 0;
        pPlayer->unk20 = 2;
    }
    switch (pPlayer->unk20) {
    case 0:
        nRet = 3;
        break;
    case 1:
        pPlayer->unk50 -= 0x1000;
        D_8009BE0C = pPlayer->unk50 >> 12;
        if (D_8009BE0C < 0x78) {
            D_8009BE0C = 0x78;
            pPlayer->unk50 = 0x78000;
            pPlayer->unk20 = 0;
        }
        break;
    case 2:
        pPlayer->unk50 += 0x1000;
        D_8009BE0C = pPlayer->unk50 >> 12;
        if (D_8009BE0C >= 0x8C) {
            D_8009BE0C = 0x8C;
            pPlayer->unk50 = 0x8C000;
            pPlayer->unk20 = 0;
        }
        break;
    }
    return nRet;
}
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_800923A8);
INCLUDE_ASM("asm/world_map/nonmatchings/main/misc12", func_800925A0);
