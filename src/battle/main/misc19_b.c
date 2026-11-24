#include "common.h"
#include "battle/actor.h"
#include "main/game.h"

// The evade and hit percents BattleResetActorStats puts a slot's stats back to.
#define BATTLE_STAT_EVADE_RESET 20
#define BATTLE_STAT_HIT_RESET 15

// The commandMask of a character who has every one of the 13 learnable command rows.
#define BATTLE_COMMAND_MASK_ALL 0xFFF8

extern BattleWorkBlock* g_BattleCurWorkBlock;
extern BattleTableEntry g_BattleWorkBlock[];
extern BattleTableEntry* g_BattleActingSlot;

extern int rand(void);
extern void BattleSetActorScaledHp(u8 arg0, BattleTableEntry* pArg);

// Retail's own symbol for the flag word this clears is D_800CCD68, which is
// g_BattleWorkBlock + 0x80, i.e. the table's activeEffectFlags2; only the addend of
// the relocation differs. The typed access is what keeps the address a symbol plus
// the index register (the byte-pointer spelling makes gcc hoist the base out of the
// loop and costs the function an instruction).
void BattleResetSlotEffectWords(u8 arg0) {
    BattleTableEntry* p;
    u16 keep;
    s32 characterId;
    u8 i;

    g_BattleCurWorkBlock->rowIndex = 0;
    p = (BattleTableEntry*)(arg0 * sizeof(BattleTableEntry) + (u32)g_BattleCurWorkBlock);
    keep = p->activeEffectFlags2;
    characterId = p->characterId;
    p->activeEffectFlags1 = 0;
    p->activeEffectFlags3 = 0;
    p->activeEffectFlags4 = 0;
    p->activeEffectFlags7 = 0;
    p->activeEffectFlags2 = keep & BATTLE_GEAR_EFFECT_FLAG;
    if (characterId == 7) {
        BattleSetActorScaledHp(arg0, p);
    }
    if (p->characterId == 3) {
        for (i = MAX_PARTY_MEMBERS; i < BATTLE_ACTOR_SLOT_COUNT; i++) {
            g_BattleWorkBlock[i].activeEffectFlags2 &= ~0x20;
        }
    }
}
void BattleDecrementGearPartUse(void) {
    u8 sel = g_BattleCurWorkBlock->rowIndex;
    s32 idx;

    switch (sel) {
    case 0:
    case 1:
    case 2:
    case 3:
        idx = g_BattleActingSlot->extraWeaponIds[0];
        break;
    case 4:
    case 5:
        return;
    case 6:
        idx = g_BattleActingSlot->extraWeaponIds[3];
        break;
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
        idx = g_BattleActingSlot->extraWeaponIds[0];
        if (g_GameState.gearPartsIds[0x6C + idx]) {
            g_GameState.gearPartsIds[0x6C + idx] = g_GameState.gearPartsIds[0x6C + idx] + 0xFF;
        }
        idx = g_BattleActingSlot->extraWeaponIds[3];
        break;
    default:
        return;
    }
    if (g_GameState.gearPartsIds[0x6C + idx]) {
        g_GameState.gearPartsIds[0x6C + idx] = g_GameState.gearPartsIds[0x6C + idx] + 0xFF;
    }
}

void BattleResetActorStats(void) {
    u8 i;

    for (i = 1; i < MAX_PARTY_MEMBERS; i++) {
        BattleTableEntry* p = (BattleTableEntry*)(i * sizeof(BattleTableEntry) + (u32)g_BattleCurWorkBlock);

        p->hp = BATTLE_STAT_PERCENT_MAX;
        p->maxHp = BATTLE_STAT_PERCENT_MAX;
        p->evade = BATTLE_STAT_EVADE_RESET;
        p->hit = BATTLE_STAT_HIT_RESET;
        p->conditionFlags = BATTLE_CONDITION_FLAGS_RESET;
        p->state.commandCounterMax = 0;
    }
}

void BattleSetActorScaledHp(u8 arg0, BattleTableEntry* pArg) {
    BattleTableEntry* p = (BattleTableEntry*)(arg0 * sizeof(BattleTableEntry) + (u32)g_BattleCurWorkBlock);
    BattleSlotState* pState = &p->state;

    if (pArg->maxHp * BATTLE_ACTOR_HP_SCALE > BATTLE_SCALED_HP_MAX) {
        if (pArg->hp * BATTLE_ACTOR_HP_SCALE > BATTLE_SCALED_HP_MAX) {
            p->state.hp = BATTLE_SCALED_HP_MAX;
        } else {
            p->state.hp = pArg->hp * BATTLE_ACTOR_HP_SCALE;
        }
        // only this store goes through the state pointer ($a3 = p + 0xA4) in retail
        pState->maxHp = BATTLE_SCALED_HP_MAX;
    } else {
        p->state.hp = pArg->hp * BATTLE_ACTOR_HP_SCALE;
        p->state.maxHp = pArg->maxHp * BATTLE_ACTOR_HP_SCALE;
    }
}

void BattleInitItemInventoryAndConditionMasks(void) {
    u_char i;

    for (i = 1; i < 48; i++) {
        g_GameState.itemIDs[i] = i;
        g_GameState.itemQuantities[i] = MAX_ITEM_QUANTITY;
    }
    g_GameState.conditionRows[0].commandMask = BATTLE_COMMAND_MASK_ALL;
    *(u16 *)&g_GameState.conditionRows[0].characterConditionMask = 0xFF00;
    g_GameState.conditionRows[0].gearCommandMask = 0xFFFF;
    *(u16 *)&g_GameState.conditionRows[0].gearConditionMask = 0xFE00;
    *(u16 *)&g_GameState.conditionRows[0].statusFlags = 0xF000;
    g_GameState.conditionRows[0].unk1B = 7;
    g_GameState.conditionRows[1].commandMask = 0xFFE0;
    *(u16 *)&g_GameState.conditionRows[1].characterConditionMask = 0xFFF0;
    g_GameState.conditionRows[1].gearCommandMask = 0xFFFF;
    *(u16 *)&g_GameState.conditionRows[1].gearConditionMask = 0xFFF0;
    *(u16 *)&g_GameState.conditionRows[1].statusFlags = 0xC000;
    g_GameState.conditionRows[1].unk1B = 7;
    g_GameState.conditionRows[2].commandMask = 0xFFE0;
    *(u16 *)&g_GameState.conditionRows[2].characterConditionMask = 0xFFE0;
    g_GameState.conditionRows[2].gearCommandMask = 0xFFFF;
    *(u16 *)&g_GameState.conditionRows[2].gearConditionMask = 0xFF00;
    *(u16 *)&g_GameState.conditionRows[2].statusFlags = 0x8000;
    g_GameState.conditionRows[2].unk1B = 7;
    g_GameState.conditionRows[3].commandMask = 0xFFE0;
    *(u16 *)&g_GameState.conditionRows[3].characterConditionMask = 0xFFC0;
    g_GameState.conditionRows[3].gearCommandMask = 0xFFFF;
    *(u16 *)&g_GameState.conditionRows[3].gearConditionMask = 0xFF00;
    *(u16 *)&g_GameState.conditionRows[3].statusFlags = 0xF000;
    g_GameState.conditionRows[3].unk1B = 7;
    g_GameState.conditionRows[4].commandMask = 0xFFC0;
    *(u16 *)&g_GameState.conditionRows[4].characterConditionMask = 0xFFC0;
    g_GameState.conditionRows[4].gearCommandMask = 0xFFFF;
    *(u16 *)&g_GameState.conditionRows[4].gearConditionMask = 0xFC00;
    *(u16 *)&g_GameState.conditionRows[4].statusFlags = 0xE000;
    g_GameState.conditionRows[4].unk1B = 7;
    g_GameState.conditionRows[5].commandMask = 0xFFC0;
    *(u16 *)&g_GameState.conditionRows[5].characterConditionMask = 0xF000;
    g_GameState.conditionRows[5].gearCommandMask = 0xFFFF;
    *(u16 *)&g_GameState.conditionRows[5].gearConditionMask = 0xF000;
    *(u16 *)&g_GameState.conditionRows[5].statusFlags = 0x8000;
    g_GameState.conditionRows[5].unk1B = 7;
    g_GameState.conditionRows[6].commandMask = 0xFFC0;
    *(u16 *)&g_GameState.conditionRows[6].characterConditionMask = 0xFF00;
    g_GameState.conditionRows[6].gearCommandMask = 0xFFFF;
    *(u16 *)&g_GameState.conditionRows[6].gearConditionMask = 0xFF00;
    *(u16 *)&g_GameState.conditionRows[6].statusFlags = 0x8000;
    g_GameState.conditionRows[6].unk1B = 7;
    g_GameState.conditionRows[7].commandMask = 0x0000;
    *(u16 *)&g_GameState.conditionRows[7].characterConditionMask = 0xFF00;
    g_GameState.conditionRows[7].gearCommandMask = 0x0000;
    *(u16 *)&g_GameState.conditionRows[7].gearConditionMask = 0xFF00;
    *(u16 *)&g_GameState.conditionRows[7].statusFlags = 0x0000;
    g_GameState.conditionRows[7].unk1B = 7;
    g_GameState.conditionRows[8].commandMask = 0x0000;
    *(u16 *)&g_GameState.conditionRows[8].characterConditionMask = 0xF800;
    g_GameState.conditionRows[8].gearCommandMask = 0xFFFF;
    *(u16 *)&g_GameState.conditionRows[8].gearConditionMask = 0x0000;
    *(u16 *)&g_GameState.conditionRows[8].statusFlags = 0xE000;
    g_GameState.conditionRows[8].unk1B = 7;
    g_GameState.conditionRows[9].commandMask = 0xFFE0;
    *(u16 *)&g_GameState.conditionRows[9].characterConditionMask = 0xFFE0;
    g_GameState.conditionRows[9].gearCommandMask = 0xFFFF;
    *(u16 *)&g_GameState.conditionRows[9].gearConditionMask = 0xFFE0;
    *(u16 *)&g_GameState.conditionRows[9].statusFlags = 0x8000;
    g_GameState.conditionRows[9].unk1B = 7;
    g_GameState.conditionRows[10].commandMask = 0xFFC0;
    *(u16 *)&g_GameState.conditionRows[10].characterConditionMask = 0xFF00;
    g_GameState.conditionRows[10].gearCommandMask = 0xFFFF;
    *(u16 *)&g_GameState.conditionRows[10].gearConditionMask = 0xFF00;
    *(u16 *)&g_GameState.conditionRows[10].statusFlags = 0xC000;
    g_GameState.conditionRows[10].unk1B = 7;
}

// PARKED (97.59%, 536 bytes = retail's size). Body COMPILES.
// Round 306: the two g_BattleSlotConditions reads are typed here as
// g_BattleWorkBlock[i].state.flags (D_800CCD68 is g_BattleWorkBlock + 0x15A, i.e.
// state.flags), and that variant is what reproduces retail's per-reference
// lui $at / addu $at form; the byte-array spelling made gcc hoist the base out of the
// loop and cost an instruction (that was the old 93.87% / 532-byte park). The second
// *arg0 read needs the volatile-lvalue cast, or gcc CSEs it away where retail reloads
// it after the mult; that is what brings the body to retail's 536 bytes.
// Remaining, three allocator/md ties: retail keeps maxHp in $v1 and hp in $a1 (this
// body the other way round, in both arms), retail's arg0 pointer is $s2 and limit $s0
// (here $s0 and $s2), and retail's rand() % 100 reads mfhi $a0 and shifts in place
// where 2.7.2 emits mfhi $t1 / sra $a0,$t1,5 -- the same md difference that keeps
// BattleResetRowIndexAndMaybeRefreshStats and BattleSetSlotActionStateAndAmount off 100. Try_flags on this body: 97.81 under
// gcc-2.6.0, 94.63 under 2.7.2-cdk. Measured this round: limit/flags/drop declaration
// order (no change), max/cur assigned cur-first (worse - $a1 becomes an address base).
// Round 397 re-measured live: 97.59% (536 bytes, 134 insns = retail's size), 97.81 under
// gcc-2.6.0-psx (try_flags), matching the note below. The s0/s2 mirror is now PROVEN
// unreachable by spelling via the -da dump (tools/gcc-2.7.2-psx/cc1, `-da` on the unit's
// preprocessed file): the three callee-saved allocnos are arg0 (6 refs / 97 insns live,
// priority 2*6/97*4 = 0.495), drop (3 / 25, 0.48) and limit (3 / 34, 0.353), so arg0 takes
// $s0 and limit $s2; retail's limit=$s0 would need limit to outrank arg0, i.e. a 6-ref
// pointer losing to a 3-ref constant, which no declaration order, width (`u8 drop` measured:
// no change at all) or statement spelling reaches. The mfhi/mflo in-place read-backs are the
// round-373-proven 2.7.2 md wall (mfhi t1 / sra a0,t1,5 vs retail's in-place pair), and
// the loop's v1/a1 maxHp/hp mirror is the same allocator contest. Three independent
// residuals, no preset closes the function, unit is mixed under 2.6.0: park is final.
// Round 426: the count/index/pointer rotation is also an allocator-ORDER difference, not a
// spelling one - retail assigns count=$a0, the i*0x170 temp=$a1, p=$a2 (allocno order),
// while 2.7.2's priority sort hands the temp $a0, p $a1 and count $a2. Two more declaration
// orders measured dead (i before count: identical 108 differing lines; max/cur hoisted to
// function scope: identical), so the whole a-register rotation plus the s0/s2 mirror is the
// 2.6.0-allocator family and the three mflo read-backs are the md wall. Park stays final.
// Round 439 re-measured live: 97.55% with the body's slot pointer written as the realistic
// &g_BattleCurWorkBlock->slots[i] (97.59% with the old (BattleTableEntry*)(i * 0x170 +
// (u32)g_BattleCurWorkBlock) cast); both 536 bytes / 134 insns = retail's exact size, 108
// differing lines, and the two spellings differ only in fuzzy noise. Swept the preset
// matrix (try_flags --sweep: best 97.81 at gcc-2.6.0, 2.7.2 97.59, cdk 94.6), so no preset
// closes it; lever a-whole-caller-saved-rotation-in-a-counting-loop already records this
// exact function. Park refreshed, no C change.
/*
void func_8009B46C(u16* arg0) {
    u8 count = 0;
    u8 i;

    for (i = 0; i < 3; i++) {
        BattleTableEntry* p = &g_BattleCurWorkBlock->slots[i];
        BattleSlotState* pState = &p->state;
        u32 max;
        u32 cur;

        if (p->characterId == 0) {
            if (g_BattleWorkBlock[i].state.flags & BATTLE_SLOT_GEAR_STATUS_FLAG) {
                max = p->state.maxHp;
                cur = p->state.hp;
            } else {
                max = p->maxHp;
                cur = p->hp;
            }
            if (cur < (max >> 1)) {
                count++;
            }
            if (cur < (max >> 2)) {
                count++;
            }
        }
        if (p->characterId == 3) {
            if (g_BattleWorkBlock[i].state.flags & BATTLE_SLOT_GEAR_STATUS_FLAG) {
                max = pState->maxHp;
                cur = pState->hp;
            } else {
                max = p->maxHp;
                cur = p->hp;
            }
            if (cur < (max >> 1)) {
                count++;
            }
            if (cur < (max >> 2)) {
                count++;
            }
        }
    }
    {
        s32 drop;
        u16 flags;
        s32 limit = 0xA;

        if (count != 0) {
            *arg0 = *(volatile u16*)arg0 + count * (*arg0 >> 1);
        }
        flags = g_BattleActingSlot->accessoryEffectFlags5;
        drop = (flags & 0x400) ? 4 : 3;
        if (flags & 0x200) {
            limit = 0x3C;
        }
        if (rand() % 100 < limit) {
            *arg0 = (drop * *arg0) >> 1;
        }
    }
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19_b", func_8009B46C);

void BattleSetCommandCodeForSlotKindFlag(u8 arg0, u16 arg1) {
    switch (arg0) {
    case 0:
        switch (arg1) {
        case 0x2000:
            g_BattleCurWorkBlock->commandCode = 1;
            break;
        case 0x1000:
            g_BattleCurWorkBlock->commandCode = 2;
            break;
        case 0x800:
            g_BattleCurWorkBlock->commandCode = 3;
            break;
        case 0x400:
            g_BattleCurWorkBlock->commandCode = 4;
            break;
        case 0x200:
            g_BattleCurWorkBlock->commandCode = 5;
            break;
        case 0x1:
            g_BattleCurWorkBlock->commandCode = 8;
            break;
        }
        break;
    case 2:
        switch (arg1) {
        case 0x2000:
            g_BattleCurWorkBlock->commandCode = 9;
            break;
        case 0x1000:
            g_BattleCurWorkBlock->commandCode = 0xA;
            break;
        case 0x800:
            g_BattleCurWorkBlock->commandCode = 0xB;
            break;
        case 0x400:
            g_BattleCurWorkBlock->commandCode = 0xC;
            break;
        case 0x1:
            g_BattleCurWorkBlock->commandCode = 0xD;
            break;
        case 0x20:
            g_BattleCurWorkBlock->commandCode = 7;
            break;
        }
        break;
    case 5:
        switch (arg1) {
        case 0x8000:
            g_BattleCurWorkBlock->commandCode = 0xE;
            break;
        case 0x4000:
            g_BattleCurWorkBlock->commandCode = 0xF;
            break;
        case 0x2000:
            g_BattleCurWorkBlock->commandCode = 0x10;
            break;
        case 0x1000:
            g_BattleCurWorkBlock->commandCode = 0x11;
            break;
        case 0x800:
            g_BattleCurWorkBlock->commandCode = 0x12;
            break;
        case 0x1800:
            g_BattleCurWorkBlock->commandCode = 0x13;
            break;
        }
        break;
    case 7:
        switch (arg1) {
        case 0x8000:
            g_BattleCurWorkBlock->commandCode = 0x15;
            break;
        case 0x4000:
            g_BattleCurWorkBlock->commandCode = 0x16;
            break;
        case 0x1000:
            g_BattleCurWorkBlock->commandCode = 0x18;
            break;
        case 0x8:
        case 0x2:
            g_BattleCurWorkBlock->commandCode = 0x19;
            break;
        case 0x1:
        case 0x4:
            g_BattleCurWorkBlock->commandCode = 0x1A;
            break;
        }
        break;
    case 9:
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

