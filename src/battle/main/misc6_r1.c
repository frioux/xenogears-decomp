#include "common.h"

// One slot of the per-slot operator register block at g_BattleSlotRegs, seen from
// its word registers (+0x20 there, stride 0x40): the command's first operand byte
// picks the register and its two-byte immediate is the value applied to it.
// Declared from this bank because the original addresses 0x800D3420 itself;
// writing g_BattleSlotRegs[index].wordRegs moves the +0x20 into the load and
// loses the match.
typedef struct {
    /* 0x00 */ u16 wordRegs[8];
    /* 0x10 */ u8 unk10[0x30]; // the slot's byte registers, then the next slot's head
} BattleSlotWordRegs; // Size: 0x40

extern BattleSlotWordRegs D_800D3420[];

void BattleWordRegAndImm(u8 **p, u8 index) {
    u16 *pWordRegs = D_800D3420[index].wordRegs;
    u8 *pArgs = *p;

    pWordRegs[pArgs[1]] &= pArgs[2] + (pArgs[3] << 8);
}
void BattleWordRegOrImm(u8 **p, u8 index) {
    u16 *pWordRegs = D_800D3420[index].wordRegs;
    u8 *pArgs = *p;

    pWordRegs[pArgs[1]] |= pArgs[2] + (pArgs[3] << 8);
}
void BattleWordRegXorImm(u8 **p, u8 index) {
    u16 *pWordRegs = D_800D3420[index].wordRegs;
    u8 *pArgs = *p;

    pWordRegs[pArgs[1]] ^= pArgs[2] + (pArgs[3] << 8);
}
