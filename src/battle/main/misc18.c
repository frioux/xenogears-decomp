#include "common.h"
#include "main/game.h"
#include "battle/actor.h"


// The timer index the flag-to-timer mapping leaves when a condition flag owns no timer.
#define BATTLE_CONDITION_TIMER_INDEX_NONE 0xF

// The percent chance BattleRollPercentAndSwapActiveActor takes to swap the acting and the target
// slot; a target whose characterId is 0 is rolled against the higher one.
#define BATTLE_SWAP_CHANCE_PERCENT 50
#define BATTLE_SWAP_CHANCE_PERCENT_CHARACTER_0 60

// The two per-actor action row tables BattleWorkBlock carries (see BattleRow): the character one
// at +0x1058 with 0x5F0 bytes per actor, the gear one at +0x2228 with 0x690. rowIndex picks the
// row in either.
#define BATTLE_CHAR_ROW_OFFSET 0x1058
#define BATTLE_CHAR_ROW_STRIDE 0x5F0
#define BATTLE_GEAR_ROW_OFFSET 0x2228
#define BATTLE_GEAR_ROW_STRIDE 0x690

extern u16 g_BattleSavedSlotUnk7A[];
extern u8 g_BattleSlotUnk98Deltas[];
extern u8 g_BattleActorCharIds[];
extern BattleWorkBlock *g_BattleCurWorkBlock;
extern BattleRow *g_BattleCurrentRow;
extern BattleTableEntry *g_BattleActingSlot;
extern u8 g_BattleActorSlotFlagTestMode;
extern BattleTableEntry *g_BattleTargetSlotEntry;
extern u8 g_BattleTargetSlotIndex;
extern u8 g_BattleCurSlotIndex;
extern u8 g_BattleAmountThirdFlag;
extern BattleSlotState *g_BattleTargetSlotState;
extern BattleSlotState *g_BattleCurSlotState;

extern u8 D_800C34AE;

extern u16 g_BattleActionParam;
extern u16 g_BattleCommandCellFlags;

// The 0x170-byte per-slot condition table. Its flag word sits at +0x8A
// (BattleTableEntry::accessoryEffectFlags4) and its timer bytes at +0x15C, which the original
// reaches through a second base of its own.
extern BattleTableEntry g_BattleWorkBlock[];
extern u8 g_BattleSlotConditions[][0x170];
extern u8 D_800CCE44[][0x170];

// Per-slot entry of the table at 0x800D220E: one word per battle slot, 0x10
// bytes apart. Only the word at +0 is read here.
typedef struct {
    /* 0x0 */ u16 unk0;
    /* 0x2 */ u8 unk2[0xE];
} BattleSlotWordEntry; // Size: 0x10

extern BattleSlotWordEntry D_800D220E[];

extern s32 rand(void);
extern s8 BattleTryApplySlotCondition(u8, u8, u16);
extern void BattleSetSlotConditionTimer(u8, u8, u16, s32);
extern void func_80098D2C(u8, u8);
extern void BattleRestoreSlotConditionFlags(void);
extern u8 BattleTestActorSlotFlagsC002ByMode(void);
extern void BattleSetCommandCodeForSlotKindFlag(u8, u16);

void BattleApplyScriptConditionTimer(void) {
    s8 ret = BattleTryApplySlotCondition(g_BattleCurrentRow->chance, g_BattleCurrentRow->conditionKind,
                           g_BattleCurrentRow->conditionFlags);

    if (g_BattleCurrentRow->actionFlags & 0x4000) {
        BattleSetSlotConditionTimer(g_BattleCurSlotIndex, g_BattleCurrentRow->conditionKind, g_BattleCurrentRow->conditionFlags, BATTLE_CONDITION_TIMER_FIXED_COUNT);
    } else {
        BattleSetSlotConditionTimer(g_BattleCurSlotIndex, g_BattleCurrentRow->conditionKind, g_BattleCurrentRow->conditionFlags,
                      g_BattleCurrentRow->power);
        if (ret != 1) {
            BattleWorkBlock *p = g_BattleCurWorkBlock;

            p->slotStates[g_BattleCurSlotIndex] = 6;
        }
    }
}
void BattleApplyActorConditionTimer(void) {
    BattleTableEntry *p = g_BattleActingSlot;

    if (BattleTryApplySlotCondition(p->chance, p->conditionKind, p->flags) == 1) {
        BattleRow *q = g_BattleCurrentRow;

        BattleSetSlotConditionTimer(g_BattleCurSlotIndex, q->conditionKind, q->conditionFlags, BATTLE_CONDITION_TIMER_FIXED_COUNT);
    }
}
void BattleRollAndClearTargetConditions(void) {
    u8 t = g_BattleCurrentRow->chance;

    if (t < rand() % BATTLE_PERCENT_ROLL_RANGE) {
        g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 6;
    } else {
        if (g_BattleCurrentRow->conditionKind & 0x80) {
            g_BattleTargetSlotEntry->activeEffectFlags1 &= 0xC000;
        }
        if (g_BattleCurrentRow->conditionKind & 0x40) {
            g_BattleTargetSlotEntry->activeEffectFlags2 = 0;
            g_BattleTargetSlotEntry->conditionFlags &= 0xFFDF;
        }
        if (g_BattleCurrentRow->conditionKind & 0x20) {
            g_BattleTargetSlotEntry->activeEffectFlags3 = 0;
        }
        if (g_BattleCurrentRow->conditionKind & 0x10) {
            g_BattleTargetSlotEntry->activeEffectFlags4 = 0;
        }
        if (g_BattleCurrentRow->conditionKind & 0x8) {
            g_BattleTargetSlotEntry->activeEffectFlags7 = 0;
        }
    }
}
void BattleRollSlotConditionAction(void) {
    u8 chance;
    u16 hp;
    u16 dmg;

    switch (g_BattleCurrentRow->chanceSource) {
    case 0:
        chance = g_BattleActingSlot->conditionChance;
        break;
    case 1:
        chance = g_BattleCurrentRow->chance;
        break;
    }

    if (chance < rand() % BATTLE_PERCENT_ROLL_RANGE) {
        g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 6;
        return;
    }

    switch (g_BattleCurrentRow->amountSource) {
    case 0:
        hp = g_BattleActingSlot->maxHp;
        break;
    case 1:
        hp = g_BattleTargetSlotEntry->maxHp;
        break;
    case 2:
        hp = g_BattleActingSlot->maxMp;
        break;
    case 3:
        hp = g_BattleTargetSlotEntry->maxMp;
        break;
    }

    dmg = hp * g_BattleCurrentRow->power / 20;
    if (g_BattleCurrentRow->amountSource == 5) {
        dmg = g_BattleTargetSlotEntry->hp - 1;
    }

    switch (g_BattleCurrentRow->amountSource) {
    case 0:
    case 1:
    case 5:
        g_BattleCurWorkBlock->slotStates[g_BattleTargetSlotIndex] = 2;
        g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 0;
        g_BattleCurWorkBlock->slotAmounts[g_BattleTargetSlotIndex] = dmg;
        g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] = dmg;
        break;
    case 2:
    case 3:
        if ((g_BattleTargetSlotEntry->activeEffectFlags4 | g_BattleTargetSlotEntry->accessoryEffectFlags4) & 0x200) {
            g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 6;
            return;
        }
        g_BattleCurWorkBlock->slotStates[g_BattleTargetSlotIndex] = 3;
        g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 1;
        g_BattleCurWorkBlock->slotAmounts[g_BattleTargetSlotIndex] = dmg;
        g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] = dmg;
        break;
    default:
        return;
    }
}
void BattleSetSlotConditionActionValue(void) {
    u8 chance;

    switch (g_BattleCurrentRow->chanceSource) {
    case 0:
        chance = g_BattleActingSlot->conditionChance;
        break;
    case 1:
        chance = g_BattleCurrentRow->chance;
        break;
    }

    if (chance < rand() % BATTLE_PERCENT_ROLL_RANGE) {
        g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 6;
        return;
    }

    g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 0;

    switch (g_BattleCurrentRow->amountSource) {
    case 0:
        if (g_BattleCurWorkBlock->slots[g_BattleCurSlotIndex].state.flags & BATTLE_SLOT_GEAR_STATUS_FLAG) {
            g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] =
                ((BattleSlotState *)g_BattleCurSlotState)->hp / g_BattleCurrentRow->power;
        } else {
            g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] =
                g_BattleTargetSlotEntry->hp / g_BattleCurrentRow->power;
        }
        break;
    case 1:
        if (g_BattleCurWorkBlock->slots[g_BattleCurSlotIndex].state.flags & BATTLE_SLOT_GEAR_STATUS_FLAG) {
            g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] = ((BattleSlotState *)g_BattleCurSlotState)->hp - 1;
        } else {
            g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] = g_BattleTargetSlotEntry->hp - 1;
        }
        break;
    case 2:
        if (g_BattleCurWorkBlock->slots[g_BattleTargetSlotIndex].state.flags & BATTLE_SLOT_GEAR_STATUS_FLAG) {
            g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] =
                g_BattleTargetSlotState->maxHp - g_BattleTargetSlotState->hp;
        } else {
            g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] =
                g_BattleActingSlot->maxHp - g_BattleActingSlot->hp;
        }
        break;
    case 3:
        g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] = g_BattleTargetSlotEntry->mp * BATTLE_MP_SCALE;
        break;
    case 4:
        g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] = 1;
        break;
    case 5:
        g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] = g_BattleTargetSlotEntry->hp;
        break;
    case 6:
        g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 0;
        if (g_BattleWorkBlock[g_BattleCurSlotIndex].state.flags & BATTLE_SLOT_GEAR_STATUS_FLAG) {
            g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] = ((BattleSlotState *)g_BattleCurSlotState)->maxHp;
            if (((BattleSlotState *)g_BattleCurSlotState)->maxHp > BATTLE_HP_MAX) {
                ((BattleSlotState *)g_BattleCurSlotState)->maxHp = BATTLE_HP_MAX;
            }
        } else {
            g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] = g_BattleTargetSlotEntry->maxHp;
            if (g_BattleTargetSlotEntry->maxHp > BATTLE_HP_MAX) {
                g_BattleTargetSlotEntry->maxHp = BATTLE_HP_MAX;
            }
        }
        break;
    case 7:
        if (g_BattleCurWorkBlock->slots[g_BattleCurSlotIndex].state.flags & BATTLE_SLOT_GEAR_STATUS_FLAG) {
            g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 6;
        } else {
            g_BattleTargetSlotEntry->activeEffectFlags1 |= 1;
            g_BattleTargetSlotEntry->activeEffectFlags2 |= 1;
        }
        break;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80096494);
/* COMPILES as written (round 111 re-check): 93.04%, 908 bytes. The parked body
   used raw u8-array spellings of g_BattleCurrentRow / g_BattleActingSlot / g_BattleTargetSlotEntry; with
   today's typed externs only the field reads below changed (a byte read of
   g_BattleTargetSlotEntry->element, because retail uses lbu there but lhu for the 0x40 test).
   Same block layout and instruction multiset (226 vs 227); every
   branch, the two duplicated (elemAtk & elem2) blocks, the element case tree,
   the /10 pair and the two /5 tails line up instruction for instruction. Left:
   register allocation (retail weak=t6, bitAtk=t5, elemDef=t2; ours weak=t5,
   bitAtk=t2, elemDef=a0), retail's base-first `addu v0,a0,v0` on the
   actor[sel].unk15A stride (both `pBase + sel * 0x170` and
   `sel * 0x170 + (s32)pBase` give index-first), and one extra
   `andi v0,t1,0xff` on the second `elemAtk & pair` (retail does the AND in
   QImode because its result only feeds a compare against 0). Notes for the next
   attempt: `weak`/`hitB` must be s32 (a u8 costs an andi at each test),
   `hitA` is u8 (its test takes andi 0xff), and the switch needs a hoisted
   `mask = elem2 << 8` local - that is what makes retail emit the single
   `sll v1,t2,0x8` before the case chain. Tried: u8 vs s32 weak/hitB, nested vs
   && for the elemAtk == 0 guard, declaration order, u16 vs u8 elemAtk.
void func_80096494(u16 *pA, u16 *pB, u8 *pFlag) {
    u8 *pBase;
    u8 bitAtk;
    u8 bitDef;
    u8 elemAtk;
    u8 elemDef;
    u8 elem2;
    s32 weak = 0;
    u8 hitA = 0;
    s32 hitB;
    u16 pair;
    u16 m;
    u16 mask;
    s8 mul;
    u8 mul2;

    pBase = g_BattleCurWorkBlock;
    bitAtk = *(u8 *)(pBase + g_BattleCurSlotIndex * 0x170 + 0x15A) >> 7;
    elemAtk = g_BattleCurrentRow->element & 0x3F;
    elemDef = *(u8 *)&g_BattleTargetSlotEntry->element & 0x3F;
    bitDef = *(u8 *)(pBase + g_BattleTargetSlotIndex * 0x170 + 0x15A) >> 7;
    if (bitDef == 0) {
        pair = g_BattleActingSlot->activeEffectFlags7 | g_BattleActingSlot->accessoryEffectFlags7;
    } else {
        pair = g_BattleTargetSlotState->activeEffectFlags3 | g_BattleTargetSlotState->accessoryEffectFlags3;
    }
    pair >>= 12;
    if (g_BattleCurrentRow->actionFlags & 0x100) {
        weak = 1;
    }
    if (elemAtk == 0) {
        if (pair != 0) {
            elemAtk = pair;
        }
    }
    if (elemAtk & pair) {
        hitA = 1;
    }
    mul = 10;
    mul2 = 10;
    if (elemAtk & elemDef) {
        if (g_BattleTargetSlotEntry->element & 0x40) {
            mul = 0x12;
        } else {
            mul = 0xF;
        }
        if (hitA) {
            mul += 2;
        }
    }
    if (bitAtk == 0) {
        m = g_BattleTargetSlotEntry->activeEffectFlags7 | g_BattleTargetSlotEntry->accessoryEffectFlags7;
        elem2 = (m & 0xF00) >> 8;
        hitB = (m >> 1) & 1;
    } else {
        hitB = 0;
        m = ((BattleSlotState *)g_BattleCurSlotState)->activeEffectFlags3 |
            ((BattleSlotState *)g_BattleCurSlotState)->accessoryEffectFlags3;
        elem2 = (m & 0xF00) >> 8;
        if (m & 2) {
            hitB = 1;
        }
    }
    if (elemAtk & elem2) {
        if (weak) {
            mul -= 3;
            if (m & 4) {
                mul -= 3;
            }
            if (m & 8) {
                *pFlag = 4;
            }
        }
    }
    if (elemAtk & elem2) {
        if (hitB) {
            mul -= 3;
            if (m & 4) {
                mul -= 3;
            }
            if (m & 8) {
                *pFlag = 4;
            }
        }
    }
    mask = elem2 << 8;
    switch (elemAtk) {
    case 1:
        if (mask & 0x200) {
            mul += 3;
        }
        break;
    case 2:
        if (mask & 0x100) {
            mul += 3;
        }
        break;
    case 4:
        if (mask & 0x800) {
            mul += 3;
        }
        break;
    case 8:
        if (mask & 0x400) {
            mul += 3;
        }
        break;
    }
    if (mul < 1) {
        mul = 1;
    }
    if (mul2 == 0) {
        mul2 = 1;
    }
    *pA = *pA * mul / 10;
    *pB = mul2 * *pB / 10;
    if (((u16 *)g_BattleActingSlot->equipmentPercents)[0] & 0x10) {
        *pA += *pA / 5;
    }
    if (((u16 *)g_BattleTargetSlotEntry->equipmentPercents)[0] & 0x10) {
        *pB += *pB / 5;
    }
}
*/
void BattleRollPercentAndSetState38(void) {
    s32 t = g_BattleActingSlot->ether + g_BattleCurrentRow->chanceBonus;

    if (rand() % BATTLE_PERCENT_ROLL_RANGE >= t) {
        g_BattleCurWorkBlock->commandCode = 0x38;
        g_BattleAmountThirdFlag = 1;
    }
}
void BattleRollPercentAndSwapActiveActor(void) {
    BattleTableEntry *pTarget;
    u8 *pBase;
    BattleTableEntry *pActor;
    BattleTableEntry *pOld;
    u16 t;
    u8 sel;
    u8 s;
    u8 tmp;
    s32 lim;

    pTarget = g_BattleTargetSlotEntry;
    if (pTarget->activeEffectFlags1 & 0xA000) {
        return;
    }
    t = pTarget->activeEffectFlags2;
    if (t & 0x1000) {
        return;
    }
    sel = g_BattleTargetSlotIndex;
    if (sel < MAX_PARTY_MEMBERS && !(t & 0x2000)) {
        return;
    }
    if (!(pTarget->activeEffectFlags4 & 0x1000)) {
        return;
    }
    t = g_BattleCurrentRow->actionFlags;
    if (t & 0x2000) {
        return;
    }
    if (t & 0x100) {
        return;
    }
    pBase = (u8 *)g_BattleCurWorkBlock->slots;
    pActor = (BattleTableEntry *)(pBase + sel * sizeof(*pActor));
    if (pActor->state.flags & BATTLE_SLOT_GEAR_STATUS_FLAG) {
        return;
    }
    lim = BATTLE_SWAP_CHANCE_PERCENT;
    if (pTarget->characterId == 0) {
        lim = BATTLE_SWAP_CHANCE_PERCENT_CHARACTER_0;
    }
    if (lim < rand() % BATTLE_PERCENT_ROLL_RANGE) {
        return;
    }
    if (g_BattleCurrentRow->opcode == 2) {
        return;
    }
    tmp = g_BattleCurSlotIndex;
    s = g_BattleTargetSlotIndex;
    pOld = g_BattleActingSlot;
    g_BattleTargetSlotIndex = tmp;
    g_BattleActingSlot = g_BattleTargetSlotEntry;
    g_BattleTargetSlotEntry = pOld;
    g_BattleCurSlotIndex = s;
    // row table at +0x1378, 0x5F0 bytes per slot (inside BattleWorkBlock's
    // unk450, so it is reached by offset)
    g_BattleCurrentRow = (BattleRow *)((u8 *)g_BattleCurWorkBlock + g_BattleTargetSlotIndex * 0x5F0 + 0x1378);
    g_BattleCurWorkBlock->slotStates[g_BattleTargetSlotIndex] = 7;
    g_BattleCurWorkBlock->slotAmounts[g_BattleTargetSlotIndex] = 0;
    g_BattleCurWorkBlock->commandCode = 0x34;
}
/* The evade modifier's read must stay volatile: written plainly cc1 folds the sign extension into
   the load instead of retail's lbu then sll 24/sra 24. The two threshold rolls each need their own
   roll and limit locals, and the first one also stores its roll into `flags` (dead by then) - the
   redefinition is what keeps cc1 from hoisting the limit's addiu into the divider's post-mult
   slots, and a fifth local instead of the reuse costs the 1288-byte frame. */
s32 BattleRollHitResult(void) {
    s32 evade;
    s32 hit;
    s32 low;
    s32 high;
    u16 flags;
    s32 diff;
    s32 roll;
    s32 limit2;
    s32 roll2;
    s32 limit;

    evade = g_BattleActingSlot->evade;
    hit = g_BattleTargetSlotEntry->hit;
    low = 0;
    high = 0;

    if ((g_BattleCurrentRow->actionFlags & 0x40) != 0 &&
        (g_BattleSlotConditions[g_BattleCurSlotIndex][0] & 0x80) != 0) {
        return 3;
    }

    if (g_BattleActingSlot->characterId == 4) {
        if ((g_BattleCurrentRow->unk0C[4] & 0x80) != 0 &&
            g_GameState.gearPartsIds[0x6C + g_BattleActingSlot->extraWeaponIds[0]] == 0) {
            D_800C34AE = 1;
            return 3;
        }
        if ((g_BattleCurrentRow->unk0C[4] & 0x10) != 0 &&
            g_GameState.gearPartsIds[0x6C + g_BattleActingSlot->extraWeaponIds[3]] == 0) {
            D_800C34AE = 1;
            return 3;
        }
    }

    if ((g_BattleCurrentRow->actionFlags & 0x200) != 0) {
        if ((g_BattleTargetSlotEntry->blockFlags & 0x8) != 0) {
            return 3;
        }
        if ((g_BattleSlotConditions[g_BattleCurSlotIndex][0] & 0x80) != 0) {
            return 3;
        }
    }

    if ((g_BattleCurrentRow->actionFlags & 0x1000) != 0) {
        return 3;
    }
    flags = g_BattleTargetSlotEntry->activeEffectFlags3 | g_BattleTargetSlotEntry->accessoryEffectFlags3;
    if ((flags & 0x100) != 0) {
        return 3;
    }
    if ((g_BattleTargetSlotEntry->activeEffectFlags1 & 0x2000) != 0) {
        return 1;
    }
    if ((g_BattleTargetSlotEntry->activeEffectFlags2 & 0x1000) != 0) {
        return 1;
    }
    if ((g_BattleCurrentRow->actionFlags & 0x8000) != 0) {
        return 1;
    }
    if ((g_BattleCurrentRow->actionFlags & 0x2) != 0) {
        return 5;
    }

    if ((g_BattleActingSlot->activeEffectFlags1 & 0x400) != 0) {
        low -= 0x32;
    }
    if ((g_BattleActingSlot->activeEffectFlags3 | g_BattleActingSlot->accessoryEffectFlags3) & 0x1000) {
        low += 0x1E;
    }
    if ((flags & 0x800) != 0) {
        high += 0x32;
    }

    // A volatile read is what keeps cc1 from folding the sign extension into the load: the
    // byte must come out lbu + sll 24 + sra 24, as retail has it.
    diff = evade + (s8)(*(volatile u8 *)&g_BattleCurrentRow->evadeModifier) - hit;
    if ((g_BattleCurWorkBlock->slots[g_BattleCurSlotIndex].state.flags & 1) != 0) {
        if (rand() % BATTLE_PERCENT_ROLL_RANGE < 0x5F) {
            return 2;
        }
        return 1;
    }
    if ((flags & 0x20) != 0) {
        if ((s16)(rand() % BATTLE_PERCENT_ROLL_RANGE - diff) >= 0x32) {
            return 3;
        }
        return 1;
    }
    if ((g_BattleTargetSlotEntry->activeEffectFlags3 | g_BattleTargetSlotEntry->accessoryEffectFlags3) & 0x40) {
        if ((s16)(rand() % BATTLE_PERCENT_ROLL_RANGE - diff) >= 0x32) {
            return 2;
        }
        return 1;
    }
    roll = (flags = rand() % BATTLE_PERCENT_ROLL_RANGE - diff);
    limit = high - 0x5A;
    if ((s16)roll >= (s16)(low - limit)) {
        return 3;
    }
    roll2 = rand() % BATTLE_PERCENT_ROLL_RANGE - diff;
    limit2 = high - 0x55;
    if ((s16)roll2 >= (s16)(low - limit2)) {
        return 2;
    }
    return 1;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80096FBC);
s16 BattleComputeSlotActionAmount(void) {
    u16 value;
    u16 amount;
    u16 stat;
    u8 i;
    u8 count;
    u16 result;

    if (g_BattleCurWorkBlock->slots[g_BattleCurSlotIndex].state.flags & BATTLE_SLOT_GEAR_STATUS_FLAG) {
        value = g_BattleCurSlotState->baseDefense;
    } else {
        amount = g_BattleTargetSlotEntry->defense;
        value = g_BattleTargetSlotEntry->defenseFromAccessories;
    }

    stat = g_BattleTargetSlotEntry->etherDefense;
    if (g_BattleCurrentRow->actionFlags & 0x100) {
        if ((g_BattleTargetSlotEntry->activeEffectFlags4 | g_BattleTargetSlotEntry->accessoryEffectFlags4) & 0x4000) {
            stat = stat + (u32)stat * 2;
            stat = stat >> 1;
        }
    } else {
        if ((g_BattleCurWorkBlock->slots[g_BattleCurSlotIndex].state.flags & BATTLE_SLOT_GEAR_STATUS_FLAG) == 0 &&
            ((g_BattleTargetSlotEntry->activeEffectFlags3 | g_BattleTargetSlotEntry->accessoryEffectFlags3) & 0x100)) {
            amount = (u32)amount * 3 >> 1;
        }
    }
    if (g_BattleCurWorkBlock->slots[g_BattleCurSlotIndex].state.flags & BATTLE_SLOT_GEAR_STATUS_FLAG) {
        amount = 0;
    }

    switch (g_BattleCurrentRow->pad1B) {
    case 0:
        result = value + amount;
        break;
    case 1:
        result = stat;
        break;
    case 2:
        result = value;
        break;
    }

    if (g_BattleCurWorkBlock->slots[g_BattleCurSlotIndex].state.flags & BATTLE_SLOT_GEAR_STATUS_FLAG) {
        return (s16)result;
    }

    count = 0;
    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        if (g_BattleCurWorkBlock->slots[i].activeEffectFlags1 & 0x8000) {
            count++;
        }
    }

    if (g_BattleTargetSlotEntry->accessoryEffectFlags5 & 0x4) {
        if (count != 0 && (g_BattleCurrentRow->actionFlags & 0x100) == 0) {
            result = (u16)result * (4 - count) / 4;
        }
    }
    if (g_BattleTargetSlotEntry->accessoryEffectFlags5 & 0x2) {
        if (count != 0 && (g_BattleCurrentRow->actionFlags & 0x100) == 0) {
            result = (u16)result * (count + 2) / 2;
        }
    }
    if (g_BattleTargetSlotEntry->accessoryEffectFlags5 & 0x1) {
        if (count != 0) {
            result = (u16)result * (count + 2) / 2;
        }
    }

    return (s16)result;
}
void BattleNop(void) {
}
s8 BattleTryApplySlotCondition(u8 chance, u8 kind, u16 flags) {
    if (g_BattleCurWorkBlock->slots[g_BattleCurSlotIndex].state.flags & BATTLE_SLOT_GEAR_STATUS_FLAG) {
        return 0;
    }

    if (chance < rand() % BATTLE_PERCENT_ROLL_RANGE) {
        return 0;
    }

    switch (kind) {
    case 0: {
        u16 mask = flags & 0xFFFD;

        if (g_BattleTargetSlotEntry->accessoryEffectFlags1 & mask) {
            return 0;
        }
        if (flags & 0x1000) {
            g_BattleTargetSlotEntry->activeEffectFlags3 &= 0x7FFF;
        }
        break;
    }
    case 2:
        if (g_BattleTargetSlotEntry->accessoryEffectFlags2 & flags) {
            return 0;
        }
        break;
    case 5:
        if ((g_BattleTargetSlotEntry->activeEffectFlags1 | g_BattleTargetSlotEntry->accessoryEffectFlags1) & 1) {
            return 0;
        }
        if (flags & 0x8000) {
            g_BattleTargetSlotEntry->activeEffectFlags1 &= 0xEFFF;
        }
        break;
    case 7:
        if ((g_BattleTargetSlotEntry->activeEffectFlags2 | g_BattleTargetSlotEntry->accessoryEffectFlags2) & 1) {
            return 0;
        }
        if (flags & 0xA) {
            if (g_BattleTargetSlotEntry->activeEffectFlags4 & 0x5) {
                g_BattleTargetSlotEntry->activeEffectFlags4 &= 0xFFFA;
                return 1;
            }
            break;
        }
        if (flags & 0x5) {
            if (g_BattleTargetSlotEntry->activeEffectFlags4 & 0xA) {
                g_BattleTargetSlotEntry->activeEffectFlags4 &= 0xFFF5;
                return 1;
            }
            break;
        }
        // falls into case 9 when the flags request neither of the 0x8A pair
    case 9:
        if (flags & 0xF000) {
            if (g_BattleTargetSlotEntry->accessoryEffectFlags7 & 0xF000) {
                g_BattleCurWorkBlock->commandCode = 0x39;
                return 0;
            }
            g_BattleTargetSlotEntry->activeEffectFlags7 &= 0xFFF;
        }
        if (flags & 0xF00) {
            if (g_BattleTargetSlotEntry->accessoryEffectFlags7 & 0xF00) {
                g_BattleCurWorkBlock->commandCode = 0x39;
                return 0;
            }
            g_BattleTargetSlotEntry->activeEffectFlags7 &= 0xF0FF;
        }
        break;
    }

    switch (kind) {
    case 0:
        g_BattleTargetSlotEntry->activeEffectFlags1 = (flags | g_BattleTargetSlotEntry->activeEffectFlags1) & 0xFFFD;
        if (flags & 0x2) {
            if (BattleTestActorSlotFlagsC002ByMode()) {
                g_BattleTargetSlotEntry->activeEffectFlags1 |= flags;
                g_BattleCurWorkBlock->commandCode = 0x31;
                g_BattleTargetSlotEntry->conditionFlags = 0xFFEF;
            } else {
                g_BattleCurWorkBlock->commandCode = 0x32;
            }
        }
        break;
    case 2:
        g_BattleTargetSlotEntry->activeEffectFlags2 |= flags;
        if (flags & 0x800) {
            g_BattleTargetSlotEntry->conditionFlags |= 0x20;
        }
        break;
    case 5:
    case 7:
    case 9: {
        // The per-kind condition words are the u16 run at conditionFlags, so
        // conditionFlags[kind] is activeEffectFlags3 for kind 5 and activeEffectFlags4 for kind 7.
        // The two halves of the sum
        // must be spelled base-first, or gcc puts the address in the index
        // register and the shared store below cannot be cross-jumped.
        mem_addr base;
        u16 *pFlags;

        base = (mem_addr)g_BattleTargetSlotEntry;
        pFlags = (u16 *)(base + (kind * 2));
        pFlags[0x3D] |= flags;
        break;
    }
    }

    BattleSetCommandCodeForSlotKindFlag(kind, flags);
    return 1;
}
void BattleClearAllSlotStates(void) {
    s16 i;

    for (i = BATTLE_SLOT_COUNT - 1; i != -1; i--) {
        g_BattleCurWorkBlock->slotStates[i] = BATTLE_SLOT_STATE_CLEARED;
        g_BattleCurWorkBlock->slotAmounts[i] = 0;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80097D5C);
/* 98.55%, 3020 bytes against retail's 3032 (round 232). Four permuter runs from the 95.18%
   park (tools/scripts/permute.sh ... --minutes 10 -j 2), hand-ported, closed 124 differing
   instructions down to 74. Every one of them is a statement position or an own-pointer local:
   - the g_BattleTargetSlotState->weight += ... statement belongs BEFORE the unk9A[2] += one, not after it;
   - the attack -> engineOutput copy in the characterId == 7 arm wants its own pointer local
     (pStats = &g_BattleActingSlot->unk57) taken before the three statements above it;
   - the blockFlags = 0 store wants its own pointer local (pSlot = g_BattleActingSlot;) taken
     after the maxHp store: that is what makes it store through the PRE-reload register, i.e.
     retail's 'sh zero,52(a1)' one += group earlier than a plain g_BattleActingSlot-> spelling
     puts it;
   - that pSlot position also wants an 'effectFlags = 0;' between it and the maxMp store, and
     the unk68 += must then be indexed [effectFlags] and not [0] - the zero has to be sitting
     in a register, which is retail's shape. That is the one ROUGH site left in the body.
   Remaining 12 bytes / 3 instructions: retail reloads g_BattleCurWorkBlock and re-computes
   i * 8 in EACH arm of the characterId == 4 test and cross-jumps only the store tail, while
   gcc hoists our single sll above the bne and fills the delay slot with it (the deref spelling
   does not change that). The rest is register naming - retail keeps the acting slot in a1 and
   the work block in a0 - and retail materialising 0xCCCCCCCD inline before each /10 where ours
   hoists it into the prologue.
   Source facts, do not re-derive: the four growth statements divide by 20 (retail 0x66666667
   >> 3), not 10; the 0x2814 field under the 0x2228 row table is stored, then /= 10, then *= 10
   (retail's inline 0xCCCCCCCD), and that store written as one statement with no value local
   scores 97.52% - the best form found, the 'value = store = expr' fusion is worse; the two row
   tables are 0x26 and 0x2A rows of 0x28 bytes at +0x1058 and +0x2228 with the flag byte at row
   +0x13 and must be indexed FLAT or gcc re-forms the base; the gates are
   accessoryEffectFlags4 & 0x2000 (double every row) and accessoryEffectFlags5 & 0x4000
   (halve); the g_GameState tail (0x1930 u16, 0x89A, 0x834, and g_BattleOptionFlags + 9 ==
   g_GameState + 0x2355) must be reached through g_GameState itself, one base and four
   displacements. The last permuter run fell 2078 -> 1343 and then stalled.
void func_80097D5C(void) {
    BattleTableEntry *pSlot;
    u8 i;
    u8 j;
    u8 *pFlags;
    u16 effectFlags;
    u8 *pSlotBytes;
    u8 *pStats;

    for (i = 0; i < 3; i++) {
        if (g_BattleActorCharIds[i] == BATTLE_SLOT_CHARACTER_NONE) {
            continue;
        }
        g_BattleActingSlot = &g_BattleCurWorkBlock->slots[i];
        g_BattleTargetSlotState = &g_BattleActingSlot->state;
        pFlags = &g_BattleActingSlot->state.unkA4;
        if (g_BattleActorCharIds[i] == 9) {
            g_GameState.characters[9].characterId = 9;
            g_BattleActingSlot->characterId = 9;
        }
        if (g_BattleActorCharIds[i] == 10) {
            g_GameState.characters[10].characterId = 10;
            g_BattleActingSlot->characterId = 10;
        }
        if (g_BattleActingSlot->characterId == 4) {
            ((u8 *)g_BattleCurWorkBlock + i * 8)[0x1028] = g_BattleActingSlot->unk04[0] + g_BattleActingSlot->unk04[0x18];
        } else {
            ((u8 *)g_BattleCurWorkBlock + i * 8)[0x1028] = g_BattleActingSlot->attack + g_BattleActingSlot->unk04[0];
        }
        ((u8 *)g_BattleCurWorkBlock + i * 8)[0x1029] = g_BattleActingSlot->evade;
        ((u8 *)g_BattleCurWorkBlock + i * 8)[0x102A] = g_BattleActingSlot->defense + g_BattleActingSlot->defenseFromAccessories;
        ((u8 *)g_BattleCurWorkBlock + i * 8)[0x102B] = g_BattleActingSlot->hit;
        ((u8 *)g_BattleCurWorkBlock + i * 8)[0x102C] = g_BattleActingSlot->ether;
        ((u8 *)g_BattleCurWorkBlock + i * 8)[0x102D] = g_BattleActingSlot->etherDefense;
        ((u8 *)g_BattleCurWorkBlock + i * 8)[0x102E] = g_BattleActingSlot->agility;
        ((u32 *)((u8 *)g_BattleCurWorkBlock + i * 8))[0x3F4] = ((u32 *)&g_BattleActingSlot->unk3A[2])[0];
        ((u32 *)((u8 *)g_BattleCurWorkBlock + i * 8))[0x3F5] = ((u32 *)&g_BattleActingSlot->unk3A[6])[0];
        ((u16 *)((u8 *)g_BattleCurWorkBlock + i * 4))[0x800] = g_BattleActingSlot->maxHp;
        pSlot = g_BattleActingSlot;
        effectFlags = 0;
        ((u16 *)((u8 *)g_BattleCurWorkBlock + i * 4))[0x801] = g_BattleActingSlot->maxMp;
        g_BattleActingSlot->attack += g_BattleActingSlot->attackFromEquipment;
        pSlot->blockFlags = 0;
        g_BattleActingSlot->defense += g_BattleActingSlot->defenseFromEquipment;
        g_BattleActingSlot->agility += g_BattleActingSlot->agilityFromEquipment;
        g_BattleActingSlot->ether += g_BattleActingSlot->etherFromEquipment;
        g_BattleActingSlot->etherDefense += g_BattleActingSlot->etherDefenseFromEquipment;
        g_BattleActingSlot->evade += g_BattleActingSlot->equipmentPercents[0];
        g_BattleActingSlot->hit += g_BattleActingSlot->equipmentPercents[1];
        if (g_BattleActingSlot->agility >= 0x11) {
            g_BattleActingSlot->agility = 0x10;
        }
        if (g_BattleActingSlot->accessoryEffectFlags5 & 0x100) {
            g_BattleActingSlot->evade += g_BattleActingSlot->evade >> 2;
            g_BattleActingSlot->hit += g_BattleActingSlot->hit >> 2;
        }
        g_BattleActingSlot->hp = g_BattleActingSlot->hp + g_BattleActingSlot->maxHp * g_BattleActingSlot->accessoryEffectFlags8 / 20;
        g_BattleActingSlot->mp = g_BattleActingSlot->mp + g_BattleActingSlot->maxMp * g_BattleActingSlot->unk31 / 20;
        g_BattleActingSlot->maxHp = g_BattleActingSlot->maxHp + g_BattleActingSlot->maxHp * g_BattleActingSlot->accessoryEffectFlags8 / 20;
        g_BattleActingSlot->maxMp = g_BattleActingSlot->maxMp + g_BattleActingSlot->maxMp * g_BattleActingSlot->unk31 / 20;
        if (g_BattleActingSlot->hp >= 1000) {
            g_BattleActingSlot->hp = 999;
        }
        if (g_BattleActingSlot->mp >= 100) {
            g_BattleActingSlot->mp = 99;
        }
        if (g_BattleActingSlot->maxHp >= 1000) {
            g_BattleActingSlot->maxHp = 999;
        }
        if (g_BattleActingSlot->maxMp >= 100) {
            g_BattleActingSlot->maxMp = 99;
        }
        pSlotBytes = (u8 *)&g_BattleCurWorkBlock->slots[i];
        pSlotBytes[0x158] = 5;
        pSlotBytes[0x159] = 5;
        pFlags[0] = 0;
        g_BattleTargetSlotState->baseDefense += g_BattleTargetSlotState->defenseFromEquipment;
        g_BattleTargetSlotState->valueBonus += g_BattleTargetSlotState->etherDefenseFromEquipment;
        g_BattleTargetSlotState->weight += g_BattleTargetSlotState->weightFromEquipment
                                          + g_BattleTargetSlotState->unk46;
        g_BattleTargetSlotState->unk9A[2] += g_BattleTargetSlotState->unk4C;
        g_BattleTargetSlotState->responsiveness += g_BattleTargetSlotState->unk4D;
        g_BattleTargetSlotState->agility += g_BattleTargetSlotState->unk4E - g_BattleTargetSlotState->unk48;
        g_BattleTargetSlotState->unk9E += g_BattleTargetSlotState->unk50[4];
        if (g_BattleTargetSlotState->unk4F != 0 && g_BattleActingSlot->gearId != 0x12) {
            ((u16 *)((u8 *)g_BattleCurWorkBlock + i * 0x690))[0x140A] =
                g_BattleTargetSlotState->maxHp / 10 * g_BattleTargetSlotState->unk4F * 2 / 9;
            ((u16 *)((u8 *)g_BattleCurWorkBlock + i * 0x690))[0x140A] /= 10;
            ((u16 *)((u8 *)g_BattleCurWorkBlock + i * 0x690))[0x140A] *= 10;
        }
        pFlags[1] = 0;
        if (g_GameState.unk16BC[g_BattleActingSlot->characterId].unk8 & 0x1C00) {
            pFlags[1] = 1;
        }
        if (g_GameState.unk16BC[g_BattleActingSlot->characterId].unk8 & 0x380) {
            pFlags[1] = 2;
        }
        if (g_GameState.unk16BC[g_BattleActingSlot->characterId].unk8 & 0x70) {
            pFlags[1] = 3;
        }
        g_BattleTargetSlotState->unk3F = g_BattleTargetSlotState->attackPower;
        g_BattleTargetSlotState->unk3E = g_BattleTargetSlotState->attackPower;
        g_BattleTargetSlotState->unk3F += g_BattleTargetSlotState->unk50[6];
        g_BattleTargetSlotState->unk3E += g_BattleTargetSlotState->unk50[6];
        if (g_BattleActingSlot->accessoryEffectFlags4 & BATTLE_GEAR_EFFECT_FLAG) {
            for (j = 0; j < 0x26; j++) {
                ((u8 *)g_BattleCurWorkBlock)[i * 0x5F0 + j * 0x28 + 0x106B] <<= 1;
            }
            for (j = 0; j < 0x2A; j++) {
                ((u8 *)g_BattleCurWorkBlock)[i * 0x690 + j * 0x28 + 0x223B] <<= 1;
            }
        }
        if (g_BattleActingSlot->accessoryEffectFlags5 & 0x4000) {
            for (j = 0; j < 0x26; j++) {
                ((u8 *)g_BattleCurWorkBlock)[i * 0x5F0 + j * 0x28 + 0x106B] =
                    (((u8 *)g_BattleCurWorkBlock)[i * 0x5F0 + j * 0x28 + 0x106B] + 1) >> 1;
            }
            for (j = 0; j < 0x2A; j++) {
                ((u8 *)g_BattleCurWorkBlock)[i * 0x690 + j * 0x28 + 0x223B] =
                    (((u8 *)g_BattleCurWorkBlock)[i * 0x690 + j * 0x28 + 0x223B] + 1) >> 1;
            }
        }
        if (g_BattleActingSlot->unk61[1] >= 50) {
            g_GameState.unk16BC[g_BattleActingSlot->characterId].unk8 |= 8;
        }
        if (g_BattleActingSlot->characterId == 7) {
            g_BattleTargetSlotState->hp = g_BattleActingSlot->hp * BATTLE_ACTOR_HP_SCALE;
            g_BattleTargetSlotState->maxHp = g_BattleActingSlot->maxHp * BATTLE_ACTOR_HP_SCALE;
            pStats = &g_BattleActingSlot->unk57;
            g_BattleTargetSlotState->engineOutput = pStats[1];
            g_BattleTargetSlotState->baseDefense = g_BattleActingSlot->defense * 12;
            g_BattleTargetSlotState->valueBonus = g_BattleActingSlot->etherDefense * 6;
            g_BattleTargetSlotState->agility = g_BattleActingSlot->agility;
            effectFlags = g_BattleTargetSlotState->accessoryEffectFlags1 | 0x3C4;
            g_BattleTargetSlotState->accessoryEffectFlags1 = effectFlags;
            if (g_BattleActingSlot->accessoryEffectFlags2 & BATTLE_GEAR_EFFECT_FLAG) {
                g_BattleTargetSlotState->accessoryEffectFlags1 = effectFlags | 0x1000;
            }
        }
        if (g_BattleActingSlot->gearId == 0xF) {
            g_GameState.unk16BC[g_BattleActingSlot->characterId].statusFlags &= 0x8FFF;
        }
        if (g_BattleActingSlot->gearId == 0x12) {
            g_BattleActingSlot->conditionFlags = 0x238;
            ((u16 *)&g_BattleTargetSlotState->unk28[0x10])[0] = 0x26AC;
            g_BattleTargetSlotState->unk3A = 0x26AC;
            pFlags[1] = 0;
            g_GameState.unk16BC[3].statusFlags = 0x8000;
        }
        if (g_GameState.fieldScriptMemory[0] >= 0xE7) {
            if ((((u8 *)&g_GameState)[0x2355] & 0x80) == 0) {
                ((u8 *)&g_GameState)[0x89A] = 0x27;
                ((u8 *)&g_GameState)[0x834] = 0x1E;
                ((u8 *)&g_GameState)[0x2355] |= 0x80;
            }
        }
    }
    g_BattleActorSlotFlagTestMode = 3;
    for (i = 0; i < 3; i++) {
        if (g_BattleActorCharIds[i] == BATTLE_SLOT_CHARACTER_NONE) {
            g_BattleActorSlotFlagTestMode--;
        }
    }
    for (i = 0; i < 3; i++) {
        g_BattleCurWorkBlock->characterAmounts[i] = 0;
        g_BattleCurWorkBlock->gearAmounts[i] = 0;
    }
}
*/
void BattleResetSlotConditionCounters(void) {
    u8 i;
    u8 j;
    u8 v;
    BattleTableEntry *pSlot;
    u16 hp;
    u16 flag;

    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        if (g_BattleActorCharIds[i] != BATTLE_SLOT_CHARACTER_NONE) {
            pSlot = &g_BattleCurWorkBlock->slots[i];
            g_BattleActingSlot = pSlot;
            hp = pSlot->conditionFlags;
            g_BattleTargetSlotState = &pSlot->state;
            g_BattleSavedSlotUnk7A[i] = hp;
            for (j = 0; j < 4; j++) {
                v = g_BattleSlotUnk98Deltas[j];
                if (v != 0) {
                    g_BattleTargetSlotState->agility -= v;
                    g_BattleTargetSlotState->agility += g_BattleTargetSlotState->unk50[j];
                }
            }
            if (g_BattleTargetSlotState->agility >= 0x11) {
                g_BattleTargetSlotState->agility = 0x10;
            }
        }
    }

    flag = (u16)g_GameState.unk16BC[8].statusFlags;
    g_GameState.characters[8].agility = 7;
    if (flag & 0x2000) {
        g_GameState.unk16BC[8].characterConditionMask |= 0x800;
    }
    if (g_GameState.fieldScriptMemory[0] < 0xBB) {
        g_GameState.gears[0].attackPower = 10;
        g_GameState.gears[1].attackPower = 10;
        g_GameState.gears[11].attackPower = 9;
        g_GameState.gears[12].attackPower = 9;
        g_GameState.gears[13].attackPower = 8;
        g_GameState.gears[14].attackPower = 12;
        g_GameState.gears[15].attackPower = 12;
        g_GameState.gears[13].frameId = 0x58;
        g_GameState.gears[7].engineId = 0;
        g_GameState.gears[15].armorId = 0x28;
    }
}
// Points the acting slot, its state and the current action row at `sel` and returns the
// countdown its caller stores: a base minus nine per row the slot still has to reach, jittered.
// The row comes from the character table or, when the slot's status flags say gear, the gear
// table. Each arm needs its OWN row-pointer local - with one shared local cc1 gives the second
// arm's last add a fresh register (99.68%) instead of accumulating into the pointer as retail does.
u8 BattleSelectActionRow(u8 sel) {
    BattleWorkBlock *pBase = g_BattleCurWorkBlock;
    BattleTableEntry *pActor = (BattleTableEntry *)(sel * sizeof(BattleTableEntry) + (s32)pBase);
    BattleRow *pRow;
    BattleRow *pGearRow;
    s32 rowOffset;
    s32 other;
    s32 t;
    s32 v;
    s32 d;

    g_BattleActingSlot = pActor;
    g_BattleTargetSlotState = &pActor->state;
    rowOffset = BATTLE_CHAR_ROW_OFFSET;

    if (sel < MAX_PARTY_MEMBERS) {
        if ((pActor->state.flags & BATTLE_SLOT_GEAR_STATUS_FLAG) == 0) {
            pRow = (BattleRow *)((u8 *)(sel * BATTLE_CHAR_ROW_STRIDE + rowOffset) + (s32)pBase +
                                 pBase->rowIndex * sizeof(BattleRow));
            g_BattleCurrentRow = pRow;
            other = pActor->agility;
            t = pRow->unk27;
            if (t < other) {
                v = (other - t) * 9;
            } else {
                v = 9;
            }
        } else {
            rowOffset = BATTLE_GEAR_ROW_OFFSET;
            pGearRow = (BattleRow *)((u8 *)(sel * BATTLE_GEAR_ROW_STRIDE + rowOffset) + (s32)pBase +
                                     pBase->rowIndex * sizeof(BattleRow));
            g_BattleCurrentRow = pGearRow;
            other = pActor->state.agility;
            t = pGearRow->unk27;
            if (t < other) {
                v = (other - t) * 9;
            } else {
                v = 9;
            }
        }
    } else {
        v = pActor->agility * 9;
    }

    if ((u16)v >= 166) {
        v = 160;
    }
    v = 165 - v;
    d = rand() % 8 - 4;
    v -= d;
    BattleRestoreSlotConditionFlags();
    return v;
}
void BattleInitActiveSlotActions(u8 sel) {
    u8 i;
    u16 mask;

    BattleClearAllSlotStates();

    mask = 1;
    for (i = 0; i < BATTLE_ACTOR_SLOT_COUNT; i++) {
        if (g_BattleCurWorkBlock->activeSlotMask & mask) {
            func_80098D2C(i, sel);
        }
        mask <<= 1;
    }

    *(u16 *)((u8 *)g_BattleCurWorkBlock + 0x5FB0) = D_800D220E[sel].unk0;

    if (g_BattleCommandCellFlags & 0x8000) {
        g_BattleActionParam = 0xC2;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80098D2C);
u8 BattleTestActorSlotFlagsC002ByMode(void) {
    u8 i;
    u8 n;
    s32 ret = 0;
    s32 mode = g_BattleActorSlotFlagTestMode;

    switch (mode) {
    case 1:
        break;
    case 2:
        n = 0;
        for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
            if (g_BattleCurWorkBlock->slots[i].activeEffectFlags1 & 0xC002) {
                n++;
            }
        }
        if (n == 0) {
            ret = 1;
        }
        break;
    case 3:
        n = 0;
        for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
            if (g_BattleCurWorkBlock->slots[i].activeEffectFlags1 & 0xC002) {
                n++;
            }
        }
        if (n != 2) {
            ret = 1;
        }
        break;
    default:
        break;
    }
    return ret;
}
void BattleSetSlotConditionTimer(u8 slot, u8 kind, u16 flag, s32 count) {
    BattleTableEntry *pEntry;
    u8 *pTimer;
    u8 idx = BATTLE_CONDITION_TIMER_INDEX_NONE;
    u8 val = count;
    u16 bits;

    if (g_BattleWorkBlock[g_BattleTargetSlotIndex].accessoryEffectFlags4 & 0x2000) {
        val *= 2;
    }
    pEntry = &g_BattleWorkBlock[slot];
    if (kind == 0) {
        if (flag != 0x1000) {
            idx = (flag == 0x2000) ? 0 : BATTLE_CONDITION_TIMER_INDEX_NONE;
        } else {
            idx = 1;
        }
    }
    if (kind == 2) {
        switch (flag) {
        case 0x1000:
            idx = 2;
            break;
        case 0x800:
            idx = 3;
            break;
        }
    }
    if (kind == 5) {
        switch (flag) {
        case 0x8000:
            idx = 4;
            break;
        case 0x4000:
            idx = 5;
            break;
        }
    }
    if (kind == 7) {
        switch (flag) {
        case 0x8000:
            idx = 9;
            break;
        case 0x4000:
            idx = 0xA;
            break;
        case 0x1000:
            idx = 0xB;
            break;
        }
    }
    if (kind == 9) {
        switch (flag) {
        case 0x1000:
        case 0x2000:
        case 0x4000:
        case 0x8000:
            idx = 0xC;
            break;
        case 0x100:
        case 0x200:
        case 0x400:
        case 0x800:
            idx = 0xD;
            break;
        }
    }
    if (idx == BATTLE_CONDITION_TIMER_INDEX_NONE) {
        return;
    }
    switch (kind) {
    case 5:
    case 7:
    case 9:
        bits = g_BattleTargetSlotEntry->activeEffectFlags4 | g_BattleTargetSlotEntry->accessoryEffectFlags4;
        if (bits & 5) {
            val *= 2;
        }
        if (bits & 0xA) {
            val >>= 1;
        }
        break;
    }
    if (pEntry->accessoryEffectFlags5 & BATTLE_ACCESSORY_TIMER_DOUBLE_FLAG) {
        switch (kind) {
        case 5:
        case 7:
        case 9:
            val *= 2;
            break;
        }
    }
    pTimer = D_800CCE44[slot];
    pTimer[idx] = val;
}
// Rolls one slot's condition timers down and returns the mask of the conditions
// that expired this frame. Each armed flag word (0x7C..0x8E) owns one timer byte
// at entry+0x15C, i.e. state.conditionTimers[1], and the 0x84/0x88 tests are a pair: the
// low halfword's flag armed with the high halfword's copy of it clear. The timer
// and state pointers are formed off the symbol base, not off pEntry: the original
// keeps that second base in a register of its own.
u16 BattleDecrementSlotConditionTimers(u8 slot) {
    BattleTableEntry *pEntry = &g_BattleWorkBlock[slot];
    BattleSlotState *pState = (BattleSlotState *)(slot * 0x170 + 0xA4 + (u32)g_BattleWorkBlock);
    u8 *pCnt = (u8 *)(slot * 0x170 + 0x15C + (u32)g_BattleWorkBlock);
    u32 ret;

    if (g_BattleSlotConditions[slot][0] & 0x80) {
        BattleDecrementConditionTimers((BattleTableEntry *)pState, pEntry, pCnt);
        return 0;
    }
    ret = 0;
    if (pEntry->activeEffectFlags1 & 0x1000) {
        pCnt[1] = pCnt[1] + 0xFF;
        if (pCnt[1] == 0) {
            ret |= 0x4000;
            pEntry->activeEffectFlags1 &= 0xEFFF;
        }
    }
    if (pEntry->activeEffectFlags2 & 0x1000) {
        pCnt[2] = pCnt[2] + 0xFF;
        if (pCnt[2] == 0) {
            ret |= 0x2000;
            pEntry->activeEffectFlags2 &= 0xEFFF;
        }
    }
    if (pEntry->activeEffectFlags2 & 0x800) {
        pCnt[3] = pCnt[3] + 0xFF;
        if (pCnt[3] == 0) {
            ret |= 0x1000;
            pEntry->activeEffectFlags2 &= 0xF7FF;
            pEntry->conditionFlags &= 0xFFDF;
        }
    }
    if ((pEntry->activeEffectFlags3 & 0x8000) && !(pEntry->accessoryEffectFlags3 & 0x8000)) {
        pCnt[4] = pCnt[4] + 0xFF;
        if (pCnt[4] == 0) {
            ret |= 0x800;
            pEntry->activeEffectFlags3 &= 0x7FFF;
        }
    }
    if ((pEntry->activeEffectFlags3 & 0x4000) && !(pEntry->accessoryEffectFlags3 & 0x4000)) {
        pCnt[5] = pCnt[5] + 0xFF;
        if (pCnt[5] == 0) {
            ret |= 0x400;
            pEntry->activeEffectFlags3 &= 0xBFFF;
        }
    }
    if ((pEntry->activeEffectFlags3 & 0x2000) && !(pEntry->accessoryEffectFlags3 & 0x2000)) {
        pCnt[6] = pCnt[6] + 0xFF;
        if (pCnt[6] == 0) {
            ret |= 0x200;
            pEntry->activeEffectFlags3 &= 0xDFFF;
        }
    }
    if ((pEntry->activeEffectFlags3 & 0x1000) && !(pEntry->accessoryEffectFlags3 & 0x1000)) {
        pCnt[7] = pCnt[7] + 0xFF;
        if (pCnt[7] == 0) {
            ret |= 0x100;
            pEntry->activeEffectFlags3 &= 0xEFFF;
        }
    }
    if ((pEntry->activeEffectFlags3 & 0x800) && !(pEntry->accessoryEffectFlags3 & 0x800)) {
        pCnt[8] = pCnt[8] + 0xFF;
        if (pCnt[8] == 0) {
            ret |= 0x80;
            pEntry->activeEffectFlags3 &= 0xF7FF;
        }
    }
    if ((pEntry->activeEffectFlags4 & 0x8000) && !(pEntry->accessoryEffectFlags4 & 0x8000)) {
        pCnt[9] = pCnt[9] + 0xFF;
        if (pCnt[9] == 0) {
            ret |= 0x40;
            pEntry->activeEffectFlags4 &= 0x7FFF;
        }
    }
    if ((pEntry->activeEffectFlags4 & 0x4000) && !(pEntry->accessoryEffectFlags4 & 0x4000)) {
        pCnt[0xA] = pCnt[0xA] + 0xFF;
        if (pCnt[0xA] == 0) {
            ret |= 0x20;
            pEntry->activeEffectFlags4 &= 0xBFFF;
        }
    }
    if ((pEntry->activeEffectFlags4 & 0x1000) && !(pEntry->accessoryEffectFlags4 & 0x1000)) {
        pCnt[0xB] = pCnt[0xB] + 0xFF;
        if (pCnt[0xB] == 0) {
            ret |= 0x10;
            pEntry->activeEffectFlags4 &= 0xEFFF;
        }
    }
    if (pEntry->activeEffectFlags7 & 0xF000) {
        if (!(pEntry->accessoryEffectFlags7 & 0xF000)) {
            pCnt[0xC] = pCnt[0xC] + 0xFF;
            if (pCnt[0xC] == 0) {
                ret |= 8;
                pEntry->activeEffectFlags7 &= 0xFFF;
            }
        }
    }
    if (pEntry->activeEffectFlags7 & 0xF00) {
        if (!(pEntry->accessoryEffectFlags7 & 0xF00)) {
            pCnt[0xD] = pCnt[0xD] + 0xFF;
            if (pCnt[0xD] == 0) {
                ret |= 4;
                pEntry->activeEffectFlags7 &= 0xF0FF;
            }
        }
    }
    return ret;
}
void BattleDecrementConditionTimers(BattleTableEntry *p, BattleTableEntry *pOther, u8 *pCnt) {
    if (p->activeEffectFlags1 & 0x200) {
        pCnt[1] = pCnt[1] + 0xFF;
        if (pCnt[1] == 0) {
            p->activeEffectFlags1 &= 0xFDFF;
        }
    }
    if (p->activeEffectFlags1 & 0x100) {
        pCnt[2] = pCnt[2] + 0xFF;
        if (pCnt[2] == 0) {
            p->activeEffectFlags1 &= 0xFEFF;
        }
    }
    if (p->activeEffectFlags1 & 0x80) {
        pCnt[3] = pCnt[3] + 0xFF;
        if (pCnt[3] == 0) {
            p->activeEffectFlags1 &= 0xFF7F;
        }
    }
    if (p->activeEffectFlags1 & 0x20) {
        pCnt[4] = pCnt[4] + 0xFF;
        if (pCnt[4] == 0) {
            p->activeEffectFlags1 &= 0xFFDF;
            pOther->activeEffectFlags1 &= 0xEFFF;
        }
    }
    if (p->activeEffectFlags1 & 0x10) {
        pCnt[5] = pCnt[5] + 0xFF;
        if (pCnt[5] == 0) {
            p->activeEffectFlags1 &= 0xFFEF;
        }
    }
    if (p->activeEffectFlags1 & 0xF000) {
        pCnt[7] = pCnt[7] + 0xFF;
        if (pCnt[7] == 0) {
            p->activeEffectFlags1 &= 0x0FFF;
        }
    }
    if (p->activeEffectFlags1 & 0xF00) {
        pCnt[8] = pCnt[8] + 0xFF;
        if (pCnt[8] == 0) {
            p->activeEffectFlags1 &= 0xF0FF;
        }
    }
    if (p->activeEffectFlags2 & 0x1000) {
        pCnt[0xA] = pCnt[0xA] + 0xFF;
        if (pCnt[0xA] == 0) {
            p->activeEffectFlags2 &= 0xEFFF;
        }
    }
    if (p->activeEffectFlags2 & 0x40) {
        pCnt[0xB] = pCnt[0xB] + 0xFF;
        if (pCnt[0xB] == 0) {
            p->activeEffectFlags2 &= 0xFFBF;
        }
    }
    if (p->activeEffectFlags2 & 0x20) {
        pCnt[0xC] = pCnt[0xC] + 0xFF;
        if (pCnt[0xC] == 0) {
            p->activeEffectFlags2 &= 0xFFDF;
        }
    }
}
void BattleSaveScriptCommandOperandsFromActor(void) {
    u16 ab = g_BattleActingSlot->activeEffectFlags7 | g_BattleActingSlot->accessoryEffectFlags7;

    g_BattleCurWorkBlock->unk5FBC = g_BattleCurrentRow->unk20;
    g_BattleCurWorkBlock->unk5FBD = g_BattleCurrentRow->unk21;
    g_BattleCurWorkBlock->element = g_BattleCurrentRow->element;
    g_BattleCurWorkBlock->unk5FBF = g_BattleCurrentRow->unk23;
    g_BattleCurWorkBlock->unk5FC0 = g_BattleCurWorkBlock->rowIndex;

    ab >>= 12;
    if ((g_BattleCurWorkBlock->element & BATTLE_ELEMENT_MASK) == 0) {
        g_BattleCurWorkBlock->element |= ab;
    }
}
void BattleFilterSlotMaskByGroup(void) {
    if (g_BattleCurWorkBlock->activeSlotMask & BATTLE_PARTY_SLOT_MASK) {
        g_BattleCurWorkBlock->activeSlotMask = g_BattleCurWorkBlock->slotMask & BATTLE_PARTY_SLOT_MASK;
    }
    if (g_BattleCurWorkBlock->activeSlotMask & BATTLE_OTHER_SLOT_MASK) {
        g_BattleCurWorkBlock->activeSlotMask = g_BattleCurWorkBlock->slotMask & BATTLE_OTHER_SLOT_MASK;
    }
}
