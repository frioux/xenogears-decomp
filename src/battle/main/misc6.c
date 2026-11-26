#include "common.h"

// The word-register bank of the per-slot register block at g_BattleSlotRegs, seen
// from g_BattleSlotWordRegs (its +0x20, stride 0x40): the instruction's operand1
// byte picks the register and its two-byte immediate is the value applied to it.
// Declared from this bank because the original addresses 0x800D3420 itself.
typedef struct {
    /* 0x00 */ u16 wordRegs[8];
    /* 0x10 */ u8 unk10[0x30]; // the slot's byte registers, then the next slot's head
} BattleSlotWordRegs; // Size: 0x40

extern BattleSlotWordRegs g_BattleSlotWordRegs[];

// The clamp of an unsigned 16-bit register: the operators that can overflow a word
// register saturate at its largest value before storing.
#define BATTLE_WORD_REG_MAX 0xFFFF

void BattleWordRegSubImmClamp(u8 **p, u8 index) {
    u8 *pArgs = *p;
    s32 value = g_BattleSlotWordRegs[index].wordRegs[pArgs[1]] - ((pArgs[3] << 8) + pArgs[2]);

    if (value < 0) {
        value = 0;
    }
    g_BattleSlotWordRegs[index].wordRegs[pArgs[1]] = value;
}
void BattleWordRegMulImmClamp(u8 **p, u8 index) {
    u8 *pArgs = *p;
    s32 value = g_BattleSlotWordRegs[index].wordRegs[pArgs[1]] * ((pArgs[3] << 8) + pArgs[2]);

    if (value > BATTLE_WORD_REG_MAX) {
        value = BATTLE_WORD_REG_MAX;
    }
    g_BattleSlotWordRegs[index].wordRegs[pArgs[1]] = value;
}

void BattleWordRegDivImm(u8 **p, u8 index) {
    u8 *pArgs = *p;

    g_BattleSlotWordRegs[index].wordRegs[pArgs[1]] /= (pArgs[3] << 8) + pArgs[2];
}

void BattleWordRegModImm(u8 **p, u8 index) {
    u8 *pArgs = *p;

    g_BattleSlotWordRegs[index].wordRegs[pArgs[1]] %= (pArgs[3] << 8) + pArgs[2];
}
