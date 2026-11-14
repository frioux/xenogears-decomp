#include "common.h"
#include "psyq/libgpu.h"

extern u8 D_8005959C;
extern u8 *D_800C3EA4;
extern u8 D_800C3E4C;
extern u8 D_800C492A;
extern u8 *D_800CCB04;
extern s32 D_800D2DAC;
extern u8 *D_800D3278;
extern s32 D_800CCB34;
extern void *D_800D2F5C;
extern u8 *D_800D2D28;
extern u8 *D_800C3E24;
extern u16 D_800C3254[];
extern u8 D_800D3280;
extern u8 D_800D2D24[];
extern u8 *D_800D2DB4;
extern u8 D_800C3EB6;
extern u8 D_800D32A1;

extern u8 D_800D36C8;
extern u8 D_800D3725[];
extern u8 D_800D3726[];

extern s32 func_8002675C(void *pDir, s32 index, void *pPrims, s32 context, s16 x, s16 y, s16 scale);
extern void func_80076C78(POLY_FT4 *p, u16 x, s16 y, u8 u, u8 v, u8 dx);
extern void func_800728B8(u8 *pDst, s32 arg1, s32 arg2);
extern void func_80072A9C(s32 index, u8 arg1);
extern void func_80072DA8(s32 index, u8 arg1);
extern s32 func_80072F38(s32 index, u8 arg1);
extern void func_80073380(s32 index);
extern s32 func_80025FA8(void *pDir, s32 id, u8 *pPrim, s32 context, u16 x, u16 y, u16 u, u16 v, u16 w);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80073B64);

void func_80073E88(void) {
    s32 i;
    s32 offset;
    u8 *p;

    if (D_800D2D28[0xCB] == 0) {
        return;
    }

    i = 0;
    offset = 0x641C;
    do {
        p = D_800D2D28;
        func_800728B8((u8 *)(offset + (s32)D_800C3EA4), *(s32 *)(p + i * 4 + 0xD0), p[0xA3]);
        offset += 0xFA0;
        i++;
    } while (i < 2);
}

void func_80073F08(void) {
    if (D_800D2D28[0x9C] != 0) {
        func_800728B8(D_800C3EA4 + 0xBA8, *(s32 *)(D_800D2D28 + 0xF8), D_800D2D28[0xA5]);
    }

    if (D_800D2D28[0x9E] != 0) {
        func_800728B8(D_800C3EA4 + 0x27C8, *(s32 *)(D_800D2D28 + 0x100), D_800D2D28[0xA7]);
    }

    if (D_800D2D28[0x9D] != 0) {
        func_800728B8(D_800C3EA4 + 0x1E68, *(s32 *)(D_800D2D28 + 0xFC), D_800D2D28[0xA6]);
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80073FB8);
/* 87.42%: instruction multiset matches except the two D_800D32A1 reads. The
   original forms `lui %hi(D_800D32A1); addu at,at,s1; lbu %lo(at)` from an
   integer index s1 = i*8 recomputed every iteration, and keeps &D_800CCB04 in
   s0; gcc 2.7.2 strength-reduces the address into a pointer biv in s1 and
   rematerialises the link address with lui/lw. Every access shape tried
   (array subscript, integer cast of the symbol address, block-scoped base
   local) produces the biv. */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_800742A0);
/* void func_800742A0(void) {
    s32 i;
    s32 offset;
    s32 value;

    for (i = 0, offset = 0; i < 3; i++, offset += 0x1C) {
        if (*(u8 *)(D_800D2D28 + i + 0x7C) != 0) {
            *(s32 *)(D_800D2D28 + i * 4 + 0xE0) = 0;
            *(s32 *)(D_800D2D28 + i * 4 + 0xEC) = 0;
            if ((&D_800C3EB6)[offset] != 0x7F) {
                func_80073380(i);
                value = func_80072F38(i, *(u8 *)((s32)&D_800D32A1 + i * 8));
                if (*(u8 *)((s32)&D_800D32A1 + i * 8) != 0) {
                    func_80072DA8(i, value);
                } else {
                    func_80072A9C(i, value);
                }
                *(u8 *)(D_800D2D28 + i + 0x93) = *(u8 *)&D_800CCB34;
                *(u8 *)(D_800D2D28 + i + 0x7C) = 0;
            }
        }
    }
} */
/* 90.29%: the only differences are that the original keeps the address of
   D_800CCB04 in s0 and reloads through it, and its `addu a1,a1,v1` (offset
   first) comes out as `addu a1,v1,a1` here. Same two problems as
   func_800744BC, but with four AddPrim uses of the link address still not
   enough to make gcc spill it to a callee-saved register. */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_800743A4);
/* void func_800743A4(void) {
    if ((*(u8 **)(D_800C3EA4 + 0xA230))[0x669] == 0) {
        return;
    }

    AddPrim(D_800CCB04 + 4, (*(u8 **)(D_800C3EA4 + 0xA230)) + (*(u8 **)(D_800C3EA4 + 0xA230))[0x668] * 40);
    AddPrim(D_800CCB04 + 4, (*(u8 **)(D_800C3EA4 + 0xA230)) + 0x50 + (*(u8 **)(D_800C3EA4 + 0xA230))[0x668] * 40);
    AddPrim(D_800CCB04 + 4, (*(u8 **)(D_800C3EA4 + 0xA230)) + 0xA0 + (*(u8 **)(D_800C3EA4 + 0xA230))[0x668] * 40);
    AddPrim(D_800CCB04 + 4, (*(u8 **)(D_800C3EA4 + 0xA230)) + 0xF0 + (*(u8 **)(D_800C3EA4 + 0xA230))[0x668] * 40);
} */
/* 79.68% (82.84% with the offset left inline). Splitting the offset into a
   named local and adding the prim pointer last fixes the final addu's operand
   order (addu a1,v0,a1, the original's addu a1,a1,v1) - the trick that matched
   func_80074F70. What is left is allocation only: gcc puts p in a1 and the sum
   in v0 where the original builds the sum in a1 and keeps p in v1, and gcc
   re-forms %hi/%lo(D_800CCB04) per call instead of keeping &D_800CCB04 in s0
   and reloading through it. func_800743A4 and func_80074554 are identical. */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_800744BC);
/* 79.68%: only allocation differs. Retail keeps p in $v1 and accumulates the
   offset chain directly in the argument register $a1 (addu $a1,$a1,$v1 in the
   jal slot) and keeps &D_800CCB04 in a saved $s0 (`lw $a0,0($s0)` twice); gcc
   gives $a1 to p and builds the sum in $v0 (addu $a1,$v0,$a1). Also tried
   `u8 **pp = &D_800CCB04;` and AddPrim(*pp + 4, ...): same 79.68%, the address
   local is folded away and none of the $s0 save/restore is emitted. Retail's
   source clearly held the OT address in a local that survived the call, which
   reload_cse rematerialises for us. Allocator tie, not a source-shape problem. */
/* void func_800744BC(void) {
    u8 *p;
    s32 off;

    p = *(u8 **)(D_800C3EA4 + 0xA230);
    off = p[0x66A] * 40 + 0x320;
    AddPrim(D_800CCB04 + 4, (u8 *)(off + (s32)p));
    p = *(u8 **)(D_800C3EA4 + 0xA230);
    off = p[0x66A] * 40 + 0x370;
    AddPrim(D_800CCB04 + 4, (u8 *)(off + (s32)p));
} */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80074554);
/* Same shape as func_800744BC with 0x280 and 0x2D0: identical result. */
/* void func_80074554(void) {
    AddPrim(D_800CCB04 + 4, (*(u8 **)(D_800C3EA4 + 0xA230)) + 0x280 + (*(u8 **)(D_800C3EA4 + 0xA230))[0x66A] * 40);
    AddPrim(D_800CCB04 + 4, (*(u8 **)(D_800C3EA4 + 0xA230)) + 0x2D0 + (*(u8 **)(D_800C3EA4 + 0xA230))[0x66A] * 40);
} */

INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_800745EC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80074AB8);
void BattleUpdateRotationBuffer(void) {
    s32 i;

    if (D_800D2D28[0xC6] == 0) {
        return;
    }

    if (D_800C3E24[0xE5] != 0) {
        *(s32 *)(D_800C3E24 + 0xE0) -= 4;
        if (*(s32 *)(D_800C3E24 + 0xE0) < 0x40) {
            D_800C3E24[0xE5] = 0;
            *(s32 *)(D_800C3E24 + 0xE0) = 0x40;
        }
    } else {
        *(s32 *)(D_800C3E24 + 0xE0) += 4;
        if (*(s32 *)(D_800C3E24 + 0xE0) >= 0x100) {
            D_800C3E24[0xE5] = 1;
            *(s32 *)(D_800C3E24 + 0xE0) = 0xFC;
        }
    }

    i = 0;
    do {
        if (*(u8 *)(D_800C3E24 + i + 0xE6) != 0) {
            ((u8 *)D_800C3E24)[(i * 2 + D_800C3E24[0xE4]) * 0x1C + 4] = D_800C3E24[0xE0];
            ((u8 *)D_800C3E24)[(i * 2 + D_800C3E24[0xE4]) * 0x1C + 5] = 0;
            ((u8 *)D_800C3E24)[(i * 2 + D_800C3E24[0xE4]) * 0x1C + 6] = 0;
            AddPrim(D_800CCB04 + 4, (u8 *)((i * 2 + D_800C3E24[0xE4]) * 0x1C + (s32)D_800C3E24));
        }
        i++;
    } while (i < 4);
}
void func_80074EEC(void) {
    if (D_800D2D28[0xA8] == 0) {
        return;
    }

    func_800728B8(D_800D2DB4 + 0x5550, D_800D2DB4[0x5D80], D_800D2DB4[0x5D8F]);
    func_800728B8(D_800D2DB4 + 0x5640, D_800D2DB4[0x5D81], D_800D2DB4[0x5D90]);
    func_800728B8(D_800D2DB4 + 0x5C80, D_800D2DB4[0x5D82], D_800D2DB4[0x5D91]);
}

void func_80074F70(void) {
    u8 *p;
    s32 off;

    if (D_800D2D28[0xC8] != 0) {
        p = D_800D3278;
        off = p[0x7F4] * 40 + 0x7A4;
        AddPrim(D_800CCB04 + 4, (u8 *)(off + (s32)p));
    }

    if (D_800D2D28[0xC9] != 0) {
        func_80034888(D_800D2DAC, D_800CCB04 + 4, D_800CCB34);
    }
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_8007500C);
/* 62.98%: the loop, the two argument sets, the u16/u8 conversions and the
   frame all match; gcc 2.7.2 hoists `la D_800D3726` (4 uses in the loop) into
   a callee-saved register, and strength-reduces the `off + base` prim address
   into a second biv, while the original keeps no address register at all: it
   re-forms `lui %hi(sym); addu $at,$at,off; lbu %lo($at)` at each use and
   `addu a0,off,base` per call (i.e. retail current loop.c reduced nothing).
   Also open: the original reaches D_800CCB34 through `&D_800CCB04 + 0x30`
   (`lw v0,0x30(s3)` with `la D_800CCB04` hoisted) in the AddPrim index only.
   Tried: explicit `off` counter, inline `i * 0x60`, `(s32)&sym + off` casts,
   `sym[off]`, literal `[0]` in the i==0 branch, plus cdk / -O1 / -O3 / -G0
   (try_flags: 44.94-62.98). */
/*
    void func_8007500C(void) {
        s32 i;
        s32 off;

        i = 0;
        off = 0;
        do {
            if (D_800D3725[off] != 0) {
                if (i == 0) {
                    func_80076C78((POLY_FT4 *)((s32)&D_800D36C8 + D_800CCB34 * 40),
                                  0x40 - (D_800D3726[off] >> 1), 0x2C, 0, 0, D_800D3726[off]);
                } else {
                    func_80076C78((POLY_FT4 *)(off + ((s32)&D_800D36C8 + D_800CCB34 * 40)),
                                  0x9A - (D_800D3726[off] >> 1), 0xCA, 0,
                                  (13 * (i / 2)) & 0xFF, D_800D3726[off]);
                }
                AddPrim(D_800CCB04 + 4,
                        (u8 *)(off + ((s32)&D_800D36C8 + D_800CCB34 * 40)));
            }
            i++;
            off += 0x60;
        } while (i < 8);
    }*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80075168);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80075938);
void func_80076418(void) {
    if (D_800C492A == 0) {
        func_8008FAD8();
        func_800742A0();
        func_80075938();
        func_80073538();
        func_80073A58();
        func_80073B64();
        func_80073E88();
        func_80073F08();
        func_8007500C();
        func_80074EEC();
        func_800745EC();
        BattleUpdateRotationBuffer();
        func_80073FB8();
        func_80088B80();
        func_80074AB8();
    }
}


void func_800764B4(void) {
    D_8005959C = 0;
    func_8008FAD8();
    func_801DE594();
    func_80073FB8();
}

void func_800764EC(void) {
    func_8008FAD8();
    func_80073538();
    func_80073F08();
    func_8007500C();
    func_80074F70();
    func_80073FB8();
    func_80088B80();
    func_80074AB8();
}

void func_80076544(void) {
    switch (D_800C3E4C) {
    case 0:
        func_800764B4();
        break;
    case 1:
        func_80076418();
        break;
    case 2:
        func_800764EC();
        break;
    }
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_800765C4);
/* 59.87%: first real attempt (this stub had no park). Shape recovered from the
   asm: pEntry = &D_800C3254[(D_800D3280 * 3 + arg0)] kept in one register;
   D_800D2D28 fields 0x44/0x34 written twice, the second time when
   (&D_800D32A1)[arg0 * 8] != 0 && D_800D2D24[arg0] != 7; then 0x3C=0x10,
   0x4C=0x98, a dead store of the divisor to 0x5C followed by 0x5C=0x100,
   0x54 = 0x34-5-0x3C, u16 0x104=0x800, u8 0xA9=6, 0x64/0x6C/0x106 = 0,
   0x54 = (height << 8) / divisor, u8 0xAB=1, D_800D2D28[0x90 + arg0]--.
   Ours reassociates the arithmetic (bare `div` where retail has `divu`, and the
   height/divisor temps land in different registers) and reloads D_800D2D28
   fewer times than retail, which rematerialises it three times in the tail.
   Retail's divisor is non-negative by construction: try unsigned types for the
   height/divisor pair first. */
/*
void func_800765C4(s32 arg0) {
    u16 *pEntry;
    s32 divisor;
    s32 height;

    pEntry = (u16 *)((D_800D3280 * 3 + arg0) * 2 + (s32)D_800C3254);
    *(s32 *)(D_800D2D28 + 0x44) = 0x1C;
    *(s32 *)(D_800D2D28 + 0x34) = arg0 * 0x60 + (*pEntry + 0x48);
    if ((&D_800D32A1)[arg0 * 8] != 0 && D_800D2D24[arg0] != 7) {
        *(s32 *)(D_800D2D28 + 0x44) = 0x24;
        *(s32 *)(D_800D2D28 + 0x34) = arg0 * 0x60 + (*pEntry + 0x44);
    }

    *(s32 *)(D_800D2D28 + 0x3C) = 0x10;
    *(s32 *)(D_800D2D28 + 0x4C) = 0x98;
    divisor = *(s32 *)(D_800D2D28 + 0x4C) - (*(s32 *)(D_800D2D28 + 0x44) + 5);
    *(s32 *)(D_800D2D28 + 0x5C) = divisor;
    *(s32 *)(D_800D2D28 + 0x5C) = 0x100;
    height = *(s32 *)(D_800D2D28 + 0x34) - 5 - *(s32 *)(D_800D2D28 + 0x3C);
    *(s32 *)(D_800D2D28 + 0x54) = height;
    *(u16 *)(D_800D2D28 + 0x104) = 0x800;
    *(u8 *)(D_800D2D28 + 0xA9) = 6;
    *(s32 *)(D_800D2D28 + 0x64) = 0;
    *(s32 *)(D_800D2D28 + 0x6C) = 0;
    *(u16 *)(D_800D2D28 + 0x106) = 0;
    *(s32 *)(D_800D2D28 + 0x54) = (height << 8) / divisor;
    D_800D2D28[0xAB] = 1;
    D_800D2D28[0x90 + arg0]--;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80076710);
/* 92.93%: everything structural matches (the 0xA9 loop, both func_80025FA8
   calls with their five stack arguments, the SetSemiTrans loop, the 0xAB loop,
   the two range tests, the 0x58 frame). Shapes that were needed: x and y are
   16-byte memory objects (`u16 v[8]`, only v[0] and v[4] used) - two u16 locals
   land at sp+0x28/0x2A and shrink the frame by 8 bytes; one `i` shared by the
   three loops keeps the middle loop's index in $s0 across the SetSemiTrans
   call, retail has it in $a3, so each loop needs its own variable; the
   SetSemiTrans offset needs its own `off` local to get retail's
   addu-in-the-jal-slot operand order. What is left is allocation only:
   (1) retail keeps &D_800CCB34 in $s0 from just after the 0x77 store and does
   `lw a3,0(s0)` at both call sites, gcc 2.7.2 rematerialises
   `lui/lw %lo(D_800CCB34)` at each site (a `s32 *pCtx` local does not survive,
   it folds back and costs 8 points); (2) loop 1 emits `addu v1,v1,v0` where
   retail has `addu v0,v0,v1` - gcc canonicalises the commutative operands, so
   swapping the C operands changes nothing; (3) the second call's
   `idx * 0x50 + 0x3768 + (s32)D_800C3EA4` chain interleaves with the stack-arg
   stores (retail emits the three shifts contiguously and adds the constant to
   the product, not to the base) and `li a1,0x1` is not stolen into the loop's
   back-edge delay slot; (4) the 0xAB loop's index is $a1 here, $a3 in retail. */
/*
void func_80076710(s32 arg0) {
    u16 v[8];
    s32 i;
    s32 j;
    s32 k;
    s32 idx;
    s32 off;

    for (i = 0; i < D_800D2D28[0xA9]; i++) {
        *(u32 *)(D_800D2D28 + 0x64) -= *(u32 *)(D_800D2D28 + 0x54);
        *(u32 *)(D_800D2D28 + 0x6C) += *(u32 *)(D_800D2D28 + 0x5C);
        v[0] = *(u16 *)(D_800D2D28 + 0x34) + (*(u32 *)(D_800D2D28 + 0x64) >> 8);
        v[4] = *(u16 *)(D_800D2D28 + 0x44) + (*(u32 *)(D_800D2D28 + 0x6C) >> 8);
    }

    D_800D2D28[0xA9] = 0;
    D_800D2D28[0x77] = 0;

    D_800D2D28[0x77] = func_80025FA8(D_800D2F5C, 0x52, D_800C3EA4 + 0x3768, D_800CCB34,
                                     v[0], v[4], *(u16 *)(D_800D2D28 + 0x104),
                                     *(u16 *)(D_800D2D28 + 0x104),
                                     *(u16 *)(D_800D2D28 + 0x106));

    idx = D_800D2D28[0x77];
    D_800D2D28[0x77] +=
        func_80025FA8(D_800D2F5C, 0x53, idx * 0x50 + 0x3768 + (s32)D_800C3EA4, D_800CCB34,
                      v[0], v[4], *(u16 *)(D_800D2D28 + 0x104), *(u16 *)(D_800D2D28 + 0x104),
                      *(u16 *)(D_800D2D28 + 0x106));

    for (j = idx * 2; j < D_800D2D28[0x77] * 2; j += 2) {
        off = (j + D_800CCB34) * 40 + 0x3768;
        SetSemiTrans((u8 *)(off + (s32)D_800C3EA4), 1);
    }

    D_800D2D28[0x87] = *(u8 *)&D_800CCB34;

    for (k = 0; k < D_800D2D28[0xAB]; k++) {
        *(u16 *)(D_800D2D28 + 0x104) += 0x66;
        *(u16 *)(D_800D2D28 + 0x106) += 0x80;
    }
    D_800D2D28[0xAB] = 0;

    if (v[0] <= *(u32 *)(D_800D2D28 + 0x3C)) {
        *(u8 *)(D_800D2D28 + arg0 + 0x90) = 1;
    }
    if (v[4] >= *(u32 *)(D_800D2D28 + 0x4C)) {
        *(u8 *)(D_800D2D28 + arg0 + 0x90) = 1;
    }
}
*/

void func_800769E8(RECT *pRect, u_long *pData) {
    LoadImage(pRect, pData);
    DrawSync(0);
}

void func_80076A10(s32 arg0, s32 arg1, s16 arg2, s16 arg3) {
    func_8002675C(D_800D2F5C, arg0, arg1, D_800CCB34, arg2, arg3, 0x1000);
}

void func_80076A6C(s32 arg0, s32 arg1, s16 arg2, s16 arg3) {
    func_8002675C(D_800D2F5C, arg0, arg1, D_800CCB34, arg2, arg3, 0x800);
}

