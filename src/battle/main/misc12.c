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
extern u8 func_80083FF4(u8, u8);
extern u8 *D_800C3EAC;
extern u8 D_800C3E90[];
extern u8 D_800C3E9B[];
extern u16 D_800C3EC0[];
extern long ratan2(long y, long x);
extern u8 D_800C3E9B[];
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
/* Same jtbl blocker as func_80082F7C below (table jtbl_80070110). */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_800829F4);
/* Same jtbl blocker as func_80082F7C below (table jtbl_80070130). */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_80082BB0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_80082D4C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_80082F7C);
/* BLOCKED by the jump table (round 41, objdiff 88.13%, one attempt):
   retail dispatches through the shared rodata table jtbl_80070170 that
   asm/battle/data/0.rodata.s owns. The switch below makes cc1 emit the same
   eight words in the same order -- but into the unit object's own .rdata
   section, i.e. a second copy next to the asm-owned one, so the overlay grows
   and make build fails (this tree links clean without the switch, so the
   switch is the cause). No source shape can fix that: the whole unit's jtbl
   set has to move out of asm/battle/data/0.rodata.s into C in address order
   as one job. Same blocker on func_800829F4 (jtbl_80070110) and func_80082BB0
   (jtbl_80070130) -- 11 of this unit's 16 stubs name a jtbl.
   Shape notes for that job: the store constant lands in the branch delay slot
   only when the if/else puts the func_8008AA74(0x4F) call in the shared tail,
   and retail's addu is base-first, so the field reads want a p/off pair.

void func_80082F7C(u8 arg0) {
    u8 a;
    a = arg0;
    switch (D_800D3014) {
    case 4:
    case 6:
    case 7:
        func_8007FD38(a);
        if (func_8008B478(a) != 0) {
            func_8008ADD0(a);
        } else {
            D_800C3EAC[0x2DD] = 0x13;
        }
        break;
    case 0:
        if (*(u16 *)(D_800C3EAC + a * 0x40 + 0x2E) != 0) {
            func_8008AA74(0x4F);
        } else {
            D_800C3EAC[0x2DD] = 0x10;
        }
        break;
    case 1:
        if (*(u16 *)(D_800C3EAC + a * 0x40 + 0x34) != 0) {
            func_8008AA74(0x4F);
        } else {
            D_800C3EAC[0x2DD] = 0x11;
        }
        break;
    case 2:
        D_800C3EAC[0x2DD] = 0x12;
        break;
    case 3:
        if (*(u16 *)(D_800C3EAC + a * 0x40 + 0x24) != 0) {
            func_8008AA74(0x4F);
        } else {
            D_800C3EAC[0x2DD] = 0x18;
        }
        break;
    }
}
*/
extern u8 D_800D3014;
extern void func_8007FD38(u8);
extern u8 func_8008B478(u8);
extern void func_8008ADD0(u8);
extern void func_8008AA74(u8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_800830A8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_80083340);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_80083580);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_80083748);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_80083948);
u8 func_80083FF4(u8 arg0, u8 arg1) {
    u8 ret;

    ret = 0;
    if (D_800D2DCC[arg1] != 0) {
        if (D_800C3EB7[arg1][0] == 0) {
            if (D_800D32A1[arg1 * 8] == 0) {
                if (*(u8 *)(D_800D3364 + (D_800C3EB4[arg0][0] * 0x40 +
                                          D_800C3EB4[arg1][0] * 8) + 0x140) == 0) {
                    ret = (D_800CCD64[arg1].unk0 & 0xC001) == 0;
                }
            } else if ((D_800CCE08[arg1].unk0 & 0xC001) == 0) {
                ret = 1;
            }
        }
    }
    return ret;
}
u8 func_80084108(u8 arg0, u8 arg1) {
    u8 ret;

    ret = 0;
    if (D_800D2DCC[arg0] != 0) {
        if (D_800C3EB7[arg0][0] == 0) {
            if (D_800D32A1[arg0 * 8] == 0) {
                if (arg1 == 0) {
                    ret = (D_800CCD64[arg0].unk0 & 0xC001) == 0;
                } else {
                    ret = 1;
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_800841E0);
u8 func_80084548(u8 arg0, u8 arg1, u8 arg2) {
    s32 i;
    s32 j;
    s32 n;
    s32 count1;
    u8 start1;
    s32 count2 = 0;
    u8 start2;
    u8 *p;
    u8 val;

    val = 0xFF;
    i = 0xB;
    p = D_800C3E9B;
    for (; i >= 0; i--) {
        *p-- = val;
    }
    switch (arg0) {
    case 0:
        start1 = 3;
        count1 = 8;
        break;
    case 1:
        start1 = 0;
        count1 = 3;
        break;
    case 2:
        if (arg2 == 0) {
            start1 = 3;
            count1 = 8;
            start2 = 0;
            count2 = 3;
        } else {
            start1 = 0;
            count1 = 3;
            start2 = 3;
            count2 = 8;
        }
        break;
    }
    n = 0;
    D_800D3274 = 0;
    D_800C3D64 = 0;
    for (i = start1; --count1 >= 0; i++) {
        j = i;
        if (func_80084108(j, arg1) != 0) {
            D_800C3E90[n] = j;
            D_800C3D64 |= func_80089C08(j);
            n++;
            D_800D3274++;
        }
    }
    for (i = start2; --count2 >= 0; i++) {
        j = i;
        if (func_80084108(j, arg1) != 0) {
            D_800C3E90[n] = j;
            D_800C3D64 |= func_80089C08(j);
            n++;
            D_800D3274++;
        }
    }
    return D_800C3E90[0];
}
u8 func_80084750(u8 arg0) {
    s32 i;
    s32 j;
    s32 n;
    s32 k;
    u8 *p;
    u8 val;

    k = 8;
    val = 0xFF;
    i = 11;
    p = D_800C3E9B;
    for (; i >= 0; i--) {
        *p-- = val;
    }
    n = 0;
    D_800D3274 = 0;
    D_800C3D64 = 0;
    for (i = 3; --k >= 0; i++) {
        j = i;
        if (func_80083FF4(arg0, j) != 0) {
            D_800C3E90[n] = j;
            D_800C3D64 |= func_80089C08(j);
            n++;
            D_800D3274++;
        }
    }
    return D_800C3E90[0];
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc12", func_80084854);
/* 73.70%: body shape matches (do-while over D_800C3E90..D_800C3E9B, switch on
   arg1, squared-distance minimum); remaining diff is register allocation -
   retail keeps 7 saved registers, this keeps 9, because gcc hoisted the
   D_800C3EC0 base and D_800C3E90 base into saved registers that retail
   rematerialises per use. Tried: idx local, for-loop, do-while with
   ++p in the condition, nested if vs continue.
u8 func_80084854(u8 arg0, u8 arg1) {
    u8 *p;
    u8 result;
    s32 mode;
    s32 angle;
    s32 flag;
    s32 dx;
    s32 dy;
    s32 dist;
    u32 best;

    result = arg0;
    mode = arg1;
    best = 0xFFFFFF;
    p = D_800C3E90;
    do {
        if (*p != 0xFF && *p != arg0) {
            angle = ratan2(D_800C3EC0[*p * 14] - D_800C3EC0[arg0 * 14],
                           D_800C3EBE[*p].unk0 - D_800C3EBE[arg0].unk0);
            flag = 0;
            switch (mode) {
            case 0:
                if ((u16)(angle + 0x200) < 0x400) {
                    flag = 1;
                }
                break;
            case 1:
                if ((u16)(angle + 0x600) < 0x400) {
                    flag = 1;
                }
                break;
            case 2:
                if ((u16)(angle + 0x800) < 0x200) {
                    flag = 1;
                }
                if ((u16)(angle - 0x600) < 0x201) {
                    flag = 1;
                }
                break;
            case 3:
                if ((u16)(angle - 0x200) < 0x400) {
                    flag = 1;
                }
                break;
            }
            if (flag != 0) {
                dx = D_800C3EC0[*p * 14] - D_800C3EC0[arg0 * 14];
                if (dx < 0) {
                    dx = -dx;
                }
                dist = dx * dx;
                dy = D_800C3EBE[*p].unk0 - D_800C3EBE[arg0].unk0;
                if (dy < 0) {
                    dy = -dy;
                }
                dist += dy * dy;
                if (dist < best) {
                    best = dist;
                    result = *p;
                }
            }
        }
    } while (++p < D_800C3E9B);
    return result;
}
*/

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
u16 func_80084D28(void) {
    s32 i;
    u16 ret;

    ret = 0;
    for (i = 0; i < 0xB; i++) {
        if (func_80089C9C(D_800C3D64, i) != 0) {
            if (D_800C3EB4[D_800C3E2C][0] == D_800C3EB4[i][0]) {
                ret |= func_80089C08(i);
            }
        }
    }
    return ret;
}
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
extern u16 D_800D2C54[][2];
extern u8 D_800D2C88[];
extern u8 D_800D2D5C[];
extern u16 D_800D2D70[];
void BattleApplySlotCurve(u8 arg0) {
    s32 i;

    for (i = 0; i < 0xB; i++) {
        D_800C3FE8[arg0].unk0[i] = D_800D2C54[i][0];
        D_800C3FE8[arg0].unk18[i] = D_800D2C88[i];
        switch (D_800D2C88[i]) {
        case 0:
        case 5:
            if (D_800D2D5C[i] == 0 || D_800D2D5C[i] == 5) {
                D_800D2D70[i] = D_800D2D70[i] + D_800D2C54[i][0];
            } else if (D_800D2D5C[i] == 2) {
                if ((s16)D_800D2C54[i][0] - (s16)D_800D2D70[i] >= 0) {
                    D_800D2D70[i] = (s16)D_800D2C54[i][0] - (s16)D_800D2D70[i];
                    D_800D2D5C[i] = 0;
                } else {
                    D_800D2D70[i] = (s16)D_800D2D70[i] - (s16)D_800D2C54[i][0];
                }
            } else {
                D_800D2D70[i] = D_800D2C54[i][0];
                D_800D2D5C[i] = D_800D2C88[i];
            }
            break;
        case 2:
            if (D_800D2D5C[i] == 2) {
                D_800D2D70[i] = D_800D2D70[i] + D_800D2C54[i][0];
            } else if (D_800D2D5C[i] == 0 || D_800D2D5C[i] == 5) {
                if ((s16)D_800D2C54[i][0] - (s16)D_800D2D70[i] < 0) {
                    D_800D2D70[i] = (s16)D_800D2D70[i] - (s16)D_800D2C54[i][0];
                } else {
                    D_800D2D70[i] = (s16)D_800D2C54[i][0] - (s16)D_800D2D70[i];
                    D_800D2D5C[i] = 2;
                }
            } else {
                D_800D2D70[i] = D_800D2C54[i][0];
                D_800D2D5C[i] = D_800D2C88[i];
            }
            break;
        }
        D_800C3FE8[arg0].unk24[i] = D_800D2D70[i];
        D_800C3FE8[arg0].unk3C[i] = D_800D2D5C[i];
    }
}
