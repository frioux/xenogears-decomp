#include "common.h"
#include "battle/actor.h"
#include "main/game.h"

extern BattleWorkBlock* g_BattleCurWorkBlock;
extern u8 g_BattleCurSlotIndex;
extern BattleRow* g_BattleCurrentRow;
extern u8* g_BattleCurSlotState;
extern u8 g_BattleSlotConditions[][0x170];
extern BattleTableEntry g_BattleWorkBlock[];
extern u8 g_BattleActorCharIds[];

extern int rand(void);

void func_8009BAC4(u8 arg0, u8* pInfo, s16* pStatus) {
    BattleTableEntry* p = &g_BattleWorkBlock[arg0];
    u16 mask;
    u8 count;
    u8 bit;

    if (g_BattleSlotConditions[arg0][0] & BATTLE_SLOT_GEAR_STATUS_FLAG) {
        mask = 1;
    } else {
        if (rand() % BATTLE_PERCENT_ROLL_RANGE < 10 && !(p->conditionFlags & 0x100) && *pStatus == 0) {
            *pInfo = 4;
            return;
        }
        switch (p->characterId) {
        case 3:
            mask = 0xC3C0;
            count = 0xA;
            break;
        case 4:
            mask = 0xDF80;
            count = 10;
            break;
        case 5:
            mask = 0x1000;
            count = 4;
            break;
        case 7:
            mask = 0xE000;
            count = 3;
            break;
        case 0:
        case 8:
            mask = 0xC000;
            count = 2;
            break;
        case 2:
        case 9:
            mask = 0xBFE0;
            count = 11;
            break;
        case 1:
        case 6:
        case 10:
            mask = 0xF000;
            count = 4;
            break;
        }
        if (rand() % BATTLE_PERCENT_ROLL_RANGE < 25 && !(p->conditionFlags & 0x20)) {
            *pInfo = 2;
            bit = rand() % count;
            if (g_GameState.conditionRows[p->characterId].characterConditionMask & ((0x8000 >> bit) & mask)) {
                pInfo[1] = bit;
                return;
            }
        }
        if (p->characterId == 8) {
            *pInfo = 0;
            pInfo[1] = 0;
            return;
        }
        mask = 1;
    }
    *pInfo = mask;
    if (rand() % BATTLE_PERCENT_ROLL_RANGE < 80) {
        if (rand() % BATTLE_PERCENT_ROLL_RANGE >= 60) {
            pInfo[1] = mask;
        } else {
            pInfo[1] = 0;
        }
    } else {
        pInfo[1] = 2;
    }
}
void func_8009BD94(void) {
    g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] = 2;
    g_BattleCurWorkBlock->slotAmounts[g_BattleCurSlotIndex] =
        (u32)(g_BattleCurrentRow->power * ((BattleSlotState*)g_BattleCurSlotState)->maxHp) / 20;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19_r1", func_8009BE0C);
// PARKED (90.12% under 2.7.2 / 97.23% under this TU's gcc 2.6.0, 576 bytes against
// retail's 580 = ONE instruction short). Body COMPILES. Round 394 re-measured both
// numbers after the unit was split: retail's size is 580 bytes, not the 552 this note
// used to claim (the size was never measured against the .s), and under 2.6.0 the
// division ties below are gone - what is left is the gear-id test pair (the missing
// instruction), the two clamp rotations, and a prologue tie: retail holds
// &g_BattleWorkBlock in $t5 and &g_GameState in $t4, 2.6.0 swaps the two.
// Structure is retail's: the outer 3-slot loop, the characterId==7 hp arm, the
// character copy (hp/mp clamp, 7 stat-weight words, the 0x3A word, the 0xC000
// flag -> hp 1), then the gear copy. Load-bearing and measured:
//  - the gear-id validation must be TWO tests
//    (`slti 17 / beqz` then `slti 8 / bnez`, both to the loop bottom). The parked
//    body below is the single-condition `(gearId < 0x11 && gearId > 7)` spelling,
//    which folds to `addiu v1,-8 / sltiu 9` and is 90.12%; writing the copy body
//    twice under `if (gearId < 7) ... else if (gearId < 0x11) { if (gearId >= 8)`
//    gives retail's two tests and cross-jumps to one copy, but it also makes gcc
//    hoist `0xCCCCCCCD` into $t3 for the two `/ 10`s, which costs a register:
//    89.99. Both spellings are wrong in a different way; that is the next lever.
//  - the `s32 gearId = p->gearId; if (gearId < 0) continue;` guard is required: it
//    is what emits retail's `bltz $v1`.
//  - `*(volatile u16 *)&p->hp = (pState->hp + 1) / 50` (only the STORE volatile)
//    is what brings back retail's `sh / lhu` store-then-reload pair; a plain store
//    makes gcc fold the reload to the value it already has.
//  - `u32 frame[2]` is unused but supplies retail's 8-byte phantom frame.
// Remaining, all register-order ties this body cannot reach:
//  1. both divisions: retail `multu v0,v1 ; mfhi v0 ; srl v0,v0,4` (and /10 the
//     same with srl 3), this compiler `mfhi $tN ; srl v0,$tN,4` - the same cc1
//     division-expansion difference that caps func_8009A9D0 (99.66) and
//     func_8009BD94 (98.33) in this unit. try_flags on this body: 98.26 under
//     gcc-2.6.0, so this is not a preset win either.
//  2. both `if (a > b) a = b;` clamps: retail loads the LEFT operand first
//     (`lhu v1,80(a1)` mp then `82` maxMp) and reloads b for the store, this body
//     loads b first and keeps it in a register (`sltu v0,v1,v0`), so the block is
//     two instructions rotated. Measured, no effect: the pChar->hp store volatile.
//  3. the pChar->maxHp load is hoisted above the pChar->hp store here.
// Round 394, on 2.6.0: the nested `if (gearId < 7) .. else if (gearId < 0x11) {
// if (gearId > 7)` spelling with the copy body written twice is 95.64% / 588 bytes -
// 2.6.0 does NOT cross-jump it to one copy, so the duplicated body costs 8 bytes and
// makes the three ties worse. The single-condition spelling below stays the best.
// A 3-minute permuter run from this body was PERMUTER_BETTER base=1601 best=1102
// and its whole finding was `j = 0x11;` then `gearId < j`, i.e. it replaced
// retail's `slti 17` immediate with a register compare - not a real lead.
/*
void func_8009BE0C(void) {
    BattleTableEntry* p;
    BattleSlotState* pState;
    GameCharacter* pChar;
    GameGear* pGear;
    s32 gearId;
    u32 frame[2];
    u8 i;
    u8 j;

    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        if (g_BattleActorCharIds[i] == BATTLE_SLOT_CHARACTER_NONE) {
            continue;
        }
        p = &g_BattleWorkBlock[i];
        pChar = &g_GameState.characters[p->characterId];
        pGear = &g_GameState.gears[p->gearId];
        pState = (BattleSlotState*)((u8*)&g_BattleWorkBlock[i] + 0xA4);
        if (p->characterId == 7 && (g_BattleSlotConditions[i * 0x170] & BATTLE_SLOT_GEAR_STATUS_FLAG)) {
            *(volatile u16*)&p->hp = (pState->hp + 1) / BATTLE_ACTOR_HP_SCALE;
            if (p->hp == 0) {
                p->hp = 1;
            }
        }
        pChar->hp = p->hp;
        pChar->mp = p->mp;
        if (pChar->hp > pChar->maxHp) {
            pChar->hp = pChar->maxHp;
        }
        if (pChar->mp > pChar->maxMp) {
            pChar->mp = pChar->maxMp;
        }
        for (j = 0; j < 7; j++) {
            pChar->statWeights[j] = p->statWeights[j];
        }
        pChar->actionCount = p->actionCount;
        if (p->activeEffectFlags1 & 0xC000) {
            pChar->hp = 1;
        }
        gearId = p->gearId;
        if (gearId < 0) {
            continue;
        }
        if (gearId < 7 || (gearId < 0x11 && gearId > 7)) {
            *(volatile u32*)&pGear->hp = pState->hp;
            pGear->fuel = pState->fuel;
            if (pGear->hp > pGear->maxHp) {
                pGear->hp = pGear->maxHp;
            }
            if (pGear->fuel > pGear->maxFuel) {
                pGear->fuel = pGear->maxFuel;
            }
            if (pState->activeEffectFlags1 & 0x8000) {
                pGear->hp = pGear->maxHp / 10;
            }
        }
    }
}
*/
// The state base (+0xA4) and the condition byte inside it (+0xA4 again) are formed
// from the slot table base before the slot stride is added, as retail does; the
// typed `&g_BattleWorkBlock[arg0].state` / `.state.unkA4` spellings derive the
// second one from the first and lose two registers (94.69%).
u8 BattleGetActorActionConditionMask(u8 arg0) {
    BattleTableEntry* pEntry = &g_BattleWorkBlock[arg0];
    BattleSlotState* pState = (BattleSlotState*)((u8*)&g_BattleWorkBlock[arg0] + 0xA4);
    u8* pCondition = (u8*)&g_BattleWorkBlock[arg0] + 0x148;
    s32 flag = pState->activeEffectFlags1 & 0x400;
    u8 ret = flag != 0;

    if (pState->hp < ((u32)pState->maxHp >> 3)) {
        if (!(pEntry->unk36 & 1)) {
            ret |= BATTLE_ACTION_CONDITION_LOW_HP;
        }
    }
    if (*pCondition == 4) {
        ret |= 4;
    }
    return ret;
}
void BattleSetSlotFields148And162(u8 arg0) {
    g_BattleWorkBlock[arg0].state.commandCounter = 4;
    g_BattleWorkBlock[arg0].state.conditionTimers[7] = 3;
    g_GameState.conditionRows[0].statusFlags |= 0x4000;
}
