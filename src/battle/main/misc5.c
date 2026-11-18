#include "common.h"
#include "psyq/libgpu.h"
#include "battle/actor.h"

extern u8 *g_BattleDisplayState;
extern u16 D_8005A3A0[];
extern u8 D_800D3420[];
extern u8 g_BattleSlotRows[];
extern u8 g_BattleSlotEnabledFlags[];

// One 0x60-byte entry of the screen slot table at D_800D3718: the VRAM rect the
// slot's text texture is loaded into, the buffer that texture is rendered into,
// the slot's enable flag and the width the rendered string came out at. Only the
// enable flag, set by BattleSet/ClearSlotFlagB4 and cleared for every slot by
// src/battle/main/misc1.c's BattleClearAllSlotFlagB4, is touched here; the other
// three are written by misc1.c's func_80071B94 and misc3.c's func_80078658, which
// reach them through the D_800D3718 (+0x0), D_800D3720 (+0x8) and D_800D3726
// (+0xE) symbols.
typedef struct {
    /* 0x00 */ RECT rect; // the VRAM rect the slot's text texture is loaded into
    /* 0x08 */ u_long *pBuf; // the slot's text buffer, rendered into then loaded through rect
    /* 0x0C */ u8 palette; // index & 1: picks g_SystemPalette2 over g_SystemPalette1, as the menu records' palette
    /* 0x0D */ u8 enabled;
    /* 0x0E */ u8 textWidth; // what SystemRenderStringEntry returns for the slot's string
    /* 0x0F */ u8 padF;
    /* 0x10 */ POLY_FT4 polys[2]; // the slot's text quads, both set from the pointer misc3.c's func_800780A8 walks
} BattleScreenSlot; // Size: 0x60

// One 0x170-byte entry of the slot table at g_BattleSlotEntries: the two flag words the
// slot state tests read. include/battle/actor.h's BattleTableEntry covers the
// same table, but keeps the word at 0x8 inside its opaque unk04 array.
typedef struct {
    /* 0x000 */ u16 flags; // tested against 0xC000 and 0xC002
    /* 0x002 */ u8 unk2[0x6];
    /* 0x008 */ u16 unk8; // tested against 0x20
    /* 0x00A */ u8 unkA[0x166];
} BattleSlotFlagEntry; // Size: 0x170

// A second view of the 0x170-byte battle slot entry at g_BattleWorkBlock, covering the
// two variable-register files the script VM's byte/word register accessors below
// index by register number: 24 byte variables and 24 word variables. Each
// register number is a distinct field of the entry; include/battle/actor.h's
// BattleTableEntry keeps these offsets inside its opaque arrays, and only the
// registers these two accessors reach are split out here.
typedef struct {
    /* 0x000 */ u8 unk000[0x4];
    /* 0x004 */ u8 byteVar0;
    /* 0x005 */ u8 unk005[0x28];
    /* 0x02D */ u8 byteVar7;
    /* 0x02E */ u8 unk02E[0x6];
    /* 0x034 */ u16 wordVar21;
    /* 0x036 */ u16 wordVar22;
    /* 0x038 */ u16 wordVar23;
    /* 0x03A */ u8 unk03A[0x12];
    /* 0x04C */ u16 hp; // word variable 1
    /* 0x04E */ u16 maxHp; // word variable 0
    /* 0x050 */ u8 unk050[0x6];
    /* 0x056 */ u8 byteVar1;
    /* 0x057 */ u8 unk057;
    /* 0x058 */ u8 byteVar4;
    /* 0x059 */ u8 byteVar5;
    /* 0x05A */ u8 byteVar6;
    /* 0x05B */ u8 byteVar9;
    /* 0x05C */ u8 byteVar10;
    /* 0x05D */ u8 byteVar8;
    /* 0x05E */ u8 byteVar11;
    /* 0x05F */ u8 byteVar12;
    /* 0x060 */ u8 byteVar13;
    /* 0x061 */ u8 byteVar14;
    /* 0x062 */ u8 unk062[0x2];
    /* 0x064 */ u8 byteVar15;
    /* 0x065 */ u8 byteVar16;
    /* 0x066 */ u8 byteVar17;
    /* 0x067 */ u8 byteVar18;
    /* 0x068 */ u8 unk068[0x14];
    /* 0x07C */ u16 wordVar2;
    /* 0x07E */ u16 wordVar3;
    /* 0x080 */ u16 wordVar4;
    /* 0x082 */ u16 wordVar5;
    /* 0x084 */ u16 wordVar6;
    /* 0x086 */ u16 wordVar7;
    /* 0x088 */ u16 wordVar8;
    /* 0x08A */ u16 wordVar9;
    /* 0x08C */ u16 wordVar10;
    /* 0x08E */ u16 wordVar11;
    /* 0x090 */ u8 unk090[0x26];
    /* 0x0B6 */ u8 byteVar20;
    /* 0x0B7 */ u8 unk0B7[0x29];
    /* 0x0E0 */ u8 byteVar2;
    /* 0x0E1 */ u8 unk0E1[0x2];
    /* 0x0E3 */ u8 byteVar3;
    /* 0x0E4 */ u8 unk0E4[0x2C];
    /* 0x110 */ u16 wordVar12;
    /* 0x112 */ u16 unk112;
    /* 0x114 */ u16 wordVar13;
    /* 0x116 */ u16 wordVar14;
    /* 0x118 */ u8 unk118[0x8];
    /* 0x120 */ u16 wordVar15;
    /* 0x122 */ u16 wordVar16;
    /* 0x124 */ u16 wordVar17;
    /* 0x126 */ u16 wordVar18;
    /* 0x128 */ u16 wordVar19;
    /* 0x12A */ u16 wordVar20;
    /* 0x12C */ u8 unk12C[0x10];
    /* 0x13C */ u8 byteVar19;
    /* 0x13D */ u8 unk13D[0x3];
    /* 0x140 */ u8 byteVar21;
    /* 0x141 */ u8 byteVar22;
    /* 0x142 */ u8 byteVar23;
    /* 0x143 */ u8 unk143[0x2D];
} BattleSlotVarEntry; // Size: 0x170

// One 0x1C-byte entry of the slot record table at D_800C3E9C; only the byte at
// 0x1B (the entry's high-bit flag) is read here. D_800C3E9C sits 0x18 before the
// table's tag records at 0x800C3EB4, so this byte is the same one
// src/battle/main/misc8.c and misc9.c name BattleSlotRecord.flags (record +3).
typedef struct {
    /* 0x00 */ u8 unk0[0x1B];
    /* 0x1B */ u8 flags;
} BattleSlotRecord; // Size: 0x1C

extern BattleScreenSlot D_800D3718[];
extern BattleSlotFlagEntry g_BattleSlotEntries[];
extern BattleSlotRecord D_800C3E9C[];
extern BattleSlotRegs g_BattleSlotRegs[];
extern BattleSlotVarEntry g_BattleWorkBlock[];

void BattleSetSlotFlagB4(u8 arg0) {
    g_BattleDisplayState[0xB4] = 1;
    D_800D3718[arg0].enabled = 1;
}

void BattleClearSlotFlagB4(u8 arg0) {
    g_BattleDisplayState[0xB4] = 0;
    D_800D3718[arg0].enabled = 0;
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
u8 BattleSlotByteVarGetSet(u8 slot, u8 reg, u8 value, u8 get) {
    u8 *p;
    u8 result;

    switch (reg) {
    case 0:
        p = &g_BattleWorkBlock[slot].byteVar0;
        break;
    case 1:
        p = &g_BattleWorkBlock[slot].byteVar1;
        break;
    case 2:
        p = &g_BattleWorkBlock[slot].byteVar2;
        break;
    case 3:
        p = &g_BattleWorkBlock[slot].byteVar3;
        break;
    case 4:
        p = &g_BattleWorkBlock[slot].byteVar4;
        break;
    case 5:
        p = &g_BattleWorkBlock[slot].byteVar5;
        break;
    case 6:
        p = &g_BattleWorkBlock[slot].byteVar6;
        break;
    case 7:
        p = &g_BattleWorkBlock[slot].byteVar7;
        break;
    case 8:
        p = &g_BattleWorkBlock[slot].byteVar8;
        break;
    case 9:
        p = &g_BattleWorkBlock[slot].byteVar9;
        break;
    case 10:
        p = &g_BattleWorkBlock[slot].byteVar10;
        break;
    case 11:
        p = &g_BattleWorkBlock[slot].byteVar11;
        break;
    case 12:
        p = &g_BattleWorkBlock[slot].byteVar12;
        break;
    case 13:
        p = &g_BattleWorkBlock[slot].byteVar13;
        break;
    case 14:
        p = &g_BattleWorkBlock[slot].byteVar14;
        break;
    case 15:
        p = &g_BattleWorkBlock[slot].byteVar15;
        break;
    case 16:
        p = &g_BattleWorkBlock[slot].byteVar16;
        break;
    case 17:
        p = &g_BattleWorkBlock[slot].byteVar17;
        break;
    case 18:
        p = &g_BattleWorkBlock[slot].byteVar18;
        break;
    case 19:
        p = &g_BattleWorkBlock[slot].byteVar19;
        break;
    case 20:
        p = &g_BattleWorkBlock[slot].byteVar20;
        break;
    case 21:
        p = &g_BattleWorkBlock[slot].byteVar21;
        break;
    case 22:
        p = &g_BattleWorkBlock[slot].byteVar22;
        break;
    case 23:
        p = &g_BattleWorkBlock[slot].byteVar23;
        break;
    }
    if (get == 0) {
        *p = value;
    } else {
        result = *p;
    }
    return result;
}
// The 16-bit half of the variable-register file read/written by BattleSlotByteVarGetSet:
// word variable `reg` of slot `slot`, stored when `get` is zero and returned
// otherwise.
u16 BattleSlotWordVarGetSet(u8 slot, u8 reg, u16 value, u8 get) {
    u16 *p;
    u16 result;

    switch (reg) {
    case 0:
        p = &g_BattleWorkBlock[slot].maxHp;
        break;
    case 1:
        p = &g_BattleWorkBlock[slot].hp;
        break;
    case 2:
        p = &g_BattleWorkBlock[slot].wordVar2;
        break;
    case 3:
        p = &g_BattleWorkBlock[slot].wordVar3;
        break;
    case 4:
        p = &g_BattleWorkBlock[slot].wordVar4;
        break;
    case 5:
        p = &g_BattleWorkBlock[slot].wordVar5;
        break;
    case 6:
        p = &g_BattleWorkBlock[slot].wordVar6;
        break;
    case 7:
        p = &g_BattleWorkBlock[slot].wordVar7;
        break;
    case 8:
        p = &g_BattleWorkBlock[slot].wordVar8;
        break;
    case 9:
        p = &g_BattleWorkBlock[slot].wordVar9;
        break;
    case 10:
        p = &g_BattleWorkBlock[slot].wordVar10;
        break;
    case 11:
        p = &g_BattleWorkBlock[slot].wordVar11;
        break;
    case 12:
        p = &g_BattleWorkBlock[slot].wordVar12;
        break;
    case 13:
        p = &g_BattleWorkBlock[slot].wordVar13;
        break;
    case 14:
        p = &g_BattleWorkBlock[slot].wordVar14;
        break;
    case 15:
        p = &g_BattleWorkBlock[slot].wordVar15;
        break;
    case 16:
        p = &g_BattleWorkBlock[slot].wordVar16;
        break;
    case 17:
        p = &g_BattleWorkBlock[slot].wordVar17;
        break;
    case 18:
        p = &g_BattleWorkBlock[slot].wordVar18;
        break;
    case 19:
        p = &g_BattleWorkBlock[slot].wordVar19;
        break;
    case 20:
        p = &g_BattleWorkBlock[slot].wordVar20;
        break;
    case 21:
        p = &g_BattleWorkBlock[slot].wordVar21;
        break;
    case 22:
        p = &g_BattleWorkBlock[slot].wordVar22;
        break;
    case 23:
        p = &g_BattleWorkBlock[slot].wordVar23;
        break;
    }
    if (get == 0) {
        *p = value;
    } else {
        result = *p;
    }
    return result;
}
u32 BattleSlotFlagsC002ClearRecordIdle(u8 arg0, u8 arg1) {
    u8 res = 0;

    if (g_BattleSlotEnabledFlags[arg0] != 0 && D_800C3E9C[arg0].flags == 0) {
        do {
            if ((g_BattleSlotEntries[arg0].flags & 0xC002) == 0) {
                res = 1;
                if (arg1 == 0) {
                    u32 f = g_BattleSlotEntries[arg0].unk8 & 0x20;

                    res = f == 0;
                }
            }
        } while (0);
    }
    return res;
}
/* The do-while is load-bearing: without it gcc 2.7.2 allocates res to v1 and the
   arg0 * 0x170 offset to a0, the opposite of the original (res in a2, offset in
   v1, reused for both flag words). The word at 0x8 is the original's
   D_800CCD6C, spelled through the entry struct, so objdiff sees an addend
   difference only. */
u32 BattleSlotFlagsC002Clear(u8 arg0, u8 arg1) {
    u8 res = 0;

    if (g_BattleSlotEnabledFlags[arg0] != 0) {
        if ((g_BattleSlotEntries[arg0].flags & 0xC002) == 0) {
            res = 1;
            if (arg1 == 0) {
                u32 f = g_BattleSlotEntries[arg0].unk8 & 0x20;

                res = f == 0;
            }
        }
    }
    return res;
}
u32 BattleSlotFlagsC000Clear(u8 arg0) {
    u8 res = 0;

    if (g_BattleSlotEnabledFlags[arg0] != 0 && D_800C3E9C[arg0].flags == 0) {
        res = (g_BattleSlotEntries[arg0].flags & 0xC000) == 0;
    }
    return res;
}
u8 BattleGridWriteOpcodeRow(BattleCommand **pCommand, u8 *arg1, u8 arg2, u8 arg3) {
    u8 *pRow = (u8 *)(arg3 * 8 + (mem_addr)arg1);

    pRow[0] = 0x80;
    pRow[1] = (*pCommand)->opcode;
    pRow[2] = (*pCommand)->operand1;
    pRow[3] = (*pCommand)->operand2;
    pRow[4] = (*pCommand)->operand3;
    arg3++;
    return arg3;
}
u8 BattleGridWriteByteAdvanceRow(BattleCommand **pCommand, u8 *arg1, u8 arg2) {
    BattleCommand *p = *pCommand;

    arg1[arg2 * 8 + p->operand1] = p->operand2;
    if ((*pCommand)->operand1 == 0) {
        arg2++;
    }
    return arg2;
}
void BattleGridWriteByteReg(BattleCommand **pCommand, u8 *arg1, u8 arg2, u8 arg3) {
    BattleCommand *p = *pCommand;
    s32 idx = arg3 * 8 + p->operand1;
    u8 *pRow = g_BattleSlotRegs[arg2].byteRegs;

    arg1[idx] = pRow[p->operand2];
}
void BattleGridCopyRow(BattleCommand **pCommand, u8 *arg1) {
    BattleCommand *p;
    s32 i;

    for (i = 0; i < 8; i++) {
        p = *pCommand;
        arg1[p->operand2 * 8 + i] = arg1[p->operand1 * 8 + i];
    }
}
void BattleByteRegSetImm(BattleCommand **pCommand, u8 arg1) {
    BattleCommand *p = *pCommand;
    u8 *pRow = g_BattleSlotRegs[arg1].byteRegs;

    pRow[p->operand1] = p->operand2;
}
void BattleWordRegSetImm(BattleCommand **pCommand, u8 arg1) {
    BattleCommand *p = *pCommand;
    u16 val = p->operand2 | (p->operand3 << 8);
    mem_addr regFileBase;

    // separate assignment: gcc materialises the base before the value loads if it is an initializer
    regFileBase = (mem_addr)D_800D3420;
    *((u16 *)((p->operand1 << 1) + (arg1 * 0x40 + regFileBase))) = val;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc5", func_8007A968);
/* 53.44% - the value expression and the index come out right, but gcc forms the
   g_BattleSlotRows lui/addiu at the top (into a0) instead of after the value, so the
   store base is 2 insns early. Tried: pRow pointer local before/after the value,
   idx local first, single-expression store (8-42%, at-register form),
   (u32*)g_BattleSlotRows + arg1*0x10 element pointer, mem_addr index-first form.
   Round 20: with u32 val = ((p->operand3<<8)+p->operand2)*0x10 first and a u8* row pointer
   used as ((u32*)pRow)[p->operand1] every instruction is right but the whole
   lui/addiu + addu a1,a1,a0 group lands BEFORE the value loads (10.31%): gcc
   sinks the one-use value into the store, so the pointer init is expanded
   first. Same wall as the old BattleWordRegSetImm body (now matched: statement-
   assigned base mem_addr + index-first inline address); gcc-2.6.0, 2.7.2-cdk,
   -O1 and -O3 all fail on it too.
   Round 135: the typed spelling `g_BattleSlotRegs[arg1].dwordRegs[p->operand1] = val` (dword
   register file, g_BattleSlotRows) written as one expression is 7.50% - the address
   expansion takes over the register allocation - so the separate-locals body
   below stays the best.
void func_8007A968(BattleCommand **pCommand, u8 arg1) {
    BattleCommand *p = *pCommand;
    u32 val = (p->operand2 + (p->operand3 << 8)) * 0x10;
    s32 idx = arg1 * 0x10 + p->operand1;

    ((u32 *)g_BattleSlotRows)[idx] = val;
} */
void BattleWordTableStoreImm(BattleCommand **pCommand) {
    BattleCommand *p = *pCommand;

    D_8005A3A0[p->operand1] = p->operand2;
}
void BattleByteRegAddImmClamp(BattleCommand **pCommand, u8 arg1) {
    BattleCommand *p = *pCommand;
    u8 *pRow = g_BattleSlotRegs[arg1].byteRegs;
    s32 v = pRow[p->operand1] + p->operand2;
    u8 res = v;

    if (v >= 0x100) {
        res = 0xFF;
    }
    pRow[p->operand1] = res;
}

/* 61.47% - clamp to 0xFF. Original keeps the sum in v0 and copies it to the
   store register v1 (move v1,v0 in the bnez delay slot) and materialises the
   D_800D3430 lui/addiu after the pointer load; gcc puts the sum straight in v1
   and the base 2 insns early. Tried: if/else with a res local (47.26%,
   branchless/min form), ternary, if-without-else, separate base/idx locals.
void BattleByteRegAddImmClamp(BattleCommand **pCommand, u8 arg1) {
    BattleCommand *p = *pCommand;
    u8 *pRow = D_800D3430 + arg1 * 0x40;
    s32 v = pRow[p->operand1] + p->operand2;

    if (v >= 0x100) {
        v = 0xFF;
    }
    pRow[p->operand1] = v;
} */
void BattleByteRegSubImmClamp(BattleCommand **pCommand, u8 arg1) {
    BattleCommand *p = *pCommand;
    u8 *pRow = g_BattleSlotRegs[arg1].byteRegs;
    s32 v = pRow[p->operand1] - p->operand2;
    u8 res = v;

    if (v < 0) {
        res = 0;
    }
    pRow[p->operand1] = res;
}
/* 67.00% - clamp at 0. Same miss as BattleByteRegAddImmClamp: the original keeps
   mem-p->operand2 in v0 and copies it into the store register v1 (move v1,v0 in the
   bgez delay slot), and forms the D_800D3430 base after the pointer load.
   Tried: if/else with a res local (60.24%, branchless add/subu/sra/and form),
   v-first declaration orders.
void BattleByteRegSubImmClamp(BattleCommand **pCommand, u8 arg1) {
    BattleCommand *p = *pCommand;
    u8 *pRow = D_800D3430 + arg1 * 0x40;
    s32 v = pRow[p->operand1] - p->operand2;

    if (v < 0) {
        v = 0;
    }
    pRow[p->operand1] = v;
} */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc5", func_8007AA60);
/* 99.09% - 88 bytes, the original's size; the only difference is the mult's
   operand order: retail has mult v1,v0 (row value first, p->operand2 second), this body
   emits mult v0,v1. Every other instruction, register included, is retail's.
   The load order (lbu p->operand1, lbu p->operand2, addu row base, lbu row) only comes out of a
   source that names p->operand2 first, and gcc then puts p->operand2's register first in the
   mult. Measured rules (round 167):
   - With p->operand2 as the mult's memory operand (sunk to the mult, collapsed to a
     single-use local) gcc does NOT swap, so the source order survives: p->operand2 first
     gives mult v0,v1, row-first spellings give mult v0,v1 with row in v0 instead
     (the row value then lands in the wrong register, and p->operand2's load moves to
     after the row load: 71.05%).
   - With BOTH operands in registers (a volatile read keeps p->operand2 out of cse) gcc
     DOES swap, so row-first source gives mult with the row value first - but the
     p->operand2 load then no longer sits after lbu p->operand1 where retail has it.
   Tried round 167: the 2 x 2 of source order against memory/reg operands, u8/s16/
   s32/u8-cast operand types, element pointer local, row value local, volatile
   reads of p->operand1 and p->operand2 (best 98.64%: retail's exact load order and register
   allocation, but the addu and the mult both with their operands inverted), and
   permuter 2259 iterations, base=20 best=20. Rounds 20/35/135/164 tried the rest.
void func_8007AA60(BattleCommand **pCommand, u8 arg1) {
    BattleCommand *p = *pCommand;
    u8 *pRow = g_BattleSlotRegs[arg1].byteRegs;
    s16 v = p->operand2 * pRow[p->operand1];
    u8 res = v;

    if (v >= 0x100) {
        res = 0xFF;
    }
    pRow[p->operand1] = res;
} */
void BattleByteRegDivImm(BattleCommand **pCommand, u8 arg1) {
    BattleCommand *p = *pCommand;
    u8 *pRow = g_BattleSlotRegs[arg1].byteRegs;

    pRow[p->operand1] /= p->operand2;
}
void BattleByteRegModImm(BattleCommand **pCommand, u8 arg1) {
    BattleCommand *p = *pCommand;
    u8 *pRow = g_BattleSlotRegs[arg1].byteRegs;

    pRow[p->operand1] %= p->operand2;
}
void BattleByteRegAndImm(BattleCommand **pCommand, u8 arg1) {
    BattleCommand *p = *pCommand;
    u8 *pRow = g_BattleSlotRegs[arg1].byteRegs;

    pRow[p->operand1] &= p->operand2;
}
void BattleByteRegOrImm(BattleCommand **pCommand, u8 arg1) {
    BattleCommand *p = *pCommand;
    u8 *pRow = g_BattleSlotRegs[arg1].byteRegs;

    pRow[p->operand1] |= p->operand2;
}
void BattleByteRegXorImm(BattleCommand **pCommand, u8 arg1) {
    BattleCommand *p = *pCommand;
    u8 *pRow = g_BattleSlotRegs[arg1].byteRegs;

    pRow[p->operand1] ^= p->operand2;
}
