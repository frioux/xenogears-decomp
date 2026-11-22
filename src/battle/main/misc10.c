#include "common.h"
#include "main/game.h"

// The battle actor slots: D_800C3EB7 holds a 0x1C-byte record per slot, and the loops here walk
// the non-party ones. Same value as include/battle/actor.h's BATTLE_ACTOR_SLOT_COUNT; duplicated
// here because this unit does not include that header.
// owned by include/battle/actor.h; this unit carries its own externs and cannot include it
#define BATTLE_ACTOR_SLOT_STRIDE 0x1C

#define BATTLE_ACTOR_SLOT_COUNT 11
// Bytes per slot record in D_800C3EB7: the stride of the byte-indexed walk and of the one flag
// byte read through it.
// The non-party actor slots: slot numbers MAX_PARTY_MEMBERS through BATTLE_ACTOR_SLOT_COUNT - 1,
// which are the ones the slot register file is indexed from 0 for.
#define BATTLE_OTHER_SLOT_COUNT (BATTLE_ACTOR_SLOT_COUNT - MAX_PARTY_MEMBERS)

// The per-slot battle register file (g_BattleSlotRegs[slot]): the dword, word and
// byte registers the script's comparison ops test against, indexed by the
// operand byte that follows the opcode.
typedef struct {
    /* 0x00 */ u8 unk0[0x10];
    /* 0x10 */ u32 dwordRegs[4];
    /* 0x20 */ u16 wordRegs[8];
    /* 0x30 */ u8 byteRegs[0x10];
} BattleSlotRegs; // Size: 0x40

// A 0x170-byte battle table entry; only the flag word at +0 is read here.
typedef struct {
    /* 0x000 */ u16 flags;
    /* 0x002 */ u8 unk02[0x16E];
} BattleTableEntry; // Size: 0x170

// A 4-byte status entry; only the first byte is read here.
typedef struct {
    /* 0x0 */ u8 unk0;
    /* 0x1 */ u8 unk1[3];
} BattleStatusEntry; // Size: 0x4

extern u8 D_800C3EB7[];
extern BattleTableEntry g_BattleSlotEntries[];
extern u8 g_BattleActorCharIds[];
extern u8 *g_BattleDisplayState;
extern BattleStatusEntry g_BattleCharSlotUsage[];
extern u16 g_BattleStateFlags;
extern BattleSlotRegs g_BattleSlotRegs[];

s32 BattleWordRegLeImm(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u16 value = p[2] | (p[3] << 8);

    return value >= g_BattleSlotRegs[arg1].wordRegs[p[1]];
}
s32 BattleByteRegGeImm(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;

    return !(g_BattleSlotRegs[arg1].byteRegs[p[1]] < p[2]);
}
s32 BattleWordRegGeImm(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u16 value = p[2] | (p[3] << 8);

    return g_BattleSlotRegs[arg1].wordRegs[p[1]] >= value;
}
s32 BattleByteRegEqReg(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;

    return g_BattleSlotRegs[arg1].byteRegs[p[1]] == g_BattleSlotRegs[arg1].byteRegs[p[2]];
}
s32 BattleWordRegEqReg(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;

    return g_BattleSlotRegs[arg1].wordRegs[p[1]] == g_BattleSlotRegs[arg1].wordRegs[p[2]];
}
s32 BattleByteRegLeReg(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;

    return g_BattleSlotRegs[arg1].byteRegs[p[1]] <= g_BattleSlotRegs[arg1].byteRegs[p[2]];
}
s32 BattleWordRegLeReg(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;

    return g_BattleSlotRegs[arg1].wordRegs[p[1]] <= g_BattleSlotRegs[arg1].wordRegs[p[2]];
}
s32 BattleByteRegTestImm(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    s32 v = g_BattleSlotRegs[arg1].byteRegs[p[1]];

    v &= p[2];
    return v != 0;
}
s32 BattleWordRegTestImm(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u16 value = p[2] + (p[3] << 8);

    return (g_BattleSlotRegs[arg1].wordRegs[p[1]] & value) != 0;
}
s32 BattleByteRegTestReg(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 lhs = g_BattleSlotRegs[arg1].byteRegs[p[1]];

    return (g_BattleSlotRegs[arg1].byteRegs[p[2]] & lhs) != 0;
}
s32 BattleWordRegTestReg(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u16 a = g_BattleSlotRegs[arg1].wordRegs[p[1]];
    u16 b = g_BattleSlotRegs[arg1].wordRegs[p[2]];

    return (b & a) != 0;
}
s32 BattleByteRegNeImm(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;

    return (g_BattleSlotRegs[arg1].byteRegs[p[1]] ^ p[2]) != 0;
}
s32 BattleWordRegNeImm(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u16 value = p[2] | (p[3] << 8);

    return (g_BattleSlotRegs[arg1].wordRegs[p[1]] ^ value) != 0;
}
s32 BattleByteRegNeReg(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;

    return (g_BattleSlotRegs[arg1].byteRegs[p[1]] ^ g_BattleSlotRegs[arg1].byteRegs[p[2]]) != 0;
}
s32 BattleWordRegNeReg(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;

    return (g_BattleSlotRegs[arg1].wordRegs[p[1]] ^ g_BattleSlotRegs[arg1].wordRegs[p[2]]) != 0;
}
s32 BattleDwordRegEqReg(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;

    return (g_BattleSlotRegs[arg1].dwordRegs[p[1]] ^ g_BattleSlotRegs[arg1].dwordRegs[p[2]]) == 0;
}
s32 BattleDwordRegLeReg(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;

    return g_BattleSlotRegs[arg1].dwordRegs[p[1]] <= g_BattleSlotRegs[arg1].dwordRegs[p[2]];
}
s32 BattleTableEntryFlag8000(u8 **arg0) {
    u8 *p = *arg0;

    return g_BattleSlotEntries[p[1]].flags >> 15;
}
s32 BattleSlotCountIsZero(u8 **arg0) {
    u8 *p = *arg0;

    return g_BattleCharSlotUsage[p[1]].unk0 == 0;
}
s32 BattleStateWordLowBitsClear(void) {
    return (g_BattleStateFlags & 3) == 0;
}
s32 func_8007EEE8(void) {
    s32 ret = 1;
    s32 i = 0;
    s32 ofs = MAX_PARTY_MEMBERS * BATTLE_ACTOR_SLOT_STRIDE;

    for (; i < BATTLE_OTHER_SLOT_COUNT; i++) {
        if ((g_BattleStateFlags >> 5) != 0) {
            if ((D_800C3EB7[ofs] & 0x80) == 0) {
                ret = 0;
                break;
            }
        }
        ofs += BATTLE_ACTOR_SLOT_STRIDE;
    }

    return ret;
}
s32 BattleActorSlotHasFlag80(u8 arg0) {
    s32 idx = arg0 + MAX_PARTY_MEMBERS;

    return D_800C3EB7[idx * BATTLE_ACTOR_SLOT_STRIDE] >> 7;
}
// The per-opcode handlers BattleExecScriptOpcode dispatches to. They live in misc4..misc9.c
// and in the still-unmatched stubs of this overlay; the grid-write ones take the
// byte grid pArgs (0x100 bytes of 8-byte rows) as their second argument.
extern u8 g_BattleCommandArgs[];


extern void BattleByteRegAddClamp(u8 **ppCommand, u8 slot);
extern void BattleByteRegAddImmClamp(u8 **ppCommand, u8 slot);
extern void BattleByteRegAnd(u8 **ppCommand, u8 slot);
extern void BattleByteRegAndImm(u8 **ppCommand, u8 slot);
extern void BattleByteRegDiv(u8 **ppCommand, u8 slot);
extern void BattleByteRegDivImm(u8 **ppCommand, u8 slot);
extern void BattleByteRegMod(u8 **ppCommand, u8 slot);
extern void BattleByteRegModImm(u8 **ppCommand, u8 slot);
extern void BattleByteRegMulClamp(u8 **ppCommand, u8 slot);
extern void BattleByteRegOr(u8 **ppCommand, u8 slot);
extern void BattleByteRegOrImm(u8 **ppCommand, u8 slot);
extern void BattleByteRegSetImm(u8 **ppCommand, u8 slot);
extern void BattleByteRegSetMatched(u8 **ppCommand, u8 slot);
extern void BattleByteRegSetRandom(u8 **ppCommand, u8 slot);
extern void BattleByteRegSetReg(u8 **ppCommand, u8 slot);
extern void BattleByteRegSubClamp(u8 **ppCommand, u8 slot);
extern void BattleByteRegSubImmClamp(u8 **ppCommand, u8 slot);
extern void BattleByteRegXor(u8 **ppCommand, u8 slot);
extern void BattleByteRegXorImm(u8 **ppCommand, u8 slot);
extern void BattleClearByteRegs(u8 slot);
extern void BattleClearSlotDwordAndSetStat4(u8 slot);
extern void BattleClearWordRegs(u8 slot);
extern void BattleDwordRegDivImm(u8 **ppCommand, u8 slot);
extern void BattleGetFontGlyph(u8 **ppCommand);
extern void BattleGridCopyRow(u8 **ppCommand, u8 *pArgs);
extern s32 BattleGridWriteByteAdvanceRow(u8 **ppCommand, u8 *pArgs, u8 row);
extern void BattleGridWriteByteReg(u8 **ppCommand, u8 *pArgs, u8 slot, u8 row);
extern s32 BattleGridWriteOpcodeRow(u8 **ppCommand, u8 *pArgs, u8 slot, u8 row);
extern void BattlePickActorSlotByLowestEntryWord(u8 **ppCommand, u8 slot);
extern void BattlePickActorSlotByLowestEntryWordInUse(u8 **ppCommand, u8 slot);
extern void BattlePickActorSlotByLowestSlotWord(u8 **ppCommand, u8 slot);
extern void BattlePickActorSlotWithDifferentTag(u8 **ppCommand, u8 slot);
extern void BattlePickActorSlotWithFlag80(u8 **ppCommand, u8 slot);
extern void BattlePickCharSlotByEntryFlags(u8 **ppCommand, u8 slot);
extern void BattlePickCharSlotByFlagRetry(u8 **ppCommand, u8 slot);
extern void BattlePickCharSlotByLowestEntryDwordInUse(u8 **ppCommand, u8 slot);
extern void BattlePickCharSlotByLowestEntryWord(u8 **ppCommand, u8 slot);
extern void BattlePickCharSlotByLowestSlotWord(u8 **ppCommand, u8 slot);
extern void BattlePickCharSlotWithDifferentTag(u8 **ppCommand, u8 slot);
extern void BattlePickCharSlotWithSameTag(u8 **ppCommand, u8 slot);
extern void BattlePickSlotByActorTag(u8 **ppCommand, u8 slot);
extern void BattlePickSlotByFlagTable(u8 **ppCommand, u8 slot);
extern void BattleResetDisplayValues();
extern void BattleSetPendingCharIndex(u8 **ppCommand, u8 slot);
extern void BattleSetSlotRecordByteAndRelease(u8 **ppCommand, u8 slot);
extern void BattleSlotSetByteField(u8 **ppCommand, u8 slot);
extern void BattleSlotSetWordField(u8 **ppCommand, u8 slot);
extern void BattleSlotStoreDwordFromWordReg(u8 **ppCommand, u8 slot);
extern void BattleSlotStoreWord0(u8 **ppCommand, u8 slot);
extern void BattleSlotStoreWord7A(u8 **ppCommand, u8 slot);
extern void BattleSlotStoreWordHighBytes(u8 **ppCommand, u8 slot);
extern void BattleSlotStoreWordLowBytes(u8 **ppCommand, u8 slot);
extern void BattleStoreActorSlotFlagsC000ClearCount(u8 **ppCommand, u8 slot);
extern void BattleStoreActorSlotMatchedTagCount(u8 **ppCommand, u8 slot);
extern void BattleStoreActorSlotTagCount(u8 **ppCommand, u8 slot);
extern void BattleStoreCharSlotFlagMask(u8 **ppCommand, u8 slot);
extern void BattleStoreCharSlotFlagsC000ClearCount(u8 **ppCommand, u8 slot);
extern void BattleStoreCharSlotMatchedTagCount(u8 **ppCommand, u8 slot);
extern void BattleStoreCharSlotTagCount(u8 **ppCommand, u8 slot);
extern void BattleStoreCommandUseCount(u8 **ppCommand, u8 slot);
extern void BattleStoreGoldWord32(u8 **ppCommand, u8 slot);
extern void BattleStoreSlotParamTableByte(u8 **ppCommand, u8 slot);
extern void BattleStoreStatWord32(u8 **ppCommand, u8 slot);
extern void BattleWordRegAddClamp(u8 **ppCommand, u8 slot);
extern void BattleWordRegAddImmClamp(u8 **ppCommand, u8 slot);
extern void BattleWordRegAnd(u8 **ppCommand, u8 slot);
extern void BattleWordRegAndImm(u8 **ppCommand, u8 slot);
extern void BattleWordRegCopyFromMatchedSlot(u8 **ppCommand, u8 slot);
extern s32 BattleWordRegCopyToMatchedSlot(u8 **ppCommand, u8 slot, u8 row);
extern void BattleWordRegDiv(u8 **ppCommand, u8 slot);
extern void BattleWordRegMod(u8 **ppCommand, u8 slot);
extern void BattleWordRegMulClamp(u8 **ppCommand, u8 slot);
extern void BattleWordRegOr(u8 **ppCommand, u8 slot);
extern void BattleWordRegOrImm(u8 **ppCommand, u8 slot);
extern void BattleWordRegSetImm(u8 **ppCommand, u8 slot);
extern void BattleWordRegSetRandomRange(u8 **ppCommand, u8 slot);
extern void BattleWordRegSetReg(u8 **ppCommand, u8 slot);
extern void BattleWordRegSetSlotBitMask(u8 **ppCommand, u8 slot);
extern void BattleWordRegSubClamp(u8 **ppCommand, u8 slot);
extern void BattleWordRegXor(u8 **ppCommand, u8 slot);
extern void BattleWordRegXorImm(u8 **ppCommand, u8 slot);
extern void BattleWordTableStoreImm(u8 **ppCommand);
extern void BattleWordTableStoreReg(u8 **ppCommand, u8 slot);
extern void BattleWriteRowFieldBytes(u8 **ppCommand, u8 *pArgs, u8 slot, u8 row);
extern void BattleWriteWorkBufferWord(u8 **ppCommand);
extern void BattleDwordRegSetImm(u8 **ppCommand, u8 slot);
extern void func_8007AA60(u8 **ppCommand, u8 slot);
extern void BattleWordRegSubImmClamp(u8 **ppCommand, u8 slot);
extern void BattleWordRegMulImmClamp(u8 **ppCommand, u8 slot);
extern void func_8007ACDC(u8 **ppCommand, u8 slot);
extern void func_8007AD24(u8 **ppCommand, u8 slot);
extern s32 BattleByteRegCopyToMatchedSlot(u8 **ppCommand, u8 slot, u8 row);
extern void BattleDwordRegSetFromSlotArgs(u8 **ppCommand, u8 slot);
extern s32 BattleSlotArgsStoreDwordFromReg(u8 **ppCommand, u8 slot, u8 row);
extern void BattleWordTableLoadReg(u8 **ppCommand, u8 slot);
extern void BattleDwordRegSetReg(u8 **ppCommand, u8 slot);
extern void BattleWordRegSetFromByteReg(u8 **ppCommand, u8 slot);
extern void BattleStoreCommandBytes(u8 **ppCommand, u8 *pArgs, u8 row);
extern void func_8007BD5C(u8 **ppCommand, u8 slot);
extern void BattlePickFreeCharSlotWithSameTag(u8 **ppCommand, u8 slot);
extern void BattlePickFreeActorSlotWithSameTag(u8 **ppCommand, u8 slot);
extern void func_8007D8C0(u8 **ppCommand, u8 slot);
extern void func_8007DA1C(u8 **ppCommand, u8 slot);
extern void func_8007DB78(u8 **ppCommand, u8 slot);
extern void func_8007DCF8(u8 **ppCommand, u8 slot);
extern void func_8007DE78(u8 **ppCommand, u8 slot);
extern void func_8007DFD4(u8 **ppCommand, u8 slot);
extern void func_8007E334(u8 **ppCommand, u8 slot);
extern void func_8007E438(u8 **ppCommand, u8 slot);
extern void func_8007E554(u8 **ppCommand, u8 slot);
extern void func_8007E6A0(u8 **ppCommand, u8 slot);
extern void func_8007E6F0(u8 **ppCommand, u8 slot);
extern void BattleDwordRegMulImm(u8 **ppCommand, u8 slot);
extern void BattleSetStatusBitAllSlots(u8 **ppCommand, u8 slot);

extern void BattleScriptCursorSkipWord(u8 **ppCommand);
// Comparison opcodes whose definitions live in misc9_260.c.
extern s32 BattleSetTextScale(u8 **ppCommand, u8 slot);
extern s32 BattleWordRegEqImm(u8 **ppCommand, u8 slot);
extern s32 BattleByteRegLeImm(u8 **ppCommand, u8 slot);

s32 BattleExecScriptOpcode(u8 **ppCommand, u8 slot, s32 row) {
    u8 *pCommand = *ppCommand;
    u8 *pArgs = g_BattleCommandArgs;

    switch (pCommand[0]) {
    case 0x01:
        row = BattleGridWriteByteAdvanceRow(ppCommand, pArgs, row);
        break;
    case 0x02:
        BattleGridWriteByteReg(ppCommand, pArgs, slot, row);
        break;
    case 0x03:
        BattleGridCopyRow(ppCommand, pArgs);
        break;
    case 0x04:
        BattleByteRegSetImm(ppCommand, slot);
        break;
    case 0x05:
        BattleWordRegSetImm(ppCommand, slot);
        break;
    case 0x06:
        BattleDwordRegSetImm(ppCommand, slot);
        break;
    case 0x07:
        BattleWordTableStoreImm(ppCommand);
        break;
    case 0x08:
        BattleByteRegAddImmClamp(ppCommand, slot);
        break;
    case 0x09:
        BattleByteRegSubImmClamp(ppCommand, slot);
        break;
    case 0x0A:
        func_8007AA60(ppCommand, slot);
        break;
    case 0x0B:
        BattleByteRegDivImm(ppCommand, slot);
        break;
    case 0x0C:
        BattleByteRegModImm(ppCommand, slot);
        break;
    case 0x0D:
        BattleByteRegAndImm(ppCommand, slot);
        break;
    case 0x0E:
        BattleByteRegOrImm(ppCommand, slot);
        break;
    case 0x0F:
        BattleByteRegXorImm(ppCommand, slot);
        break;
    case 0x10:
        BattleWordRegAddImmClamp(ppCommand, slot);
        break;
    case 0x11:
        BattleWordRegSubImmClamp(ppCommand, slot);
        break;
    case 0x12:
        BattleWordRegMulImmClamp(ppCommand, slot);
        break;
    case 0x13:
        func_8007ACDC(ppCommand, slot);
        break;
    case 0x14:
        func_8007AD24(ppCommand, slot);
        break;
    case 0x15:
        BattleWordRegAndImm(ppCommand, slot);
        break;
    case 0x16:
        BattleWordRegOrImm(ppCommand, slot);
        break;
    case 0x17:
        BattleWordRegXorImm(ppCommand, slot);
        break;
    case 0x18:
        BattleByteRegAddClamp(ppCommand, slot);
        break;
    case 0x19:
        BattleByteRegSubClamp(ppCommand, slot);
        break;
    case 0x1A:
        BattleByteRegMulClamp(ppCommand, slot);
        break;
    case 0x1B:
        BattleByteRegDiv(ppCommand, slot);
        break;
    case 0x1C:
        BattleByteRegMod(ppCommand, slot);
        break;
    case 0x1D:
        BattleByteRegAnd(ppCommand, slot);
        break;
    case 0x1E:
        BattleByteRegOr(ppCommand, slot);
        break;
    case 0x1F:
        BattleByteRegXor(ppCommand, slot);
        break;
    case 0x20:
        BattleWordRegAddClamp(ppCommand, slot);
        break;
    case 0x21:
        BattleWordRegSubClamp(ppCommand, slot);
        break;
    case 0x22:
        BattleWordRegMulClamp(ppCommand, slot);
        break;
    case 0x23:
        BattleWordRegDiv(ppCommand, slot);
        break;
    case 0x24:
        BattleWordRegMod(ppCommand, slot);
        break;
    case 0x25:
        BattleWordRegAnd(ppCommand, slot);
        break;
    case 0x26:
        BattleWordRegOr(ppCommand, slot);
        break;
    case 0x27:
        BattleWordRegXor(ppCommand, slot);
        break;
    case 0x28:
        BattleSlotSetByteField(ppCommand, slot);
        break;
    case 0x29:
        BattleSlotSetWordField(ppCommand, slot);
        break;
    case 0x2A:
        BattleByteRegSetMatched(ppCommand, slot);
        break;
    case 0x2B:
        row = BattleByteRegCopyToMatchedSlot(ppCommand, slot, row);
        break;
    case 0x2C:
        BattleWordRegCopyFromMatchedSlot(ppCommand, slot);
        break;
    case 0x2D:
        row = BattleWordRegCopyToMatchedSlot(ppCommand, slot, row);
        break;
    case 0x2E:
        BattleDwordRegSetFromSlotArgs(ppCommand, slot);
        break;
    case 0x2F:
        row = BattleSlotArgsStoreDwordFromReg(ppCommand, slot, row);
        break;
    case 0x30:
        BattleWordTableLoadReg(ppCommand, slot);
        break;
    case 0x31:
        BattleWordTableStoreReg(ppCommand, slot);
        break;
    case 0x32:
        BattleByteRegSetReg(ppCommand, slot);
        break;
    case 0x33:
        BattleWordRegSetReg(ppCommand, slot);
        break;
    case 0x34:
        BattleDwordRegSetReg(ppCommand, slot);
        break;
    case 0x35:
        BattleWordRegSetFromByteReg(ppCommand, slot);
        break;
    case 0x36:
        BattleSlotStoreDwordFromWordReg(ppCommand, slot);
        break;
    case 0x37:
        BattleClearByteRegs(slot);
        break;
    case 0x38:
        BattleClearWordRegs(slot);
        break;
    case 0x39:
        BattleSlotStoreWord0(ppCommand, slot);
        break;
    case 0x3A:
        BattleSlotStoreWord7A(ppCommand, slot);
        break;
    case 0x3B:
        BattleSlotStoreWordHighBytes(ppCommand, slot);
        break;
    case 0x3C:
        BattleSlotStoreWordLowBytes(ppCommand, slot);
        break;
    case 0x3D:
        BattleStoreCommandBytes(ppCommand, pArgs, row);
        break;
    case 0x3E:
        BattleByteRegSetRandom(ppCommand, slot);
        break;
    case 0x3F:
        BattleWordRegSetRandomRange(ppCommand, slot);
        break;
    case 0x40:
        func_8007BD5C(ppCommand, slot);
        break;
    case 0x41:
        BattlePickFreeCharSlotWithSameTag(ppCommand, slot);
        break;
    case 0x42:
        BattlePickFreeActorSlotWithSameTag(ppCommand, slot);
        break;
    case 0x43:
        BattlePickCharSlotWithDifferentTag(ppCommand, slot);
        break;
    case 0x44:
        BattlePickActorSlotWithDifferentTag(ppCommand, slot);
        break;
    case 0x45:
        BattlePickCharSlotByLowestSlotWord(ppCommand, slot);
        break;
    case 0x46:
        BattlePickActorSlotByLowestSlotWord(ppCommand, slot);
        break;
    case 0x47:
        BattlePickCharSlotByLowestEntryWord(ppCommand, slot);
        break;
    case 0x48:
        BattlePickActorSlotByLowestEntryWord(ppCommand, slot);
        break;
    case 0x49:
        BattlePickCharSlotWithSameTag(ppCommand, slot);
        break;
    case 0x4A:
        BattlePickCharSlotByFlagRetry(ppCommand, slot);
        break;
    case 0x4B:
        BattlePickSlotByFlagTable(ppCommand, slot);
        break;
    case 0x4C:
        BattleStoreCharSlotTagCount(ppCommand, slot);
        break;
    case 0x4D:
        BattleStoreActorSlotTagCount(ppCommand, slot);
        break;
    case 0x4E:
        BattleStoreSlotParamTableByte(ppCommand, slot);
        break;
    case 0x4F:
        BattleStoreCharSlotMatchedTagCount(ppCommand, slot);
        break;
    case 0x50:
        BattleStoreActorSlotMatchedTagCount(ppCommand, slot);
        break;
    case 0x51:
        BattleStoreCommandUseCount(ppCommand, slot);
        break;
    case 0x52:
        BattleWriteRowFieldBytes(ppCommand, pArgs, slot, row);
        break;
    case 0x53:
        BattleStoreGoldWord32(ppCommand, slot);
        break;
    case 0x54:
        BattlePickSlotByActorTag(ppCommand, slot);
        break;
    case 0x55:
        BattleStoreStatWord32(ppCommand, slot);
        break;
    case 0x56:
        BattlePickActorSlotWithFlag80(ppCommand, slot);
        break;
    case 0x57:
        BattlePickCharSlotByEntryFlags(ppCommand, slot);
        break;
    case 0x58:
        BattleStoreCharSlotFlagsC000ClearCount(ppCommand, slot);
        break;
    case 0x59:
        BattleStoreActorSlotFlagsC000ClearCount(ppCommand, slot);
        break;
    case 0x5A:
        BattlePickCharSlotByLowestEntryDwordInUse(ppCommand, slot);
        break;
    case 0x5B:
        BattlePickActorSlotByLowestEntryWordInUse(ppCommand, slot);
        break;
    case 0x5C:
        func_8007D8C0(ppCommand, slot);
        break;
    case 0x5D:
        func_8007DA1C(ppCommand, slot);
        break;
    case 0x5E:
        func_8007DB78(ppCommand, slot);
        break;
    case 0x5F:
        func_8007DCF8(ppCommand, slot);
        break;
    case 0x60:
        func_8007DE78(ppCommand, slot);
        break;
    case 0x61:
        func_8007DFD4(ppCommand, slot);
        break;
    case 0x62: // no handler: opcode 0x62 only marks the script (BattleScriptScanForOpcode62)
        break;
    case 0x63:
        BattleSetSlotRecordByteAndRelease(ppCommand, slot);
        break;
    case 0x64:
        BattleWordRegSetSlotBitMask(ppCommand, slot);
        break;
    case 0x65:
        BattleStoreCharSlotFlagMask(ppCommand, slot);
        break;
    case 0x66:
        func_8007E334(ppCommand, slot);
        break;
    case 0x67:
        func_8007E438(ppCommand, slot);
        break;
    case 0x68:
        func_8007E554(ppCommand, slot);
        break;
    case 0x69:
        BattleClearSlotDwordAndSetStat4(slot);
        break;
    case 0x6A:
        func_8007E6A0(ppCommand, slot);
        break;
    case 0x6B:
        func_8007E6F0(ppCommand, slot);
        break;
    case 0x6C:
        BattleDwordRegMulImm(ppCommand, slot);
        break;
    case 0x6D:
        BattleDwordRegDivImm(ppCommand, slot);
        break;
    case 0x70:
        BattleWriteWorkBufferWord(ppCommand);
        break;
    case 0x71:
        BattleSetStatusBitAllSlots(ppCommand, slot);
        break;
    case 0x72:
        BattleGetFontGlyph(ppCommand);
        break;
    case 0x73:
        BattleSetPendingCharIndex(ppCommand, slot);
        break;
    case 0x74:
        BattleResetDisplayValues();
        break;
    default:
        row = BattleGridWriteOpcodeRow(ppCommand, pArgs, slot, row);
        break;
    }

    BattleScriptCursorSkipWord(ppCommand);
    return row & 0xFF;
}

/* Best: 99.30%, 684 bytes against retail 688. The switch (28 cases, jtbl in this unit), the loop and every call site match; the ONLY residual is one missing copy at the loop bottom. Retail: or v1,s3,s0 ; move s3,v1 (beqz slot) ; move s0,s3 ; andi v0,s0,0xff. This body: or s0,s3,s0 ; move s3,s0 ; andi v0,s0,0xff - cc1 always coalesces acc and ret into one register, so the temp copy disappears. Tried: ret=acc moved after the loop (93.49, gcc folds to or s3,s3,s0 in the beqz slot and drops the ret copy), ret=next with return acc and 0xff (99.30, byte-identical to this body), chained acc/ret assignment, u8 opcode local, for(;;)+break, two inline returns, and a third permuter run from this body (1479 iterations, RESULT PERMUTER_NONE; rounds 173/previous also NONE). Preset is not the cause: a --sweep puts this body at 99.3 under every 2.7.2 -G4/-G8 combination, 94.59 under 2.7.2-cdk -O2. The note below was the pre-existing park text.
   Retail loop tail: andi v0,s4,0xff ; beqz v0,exit ; andi v0,s0,0xff ; lw v0,0(s1) ; or v1,s3,s0 ; lbu v0,0(v0) ; nop ; sltiu v0,v0,0x80 ; beqz v0,looptop ; addu s3,v1,zero ; addu s0,s3,zero ; andi v0,s0,0xff.
   BattleSetTextScale, BattleWordRegEqImm and BattleByteRegLeImm are declared at the top of this file with a u8 second parameter, which is what produces the andi a1,s2,0xff at each call.
*/
/*
s32 func_8007F8C0(u8 **pCursor, u8 arg1) {
    s32 ret = 0;
    s32 cont = 0;
    s32 acc = 0;
    s32 next;

    do {
        switch ((*pCursor)[0]) {
        case 0x80:
        case 0x9A:
            ret = 1;
            break;
        case 0x99:
            cont = 1;
            break;
        case 0x81:
            ret = BattleSetTextScale(pCursor, arg1);
            break;
        case 0x82:
            ret = BattleWordRegEqImm(pCursor, arg1);
            break;
        case 0x83:
            ret = BattleByteRegLeImm(pCursor, arg1);
            break;
        case 0x84:
            ret = BattleWordRegLeImm(pCursor, arg1);
            break;
        case 0x85:
            ret = BattleByteRegGeImm(pCursor, arg1);
            break;
        case 0x86:
            ret = BattleWordRegGeImm(pCursor, arg1);
            break;
        case 0x87:
            ret = BattleByteRegEqReg(pCursor, arg1);
            break;
        case 0x88:
            ret = BattleWordRegEqReg(pCursor, arg1);
            break;
        case 0x89:
            ret = BattleByteRegLeReg(pCursor, arg1);
            break;
        case 0x8A:
            ret = BattleWordRegLeReg(pCursor, arg1);
            break;
        case 0x8B:
            ret = BattleByteRegTestImm(pCursor, arg1);
            break;
        case 0x8C:
            ret = BattleWordRegTestImm(pCursor, arg1);
            break;
        case 0x8D:
            ret = BattleByteRegTestReg(pCursor, arg1);
            break;
        case 0x8E:
            ret = BattleWordRegTestReg(pCursor, arg1);
            break;
        case 0x8F:
            ret = BattleByteRegNeImm(pCursor, arg1);
            break;
        case 0x90:
            ret = BattleWordRegNeImm(pCursor, arg1);
            break;
        case 0x91:
            ret = BattleByteRegNeReg(pCursor, arg1);
            break;
        case 0x92:
            ret = BattleWordRegNeReg(pCursor, arg1);
            break;
        case 0x93:
            ret = BattleDwordRegEqReg(pCursor, arg1);
            break;
        case 0x94:
            ret = BattleDwordRegLeReg(pCursor, arg1);
            break;
        case 0x95:
            ret = BattleTableEntryFlag8000(pCursor);
            break;
        case 0x96:
            ret = BattleSlotCountIsZero(pCursor);
            break;
        case 0x97:
            ret = BattleStateWordLowBitsClear();
            break;
        case 0x98:
            ret = func_8007EEE8();
            break;
        case 0x9B:
            ret = BattleActorSlotHasFlag80(arg1);
            break;
        }
        BattleScriptCursorSkipWord(pCursor);
        if (!(cont & 0xFF)) {
            return ret & 0xFF;
        }
        next = acc | ret;
        acc = next;
        ret = acc;
    } while ((*pCursor)[0] >= 0x80);

    return ret & 0xFF;
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc10", func_8007F8C0);

void BattleReleaseEffectSlotIfChar4(u8 arg0) {
    if (g_BattleActorCharIds[arg0] == 4 && g_BattleDisplayState[0x8E] != 0) {
        BattleFreeSlotBuffers(0);
        BattleFreePacketPrimitives();
        g_BattleDisplayState[0x8E] = 0;
    }
}
