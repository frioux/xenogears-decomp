#include "common.h"

extern u8 D_800D2D5C[];
extern u16 D_800D2D70[];
typedef struct {
    /* 0x00 */ u16 unk0;
    /* 0x02 */ u8 unk2[0x1A];
} BattleUnk1C; // Size: 0x1C

extern BattleUnk1C D_800C3EBE[];
extern u8 D_800C3EB4[][0x1C];
extern u16 D_800C3D64;
extern u8 D_800C3E2C;
extern u16 func_80089C9C(u16, u8);
extern u16 func_80089C08(u8);
extern u8 *D_800C3EAC;
extern u8 D_800C3E90[];
extern u8 D_800D3274;
extern void func_800841E0(void);

typedef struct {
    /* 0x00 */ u16 unk0[12];
    /* 0x18 */ u8 unk18[12];
    /* 0x24 */ u16 unk24[12];
    /* 0x3C */ u8 unk3C[12];
} BattleUnk48; // Size: 0x48

extern BattleUnk48 D_800C3FE8[];

typedef struct {
    /* 0x00 */ u16 unk0;
    /* 0x02 */ u8 pad2[0x16E];
} BattleUnk170; // Size: 0x170

extern u8 D_800D2DCC[];
extern u8 D_800C3EB7[][0x1C];
extern u8 D_800D32A1[];
extern u8 *D_800D3364;
extern BattleUnk170 D_800CCD64[];
extern BattleUnk170 D_800CCE08[];

INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_80082820);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_800829F4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_80082BB0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_80082D4C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_80082F7C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_800830A8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_80083340);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_80083580);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_80083748);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_80083948);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_80083FF4);
/* 89.49%: the D_800C3EB4[][0x1C] / D_800C3EB7[][0x1C] types kill the 8-byte phantom frame
   the u8[] + [i*0x1C] spelling caused (was 86.45%). Left over: gcc evaluates the two
   D_800C3EB4 loads and their *8 / *0x40 shifts in the mirrored register order, and the
   `ret = (x & 0xC001) == 0` tail differs (`move t0,v0` + later or vs `andi`/`sltiu`).
u32 func_80083FF4(u8 arg0, u8 arg1) {
    u32 ret;

    ret = 0;
    if (D_800D2DCC[arg1] != 0) {
        if (D_800C3EB7[arg1][0] == 0) {
            if (D_800D32A1[arg1 * 8] == 0) {
                if (*(u8 *)(D_800D3364 + D_800C3EB4[arg1][0] * 8 +
                            D_800C3EB4[arg0][0] * 0x40 + 0x140) == 0) {
                    ret = (D_800CCD64[arg1].unk0 & 0xC001) == 0;
                }
            } else {
                ret = (D_800CCE08[arg1].unk0 & 0xC001) == 0;
            }
        }
    }
    return ret;
}

*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_80084108);
/* 95.93% (spelled with the old u8[] + [i*0x1C] types): one instruction class off - gcc emits `sltiu a2,v0,1` in the j delay slot where the
   original keeps `sltiu v0,v0,1` + `move a2,v0` (ret copy not coalesced). `== 0` spelled
   explicitly instead of `? 0 : 1` gives 94.35% with the same sltiu-into-a2 shape.
u32 func_80084108(u8 arg0, u8 arg1) {
    u32 ret;

    ret = 0;
    if (D_800D2DCC[arg0] == 0) {
        if (D_800C3EB7[arg0][0] == 0) {
            if (D_800D32A1[arg0 * 8] == 0) {
                if (arg1 != 0) {
                    ret = 1;
                } else {
                    ret = (D_800CCD64[arg0].unk0 & 0xC001) ? 0 : 1;
                }
            } else {
                if (arg1 == 0) {
                    if ((D_800CCE08[arg0].unk0 & 0xC001) == 0) {
                        ret = 1;
                    }
                } else {
                    ret = 1;
                }
            }
        }
    }
    return ret;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_800841E0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_80084548);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_80084750);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_80084854);
void func_80084A7C(u8 arg0) {
    s32 off;
    s32 found;
    s32 i;

    off = arg0 * 0x40;
    D_800C3EAC[0x2E8] = (D_800C3EAC + off)[0x3C];
    found = 0;
    func_800841E0();
    for (i = 0; i < D_800D3274; i++) {
        if (D_800C3E90[i] == (D_800C3EAC + off)[0x3C]) {
            found++;
            break;
        }
    }
    if (found == 0) {
        D_800C3EAC[0x2E8] = D_800C3E90[0];
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_80084B40);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_80084D28);
/* 95.21%: the 2D-array type fixes the phantom frame the u8[] + [i*0x1C] spelling caused.
   Left over: the original keeps `i++` in the beqz delay slot and duplicates the
   `slti $s0,0xB` loop test into the bne delay slot; gcc puts both in the latch. A do-while
   spelling (idx = i++) drops to 80.00% because i stops being an induction variable.
u16 func_80084D28(void) {
    s32 i;
    u16 ret;
    u8 idx;

    ret = 0;
    for (i = 0; i < 0xB; i++) {
        idx = i;
        if (func_80089C9C(D_800C3D64, idx) != 0) {
            if (D_800C3EB4[D_800C3E2C][0] == D_800C3EB4[idx][0]) {
                ret |= func_80089C08(idx);
            }
        }
    }
    return ret;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_80084DE4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_80085084);
u32 func_80085310(u8 arg0, u8 arg1) {
    return D_800C3EBE[arg0].unk0 > D_800C3EBE[arg1].unk0;
}
void func_80085350(void) {
    s32 i;

    for (i = 0; i < 11; i++) {
        D_800D2D5C[i] = 0xFF;
        D_800D2D70[i] = 0;
    }
}
void func_80085388(void) {
    s32 i;

    for (i = 0; i < 0xB; i++) {
        D_800C3FE8[D_800C3EAC[0x2DA]].unk0[i] = 0;
        D_800C3FE8[D_800C3EAC[0x2DA]].unk18[i] = 0xFF;
        D_800C3FE8[D_800C3EAC[0x2DA]].unk24[i] = 0;
        D_800C3FE8[D_800C3EAC[0x2DA]].unk3C[i] = 0xFF;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_80085454);
