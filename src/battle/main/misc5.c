#include "common.h"

extern u8 *D_800D2D28;
extern u16 D_8005A3A0[];
extern u8 D_800D3420[];
extern u8 D_800D3410[];
extern u8 D_800D2DCC[];

// The per-slot battle register file (D_800D3400[slot]): the dword, word and
// byte registers the script's arithmetic ops are applied to. Same layout as
// src/battle/main/misc10.c's BattleSlotRegs.
typedef struct {
    /* 0x00 */ u8 unk0[0x10];
    /* 0x10 */ u32 unk10[4]; // dword registers
    /* 0x20 */ u16 unk20[8]; // word registers
    /* 0x30 */ u8 unk30[0x10]; // byte registers
} BattleSlotRegs; // Size: 0x40

// One 0x60-byte entry of the screen slot table at D_800D3718; only the byte at
// 0xD (the slot's enable flag) is touched here. src/battle/main/misc3.c reaches
// the same table by byte offset.
typedef struct {
    /* 0x00 */ u8 unk0[0xD];
    /* 0x0D */ u8 unkD;
    /* 0x0E */ u8 unkE[0x52];
} BattleScreenSlot; // Size: 0x60

// One 0x170-byte entry of the slot table at D_800CCD64: the two flag words the
// slot state tests read. include/battle/actor.h's BattleTableEntry covers the
// same table, but keeps these two words inside its opaque unk5C array.
typedef struct {
    /* 0x000 */ u16 flags; // tested against 0xC000 and 0xC002
    /* 0x002 */ u8 unk2[0x6];
    /* 0x008 */ u16 unk8; // tested against 0x20
    /* 0x00A */ u8 unkA[0x166];
} BattleSlotFlagEntry; // Size: 0x170

// A second view of the 0x170-byte battle slot entry at D_800CCCE8, covering the
// two variable-register files the script VM's byte/word register accessors below
// index by register number: 24 byte variables and 24 word variables. Each
// register number is a distinct field of the entry; include/battle/actor.h's
// BattleTableEntry keeps these offsets inside its opaque arrays, and only the
// registers these two accessors reach are split out here.
typedef struct {
    /* 0x000 */ u8 unk000[0x4];
    /* 0x004 */ u8 unk004; // byte variable 0
    /* 0x005 */ u8 unk005[0x28];
    /* 0x02D */ u8 unk02D; // byte variable 7
    /* 0x02E */ u8 unk02E[0x6];
    /* 0x034 */ u16 unk034; // word variable 21
    /* 0x036 */ u16 unk036; // word variable 22
    /* 0x038 */ u16 unk038; // word variable 23
    /* 0x03A */ u8 unk03A[0x12];
    /* 0x04C */ u16 unk04C; // word variable 1
    /* 0x04E */ u16 unk04E; // word variable 0
    /* 0x050 */ u8 unk050[0x6];
    /* 0x056 */ u8 unk056; // byte variable 1
    /* 0x057 */ u8 unk057;
    /* 0x058 */ u8 unk058; // byte variable 4
    /* 0x059 */ u8 unk059; // byte variable 5
    /* 0x05A */ u8 unk05A; // byte variable 6
    /* 0x05B */ u8 unk05B; // byte variable 9
    /* 0x05C */ u8 unk05C; // byte variable 10
    /* 0x05D */ u8 unk05D; // byte variable 8
    /* 0x05E */ u8 unk05E; // byte variable 11
    /* 0x05F */ u8 unk05F; // byte variable 12
    /* 0x060 */ u8 unk060; // byte variable 13
    /* 0x061 */ u8 unk061; // byte variable 14
    /* 0x062 */ u8 unk062[0x2];
    /* 0x064 */ u8 unk064; // byte variable 15
    /* 0x065 */ u8 unk065; // byte variable 16
    /* 0x066 */ u8 unk066; // byte variable 17
    /* 0x067 */ u8 unk067; // byte variable 18
    /* 0x068 */ u8 unk068[0x14];
    /* 0x07C */ u16 unk07C; // word variable 2
    /* 0x07E */ u16 unk07E; // word variable 3
    /* 0x080 */ u16 unk080; // word variable 4
    /* 0x082 */ u16 unk082; // word variable 5
    /* 0x084 */ u16 unk084; // word variable 6
    /* 0x086 */ u16 unk086; // word variable 7
    /* 0x088 */ u16 unk088; // word variable 8
    /* 0x08A */ u16 unk08A; // word variable 9
    /* 0x08C */ u16 unk08C; // word variable 10
    /* 0x08E */ u16 unk08E; // word variable 11
    /* 0x090 */ u8 unk090[0x26];
    /* 0x0B6 */ u8 unk0B6; // byte variable 20
    /* 0x0B7 */ u8 unk0B7[0x29];
    /* 0x0E0 */ u8 unk0E0; // byte variable 2
    /* 0x0E1 */ u8 unk0E1[0x2];
    /* 0x0E3 */ u8 unk0E3; // byte variable 3
    /* 0x0E4 */ u8 unk0E4[0x2C];
    /* 0x110 */ u16 unk110; // word variable 12
    /* 0x112 */ u16 unk112;
    /* 0x114 */ u16 unk114; // word variable 13
    /* 0x116 */ u16 unk116; // word variable 14
    /* 0x118 */ u8 unk118[0x8];
    /* 0x120 */ u16 unk120; // word variable 15
    /* 0x122 */ u16 unk122; // word variable 16
    /* 0x124 */ u16 unk124; // word variable 17
    /* 0x126 */ u16 unk126; // word variable 18
    /* 0x128 */ u16 unk128; // word variable 19
    /* 0x12A */ u16 unk12A; // word variable 20
    /* 0x12C */ u8 unk12C[0x10];
    /* 0x13C */ u8 unk13C; // byte variable 19
    /* 0x13D */ u8 unk13D[0x3];
    /* 0x140 */ u8 unk140; // byte variable 21
    /* 0x141 */ u8 unk141; // byte variable 22
    /* 0x142 */ u8 unk142; // byte variable 23
    /* 0x143 */ u8 unk143[0x2D];
} BattleSlotVarEntry; // Size: 0x170

// One 0x1C-byte entry of the slot record table at D_800C3E9C; only the byte at
// 0x1B (the entry's high-bit flag) is read here.
typedef struct {
    /* 0x00 */ u8 unk0[0x1B];
    /* 0x1B */ u8 unk1B;
} BattleSlotRecord; // Size: 0x1C

extern BattleScreenSlot D_800D3718[];
extern BattleSlotFlagEntry D_800CCD64[];
extern BattleSlotRecord D_800C3E9C[];
extern BattleSlotRegs D_800D3400[];
extern BattleSlotVarEntry D_800CCCE8[];

void BattleSetSlotFlagB4(u8 arg0) {
    D_800D2D28[0xB4] = 1;
    D_800D3718[arg0].unkD = 1;
}

void BattleClearSlotFlagB4(u8 arg0) {
    D_800D2D28[0xB4] = 0;
    D_800D3718[arg0].unkD = 0;
}

extern u16 BattleMaskHasSlotBit(u16, u8);


u8 BattleFindFirstFlagTableMatch(u16 arg0) {
    s32 i;

    for (i = 0; i < 0xB; i++) {
        if (BattleMaskHasSlotBit(arg0, i) != 0) {
            break;
        }
    }
    return i;
}
// Reads or writes byte variable `reg` of slot `slot`: a zero `pRead` stores
// `value` into it, a non-zero one returns its current value.
u8 func_80079ED8(u8 slot, u8 reg, u8 value, u8 get) {
    u8 *p;
    u8 result;

    switch (reg) {
    case 0:
        p = &D_800CCCE8[slot].unk004;
        break;
    case 1:
        p = &D_800CCCE8[slot].unk056;
        break;
    case 2:
        p = &D_800CCCE8[slot].unk0E0;
        break;
    case 3:
        p = &D_800CCCE8[slot].unk0E3;
        break;
    case 4:
        p = &D_800CCCE8[slot].unk058;
        break;
    case 5:
        p = &D_800CCCE8[slot].unk059;
        break;
    case 6:
        p = &D_800CCCE8[slot].unk05A;
        break;
    case 7:
        p = &D_800CCCE8[slot].unk02D;
        break;
    case 8:
        p = &D_800CCCE8[slot].unk05D;
        break;
    case 9:
        p = &D_800CCCE8[slot].unk05B;
        break;
    case 10:
        p = &D_800CCCE8[slot].unk05C;
        break;
    case 11:
        p = &D_800CCCE8[slot].unk05E;
        break;
    case 12:
        p = &D_800CCCE8[slot].unk05F;
        break;
    case 13:
        p = &D_800CCCE8[slot].unk060;
        break;
    case 14:
        p = &D_800CCCE8[slot].unk061;
        break;
    case 15:
        p = &D_800CCCE8[slot].unk064;
        break;
    case 16:
        p = &D_800CCCE8[slot].unk065;
        break;
    case 17:
        p = &D_800CCCE8[slot].unk066;
        break;
    case 18:
        p = &D_800CCCE8[slot].unk067;
        break;
    case 19:
        p = &D_800CCCE8[slot].unk13C;
        break;
    case 20:
        p = &D_800CCCE8[slot].unk0B6;
        break;
    case 21:
        p = &D_800CCCE8[slot].unk140;
        break;
    case 22:
        p = &D_800CCCE8[slot].unk141;
        break;
    case 23:
        p = &D_800CCCE8[slot].unk142;
        break;
    }
    if (get == 0) {
        *p = value;
    } else {
        result = *p;
    }
    return result;
}
// The 16-bit half of the variable-register file read/written by func_80079ED8:
// word variable `reg` of slot `slot`, stored when `get` is zero and returned
// otherwise.
u16 func_8007A280(u8 slot, u8 reg, u16 value, u8 get) {
    u16 *p;
    u16 result;

    switch (reg) {
    case 0:
        p = &D_800CCCE8[slot].unk04E;
        break;
    case 1:
        p = &D_800CCCE8[slot].unk04C;
        break;
    case 2:
        p = &D_800CCCE8[slot].unk07C;
        break;
    case 3:
        p = &D_800CCCE8[slot].unk07E;
        break;
    case 4:
        p = &D_800CCCE8[slot].unk080;
        break;
    case 5:
        p = &D_800CCCE8[slot].unk082;
        break;
    case 6:
        p = &D_800CCCE8[slot].unk084;
        break;
    case 7:
        p = &D_800CCCE8[slot].unk086;
        break;
    case 8:
        p = &D_800CCCE8[slot].unk088;
        break;
    case 9:
        p = &D_800CCCE8[slot].unk08A;
        break;
    case 10:
        p = &D_800CCCE8[slot].unk08C;
        break;
    case 11:
        p = &D_800CCCE8[slot].unk08E;
        break;
    case 12:
        p = &D_800CCCE8[slot].unk110;
        break;
    case 13:
        p = &D_800CCCE8[slot].unk114;
        break;
    case 14:
        p = &D_800CCCE8[slot].unk116;
        break;
    case 15:
        p = &D_800CCCE8[slot].unk120;
        break;
    case 16:
        p = &D_800CCCE8[slot].unk122;
        break;
    case 17:
        p = &D_800CCCE8[slot].unk124;
        break;
    case 18:
        p = &D_800CCCE8[slot].unk126;
        break;
    case 19:
        p = &D_800CCCE8[slot].unk128;
        break;
    case 20:
        p = &D_800CCCE8[slot].unk12A;
        break;
    case 21:
        p = &D_800CCCE8[slot].unk034;
        break;
    case 22:
        p = &D_800CCCE8[slot].unk036;
        break;
    case 23:
        p = &D_800CCCE8[slot].unk038;
        break;
    }
    if (get == 0) {
        *p = value;
    } else {
        result = *p;
    }
    return result;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc5", func_8007A628);
/* 99.12% - identical instruction sequence, only the two register choices
   differ: the original keeps res in a2 and the row offset arg0*0x170 in v1
   (sll v1,v0,4 / addu at,at,v1 / addu at,at,v1), ours puts res in v1 and the
   offset in a0. Same body as BattleSlotFlagsC002Clear (which matches 100% with res in v1,
   offset in a0), so the two differ only in allocation: this one needs res out of
   v1. Tried: res declared before/after off, off/f as top-level declarations,
   u32/u8 res. The u8 res is what produces the sltiu v0 + move (a2,v0) copy.
   Round 35: everything (three branches, both delay-slot stores of res, all
   loads, the arg0*0x170 sequence) is identical; the ONLY difference is res
   landing in v1 instead of a2 while off lands in a0 instead of v1 - a global.c
   priority tie. Also tried: the offset written inline twice (cse makes one
   pseudo, same 99.12%) and dropping the `u32 f` local (89.10%, it turns the
   andi+sltiu into srl+xori).
u32 func_8007A628(u8 arg0, u8 arg1) {
    s32 off;
    u32 f;
    u8 res = 0;

    if (D_800D2DCC[arg0] != 0 && D_800C3EB7[arg0 * 0x1C] == 0) {
        off = arg0 * 0x170;
        if ((*(u16 *)(D_800CCD64 + off) & 0xC002) == 0) {
            res = 1;
            if (arg1 == 0) {
                f = *(u16 *)(D_800CCD6C + off) & 0x20;
                res = f == 0;
            }
        }
    }
    return res;
} */
u32 BattleSlotFlagsC002Clear(u8 arg0, u8 arg1) {
    u8 res = 0;

    if (D_800D2DCC[arg0] != 0) {
        if ((D_800CCD64[arg0].flags & 0xC002) == 0) {
            res = 1;
            if (arg1 == 0) {
                u32 f = D_800CCD64[arg0].unk8 & 0x20;

                res = f == 0;
            }
        }
    }
    return res;
}
u32 BattleSlotFlagsC000Clear(u8 arg0) {
    u8 res = 0;

    if (D_800D2DCC[arg0] != 0 && D_800C3E9C[arg0].unk1B == 0) {
        res = (D_800CCD64[arg0].flags & 0xC000) == 0;
    }
    return res;
}
u8 BattleGridWriteOpcodeRow(u8 **arg0, u8 *arg1, u8 arg2, u8 arg3) {
    u8 *pRow = (u8 *)(arg3 * 8 + (mem_addr)arg1);

    pRow[0] = 0x80;
    pRow[1] = (*arg0)[0];
    pRow[2] = (*arg0)[1];
    pRow[3] = (*arg0)[2];
    pRow[4] = (*arg0)[3];
    arg3++;
    return arg3;
}
u8 BattleGridWriteByteAdvanceRow(u8 **arg0, u8 *arg1, u8 arg2) {
    u8 *p = *arg0;

    arg1[arg2 * 8 + p[1]] = p[2];
    if ((*arg0)[1] == 0) {
        arg2++;
    }
    return arg2;
}
void BattleGridWriteByteReg(u8 **arg0, u8 *arg1, u8 arg2, u8 arg3) {
    u8 *p = *arg0;
    s32 idx = arg3 * 8 + p[1];
    u8 *pRow = D_800D3400[arg2].unk30;

    arg1[idx] = pRow[p[2]];
}
void BattleGridCopyRow(u8 **arg0, u8 *arg1) {
    u8 *p;
    s32 i;

    for (i = 0; i < 8; i++) {
        p = *arg0;
        arg1[p[2] * 8 + i] = arg1[p[1] * 8 + i];
    }
}
void BattleByteRegSetImm(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3400[arg1].unk30;

    pRow[p[1]] = p[2];
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc5", func_8007A92C);
/* 84.67% - everything but the final two addus matches (order, registers, the
   lui/addiu base after the p[1] load). Ours associates the sum as
   (idx2 + base) + rowOff (addu v0,v0,v1 / addu a1,a1,v0); the original wants
   (rowOff + base) + idx2 (addu a1,a1,v1 / addu v0,v0,a1), i.e. rowOff must be
   the FIRST operand of the first addu and idx2 the first of the second.
   Naming (rowOff + sym) in a local fixes the add order but makes gcc hoist the
   lui/addiu above the p[] loads (18.33%); `u8 idx = p[1]` with one local gives
   the perfect val/lbu order but hoists the base the same way; an array form
   `((u16*)(sym + arg1*0x40))[p[1]]` and the fully inline sum both become the
   %at address (28.33%).
   Round 35: 10 shapes. A named base local ((u8*)(rowOff + sym) then
   *(u16*)(pBase + (p[1] << 1))) DOES fix the association - the two addus become
   (rowOff+base) then +idx2, as retail - but gcc then emits the lui/addiu group
   and the `lw p` two statements early (17.00%). 84.67% is still the best.
void func_8007A92C(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    s32 rowOff = arg1 * 0x40;
    u16 val = p[2] | (p[3] << 8);
    u16 *pRow = (u16 *)(rowOff + (mem_addr)D_800D3420 + (p[1] << 1));

    *pRow = val;
} */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc5", func_8007A968);
/* 53.44% - the value expression and the index come out right, but gcc forms the
   D_800D3410 lui/addiu at the top (into a0) instead of after the value, so the
   store base is 2 insns early. Tried: pRow pointer local before/after the value,
   idx local first, single-expression store (8-42%, at-register form),
   (u32*)D_800D3410 + arg1*0x10 element pointer, mem_addr index-first form.
   Round 20: with u32 val = ((p[3]<<8)+p[2])*0x10 first and a u8* row pointer
   used as ((u32*)pRow)[p[1]] every instruction is right but the whole
   lui/addiu + addu a1,a1,a0 group lands BEFORE the value loads (10.31%): gcc
   sinks the one-use value into the store, so the pointer init is expanded
   first. Same wall as func_8007A92C; gcc-2.6.0, 2.7.2-cdk, -O1 and -O3 all
   fail on it too.
void func_8007A968(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u32 val = (p[2] + (p[3] << 8)) * 0x10;
    s32 idx = arg1 * 0x10 + p[1];

    ((u32 *)D_800D3410)[idx] = val;
} */
void BattleWordTableStoreImm(u8 **arg0) {
    u8 *p = *arg0;

    D_8005A3A0[p[1]] = p[2];
}
void BattleByteRegAddImmClamp(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3400[arg1].unk30;
    s32 v = pRow[p[1]] + p[2];
    u8 res = v;

    if (v >= 0x100) {
        res = 0xFF;
    }
    pRow[p[1]] = res;
}

/* 61.47% - clamp to 0xFF. Original keeps the sum in v0 and copies it to the
   store register v1 (move v1,v0 in the bnez delay slot) and materialises the
   D_800D3430 lui/addiu after the pointer load; gcc puts the sum straight in v1
   and the base 2 insns early. Tried: if/else with a res local (47.26%,
   branchless/min form), ternary, if-without-else, separate base/idx locals.
void BattleByteRegAddImmClamp(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;
    s32 v = pRow[p[1]] + p[2];

    if (v >= 0x100) {
        v = 0xFF;
    }
    pRow[p[1]] = v;
} */
void BattleByteRegSubImmClamp(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3400[arg1].unk30;
    s32 v = pRow[p[1]] - p[2];
    u8 res = v;

    if (v < 0) {
        res = 0;
    }
    pRow[p[1]] = res;
}
/* 67.00% - clamp at 0. Same miss as BattleByteRegAddImmClamp: the original keeps
   mem-p[2] in v0 and copies it into the store register v1 (move v1,v0 in the
   bgez delay slot), and forms the D_800D3430 base after the pointer load.
   Tried: if/else with a res local (60.24%, branchless add/subu/sra/and form),
   v-first declaration orders.
void BattleByteRegSubImmClamp(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;
    s32 v = pRow[p[1]] - p[2];

    if (v < 0) {
        v = 0;
    }
    pRow[p[1]] = v;
} */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc5", func_8007AA60);
/* 99.55% - one insn off: the original's multiply is mult v1,v0 (value first,
   p[2] second); writing `p[2] * pRow[p[1]]` is the only shape that also gets the
   lbu order right (lbu p[1], lbu p[2], addu row base, lbu row), but then gcc
   emits mult v0,v1. `pRow[p[1]] * p[2]` flips the mult but also flips the loads
   to p[1], addu, row, p[2] (71.50%). Splitting the operands into locals
   (`u8 idx = p[1]; u8 n = p[2]; v = pRow[idx] * n;`) is also 71.50%: cse sinks
   the p[2] load past the row addu anyway.
   Tried: s16 v with u8 res copy (99.55%), s32 operands
   via a cur local, u8 res = product then s16 v = res (87.95%, extension folded),
   operand locals before/after the row pointer (71.50%).
   Round 20: still 99.55% and also exactly 99.55% under gcc-2.7.2-cdk, so this
   one is not the cdk-flippable tie that func_8007ABD8 is.
   Round 35: the last difference is only the mult operand order (mult v0,v1 vs
   v1,v0); every other insn, including both lbu orders, is identical. gcc spells
   'mult <first source operand>,<second>' and always defers the SECOND operand's
   lbu to the mult point, so 'pRow[p[1]] * p[2]' buys the mult order and loses
   the p[2] load (71.50%). The two are mutually exclusive.
void func_8007AA60(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;
    s16 v = p[2] * pRow[p[1]];
    u8 res = v;

    if (v >= 0x100) {
        res = 0xFF;
    }
    pRow[p[1]] = res;
} */
void BattleByteRegDivImm(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3400[arg1].unk30;

    pRow[p[1]] /= p[2];
}
void BattleByteRegModImm(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3400[arg1].unk30;

    pRow[p[1]] %= p[2];
}
void BattleByteRegAndImm(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3400[arg1].unk30;

    pRow[p[1]] &= p[2];
}
void BattleByteRegOrImm(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3400[arg1].unk30;

    pRow[p[1]] |= p[2];
}
void BattleByteRegXorImm(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3400[arg1].unk30;

    pRow[p[1]] ^= p[2];
}
