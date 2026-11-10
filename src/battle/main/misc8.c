#include "common.h"
#include "main/game.h"

// A 0x170-byte battle table entry; only the flag word at +0 is read here.
typedef struct {
    /* 0x000 */ u16 flags;
    /* 0x002 */ u8 unk02[0x16E];
} BattleTableEntry; // Size: 0x170

extern BattleTableEntry D_800CCD64[];
extern u8 D_800C3EB4[];
extern u8 D_800C3EB7[];
extern u8 D_800D2C8B[];
extern u8 D_800D2CB0[];
extern u8 D_800D2CE0[];
extern u8 D_800D2DCC[];
extern u8 D_800D2E0C[];
extern u8 D_800D32A1[];
extern u8 D_800CCDEC[];
extern u8 D_800D3410[];
extern u8 D_800D3420[];
extern u8 D_800D3430[];
extern u8 func_8007A628(u8 arg0, u8 arg1);
extern u16 func_80089C08(u8 arg0);
extern u8 D_800D2E06[];
extern u8 *D_800D3364;
extern u8 D_800CCD34[];
extern u8 func_80079E7C(u16 arg0);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007C1A4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007C33C);
void func_8007C4A0(u8 **arg0, u8 arg1) {
    u16 *pRow;
    u16 result;
    u8 best = 0xFF;
    s32 bestIndex = 0;
    s32 i;

    for (i = 0; i < 3; i++) {
        if (func_8007A628(i, (*arg0)[2]) != 0 && best >= *(s16 *)&D_800D2E06[i * 2]) {
            best = D_800D2E06[i * 2];
            bestIndex = i;
        }
    }
    result = func_80089C08(bestIndex);
    pRow = (u16 *)(D_800D3420 + arg1 * 0x40);
    pRow[(*arg0)[1]] = result;
}
void func_8007C580(u8 **arg0, u8 arg1) {
    u16 *pRow;
    u16 result;
    u8 best = 0xFF;
    s32 bestIndex = 0;
    s32 i = 3;
    s32 slot = arg1 + 3;

    for (; i < 0xB; i++) {
        if (func_8007A628(i, (*arg0)[2]) != 0 && best >= *(s16 *)&D_800D2E0C[(i - 3) * 2] && i != slot) {
            best = D_800D2E0C[(i - 3) * 2];
            bestIndex = i;
        }
    }
    result = func_80089C08(bestIndex);
    pRow = (u16 *)(D_800D3420 + arg1 * 0x40);
    pRow[(*arg0)[1]] = result;
}
void func_8007C678(u8 **arg0, u8 arg1) {
    u16 *pRow;
    u16 result;
    u16 best = 0xFFFF;
    s32 bestIndex = 0;
    s32 i;
    s32 offset;

    for (i = 0, offset = 0; i < 3; i++, offset += 0x170) {
        if (func_8007A628(i, (*arg0)[2]) != 0 && best >= *(u16 *)(D_800CCD34 + offset)) {
            best = *(u16 *)(D_800CCD34 + offset);
            bestIndex = i;
        }
    }
    result = func_80089C08(bestIndex);
    pRow = (u16 *)(D_800D3420 + arg1 * 0x40);
    pRow[(*arg0)[1]] = result;
}
void func_8007C75C(u8 **arg0, u8 arg1) {
    u16 *pRow;
    u16 result;
    u16 best = 0xFFFF;
    s32 bestIndex = 0;
    s32 i;
    s32 offset;

    for (i = 3, offset = 0x450; i < 0xB; i++, offset += 0x170) {
        if (func_8007A628(i, (*arg0)[2]) != 0 && best >= *(u16 *)(D_800CCD34 + offset)) {
            best = *(u16 *)(D_800CCD34 + offset);
            bestIndex = i;
        }
    }
    result = func_80089C08(bestIndex);
    pRow = (u16 *)(D_800D3420 + arg1 * 0x40);
    pRow[(*arg0)[1]] = result;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007C840);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007C9D4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007CB20);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007CC50);
/* best: 95.10%. A source-level offset variable is what produces the
   D_800C3EB4 at-register (lui at/%hi, addu at,at,s1, lbu %lo) form; the
   earlier i * 0x1C spelling got the symbol folded into a pointer (87.29%).
   Only reorg placement is left: the original steals i++ into the post-call
   beqz delay slot and duplicates the loop's slti into the bne delay slot,
   gcc emits i++ after count++ and a single slti.

void func_8007CC50(u8 **arg0, u8 arg1) {
    u8 *pRow;
    s32 count = 0;
    s32 i;
    s32 offset;

    for (i = 0, offset = 0; i < 3; offset += 0x1C, i++) {
        if (func_8007A628(i, 0) != 0 && D_800C3EB4[offset] == (*arg0)[2]) {
            count++;
        }
    }
    pRow = &D_800D3430[arg1 * 0x40];
    pRow[(*arg0)[1]] = count;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007CD10);
/* best: 87.29%. func_8007CC50 with the loop bounds 3..0xB; same miss (the
   D_800C3EB4 offset induction and the i++ delay slot).

void func_8007CD10(u8 **arg0, u8 arg1) {
    u8 *pRow;
    s32 count = 0;
    s32 i;

    for (i = 3; i < 0xB; i++) {
        if (func_8007A628(i, 0) != 0 && D_800C3EB4[i * 0x1C] == (*arg0)[2]) {
            count++;
        }
    }
    pRow = &D_800D3430[arg1 * 0x40];
    pRow[(*arg0)[1]] = count;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007CDD0);
/* best: 78.26%. Every instruction of the loop/tail is present; only the
   callee-saved set and two intra-block orders differ: the original keeps the
   D_800D3420 base in s0 across the func_80079E7C call (4 saved regs), gcc
   folds the whole row base into s1 before the call (3 saved regs), and it
   emits the two D_800C3EB4 loads in the opposite order. Tried: pRow/pField as
   separate locals, slot = arg1 + 3 assigned after the call, and the two table
   bytes split into operator/row locals - all three give the same code.

void func_8007CDD0(u8 **arg0, u8 arg1) {
    u16 *pField = (u16 *)(D_800D3420 + arg1 * 0x40);
    u8 *pRow = D_800D3420 + 0x10 + arg1 * 0x40;
    u8 value = func_80079E7C(pField[(*arg0)[2]]);
    s32 slot = arg1 + 3;
    u8 operator = D_800C3EB4[value * 0x1C];
    u8 row = D_800C3EB4[slot * 0x1C];

    pRow[(*arg0)[1]] = D_800D3364[operator * 8 + row * 0x40 + 0x140];
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007CEA4);
/* best: 72.38%. Structure, register set and the slti-in-bne-delay-slot all
   come out right with the func_8007A628(i++, 0) spelling and func_80079E7C
   declared u8 (it returns u8, misc5.c defines it; the s32 extern here was
   wrong). Two things still miss: (1) gcc hoists &D_800C3EB4 into a callee-
   saved s6 for the two lookups, so the frame grows to 0x30+s6 and the second
   lookup is lbu 0(v1) instead of retail's per-use lui at/%hi + addu at,at,v1 +
   lbu %lo(at); (2) with i++ in the call argument gcc emits i++ BEFORE the jal
   and puts slti in the beqz delay slot, retail has i++ in the beqz slot and
   the test in the bne slot. Also tried the plain 'for(...; i < 3; i++, offset
   += 0x1C)' form with i (71.96%, i++ lands in the latch).

void func_8007CEA4(u8 **arg0, u8 arg1) {
    u16 *pRow = (u16 *)(D_800D3420 + arg1 * 0x40);
    u8 *pCount;
    s32 count = 0;
    s32 i;
    s32 offset;

    for (i = 0, offset = 0; i < 3; offset += 0x1C) {
        if (func_8007A628(i++, 0) != 0
            && D_800C3EB4[offset] == D_800C3EB4[func_80079E7C(pRow[(*arg0)[2]]) * 0x1C]) {
            count++;
        }
    }
    pCount = &D_800D3430[arg1 * 0x40];
    pCount[(*arg0)[1]] = count;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007CFB8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007D0CC);
/* best: 85.97%. Loop body and the D_800D2CE0/D_800D2CB0 at-register lookups
   match exactly; the prologue order does not - the original forms the
   D_800D3430 base before the p[1] load and its i = 0 lands after it, gcc
   hoists i = 0 to the top and loads p first. Also tried i = 0 between the
   store and the loop, and a separate pDst local; both give the same order.

void func_8007D0CC(u8 **arg0, u8 arg1) {
    u8 *pRow = D_800D3430 + arg1 * 0x40;
    s32 i;

    pRow[(*arg0)[1]] = 0;
    for (i = 0; i < 0x30; i++) {
        if (D_800D2CE0[i] == (*arg0)[2]) {
            pRow[(*arg0)[1]] = D_800D2CB0[i];
            break;
        }
    }
}
*/
void func_8007D148(u8 **arg0, u8 *arg1, u8 arg2, u8 arg3) {
    u16 *pField = (u16 *)(D_800D3420 + arg2 * 0x40);
    u16 value = pField[(*arg0)[2]];

    arg1[arg3 * 8 + (*arg0)[1]] = value;
    *(arg1 + (arg3 * 8 + (*arg0)[1]) + 1) = value >> 8;
}
void func_8007D1A8(u8 **arg0, u8 arg1) {
    u32 *pRow = (u32 *)(D_800D3410 + arg1 * 0x40);
    u8 *p = *arg0;

    pRow[p[1]] = g_GameState.gold;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007D1DC);
void func_8007D30C(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u32 *pRow = (u32 *)(D_800D3410 + arg1 * 0x40);

    pRow[p[1]] = D_800D2C8B[arg1];
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007D344);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007D478);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007D5B0);
/* best: 93.33%. Insn multiset and the whole tail match; only the
   strength-reduced D_800CCD64 offset increment sits one block early in the
   original (in the loop-top bnez delay slot, before count++), gcc puts it in
   the loop-branch delay slot. An explicit offset variable (which is what gives
   the at-register form in func_8007CC50 and func_8007D6A8) is rewritten here
   into a countdown giv, because this loop - unlike those two - has no call, so
   the full giv combine runs. Also tried ++i, count += (flags & 0xC000) == 0 and
   (u8 *)D_800CCD64 + i * 0x170 (85.83%).

void func_8007D5B0(u8 **arg0, u8 arg1) {
    u8 *pRow;
    s32 count = 0;
    s32 i;

    for (i = 0; i < 3; i++) {
        if ((D_800CCD64[i].flags & 0xC000) == 0) {
            count++;
        }
    }
    pRow = &D_800D3430[arg1 * 0x40];
    pRow[(*arg0)[1]] = count;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007D610);
/* best: 95.50%. Whole function matches except ONE insn position: retail
   steals i++ (addiu a2,a2,1) into the beqz delay slot of the D_800D2DCC[i]
   test, gcc emits it in the latch after count++. Register assignment is exact
   once the accumulators are declared record-then-offset (t0=0x54, a3=0x450).
   Writing the increment as 'D_800D2DCC[i++]' DOES put it in that delay slot
   (identical prologue) but gcc then strength-reduces i to a pointer into
   D_800D2DCC (slt v0,a2,t2 instead of slti v0,a2,0xB), 88.11%. Also tried:
   i++ as the last body statement, a while loop with the increments in the
   body, and the explicit-offset form under (u8 *)D_800CCD64 (79.67%, gcc
   turns the i giv into a countdown). Parked at 95.50.

void func_8007D610(u8 **arg0, u8 arg1) {
    u8 *pRow;
    s32 count = 0;
    s32 i;
    s32 record;
    s32 offset;

    for (i = 3, record = 0x54, offset = 0x450; i < 0xB; i++, record += 0x1C, offset += 0x170) {
        if (D_800D2DCC[i] != 0 && (*(u16 *)((u8 *)D_800CCD64 + offset) & 0xC000) == 0 && D_800C3EB7[record] == 0) {
            count++;
        }
    }
    pRow = &D_800D3430[arg1 * 0x40];
    pRow[(*arg0)[1]] = count;
}
*/
void func_8007D6A8(u8 **arg0, u8 arg1) {
    u16 *pRow;
    u16 result;
    u32 best = -1;
    s32 bestIndex = 0;
    s32 i;
    s32 offset;
    s32 record;

    for (i = 0, offset = 0, record = 0; i < 3; offset += 0x170, i++, record += 8) {
        if (func_8007A628(i, (*arg0)[2]) != 0 && D_800D32A1[record] != 0 && best >= *(u32 *)(D_800CCDEC + offset)) {
            best = *(u32 *)(D_800CCDEC + offset);
            bestIndex = i;
        }
    }
    result = func_80089C08(bestIndex);
    pRow = (u16 *)(D_800D3420 + arg1 * 0x40);
    pRow[(*arg0)[1]] = result;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007D7B4);
