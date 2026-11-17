#include "common.h"
#include "battle/actor.h"
#include "main/game.h"

// state.flags bit that selects the gear-side status word over the actor's
#define BATTLE_SLOT_GEAR_STATUS_FLAG 0x80
// hp and maxHp are percents; state.hp/state.maxHp hold them times this
#define BATTLE_ACTOR_HP_SCALE 50
#define BATTLE_SCALED_HP_MAX 99999
// bit of the BattleGetActorActionConditionMask result set while hp is under an eighth of maxHp
#define BATTLE_ACTION_CONDITION_LOW_HP 0x2

extern BattleWorkBlock* g_BattleCurWorkBlock;
extern u8 g_BattleCurSlotIndex;
extern BattleRow* g_BattleCurrentRow;
extern u8* g_BattleCurSlotState;
extern u8 g_BattleSlotConditions[];
extern BattleTableEntry g_BattleWorkBlock[];
extern u8 D_800D2C34;
extern u8* g_BattleActingSlot;
extern u8 D_800CCD68[];

extern int rand(void);
extern void func_8009BE0C(void);
extern void BattleSetActorScaledHp(u8 arg0, BattleTableEntry* pArg);

s32 BattleGetActorConditionCode(u8 arg0) {
    BattleTableEntry* p = (BattleTableEntry*)(arg0 * 0x170 + (u32)g_BattleCurWorkBlock);
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
    BattleTableEntry* p = (BattleTableEntry*)(arg0 * 0x170 + (u32)g_BattleCurWorkBlock);
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

s32 BattleIsCharacterInParty(u8 arg0) {
    u8 id = ((BattlePartyRecord*)((u8*)g_BattleCurWorkBlock + 0x5818))[arg0].characterId;

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
    if ((p->state.flags & BATTLE_SLOT_GEAR_STATUS_FLAG) && (p->state.unk82 & 0x10)) {
        p->state.unk7C &= 0xFE4F;
        p->activeEffectFlags1 &= 0xEFFF;
    }
    if (D_800D2C34 == 4) {
        g_BattleCurWorkBlock->commandCode = 0x3D;
    }
}

u8 BattleClearSlotFlag15A1(u8 arg0) {
    BattleTableEntry* p = &g_BattleCurWorkBlock->slots[arg0];
    u8 flags = p->state.flags & 0xFE;

    p->state.flags = flags;
    return flags;
}

void BattleSetSlotEntryFieldsTo1(u8 arg0) {
    BattleTableEntry* p = &g_BattleCurWorkBlock->slots[arg0];
    BattleTableEntry* r;
    u16 f;

    if (p->state.flags & BATTLE_SLOT_GEAR_STATUS_FLAG) {
        f = p->state.unk80 & 0x2000;
    } else {
        f = p->activeEffectFlags4 & 0x400;
    }
    if (f == 0) {
        return;
    }
    r = &g_BattleCurWorkBlock->slots[arg0];
    if (r->state.flags & BATTLE_SLOT_GEAR_STATUS_FLAG) {
        s32 rowOffset = arg0 * 0x690 + 0x2228;
        BattleRow* pRow = (BattleRow*)((u8*)rowOffset + (u32)g_BattleCurWorkBlock);

        pRow[21].flags = 1;
        pRow[23].flags = 1;
        pRow[24].flags = 1;
        pRow[25].flags = 1;
        pRow[26].flags = 1;
        pRow[27].flags = 1;
        pRow[28].flags = 1;
    } else {
        s32 rowOffset = arg0 * 0x5F0 + 0x1058;
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

    if (arg1 != 0) {
        if (!(g_BattleCurrentRow->actionFlags & 0x100)) {
            return;
        }
    }
    p = &g_BattleCurWorkBlock->slots[arg0];
    if (p->state.flags & BATTLE_SLOT_GEAR_STATUS_FLAG) {
        f = p->state.unk80 & 0x2000;
    } else {
        f = p->activeEffectFlags4 & 0x400;
    }
    if (f == 0) {
        return;
    }
    r = &g_BattleCurWorkBlock->slots[arg0];
    if (r->state.flags & BATTLE_SLOT_GEAR_STATUS_FLAG) {
        s32 rowOffset = arg0 * 0x690 + 0x2228;
        BattleRow* pRow = (BattleRow*)((u8*)rowOffset + (u32)g_BattleCurWorkBlock);

        pRow[21].flags = 0x2000;
        pRow[23].flags = 0x2000;
        pRow[24].flags = 0x2000;
        pRow[25].flags = 0x2000;
        pRow[26].flags = 0x2000;
        pRow[27].flags = 0x2000;
        pRow[28].flags = 0x2000;
        r->state.unk80 &= 0xDFFF;
    } else {
        s32 rowOffset = arg0 * 0x5F0 + 0x1058;
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
        r->activeEffectFlags4 &= 0xFBFF;
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
// best 90.27% (2.7.2-psx and gcc-2.6.0); 89.18% cdk. loop.c hoists the
// D_800CCD68 base out of the 3..11 loop; retail re-forms it at both references.
// PARKED (90.27%). Body COMPILES (park triage, round 111) but is 216 bytes
// against retail's 220 - the missing instruction is the D_800CCD68 base that
// retail re-forms per reference and gcc hoists, so it cannot link either.
// Round 123 measured: index-first spelling (`i * 0x170 + (u32)D_800CCD68`) 90.27,
// a `u16* q` pointer local assigned inside the loop 90.27, a `do { } while (0)`
// around the RMW 87.36 (220 bytes, but it reaches 220 by a nop, not by keeping
// the base per reference). retail's per-reference `lui at,%hi/addu at,at,v0` is
// the maspsx expansion of a `symbol+reg` mem operand; ours materialises
// `&D_800CCD68` with lui/addiu in the preheader (loop.c) and then does
// `addu v0,v0,a1`. gcc's loop.c base hoist is the gate, not the loop shape.
/*
Best attempt: 88.27%. Everything matches except the 3..11 loop: the original
   re-forms `lui at,%hi(D_800CCD68); addu at,at,v0` at BOTH memory references,
   while gcc hoists the symbol base into a register before the loop (loop.c moves
   the invariant), which then CSEs the address and drops 7 insns. Tried: plain
   `*(u16*)(D_800CCD68 + i*0x170) &= 0xFFDF`, the `((u16*)(...))[0] &=` spelling
   (86.36%), `u8 t` vs `s32 t` for the 0x56 temp. Everything else (two g_BattleCurWorkBlock
   loads, the four zero stores, the `f & 0x2000` store and the BattleSetActorScaledHp call)
   lines up exactly.
void func_8009AEFC(u8 arg0) {
    u8* p;
    u16 f;
    s32 t;

    *(u8*)((u8*)g_BattleCurWorkBlock + 0x5FC2) = 0;
    p = arg0 * 0x170 + (u32)g_BattleCurWorkBlock;
    f = *(u16*)(p + 0x80);
    t = p[0x56];
    *(u16*)(p + 0x7C) = 0;
    *(u16*)(p + 0x84) = 0;
    *(u16*)(p + 0x88) = 0;
    *(u16*)(p + 0x8C) = 0;
    *(u16*)(p + 0x80) = f & 0x2000;
    if (t == 7) {
        BattleSetActorScaledHp(arg0, p);
    }
    if (p[0x56] == 3) {
        u8 i;

        for (i = 3; i < 11; i++) {
            *(u16*)(D_800CCD68 + i * 0x170) &= 0xFFDF;
        }
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009AEFC);
void BattleDecrementGearPartUse(void) {
    u8 sel = g_BattleCurWorkBlock->rowIndex;
    s32 idx;

    switch (sel) {
    case 0:
    case 1:
    case 2:
    case 3:
        idx = g_BattleActingSlot[0x6F];
        break;
    case 4:
    case 5:
        return;
    case 6:
        idx = g_BattleActingSlot[0x72];
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
        idx = g_BattleActingSlot[0x6F];
        if (g_GameState.gearPartsIds[0x6C + idx]) {
            g_GameState.gearPartsIds[0x6C + idx] = g_GameState.gearPartsIds[0x6C + idx] + 0xFF;
        }
        idx = g_BattleActingSlot[0x72];
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

    for (i = 1; i < 3; i++) {
        BattleTableEntry* p = (BattleTableEntry*)(i * 0x170 + (u32)g_BattleCurWorkBlock);

        p->hp = 100;
        p->maxHp = 100;
        p->evade = 20;
        p->hit = 15;
        p->unk7A = 0x1FBF;
        p->state.unkA5 = 0;
    }
}

void BattleSetActorScaledHp(u8 arg0, BattleTableEntry* pArg) {
    BattleTableEntry* p = (BattleTableEntry*)(arg0 * 0x170 + (u32)g_BattleCurWorkBlock);
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

INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009B1E4);
// best 94.23% under gcc-2.6.0 (93.87% on 2.7.2, 94.10% cdk): the g_BattleSlotConditions
// base is hoisted out of the loop where retail re-forms lui/addiu per reference.
// PARKED (93.87%). Body COMPILES (park triage, round 111; it needed the extern u8
// g_BattleSlotConditions[] that is now at the top of the file). 532 bytes against retail's
// 536 - the missing instruction is the g_BattleSlotConditions base retail re-forms per
// reference and gcc hoists, the same gap as func_8009AEFC above.
/*
Best attempt: 93.87%. Every instruction from the `p[0x56] == 3` test onward
   matches; the only difference is the g_BattleSlotConditions access. The original re-forms
   `lui at,%hi(g_BattleSlotConditions); addu at,at,idx; lbu %lo(at)` at BOTH references,
   while gcc's loop.c hoists the symbol base into a register before the loop
   (`lui t1; addiu t1` at function entry) and then does `addu v0,idx,t1;
   lbu 0(v0)` -- 4 bytes larger, which shifts every branch target before the
   second block and swaps the max/cur registers ($a1/$v1). Same signature as the
   func_8009AEFC park (D_800CCD68) in this unit.
   Tried: `g_BattleSlotConditions[i*0x170]`, `((u32)i*0x170 + (u32)g_BattleSlotConditions)`,
   `((u8*)&g_BattleSlotConditions)[(u32)i*0x170]`, `((u8*)&g_BattleSlotConditions)[i*0x170]`,
   `*(volatile u8*)((u32)g_BattleSlotConditions + (u32)i*0x170)`, gcc-2.7.2-cdk (94.10%),
   -O3 (93.87%) and -G0 (93.87%) -- all identical. Established from the matched
   functions in this overlay (BattleGetGameCharacterField5 here, BattleActorPosXGreaterThan in misc12) that
   gcc emits the $at form only when the symbol's address is credited to no
   pseudo at all; here the same `sym + i*0x170` value occurs in both blocks, so
   CSE credits it and the loop hoists it. Not source-reachable without changing
   the instruction multiset (a hand-written shift chain for one of the two
   indices would defeat the CSE but break retail's multiply-expansion chain).
   NOTE: this file needs `extern u8 g_BattleSlotConditions[];` (added) or the attempt below
   does not compile at all.
void func_8009B46C(u16* arg0) {
    u8 count = 0;
    u8 i;

    for (i = 0; i < 3; i++) {
        u8* p = (u8*)g_BattleCurWorkBlock + i * 0x170;
        u8* q = p + 0xA4;
        u32 cur;
        u32 max;

        if (p[0x56] == 0) {
            if (g_BattleSlotConditions[i * 0x170] & 0x80) {
                max = *(u32*)(p + 0x108);
                cur = *(u32*)(p + 0x104);
            } else {
                max = *(u16*)(p + 0x4E);
                cur = *(u16*)(p + 0x4C);
            }
            if (cur < (max >> 1)) {
                count++;
            }
            if (cur < (max >> 2)) {
                count++;
            }
        }
        if (p[0x56] == 3) {
            if (g_BattleSlotConditions[i * 0x170] & 0x80) {
                max = *(u32*)(q + 0x64);
                cur = *(u32*)(q + 0x60);
            } else {
                max = *(u16*)(p + 0x4E);
                cur = *(u16*)(p + 0x4C);
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
        u32 limit = 0xA;
        u16 f;
        s32 drop;

        if (count != 0) {
            *arg0 = *arg0 + count * (*arg0 >> 1);
        }
        f = *(u16*)(g_BattleActingSlot + 0x32);
        drop = (f & 0x400) ? 4 : 3;
        if (f & 0x200) {
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
            if (*(u16*)&g_GameState.unk16BC[p->characterId].unk6 & ((0x8000 >> bit) & mask)) {
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
// best 98.33% (was 86.33%): only the last four instructions differ, and only by
// register: gcc puts the second g_BattleCurSlotIndex load in $v0 where retail has $v1, so
// mfhi lands in $a1 instead of $v0. Permuter score 50, stuck over 1907+1250
// iterations, so this is an allocator tie and not a structural miss.
// The gain came from splitting the two statements' base loads: a u8* local for
// the first store, and a plain expression for the second so (u32)g_BattleCurWorkBlock is
// loaded again; the int idx local stops the second lbu being CSE'd into the
// first, which is what lets the base reload schedule after multu.
// Also tried, all 98.33 or worse: index-first addu operand order (97.33), two
// separate locals for idx and address, declaration order swapped, u8* address
// local, u32 quotient local stored separately, 20u divisor, dropping the idx
// temp, u32* destination pointer local (70.17, CSEs the base).
// PARKED (98.33%). Body COMPILES again (park triage, round 111): the park spelled
// g_BattleCurrentRow[0x11] from a time when the symbol was a u8*; it is a BattleRow*
// now, so the same byte is g_BattleCurrentRow->power (identical code). Only the last
// four instructions differ, and only by register - see the note above.
// PRESET_260: 100.00% under gcc-2.6.0 (tools/scripts/try_flags.sh), 98.33% under
// the unit's 2.7.2 (and under 2.7.2-cdk). The last four instructions differ:
// retail reloads the second base into $v0/$v1 and then mfhi's into $v0 with the
// srl in place, so the mfhi result is coalesced with its consumer; 2.7.2 copies
// it to $a1 and shifts into $v0. That is the same mfhi-coalescing difference
// func_8009A9D0 has, i.e. a smulsi3_highpart md difference between the two cc1s,
// not a source shape. See func_8009A9D0's note for the whole-unit screen.
// The gain to 98.33 came from splitting the two statements' base loads: a u8*
// local for the first store, and a plain expression for the second so
// (u32)g_BattleCurWorkBlock is loaded again; the int idx local stops the second lbu being
// CSE'd into the first, which is what lets the base reload schedule after multu.
// Also tried, all 98.33 or worse: index-first addu operand order (97.33), two
// separate locals for idx and address, declaration order swapped, u8* address
// local, u32 quotient local stored separately, 20u divisor, dropping the idx
// temp, u32* destination pointer local (70.17, CSEs the base). Permuter score 50,
// stuck over 1907+1250 iterations.
// PARKED (98.33%). Body compiles; g_BattleCurrentRow is a BattleRow* so the old
// g_BattleCurrentRow[0x11] spelling is now ->power (identical code).
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
u8 BattleGetActorActionConditionMask(u8 arg0) {
    BattleTableEntry* pEntry = (BattleTableEntry*)(arg0 * 0x170 + (u32)g_BattleWorkBlock);
    BattleSlotState* pState = (BattleSlotState*)(arg0 * 0x170 + 0xA4 + (u32)g_BattleWorkBlock);
    u8* pCondition = (u8*)g_BattleWorkBlock + 0x148 + arg0 * 0x170;
    s32 flag = pState->unk7C & 0x400;
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
