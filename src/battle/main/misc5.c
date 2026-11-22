#include "common.h"
#include "psyq/libgpu.h"
#include "battle/actor.h"

extern u8 *g_BattleDisplayState;
extern u16 D_8005A3A0[];
extern u8 g_BattleSlotWordRegs[];
extern u8 g_BattleSlotRows[];
extern u8 g_BattleSlotEnabledFlags[];

// One 0x60-byte entry of the screen slot table at g_BattleTextSlotRects: the VRAM rect the
// slot's text texture is loaded into, the buffer that texture is rendered into,
// the slot's enable flag and the width the rendered string came out at. Only the
// enable flag, set by BattleSet/ClearSlotFlagB4 and cleared for every slot by
// src/battle/main/misc1.c's BattleClearAllSlotFlagB4, is touched here; the other
// three are written by misc1.c's func_80071B94 and misc3.c's BattleQueueCommandFA, which
// reach them through the g_BattleTextSlotRects (+0x0), g_BattleTextSlotBuffers (+0x8) and g_BattleTextSlotFlags
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

// One 0x170-byte entry of the slot table at g_BattleSlotEntries: the flag words the
// slot state tests read. The table starts 0x7C into the slot entry, so this view's
// 0x0 and 0x8 are include/battle/actor.h's BattleTableEntry flag words at 0x7C
// (activeEffectFlags1) and 0x84 (activeEffectFlags3); the three words in between are
// the same grid's accessoryEffectFlags1/activeEffectFlags2/accessoryEffectFlags2.
typedef struct {
    /* 0x000 */ u16 flags; // tested against 0xC000 and 0xC002
    /* 0x002 */ u16 accessoryEffectFlags1;
    /* 0x004 */ u16 activeEffectFlags2;
    /* 0x006 */ u16 accessoryEffectFlags2;
    /* 0x008 */ u16 activeEffectFlags3; // tested against 0x20
    /* 0x00A */ u8 unkA[0x166];
} BattleSlotFlagEntry; // Size: 0x170

// A second view of the 0x170-byte battle slot entry at g_BattleWorkBlock, covering the
// two variable-register files the script VM's byte/word register accessors below
// index by register number: 24 byte variables and 24 word variables. Each
// register number is a distinct field of the entry; include/battle/actor.h's
// BattleTableEntry keeps these offsets inside its opaque arrays, and only the
// registers these two accessors reach are split out here, plus the few entries
// BattleTableEntry names that fall between them.
typedef struct {
    /* 0x000 */ u16 flags; // as BattleTableEntry.flags
    /* 0x002 */ u8 chance; // as BattleTableEntry.chance
    /* 0x003 */ u8 conditionKind; // as BattleTableEntry.conditionKind
    /* 0x004 */ u8 byteVar0;
    /* 0x005 */ u8 unk005[0x28];
    /* 0x02D */ u8 byteVar7;
    /* 0x02E */ u8 unk02E[0x4];
    /* 0x032 */ u16 accessoryEffectFlags5; // as BattleTableEntry.accessoryEffectFlags5
    /* 0x034 */ u16 wordVar21;
    /* 0x036 */ u16 wordVar22;
    /* 0x038 */ u16 wordVar23;
    /* 0x03A */ u8 unk03A[0x12];
    /* 0x04C */ u16 hp; // word variable 1
    /* 0x04E */ u16 maxHp; // word variable 0
    /* 0x050 */ u16 mp; // as BattleTableEntry.mp, neither word register
    /* 0x052 */ u16 maxMp; // as BattleTableEntry.maxMp
    /* 0x054 */ u8 unk054[0x2];
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
    /* 0x068 */ u8 unk068[0x12];
    /* 0x07A */ u16 conditionFlags; // as BattleTableEntry.conditionFlags
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
    /* 0x090 */ u8 unk090[0x10];
    /* 0x0A0 */ u8 gearId; // as BattleTableEntry.gearId
    /* 0x0A1 */ u8 unk0A1[0x15];
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
    /* 0x12C */ u8 statusPenaltyPercent[0x10]; // as BattleSlotState.statusPenaltyPercent
    /* 0x13C */ u8 byteVar19;
    /* 0x13D */ u8 penaltyPercent; // as BattleSlotState.penaltyPercent
    /* 0x13E */ u8 unk13E[0x2];
    /* 0x140 */ u8 byteVar21;
    /* 0x141 */ u8 byteVar22;
    /* 0x142 */ u8 byteVar23;
    /* 0x143 */ s8 responsiveness; // as BattleSlotState.responsiveness
    /* 0x144 */ u8 unk144[0x2C];
} BattleSlotVarEntry; // Size: 0x170

// One 0x1C-byte entry of the slot record table at D_800C3E9C; only the byte at
// 0x1B (the entry's high-bit flag) is read here. D_800C3E9C sits 0x18 before the
// table's tag records at 0x800C3EB4, so this byte is the same one
// src/battle/main/misc8.c and misc9.c name BattleSlotRecord.flags (record +3).
typedef struct {
    /* 0x00 */ u8 unk0[0x18]; // the previous record's tail: this base is 0x18 before the tag
    /* 0x18 */ u8 tag; // the same record tag misc8.c and misc9.c read as BattleSlotRecord.tag
    /* 0x19 */ u8 unk19[0x2];
    /* 0x1B */ u8 flags;
} BattleSlotRecord; // Size: 0x1C

extern BattleScreenSlot g_BattleTextSlotRects[];
extern BattleSlotFlagEntry g_BattleSlotEntries[];
extern BattleSlotRecord D_800C3E9C[];
extern BattleSlotRegs g_BattleSlotRegs[];
extern BattleSlotVarEntry g_BattleWorkBlock[];

void BattleSetSlotFlagB4(u8 arg0) {
    g_BattleDisplayState[0xB4] = 1;
    g_BattleTextSlotRects[arg0].enabled = 1;
}

void BattleClearSlotFlagB4(u8 arg0) {
    g_BattleDisplayState[0xB4] = 0;
    g_BattleTextSlotRects[arg0].enabled = 0;
}

extern u16 BattleMaskHasSlotBit(u16, u8);


u8 BattleFindFirstFlagTableMatch(u16 arg0) {
    s32 i;

    for (i = 0; i < BATTLE_ACTOR_SLOT_COUNT; i++) {
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
                    u32 f = g_BattleSlotEntries[arg0].activeEffectFlags3 & 0x20;

                    res = f == 0;
                }
            }
        } while (0);
    }
    return res;
}
/* The do-while is load-bearing: without it gcc 2.7.2 allocates res to v1 and the
   arg0 * 0x170 offset to a0, the opposite of the original (res in a2, offset in
   v1, reused for both flag words). activeEffectFlags3 is the original's
   D_800CCD6C (the entry at 0x84), spelled through the entry struct, so objdiff
   sees an addend difference only. */
u32 BattleSlotFlagsC002Clear(u8 arg0, u8 arg1) {
    u8 res = 0;

    if (g_BattleSlotEnabledFlags[arg0] != 0) {
        if ((g_BattleSlotEntries[arg0].flags & 0xC002) == 0) {
            res = 1;
            if (arg1 == 0) {
                u32 f = g_BattleSlotEntries[arg0].activeEffectFlags3 & 0x20;

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
    regFileBase = (mem_addr)g_BattleSlotWordRegs;
    *((u16 *)((p->operand1 << 1) + (arg1 * 0x40 + regFileBase))) = val;
}
void BattleDwordRegSetImm(BattleCommand **pCommand, u8 arg1) {
    BattleCommand *p = *pCommand;
    u32 val = ((p->operand3 << 8) + p->operand2) * 0x10;

    g_BattleSlotRegs[arg1].dwordRegs[p->operand1] = val;
}
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
   g_BattleSlotByteRegs lui/addiu after the pointer load; gcc puts the sum straight in v1
   and the base 2 insns early. Tried: if/else with a res local (47.26%,
   branchless/min form), ternary, if-without-else, separate base/idx locals.
void BattleByteRegAddImmClamp(BattleCommand **pCommand, u8 arg1) {
    BattleCommand *p = *pCommand;
    u8 *pRow = g_BattleSlotByteRegs + arg1 * 0x40;
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
   bgez delay slot), and forms the g_BattleSlotByteRegs base after the pointer load.
   Tried: if/else with a res local (60.24%, branchless add/subu/sra/and form),
   v-first declaration orders.
void BattleByteRegSubImmClamp(BattleCommand **pCommand, u8 arg1) {
    BattleCommand *p = *pCommand;
    u8 *pRow = g_BattleSlotByteRegs + arg1 * 0x40;
    s32 v = pRow[p->operand1] - p->operand2;

    if (v < 0) {
        v = 0;
    }
    pRow[p->operand1] = v;
} */
/* 99.09% - 88 bytes, retail 88. NAMED RESIDUAL: the mult operand order. Retail is `mult v1,v0`
   (row value in v1, p->operand2 in v0); the body below is instruction-, register- and load-cluster
   exact everywhere else but emits `mult v0,v1`, i.e. its source order (operand2 first) is locked to
   the load order that reproduces retail's five loads. mult is commutative, so the hardware result is
   identical - this is one ENCODING apart and it can never link.
   Measured round 411: the row-first spelling (`pRow[p->operand1] * p->operand2`) gives retail's mult
   ORDER but swaps the registers (row in v0, operand2 in v1) and sinks operand2's lbu below the row
   load: 71.05. An `s32 mul = p->operand2;` local keeps operand2 in v1 and the same 71.09.
   2.6.0 / 2.7.2-cdk emit retail's load order from the row-first body but `slt` for the clamp where
   retail has `slti` (2.6.0: 99.55). Two 3-minute permuter runs (1733 and 2259 iterations): base=20
   best=20, nothing found. The body below is the 99.09 spelling. */
/*
void func_8007AA60(BattleCommand **pCommand, u8 arg1) {
    BattleCommand *p = *pCommand;
    u8 *pRow = g_BattleSlotRegs[arg1].byteRegs;
    s16 v = p->operand2 * pRow[p->operand1];
    u8 res = v;

    if (v >= 0x100) {
        res = 0xFF;
    }
    pRow[p->operand1] = res;
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc5", func_8007AA60);

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
