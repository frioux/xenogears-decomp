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

void func_8007AC30(u8 **p, u8 index) {
    u8 *pArgs = *p;
    s32 value = g_BattleSlotWordRegs[index].wordRegs[pArgs[1]] - ((pArgs[3] << 8) + pArgs[2]);

    if (value < 0) {
        value = 0;
    }
    g_BattleSlotWordRegs[index].wordRegs[pArgs[1]] = value;
}
void func_8007AC80(u8 **p, u8 index) {
    u8 *pArgs = *p;
    s32 value = g_BattleSlotWordRegs[index].wordRegs[pArgs[1]] * ((pArgs[3] << 8) + pArgs[2]);

    if (value > BATTLE_WORD_REG_MAX) {
        value = BATTLE_WORD_REG_MAX;
    }
    g_BattleSlotWordRegs[index].wordRegs[pArgs[1]] = value;
}

// best 83.06% (cdk; 81.94 2.7.2/2.6.0; --expand-div drops it to 31.94, so not the
// div trap). The tie here is the same one func_8007AC30 had (the `la` temp for the
// table base against the operand temp: retail $v0/$v1, ours $v1/$v0), and its fix
// was to spell the bank access out at each site and to put the shifted byte first
// (`g_BattleSlotWordRegs[index].wordRegs[pArgs[1]] /= (pArgs[3] << 8) + pArgs[2]`);
// the body below is the older one and is what the 83.06% was measured on.
/*
void func_8007ACDC(u8 **p, u8 index) {
    u16 *pField = (u16 *)(g_BattleSlotWordRegs + index * 0x40);
    u8 *pArgs = *p;

    pField[pArgs[1]] /= pArgs[2] + (pArgs[3] << 8);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc6", func_8007ACDC);
// best 83.06% (cdk; 81.94 2.7.2/2.6.0): modulus form of func_8007ACDC; same residual
// and same untested lead as its `/=` sibling above.
/*
void func_8007AD24(u8 **p, u8 index) {
    u16 *pField = (u16 *)(g_BattleSlotWordRegs + index * 0x40);
    u8 *pArgs = *p;

    pField[pArgs[1]] %= pArgs[2] + (pArgs[3] << 8);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc6", func_8007AD24);
