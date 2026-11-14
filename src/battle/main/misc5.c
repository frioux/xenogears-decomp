#include "common.h"

extern u8 *D_800D2D28;
extern u8 D_800D3725[];
extern u16 D_8005A3A0[];
extern u8 D_800D3430[];
extern u8 D_800D3420[];
extern u8 D_800D3410[];
extern u8 D_800D2DCC[];
extern u8 D_800CCD64[];
extern u8 D_800CCD6C[];
extern u8 D_800C3EB7[];

void func_80079E18(u8 arg0) {
    D_800D2D28[0xB4] = 1;
    D_800D3725[arg0 * 0x60] = 1;
}

void func_80079E4C(u8 arg0) {
    D_800D2D28[0xB4] = 0;
    D_800D3725[arg0 * 0x60] = 0;
}

extern u16 func_80089C9C(u16, u8);


u8 func_80079E7C(u16 arg0) {
    s32 i;

    for (i = 0; i < 0xB; i++) {
        if (func_80089C9C(arg0, i) != 0) {
            break;
        }
    }
    return i;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc5", func_80079ED8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc5", func_8007A280);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc5", func_8007A628);
/* 99.12% - identical instruction sequence, only the two register choices
   differ: the original keeps res in a2 and the row offset arg0*0x170 in v1
   (sll v1,v0,4 / addu at,at,v1 / addu at,at,v1), ours puts res in v1 and the
   offset in a0. Same body as func_8007A6C8 (which matches 100% with res in v1,
   offset in a0), so the two differ only in allocation: this one needs res out of
   v1. Tried: res declared before/after off, off/f as top-level declarations,
   u32/u8 res. The u8 res is what produces the sltiu v0 + move (a2,v0) copy.
   Round 35: everything (three branches, both delay-slot stores of res, all
   loads, the arg0*0x170 sequence) is identical; the ONLY difference is res
   landing in v1 instead of a2 while off lands in a0 instead of v1 - a global.c
   priority tie. Also tried: the offset written inline twice (cse makes one
   pseudo, same 99.12%) and dropping the `u32 f` local (89.10%, it turns the
   andi+sltiu into srl+xori).
u32 func_8007A628(u8 arg0, u8 arg1) {
    s32 off;
    u32 f;
    u8 res = 0;

    if (D_800D2DCC[arg0] != 0 && D_800C3EB7[arg0 * 0x1C] == 0) {
        off = arg0 * 0x170;
        if ((*(u16 *)(D_800CCD64 + off) & 0xC002) == 0) {
            res = 1;
            if (arg1 == 0) {
                f = *(u16 *)(D_800CCD6C + off) & 0x20;
                res = f == 0;
            }
        }
    }
    return res;
} */
u32 func_8007A6C8(u8 arg0, u8 arg1) {
    u8 res = 0;

    if (D_800D2DCC[arg0] != 0) {
        s32 off = arg0 * 0x170;

        if ((*(u16 *)(D_800CCD64 + off) & 0xC002) == 0) {
            res = 1;
            if (arg1 == 0) {
                u32 f = *(u16 *)(D_800CCD6C + off) & 0x20;

                res = f == 0;
            }
        }
    }
    return res;
}
u32 func_8007A744(u8 arg0) {
    u8 res = 0;

    if (D_800D2DCC[arg0] != 0 && D_800C3EB7[arg0 * 0x1C] == 0) {
        res = (*(u16 *)(D_800CCD64 + arg0 * 0x170) & 0xC000) == 0;
    }
    return res;
}
u8 func_8007A7BC(u8 **arg0, u8 *arg1, u8 arg2, u8 arg3) {
    u8 *pRow = (u8 *)(arg3 * 8 + (mem_addr)arg1);

    pRow[0] = 0x80;
    pRow[1] = (*arg0)[0];
    pRow[2] = (*arg0)[1];
    pRow[3] = (*arg0)[2];
    pRow[4] = (*arg0)[3];
    arg3++;
    return arg3;
}
u8 func_8007A828(u8 **arg0, u8 *arg1, u8 arg2) {
    u8 *p = *arg0;

    arg1[arg2 * 8 + p[1]] = p[2];
    if ((*arg0)[1] == 0) {
        arg2++;
    }
    return arg2;
}
void func_8007A874(u8 **arg0, u8 *arg1, u8 arg2, u8 arg3) {
    u8 *p = *arg0;
    s32 idx = arg3 * 8 + p[1];
    u8 *pRow = D_800D3430 + arg2 * 0x40;

    arg1[idx] = pRow[p[2]];
}
void func_8007A8B4(u8 **arg0, u8 *arg1) {
    u8 *p;
    s32 i;

    for (i = 0; i < 8; i++) {
        p = *arg0;
        arg1[p[2] * 8 + i] = arg1[p[1] * 8 + i];
    }
}
void func_8007A900(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;

    pRow[p[1]] = p[2];
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc5", func_8007A92C);
/* 84.67% - everything but the final two addus matches (order, registers, the
   lui/addiu base after the p[1] load). Ours associates the sum as
   (idx2 + base) + rowOff (addu v0,v0,v1 / addu a1,a1,v0); the original wants
   (rowOff + base) + idx2 (addu a1,a1,v1 / addu v0,v0,a1), i.e. rowOff must be
   the FIRST operand of the first addu and idx2 the first of the second.
   Naming (rowOff + sym) in a local fixes the add order but makes gcc hoist the
   lui/addiu above the p[] loads (18.33%); `u8 idx = p[1]` with one local gives
   the perfect val/lbu order but hoists the base the same way; an array form
   `((u16*)(sym + arg1*0x40))[p[1]]` and the fully inline sum both become the
   %at address (28.33%).
   Round 35: 10 shapes. A named base local ((u8*)(rowOff + sym) then
   *(u16*)(pBase + (p[1] << 1))) DOES fix the association - the two addus become
   (rowOff+base) then +idx2, as retail - but gcc then emits the lui/addiu group
   and the `lw p` two statements early (17.00%). 84.67% is still the best.
void func_8007A92C(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    s32 rowOff = arg1 * 0x40;
    u16 val = p[2] | (p[3] << 8);
    u16 *pRow = (u16 *)(rowOff + (mem_addr)D_800D3420 + (p[1] << 1));

    *pRow = val;
} */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc5", func_8007A968);
/* 53.44% - the value expression and the index come out right, but gcc forms the
   D_800D3410 lui/addiu at the top (into a0) instead of after the value, so the
   store base is 2 insns early. Tried: pRow pointer local before/after the value,
   idx local first, single-expression store (8-42%, at-register form),
   (u32*)D_800D3410 + arg1*0x10 element pointer, mem_addr index-first form.
   Round 20: with u32 val = ((p[3]<<8)+p[2])*0x10 first and a u8* row pointer
   used as ((u32*)pRow)[p[1]] every instruction is right but the whole
   lui/addiu + addu a1,a1,a0 group lands BEFORE the value loads (10.31%): gcc
   sinks the one-use value into the store, so the pointer init is expanded
   first. Same wall as func_8007A92C; gcc-2.6.0, 2.7.2-cdk, -O1 and -O3 all
   fail on it too.
void func_8007A968(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u32 val = (p[2] + (p[3] << 8)) * 0x10;
    s32 idx = arg1 * 0x10 + p[1];

    ((u32 *)D_800D3410)[idx] = val;
} */
void func_8007A9A8(u8 **arg0) {
    D_8005A3A0[(*arg0)[1]] = (*arg0)[2];
}
void func_8007A9D0(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;
    s32 v = pRow[p[1]] + p[2];
    u8 res = v;

    if (v >= 0x100) {
        res = 0xFF;
    }
    pRow[p[1]] = res;
}

/* 61.47% - clamp to 0xFF. Original keeps the sum in v0 and copies it to the
   store register v1 (move v1,v0 in the bnez delay slot) and materialises the
   D_800D3430 lui/addiu after the pointer load; gcc puts the sum straight in v1
   and the base 2 insns early. Tried: if/else with a res local (47.26%,
   branchless/min form), ternary, if-without-else, separate base/idx locals.
void func_8007A9D0(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;
    s32 v = pRow[p[1]] + p[2];

    if (v >= 0x100) {
        v = 0xFF;
    }
    pRow[p[1]] = v;
} */
void func_8007AA1C(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;
    s32 v = pRow[p[1]] - p[2];
    u8 res = v;

    if (v < 0) {
        res = 0;
    }
    pRow[p[1]] = res;
}
/* 67.00% - clamp at 0. Same miss as func_8007A9D0: the original keeps
   mem-p[2] in v0 and copies it into the store register v1 (move v1,v0 in the
   bgez delay slot), and forms the D_800D3430 base after the pointer load.
   Tried: if/else with a res local (60.24%, branchless add/subu/sra/and form),
   v-first declaration orders.
void func_8007AA1C(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;
    s32 v = pRow[p[1]] - p[2];

    if (v < 0) {
        v = 0;
    }
    pRow[p[1]] = v;
} */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc5", func_8007AA60);
/* 99.55% - one insn off: the original's multiply is mult v1,v0 (value first,
   p[2] second); writing `p[2] * pRow[p[1]]` is the only shape that also gets the
   lbu order right (lbu p[1], lbu p[2], addu row base, lbu row), but then gcc
   emits mult v0,v1. `pRow[p[1]] * p[2]` flips the mult but also flips the loads
   to p[1], addu, row, p[2] (71.50%). Splitting the operands into locals
   (`u8 idx = p[1]; u8 n = p[2]; v = pRow[idx] * n;`) is also 71.50%: cse sinks
   the p[2] load past the row addu anyway.
   Tried: s16 v with u8 res copy (99.55%), s32 operands
   via a cur local, u8 res = product then s16 v = res (87.95%, extension folded),
   operand locals before/after the row pointer (71.50%).
   Round 20: still 99.55% and also exactly 99.55% under gcc-2.7.2-cdk, so this
   one is not the cdk-flippable tie that func_8007ABD8 is.
   Round 35: the last difference is only the mult operand order (mult v0,v1 vs
   v1,v0); every other insn, including both lbu orders, is identical. gcc spells
   'mult <first source operand>,<second>' and always defers the SECOND operand's
   lbu to the mult point, so 'pRow[p[1]] * p[2]' buys the mult order and loses
   the p[2] load (71.50%). The two are mutually exclusive.
void func_8007AA60(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;
    s16 v = p[2] * pRow[p[1]];
    u8 res = v;

    if (v >= 0x100) {
        res = 0xFF;
    }
    pRow[p[1]] = res;
} */
void func_8007AAB8(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;

    pRow[p[1]] /= p[2];
}
void func_8007AAF4(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;

    pRow[p[1]] %= p[2];
}
void func_8007AB30(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;

    pRow[p[1]] &= p[2];
}
void func_8007AB68(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;

    pRow[p[1]] |= p[2];
}
void func_8007ABA0(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u8 *pRow = D_800D3430 + arg1 * 0x40;

    pRow[p[1]] ^= p[2];
}
