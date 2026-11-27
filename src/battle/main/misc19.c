#include "common.h"
#include "battle/actor.h"
#include "main/game.h"

// The battle work block's per-actor gear action-row table: 0x2A rows of 0x28 bytes
// per actor at +0x2228.
#define BATTLE_GEAR_ROW_TABLE 0x2228
#define BATTLE_GEAR_ROW_STRIDE 0x690
// The action work record the current action is built in, inside the block's work area.
#define BATTLE_ACTION_WORK_TABLE 0x5F24

extern BattleWorkBlock* g_BattleCurWorkBlock;
extern BattleTableEntry g_BattleWorkBlock[];

extern int rand(void);

s32 BattleGetActorConditionCode(u8 arg0) {
    BattleTableEntry* p = (BattleTableEntry*)(arg0 * sizeof(BattleTableEntry) + (u32)g_BattleCurWorkBlock);
    u16 f7C = p->activeEffectFlags1;
    u16 f80;

    if (f7C & 0x4000) {
        return 8;
    }
    if (f7C & 0x8000) {
        return 1;
    }
    if (f7C & 0x2000) {
        return 2;
    }
    f80 = p->activeEffectFlags2;
    if (f80 & 0x1000) {
        return 3;
    }
    if (f80 & 0x2000) {
        return 4;
    }
    if (f7C & 0x800) {
        return 5;
    }
    if (f7C & 0x1000) {
        return 6;
    }
    if (f7C & 0x2) {
        return 3;
    }
    if ((p->activeEffectFlags3 | p->accessoryEffectFlags3) & 0x8000) {
        return 7;
    }
    return p->hp < (p->maxHp >> 3) ? 5 : 0;
}
u16 FieldScriptClearAnimationOverrideIfFlag16(u8 arg0) {
    BattleTableEntry* p = (BattleTableEntry*)(arg0 * sizeof(BattleTableEntry) + (u32)g_BattleCurWorkBlock);
    u16 f7C = p->activeEffectFlags1;
    u16 ret = 0;
    u16 f;

    if (f7C & 0x8000) {
        return 0;
    }
    f = p->activeEffectFlags2;
    if (f & 0x1000) {
        ret |= 0x8000;
    }
    if (f & 0x2000) {
        ret |= 0x4000;
    }
    if (f7C & 0x800) {
        ret |= 0x2000;
    }
    f = p->activeEffectFlags7 | p->accessoryEffectFlags7;
    if (f & 0xF00) {
        ret |= (f & 0xF00) >> 3;
    }
    if (f & 0xF000) {
        ret |= (f & 0xF000) >> 3;
    }
    return ret;
}
// PARKED (99.68%, 124 bytes = retail's size, 31 insns). Body COMPILES; re-measured round 418.
// The residual is ONE instruction pair at the sum. Retail's tail is
//   lbu v0,20(v1) / lbu v1,91(a2) / addu v0,v0,v1 / move v1,v0 / andi v0,v1,0xff / sltiu v0,v0,0x65
// i.e. the sum is IN PLACE in the DYING p-byte register v0 and the u8 `value` copy takes the
// freed q-byte register v1. This cc1 gives the add a fresh `a0` and copies from there
// (addu a0,v0,v1 / move v1,a0); the two loads, the copy, the clamp and the andi return all match.
// Round 418 read the `-da` RTL dump and named the mechanism with its pseudo numbers:
//   insn 60  (reg/v:HI 87)  = zero_extend:HI (mem QI p+0x14)   -> 87 in $2 (v0)
//   insn 64  (reg:HI 104)   = zero_extend:HI (mem QI q+0x5B)   -> 104 in $3 (v1)
//   insn 66  (reg:SI 105)   = plus:SI (subreg 87) (subreg 104) -> 105 in $4 (a0)  <- the residual
//   insn 71  (reg/v:QI 88)  = subreg:QI (105)                  -> 88 in $3 (v1), the surviving move
// Retail needs 105 in $2 (the add in place on the dying p load). With the plain clamp, combine FOLDS
// the copy into the add - (set (subreg:SI (reg/v:QI 88)) (plus:SI ...)) - one insn, no move, 120
// bytes / 95.97 (re-measured). The move exists here only because the sum has a second use (the
// `sum || sum` arm test); that use is in the NEXT block, so 87/105 become global allocnos and
// global_alloc's priority order (88 first, then 105) hands 105 $4 while $2 is held by 87, whose live
// range overlaps 105's def: a 3-operand SI add whose output is a fresh allocno can never share the
// dying input's register. The two horns are structural, not spelling:
//   - no second use -> combine folds and the move is gone (120 bytes, 95.97)
//   - second use in another block -> the move survives, the add leaves v0 (124 bytes, 99.68)
// A second use in the SAME block would keep the sum local and local-alloc DOES tie output to the
// dying input (that is why the 120-byte body is in place on the p load), but nothing keeps such a
// use: a dead `sum = 0;` after the copy is deleted before combine (re-measured: still 120 / 95.97).
// The SImode family is the other horn: `s32 sum` DOES tie (addu a1,a1,v0 + move v1,a1, so the copy
// survives and value keeps v1) but shifts BOTH loads up to a1/v0 - 99.03% at the same 124 bytes
// (re-measured round 418).
// Measured in round 405, all at 124 bytes: s32/u16 sums, `+=`, (short)/(s32) casts on the summands,
// a `short b` summand, a brace block, `register short`, declaration initialisers, q-first and
// commuted sums, u8 byte locals (98.39), volatile p read (98.39), volatile q read (99.19, addu
// a1,v0,a1), s16-via-s32-temp (98.39). Every plain HImode spelling lands on the identical 99.68
// pair. Presets are not the way out: this body 99.68 / 95.97 (2.6.0 / cdk), the s32 body 99.03 /
// 95.97, and the unit is mixed under 2.6.0 (see the BattleResetRowIndexAndMaybeRefreshStats note). Fifth and sixth permuter
// runs: PERMUTER_NONE base=10 best=10 (1587 and 1429 iterations). Seventh run (round 546): PERMUTER_NONE
// base=10 best=10 (1109 iterations). Round 546 also measured: `value = p[0x14]; value += q[0x5B];` (same-mode
// QI copies, so combine_regs coalesces them) = 95.97 / 120 bytes, no move at all; `s32 sum` with the sum in ONE
// expression (rather than two statements) = 95.97 too, so the s32 tie needs the split statements; and the typed
// spelling (`BattleRow* pRow = (BattleRow*)((u8*)g_BattleCurWorkBlock + 0x1058 + arg0 * 0x5F0)` plus
// `pEntry->ether`) = 69.03 / 124 bytes: declaring the two pointers in one statement pair hoists the
// g_BattleCurWorkBlock load and re-hashes the whole entry block, so the raw `s32 off` spelling below is
// load-bearing. The two fields are (include/battle/actor.h): BattleRow.chanceBonus at the row's +0x14 (the
// per-actor row table at +0x1058, 0x5F0 per actor, 0x28 per row) and BattleTableEntry.ether at +0x5B, summed
// and clamped at 100 -> the acting row's chance percent.
/*
u8 func_8009A258(u8 arg0, u8 arg1) {
    s32 off = arg0 * 0x5F0 + 0x1058;
    u8* p = (u8*)off + (u32)g_BattleCurWorkBlock;
    u8* q;
    short sum;
    u8 value;

    p = p + arg1 * 0x28;
    q = (u8*)(arg0 * 0x170) + (u32)g_BattleCurWorkBlock;
    sum = p[0x14];
    sum = sum + q[0x5B];
    value = sum;
    if (value > 0x64) {
        if (sum || sum) {
            value = 0x64;
        } else {
            value = 0x64;
        }
    }
    return value;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009A258);

// The action work record the battle loop reads back at +0x5F24 of the battle work
// block: what the acting slot's action costs, hits for and deals.
typedef struct {
    /* 0x00 */ u16 rowCosts[15]; // the commandPoints cost of each of the actor's gear action rows
    /* 0x1E */ u16 attackAmount; // the slot's attack figure: two stats times the weapon's attack
    /* 0x20 */ u8 unk20[4];
    /* 0x24 */ u16 flags; // the state's active effect bits and the low-hp/evade conditions
    /* 0x26 */ u16 cost; // the action's cost, scaled by the state code it starts from
    /* 0x28 */ u8 stateCode; // the slot state code the action started from
    /* 0x29 */ u8 unk29; // the slot state's agility
    /* 0x2A */ u8 unk2A; // the slot state's penaltyPercent
    /* 0x2B */ u8 unk2B;
    /* 0x2C */ u8 unk2C;
    /* 0x2D */ u8 unk2D;
    /* 0x2E */ u16 amount; // what the action deals
} BattleActionWork; // Size: 0x30

// PARKED (99.28% after round 541's nudge below; 99.19% without it; 1256 bytes; retail is 1252 = 313 instructions - the old note's "1248" was
// wrong). Body COMPILES. Structure is retail's: the pointer set (pEntry/pState/pCondition/
// pGearRow/pWork), the 15-row cost copy loop, the state-code switch (a `switch` reproduces
// retail's test chain where an if/else-if chain does not), and the tails. Load-bearing shapes:
// `u8 amount` (a u32 makes gcc coalesce the division result and lose retail's `move s1,v1`),
// the split `attack = pState->engineOutput; attack = attack * pState->attack;`, a `u16` local
// for the activeEffectFlags2 test, and a SECOND pCondition pointer (`new_var2`) for the tail
// blocks, which is what fixed retail's sp.N regs (94 -> 98.85).
// Round 372 found the missing load and the extra one:
//  - retail loads 0x57(s0) (statBonuses[7]) TWICE, once for the != 0 test and once for the
//    product; the second load comes back only through a volatile read at the multiply site:
//    pWork->cost = *(volatile u8*)&pState->statBonuses[7] * pWork->rowCosts[0];  (the 98.85
//    body CSEs it and is one load short)
//  - with the reload in, the body is exactly ONE instruction over retail, and that instruction is
//    the `move a2,s2` copy of `new_var2 = pCondition;`. Spelling new_var2 as its own address
//    expression with a declaration initialiser (u8* new_var2 = (u8*)&g_BattleWorkBlock[arg0] +
//    0x148;) drops the copy and lands on retail's exact 1252 bytes, but new_var2 then takes $t0
//    where retail reuses $s2 (98.21%). Using pCondition alone in the tail blocks is 96.59% at
//    1248 (an extra `andi` appears on the *pCondition == 4 test). So (reload, no copy) is the
//    one combination this body does not reach.
// Remaining, and capping: the three mfhi/mflo destinations - retail `mult / mflo v0` and
// `mfhi v0 / srl v0,v0,3`, this compiler `mflo $tN / srl v0,$tN,3` - the same 2.7.2
// division-expansion difference that caps BattleResetRowIndexAndMaybeRefreshStats/BD94/B46C; try_flags tops out at 99.74
// under gcc 2.6.0. Plus four register-role swaps around those mflo pairs, and the maxHp==hp
// `amount++` block being stolen into the `beq` delay slot.
// The three g_BattleWorkBlock+0x162/+0x32 condition-timer sites print as g_BattleWorkBlock+354
// against retail's D_800CCE4A / D_800CCD1A: same linked bytes (the documented RELOC case).
// Round 393 re-measured this body as-is: 99.19% (314 insns / 1256 bytes against retail 313 /
// 1252; the extra instruction is `move a2,s2`), residual = three mfhi/mflo destination sites
// plus four register-role swaps chained off them. try_flags on this body: 99.52% under
// gcc-2.6.0 (even a 2.6.0 carve-out would not close it), 91.31% under gcc-2.7.2-cdk. Swapping
// the operands of the statBonuses[7] * rowCosts[0] product is WORSE: 99.15%, 119 differing lines.
// Round 414 measured the third combination and re-confirmed the other two: the copy of
// pCondition declared (`u8* new_var2 = pCondition;`) instead of assigned mid-body gives retail's
// exact 313 insns / 1252 bytes but 98.21% (new_var2 takes `$t0` and `move t0,s2` moves into the
// entry block, which frees the `beq` slot so retail's stolen `amount++` is no longer stolen -
// 93 differing lines of register roles downstream); dropping the second pointer entirely is
// 96.59% / 312 insns / 1248 bytes. So the three sizes bracket retail's and none of them is it;
// the residual that caps all of them is the md: retail `mflo v0` / `mfhi v0 / srl v0,v0,3`,
// this compiler `mflo $tN / srl v0,$tN,3` (same family as BattleResetRowIndexAndMaybeRefreshStats).

// Round 427 NEW measurement, worth keeping: the SECOND pointer is what costs the extra
// instruction, and dropping it completely (`new_var2` removed, every tail read through
// pCondition) gives retail's EXACT size - 313 insns / 1252 bytes - under gcc-2.6.0 at
// 99.74% (under the unit's 2.7.2 it is 96.59% / 1248, the old 312-insn figure). Its three
// remaining 2.6.0 statics are ENCODING-ONLY commutative operand swaps with retail's
// registers already correct, so no C spelling reaches them:
//   retail `mult v0,v1`   -> 2.6.0 `mult v1,v0`      (loads lbu v0,87(s0) / lhu v1,0(a1) match)
//   retail `addu a0,v0,v1`-> 2.6.0 `addu a0,v1,v0`   (attack += weaponData[0].attack)
//   retail `addu v0,v0,a0`-> 2.6.0 `addu v1,v0,a0`   (attack += weaponData[2].attack)
// Measured this round with no effect on any of them: swapping the source operands of the
// `statBonuses[7] * rowCosts[0]` product (cluster AND mult unchanged), swapping
// `attack + weaponData[0].attack` to weapon-first (output byte-identical), and storing the
// second sum straight with `pWork->attackAmount = attack + weaponData[2].attack;` (that one
// fixes the destination but flips the earlier product's load cluster: 21 differing lines).
// So a 2.6.0 carve-out of this run (0x2A7E4..0x2ACC8) buys 99.74, not a match; the extra
// `move a2,s2` of the second-pointer body costs exactly one instruction but that body is
// the one that is 99.19% under the unit's preset, so it is the one parked below.
//
// Round 541 AUDIT: every percentage above came from a STALE object - the park did not
// compile. `g_BattleWorkBlock` had no declaration in this unit (the only `extern
// BattleTableEntry g_BattleWorkBlock[];` lives in misc13.c), so cc1 rejected the body and
// diff_func.sh scored the previous .o. The declaration is now at the top of this file; the
// body compiles and re-measured 99.19% / 1256 bytes / 314 insns (retail 313 / 1252), as above.
// NOTE the else arm's `amount++` reads `amount` uninitialised and retail does the same (its
// `addiu s1,s1,1` on the maxHp == hp path follows no write to $s1), so that UB is the
// original's, not the park's.
// Round 541 NEW, worth keeping: a constant pseudo assigned right BEFORE the amount product
// and used as the case-3 arm's mask (`s32 new_var3;` ... `new_var3 = 0x40;` then
// `if (accessoryEffectFlags5 & new_var3)`) moves 117 -> 109 differing lines, 99.19 -> 99.28%
// at the same 1256 bytes: the product's load cluster becomes retail's exactly
// (`lbu v0,84(a3)` / `andi v1,s1,0xff` / `addiu v0,v0,5` / `mult v1,v0`). It is an ALLOCATOR
// NUDGE, not retail's source - retail masks with the immediate (`andi v0,v0,0x40`) as well, and
// the pseudo is just live across the `rand()` call, which reorders the hand-out. The POSITION is
// the whole effect: as a declaration initialiser at the top of the body it is 99.19 again.
// Permuter from the 99.28 body: base=245 best=245 (985 iterations), after the run that found
// the nudge (275 -> 245, 1068 iterations).
// Also re-measured this round: dropping the second pointer is 96.59% / 1248 bytes / 312 insns
// (unchanged by the nudge), and spelling the whole tail through new_var2 so pCondition dies at
// the head is 96.65% / 1256 (gcc CSEs the two and the hand-out rotates).
/*
void func_8009A2D4(u8 arg0) {
    BattleTableEntry* pEntry = &g_BattleWorkBlock[arg0];
    BattleSlotState* pState = (BattleSlotState*)((u8*)&g_BattleWorkBlock[arg0] + 0xA4);
    u8* pCondition = (u8*)&g_BattleWorkBlock[arg0] + 0x148;
    BattleRow* pGearRow =
        (BattleRow*)(arg0 * BATTLE_GEAR_ROW_STRIDE + BATTLE_GEAR_ROW_TABLE + (u32)g_BattleWorkBlock);
    BattleActionWork* pWork = (BattleActionWork*)((u8*)g_BattleWorkBlock + BATTLE_ACTION_WORK_TABLE);
    u16 new_var;
    s32 new_var3;
    u8* new_var2;
    u32 quotient;
    u_char i;
    u8 amount;
    u16 attack;

    for (i = 0; i < 15; i++) {
        pWork->rowCosts[i] = pGearRow->commandPointCost;
        pGearRow++;
    }
    if (pState->statBonuses[7] != 0) {
        pWork->cost = *(volatile u8*)&pState->statBonuses[7] * pWork->rowCosts[0];
    } else {
        pWork->cost = 0x1E;
    }
    switch (*pCondition) {
    case 1:
    case 2:
    case 3:
        pWork->cost += *pCondition * 20;
        break;
    case 4:
        pWork->cost = pWork->cost * 10;
        break;
    }
    attack = pState->engineOutput;
    attack = attack * pState->attack;
    pWork->attackAmount = attack;
    attack = attack + pState->weaponData[0].attack;
    pWork->attackAmount = attack;
    if (pEntry->characterId == 4) {
        attack = attack + pState->weaponData[2].attack;
        pWork->attackAmount = attack;
    }
    new_var2 = pCondition;
    pWork->unk29 = pState->agility;
    pWork->unk2A = pState->penaltyPercent;
    if (pState->maxHp != pState->hp) {
        quotient = (pState->maxHp - pState->hp) / (pState->maxHp / 10);
        amount = quotient;
        if ((amount & 0xFF) == 0) {
            amount = quotient + 1;
        }
    } else {
        amount++;
    }
    new_var3 = 0x40;
    amount = (amount & 0xFF) * (*(u8*)&pEntry->unk54 + 5);
    if ((g_GameState.unk22B6 & 0x4000) == 0) {
        amount = 0;
    }
    if (*(u8*)&pEntry->commandPoints < 0x32) {
        amount = 0;
    }
    if (pEntry->gearId == 3) {
        amount = 0;
    }
    if (pEntry->gearId == 0xF) {
        amount = 0x63;
    }
    if ((amount & 0xFF) > 0x63) {
        amount = 0x63;
    }
    pWork->amount = amount & 0xFF;
    pWork->flags = 0;
    if (pState->activeEffectFlags1 & 0x100) {
        pWork->flags |= 0x8000;
    }
    if (pState->activeEffectFlags1 & 0x200) {
        pWork->flags |= 0x4000;
    }
    if (pState->activeEffectFlags1 & 0x80) {
        pWork->flags |= 0x2000;
    }
    if (pState->activeEffectFlags1 & 0x10) {
        pWork->flags |= 0x1000;
    }
    if (*(u16*)((u8*)pState + 0x38) < (*(u16*)((u8*)pState + 0x3A) >> 3)) {
        pWork->flags |= 0x800;
    } else {
        pWork->flags &= 0xF7FF;
    }
    if (*(u16*)((u8*)pState + 0x38) == 0) {
        pState->activeEffectFlags2 &= 0x7FFF;
        pEntry->activeEffectFlags3 &= 0x7FFF;
    }
    new_var = pState->activeEffectFlags2;
    if (new_var & 0x8000) {
        pWork->unk2C = 1;
    } else {
        pWork->unk2C = 0;
    }
    if (*new_var2 == 4) {
        pWork->stateCode = 4;
        if (--g_BattleWorkBlock[arg0].state.conditionTimers[7] == 0) {
            pState->activeEffectFlags2 &= 0xBFFF;
            *new_var2 = 0;
            *(u8*)&pEntry->unk54 = 0;
        }
    } else {
        pWork->stateCode = *pCondition;
        if (*pCondition == 3 && (g_GameState.unk22B6 & 0x4000) != 0) {
            if (rand() % 100 < (amount & 0xFF)) {
                pState->activeEffectFlags2 |= 0x4000;
                g_BattleWorkBlock[arg0].state.conditionTimers[7] = 3;
                if (g_BattleWorkBlock[arg0].accessoryEffectFlags5 & new_var3) {
                    g_BattleWorkBlock[arg0].state.conditionTimers[7] = 6;
                }
                (*pCondition)++;
            }
        }
    }
}*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009A2D4);





u8 BattleGetGameCharacterField5(u8 arg0) {
    return g_GameState.characters[arg0].slots[0].unk5;
}

// One 0x10-byte record the battle work block keeps at +0x5818, one per slot,
// holding the slot's character id at +0x6 (the same position the party list in
// g_GameState.characters[4] keeps it).
typedef struct {
    /* 0x0 */ u8 unk0[6];
    /* 0x6 */ u8 characterId;
    /* 0x7 */ u8 unk7[9];
} BattlePartyRecord; // Size: 0x10

// The battle work block's working copy of the party list, one record per slot at
// +0x5818.
typedef struct {
    /* 0x0000 */ u8 unk0000[0x5818];
    /* 0x5818 */ BattlePartyRecord party[BATTLE_SLOT_COUNT];
} BattleWorkBlockParty;

s32 BattleIsCharacterInParty(u8 arg0) {
    BattlePartyRecord* pRecord = &((BattleWorkBlockParty*)g_BattleCurWorkBlock)->party[arg0];
    u8 id = pRecord->characterId;

    s32 ret;

    if (g_GameState.characters[4].slots[0].characterId == id) {
        ret = TRUE;
    } else if (g_GameState.characters[4].slots[1].characterId == id) {
        ret = TRUE;
    } else if (g_GameState.characters[4].slots[2].characterId == id) {
        ret = TRUE;
    } else {
        ret = g_GameState.characters[4].slots[3].characterId == id;
    }
    return ret;
}

// Fills battle party slot record `row` of g_GameState.characters[4] (the party the battle
// is fought with) from the work block's own copy at +0x54F8, then copies the same three
// condition bytes into every block slot whose character is 4 (the four-weapon character),
// which is what gives that character's guns their conditions.
// Load-bearing shapes, all measured: the store addresses must be written as MEMBERS of the
// g_GameState struct (`characters[4].slots[row].attack`), which is what lets gcc hand maspsx
// the `sym+const(reg)` operand that becomes retail's `lui $at / addu $at,$at,$reg / sb
// %lo(sym+const)($at)`; a `((u8*)&g_GameState)[N]` spelling CSEs one base register and is
// 76.74%. The p offset in `off` is its own statement or gcc moves the 0x54F8 addend onto the
// block pointer (`addiu` on the wrong register), and `q` is spelled index-first or the `addu`
// operands swap. `i = 0` sits before the fourth test and `i++` leads the loop body: gcc sinks
// a loop-initialiser written anywhere else down to the loop, and these two positions are
// retail's fourth-bne delay slot (`addu $a3,zero,zero`) and its loop body head.
void BattleFillPartySlotWeaponRecord(u8 arg0, u8 arg1) {
    s32 off;
    u8* p;
    u8 id;
    u8 row;
    u8 i;

    off = arg0 * 0x10 + 0x54F8;
    p = (u8*)off + (u32)g_BattleCurWorkBlock;
    id = p[6];
    row = arg1;

    if (g_GameState.characters[4].slots[0].characterId == id) {
        row = 0;
    }
    if (g_GameState.characters[4].slots[1].characterId == id) {
        row = 1;
    }
    if (g_GameState.characters[4].slots[2].characterId == id) {
        row = 2;
    }
    i = 0;
    if (g_GameState.characters[4].slots[3].characterId == id) {
        row = 3;
    }
    g_GameState.characters[4].slots[row].attack = p[0xC];
    g_GameState.characters[4].slots[row].conditionKind = p[0xB];
    g_GameState.characters[4].slots[row].chance = p[0xA];
    g_GameState.characters[4].slots[row].conditionKind = p[0xB];
    g_GameState.characters[4].extraWeaponIds[row] = arg0;
    g_GameState.gearPartsIds[0x6C + arg0] = p[3];
    g_GameState.characters[4].extraWeaponIds[row] = arg0;
    for (; i < 3; ) {
        BattleTableEntry* q = (BattleTableEntry*)(i * 0x170 + (u32)g_BattleCurWorkBlock);

        i++;
        if (q->characterId == 4) {
            GameCharacterSlot* pSlot = (GameCharacterSlot*)((u8*)q + row * 8);

            pSlot->attack = p[0xC];
            pSlot->conditionKind = p[0xB];
            pSlot->chance = p[0xA];
            pSlot->conditionKind = p[0xB];
            q->extraWeaponIds[row] = arg0;
        }
    }
}
