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
/* 84.67% - only the last addu is transposed: gcc forms ((idx*2 + base) + row)
   where the original forms ((row + base) + idx*2), i.e. it emits
   addu v0,v0,a1 (idx, rowbase) after addu a1,a1,v1 (row, base). The base is
   formed after the value here, which is right. 28.33% forms (28%/35.67%) all
   let gcc fold the symbol through $at or hoist the base 2-3 insns early.
   Tried: index in the pointer initialiser (E), mem_addr form (gives $at),
   base/idx as separate locals (base hoists), (u16*)sym + arg1*0x20, inline
   value, u8* base + cast store, split |= statement.
void func_8007A92C(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u16 val = p[2] | (p[3] << 8);
    u16 *pRow = (u16 *)(D_800D3420 + arg1 * 0x40 + p[1] * 2);

    *pRow = val;
} */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc5", func_8007A968);
/* 53.44% - the value expression and the index come out right, but gcc forms the
   D_800D3410 lui/addiu at the top (into a0) instead of after the value, so the
   store base is 2 insns early. Tried: pRow pointer local before/after the value,
   idx local first, single-expression store (8-42%, at-register form),
   (u32*)D_800D3410 + arg1*0x10 element pointer, mem_addr index-first form.
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
   lbu order right, but then gcc emits mult v0,v1. `pRow[p[1]] * p[2]` swaps the
   lbu order too (71.50%). Tried: s16 v with u8 res copy (99.55%), s32 operands
   via a cur local, u8 res = product then s16 v = res (87.95%, extension folded).
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc5", func_8007ABD8);
/* 82.73% - the clamped u16 read-modify-write. The original forms the row base
   once (addu a1,v1,a1, index-first) and keeps it for both the lhu and the sh;
   spelling the address twice (this attempt) or keeping one pRow local both
   leave 2-3 insns of address formation off. Tried: one pRow local (56.14%,
   base hoisted before the pointer load), address spelled twice (82.73%),
   u32/s32 v with if-clamp, mem_addr index-first form.
void func_8007ABD8(u8 **arg0, u8 arg1) {
    u8 *p = *arg0;
    u32 v = *((u16 *)(D_800D3420 + arg1 * 0x40 + p[1] * 2)) + (p[2] + (p[3] << 8));

    if (v > 0xFFFF) {
        v = 0xFFFF;
    }
    *((u16 *)(D_800D3420 + arg1 * 0x40 + p[1] * 2)) = v;
} */
