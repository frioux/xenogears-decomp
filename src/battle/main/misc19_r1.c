#include "common.h"
#include "battle/actor.h"
#include "main/game.h"

extern BattleWorkBlock* g_BattleCurWorkBlock;
extern u8 g_BattleCurSlotIndex;
extern BattleRow* g_BattleCurrentRow;
extern u8* g_BattleCurSlotState;
extern u8 g_BattleSlotConditions[];
extern BattleTableEntry g_BattleWorkBlock[];
extern u8 g_BattleActorCharIds[];

extern int rand(void);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc19_r1", func_8009BAC4);
// PARKED (99.72% under this unit's gcc-2.7.2, 720 bytes = retail's exact size,
// instruction multiset and 11-word switch table). The ONLY difference is the
// destination of the four `mfhi` in the `rand() % 100` sequences: retail has
// `mfhi a0; sra a0,a0,5`, 2.7.2 emits `mfhi a3; sra a0,a3,5`.
// That is a cc1 division-expansion difference, not a source shape: a full preset
// sweep (2.6.0 / 2.7.2 / 2.7.2-cdk x -O1..-O3 x -G4/-G8 x 3000/r8000) keeps every
// 2.7.2 variant at 99.72 and puts gcc 2.6.0 at 99.94, and a 4-minute permuter run
// from this body found nothing (base=40 best=40, 1146 iterations).
// Under gcc-2.6.0 the object is byte-exact: all 180 instructions and all 11 table
// words equal retail's, and the residual 0.06% is only the `jtbl_800704A0` vs
// `.rodata+0x78` spelling of the table operand (the documented JTBL case, same
// linked bytes). So this body is a MATCH under XenoBattle260, and un-parks as soon
// as this function's run of the unit gets the 2.6.0 preset.
// Two shapes are load-bearing: the switch cases are listed in retail's emitted body
// order (3,4,5,7,0/8,2/9,1/6/10 - any other order moves the blocks) and the tail is
// `if (r >= 0x3C) pInfo[1] = mask; else pInfo[1] = 0;`, which is what lets gcc
// tail-merge the zero store with the characterId == 8 one (the if/else written the
// other way round is 8 bytes longer).
// Also tried, all worse: 6 declaration orders of mask/count/bit, u32 bit (716
// bytes), a shared `s32 r = rand() % 100` local (99.50), an unused u32 local,
// `% 0x64`, early returns instead of the if/else tail, both operand orders of
// `(0x8000 >> bit) & mask` (the swapped one is what matches).
// Round 391: the parked body did NOT COMPILE - it said `p->unk7A`, which BattleTableEntry now
// names `conditionFlags` at 0x7A - so the number above came from a stale object. Fixed (two
// sites, same `lhu 0x7A` width) and re-measured live: 99.72%, 720 bytes, residual unchanged
// (the four `mfhi` destinations). try_flags re-run this round: gcc-2.6.0 99.94, i.e. all 180
// instructions and all 11 table words equal retail with only the jtbl spelling left, so this
// body is a MATCH on a 2.6.0 preset.
/*
void func_8009BAC4(u8 arg0, u8* pInfo, s16* pStatus) {
    BattleTableEntry* p = &g_BattleWorkBlock[arg0];
    u16 mask;
    u8 count;
    u8 bit;

    if (g_BattleSlotConditions[arg0 * 0x170] & 0x80) {
        mask = 1;
    } else {
        if (rand() % 100 < 10 && !(p->conditionFlags & 0x100) && *pStatus == 0) {
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
            count = 0xA;
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
            count = 0xB;
            break;
        case 1:
        case 6:
        case 10:
            mask = 0xF000;
            count = 4;
            break;
        }
        if (rand() % 100 < 25 && !(p->conditionFlags & 0x20)) {
            *pInfo = 2;
            bit = rand() % count;
            if (*(u16*)&g_GameState.conditionRows[p->characterId].characterConditionMask & ((0x8000 >> bit) & mask)) {
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
    if (rand() % 100 < 0x50) {
        if (rand() % 100 >= 0x3C) {
            pInfo[1] = mask;
        } else {
            pInfo[1] = 0;
        }
    } else {
        pInfo[1] = 2;
    }
}
*/
// best 98.33% (120 bytes = retail's size). Layout: g_BattleCurWorkBlock->slotStates
// [g_BattleCurSlotIndex] = 2, then slotAmounts[g_BattleCurSlotIndex] = the row's
// power * the slot state's maxHp / 20. Only the tail differs, and only by register:
// retail loads the second g_BattleCurSlotIndex into $v1 and the block base into $v0,
// then does the index-first addu and mfhi/srl IN PLACE in $v0; 2.7.2 always loads
// the index into $v0 and the base into $v1 and leaves mfhi in $a1 with the shift in
// $v1. PRESET_260: 100.00% under gcc-2.6.0 (same md difference as func_8009A9D0);
// the unit cannot move - see that function's note for the screen.
// Round 195 type-width sweep, all 97.33% / 120 bytes with the identical tail: the
// typed form g_BattleCurWorkBlock->slotStates[g_BattleCurSlotIndex] (97.33, its
// addu is index-first like retail's, so it differs in more registers than this
// body), idx as s32 / u32 / u16 / u8 local, maxHp spelled
// ((BattleSlotState*)g_BattleCurSlotState)->maxHp or *(s32*)(...+0x64), and the
// destination as *(s32*)((u8*)block + 0x5F6C + idx * 4). Worse: a pointer local for
// slotAmounts (81.47, 112 bytes, CSEs the base into a saved reg), the amount in a
// u32/s32 local (43.33 - it moves the divide above the sb). The index always lands
// in $v0, so the register roles are not source-controllable on 2.7.2.
// The 98.33 came from splitting the two statements' base loads (a u8* local for the
// first store, a plain expression for the second) and the int idx local that stops
// the second lbu being CSE'd into the first. Permuter score 50 over 1907+1250
// iterations. Also tried: two separate locals for idx and address, declaration
// order swapped, u8* address local, u32 quotient local, 20u divisor, u32*
// destination pointer local (70.17).
// Round 306 re-measured it: the fully typed body
// (g_BattleCurWorkBlock->slotAmounts[idx] = (u32)(power * maxHp) / 20) is 97.33%
// with the identical tail swap, and a BattleWorkBlock* local for the store coalesces
// away (97.33, no change), so the park below is still the best.
// PARKED (98.33%).
/*
void func_8009BD94(void) {
    int new_var;
    u8* new_var2;

    new_var2 = (u8*)g_BattleCurWorkBlock;
    *(u8*)(new_var2 + g_BattleCurSlotIndex + 0x5FA0) = 2;
    new_var = g_BattleCurSlotIndex;
    new_var = (u32)g_BattleCurWorkBlock + new_var * 4 - -0x5F6C;
    *(u32*)new_var =
        (u32)(g_BattleCurrentRow->power * *(s32*)(g_BattleCurSlotState + 0x64)) / 20;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19_r1", func_8009BD94);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19_r1", func_8009BE0C);
// PARKED (90.12%, 552 bytes = retail's exact size, instruction multiset equal
// apart from the three ties below). Body COMPILES.
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
