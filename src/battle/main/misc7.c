#include "common.h"
#include "battle/actor.h"
#include "main/game.h"

// The two slot-argument values BattleDwordRegSetFromSlotArgs can load a dword
// register from, selected by the instruction's operand2: the slot state's hp, or
// the party's gold.
#define BATTLE_SLOT_ARG_SOURCE_HP 1
#define BATTLE_SLOT_ARG_SOURCE_GOLD 2

// One slot of the per-slot operator register block at g_BattleSlotRegs (stride 0x40):
// the same arguments in 32-, 16- and 8-bit form. g_BattleSlotRows, g_BattleSlotWordRegs,
// g_BattleSlotByteRegs and D_800D343F are its +0x10, +0x20, +0x30 and +0x3F, and the two
// cursors at +0x00/+0x08 are those include/battle/actor.h's BattleSlotRegs names.
// Accessing a bank as g_BattleSlotRegs[index].wordRegs is the same code as the old
// untyped `(u16 *)(g_BattleSlotWordRegs + index * 0x40)`, and it is what retail's
// register shapes want: the base stays g_BattleSlotWordRegs and only the HI16/LO16
// addend folds, so diff_func.sh prints 99.x for the bank accesses that are
// byte-identical (check_bin.sh battle: OK).
typedef struct {
    /* 0x00 */ u8 *pScriptCursor;
    /* 0x04 */ u8 unk04[0x4];
    /* 0x08 */ u8 *pScanCursor;
    /* 0x0C */ u8 unk0C[0x4];
    /* 0x10 */ u32 dwordRegs[4];
    /* 0x20 */ u16 wordRegs[8];
    /* 0x30 */ u8 byteRegs[0x10];
} BattleOpRegs; // Size: 0x40

extern BattleOpRegs g_BattleSlotRegs[];
extern u8 g_BattleSlotWordRegs[];
extern u16 D_8005A3A0[];

// One of the 8-byte per-slot "can be picked" records at g_BattleSlotFlags; only
// its first byte is read here.
typedef struct {
    /* 0x0 */ u8 inUse;
    /* 0x1 */ u8 unk1[0x7];
} BattleSlotFlag; // Size: 0x8

extern BattleSlotFlag g_BattleSlotFlags[];

// One of the 0x1C-byte per-slot records at g_BattleSlotRecords: the tag at +0 is
// the character occupying the slot.
typedef struct {
    /* 0x0 */ u8 tag;
    /* 0x1 */ u8 unk1[0x1B];
} BattleSlotRecord; // Size: 0x1C

extern BattleSlotRecord g_BattleSlotRecords[];

// One slot of the per-slot operator argument block at g_BattleSlotFileCount (stride
// 0x170): g_BattleWorkBlock + 0x100, i.e. the BattleTableEntry rows seen from
// BattleSlotState +0x5C. +0x04 and +0x08 are the state's scaled hp and maxHp,
// which BattleDwordRegSetFromSlotArgs reads and BattleSlotArgsStoreDwordFromReg
// writes. The writers below put one argument word into it as a dword, as a
// word, and as its two bytes, hence the byte-by-byte fields.
typedef struct {
    /* 0x00 */ u8 unk00[0x4];
    /* 0x04 */ u32 hp; // BattleSlotState.hp: BattleDwordRegSetFromSlotArgs copies it into a dword register
    /* 0x08 */ u32 maxHp; // BattleSlotState.maxHp: BattleSlotArgsStoreDwordFromReg stores into it when operand2 is 0
    /* 0x0C */ u8 unk0C[0x40];
    /* 0x4C */ u32 unk4C;
    /* 0x50 */ u8 unk50;
    /* 0x51 */ u8 unk51;
    /* 0x52 */ u8 unk52;
    /* 0x53 */ u8 unk53;
    /* 0x54 */ u8 unk54;
    /* 0x55 */ u8 unk55;
    /* 0x56 */ u16 unk56;
    /* 0x58 */ u8 unk58[0x118];
} BattleSlotArgs; // Size: 0x170

extern BattleSlotArgs g_BattleSlotFileCount[];

// One 8-byte row of the 0x100-byte battle command argument grid at
// g_BattleCommandArgs, which battle/main/misc4 names BattleCommandArg and walks
// actor by actor. A row's field is chosen by the instruction's operand1, so
// BattleStoreCommandBytes reaches it as a byte of the row.
typedef struct {
    /* 0x0 */ u8 opcode;
    /* 0x1 */ u8 unk1;
    /* 0x2 */ u8 unk2;
    /* 0x3 */ u8 unk3;
    /* 0x4 */ u8 unk4;
    /* 0x5 */ u8 unk5;
    /* 0x6 */ u16 slotMask;
} BattleCommandArg; // Size: 0x8

extern BattleCommandArg g_BattleCommandArgs[];

u8 GameRandomByteRange(int, u8);
u16 BattleRandomRange(int, u16);
u8 BattleSlotFlagsC002ClearRecordIdle(u8, u8);
u16 BattleSlotBitMask(u8);
s32 BattleFindFirstFlagTableMatch(u16);
s32 BattleSlotByteVarGetSet(u8, u8, u8, s32);
s32 BattleSlotWordVarGetSet(u8, u8, s32, s32);

// Preset screened twice, both times Default wins. gcc 2.7.2-cdk is ruled out:
// the 11 matched functions drop to 41.73-99.09% and, with maspsx
// --aspsx-version=2.67 as well, BattleClearByteRegs/BAB8 fall to 95.00% too. gcc 2.6.0
// keeps all 11 matched functions at 100.00% and, of the 13 bodies parked below,
// only BattlePickFreeActorSlotWithSameTag scores better (72.75% vs 65.42%) and it is still far off.
// So every park here is an allocator tie under all three, not a wrong compiler.

// best: 90.81%. The pTable = (u8 *)pField + 0x10 form is what puts the
// addiu 0x10 in the jal delay slot like the original. Remaining gap is the
// allocator: the original keeps slot in s0 and base+0x10 in s1, gcc folds
// slot+base into s0 before the call and needs one saved register fewer.
// screened: 90.81% is the max of Default / 2.7.2-cdk 80.16 / 2.6.0 90.81.
void BattleByteRegSetMatched(BattleCommand **p, u8 index) {
    u16 *pField = g_BattleSlotRegs[index].wordRegs;
    u8 value = BattleFindFirstFlagTableMatch(pField[(*p)->operand3]);
    u8 result = BattleSlotByteVarGetSet(value, (*p)->operand2, 0, 1);
    u8 *pTable = g_BattleSlotRegs[index].byteRegs;

    pTable[(*p)->operand1] = result;
}
u8 BattleByteRegCopyToMatchedSlot(BattleCommand **p, u8 index, u8 arg2) {
    u8 ret = arg2;
    u8 value = BattleFindFirstFlagTableMatch(g_BattleSlotRegs[index].wordRegs[(*p)->operand3]);

    if (value < 3) {
        g_BattleCommandArgs[0].opcode = 0x20;
        ret = arg2 + 1;
    } else {
        BattleSlotByteVarGetSet(value, (*p)->operand2, g_BattleSlotRegs[index].byteRegs[(*p)->operand1], 0);
    }
    return ret;
}
void BattleWordRegCopyFromMatchedSlot(BattleCommand **p, u8 index) {
    u16 *pTable = g_BattleSlotRegs[index].wordRegs;
    u8 value = BattleFindFirstFlagTableMatch(pTable[(*p)->operand3]);

    pTable[(*p)->operand1] = BattleSlotWordVarGetSet(value, (*p)->operand2, 0, 1);
}
u8 BattleWordRegCopyToMatchedSlot(BattleCommand **p, u8 index, u8 arg2) {
    u16 *pField = g_BattleSlotRegs[index].wordRegs;
    u8 ret = arg2;
    u8 value = BattleFindFirstFlagTableMatch(pField[(*p)->operand3]);

    if (value < 3) {
        g_BattleCommandArgs[0].opcode = 0x20;
        ret = arg2 + 1;
    } else {
        BattleSlotWordVarGetSet(value, (*p)->operand2, pField[(*p)->operand1], 0);
    }
    return ret;
}
void BattleDwordRegSetFromSlotArgs(BattleCommand **p, u8 index) {
    switch ((*p)->operand2) {
    case BATTLE_SLOT_ARG_SOURCE_HP: {
        u8 value = BattleFindFirstFlagTableMatch(g_BattleSlotRegs[index].wordRegs[(*p)->operand3]);

        g_BattleSlotRegs[index].dwordRegs[(*p)->operand1] = g_BattleSlotFileCount[value].hp;
        break;
    }
    case BATTLE_SLOT_ARG_SOURCE_GOLD:
        g_BattleSlotRegs[index].dwordRegs[(*p)->operand1] = g_GameState.gold;
        break;
    }
}
u8 BattleSlotArgsStoreDwordFromReg(BattleCommand **p, u8 index, u8 arg2) {
    u8 ret = arg2;
    u8 value = BattleFindFirstFlagTableMatch(g_BattleSlotRegs[index].wordRegs[(*p)->operand3]);

    if (value < 3) {
        g_BattleCommandArgs[0].opcode = 0x20;
        ret = arg2 + 1;
    } else if ((*p)->operand2 == 0) {
        g_BattleSlotFileCount[value].maxHp = g_BattleSlotRegs[index].dwordRegs[(*p)->operand1];
    } else {
        g_BattleSlotFileCount[value].hp = g_BattleSlotRegs[index].dwordRegs[(*p)->operand1];
    }
    return ret;
}
void BattleWordTableLoadReg(BattleCommand **p, u8 index) {
    BattleCommand *pCmd = *p;

    g_BattleSlotRegs[index].wordRegs[pCmd->operand1] = D_8005A3A0[pCmd->operand2];
}
void BattleWordTableStoreReg(BattleCommand **p, u8 index) {
    u16 *pField = g_BattleSlotRegs[index].wordRegs;
    BattleCommand *pCmd = *p;

    D_8005A3A0[pCmd->operand2] = pField[pCmd->operand1];
}
void BattleByteRegSetReg(BattleCommand **p, u8 index) {
    u8 *pTable = g_BattleSlotRegs[index].byteRegs;
    BattleCommand *pCmd = *p;

    pTable[pCmd->operand2] = pTable[pCmd->operand1];
}
void BattleWordRegSetReg(BattleCommand **p, u8 index) {
    // The dead pField is load-bearing: its initializer makes gcc emit the
    // index*0x40 shift before the base lui/addiu, as retail does, and the base
    // has to stay an untyped byte pointer for the two stores to share it
    // (the typed g_BattleSlotRegs[index].wordRegs spelling is 84.00%).
    u16 *pField = (u16 *)(g_BattleSlotWordRegs + index * 0x40);
    BattleCommand *pCmd = *p;
    u8 *base;

    base = g_BattleSlotWordRegs;
    ((u16 *)(base + index * 0x40))[pCmd->operand2] = ((u16 *)(base + index * 0x40))[pCmd->operand1];
}
void BattleDwordRegSetReg(BattleCommand **p, u8 index) {
    BattleCommand *pCmd = *p;

    g_BattleSlotRegs[index].dwordRegs[pCmd->operand2] = g_BattleSlotRegs[index].dwordRegs[pCmd->operand1];
}
void BattleWordRegSetFromByteReg(BattleCommand **p, u8 index) {
    BattleCommand *pCmd = *p;

    g_BattleSlotRegs[index].wordRegs[pCmd->operand2] = g_BattleSlotRegs[index].byteRegs[pCmd->operand1];
}
void BattleSlotStoreDwordFromWordReg(BattleCommand **p, u8 index) {
    u32 *pField32 = g_BattleSlotRegs[index].dwordRegs;
    u16 *pWordRegs;
    BattleCommand *pCmd = *p;

    // the assignment inside the subscript is load-bearing: with a plain local
    // gcc picks v1 for the stored halfword instead of retail's a0 (95.29%).
    pField32[pCmd->operand2] = (pWordRegs = g_BattleSlotRegs[index].wordRegs)[pCmd->operand1];
}
void BattleClearByteRegs(u8 index) {
    s32 i = 0xF;
    u8 *p = &g_BattleSlotRegs[index].byteRegs[0xF];

    for (; i >= 0; i--) {
        *p-- = 0;
    }
}
void BattleClearWordRegs(u8 index) {
    s32 i = 0x7;
    u16 *p = &g_BattleSlotRegs[index].wordRegs[0x7];

    for (; i >= 0; i--) {
        *p-- = 0;
    }
}
void BattleSlotStoreWord0(BattleCommand **p, u8 index) {
    s32 slot = index + 3;
    s32 hi = (*p)->operand2 << 8;

    g_BattleSlotFileCount[slot].unk4C = (*p)->operand1 | hi;
}
void BattleSlotStoreWord7A(BattleCommand **p, u8 index) {
    u32 value = (*p)->operand1 | ((*p)->operand2 << 8);
    s32 slot = index + 3;

    g_BattleSlotFileCount[slot].unk56 = value;
}
void BattleSlotStoreWordHighBytes(BattleCommand **p, u8 index) {
    s32 slot = index + 3;

    g_BattleSlotFileCount[slot].unk55 = (*p)->operand1;
    g_BattleSlotFileCount[slot].unk53 = (*p)->operand2;
    g_BattleSlotFileCount[slot].unk51 = (*p)->operand3;
}
void BattleSlotStoreWordLowBytes(BattleCommand **p, u8 index) {
    s32 slot = index + 3;

    g_BattleSlotFileCount[slot].unk54 = (*p)->operand1;
    g_BattleSlotFileCount[slot].unk52 = (*p)->operand2;
    g_BattleSlotFileCount[slot].unk50 = (*p)->operand3;
}
void BattleStoreCommandBytes(BattleCommand **p, BattleCommandArg *pArgs, u8 count) {
    ((u8 *)pArgs)[count * sizeof(BattleCommandArg) + (*p)->operand1] = (*p)->operand2;
    (count * sizeof(BattleCommandArg) + (*p)->operand1 + (u8 *)pArgs)[1] = (*p)->operand3;
}
void BattleByteRegSetRandom(BattleCommand **p, u8 index) {
    u8 value = GameRandomByteRange(0, (*p)->operand2);
    u8 *pTable = g_BattleSlotRegs[index].byteRegs;

    pTable[(*p)->operand1] = value;
}
void BattleWordRegSetRandomRange(BattleCommand **p, u8 index) {
    u16 value = BattleRandomRange(0, (*p)->operand2 | ((*p)->operand3 << 8));
    u16 *pField = g_BattleSlotRegs[index].wordRegs;

    pField[(*p)->operand1] = value;
}
// The two pointers are deliberately the untyped word bank: the typed
// g_BattleSlotRegs[index].wordRegs spelling folds the +0x20 into the symbol
// and names g_BattleSlotRegs instead of g_BattleSlotWordRegs in the relocation.
// The second copy of the row pointer is load-bearing (retail re-forms it in
// the store path; one variable leaves it in a1 and costs the frame).
void BattlePickRandomFreeCharSlot(BattleCommand **p, u8 index) {
    u8 buf[3];
    u16 *pField = (u16 *)(g_BattleSlotWordRegs + index * 0x40);

    buf[0] = buf[1] = buf[2] = 0;
    pField[(*p)->operand1] = 0;
    while ((buf[0] & buf[1] & buf[2]) == 0) {
        s32 slot = GameRandomByteRange(0, 2);
        u8 idx = slot;
        u8 tried = buf[idx];
        u16 *pLoopField = (u16 *)(g_BattleSlotWordRegs + index * 0x40);

        if (tried == 0) {
            if (BattleSlotFlagsC002ClearRecordIdle(idx, (*p)->operand2) != 0 && g_BattleSlotFlags[idx].inUse == 0) {
                pLoopField[(*p)->operand1] = BattleSlotBitMask(idx);
                break;
            }
            buf[idx] = 1;
        }
    }
}
// best: 76.81%. Same body as BattlePickRandomFreeCharSlot plus the g_BattleSlotRecords entry
// comparison; the frame/register set is off for the same reason - gcc
// hoists the absolute bases of g_BattleSlotRecords and g_BattleSlotFlags into extra
// callee-saved registers instead of re-forming `lui %hi` each iteration
// like retail (which only strength-reduces the *offsets*: s3 starts at
// 0x54 and steps by 0x1C). Nothing found yet that keeps the symbol inside
// the mem address.
// screened: cdk 77.76% and 2.6.0 76.81% against 76.81% Default - not close.
void BattlePickFreeCharSlotWithSameTag(BattleCommand **p, u8 index) {
    u8 buf[3];

    buf[0] = buf[1] = buf[2] = 0;
    g_BattleSlotRegs[index].wordRegs[(*p)->operand1] = 0;
    while ((buf[0] & buf[1] & buf[2]) == 0) {
        s32 slot = GameRandomByteRange(0, 2);
        u8 idx = slot;

        if (buf[idx] == 0) {
            if (BattleSlotFlagsC002ClearRecordIdle(idx, (*p)->operand2) != 0
                && g_BattleSlotRecords[index + 3].tag == g_BattleSlotRecords[idx].tag
                && g_BattleSlotFlags[idx].inUse == 0) {
                g_BattleSlotRegs[index].wordRegs[(*p)->operand1] = BattleSlotBitMask(idx);
                break;
            }
            buf[idx] = 1;
        }
    }
}
// best: 65.42%. Structure recovered (collect the 3..10 slots whose
// g_BattleSlotRecords entry equals the (index+3) one into a stack buffer, then pick
// one at random; both g_BattleSlotWordRegs stores are spelled inline, retail
// re-forms their base with a fresh lui as well). Gap is the same hoist as
// BattlePickFreeCharSlotWithSameTag (g_BattleSlotRecords/g_BattleSlotFlags bases into callee-saved registers,
// +8 frame). -G0 changes nothing.
// screened: 2.6.0 gives 72.75% vs 65.42% Default, still far off; cdk 65.48%.
void BattlePickFreeActorSlotWithSameTag(BattleCommand **p, u8 index) {
    u8 buf[8];
    s32 i = 3;
    s32 count = 0;
    s32 lastIndex;
    u16 *pField = g_BattleSlotRegs[index].wordRegs;

    pField[(*p)->operand1] = 0;
    for (; i < 0xB; i++) {
        u8 idx = i;

        if (BattleSlotFlagsC002ClearRecordIdle(idx, (*p)->operand2) != 0
            && g_BattleSlotRecords[index + 3].tag == g_BattleSlotRecords[i].tag
            && g_BattleSlotFlags[i].inUse == 0) {
            buf[count++] = idx;
        }
    }
    // retail forms the bound before the empty check, so it is a statement here
    lastIndex = count - 1;
    if (count != 0) {
        u16 result = BattleSlotBitMask(buf[GameRandomByteRange(0, lastIndex)]);
        s32 slot = (*p)->operand1;
        u16 *pRow = g_BattleSlotRegs[index].wordRegs;

        pRow[slot] = result;
    }
}
