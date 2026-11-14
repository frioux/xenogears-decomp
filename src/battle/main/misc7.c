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
u16 func_80089C08(u8);
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
/*
void func_8007B424(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pTable = (u8 *)pField + 0x10;
    u8 value = BattleFindFirstFlagTableMatch(pField[(*p)[3]]);

    pTable[(*p)[1]] = func_80079ED8(value, (*p)[2], 0, 1);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007B424);
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
// best: 89.53%. Structurally complete (both cases, the -0x10 derived from the
// D_800D3420 base in the jal delay slot, u8 value local, g_GameState.gold), but
// the original keeps slot and base in separate callee-saved regs (s1, s0) and
// combines them after the call, gcc combines them before and needs one saved
// register fewer (228 vs 224 bytes). 85.93% with the case-2 pointer inlined.
// screened: 89.53% is the max of Default / cdk 78.78 / 2.6.0 89.53.
/*
void func_8007B6C0(u8 **p, u8 index) {
    switch ((*p)[2]) {
    case 1: {
        u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
        u32 *pField32 = (u32 *)((u8 *)pField - 0x10);
        u8 value = BattleFindFirstFlagTableMatch(pField[(*p)[3]]);

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
// best: 87.67%. Body recovered from the .s (u8 return: ret = arg2, the
// D_800D2E5C = 0x20 / arg2+1 path for value < 3, then a two-way store of
// ((u32 *)((u8 *)pField - 0x10))[(*p)[1]] into D_800CCDF0 / D_800CCDEC +
// value * 0x170 selected by (*p)[2]). Sole gap: retail keeps D_800D3420 in one
// saved register (s3) and index * 0x40 in another (s0) across the call and
// re-forms base-0x10 with `addiu s3,-0x10` in each branch, gcc forms the whole
// pField sum once (frame 0x20 instead of 0x28) and folds the -0x10 into the
// load displacement. Spelling D_800D3420 - 0x10 inline instead of pField makes
// the frame 0x30 and drops it to 76.07%; a hoisted pField32 local gives 82.36%.
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
            ((u32 *)((u8 *)pField - 0x10))[(*p)[1]];
    } else {
        *(u32 *)(D_800CCDEC + value * 0x170) =
            ((u32 *)((u8 *)pField - 0x10))[(*p)[1]];
    }
    return ret;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007B7B0);
// best: 73.75%. Instruction multiset identical; the original loads *p into v0
// and puts the D_800D3420 base in a0, gcc emits the base first instead. Same
// allocator tie as the rest of the D_800D34x0 operators in misc6.
// screened: 73.75% is the max of Default / cdk 50.94 / 2.6.0 73.75.
/*
void func_8007B8D4(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pArgs = *p;

    pField[pArgs[1]] = D_8005A3A0[pArgs[2]];
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
// best: 84.67%. Instruction multiset identical; the original loads *p into v0
// before materialising the D_800D3420 base in a0, gcc always emits the base
// first and reuses the dead a0 for the load. Declaration order, an explicit
// dst local, a bare *p inline twice and a volatile load all leave it unchanged,
// and 2.6.0 / 2.7.2-cdk both give the same 84.67%.
// screened: identical 84.67% under Default, 2.7.2-cdk and 2.6.0 - a register
// allocation tie, not a compiler question.
/*
void func_8007B98C(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pArgs = *p;

    pField[pArgs[2]] = pField[pArgs[1]];
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007B98C);
// best: 84.67%. Instruction multiset identical; the original loads *p into v0
// and puts the D_800D3410 base in a0, gcc emits the base first instead. Same
// allocator tie as func_8007B98C.
// screened: identical 84.67% under Default, 2.7.2-cdk and 2.6.0 - a register
// allocation tie, not a compiler question.
/*
void func_8007B9C8(u8 **p, u8 index) {
    u32 *pField = (u32 *)(D_800D3410 + index * 0x40);
    u8 *pArgs = *p;

    pField[pArgs[2]] = pField[pArgs[1]];
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007B9C8);
// best: 95.94%. Pure register renaming (gcc swaps v0 and v1 and keeps the *p
// load in v1 instead of reusing the dead a0); identical multiset, unchanged by
// gcc 2.6.0 and 2.7.2-cdk. The D_800D3430 base is already derived as
// D_800D3420+0x10 like the original.
// screened: identical 95.94% under Default, 2.7.2-cdk and 2.6.0 - a register
// allocation tie, not a compiler question.
/*
void func_8007BA04(u8 **p, u8 index) {
    u16 *pField = (u16 *)(D_800D3420 + index * 0x40);
    u8 *pTable = D_800D3420 + 0x10 + index * 0x40;
    u8 *pArgs = *p;

    pField[pArgs[2]] = pTable[pArgs[1]];
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007BA04);
// best: 95.29%. Only the register holding the stored halfword differs
// (original a0, gcc v1); everything else, including the D_800D3420 base
// derived as D_800D3410+0x10, is identical.
// screened: identical 95.29% under Default, 2.7.2-cdk and 2.6.0 - a register
// allocation tie, not a compiler question.
/*
void func_8007BA44(u8 **p, u8 index) {
    u32 *pField32 = (u32 *)(D_800D3410 + index * 0x40);
    u16 *pField = (u16 *)(D_800D3410 + 0x10 + index * 0x40);
    u8 *pArgs = *p;

    pField32[pArgs[2]] = pField[pArgs[1]];
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007BA44);
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
// best: 96.76%. Only the register holding *p differs (original v1, gcc
// reuses a0). The two-statement slot local is needed: written as
// (index + 3) * 0x170 inside the store, gcc folds 3*0x170 into the
// displacement instead of the addiu the original has. Hoisting the value
// into a u32 local (which works for BattleSlotStoreWord7A) drops it to 41.18%.
// screened: 96.76% is the max of Default / cdk 53.82 / 2.6.0 96.76.
/*
void func_8007BAE8(u8 **p, u8 index) {
    s32 slot = index + 3;

    *(u32 *)(D_800CCE34 + slot * 0x170) = (*p)[1] | ((*p)[2] << 8);
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007BAE8);
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
                    pField[(*p)[1]] = func_80089C08(idx);
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
                ((u16 *)(D_800D3420 + index * 0x40))[(*p)[1]] = func_80089C08(idx);
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
            func_80089C08(buf[func_8001BD40(0, count - 1)]);
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc7", func_8007C040);
