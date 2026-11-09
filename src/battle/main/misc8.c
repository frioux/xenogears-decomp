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
extern u8 D_800D3410[];
extern u8 D_800D3420[];
extern u8 D_800D3430[];
extern u8 func_8007A628(u8 arg0, u8 arg1);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007C1A4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007C33C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007C4A0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007C580);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007C678);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007C75C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007C840);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007C9D4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007CB20);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007CC50);
/* best: 87.29%. The whole tail matches once the row pointer is a local
   assigned after the loop (a direct D_800D3430[...] index makes maspsx emit
   the at-register form). Remaining: the original increments the D_800C3EB4
   offset (s1 = i * 0x1C) in the loop-branch delay slot and keeps the base
   as %hi/%lo per access, and steals i++ into the post-call beqz delay slot;
   gcc strength-reduces s1 to an absolute pointer and puts i++ last.

void func_8007CC50(u8 **arg0, u8 arg1) {
    u8 *pRow;
    s32 count = 0;
    s32 i;

    for (i = 0; i < 3; i++) {
        if (func_8007A628(i, 0) != 0 && D_800C3EB4[i * 0x1C] == (*arg0)[2]) {
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007CEA4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007CFB8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007D0CC);
/* best: 85.97%. Loop body and the D_800D2CE0/D_800D2CB0 at-register lookups
   match exactly; the prologue order does not - the original forms the
   D_800D3430 base before the p[1] load and its i = 0 lands after it, gcc
   hoists i = 0 to the top and loads p first.

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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007D148);
/* best: 99.96%. Everything matches except the second byte store lands at
   0(a1) instead of 1(a1); writing the +1 index flips the final addu's
   destination from a1 to a3 (99.17%). Tried: +1 in the index, swapped
   operand order, (arg1 + 1)[idx] - allocator tie.

void func_8007D148(u8 **arg0, u8 *arg1, u8 arg2, u8 arg3) {
    u16 *pField = (u16 *)(D_800D3420 + arg2 * 0x40);
    u16 value = pField[(*arg0)[2]];

    arg1[arg3 * 8 + (*arg0)[1]] = value;
    arg1[arg3 * 8 + (*arg0)[1] + 1] = value >> 8;
}
*/
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
   the loop-branch delay slot.

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
/* best: 86.05%. Tail matches; same induction/delay-slot placement miss as
   func_8007D5B0 plus the D_800C3EB7 offset (s1-style at-register lookup) in
   the three-way && chain.

void func_8007D610(u8 **arg0, u8 arg1) {
    u8 *pRow;
    s32 count = 0;
    s32 i;

    for (i = 3; i < 0xB; i++) {
        if (D_800D2DCC[i] != 0 && (D_800CCD64[i].flags & 0xC000) == 0 && D_800C3EB7[i * 0x1C] == 0) {
            count++;
        }
    }
    pRow = &D_800D3430[arg1 * 0x40];
    pRow[(*arg0)[1]] = count;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007D6A8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc8", func_8007D7B4);
