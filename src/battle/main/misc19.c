#include "common.h"
#include "battle/actor.h"
#include "main/game.h"

// The battle work block's per-actor action-row tables: 0x26 rows of 0x28 bytes per
// actor at +0x1058 and 0x2A rows per actor at +0x2228, three actors each.
#define BATTLE_ACTOR_ROW_TABLE 0x1058
#define BATTLE_ACTOR_ROW_STRIDE 0x5F0
#define BATTLE_GEAR_ROW_TABLE 0x2228
#define BATTLE_GEAR_ROW_STRIDE 0x690
// The action work record the current action is built in, inside the block's work area.
#define BATTLE_ACTION_WORK_TABLE 0x5F24

// The evade and hit percents BattleResetActorStats puts a slot's stats back to.
#define BATTLE_STAT_EVADE_RESET 20
#define BATTLE_STAT_HIT_RESET 15

// The commandMask of a character who has every one of the 13 learnable command rows.
#define BATTLE_COMMAND_MASK_ALL 0xFFF8


extern BattleWorkBlock* g_BattleCurWorkBlock;
extern u8 g_BattleCurSlotIndex;
extern BattleRow* g_BattleCurrentRow;
extern u8* g_BattleCurSlotState;
extern u8 g_BattleSlotConditions[];
extern BattleTableEntry g_BattleWorkBlock[];
extern u8 g_BattleCommandCharIndex;
extern BattleTableEntry* g_BattleActingSlot;
extern u8 D_800CCD68[];
extern u8 g_BattleActorCharIds[];

extern int rand(void);
extern void func_8009BE0C(void);
extern void BattleSetActorScaledHp(u8 arg0, BattleTableEntry* pArg);

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
// Round 373 re-measured: ONE instruction differs, `addu a0,v0,v1` against retail's in-place
// `addu v0,v0,v1` (the HImode sum coalescing with its FIRST operand where gcc picks a fresh
// register; the `move v1,..`, the clamp and the `andi` return all match). Probed as a
// standalone snippet the sum coalesces with its SECOND operand instead (`addu $3,$2,$3`), so
// the probe is not representative of the frame. 99.68 under gcc-2.6.0 too, so this body is not
// a preset question either. Five spellings with no effect: u16/s32/u8 sums, declaration order
// swapped, the add written `q[0x5B] + sum`.
// PARKED (99.68%, 124 bytes = retail's size). Body COMPILES. Re-measured round 369:
// unchanged, and the whole residual is ONE instruction pair. Retail's `rand`-free tail is
// `lbu v0,p[0x14] / lbu v1,q[0x5B] / addu v0,v0,v1 / move v1,v0` - the sum coalesces onto the
// DYING p-byte register v0 - while this compiler gives the sum a fresh a0 and copies from
// there. So the loads, the `move v1,v0` (the u8 `value` taking the freed q-byte register), the
// `u8` clamp test and the `andi` return all match; only sum's home differs. The one-expression
// `sum = p[0x14] + q[0x5B]` DOES coalesce (98.39%) but onto the wrong operand's register.
// Four permuter runs across rounds 122/123/169/369 (best 10-40) and 20+ hand shapes have not
// moved it: it is the allocator's choice between two free registers, not a source shape.
// Round 372 narrowed the direction: an UNSIGNED HImode sum (`u16`/`u32`/`s32`, same `u8 value`,
// same clamp) DOES coalesce the add onto the p byte's register (`addu a1,a1,v0` + `move v1,a1`,
// so the copy survives and value still takes v1), but then both byte loads move to a1/v0:
// 99.03% at the same 124 bytes. `s16` is what leaves the loads in retail's v0/v1 and loses only
// the in-place tie, so 99.68 stays the best. Measured dead this round: u8 sum (120 bytes - the
// copy and the bnez nop both disappear), one-expression sums through u8/s16/u16 temps, a
// declaration initialiser, a brace block around the sum, load order swapped, `short value`, and
// comparing `sum` instead of `value`.
// Round 391 re-measured (un-parked, live): still 99.68% at 124 bytes with exactly two instructions
// out of place (the add's destination `a0` where retail is in place in `v0`, and the copy that
// reads it). No lever names this shape: a-three-operand-or-never-ties says a 3-operand `addu`
// never ties to its input, which is why no spelling reaches retail's in-place form here.
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

INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009A2D4);

// PARKED (99.19%, 1256 bytes; retail is 1252 = 313 instructions - the old note's "1248" was
// wrong). Body COMPILES. Structure is retail's: the pointer set (pEntry/pState/pCondition/
// pGearRow/pWork), the 15-row cost copy loop, the state-code switch (a `switch` reproduces
// retail's test chain where an if/else-if chain does not), and the tails. Load-bearing shapes:
// `u8 amount` (a u32 makes gcc coalesce the division result and lose retail's `move s1,v1`),
// the split `attack = pState->engineOutput; attack = attack * pState->unk3F;`, a `u16` local
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
// division-expansion difference that caps func_8009A9D0/BD94/B46C; try_flags tops out at 99.74
// under gcc 2.6.0. Plus four register-role swaps around those mflo pairs, and the maxHp==hp
// `amount++` block being stolen into the `beq` delay slot.
// The three g_BattleWorkBlock+0x162/+0x32 condition-timer sites print as g_BattleWorkBlock+354
// against retail's D_800CCE4A / D_800CCD1A: same linked bytes (the documented RELOC case).
// Round 393 re-measured this body as-is: 99.19% (314 insns / 1256 bytes against retail 313 /
// 1252; the extra instruction is `move a2,s2`), residual = three mfhi/mflo destination sites
// plus four register-role swaps chained off them. try_flags on this body: 99.52% under
// gcc-2.6.0 (even a 2.6.0 carve-out would not close it), 91.31% under gcc-2.7.2-cdk. Swapping
// the operands of the statBonuses[7] * rowCosts[0] product is WORSE: 99.15%, 119 differing lines.
/*
void func_8009A2D4(u8 arg0) {
    BattleTableEntry* pEntry = &g_BattleWorkBlock[arg0];
    BattleSlotState* pState = (BattleSlotState*)((u8*)&g_BattleWorkBlock[arg0] + 0xA4);
    u8* pCondition = (u8*)&g_BattleWorkBlock[arg0] + 0x148;
    BattleRow* pGearRow =
        (BattleRow*)(arg0 * BATTLE_GEAR_ROW_STRIDE + BATTLE_GEAR_ROW_TABLE + (u32)g_BattleWorkBlock);
    BattleActionWork* pWork = (BattleActionWork*)((u8*)g_BattleWorkBlock + BATTLE_ACTION_WORK_TABLE);
    u16 new_var;
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
    attack = attack * pState->unk3F;
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
                if (g_BattleWorkBlock[arg0].accessoryEffectFlags5 & 0x40) {
                    g_BattleWorkBlock[arg0].state.conditionTimers[7] = 6;
                }
                (*pCondition)++;
            }
        }
    }
}
*/


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

// Round 373: the tie is PROVEN a compiler difference, not a source shape. One snippet with
// six spellings (`rand() % 100` inline, into a local, immediate compare, two returns, a store
// before the call, misc18's own body verbatim) compiles to `mfhi $temp / sra $dest,$temp,5`
// under 2.7.2 and to retail's in-place `mfhi $dest / sra $dest,$dest,5` under 2.6.0 - every
// spelling, both compilers (tools/scripts/cc1_probe.sh ... --gcc gcc-2.6.0-psx). battle/misc18
// is a XenoBattle 2.6.0 unit, so its 100.00% is not evidence that 2.7.2 can reach this shape.
// best 99.66% on the unit's 2.7.2, 100.00% under gcc-2.6.0: the mfhi temp of the
// `% 100` expansion is $a0 there and $a1 here, so retail's in-place
// `mfhi a0 / sra a0,a0,5` becomes `mfhi a1 / sra a0,a1,5`. The unit cannot move to
// 2.6.0 (BattleClearSlotFlag15A1 95.71, BattleSetSlotFlag15A1 99.57 under it).
// Shapes with no effect on the tie: `rand() % 100 < 50` inline vs `int r` local,
// `50 > rand() % 100`, `rand() % 100 <= 49`, `(s32)(rand() % 100) < 50`; the
// permuter found nothing either.
// PARKED (99.66%). Body COMPILES (park triage, round 111); 100.00% only under
// gcc-2.6.0, which the unit cannot move to. The gap is one register: gcc copies
// mfhi into $a1 and shifts into $a0 where retail mfhi's into $a0 and shifts in
// place. Two permuter runs found nothing.
/* PRESET_260: 100.00% under gcc-2.6.0 at every -O and -G (tools/scripts/try_flags.sh
   battle/main/misc19 func_8009A9D0 --sweep); 99.66% under the unit's 2.7.2 at every
   -O and -G. Whole-unit screen under 2.6.0: 100.00 BattleGetActorConditionCode,
   FieldScriptClearAnimationOverrideIfFlag16, BattleGetGameCharacterField5,
   BattleIsCharacterInParty, BattleResetActorStats, BattleSetActorScaledHp,
   BattleGetActorActionConditionMask, func_8009A9D0; below 100 BattleSetSlotFlag15A1
   99.57, BattleClearSlotFlag15A1 95.71, BattleSetSlotEntryFieldsTo1 99.71,
   BattleSetSlotEntryFieldsTo2000 99.77. So the unit is mixed; this one function is a
   contiguous run of its own and could be carved into its own .c on the XenoLibrary
   preset. The gap under 2.7.2 is one register: gcc 2.7.2 copies the mfhi result to
   $a1 and shifts into $a0, retail mfhi's into $a0 and shifts in place (a
   smulsi3_highpart md difference, not source-controllable). Shapes tried with no
   effect: inline `rand() % 100` vs an `s32 r` local for the rand() result,
   `50 > rand() % 100`, `rand() % 100 <= 49`, `(s32)(rand() % 100) < 50`; two
   permuter runs found nothing (third run, round 112: PERMUTER_NONE base=10
   best=10, 899 iterations).
   PARKED (99.66%). */
// Round 372 re-measured: 99.66% (116 bytes), still the single mfhi/sra pair, and 100.00% under
// gcc-2.6.0 confirmed. The whole-unit 2.6.0 screen is still mixed (BattleClearSlotFlag15A1
// 95.71, BattleFillPartySlotWeaponRecord 96.15, BattleSetSlotFields148And162 99.05,
// BattleResetSlotEffectWords 99.64), so this stays a one-function run that wants carving out.
// BattleSetSlotActionStateAndAmount is the same story (98.33% here, 100.00 under 2.6.0).
// Round 393 re-measured: 99.66% (116 bytes, 29/29 insns), still only the mfhi a1 + sra a0,a1,5
// pair; 100.00% under gcc-2.6.0 and 99.66% under cdk confirmed. Whole-unit 2.6.0 screen (round 393,
// try_flags --all): 100.00 BattleGetActorActionConditionMask, BattleGetActorConditionCode,
// BattleGetGameCharacterField5, BattleIsCharacterInParty, BattleResetActorStats,
// BattleSetActorScaledHp, FieldScriptClearAnimationOverrideIfFlag16, func_8009A9D0; below 100
// BattleSetCommandCodeForSlotKindFlag 99.96, BattleDecrementGearPartUse 99.79,
// BattleSetSlotEntryFieldsTo2000 99.77, BattleSetSlotEntryFieldsTo1 99.71,
// BattleResetSlotEffectWords 99.64, BattleSetSlotFlag15A1 99.57, func_8009A2D4 99.52,
// BattleInitItemInventoryAndConditionMasks 99.38, BattleSetSlotFields148And162 99.05,
// BattleFillPartySlotWeaponRecord 96.15, BattleClearSlotFlag15A1 95.71. So this one function needs
// a one-function carve-out on the XenoLibrary preset, not a unit switch.
/*
s32 func_8009A9D0(void) {
    g_BattleCurWorkBlock->rowIndex = 0;
    if (rand() % 100 < 50) {
        func_8009BE0C();
        return 1;
    }
    return 0;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009A9D0);


void BattleSetSlotFlag15A1(u8 arg0) {
    BattleTableEntry* p;
    BattleTableEntry* q;

    g_BattleCurWorkBlock->rowIndex = 0;
    q = &g_BattleCurWorkBlock->slots[arg0];
    q->state.flags |= 1;
    p = &g_BattleCurWorkBlock->slots[arg0];
    if ((p->state.flags & BATTLE_SLOT_GEAR_STATUS_FLAG) && (p->state.accessoryEffectFlags2 & 0x10)) {
        p->state.activeEffectFlags1 &= 0xFE4F;
        p->activeEffectFlags1 &= 0xEFFF;
    }
    if (g_BattleCommandCharIndex == 4) {
        g_BattleCurWorkBlock->commandCode = 0x3D;
    }
}

u8 BattleClearSlotFlag15A1(u8 arg0) {
    BattleTableEntry* p = &g_BattleCurWorkBlock->slots[arg0];
    u8 flags = p->state.flags & 0xFE;

    p->state.flags = flags;
    return flags;
}

// The row walks below cannot use a typed table view of g_BattleCurWorkBlock: the
// address has to stay an integer sum added to the block base, because gcc emits
// `addu d,base,offset` for `block->table[arg0]` where retail has `addu d,offset,base`
// (measured 99.71% / 99.77% with the tables declared on the struct).
// Re-checked with a .c-local view struct (2D BattleRow arrays at +0x1058/+0x2228):
// inline per-store indexing, a block pointer local in the arm and at the top of the
// function, a sizeof-scaled byte offset off the typed member, `[0][0]` addressing and
// a row-array typedef all land on 99.71/99.77 at retail's size, and a 4-minute
// permuter run (2294 iterations, base=20 best=20) found nothing. Also measured, both
// worse: sharing the two `slots[arg0]` pointers p and r (98.68), and a block pointer
// live across the function (90.81 in To2000). So the cast sum stays.
// Round 327 re-derived it with a .c-local view struct above the functions
// (`->gearRows[arg0]`, and the index-first `arg0 * rows + ->gearRows[0]`): 99.71/99.77
// again, the two addu alone, and a single-expression typed sum with no `s32 rowOffset`
// local is 93.75. The row base stays a byte offset in a local added to the block base.
void BattleSetSlotEntryFieldsTo1(u8 arg0) {
    BattleTableEntry* p = &g_BattleCurWorkBlock->slots[arg0];
    BattleTableEntry* r;
    u16 f;

    if (p->state.flags & BATTLE_SLOT_GEAR_STATUS_FLAG) {
        f = p->state.activeEffectFlags2 & BATTLE_GEAR_EFFECT_FLAG;
    } else {
        f = p->activeEffectFlags4 & BATTLE_ACTOR_EFFECT_FLAG;
    }
    if (f == 0) {
        return;
    }
    r = &g_BattleCurWorkBlock->slots[arg0];
    if (r->state.flags & BATTLE_SLOT_GEAR_STATUS_FLAG) {
        s32 rowOffset = arg0 * BATTLE_GEAR_ROW_STRIDE + BATTLE_GEAR_ROW_TABLE;
        BattleRow* pRow = (BattleRow*)(rowOffset + (u32)g_BattleCurWorkBlock);

        pRow[21].flags = 1;
        pRow[23].flags = 1;
        pRow[24].flags = 1;
        pRow[25].flags = 1;
        pRow[26].flags = 1;
        pRow[27].flags = 1;
        pRow[28].flags = 1;
    } else {
        s32 rowOffset = arg0 * BATTLE_ACTOR_ROW_STRIDE + BATTLE_ACTOR_ROW_TABLE;
        BattleRow* pRow = (BattleRow*)(rowOffset + (u32)g_BattleCurWorkBlock);

        pRow[22].flags = 1;
        pRow[24].flags = 1;
        pRow[25].flags = 1;
        pRow[26].flags = 1;
        pRow[27].flags = 1;
        pRow[28].flags = 1;
        pRow[29].flags = 1;
        pRow[30].flags = 1;
        pRow[31].flags = 1;
        pRow[32].flags = 1;
    }
}
void BattleSetSlotEntryFieldsTo2000(u8 arg0, u8 arg1) {
    BattleTableEntry* p;
    BattleTableEntry* r;
    u16 f;

    if (arg1 != 0 && !(g_BattleCurrentRow->actionFlags & 0x100)) {
        return;
    }
    p = &g_BattleCurWorkBlock->slots[arg0];
    if (p->state.flags & BATTLE_SLOT_GEAR_STATUS_FLAG) {
        f = p->state.activeEffectFlags2 & BATTLE_GEAR_EFFECT_FLAG;
    } else {
        f = p->activeEffectFlags4 & BATTLE_ACTOR_EFFECT_FLAG;
    }
    if (f == 0) {
        return;
    }
    r = &g_BattleCurWorkBlock->slots[arg0];
    if (r->state.flags & BATTLE_SLOT_GEAR_STATUS_FLAG) {
        s32 rowOffset = arg0 * BATTLE_GEAR_ROW_STRIDE + BATTLE_GEAR_ROW_TABLE;
        BattleRow* pRow = (BattleRow*)(rowOffset + (u32)g_BattleCurWorkBlock);

        pRow[21].flags = BATTLE_GEAR_EFFECT_FLAG;
        pRow[23].flags = BATTLE_GEAR_EFFECT_FLAG;
        pRow[24].flags = BATTLE_GEAR_EFFECT_FLAG;
        pRow[25].flags = BATTLE_GEAR_EFFECT_FLAG;
        pRow[26].flags = BATTLE_GEAR_EFFECT_FLAG;
        pRow[27].flags = BATTLE_GEAR_EFFECT_FLAG;
        pRow[28].flags = BATTLE_GEAR_EFFECT_FLAG;
        r->state.activeEffectFlags2 &= ~BATTLE_GEAR_EFFECT_FLAG;
    } else {
        s32 rowOffset = arg0 * BATTLE_ACTOR_ROW_STRIDE + BATTLE_ACTOR_ROW_TABLE;
        BattleRow* pRow = (BattleRow*)(rowOffset + (u32)g_BattleCurWorkBlock);

        pRow[22].flags = BATTLE_GEAR_EFFECT_FLAG;
        pRow[24].flags = BATTLE_GEAR_EFFECT_FLAG;
        pRow[25].flags = BATTLE_GEAR_EFFECT_FLAG;
        pRow[26].flags = BATTLE_GEAR_EFFECT_FLAG;
        pRow[27].flags = BATTLE_GEAR_EFFECT_FLAG;
        pRow[28].flags = BATTLE_GEAR_EFFECT_FLAG;
        pRow[29].flags = BATTLE_GEAR_EFFECT_FLAG;
        pRow[30].flags = BATTLE_GEAR_EFFECT_FLAG;
        pRow[31].flags = BATTLE_GEAR_EFFECT_FLAG;
        pRow[32].flags = BATTLE_GEAR_EFFECT_FLAG;
        r->activeEffectFlags4 &= ~BATTLE_ACTOR_EFFECT_FLAG;
    }
}
// 100.00% only under gcc-2.6.0 (88.68% on 2.7.2): 2.6.0 puts each `/20` mul by
// 0xCCCCCCCD first in the guarded block and mfhi in $v0, which is retail's order.
// 0x164 bytes against retail's 0x15c: the body is 8 bytes too long, so it cannot
// be converted even if it diffs clean (see the size check in AGENTS.md).
// PARKED (88.68%). Body COMPILES (park triage, round 111) and is 356 bytes
// against retail's 348: 8 bytes too long, so it cannot be converted even if it
// reached 100. See the note above for the /20 ordering difference.
// Round 397 re-measured live: 88.68% (356 bytes, 89 insns vs retail 87) under the unit's
// 2.7.2, and 100.0 under gcc-2.6.0-psx at -O2 -G8 (try_flags), confirming the note below.
// The whole 2.7.2 residual per '/20' block is the 2.6.0 ordering: retail fills the branch
// delay slot with the constant's `lui`, keeps the magic in v1 with the loaded value in v0,
// and does `mfhi v0 / srl v0,v0,0x4` in place, where 2.7.2 emits a nop delay slot, the lhu
// first, the constant in v0, and `mfhi a3 / srl v1,a3,0x4`. Splitting the load from the
// division (val = p[0x4E]; val = val / 20;) measured no change. The unit is mixed under
// 2.6.0 (round 393 screen: BattleClearSlotFlag15A1 95.71, BattleFillPartySlotWeaponRecord
// 96.15), so like func_8009A9D0 this is a one-function XenoLibrary carve-out candidate.
/*
s32 func_8009ADA0(u8 arg0, u32* arg1) {
    u8* p = arg0 * 0x170 + (u32)g_BattleCurWorkBlock;
    u16 f = *(u16*)(p + 0x7C);
    u16 val;
    s32 ret = 0;

    if (f & 0x8000) {
        return 0;
    }
    if (f & 0x800) {
        val = *(u16*)(p + 0x4E) / 20;
        *arg1 = val;
        ret = 1;
    }
    arg1++;
    if (*(u16*)(p + 0x80) & 0x200) {
        val = *(u16*)(p + 0x52) / 20;
        *arg1 = val;
        ret = 1;
    }
    if (*(u16*)(p + 0x120) & 0x200) {
        val = *(u16*)(p + 0x52) / 20;
        *arg1 = val;
        ret = 1;
    }
    arg1++;
    *arg1 = 0;
    if (*(u16*)(p + 0x120) & 0x80) {
        val = *(u16*)(p + 0xDE) / 50;
        *arg1 = val;
        ret = 1;
    }
    if (*(u16*)(p + 0x124) & 0x8000) {
        val = *(u16*)(p + 0xDE) / 50;
        *arg1 += val;
        ret = 1;
    }
    return ret;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009ADA0);
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
// func_8009A9D0 and BattleSetSlotActionStateAndAmount off 100. Try_flags on this body: 97.81 under
// gcc-2.6.0, 94.63 under 2.7.2-cdk. Measured this round: limit/flags/drop declaration
// order (no change), max/cur assigned cur-first (worse - $a1 becomes an address base).
/*
void func_8009B46C(u16* arg0) {
    u8 count = 0;
    u8 i;

    for (i = 0; i < 3; i++) {
        BattleTableEntry* p = (BattleTableEntry*)(i * sizeof(BattleTableEntry) + (u32)g_BattleCurWorkBlock);
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
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009B46C);
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
