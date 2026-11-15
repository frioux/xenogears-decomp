#include "common.h"
#include "main/game.h"

// Operator argument tables: D_800D3420 is the 16-bit field table, D_800D3430
// the 8-bit one, D_800D3410 the 32-bit one; all are indexed by slot * 0x40.
extern u8 D_800D3410[];
extern u8 D_800D3420[];
extern u8 D_800D342E[];
extern u8 D_800D3430[];
extern u8 D_800D343F[];
extern u16 D_8005A3A0[];
extern u8 D_800D2E5C;
extern u8 D_800D32A1[];
extern u8 D_800C3EB4[];
extern u8 D_800CCDEC[];
extern u8 D_800CCDF0[];
extern u8 D_800CCE34[];
extern u8 D_800CCE38[];
extern u8 D_800CCE39[];
extern u8 D_800CCE3A[];
extern u8 D_800CCE3B[];
extern u8 D_800CCE3C[];
extern u8 D_800CCE3D[];
extern u8 D_800CCE3E[];

u8 func_8001BD40(int, u8);
u16 func_80089B50(int, u16);
u8 func_8007A628(u8, u8);
u16 BattleSlotBitMask(u8);
s32 BattleFindFirstFlagTableMatch(u16);
s32 func_80079ED8(u8, u8, u8, s32);
s32 func_8007A280(u8, u8, s32, s32);

// Preset screened twice, both times Default wins. gcc 2.7.2-cdk is ruled out:
// the 11 matched functions drop to 41.73-99.09% and, with maspsx
// --aspsx-version=2.67 as well, BattleClearByteRegs/BAB8 fall to 95.00% too. gcc 2.6.0
// keeps all 11 matched functions at 100.00% and, of the 13 bodies parked below,
// only func_8007C040 scores better (72.75% vs 65.42%) and it is still far off.
// So every park here is an allocator tie under all three, not a wrong compiler.

// best: 90.81%. The pTable = (u8 *)pField + 0x10 form is what puts the
// addiu 0x10 in the jal delay slot like the original. Remaining gap is the
// allocator: the original keeps slot in s0 and base+0x10 in s1, gcc folds
// slot+base into s0 before the call and needs one saved register fewer.
// screened: 90.81% is the max of Default / 2.7.2-cdk 80.16 / 2.6.0 90.81.
void func_8007B424(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 value = BattleFindFirstFlagTableMatch(pField[(*p)[3]]);
    u8 result = func_80079ED8(value, (*p)[2], 0, 1);
    u8 *pTable = (u8 *)(D_800D3420 + 0x10) + index * 0x40;

    pTable[(*p)[1]] = result;
}
// best: 81.15%. Frame and register set now match; the only gap is that the
// original materialises D_800D3420+0x10 with its own addiu and then adds slot
// and the arg byte, gcc folds the 0x10 into the lbu displacement. 62.17% if
// pTable is a pointer local declared before the call.
// screened: 81.15% is the max of Default / cdk 78.94 / 2.6.0 81.15.
/*
u8 func_8007B4B8(u8 **p, u8 index, u8 arg2) {
    s32 slot = index * 0x40;
    u16 *pField = (u16 *)(D_800D3420 + slot);
    u8 ret = arg2;
    u8 value = BattleFindFirstFlagTableMatch(pField[(*p)[3]]);

    if (value < 3) {
        D_800D2E5C = 0x20;
        ret = arg2 + 1;
    } else {
        func_80079ED8(value, (*p)[2], (D_800D3420 + 0x10 + slot)[(*p)[1]], 0);
    }
    return ret;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007B4B8);
void BattleWordRegCopyFromMatchedSlot(u8 **p, u8 index) {
    u16 *pTable = (u16 *)(D_800D3420 + index * 0x40);
    u8 value = BattleFindFirstFlagTableMatch(pTable[(*p)[3]]);

    pTable[(*p)[1]] = func_8007A280(value, (*p)[2], 0, 1);
}
u8 BattleWordRegCopyToMatchedSlot(u8 **p, u8 index, u8 arg2) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 ret = arg2;
    u8 value = BattleFindFirstFlagTableMatch(pField[(*p)[3]]);

    if (value < 3) {
        D_800D2E5C = 0x20;
        ret = arg2 + 1;
    } else {
        func_8007A280(value, (*p)[2], pField[(*p)[1]], 0);
    }
    return ret;
}
// best: 96.17%. Both arms and every instruction now match; the whole gap is
// one allocator coin: the top `*p` load result and the switch discriminant
// keep swapped registers (retail: pArgs in a0 from `lw a0,0(s2)`, discriminant
// in a2; gcc: pArgs a2, discriminant a0). Case 2's `lbu v0,1(a0)` then also
// lands after the D_800D3410 base instead of before it, which is a knock-on of
// the same swap (retail can only load it early because the base reuses a0).
// Structure that got it here from the 89.53 park: keep slot and base apart so
// the sums form at their points of use - case 1 spells the field-table base
// `(u8 *)(D_800D3420 - 0x10) + index * 0x40` (the -0x10 lands in the jal delay
// slot) instead of deriving it from pField, and the D_800DCCDEC load and the
// store keep retail's order.
// Tried and no better: `switch ((*p)[2])` with (*p)[k] inline 96.17, a `u8 kind`
// local fed from (*p)[2] 96.17, `u8 *pArgs` used for all three subscripts 89.22
// (pArgs then spans the call and takes s2), a u8 index local for case 2 96.17,
// permuter 2423 iterations base=230 best=200 (its best candidate hoists the
// case-2 index load into a volatile temp - the load order moves, the a0/a2 swap
// does not).
// screened: 89.53% was the max of Default / cdk 78.78 / 2.6.0 89.53.
/*
void func_8007B6C0(u8 **p, u8 index) {
    u8 *pArgs = *p;

    switch (pArgs[2]) {
    case 1: {
        u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
        u8 value = BattleFindFirstFlagTableMatch(pField[(*p)[3]]);
        u32 *pField32 = (u32 *)((u8 *)(D_800D3420 - 0x10) + index * 0x40);

        pField32[(*p)[1]] = *(u32 *)(D_800CCDEC + value * 0x170);
        break;
    }
    case 2: {
        u32 *pField32 = (u32 *)(D_800D3410 + index * 0x40);

        pField32[(*p)[1]] = g_GameState.gold;
        break;
    }
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007B6C0);
// best: 88.95%. Body recovered from the .s: u8 return (ret = arg2), the
// D_800D2E5C = 0x20 / arg2+1 path for value < 3, then (*p)[2] == 0 selects
// D_800CCDF0 and everything else D_800CCDEC, storing the
// ((u32 *)((u8 *)D_800D3420 - 0x10))[index * 0x40 + (*p)[1]] dword into
// value * 0x170 of it. Writing that field-table address INLINE, rather than
// deriving it from pField, is what took the park from 87.67 to 88.95: the base
// stays in s3 and index * 0x40 in s0 across the call, like retail. Two gaps
// left, both allocator ties:
//  - in each arm gcc folds the -0x10 into the load displacement and sums
//    index*4 + slot first (`lw v1,-16(v1)`); retail does `addiu v1,s3,-0x10`,
//    `addu v1,s0,v1`, then adds the index. A `u8 *pField32` local inside the
//    arm instead drops to 80.92 (p -> s2, arg2 -> s3, base -> s1).
//  - the u8 `value` lands in a1 (the dead index parameter) where retail uses
//    v1; every other instruction matches.
// The store direction matters: arg2 is the return value, the field-table dword
// goes INTO D_800CCD*, not the other way round.
// screened: 87.67% was the max of Default / cdk 78.94 / 2.6.0 87.67.
/*
u8 func_8007B7B0(u8 **p, u8 index, u8 arg2) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 ret = arg2;
    u8 value = BattleFindFirstFlagTableMatch(pField[(*p)[3]]);

    if (value < 3) {
        D_800D2E5C = 0x20;
        ret = arg2 + 1;
    } else if ((*p)[2] == 0) {
        *(u32 *)(D_800CCDF0 + value * 0x170) =
            ((u32 *)((u8 *)(D_800D3420 - 0x10) + index * 0x40))[(*p)[1]];
    } else {
        *(u32 *)(D_800CCDEC + value * 0x170) =
            ((u32 *)((u8 *)(D_800D3420 - 0x10) + index * 0x40))[(*p)[1]];
    }
    return ret;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007B7B0);
// best: 98.00%, permuter score 40. Sole gap: the pArgs[2] table index and the
// pArgs[1] slot index keep swapped registers (retail puts the loaded value in
// v0 and the slot index in v1, gcc the reverse); multiset identical.
// The value temp computed before pField is what took it from 73.75 to 98.00.
// Tried and no better: inline rhs 73.75, value with initializer 71.75, pArgs
// declared first 71.75, assignment inside the store expr 73.75, u8 index temp
// 73.75, s16 value 98.00, u8 pointer arithmetic store 98.00, 3121 permuter
// iterations at base 40.
// screened: 73.75% was the max of Default / cdk 50.94 / 2.6.0 73.75.
/*
void func_8007B8D4(u8 **p, u8 index) {
    u16 value;
    u16 *pField;
    u8 *pArgs = *p;

    value = D_8005A3A0[pArgs[2]];
    pField = (u16 *)(D_800D3420 + index * 0x40);
    pField[pArgs[1]] = value;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007B8D4);
void BattleWordTableStoreReg(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pArgs = *p;

    D_8005A3A0[pArgs[2]] = pField[pArgs[1]];
}
void BattleByteRegSetReg(u8 **p, u8 index) {
    u8 *pTable = D_800D3430 + index * 0x40;
    u8 *pArgs = *p;

    pTable[pArgs[2]] = pTable[pArgs[1]];
}
void BattleWordRegSetReg(u8 **p, u8 index) {
    // The dead pField is load-bearing: its initializer makes gcc emit the
    // index*0x40 shift before the base lui/addiu, as retail does.
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pArgs = *p;
    u8 *base;

    base = D_800D3420;
    ((u16 *)(base + index * 0x40))[pArgs[2]] = ((u16 *)(base + index * 0x40))[pArgs[1]];
}
// best: 85.33%, permuter score 80. The copy inside the second subscript closed
// everything except the prologue: retail loads *p into v0 first and then forms
// the D_800D3410 base into a0 (reusing dead p); gcc forms the base into v1 (a0
// when the do-while wrapper is dropped, 84.67) and loads *p after. Same
// allocator tie as BattleWordRegSetReg. The do-while wrapper is load-bearing.
// Tried and no better: base u8 local 73.33, base local plus dead pField 66.27,
// *p written inline twice 78.67, u32 value temp 67.87, pre-assigned pArgsCopy
// 84.67, pField + (pArgs - pArgs) to delay the base 85.33, 3025 permuter
// iterations from base 80.
// The base-first order survives every source order tried (round 76): pArgs
// declared first 84.67, both bases inline 74.33, two-statement base split 58.67,
// u8 slot copy 84.67, volatile *p read 84.67, index term first 84.67, typed
// u32* base + index * 16 84.67, load and base in one do-while 82.67, plus a
// third permuter run (2026 iterations, base=80 best=80). That it is emission
// order and not an allocator tie is shown by BattleByteRegSetReg, the same
// table family one function earlier: it is matched with retail's base-then-load
// order, so retail's load-first here needs a dependency the source has none of.
// screened: identical 84.67% under Default, 2.7.2-cdk and 2.6.0 - a register
// allocation tie, not a compiler question.
// ROUND 78: the emission order IS movable, and the remaining diff is narrower
// than "the load must come first". Two shapes get retail's order:
//   a) `s32 slot = index * 0x40;` declared after pArgs, base spelled INLINE in
//      both subscripts ((u32 *)(D_800D3410 + slot))[k] -> andi, sll, lw v0,0(a0),
//      lui a0, addiu a0 exactly; 79.67% because gcc then sums slot+base twice
//      (16 insns, addu v1,v1,a1 / addu v1,v1,a0 pairs).
//   b) the pField local + both stores wrapped in do { } while (0) AFTER the
//      load -> the load is hoisted to the first instruction, then andi/sll,
//      then lui/addiu; 84.67% with the load in a0 and the base in v0, i.e. the
//      exact mirror of (a) and of retail (retail: load v0, base a0).
// So retail's build gave the register freed by the dead parameter to the BASE
// and my builds give it to the LOAD; every other insn matches in (b). Next
// lever to try: something that makes the load's result not tie to a0 (a second
// use of p, a u8 copy of the loaded pointer, the base written before the load
// inside the do-while but assigned after it) - not more statement reordering.
/*
void func_8007B9C8(u8 **p, u8 index) {
    u8 *pArgs = *p;
    u8 *pArgsCopy;
    u32 *pField;

    pField = (u32 *)(D_800D3410 + index * 0x40);
    do {
        pField[pArgs[2]] = pField[(pArgsCopy = pArgs)[1]];
    } while (0);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007B9C8);
// best: 98.12%. Sole gap: the pTable sum is formed into the base register (v0)
// where retail reuses the index register (a1); every other register and
// instruction matches, including the D_800D3430 base derived as
// D_800D3420+0x10. Permuter score 30 is this body and 778 more iterations found
// nothing better; writing the index term first fixes the addu operand order but
// moves the *p load to v1 and drops to 95.94%.
// screened: the plain pTable form is identical 95.94% under Default, 2.7.2-cdk
// and 2.6.0 - a register allocation tie, not a compiler question.
// Round 79: a third permuter run (1903 iterations) from base 30 also found
// nothing. The tie is exactly two coins: pTable's dst (retail a1 = the slot
// register, so the sum is (plus slot base2)) and the *p load dst (retail a0,
// so retail's pTable dst and pArgs do NOT collide). Every shape that commutes
// the sum (a `u32 slot` local, `index * 0x40 + (u8 *)(D_800D3420 + 0x10)`,
// the same with (*p)[1] inline so the load is CSE'd) gives retail's
// `addu a1,a1,v0` but moves the load to v1 -> 95.94. Spelling D_800D3430
// directly gives it its own lui/addiu (88.38), so the +0x10 really is derived
// from the base.
/*
void func_8007BA04(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pTable;
    u8 *pArgs = *p;

    pTable = (D_800D3420 + 0x10) - (-(index * 0x40));
    pField[pArgs[2]] = pTable[pArgs[1]];
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007BA04);
void BattleSlotStoreDwordFromWordReg(u8 **p, u8 index) {
    u32 *pField32 = (u32 *)(D_800D3410 + index * 0x40);
    u16 *pWordRegs;
    u8 *pArgs = *p;

    // the assignment inside the subscript is load-bearing: with a plain local
    // gcc picks v1 for the stored halfword instead of retail's a0 (95.29%).
    pField32[pArgs[2]] = (pWordRegs = (u16 *)(D_800D3410 + 0x10 + index * 0x40))[pArgs[1]];
}
void BattleClearByteRegs(u8 index) {
    s32 i = 0xF;
    u8 *p = D_800D343F + index * 0x40;

    for (; i >= 0; i--) {
        *p-- = 0;
    }
}
void BattleClearWordRegs(u8 index) {
    s32 i = 0x7;
    u16 *p = (u16 *)(D_800D342E + index * 0x40);

    for (; i >= 0; i--) {
        *p-- = 0;
    }
}
void BattleSlotStoreWord0(u8 **p, u8 index) {
    s32 slot = index + 3;
    s32 hi = (*p)[2] << 8;

    *(u32 *)(D_800CCE34 + slot * 0x170) = (*p)[1] | hi;
}
void BattleSlotStoreWord7A(u8 **p, u8 index) {
    u32 value = (*p)[1] | ((*p)[2] << 8);
    s32 slot = index + 3;

    *(u16 *)(D_800CCE3E + slot * 0x170) = value;
}
void BattleSlotStoreWordHighBytes(u8 **p, u8 index) {
    s32 slot = index + 3;

    *(u8 *)(D_800CCE3D + slot * 0x170) = (*p)[1];
    *(u8 *)(D_800CCE3B + slot * 0x170) = (*p)[2];
    *(u8 *)(D_800CCE39 + slot * 0x170) = (*p)[3];
}
void BattleSlotStoreWordLowBytes(u8 **p, u8 index) {
    s32 slot = index + 3;

    *(u8 *)(D_800CCE3C + slot * 0x170) = (*p)[1];
    *(u8 *)(D_800CCE3A + slot * 0x170) = (*p)[2];
    *(u8 *)(D_800CCE38 + slot * 0x170) = (*p)[3];
}
// best: 98.82%. Only the last addu is commuted (original base first into a1,
// gcc reuses the dead a2): identical multiset, same 98.82% under gcc 2.6.0 and
// 2.7.2-cdk. Writing *p inline twice is required, the second load is CSE'd if
// it goes through a local.
// screened: identical 98.82% under Default, 2.7.2-cdk and 2.6.0 - a register
// allocation tie, not a compiler question.
/*
void func_8007BC40(u8 **p, u8 *pDest, u8 count) {
    pDest[count * 8 + (*p)[1]] = (*p)[2];
    pDest[count * 8 + ((*p)[1] + 1)] = (*p)[3];
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007BC40);
void BattleByteRegSetRandom(u8 **p, u8 index) {
    u8 value = func_8001BD40(0, (*p)[2]);
    u8 *pTable = D_800D3430 + index * 0x40;

    pTable[(*p)[1]] = value;
}
void BattleWordRegSetRandomRange(u8 **p, u8 index) {
    u16 value = func_80089B50(0, (*p)[2] | ((*p)[3] << 8));
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);

    pField[(*p)[1]] = value;
}
// best: 91.53%. Structure recovered (3-byte `buf[0] = buf[1] = buf[2] = 0`
// is what gives the three sb zero; `while (!(buf[0] & buf[1] & buf[2]))`
// reproduces both copies of the test). Sole gap: gcc hoists the loop-
// invariant absolute base of D_800D32A1 into a callee-saved register (s5)
// at the preheader, so the frame is 56 instead of 48 and pField is added
// straight into s4 instead of a1. Retail re-forms `lui $at,%hi(D_800D32A1)`
// every iteration, i.e. its RTL keeps the symbol inside the mem address.
// Unchanged by nested ifs, `&&`, -G0 and 2.6.0/cdk.
// screened: 91.53% is the max of Default / cdk 88.92 / 2.6.0 91.53.
/*
void func_8007BD5C(u8 **p, u8 index) {
    u8 buf[3];
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 idx;

    buf[0] = buf[1] = buf[2] = 0;
    pField[(*p)[1]] = 0;
    while (!(buf[0] & buf[1] & buf[2])) {
        idx = func_8001BD40(0, 2);
        if (buf[idx] == 0) {
            if (func_8007A628(idx, (*p)[2]) != 0) {
                if (D_800D32A1[idx * 8] == 0) {
                    pField[(*p)[1]] = BattleSlotBitMask(idx);
                    break;
                }
            }
            buf[idx] = 1;
        }
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007BD5C);
// best: 76.81%. Same body as func_8007BD5C plus the D_800C3EB4 entry
// comparison; the frame/register set is off for the same reason - gcc
// hoists the absolute bases of D_800C3EB4 and D_800D32A1 into extra
// callee-saved registers instead of re-forming `lui %hi` each iteration
// like retail (which only strength-reduces the *offsets*: s3 starts at
// 0x54 and steps by 0x1C). Nothing found yet that keeps the symbol inside
// the mem address.
// screened: cdk 77.76% and 2.6.0 76.81% against 76.81% Default - not close.
/*
void func_8007BEA8(u8 **p, u8 index) {
    u8 buf[3];
    u8 idx;

    buf[0] = buf[1] = buf[2] = 0;
    ((u16 *)(D_800D3420 + index * 0x40))[(*p)[1]] = 0;
    while (!(buf[0] & buf[1] & buf[2])) {
        idx = func_8001BD40(0, 2);
        if (buf[idx] == 0) {
            if (func_8007A628(idx, (*p)[2]) != 0
                && D_800C3EB4[(index + 3) * 0x1C] == D_800C3EB4[idx * 0x1C]
                && D_800D32A1[idx * 8] == 0) {
                ((u16 *)(D_800D3420 + index * 0x40))[(*p)[1]] = BattleSlotBitMask(idx);
                break;
            }
            buf[idx] = 1;
        }
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007BEA8);
// best: 65.42%. Structure recovered (collect the 3..10 slots whose
// D_800C3EB4 entry equals the (index+3) one into a stack buffer, then pick
// one at random; both D_800D3420 stores are spelled inline, retail
// re-forms their base with a fresh lui as well). Gap is the same hoist as
// func_8007BEA8 (D_800C3EB4/D_800D32A1 bases into callee-saved registers,
// +8 frame). -G0 changes nothing.
// screened: 2.6.0 gives 72.75% vs 65.42% Default, still far off; cdk 65.48%.
/*
void func_8007C040(u8 **p, u8 index) {
    u8 buf[9];
    s32 count = 0;
    s32 i;

    ((u16 *)(D_800D3420 + index * 0x40))[(*p)[1]] = 0;
    for (i = 3; i < 11; i++) {
        if (func_8007A628(i, (*p)[2]) != 0
            && D_800C3EB4[(index + 3) * 0x1C] == D_800C3EB4[i * 0x1C]
            && D_800D32A1[i * 8] == 0) {
            buf[count++] = i;
        }
    }
    if (count != 0) {
        ((u16 *)(D_800D3420 + index * 0x40))[(*p)[1]] =
            BattleSlotBitMask(buf[func_8001BD40(0, count - 1)]);
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007C040);
