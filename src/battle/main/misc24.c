#include "common.h"
#include "psyq/libgte.h"
#include "system/memory.h"

extern u8 D_800C3B74;
extern u8 D_800C402F[];
extern s32 *D_800D3368[];
extern u8 *D_8005919C;
extern u8 *D_800C4924;
extern s16 func_800A3490(s16 angle, s16 divisor, s16 base);
extern s16 func_800A3514(s16 angle, s16 divisor, s16 base);
extern s16 func_800A3578(s16 angle, s16 divisor, s16 base);
extern s16 func_800A35C8(s16 angle, s16 divisor, s16 base);
extern u16 D_800C3D40;
extern u16 D_800C3E30;
extern u8 D_800C3D0C[];
extern s32 func_800AF400(void);
extern void func_800AA934(u8 *arg0, u8 *arg1, void *arg2, s32 arg3);
extern void func_800AAD54(void *arg0, void *arg1, s32 arg2, s32 arg3, s32 arg4);
extern void func_800AFF9C(u8 *arg0);
extern s32 func_8009F1C4(u32 arg0, s16 arg1);
extern s32 func_8009EF3C(u32 arg0, s16 arg1);
extern s32 func_800A0838(u8 *arg0, u32 arg1, u16 arg2, s16 arg3);
extern void func_800AE2A4(u8 *arg0, u8 *arg1, u8 *arg2);

u8 func_800AA514(s16 arg0, s16 arg1, s32 arg2) {
    s16 v;

    v = arg2 + arg1 * arg0 / 256;
    if (v >= 0x100) {
        v = 0xFF;
    }
    return v;
}
void func_800AA564(u8 *arg0, u16 arg1, u16 arg2, void *arg3) {
    u8 *p;

    p = (u8 *)D_800D3368[arg1];
    D_800C3D40 = arg1;
    D_800C3E30 = arg2;
    *(u8 *)(p + 0x35) = 0;
    if (D_800D3368[arg1] != 0) {
        if (!(arg1 == func_800AF400())) {
            func_800AA934((u8 *)D_800D3368[arg1], arg0, D_800C3D0C, arg3);
        }
    }
}
s32 func_800AA600(s32 arg0) {
    u8 *p;

    p = (u8 *)D_800D3368[arg0];
    if (p != 0) {
        return (*(s16 *)(p + 0x24) * ((*(s16 *)(p + 0x1C) * *(s16 *)(*(u8 **)(p + 4) + 0x4E)) >> 12)) >> 12;
    }
    return 0;
}
s32 func_800AA650(s32 arg0) {
    u8 *p;
    s32 a;
    s32 b;
    s32 r;

    r = 0;
    p = (u8 *)D_800D3368[arg0];
    if (p != 0) {
        if (*(u16 *)(p + 0x4A) & 8) {
            a = *(s16 *)(p + 0x26);
            b = *(s16 *)(*(u8 **)(p + 4) + 0x4C);
        } else {
            a = *(s16 *)(p + 0x28);
            b = *(s16 *)(*(u8 **)(p + 4) + 0x50);
        }
        r = (*(s16 *)((u8 *)D_800D3368[arg0] + 0x1C) * b) >> 12;
        r = (a * r) >> 12;
    }
    return r;
}
void func_800AA6E0(u8 *arg0) {
    u8 *p;
    s32 i;

    if (*(u8 *)(arg0 + 0x10C) != 0) {
        p = HeapAlloc(*(u8 *)(arg0 + 0x10C) * 0x70, 0);
        for (i = 0; i < *(u8 *)(arg0 + 0x10C); i++) {
            *(s16 *)(p + i * 0x70) = -1;
            *(u32 *)(p + i * 0x70 + 8) = 0;
        }
        *(u32 *)(arg0 + 0x110) = (u32)p;
    }
}
void func_800AA760(s32 arg0, u8 arg1) {
    s32 p;

    p = D_800D3368[arg0];
    if (p != 0) {
        *(u8 *)(p + 0x2A) = arg1;
    }
}
void func_800AA788(u32 arg0) {
    D_800C3B74 = arg0 & 1;
}
void func_800AA79C(s32 arg0, s32 arg1) {
    s32 tmp;

    ((u8 *)D_800D3368[arg0])[0x34] = 0;
    ((u8 *)D_800D3368[arg1])[0x34] = 1;
    tmp = D_800D3368[arg0];
    D_800D3368[arg0] = D_800D3368[arg1];
    D_800D3368[arg1] = tmp;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AA7DC);
/* 88.24% - instruction multiset identical, only the hoisted "li a1,0xF7" of
   the loop condition sits after the arg0*0x48 preheader instead of before it.
   The indexed form D_800C402F[i*0x48] does put "li a1,0xF7" first (gcc emits
   hoisted invariants before strength-reduction inits) but then turns the biv
   into a pointer, folding %hi/%lo into a register in the preheader
   (lui v1/addiu v1/addu v0,v0,v1/lbu 0(v0)) instead of re-forming the symbol
   address each iteration - 56.76% and 2 insns longer. A "u8 stop = 0xF7;"
   local changes nothing (the constant is re-folded).
u8 func_800AA7DC(s32 arg0) {
    s32 o;
    u8 v;

    o = arg0 * 0x48;
    do {
        v = D_800C402F[o];
        o += 0x48;
    } while (v == 0xF7);
    if (v == 0xFF) {
        v = 0xFE;
    }
    return v;
}
*/
s16 (*func_800AA820(s32 arg0))(s16 angle, s16 divisor, s16 base) {
    switch (arg0) {
    case 1:
        return func_800A3514;
    case 2:
        return func_800A3578;
    case 3:
        return func_800A35C8;
    }
    return func_800A3490;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AA898);
/* 74.36% - every store and constant is identical and in the same order; the
   only difference is where the "-1" constant is materialised: the original
   does "li v1,-1" as the very first instruction (before "li v0,0xffff"), this
   emits it just before the sh to 0x98. Tried: raw -1 literals for the three
   s16 fields (74.36), a "s16 m1 = -1;" decl initialiser at the top (74.36),
   a separate "m1 = -1;" statement first (74.36) - gcc sinks the constant to
   its first use here, and the 0xFFFF/0xFF/0x6B/0x1 loads already match.
   Note *(u16 *)(arg0 + 0x3C) is unsigned (ori 0xffff) while 0x98/0x3A/0x1E are
   signed -1 (addiu), so the two constants must stay spelled differently.
void func_800AA898(u8 *arg0, s32 arg1, s32 arg2, s32 arg3) {
    *(u16 *)(arg0 + 0x3C) = -1;
    *(u8 *)(arg0 + 0x5C) = 0xFF;
    *(u8 *)(arg0 + 0x39) = 0x6B;
    *(s32 *)(arg0 + 0x8) = arg2;
    *(s32 *)(arg0 + 0xC) = 0;
    *(s32 *)(arg0 + 0x10) = 0;
    *(s32 *)(arg0 + 0x14) = arg3;
    *(s32 *)(arg0 + 0x18) = 0;
    *(u8 *)(arg0 + 0x2B) = 0;
    *(s16 *)(arg0 + 0x98) = -1;
    *(u16 *)(arg0 + 0x58) = 0;
    *(u8 *)(arg0 + 0x35) = 0;
    *(u8 *)(arg0 + 0x37) = 0;
    *(u8 *)(arg0 + 0x38) = 0;
    *(s16 *)(arg0 + 0x3A) = -1;
    *(u16 *)(arg0 + 0x70) = 0;
    *(u16 *)(arg0 + 0x72) = 0;
    *(u16 *)(arg0 + 0x74) = 0;
    *(u16 *)(arg0 + 0x76) = 0;
    *(u16 *)(arg0 + 0x78) = 0;
    *(u16 *)(arg0 + 0x7A) = 0;
    *(u16 *)(arg0 + 0x7C) = 0;
    *(u16 *)(arg0 + 0x7E) = 0;
    *(u16 *)(arg0 + 0x80) = 0;
    *(u16 *)(arg0 + 0x82) = 0;
    *(u16 *)(arg0 + 0x84) = 0;
    *(u16 *)(arg0 + 0x86) = 0;
    *(u16 *)(arg0 + 0x88) = 0;
    *(u16 *)(arg0 + 0x8A) = 0;
    *(u16 *)(arg0 + 0x8C) = 0;
    *(u16 *)(arg0 + 0x8E) = 1;
    *(u8 *)(arg0 + 0x36) = 0;
    *(s16 *)(arg0 + 0x1E) = -1;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AA934);
/* 88.71% - structure, branch layout, both table reads and the whole tail
   (the stores + func_800AAD54(arg0, arg2, -1, 1, 0)) already line up. Three
   differences left: (1) the original reloads arg0[0x2B] for the "++"
   (lbu, nop, addiu) while gcc CSEs it into one load and masks it
   ("andi v0,v1,0xff"), so the two blocks are 4 instructions apart;
   (2) the if-branch table pointer lands in $a1 instead of $v0;
   (3) the else branch folds the +4 of *(u8 **)(arg1 + 0xC) + 4 into the
   -0x138 displacement (-0x134) where the original keeps "lw v1,4(v0)".
   Tried: nested ifs with bare dereferences (64.61), a u8 *pCount = arg0 +
   0x2B local (88.71, no change), pointer locals for both table bases (88.71),
   and a u16 n = D_800C3E30 local which is needed to hoist the lhu before the
   stores. Same-family blocker as the note above: a constant/pointer being
   re-formed where the original keeps it live.
void func_800AA934(u8 *arg0, u8 *arg1, void *arg2, s32 arg3) {
    s32 *pTable;
    u8 *pCount;
    u16 n;

    pCount = arg0 + 0x2B;
    if (arg0 != 0) {
        if (arg1 != 0) {
            if (*pCount != 0) {
                if (*pCount < 5) {
                    *pCount = *pCount + 1;
                }
                ((u8 *)arg0)[*pCount + 0x2A] = *(u8 *)(arg1 + 0x20);
                ((u8 *)arg0)[*pCount + 0x2E] = arg3;
            } else {
                if (arg3 < 0x50) {
                    pTable = (s32 *)*(u8 **)(arg1 + 8);
                    *(s32 *)(arg0 + 0x10) = pTable[arg3];
                } else {
                    pTable = (s32 *)(*(u8 **)(arg1 + 0xC) + 4);
                    *(s32 *)(arg0 + 0x10) = *(s32 *)((u8 *)pTable + arg3 * 4 - 0x138);
                }
                n = D_800C3E30;
                *(u16 *)(arg0 + 0x42) = 0;
                *(u16 *)(arg0 + 0x40) = 0;
                *(s32 *)(arg0 + 0x50) = 0;
                *(s32 *)(arg0 + 0x54) = 0;
                *(s32 *)(arg0 + 0x4C) = 0;
                *(u8 *)(arg0 + 0x23) = 0;
                *(u16 *)(arg0 + 0x10A) = n;
                func_800AAD54(arg0, arg2, -1, 1, 0);
            }
        }
    }
}
*/
s32 func_800AAA20(u8 *arg0, u8 *arg1, s32 arg2, u8 *arg3, s32 arg4) {
    s32 r;
    s32 i;

    if (*(u32 *)arg0 != 0) {
        r = 0;
        if (*(u8 *)(arg0 + 0x34) != 0) {
            func_800AFF9C(arg0);
            if (*(u8 *)(arg0 + 0x37) != 0) {
                func_8009F1C4(*(u32 *)(arg0 + 4), *(s16 *)(arg0 + 0x1C));
            } else {
                func_8009EF3C(*(u32 *)(arg0 + 4), *(s16 *)(arg0 + 0x1C));
            }
            for (i = 0; i < arg2; i++) {
                r |= func_800A0838(arg1, *(u32 *)(arg0 + 4), *(u16 *)(arg0 + 0x3C), *(s16 *)(arg0 + 0x1C));
                func_800AE2A4(arg0, arg1, arg3);
            }
        }
        func_800AAD54(arg0, arg1, r, arg2, arg4);
    }
    return r;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AAB34);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AAD54);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800ADF1C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AE098);
void func_800AE1BC(u8 *arg0, u8 *arg1, s32 arg2) {
    u8 *p;

    if (*(u16 *)(arg1 + 0x12) != 0) {
        *(s16 *)(arg0 + 0x98) = 0;
        if (arg2 != 0) {
            *(s16 *)(arg0 + 0x9A) = *(u16 *)(arg1 + 0x2);
        } else {
            *(s16 *)(arg0 + 0x9A) = -1;
        }
        *(s16 *)(arg0 + 0x9C) = 0;
        *(s16 *)(arg0 + 0x9E) = *(u16 *)(arg1 + 0x12);
        p = arg1 + *(u32 *)(arg1 + 0x14);
        *(u32 *)(arg0 + 0xA0) = (u32)p;
        *(u32 *)(arg0 + 0xA4) = (u32)p;
    } else {
        *(s16 *)(arg0 + 0x98) = -1;
    }
}
s32 func_800AE220(u8 *arg0, s32 arg1) {
    if (arg1 == 0) {
        return *(u16 *)(D_8005919C + 0x14) << 16;
    }
    if (arg1 == 1) {
        return *(u16 *)(*(u8 **)(*(u8 **)(arg0 + 0xB0) + 8) + 0x14) << 16;
    }
    if (arg1 == 2) {
        return *(u16 *)(*(u8 **)(*(u8 **)(arg0 + 0xB4) + 8) + 0x14) << 16;
    }
    if (arg1 == 3) {
        return *(u16 *)(D_800C4924 + 0x14) << 16;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AE2A4);
void func_800AEEEC(u8 *pArg) {
    *(s16 *)(pArg + 0x98) = -1;
}
s32 func_800AEEF8(u8 *arg0) {
    s32 dx;
    s32 dy;
    s32 dz;

    dx = *(s16 *)(arg0 + 0x88) - *(s32 *)(*(u8 **)(arg0 + 4) + 0x5C);
    dy = *(s16 *)(arg0 + 0x8A) - *(s32 *)(*(u8 **)(arg0 + 4) + 0x60);
    dz = *(s16 *)(arg0 + 0x8C) - *(s32 *)(*(u8 **)(arg0 + 4) + 0x64);
    return SquareRoot0(dx * dx + dy * dy + dz * dz);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AEF68);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AF180);
/* 97.43% - instruction multiset identical; the only difference is two swapped
   register pairs in the allocator: original has s0=p/s1=i and s3=arg0/s4=arg2,
   this emits s0=i/s1=p and s3=arg2/s4=arg0 (and the two operands of the two
   "addu" address sums come out base-first). Tried: 6 declaration orders for the
   locals (identical), q=p order swapped (80.40), while-loop form (97.43),
   n loaded before p/q (97.43), "arg1*0x7C + ptr" instead of "ptr + arg1*0x7C"
   (97.43, gcc canonicalises either way). Allocator tie, same class as the notes
   above.
extern s32 func_800A23E8(u8 *arg0, u8 *arg1);
void func_800AF180(u8 *arg0, s32 arg1, u8 *arg2, u8 *arg3) {
    u8 *p;
    u8 *q;
    u8 *pChild;
    s32 n;
    s32 i;

    p = arg1 * 0x7C + arg2;
    q = arg1 * 0x7C + arg3;
    n = *(u16 *)(arg2 + 0xA);
    *(u8 *)(p + 7) = 0;
    *(u8 *)(q + 7) = 1;
    func_800A23E8(arg0, *(s32 *)(p + 0x70));
    *(s32 *)(p + 0x70) = 0;
    func_800A23E8(arg0, *(s32 *)(p + 0x74));
    *(s32 *)(p + 0x74) = 0;
    func_800A23E8(arg0, *(s32 *)(p + 0x78));
    *(s32 *)(p + 0x78) = 0;
    pChild = arg2;
    for (i = 1; i < n; i++) {
        pChild += 0x7C;
        if (*(u8 **)pChild == p) {
            func_800AF180(arg0, *(u16 *)(pChild + 0xA), arg2, arg3);
        }
    }
}

*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc24", func_800AF270);
/* 85.71% - instruction multiset identical, only the preheader order differs:
   the original emits the hoisted "li a3,1" before "addiu a0,a0,7", here the
   arg0 += 7 statement lands there first (and reorg then fills the beqz delay
   slot with it, so the frame is one insn shorter). Indexed forms
   arg0[i*0x7C+7] (90.33%) or p=arg0+7 / p[i*0x7C] (90.43%) put li a3 first
   but let gcc fold the +7 into the load displacement (lbu 7(a0)) or into the
   biv init (addiu a0,a0,0x83); a u8 one; local made it worse (85.71%, the
   addiu hoisted above the slt).
void func_800AF270(u8 *arg0, u8 *arg1) {
    s32 n;
    s32 i;

    n = *(u16 *)(arg0 + 0xA);
    arg0 += 7;
    for (i = 1; i < n; i++) {
        arg1 += 0x7C;
        arg0 += 0x7C;
        if (*arg0 != 0) {
            *arg0 = 0;
            arg1[7] = 1;
        }
    }
}
*/
