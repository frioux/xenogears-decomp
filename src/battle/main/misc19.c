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


extern BattleWorkBlock* g_BattleCurWorkBlock;
extern u8 g_BattleCurSlotIndex;
extern BattleRow* g_BattleCurrentRow;
extern u8* g_BattleCurSlotState;
extern u8 g_BattleSlotConditions[];
extern BattleTableEntry g_BattleWorkBlock[];
extern u8 g_BattleCommandCharIndex;
extern BattleTableEntry* g_BattleActingSlot;
extern u8 D_800CCD68[];

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
// PARKED (99.68%, 124 bytes = retail's size). Body COMPILES. Round 122: the permuter
// broke the old 95.32 wall. The `u8 value = sum;` copy retail materialises
// (`addu v1,v0,zero`) DOES appear once the clamp's store sits in two branches -
// `if (sum || sum) { value = 0x64; } else { value = 0x64; }` - because two store
// sites stop gcc 2.7.2 coalescing value into the sum pseudo. Also load-bearing:
// sum is `short` (s32 instead mirrors the two lbu), the byte sum is two statements
// (`sum = p[0x14]; sum = sum + q[0x5B];`), and q is formed index-first
// (`(u8*)(arg0 * 0x170) + (u32)g_BattleCurWorkBlock`) for retail's `addu a2,a2,a3`.
// Only remaining diff, one register: retail `addu v0,v0,v1` + `move v1,v0`, here
// `addu a0,v0,v1` + `move v1,a0`. Everything else matches, including both lbu
// (retail p byte in v0, q byte in v1). sum's home is a0, the lowest free reg,
// where retail kept it in v0.
// Tried this round: u8 sum (loses the copy, 95.65), u_short sum (addu into a1,
// 99.03), s32 sum with the split (lbu mirror a1/v0, 99.03), s32 with value loaded
// first (98.39), one-expression sum with the duplicate store (98.06), value/sum
// declaration order swapped (no change). Permuter rounds this session, base/best
// score: 60/40 (split the sum), 40/20 (short sum), 20/10 (the sum || sum condition).
// Round 123: an 8-minute permuter run from this exact body (base=10, best=10,
// 3886 iterations) found nothing, and neither did 15 hand shapes. The pattern is
// that a merged accumulator (u16/s32 sum, or a one-expression sum, or u8 temps
// feeding an s32 'a + b') gives retail's two-address 'addu d,d,s' shape but puts
// the accumulator in a1/v1, while the HImode two-statement form (this body) is the
// only one that gets retail's two lbu (p byte v0, q byte v1) yet then needs a
// separate home for sum (a0). `short sum = p[0x14];` written mid-block is an
// INVALID body on this preset: gcc 2.6.0 exits 33 (mixed declarations), diff_func
// then prints only the objdiff line and no diff at all.
// Round 169 (re-measured): 99.68%, 124 bytes = retail's. cdk is 95.97 here, so the
// pending cdk split would LOSE this function; do not switch the unit for it. New
// finding about the remaining tie: the ONE-expression form `short sum = p[0x14] +
// q[0x5B];` does give retail's in-place two-address add (`addu v1,v1,v0`, exact
// 124-byte multiset, 98.39%) but coalesces sum with the dying p-pointer register
// (v1) instead of a fresh one, and moves value to a0; the two-statement form below
// gets the loads right (p byte v0, q byte v1) and loses only sum's home. So the
// whole remaining gap is one allocator choice: retail's p-byte load went to a
// FRESH v0 rather than reusing the base register that dies at it. A 3-minute
// permuter run from the one-expression body (base=50 best=10, 1404 iterations)
// walked straight back to the two-statement body below - i.e. it agrees this is
// the floor. Shapes measured this round: swapped add operands (no change), a `u8
// pbyte` temp (7 registers off, 98.39), the one-expression sum (98.39).
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
    /* 0x29 */ u8 unk29; // the slot state's unk98
    /* 0x2A */ u8 unk2A; // the slot state's penaltyPercent
    /* 0x2B */ u8 unk2B;
    /* 0x2C */ u8 unk2C;
    /* 0x2D */ u8 unk2D;
    /* 0x2E */ u16 amount; // what the action deals
} BattleActionWork; // Size: 0x30

INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009A2D4);

// PARKED (98.85%, 1252 bytes against retail's 1248). Body COMPILES.
// Structure is retail's: the pointer set (pEntry/pState/pCondition/pGearRow/pWork), the
// 15-row cost copy loop, the state-code switch (a `switch` reproduces retail's test chain
// where an if/else-if chain does not), and the tails. Load-bearing shapes found this round:
// `u8 amount` (a u32 makes gcc coalesce the division result and lose retail's `move s1,v1`),
// the split `attack = pState->unk3C[0]; attack = attack * pState->unk3C[3];`, a `u16` local
// for the activeEffectFlags2 test, and a SECOND pCondition pointer (`new_var2`) for the tail
// blocks, which is what fixed retail's sp.N regs (94 -> 98.85). Remaining: four register-role
// swaps (retail `mflo v1` / `mfhi v0`, and the unk54/amount multiply pair in v1/v0) plus the
// `pState->unk50[7]` byte load, which gcc CSEs across the branch where retail reloads it
// because retail's copy dies in the `li v0,0x1E` delay slot. Retail is 4 bytes LONGER here.
// `new_var2` also lands in a2 where retail reuses s2, and the maxHp==hp `amount++` block is
// stolen into the `beq` delay slot instead of staying out of line. Two permuter rounds
// (1506 and 1597 iterations) took this from 92.76 to 98.85 and found nothing further.
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
    if (pState->unk50[7] != 0) {
        pWork->cost = pState->unk50[7] * pWork->rowCosts[0];
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
    attack = pState->unk3C[0];
    attack = attack * pState->unk3C[3];
    pWork->attackAmount = attack;
    attack = attack + pState->weaponData[0].attack;
    pWork->attackAmount = attack;
    if (pEntry->characterId == 4) {
        attack = attack + pState->weaponData[2].attack;
        pWork->attackAmount = attack;
    }
    new_var2 = pCondition;
    pWork->unk29 = pState->unk98;
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
        ret = 1;
    } else if (g_GameState.characters[4].slots[1].characterId == id) {
        ret = 1;
    } else if (g_GameState.characters[4].slots[2].characterId == id) {
        ret = 1;
    } else {
        ret = g_GameState.characters[4].slots[3].characterId == id;
    }
    return ret;
}

// best 84.95% under gcc-2.6.0 (76.74% on 2.7.2, 72.68% cdk): retail folds each
// g_GameState store address into the relocation, gcc CSEs one register for all four.
// PARKED (76.74%). Body COMPILES (park triage, round 111); nothing changed in it.
// The note above is still the whole gap: retail folds each g_GameState store
// address into its relocation, gcc CSEs one base register for all four.
/*
Best attempt: 76.74% (round 18, first attempt on this stub). The 4-way
   compare chain, the `andi/sltiu` loop and every offset/mask are right; what is
   wrong is the g_GameState addressing. Retail materialises each store address
   fresh as `lui at,%hi(g_GameState+0x500); addu at,at,idx; sb %lo(at)` with the
   constant folded into the relocation, while gcc CSEs the first
   `g_GameState+0x502` load into a register (emitting `lui a0; addiu a0,1282;
   lbu 0(a0)`, i.e. an `la`, instead of the folded `lbu %lo(g_GameState+0x502)`)
   and then reuses it as `a0-2/-3/-4` for the stores. BattleIsCharacterInParty in this same
   unit uses the identical `g_GameState.characters[4].slots[0].characterId` expression and
   DOES fold, so the trigger is the store block coexisting with the loads; raw
   `((u8*)&g_GameState)[N]` and struct spellings both give 76.74%.
PARKED (76.74%)
void func_8009A854(u8 arg0, u8 arg1) {
    u8* p = (u8*)(arg0 * 16 + 0x54F8) + (u32)g_BattleCurWorkBlock;
    u8 id = p[6];
    u8 row = arg1;
    u8 i;

    if (((u8*)&g_GameState)[0x502] == id) {
        row = 0;
    }
    if (((u8*)&g_GameState)[0x50A] == id) {
        row = 1;
    }
    if (((u8*)&g_GameState)[0x512] == id) {
        row = 2;
    }
    if (((u8*)&g_GameState)[0x51A] == id) {
        row = 3;
    }
    *(u8*)((u8*)&g_GameState + 0x500 + row * 8) = p[0xC];
    *(u8*)((u8*)&g_GameState + 0x4FF + row * 8) = p[0xB];
    *(u8*)((u8*)&g_GameState + 0x4FE + row * 8) = p[0xA];
    *(u8*)((u8*)&g_GameState + 0x4FF + row * 8) = p[0xB];
    ((u8*)&g_GameState)[0x56B + row] = arg0;
    g_GameState.gearPartsIds[0x6C + arg0] = p[3];
    ((u8*)&g_GameState)[0x56B + row] = arg0;
    for (i = 0; i < 3; i++) {
        u8* q = (u8*)(i * 0x170 + (u32)g_BattleCurWorkBlock);

        if (q[0x56] == 4) {
            u8* r = q + row * 8;

            r[4] = p[0xC];
            r[3] = p[0xB];
            r[2] = p[0xA];
            r[3] = p[0xB];
            q[0x6F + row] = arg0;
        }
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009A854);

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
        BattleRow* pRow = (BattleRow*)((u8*)rowOffset + (u32)g_BattleCurWorkBlock);

        pRow[21].flags = 1;
        pRow[23].flags = 1;
        pRow[24].flags = 1;
        pRow[25].flags = 1;
        pRow[26].flags = 1;
        pRow[27].flags = 1;
        pRow[28].flags = 1;
    } else {
        s32 rowOffset = arg0 * BATTLE_ACTOR_ROW_STRIDE + BATTLE_ACTOR_ROW_TABLE;
        BattleRow* pRow = (BattleRow*)((u8*)rowOffset + (u32)g_BattleCurWorkBlock);

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
        BattleRow* pRow = (BattleRow*)((u8*)rowOffset + (u32)g_BattleCurWorkBlock);

        pRow[21].flags = 0x2000;
        pRow[23].flags = 0x2000;
        pRow[24].flags = 0x2000;
        pRow[25].flags = 0x2000;
        pRow[26].flags = 0x2000;
        pRow[27].flags = 0x2000;
        pRow[28].flags = 0x2000;
        r->state.activeEffectFlags2 &= ~BATTLE_GEAR_EFFECT_FLAG;
    } else {
        s32 rowOffset = arg0 * BATTLE_ACTOR_ROW_STRIDE + BATTLE_ACTOR_ROW_TABLE;
        BattleRow* pRow = (BattleRow*)((u8*)rowOffset + (u32)g_BattleCurWorkBlock);

        pRow[22].flags = 0x2000;
        pRow[24].flags = 0x2000;
        pRow[25].flags = 0x2000;
        pRow[26].flags = 0x2000;
        pRow[27].flags = 0x2000;
        pRow[28].flags = 0x2000;
        pRow[29].flags = 0x2000;
        pRow[30].flags = 0x2000;
        pRow[31].flags = 0x2000;
        pRow[32].flags = 0x2000;
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
        for (i = 3; i < 11; i++) {
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
        p->state.unkA5 = 0;
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

    for (i = 1; i < 0x30; i++) {
        g_GameState.itemIDs[i] = i;
        g_GameState.itemQuantities[i] = MAX_ITEM_QUANTITY;
    }
    *(u16 *)&g_GameState.unk16BC[0].unk4 = 0xFFF8;
    *(u16 *)&g_GameState.unk16BC[0].characterConditionMask = 0xFF00;
    *(u16 *)&g_GameState.unk16BC[0].unk8 = 0xFFFF;
    *(u16 *)&g_GameState.unk16BC[0].gearConditionMask = 0xFE00;
    *(u16 *)&g_GameState.unk16BC[0].statusFlags = 0xF000;
    ((u_char *)&g_GameState.unk16BC[0])[0x1B] = 7;
    *(u16 *)&g_GameState.unk16BC[1].unk4 = 0xFFE0;
    *(u16 *)&g_GameState.unk16BC[1].characterConditionMask = 0xFFF0;
    *(u16 *)&g_GameState.unk16BC[1].unk8 = 0xFFFF;
    *(u16 *)&g_GameState.unk16BC[1].gearConditionMask = 0xFFF0;
    *(u16 *)&g_GameState.unk16BC[1].statusFlags = 0xC000;
    ((u_char *)&g_GameState.unk16BC[1])[0x1B] = 7;
    *(u16 *)&g_GameState.unk16BC[2].unk4 = 0xFFE0;
    *(u16 *)&g_GameState.unk16BC[2].characterConditionMask = 0xFFE0;
    *(u16 *)&g_GameState.unk16BC[2].unk8 = 0xFFFF;
    *(u16 *)&g_GameState.unk16BC[2].gearConditionMask = 0xFF00;
    *(u16 *)&g_GameState.unk16BC[2].statusFlags = 0x8000;
    ((u_char *)&g_GameState.unk16BC[2])[0x1B] = 7;
    *(u16 *)&g_GameState.unk16BC[3].unk4 = 0xFFE0;
    *(u16 *)&g_GameState.unk16BC[3].characterConditionMask = 0xFFC0;
    *(u16 *)&g_GameState.unk16BC[3].unk8 = 0xFFFF;
    *(u16 *)&g_GameState.unk16BC[3].gearConditionMask = 0xFF00;
    *(u16 *)&g_GameState.unk16BC[3].statusFlags = 0xF000;
    ((u_char *)&g_GameState.unk16BC[3])[0x1B] = 7;
    *(u16 *)&g_GameState.unk16BC[4].unk4 = 0xFFC0;
    *(u16 *)&g_GameState.unk16BC[4].characterConditionMask = 0xFFC0;
    *(u16 *)&g_GameState.unk16BC[4].unk8 = 0xFFFF;
    *(u16 *)&g_GameState.unk16BC[4].gearConditionMask = 0xFC00;
    *(u16 *)&g_GameState.unk16BC[4].statusFlags = 0xE000;
    ((u_char *)&g_GameState.unk16BC[4])[0x1B] = 7;
    *(u16 *)&g_GameState.unk16BC[5].unk4 = 0xFFC0;
    *(u16 *)&g_GameState.unk16BC[5].characterConditionMask = 0xF000;
    *(u16 *)&g_GameState.unk16BC[5].unk8 = 0xFFFF;
    *(u16 *)&g_GameState.unk16BC[5].gearConditionMask = 0xF000;
    *(u16 *)&g_GameState.unk16BC[5].statusFlags = 0x8000;
    ((u_char *)&g_GameState.unk16BC[5])[0x1B] = 7;
    *(u16 *)&g_GameState.unk16BC[6].unk4 = 0xFFC0;
    *(u16 *)&g_GameState.unk16BC[6].characterConditionMask = 0xFF00;
    *(u16 *)&g_GameState.unk16BC[6].unk8 = 0xFFFF;
    *(u16 *)&g_GameState.unk16BC[6].gearConditionMask = 0xFF00;
    *(u16 *)&g_GameState.unk16BC[6].statusFlags = 0x8000;
    ((u_char *)&g_GameState.unk16BC[6])[0x1B] = 7;
    *(u16 *)&g_GameState.unk16BC[7].unk4 = 0x0000;
    *(u16 *)&g_GameState.unk16BC[7].characterConditionMask = 0xFF00;
    *(u16 *)&g_GameState.unk16BC[7].unk8 = 0x0000;
    *(u16 *)&g_GameState.unk16BC[7].gearConditionMask = 0xFF00;
    *(u16 *)&g_GameState.unk16BC[7].statusFlags = 0x0000;
    ((u_char *)&g_GameState.unk16BC[7])[0x1B] = 7;
    *(u16 *)&g_GameState.unk16BC[8].unk4 = 0x0000;
    *(u16 *)&g_GameState.unk16BC[8].characterConditionMask = 0xF800;
    *(u16 *)&g_GameState.unk16BC[8].unk8 = 0xFFFF;
    *(u16 *)&g_GameState.unk16BC[8].gearConditionMask = 0x0000;
    *(u16 *)&g_GameState.unk16BC[8].statusFlags = 0xE000;
    ((u_char *)&g_GameState.unk16BC[8])[0x1B] = 7;
    *(u16 *)&g_GameState.unk16BC[9].unk4 = 0xFFE0;
    *(u16 *)&g_GameState.unk16BC[9].characterConditionMask = 0xFFE0;
    *(u16 *)&g_GameState.unk16BC[9].unk8 = 0xFFFF;
    *(u16 *)&g_GameState.unk16BC[9].gearConditionMask = 0xFFE0;
    *(u16 *)&g_GameState.unk16BC[9].statusFlags = 0x8000;
    ((u_char *)&g_GameState.unk16BC[9])[0x1B] = 7;
    *(u16 *)&g_GameState.unk16BC[10].unk4 = 0xFFC0;
    *(u16 *)&g_GameState.unk16BC[10].characterConditionMask = 0xFF00;
    *(u16 *)&g_GameState.unk16BC[10].unk8 = 0xFFFF;
    *(u16 *)&g_GameState.unk16BC[10].gearConditionMask = 0xFF00;
    *(u16 *)&g_GameState.unk16BC[10].statusFlags = 0xC000;
    ((u_char *)&g_GameState.unk16BC[10])[0x1B] = 7;
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
// func_8009A9D0 and func_8009BD94 off 100. Try_flags on this body: 97.81 under
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009BAC4);
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
/*
void func_8009BAC4(u8 arg0, u8* pInfo, s16* pStatus) {
    BattleTableEntry* p = &g_BattleWorkBlock[arg0];
    u16 mask;
    u8 count;
    u8 bit;

    if (g_BattleSlotConditions[arg0 * 0x170] & 0x80) {
        mask = 1;
    } else {
        if (rand() % 100 < 10 && !(p->unk7A & 0x100) && *pStatus == 0) {
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
        if (rand() % 100 < 25 && !(p->unk7A & 0x20)) {
            *pInfo = 2;
            bit = rand() % count;
            if (*(u16*)&g_GameState.unk16BC[p->characterId].characterConditionMask & ((0x8000 >> bit) & mask)) {
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009BD94);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009BE0C);
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
    g_BattleWorkBlock[arg0].state.unkA4 = 4;
    g_BattleWorkBlock[arg0].state.conditionTimers[7] = 3;
    g_GameState.unk16BC[0].statusFlags |= 0x4000;
}
