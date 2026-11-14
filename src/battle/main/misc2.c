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
void func_80074D4C(void) {
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
        func_80074D4C();
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80076710);

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

