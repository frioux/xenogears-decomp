#include "common.h"
#include "battle/actor.h"
#include "main/game.h"

// The percent chance BattleAdjustSlotAmountByCondition halves an action's amount instead of adding
// half of it, when the target's accessoryEffectFlags5 has 0x80; a target whose characterId is 0 is
// rolled against the higher one.
#define BATTLE_AMOUNT_HALVE_CHANCE_PERCENT 60
#define BATTLE_AMOUNT_HALVE_CHANCE_PERCENT_CHARACTER_0 80

// The percent chance BattleAdjustSlotAmountByCondition takes the target's activeEffectFlags2 bit
// 0x2000 and the acting slot's activeEffectFlags1 bit 0x1000 off the target.
#define BATTLE_EFFECT_REMOVE_CHANCE_PERCENT 80

// The row's power is in twentieths: `power * stat / this` leaves the action's amount unchanged.

// A slot's penalty percent scales an action's value by (100 - it) / 100, and a full one would leave
// nothing, so BattleTryApplyCondition clamps it one step below.
#define BATTLE_PERCENT_FULL 100
#define BATTLE_PENALTY_PERCENT_MAX 99

// amountMultiplier is in quarter units, this being the value that leaves the action's amount
// unchanged.
#define BATTLE_AMOUNT_MULTIPLIER_UNCHANGED 4

// The amount BattleComputeActionAmountFromSlotStates clamps an action's value to before storing it in the work block.

extern u8 *D_800C3D60;
extern BattleRow *g_BattleCurrentRow;
extern BattleTableEntry *g_BattleActingSlot;
extern BattleTableEntry *g_BattleTargetSlotEntry;
extern BattleWorkBlock *g_BattleCurWorkBlock;
extern BattleSlotState *g_BattleCurSlotState;
extern BattleSlotState *g_BattleTargetSlotState;
extern u8 g_BattleCurSlotIndex;
extern u8 g_BattleTargetSlotIndex;
extern u8 g_BattleAmountThirdFlag;
extern u8 g_BattleCommandCharIndex;
extern u8 *g_BattleCommandCounter;
extern s32 D_800C34DC[];
extern BattleTableEntry g_BattleWorkBlock[];
extern u16 g_BattleActionSlotMask;
extern s32 g_BattleCurveTargets[];
extern u8 g_BattleCurveTargetModes[];
extern s8 BattleTryApplyCondition(u8);
extern s32 BattleComputeSlotActionAmount();
extern s32 func_8009D3A0(void);
extern void func_80096494(u16 *, u16 *, u8 *);
extern void BattleSetSlotEntryFieldsTo2000(u8);
extern void BattleRollPercentAndSetState38(void);

void BattleSetActorMaskForSlotRecord3(void) {
    u8 i = 0;
    s32 target = 3; /* local, not a literal: the compare constant must live in a
                       register outside the loop */
    u16 *p = &g_BattleActionSlotMask;

    for (; i < MAX_PARTY_MEMBERS; i++) {
        if (g_BattleWorkBlock[i].characterId == target) {
            *p = 1 << i;
        }
    }
}
void BattleExecuteActionRow(void) {
    BattleTableEntry *pSlot;
    u16 flags;
    u16 mask;
    u32 actor;
    u8 one;

    if (g_BattleTargetSlotIndex < MAX_PARTY_MEMBERS) {
        g_BattleCurrentRow = (BattleRow *)((u8 *)g_BattleCurWorkBlock +
                                   (g_BattleCurWorkBlock->actorIndex * 0x690 + 0x2228) +
                                   g_BattleCurWorkBlock->rowIndex * 0x28);
    } else {
        g_BattleCurrentRow = (BattleRow *)((u8 *)g_BattleCurWorkBlock +
                                   (g_BattleCurWorkBlock->rowIndex * 0x28 + 0x35D8));
    }
    BattleSetSlotEntryFieldsTo2000(g_BattleTargetSlotIndex);
    actor = g_BattleCurWorkBlock->actorIndex;
    g_BattleAmountThirdFlag = 0;
    g_BattleCommandCounter = &g_BattleCurWorkBlock->slots[actor].state.commandCounter;
    if (g_BattleCurrentRow->actionFlags & 0x100) {
        if (g_BattleActingSlot->characterId == 1) {
            BattleRollPercentAndSetState38();
        }
    }
    if (g_BattleActingSlot->gearId == 5 || g_BattleActingSlot->gearId == 0xD) {
        if (g_BattleCurWorkBlock->rowIndex == 1) {
            g_BattleCurrentRow->element = g_BattleTargetSlotState->weaponData[1].attack;
        }
    }
    mask = 1;
    g_BattleCurSlotIndex = 0;
    do {
        if (g_BattleCurWorkBlock->activeSlotMask & mask) {
            pSlot = &g_BattleCurWorkBlock->slots[g_BattleCurSlotIndex];
            g_BattleTargetSlotEntry = pSlot;
            g_BattleCurSlotState = &pSlot->state;
            D_800C3D60 = &pSlot->state.commandCounter;
            BattleAdvanceActionRow();
            ((void (*)(void))D_800C34DC[g_BattleCurrentRow->opcode])();
            if (g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] == 0) {
                one = 1;
                flags = g_BattleCurrentRow->actionFlags;
                if (!(flags & 0x800)) {
                    one = 0;
                    if (!(flags & 0x4000)) {
                        goto skip;
                    }
                }
                BattleTryApplyCondition(one);
            }
        skip:
            g_BattleCurWorkBlock->unk5FB0 = g_BattleCurrentRow->unk02;
            BattleAdjustSlotAmountByCondition();
            BattleSetSlotBit20FromStatusBit200(g_BattleCurSlotIndex);
        }
        g_BattleCurSlotIndex++;
        mask <<= 1;
    } while (g_BattleCurSlotIndex < BATTLE_ACTOR_SLOT_COUNT);
    if (g_BattleActingSlot->characterId == 4) {
        func_8009E788();
    }
    BattleSaveScriptCommandOperands();
}
void BattleAdjustSlotAmountByCondition(void) {
    s32 percent;

    if (g_BattleTargetSlotIndex < MAX_PARTY_MEMBERS) {
        if (g_BattleCurWorkBlock->rowIndex < 3) {
            if (++g_BattleCommandCounter[0] > g_BattleCommandCounter[1]) {
                g_BattleCommandCounter[0]--;
            }
        }
        if (g_BattleCurWorkBlock->rowIndex >= 3 && g_BattleCurWorkBlock->rowIndex < 6) {
            g_BattleCommandCounter[0] = g_BattleCommandCharIndex + 0xFF;
        }
        if (g_BattleCurWorkBlock->rowIndex >= 6 && g_BattleCurWorkBlock->rowIndex < 9) {
            g_BattleCommandCounter[0] = g_BattleCommandCharIndex + 0xFE;
        }
        if (g_BattleCurWorkBlock->rowIndex >= 9 && g_BattleCurWorkBlock->rowIndex < 12) {
            g_BattleCommandCounter[0] = g_BattleCommandCharIndex + 0xFD;
        }
        if (g_BattleCurWorkBlock->rowIndex >= 3 && g_BattleCurWorkBlock->rowIndex < 12) {
            *(u8 *)&g_BattleActingSlot->unk54 += (u32)g_BattleCurWorkBlock->rowIndex / 3;
        }
        if (g_BattleCurWorkBlock->rowIndex == 5) {
            *(u8 *)&g_BattleActingSlot->unk54 += 1;
        }
        if (g_BattleCurWorkBlock->rowIndex == 8) {
            *(u8 *)&g_BattleActingSlot->unk54 += 2;
        }
        if (g_BattleCurWorkBlock->rowIndex == 0xB) {
            *(u8 *)&g_BattleActingSlot->unk54 += 3;
        }
    }
    if (g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] == 0) {
        if (g_BattleTargetSlotEntry->activeEffectFlags2 & 0x2000) {
            if (rand() % BATTLE_PERCENT_ROLL_RANGE < BATTLE_EFFECT_REMOVE_CHANCE_PERCENT) {
                g_BattleTargetSlotEntry->activeEffectFlags2 &= 0xDFFF;
                g_BattleCurSlotState->activeEffectFlags1 &= 0xEFFF;
            }
        }
    }
    if (g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] == 0xA) {
        if (g_BattleCurSlotState->accessoryEffectFlags1 & 0x80) {
            g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] = 0;
        }
    }
    if (g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] == 0) {
        if (g_BattleTargetSlotEntry->accessoryEffectFlags5 & 0x80) {
            if (g_BattleTargetSlotEntry->characterId == 0) {
                percent = BATTLE_AMOUNT_HALVE_CHANCE_PERCENT_CHARACTER_0;
            } else {
                percent = BATTLE_AMOUNT_HALVE_CHANCE_PERCENT;
            }
            if (rand() % BATTLE_PERCENT_ROLL_RANGE < percent) {
                g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] =
                    (u32)g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] >> 1;
            } else {
                g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] +=
                    (u32)g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] >> 1;
            }
        }
        if (g_BattleTargetSlotEntry->accessoryEffectFlags5 & 0x20) {
            g_BattleCurWorkBlock->slotStates[g_BattleTargetSlotIndex] = 0;
            g_BattleCurWorkBlock->slotAmounts[g_BattleTargetSlotIndex] =
                g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex];
        }
    }
    if (g_BattleTargetSlotEntry->accessoryEffectFlags4 & 0x200) {
        if (g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] == 1) {
            g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] = 0;
        }
    }
}
void BattleSaveScriptCommandOperands(void) {
    u16 a = g_BattleTargetSlotState->activeEffectFlags3;
    u16 b = g_BattleActingSlot->accessoryEffectFlags4;
    u16 c;

    g_BattleCurWorkBlock->unk5FBC = g_BattleCurrentRow->unk20;
    g_BattleCurWorkBlock->unk5FBD = g_BattleCurrentRow->unk21;
    g_BattleCurWorkBlock->element = g_BattleCurrentRow->element;
    g_BattleCurWorkBlock->unk5FBF = g_BattleCurrentRow->unk23;
    g_BattleCurWorkBlock->unk5FC0 = g_BattleCurWorkBlock->rowIndex;
    c = (a | b) >> 12;
    if ((g_BattleCurWorkBlock->element & BATTLE_ELEMENT_MASK) == 0) {
        g_BattleCurWorkBlock->element |= c;
    }
}
void BattleAdvanceActionRow(void) {
    if (g_BattleCurWorkBlock->rowIndex >= 0xC && g_BattleCurWorkBlock->rowIndex < 0xF &&
        !(g_BattleCurWorkBlock->slots[g_BattleCurSlotIndex].state.flags & 0x80)) {
        g_BattleCurrentRow += 3;
    } else if (g_BattleCurWorkBlock->rowIndex < 3 &&
               !(g_BattleCurWorkBlock->slots[g_BattleCurSlotIndex].state.flags & 0x80)) {
        g_BattleCurrentRow += 15;
    }
}
void BattleSetSlotBit20FromStatusBit200(u8 arg0) {
    BattleSlotState *pState = &g_BattleWorkBlock[arg0].state;
    BattleTableEntry *pSlot = &g_BattleWorkBlock[arg0];
    u16 v;

    if (pState->activeEffectFlags2 & 0x200) {
        v = pSlot->activeEffectFlags3 | 0x20;
    } else {
        v = pSlot->activeEffectFlags3 & 0xFFDF;
    }
    pSlot->activeEffectFlags3 = v;
}
void BattleComputeActionAmountFromSlotStates(void) {
    u16 atkValue;
    u16 defValue;
    s8 command;
    s32 rate;
    s32 amount;
    s32 mulAtk;
    s32 mulDef;
    s32 amountSource;
    u8 reduction;

    rate = g_BattleCurrentRow->power;
    command = func_8009D3A0();
    atkValue = BattleScaleValueByMultiplierByte();
    defValue = BattleApplySlotPercentToValue();
    func_80096494(&atkValue, &defValue, &command);
    if ((g_BattleTargetSlotState->activeEffectFlags2 | g_BattleTargetSlotState->accessoryEffectFlags2) & 8) {
        atkValue += atkValue / 5;
    }
    if ((g_BattleTargetSlotState->activeEffectFlags2 | g_BattleTargetSlotState->accessoryEffectFlags2) & 2) {
        atkValue += atkValue / 10;
    }
    if ((g_BattleTargetSlotState->activeEffectFlags2 | g_BattleTargetSlotState->accessoryEffectFlags2) & 4) {
        atkValue -= atkValue / 5;
    }
    if ((g_BattleTargetSlotState->activeEffectFlags2 | g_BattleTargetSlotState->accessoryEffectFlags2) & 1) {
        atkValue -= atkValue / 10;
    }
    if ((g_BattleCurSlotState->activeEffectFlags2 | g_BattleCurSlotState->accessoryEffectFlags2) & 4) {
        defValue += defValue / 5;
    }
    if ((g_BattleCurSlotState->activeEffectFlags2 | g_BattleCurSlotState->accessoryEffectFlags2) & 1) {
        defValue += defValue / 10;
    }
    if ((g_BattleCurSlotState->activeEffectFlags2 | g_BattleCurSlotState->accessoryEffectFlags2) & 8) {
        defValue -= defValue / 5;
    }
    if ((g_BattleCurSlotState->activeEffectFlags2 | g_BattleCurSlotState->accessoryEffectFlags2) & 2) {
        defValue -= defValue / 10;
    }
    if (g_BattleCurrentRow->element & 0x10) {
        if (!(g_BattleTargetSlotEntry->accessoryEffectFlags2 & 0x40)) {
            g_BattleTargetSlotEntry->activeEffectFlags2 |= 0x40;
        }
        if ((g_BattleTargetSlotState->activeEffectFlags3 | g_BattleTargetSlotState->accessoryEffectFlags3) & 0x4000) {
            g_BattleCurveTargetModes[g_BattleTargetSlotIndex] = 3;
            g_BattleCurveTargets[g_BattleTargetSlotIndex] =
                (u16)(g_BattleActingSlot->maxMp / BATTLE_MP_SCALE) * 2;
        }
        if ((g_BattleTargetSlotState->activeEffectFlags3 | g_BattleTargetSlotState->accessoryEffectFlags3) & 0x1000) {
            g_BattleCurveTargetModes[g_BattleTargetSlotIndex] = 2;
            g_BattleCurveTargets[g_BattleTargetSlotIndex] = g_BattleTargetSlotState->maxHp / 10 * 2;
        }
    }
    if (g_BattleCurrentRow->element & 0x20) {
        if (!(g_BattleTargetSlotEntry->accessoryEffectFlags2 & 0x80)) {
            g_BattleTargetSlotEntry->activeEffectFlags2 |= 0x80;
        }
    }
    if (g_BattleTargetSlotEntry->activeEffectFlags2 & 0x40) {
        defValue -= defValue >> 2;
        g_BattleTargetSlotEntry->activeEffectFlags2 &= ~0x40;
    }
    if (g_BattleActingSlot->activeEffectFlags2 & 0x80) {
        atkValue -= atkValue >> 2;
        g_BattleActingSlot->activeEffectFlags2 &= ~0x80;
    }
    if (g_BattleCurrentRow->actionFlags & 0x400) {
        rate = BATTLE_ACTION_POWER_FULL;
    }
    if (g_BattleCurrentRow->actionFlags & 0x100) {
        mulAtk = 5;
        mulDef = 4;
    } else {
        mulAtk = 4;
        mulDef = 3;
    }
    if (defValue != 0) {
        amount = mulAtk * atkValue - mulDef * defValue;
    } else {
        amount = mulAtk * atkValue;
    }
    amountSource = g_BattleCurrentRow->amountSource;
    if (amountSource >= 0) {
        if (amountSource < 2) {
            amount = rate * amount / BATTLE_ACTION_POWER_FULL;
        }
    }
    if (amount <= 0) {
        amount = 0;
    } else if (amount < 15) {
        amount += rand() % 3;
    } else {
        amount += rand() % (amount / 15 + 2);
    }
    if (g_BattleCurrentRow->statusMask != 0) {
        amount = BattleScaleValueByRowBitPercent(amount);
    }
    switch (command) {
    case 1:
        g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 0;
        break;
    case 2:
        g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 5;
        reduction = g_BattleCurSlotState->damageReduction;
        if (reduction >= BATTLE_ACTION_POWER_FULL / 2) {
            reduction = BATTLE_ACTION_POWER_FULL / 2 - 1;
        }
        if (amount != 0) {
            amount = amount * (BATTLE_ACTION_POWER_FULL / 2 - reduction) / BATTLE_ACTION_POWER_FULL;
        }
        break;
    case 3:
        amount = 0;
        g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 4;
        break;
    case 4:
        g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 2;
        break;
    }
    if (g_BattleAmountThirdFlag != 0) {
        if ((g_BattleCurrentRow->actionFlags & 0x100) && amount != 0) {
            amount /= 3;
        }
    }
    if (amount > BATTLE_ACTION_AMOUNT_MAX) {
        amount = BATTLE_ACTION_AMOUNT_MAX;
    }
    if (amount < 0) {
        amount = 0;
    }
    g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] = amount;
}
void BattleSetCurrentSlotState6(void) {
    BattleWorkBlock *p;
    u8 i;

    if (BattleTryApplyCondition(0) == 0) {
        p = g_BattleCurWorkBlock;
        i = g_BattleCurSlotIndex;
        p->slotStates[i] = 6;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009D3A0);
// best 99.61%: 362 insns / 1448 bytes, EXACTLY retail's size, 8 differing lines in
// 3 places. The twin is misc18's BattleRollHitResult (same 100/50/0x5F/0x55 arms,
// matched) - port its spellings, they are what closed 96.67 -> 99.61 this round:
//   - BOTH roll arms must be the SAME spelling, `if ((s16)(rand() % 100 - chance) >=
//     50) return N; return 1;` with the compare inline. Written that way gcc keeps
//     retail's two word-identical 22-instruction chains apart and each arm's polarity
//     is retail's own (bnez / li 1 / j / li N). The old flipped pair (`< 50` and
//     `> 49`, two s16 locals) scored 96.67 with the 0x20 arm's branch inverted, so
//     cross-jumping is NOT the wall the old note claimed - the inline form is.
//   - evadeModifier must be `(s8)(*(volatile u8 *)&g_BattleCurrentRow->evadeModifier)`
//     (the twin's comment explains it): lbu + sll 24 + sra 24. The plain `(s8)` field
//     read folds to `lb` and costs exactly 2 instructions (360 / 1440 = 98.33).
//   - `limit = limit + 0x55; limit = limit - penalty;` as TWO statements. One
//     statement reassociates to limit - (penalty - 0x55): addiu v1,s3,-85 / subu
//     v1,s0,v1 where retail has addiu v1,s0,85 / subu v1,v1,s3.
//   - `if (rollD >= limit) return 2; return 1;` is retail's `beqz / li v0,2 /
//     fallthrough li v0,1`; the `< limit` early-return spelling inverts it (2 lines).
// Residuals left, all measured dead this round:
//   - the 0x2D9E4 add: `evade += g_BattleTargetSlotState->responsiveness` emits
//     `addu a2,a2,v0`, retail `addu a2,v0,a2`. `+=`, `resp + evade`, an embedded
//     assignment `(resp = field) + evade` and the double negation
//     `evade - -(s32)field` all emit a2-first: a plain variable+field add has no
//     operand-order spelling (contrast the address levers).
//   - `srl` for the last shift of the two signed divisions (abandoning
//     `hit = resp / 2 + hit`'s own operand order too). Tools/scripts/cc1_probe.sh on
//     10 shapes: every signed `/2` in gcc 2.6.0 ends in `sra`, and every spelling that
//     makes the last shift `srl` drops the sign fixup entirely (a u16 or u8-ranged
//     dividend compiles to lbu + srl 1). Fixup + srl is unreachable from C - the two
//     retail instructions are not a `/2` spelling in this compiler.
//   - the permuter (3 min, -j 2) found only a chance -> s16 rollD retype, which
//     truncates chance: wrong code, rejected.
/*
s32 func_8009D3A0(void) {
    s32 penalty = 0;
    s16 limit = 0;
    s32 hit;
    s32 evade;
    s32 chance;
    s16 rollA;
    s16 rollB;
    s16 rollC;
    s16 rollD;
    u8 isGear;

    if ((g_BattleCurrentRow->actionFlags & 0x200) && (*(u16 *)((u8 *)g_BattleTargetSlotEntry + 0x34) & 8)) {
        return 3;
    }
    if (g_BattleCurrentRow->actionFlags & 0x1000) {
        return 3;
    }
    if ((g_BattleTargetSlotEntry->activeEffectFlags3 | g_BattleTargetSlotEntry->accessoryEffectFlags3) & 0x100) {
        return 3;
    }
    if (g_BattleTargetSlotEntry->activeEffectFlags1 & 0x2000) {
        return 1;
    }
    if (g_BattleTargetSlotEntry->activeEffectFlags2 & 0x1000) {
        return 1;
    }
    if (g_BattleCurrentRow->actionFlags & 0x8000) {
        return 1;
    }
    hit = g_BattleTargetSlotEntry->hit;
    isGear = ((u8 *)&g_GameState.unk22B6)[g_BattleTargetSlotState->extraWeaponIds[0]];
    evade = g_BattleActingSlot->evade;
    if (!isGear) {
        evade += g_BattleTargetSlotState->responsiveness;
    }
    if (g_BattleCurSlotState->responsiveness != 0) {
        hit = g_BattleCurSlotState->responsiveness / 2 + hit;
    }
    if (g_BattleActingSlot->characterId == 4) {
        if ((g_BattleCurrentRow->weaponSlotFlags & 0x80) && !isGear) {
            return 3;
        }
        if ((g_BattleCurrentRow->weaponSlotFlags & 0x20) &&
            ((u8 *)&g_GameState.unk22B6)[g_BattleTargetSlotState->extraWeaponIds[3]] == 0) {
            return 3;
        }
    }
    if ((g_BattleTargetSlotState->activeEffectFlags2 | g_BattleTargetSlotState->accessoryEffectFlags2) & 0x800) {
        evade += (s16)evade / 2;
    }
    if (g_BattleTargetSlotState->activeEffectFlags1 & 0x10) {
        penalty = 0x3C;
    }
    if (g_BattleCurrentRow->actionFlags & 0x1000) {
        return 3;
    }
    if (g_BattleCurWorkBlock->slots[g_BattleCurSlotIndex].state.flags & 0x80) {
        if ((g_BattleCurSlotState->activeEffectFlags2 | g_BattleCurSlotState->accessoryEffectFlags2) & 0x400) {
            hit += (s16)hit / 2;
        }
        if (g_BattleCurSlotState->activeEffectFlags1 & 0x10) {
            limit = 0x1E;
        }
        if (g_BattleCurSlotState->activeEffectFlags1 & 0xC00) {
            return 1;
        }
    } else {
        evade = (s16)evade / 2;
        if (g_BattleTargetSlotEntry->activeEffectFlags1 & 0x2000) {
            return 1;
        }
        if (g_BattleTargetSlotEntry->activeEffectFlags2 & 0x1000) {
            return 1;
        }
        if ((g_BattleTargetSlotEntry->activeEffectFlags3 | g_BattleTargetSlotEntry->accessoryEffectFlags3) & 0x800) {
            hit += (s16)hit / 2;
        }
    }
    if (g_BattleCurWorkBlock->slots[g_BattleCurSlotIndex].state.flags & 1) {
        if (rand() % 100 < 0x5F) {
            return 2;
        }
        return 1;
    }
    // The volatile read is what keeps cc1 from folding the sign extension into the load: the byte
    // must come out lbu + sll 24 + sra 24, as retail has it.
    chance = evade + (s8)(*(volatile u8 *)&g_BattleCurrentRow->evadeModifier) - hit;
    if ((g_BattleTargetSlotEntry->activeEffectFlags3 | g_BattleTargetSlotEntry->accessoryEffectFlags3) & 0x20) {
        if ((s16)(rand() % 100 - chance) >= 50) {
            return 3;
        }
        return 1;
    }
    if ((g_BattleTargetSlotEntry->activeEffectFlags3 | g_BattleTargetSlotEntry->accessoryEffectFlags3) & 0x40) {
        if ((s16)(rand() % 100 - chance) >= 50) {
            return 2;
        }
        return 1;
    }
    rollC = rand() % 100 - chance;
    limit = limit + 0x55;
    limit = limit - penalty;
    if (rollC >= limit) {
        return 3;
    }
    rollD = rand() % 100 - chance;
    if (rollD >= limit) {
        return 2;
    }
    return 1;
}
*/

u16 BattleScaleValueByMultiplierByte(void) {
    u16 ret = BattleComputeActionAttackValue();
    u16 f;
    u32 b;
    s32 n;
    if (g_BattleCurrentRow->actionFlags & 0x100) {
        f = g_BattleTargetSlotState->activeEffectFlags1;
        b = g_BattleTargetSlotState->amountMultiplier;
        if ((f & 0x100) || b == 0) {
            n = BATTLE_AMOUNT_MULTIPLIER_UNCHANGED;
        } else {
            n = b;
        }
        ret = n * ret / BATTLE_AMOUNT_MULTIPLIER_UNCHANGED;
        if ((g_BattleTargetSlotState->activeEffectFlags2 | g_BattleTargetSlotState->accessoryEffectFlags2) & 0x40) {
            ret = ret + ((ret & 0xFFFF) >> 1);
        }
    }
    return ret;
}
u16 BattleApplySlotPercentToValue(void) {
    u16 ret = BattleComputeSlotActionAmount();
    u16 v;
    u8 n;

    if (g_BattleCurrentRow->actionFlags & 0x100) {
        n = g_BattleCurSlotState->penaltyPercent;
        v = g_BattleCurSlotState->valueBonus;
        if (n != 0 && v != 0) {
            v = v * (BATTLE_PERCENT_FULL - n) / BATTLE_PERCENT_FULL;
        }
        if (g_BattleCurWorkBlock->slots[g_BattleCurSlotIndex].state.flags & 0x80) {
            ret = ret + v;
        }
        if ((g_BattleCurSlotState->activeEffectFlags2 | g_BattleCurSlotState->accessoryEffectFlags2) & 0x20) {
            ret = ret + (ret >> 1);
        }
    } else {
        n = g_BattleCurSlotState->penaltyPercent;
        if (n != 0) {
            ret = ret * (BATTLE_PERCENT_FULL - n) / BATTLE_PERCENT_FULL;
        }
    }
    return ret;
}
s32 BattleScaleValueByRowBitPercent(s32 arg0) {
    u16 mask = g_BattleCurrentRow->statusMask;
    u8 i;
    u8 idx; /* left unassigned when the scan finds no bit, as in retail */
    u8 v;

    for (i = 0; i < sizeof(g_BattleCurSlotState->statusPenaltyPercent); i++) {
        if (mask & (0x8000 >> i)) {
            idx = i;
            break;
        }
    }
    v = g_BattleCurSlotState->statusPenaltyPercent[idx];
    if (v != 0) {
        if (v < BATTLE_STATUS_PENALTY_PERCENT_MAX) {
            arg0 = arg0 * (BATTLE_STATUS_PENALTY_PERCENT_MAX - v) / BATTLE_STATUS_PENALTY_PERCENT_MAX;
        } else {
            arg0 = 0;
        }
    }
    return arg0;
}
s8 BattleTryApplyCondition(u8 arg0) {
    s8 chance;
    u8 kind;
    u16 flags;
    u8 amount;
    BattleTableEntry *pEntry;
    BattleTableEntry *pSlot;

    pEntry = &g_BattleCurWorkBlock->slots[g_BattleCurSlotIndex];
    pSlot = pEntry;
    if (arg0 != 0) {
        chance = (s8)g_BattleTargetSlotState->weaponData[0].chance;
        kind = g_BattleTargetSlotState->weaponData[0].conditionKind;
        flags = g_BattleTargetSlotState->weaponData[0].flags;
        amount = BATTLE_CONDITION_TIMER_FIXED_COUNT;
    } else {
        chance = (s8)g_BattleCurrentRow->chance;
        kind = g_BattleCurrentRow->conditionKind;
        flags = g_BattleCurrentRow->conditionFlags;
        amount = g_BattleCurrentRow->power;
    }
    if (!(g_BattleCurWorkBlock->slots[g_BattleCurSlotIndex].state.flags &
          BATTLE_SLOT_GEAR_STATUS_FLAG)) {
        return 0;
    }
    if (chance < rand() % BATTLE_PERCENT_ROLL_RANGE) {
        return 0;
    }
    switch (kind) {
    case 0:
        if ((flags & 0x20) && (g_BattleCurSlotState->activeEffectFlags2 & 0x8000)) {
            g_BattleCurSlotState->activeEffectFlags2 &= 0x7FFF;
            g_BattleTargetSlotEntry->activeEffectFlags3 &= 0x7FFF;
            return 1;
        }
        break;
    case 1:
        if (flags & 0xA) {
            if (g_BattleCurSlotState->activeEffectFlags2 & 5) {
                g_BattleCurSlotState->activeEffectFlags2 &= 0xFFFA;
                return 1;
            }
        } else if (flags & 5) {
            if (g_BattleCurSlotState->activeEffectFlags2 & 0xA) {
                g_BattleCurSlotState->activeEffectFlags2 &= 0xFFF5;
                return 1;
            }
        }
        break;
    }
    if (kind == 0) {
        if (flags & g_BattleCurSlotState->accessoryEffectFlags1) {
            return 0;
        }
        switch (flags) {
        case 0x400:
            pSlot->state.conditionTimers[1] = amount;
            g_BattleCurSlotState->activeEffectFlags1 &= 0xFBFF;
            pEntry->activeEffectFlags1 |= 0x2000;
            break;
        case 0x1000:
            g_BattleCurSlotState->activeEffectFlags1 &= 0xEFFF;
            pEntry->activeEffectFlags2 |= 0x2000;
            break;
        case 0x200:
            pSlot->state.conditionTimers[2] = amount;
            break;
        case 0x100:
            pSlot->state.conditionTimers[3] = amount;
            break;
        case 0x80:
            pSlot->state.conditionTimers[4] = amount;
            break;
        case 0x20:
            pSlot->state.conditionTimers[5] = amount;
            pEntry->activeEffectFlags1 |= 0x1000;
            break;
        case 0x10:
            pSlot->state.conditionTimers[6] = amount;
            break;
        }
        g_BattleCurSlotState->activeEffectFlags1 |= flags;
    }
    if (kind == 1) {
        g_BattleCurSlotState->activeEffectFlags2 |= flags;
        switch (flags) {
        case 0x1000:
            pSlot->state.conditionTimers[0xB] = amount;
            break;
        case 0x40:
            pSlot->state.conditionTimers[0xC] = amount;
            break;
        case 0x20:
            pSlot->state.conditionTimers[0xD] = amount;
            break;
        }
    }
    if (kind == 3) {
        if (flags & 0xF000) {
            if (g_BattleCurSlotState->accessoryEffectFlags3 & 0xF000) {
                return 0;
            }
            g_BattleCurSlotState->activeEffectFlags3 =
                (g_BattleCurSlotState->activeEffectFlags3 & 0xFFF) | flags;
            pSlot->state.conditionTimers[8] = amount;
        }
        if (flags & 0xF00) {
            if (g_BattleCurSlotState->accessoryEffectFlags3 & 0xF00) {
                return 0;
            }
            g_BattleCurSlotState->activeEffectFlags3 =
                (g_BattleCurSlotState->activeEffectFlags3 & 0xF0FF) | flags;
            pSlot->state.conditionTimers[9] = amount;
        }
    }
    if (kind == 0xA) {
        g_BattleCurSlotState->activeEffectFlags3 &= ~flags;
    }
    if (kind == 0xB) {
        if (!(g_BattleCurSlotState->accessoryEffectFlags1 & 0x40)) {
            g_BattleCurSlotState->penaltyPercent += flags;
            if (g_BattleCurSlotState->penaltyPercent >= BATTLE_PERCENT_ROLL_RANGE) {
                g_BattleCurSlotState->penaltyPercent = BATTLE_PENALTY_PERCENT_MAX;
            }
            kind = 0;
            flags = 0x40;
        }
    }
    if (kind == 0xC) {
        g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 0;
        g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] = g_BattleCurSlotState->hp / flags;
    }
    if (kind == 0xD) {
        g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 0;
        g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] = g_BattleCurSlotState->hp - 1;
    }
    if (kind == 0xE) {
        g_BattleCurSlotState->activeEffectFlags2 = 0;
        g_BattleCurSlotState->activeEffectFlags3 = 0;
        if (flags == 1) {
            g_BattleCurSlotState->accessoryEffectFlags2 = 0;
            g_BattleCurSlotState->accessoryEffectFlags3 = 0;
        }
    }
    if (kind == 0x10) {
        g_BattleCurSlotState->activeEffectFlags1 |= 1;
        g_BattleTargetSlotEntry->activeEffectFlags1 |= 0x80;
    }
    if (kind == 0xF) {
        g_BattleCurSlotState->activeEffectFlags1 &= 0xFFFE;
        g_BattleTargetSlotEntry->activeEffectFlags1 &= 0xFF7F;
    }
    BattleSetCommandCodeForFlagBit(kind, flags);
    return 1;
}
void BattleClearActorPercent99(void) {
    g_BattleCurSlotState->penaltyPercent = 0;
}
void BattleSetSlotState2FromActorField4F(void) {
    BattleSlotState *p = g_BattleCurSlotState;
    u32 v = p->unk4F * p->maxHp;
    u32 q = v / 10;

    g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 2;
    g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] = q;
}
void BattleSetSlotState2FromScriptField11Div20(void) {
    u32 v = g_BattleCurrentRow->power * g_BattleCurSlotState->maxHp;
    u32 q = v / BATTLE_ACTION_POWER_FULL;

    g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 2;
    g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] = q;
}
void BattleSetSlotState2FromActorField5B(void) {
    s32 v = g_BattleActingSlot->ether * g_BattleCurrentRow->power;

    g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 2;
    g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] = v;
}
void BattleSetSlotFields148And10C(void) {
    D_800C3D60[0] = 4;
    g_BattleWorkBlock[g_BattleCurSlotIndex].state.conditionTimers[7] = 3;
}
void BattleSetSlotState0AFromActorWord3ADiv20(void) {
    s32 v = g_BattleCurSlotState->maxFuel * g_BattleCurrentRow->power;
    s32 q = v / BATTLE_ACTION_POWER_FULL;

    g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 0xA;
    g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] = q;
}
void BattleSetSlotState0BFromActorWord3ADiv20(void) {
    s32 v = g_BattleCurSlotState->maxFuel * g_BattleCurrentRow->power;
    s32 q = v / BATTLE_ACTION_POWER_FULL;

    g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 0xB;
    g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] = q;
}
void BattleClearSlotConditionBits(void) {
    BattleSlotState *p = g_BattleCurSlotState;
    BattleTableEntry *q = g_BattleTargetSlotEntry;

    p->activeEffectFlags1 &= 0xF80B;
    q->conditionFlags &= 0xFFDF;
}
s32 BattleGearHasWeaponId(u8 arg0) {
    BattleGearWeaponRecord *pRec = &g_BattleCurWorkBlock->gearWeaponIds[arg0];
    u8 v = pRec->weaponId;
    u8 id = g_GameState.characters[4].gearId;

    if (g_GameState.gears[id].weaponData[0].weaponId == v ||
        g_GameState.gears[id].weaponData[1].weaponId == v) {
        return 1;
    }
    return g_GameState.gears[id].weaponData[2].weaponId == v;
}
void BattleApplyGearWeaponRecord(u8 arg0) {
    BattleGearWeaponRecord *p = &g_BattleCurWorkBlock->gearWeapons[arg0];
    u8 v = p->weaponId;
    u8 id = g_GameState.characters[4].gearId;
    u8 sel;
    u8 i;

    if (g_GameState.gears[id].weaponData[0].weaponId == v) {
        sel = 0;
    }
    if (g_GameState.gears[id].weaponData[1].weaponId == v) {
        sel = 1;
    }
    if (g_GameState.gears[id].weaponData[2].weaponId == v) {
        sel = 3;
    }
    g_GameState.gears[id].weaponData[sel].attack = p->attack;
    g_GameState.gears[id].weaponData[sel].conditionKind = p->conditionKind;
    g_GameState.gears[id].weaponData[sel].chance = p->chance;
    g_GameState.gears[id].weaponData[sel].conditionKind = p->conditionKind;
    g_GameState.gears[id].extraWeaponIds[sel] = arg0;
    g_GameState.gearPartsIds[0x6C + arg0] = p->partId;
    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        if ((g_BattleCurWorkBlock->slots + i)->characterId == 4) {
            g_BattleCurWorkBlock->slots[i].state.weaponData[sel].attack = p->attack;
            g_BattleCurWorkBlock->slots[i].state.weaponData[sel].conditionKind = p->conditionKind;
            g_BattleCurWorkBlock->slots[i].state.weaponData[sel].chance = p->chance;
            g_BattleCurWorkBlock->slots[i].state.weaponData[sel].conditionKind = p->conditionKind;
            g_BattleCurWorkBlock->slots[i].state.extraWeaponIds[sel] = arg0;
        }
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009E788);
// Best honest body 89.11% (200 bytes / 50 insns against retail's 224 / 56). Everything but ONE
// block matches: retail keeps two byte-identical copies of the "idx = extraWeaponIds[0]" arm
// (case 0 at .L8009E7C0 and case 15 at .L8009E814, both lui/lw/nop/lbu v1,4/j tail/nop), and
// this cc1 deletes one of them. The arm ORDER below is retail's own block order (case 0, the
// case 3..14 group, case 15, the case 2/17 group, the returns) and reproduces retail's layout
// exactly, minus the second copy.
// Not a spelling problem: the copy has to survive, and the merge is a whole-block RTL compare,
// so a spelling that emits the same six instructions cannot differ in RTL. Measured: volatile
// load in case 15 keeps both blocks but the un-fusable volatile QI load costs "lbu v0,4 / andi
// v1,v0,0xff" where retail has "lbu v1,4 / nop" - 224 bytes at 98.66%, measured and not kept.
// The source position of the two arms does not help either: in both orders the later copy wins
// and the earlier one is deleted.
// Not a preset problem either: gcc-2.7.2 85.41%, gcc-2.7.2-cdk 66.39%, unit flags 89.11.
// This cc1 (2.6.0) rejects -fno-cross-jumping outright ("Invalid option"), so the
// identical-block merge cannot be turned off for one function, and the unit's other matched
// functions pin it to 2.6.0. A 4-minute permuter run found only a wrong-code candidate (dead
// volatile read that drops case 15 into the 0x7 arm). Wall: leave the stub.
/*
void func_8009E788(void) {
    s32 idx;

    switch (g_BattleCurWorkBlock->rowIndex) {
    case 0:
        idx = g_BattleTargetSlotState->extraWeaponIds[0];
        break;
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
        idx = g_BattleTargetSlotState->extraWeaponIds[0];
        if (((u8 *)&g_GameState.unk22B6)[idx] != 0) {
            ((u8 *)&g_GameState.unk22B6)[idx] = ((u8 *)&g_GameState.unk22B6)[idx] + 0xFF;
        }
        idx = g_BattleTargetSlotState->extraWeaponIds[3];
        break;
    case 15:
        idx = g_BattleTargetSlotState->extraWeaponIds[0];
        break;
    case 2:
    case 17:
        idx = g_BattleTargetSlotState->extraWeaponIds[3];
        break;
    case 1:
    case 16:
    default:
        return;
    }
    if (((u8 *)&g_GameState.unk22B6)[idx] != 0) {
        ((u8 *)&g_GameState.unk22B6)[idx] = ((u8 *)&g_GameState.unk22B6)[idx] + 0xFF;
    }
}
*/
void BattleSetCommandCodeForFlagBit(u8 arg0, u16 arg1) {
    switch (arg0) {
    case 0:
        switch (arg1) {
        case 0x400:
            g_BattleCurWorkBlock->commandCode = 0x24;
            break;
        case 0x200:
            g_BattleCurWorkBlock->commandCode = 0x25;
            break;
        case 0x100:
            g_BattleCurWorkBlock->commandCode = 0x26;
            break;
        case 0x80:
            g_BattleCurWorkBlock->commandCode = 0x27;
            break;
        case 0x40:
            g_BattleCurWorkBlock->commandCode = 0x28;
            break;
        case 0x20:
            g_BattleCurWorkBlock->commandCode = 0x29;
            break;
        case 0x10:
            g_BattleCurWorkBlock->commandCode = 0x2A;
            break;
        case 4:
            g_BattleCurWorkBlock->commandCode = 0x2B;
            break;
        }
        break;
    case 1:
        switch (arg1) {
        case 0x1000:
            g_BattleCurWorkBlock->commandCode = 0x2D;
            break;
        case 0x800:
            g_BattleCurWorkBlock->commandCode = 0x2E;
            break;
        case 0x400:
            g_BattleCurWorkBlock->commandCode = 0x2F;
            break;
        case 0x40:
            g_BattleCurWorkBlock->commandCode = 0x15;
            break;
        case 0x20:
            g_BattleCurWorkBlock->commandCode = 0x16;
            break;
        case 8:
        case 2:
            g_BattleCurWorkBlock->commandCode = 0x19;
            break;
        case 4:
        case 1:
            g_BattleCurWorkBlock->commandCode = 0x1A;
            break;
        }
        break;
    case 3:
        switch (arg1) {
        case 0x8000:
            g_BattleCurWorkBlock->commandCode = 0x1B;
            break;
        case 0x4000:
            g_BattleCurWorkBlock->commandCode = 0x1C;
            break;
        case 0x2000:
            g_BattleCurWorkBlock->commandCode = 0x1D;
            break;
        case 0x1000:
            g_BattleCurWorkBlock->commandCode = 0x1E;
            break;
        case 0x400:
            g_BattleCurWorkBlock->commandCode = 0x1F;
            break;
        case 0x800:
            g_BattleCurWorkBlock->commandCode = 0x20;
            break;
        case 0x100:
            g_BattleCurWorkBlock->commandCode = 0x21;
            break;
        case 0x200:
            g_BattleCurWorkBlock->commandCode = 0x22;
            break;
        }
        break;
    }
}
