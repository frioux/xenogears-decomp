#include "common.h"
#include "battle/actor.h"
#include "main/game.h"

extern BattleWorkBlock* D_800C34B0;
extern u8 D_800C3E50;
extern BattleRow* D_800C3DFC;
extern u8* D_800D2DC8;
extern u8 D_800CCE42[];
extern BattleTableEntry D_800CCCE8[];
extern u8 D_800D2C34;
extern u8* D_800C3E00;
extern u8 D_800CCD68[];

extern int rand(void);
extern void func_8009BE0C(void);
extern void BattleSetActorScaledHp(u8 arg0, BattleTableEntry* pArg);

s32 BattleGetActorConditionCode(u8 arg0) {
    BattleTableEntry* p = (BattleTableEntry*)(arg0 * 0x170 + (u32)D_800C34B0);
    u16 f7C = p->unk7C;
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
    f80 = p->unk80;
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
    if ((p->unk84 | p->unk86) & 0x8000) {
        return 7;
    }
    return p->unk4C < (p->unk4E >> 3) ? 5 : 0;
}
u16 FieldScriptClearAnimationOverrideIfFlag16(u8 arg0) {
    BattleTableEntry* p = (BattleTableEntry*)(arg0 * 0x170 + (u32)D_800C34B0);
    u16 f7C = p->unk7C;
    u16 ret = 0;
    u16 f;

    if (f7C & 0x8000) {
        return 0;
    }
    f = p->unk80;
    if (f & 0x1000) {
        ret |= 0x8000;
    }
    if (f & 0x2000) {
        ret |= 0x4000;
    }
    if (f7C & 0x800) {
        ret |= 0x2000;
    }
    f = p->unk8C | p->unk8E;
    if (f & 0xF00) {
        ret |= (f & 0xF00) >> 3;
    }
    if (f & 0xF000) {
        ret |= (f & 0xF000) >> 3;
    }
    return ret;
}
// PARKED (95.32%). Body COMPILES (park triage, round 111). 124 bytes of retail
// against 120 here, and the 4 missing bytes are retail's `addu v1,v0,zero` copy of
// the byte sum into the u8 result before the clamp compare - gcc 2.7.2 coalesces
// `u8 value = sum;` into the sum pseudo, so the body cannot link as it stands.
// What got it from 94.68 to 95.32: writing the row bump as an ASSIGNMENT
// (`p = p + arg1 * 0x28;`) gives retail's base-first `addu v1,v1,v0`, and forming
// q next to the two loads gives retail's two-addu-then-two-lbu order.
// Remaining gap: that copy, and a mirror register allocation (retail p byte in
// $v0, q byte in $v1; here the other way round). Tried: s32 sum + u8 copy, `+=` vs
// one expression, sum kept live via a wider compare, the 0x5B offset in a local
// (permuter found that one, no change), permuter 1837 iterations (best 91.8%).
/*
u8 func_8009A258(u8 arg0, u8 arg1) {
    s32 off = arg0 * 0x5F0 + 0x1058;
    u8* p = (u8*)off + (u32)D_800C34B0;
    u8* q;
    s32 sum;
    u8 value;

    p = p + arg1 * 0x28;
    q = (u8*)D_800C34B0 + arg0 * 0x170;
    sum = p[0x14] + q[0x5B];
    value = sum;
    if (value > 0x64) {
        value = 0x64;
    }
    return value;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009A258);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009A2D4);

u8 BattleGetGameCharacterField5(u8 arg0) {
    return g_GameState.characters[arg0].unk0[5];
}

s32 BattleIsCharacterInParty(u8 arg0) {
    u8 id = *(u8*)((u8*)D_800C34B0 + arg0 * 16 + 0x581E);

    s32 ret;

    if (g_GameState.characters[4].unk0[6] == id) {
        ret = 1;
    } else if (g_GameState.characters[4].unk0[0xE] == id) {
        ret = 1;
    } else if (g_GameState.characters[4].unk0[0x16] == id) {
        ret = 1;
    } else {
        ret = g_GameState.characters[4].unk0[0x1E] == id;
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
   unit uses the identical `g_GameState.characters[4].unk0[6]` expression and
   DOES fold, so the trigger is the store block coexisting with the loads; raw
   `((u8*)&g_GameState)[N]` and struct spellings both give 76.74%.
PARKED (76.74%)
void func_8009A854(u8 arg0, u8 arg1) {
    u8* p = (u8*)(arg0 * 16 + 0x54F8) + (u32)D_800C34B0;
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
        u8* q = (u8*)(i * 0x170 + (u32)D_800C34B0);

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
    D_800C34B0->unk5FC2 = 0;
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

    D_800C34B0->unk5FC2 = 0;
    q = &D_800C34B0->slots[arg0];
    q->state.unkB6 |= 1;
    p = &D_800C34B0->slots[arg0];
    if ((p->state.unkB6 & 0x80) && (p->state.unk82 & 0x10)) {
        p->state.unk7C &= 0xFE4F;
        p->unk7C &= 0xEFFF;
    }
    if (D_800D2C34 == 4) {
        D_800C34B0->unk5FC7 = 0x3D;
    }
}

u8 BattleClearSlotFlag15A1(u8 arg0) {
    BattleTableEntry* p = &D_800C34B0->slots[arg0];
    u8 flags = p->state.unkB6 & 0xFE;

    p->state.unkB6 = flags;
    return flags;
}

void BattleSetSlotEntryFieldsTo1(u8 arg0) {
    BattleTableEntry* p = &D_800C34B0->slots[arg0];
    BattleTableEntry* r;
    u16 f;

    if (p->state.unkB6 & 0x80) {
        f = p->state.unk80 & 0x2000;
    } else {
        f = p->unk88 & 0x400;
    }
    if (f == 0) {
        return;
    }
    r = &D_800C34B0->slots[arg0];
    if (r->state.unkB6 & 0x80) {
        s32 off = arg0 * 0x690 + 0x2228;
        BattleRow* pRow = (BattleRow*)((u8*)off + (u32)D_800C34B0);

        pRow[21].flags = 1;
        pRow[23].flags = 1;
        pRow[24].flags = 1;
        pRow[25].flags = 1;
        pRow[26].flags = 1;
        pRow[27].flags = 1;
        pRow[28].flags = 1;
    } else {
        s32 off = arg0 * 0x5F0 + 0x1058;
        BattleRow* pRow = (BattleRow*)((u8*)off + (u32)D_800C34B0);

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
        if (!(D_800C3DFC->unk0A & 0x100)) {
            return;
        }
    }
    p = &D_800C34B0->slots[arg0];
    if (p->state.unkB6 & 0x80) {
        f = p->state.unk80 & 0x2000;
    } else {
        f = p->unk88 & 0x400;
    }
    if (f == 0) {
        return;
    }
    r = &D_800C34B0->slots[arg0];
    if (r->state.unkB6 & 0x80) {
        s32 off = arg0 * 0x690 + 0x2228;
        BattleRow* pRow = (BattleRow*)((u8*)off + (u32)D_800C34B0);

        pRow[21].flags = 0x2000;
        pRow[23].flags = 0x2000;
        pRow[24].flags = 0x2000;
        pRow[25].flags = 0x2000;
        pRow[26].flags = 0x2000;
        pRow[27].flags = 0x2000;
        pRow[28].flags = 0x2000;
        r->state.unk80 &= 0xDFFF;
    } else {
        s32 off = arg0 * 0x5F0 + 0x1058;
        BattleRow* pRow = (BattleRow*)((u8*)off + (u32)D_800C34B0);

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
        r->unk88 &= 0xFBFF;
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
    u8* p = arg0 * 0x170 + (u32)D_800C34B0;
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
/*
Best attempt: 88.27%. Everything matches except the 3..11 loop: the original
   re-forms `lui at,%hi(D_800CCD68); addu at,at,v0` at BOTH memory references,
   while gcc hoists the symbol base into a register before the loop (loop.c moves
   the invariant), which then CSEs the address and drops 7 insns. Tried: plain
   `*(u16*)(D_800CCD68 + i*0x170) &= 0xFFDF`, the `((u16*)(...))[0] &=` spelling
   (86.36%), `u8 t` vs `s32 t` for the 0x56 temp. Everything else (two D_800C34B0
   loads, the four zero stores, the `f & 0x2000` store and the BattleSetActorScaledHp call)
   lines up exactly.
void func_8009AEFC(u8 arg0) {
    u8* p;
    u16 f;
    s32 t;

    *(u8*)((u8*)D_800C34B0 + 0x5FC2) = 0;
    p = arg0 * 0x170 + (u32)D_800C34B0;
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
void func_8009AFD8(void) {
    u8 sel = D_800C34B0->unk5FC2;
    s32 idx;

    switch (sel) {
    case 0:
    case 1:
    case 2:
    case 3:
        idx = D_800C3E00[0x6F];
        break;
    case 4:
    case 5:
        return;
    case 6:
        idx = D_800C3E00[0x72];
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
        idx = D_800C3E00[0x6F];
        if (g_GameState.gearPartsIds[0x6C + idx]) {
            g_GameState.gearPartsIds[0x6C + idx] = g_GameState.gearPartsIds[0x6C + idx] + 0xFF;
        }
        idx = D_800C3E00[0x72];
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
        BattleTableEntry* p = (BattleTableEntry*)(i * 0x170 + (u32)D_800C34B0);

        p->unk4C = 0x64;
        p->unk4E = 0x64;
        p->unk5C[2] = 0x14;
        p->unk5C[3] = 0xF;
        p->unk7A = 0x1FBF;
        p->state.unkA5 = 0;
    }
}

void BattleSetActorScaledHp(u8 arg0, BattleTableEntry* pArg) {
    BattleTableEntry* p = (BattleTableEntry*)(arg0 * 0x170 + (u32)D_800C34B0);
    BattleSlotState* pState = &p->state;

    if (pArg->unk4E * 50 > 0x1869F) {
        if (pArg->unk4C * 50 > 0x1869F) {
            p->state.unk60 = 0x1869F;
        } else {
            p->state.unk60 = pArg->unk4C * 50;
        }
        // only this store goes through the state pointer ($a3 = p + 0xA4) in retail
        pState->unk64 = 0x1869F;
    } else {
        p->state.unk60 = pArg->unk4C * 50;
        p->state.unk64 = pArg->unk4E * 50;
    }
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009B1E4);
// best 94.23% under gcc-2.6.0 (93.87% on 2.7.2, 94.10% cdk): the D_800CCE42
// base is hoisted out of the loop where retail re-forms lui/addiu per reference.
// PARKED (93.87%). Body COMPILES (park triage, round 111; it needed the extern u8
// D_800CCE42[] that is now at the top of the file). 532 bytes against retail's
// 536 - the missing instruction is the D_800CCE42 base retail re-forms per
// reference and gcc hoists, the same gap as func_8009AEFC above.
/*
Best attempt: 93.87%. Every instruction from the `p[0x56] == 3` test onward
   matches; the only difference is the D_800CCE42 access. The original re-forms
   `lui at,%hi(D_800CCE42); addu at,at,idx; lbu %lo(at)` at BOTH references,
   while gcc's loop.c hoists the symbol base into a register before the loop
   (`lui t1; addiu t1` at function entry) and then does `addu v0,idx,t1;
   lbu 0(v0)` -- 4 bytes larger, which shifts every branch target before the
   second block and swaps the max/cur registers ($a1/$v1). Same signature as the
   func_8009AEFC park (D_800CCD68) in this unit.
   Tried: `D_800CCE42[i*0x170]`, `((u32)i*0x170 + (u32)D_800CCE42)`,
   `((u8*)&D_800CCE42)[(u32)i*0x170]`, `((u8*)&D_800CCE42)[i*0x170]`,
   `*(volatile u8*)((u32)D_800CCE42 + (u32)i*0x170)`, gcc-2.7.2-cdk (94.10%),
   -O3 (93.87%) and -G0 (93.87%) -- all identical. Established from the matched
   functions in this overlay (BattleGetGameCharacterField5 here, BattleActorPosXGreaterThan in misc12) that
   gcc emits the $at form only when the symbol's address is credited to no
   pseudo at all; here the same `sym + i*0x170` value occurs in both blocks, so
   CSE credits it and the loop hoists it. Not source-reachable without changing
   the instruction multiset (a hand-written shift chain for one of the two
   indices would defeat the CSE but break retail's multiply-expansion chain).
   NOTE: this file needs `extern u8 D_800CCE42[];` (added) or the attempt below
   does not compile at all.
void func_8009B46C(u16* arg0) {
    u8 count = 0;
    u8 i;

    for (i = 0; i < 3; i++) {
        u8* p = (u8*)D_800C34B0 + i * 0x170;
        u8* q = p + 0xA4;
        u32 cur;
        u32 max;

        if (p[0x56] == 0) {
            if (D_800CCE42[i * 0x170] & 0x80) {
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
            if (D_800CCE42[i * 0x170] & 0x80) {
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
        f = *(u16*)(D_800C3E00 + 0x32);
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009B684);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009BAC4);
// best 98.33% (was 86.33%): only the last four instructions differ, and only by
// register: gcc puts the second D_800C3E50 load in $v0 where retail has $v1, so
// mfhi lands in $a1 instead of $v0. Permuter score 50, stuck over 1907+1250
// iterations, so this is an allocator tie and not a structural miss.
// The gain came from splitting the two statements' base loads: a u8* local for
// the first store, and a plain expression for the second so (u32)D_800C34B0 is
// loaded again; the int idx local stops the second lbu being CSE'd into the
// first, which is what lets the base reload schedule after multu.
// Also tried, all 98.33 or worse: index-first addu operand order (97.33), two
// separate locals for idx and address, declaration order swapped, u8* address
// local, u32 quotient local stored separately, 20u divisor, dropping the idx
// temp, u32* destination pointer local (70.17, CSEs the base).
// PARKED (98.33%). Body COMPILES again (park triage, round 111): the park spelled
// D_800C3DFC[0x11] from a time when the symbol was a u8*; it is a BattleRow*
// now, so the same byte is D_800C3DFC->unk11 (identical code). Only the last
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
// (u32)D_800C34B0 is loaded again; the int idx local stops the second lbu being
// CSE'd into the first, which is what lets the base reload schedule after multu.
// Also tried, all 98.33 or worse: index-first addu operand order (97.33), two
// separate locals for idx and address, declaration order swapped, u8* address
// local, u32 quotient local stored separately, 20u divisor, dropping the idx
// temp, u32* destination pointer local (70.17, CSEs the base). Permuter score 50,
// stuck over 1907+1250 iterations.
// PARKED (98.33%). Body compiles; D_800C3DFC is a BattleRow* so the old
// D_800C3DFC[0x11] spelling is now ->unk11 (identical code).
/*
void func_8009BD94(void) {
    int new_var;
    u8* new_var2;

    new_var2 = (u8*)D_800C34B0;
    *(u8*)(new_var2 + D_800C3E50 + 0x5FA0) = 2;
    new_var = D_800C3E50;
    new_var = (u32)D_800C34B0 + new_var * 4 - -0x5F6C;
    *(u32*)new_var =
        (u32)(D_800C3DFC->unk11 * *(s32*)(D_800D2DC8 + 0x64)) / 20;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009BD94);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc19", func_8009BE0C);
u8 BattleGetActorActionConditionMask(u8 arg0) {
    BattleTableEntry* pEntry = (BattleTableEntry*)(arg0 * 0x170 + (u32)D_800CCCE8);
    BattleSlotState* pState = (BattleSlotState*)(arg0 * 0x170 + 0xA4 + (u32)D_800CCCE8);
    u8* pCondition = (u8*)D_800CCCE8 + 0x148 + arg0 * 0x170;
    s32 flag = pState->unk7C & 0x400;
    u8 ret = flag != 0;

    if (pState->unk60 < ((u32)pState->unk64 >> 3)) {
        if (!(*(u16*)(pEntry->unk04 + 0x32) & 1)) {
            ret |= 2;
        }
    }
    if (*pCondition == 4) {
        ret |= 4;
    }
    return ret;
}
void BattleSetSlotFields148And162(u8 arg0) {
    D_800CCCE8[arg0].state.unkA4 = 4;
    D_800CCCE8[arg0].state.unkB7[7] = 3;
    g_GameState.unk16BC[0].unk1E |= 0x4000;
}
