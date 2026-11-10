#include "common.h"

extern u8 *D_800C34B0;
extern u8 *D_800C3DFC;
extern u8 *D_800C3E00;
extern u8 D_800C34AD;
extern u8 D_800C3E50;
extern u8 D_800D2DC4;

extern u8 D_800D220E[];
extern u16 D_800D2C98;
extern u16 D_800D2C9E;

extern s32 rand(void);
extern s8 func_80097964(u8, u8, u16);
extern void func_800995A0(u8, u8, u16, s32);
extern void func_80098D2C(u8, u8);

void func_80095A78(void) {
    s8 ret = func_80097964(D_800C3DFC[0x1C], D_800C3DFC[0x1D],
                           *(u16 *)(D_800C3DFC + 0x1E));

    if (*(u16 *)(D_800C3DFC + 0xA) & 0x4000) {
        func_800995A0(D_800C3E50, D_800C3DFC[0x1D], *(u16 *)(D_800C3DFC + 0x1E), 5);
    } else {
        func_800995A0(D_800C3E50, D_800C3DFC[0x1D], *(u16 *)(D_800C3DFC + 0x1E),
                      D_800C3DFC[0x11]);
        if (ret != 1) {
            u8 *p = D_800C34B0;

            *(u8 *)(p + D_800C3E50 + 0x5FA0) = 6;
        }
    }
}
void func_80095B44(void) {
    u8 *p = D_800C3E00;

    if (func_80097964(p[2], p[3], *(u16 *)p) == 1) {
        u8 *q = D_800C3DFC;

        func_800995A0(D_800C3E50, q[0x1D], *(u16 *)(q + 0x1E), 5);
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80095BAC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80095D4C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80096018);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80096494);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80096824);
/* best: 99.74%. Only the quotient temp of `rand() % 100` differs: the original
   computes it in a0, gcc 2.7.2 hands it a1. Tried: inline `rand() % 100` in the
   condition, an `s32 r` local (99.23%), and a pointer local for the
   D_800C34B0 store. Instruction multiset is identical; allocator tie.

void func_80096824(void) {
    u8 *p = D_800C3E00;
    u8 *q = D_800C3DFC;
    s32 t = p[0x5B] + q[0x14];

    if (rand() % 100 >= t) {
        u8 *r = D_800C34B0;

        r[0x5FC7] = 0x38;
        D_800D2DC4 = 1;
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_800968C0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80096AB8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80096FBC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80097610);
void func_8009795C(void) {
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80097964);
void func_80097D08(void) {
    s16 i;

    for (i = 0xB; i != -1; i--) {
        *(u8 *)(D_800C34B0 + i + 0x5FA0) = 0xFF;
        *(u32 *)(D_800C34B0 + 0x5F6C + i * 4) = 0;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80097D5C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_8009892C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80098AF8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80098C6C);
/* best: 95.83%. Same instruction multiset; only the mask shift sits one slot
   later than in the original (it fills the back edge's delay slot instead of
   the `beqz`'s, i.e. gcc puts `mask <<= 1` after the loop test). The 8-bit
   loop counter, the `mask = 1` after the first call and everything else line
   up byte for byte.

void func_80098C6C(u8 sel) {
    u8 i;
    u16 mask;

    func_80097D08();

    mask = 1;
    for (i = 0; i < 11; i++) {
        if (*(u16 *)(D_800C34B0 + 0x5FAC) & mask) {
            func_80098D2C(i, sel);
        }
        mask <<= 1;
    }

    *(u16 *)(D_800C34B0 + 0x5FB0) = *(u16 *)(D_800D220E + sel * 0x10);

    if (D_800D2C9E & 0x8000) {
        D_800D2C98 = 0xC2;
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80098D2C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80099498);
/* best: 91.52%. Two allocations off: the original scales into the base
   (`addu v1,a2,v1`), mine into the index (`addu v1,v1,a2`), and the original
   puts `i++` in the `beqz`'s delay slot (mine emits it after `n++` with a nop
   in the slot). Tried: flat pointer arithmetic both operand orders, a
   D_800C34B0 pointer local (worse: hoists the base load), and a 0x170-step
   `BattleActor { pad0[0x7C]; u16 flags; }` array access (81.52%).

s32 func_80099498(void) {
    u8 i;
    u8 n;
    s32 ret = 0;
    switch (D_800C34AD) {
    case 2:
        n = 0;
        for (i = 0; i < 3; i++) {
            if ((*(u16 *)(i * 368 + D_800C34B0 + 0x7C) & 0xC002) != 0) {
                n++;
            }
        }
        if (n == 0) {
            ret = 1;
        }
        break;
    case 3:
        n = 0;
        for (i = 0; i < 3; i++) {
            if ((*(u16 *)(i * 368 + D_800C34B0 + 0x7C) & 0xC002) != 0) {
                n++;
            }
        }
        if (n != 2) {
            ret = 1;
        }
        break;
    }
    return ret;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_800995A0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80099890);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80099CF0);
void func_80099FB0(void) {
    u16 ab = *(u16 *)(D_800C3E00 + 0x8C) | *(u16 *)(D_800C3E00 + 0x8E);

    D_800C34B0[0x5FBC] = D_800C3DFC[0x20];
    D_800C34B0[0x5FBD] = D_800C3DFC[0x21];
    D_800C34B0[0x5FBE] = D_800C3DFC[0x22];
    D_800C34B0[0x5FBF] = D_800C3DFC[0x23];
    D_800C34B0[0x5FC0] = D_800C34B0[0x5FC2];

    ab >>= 12;
    if ((D_800C34B0[0x5FBE] & 0x3F) == 0) {
        D_800C34B0[0x5FBE] |= ab;
    }
}
void func_8009A074(void) {
    if (*(u16 *)(D_800C34B0 + 0x5FAC) & 7) {
        *(u16 *)(D_800C34B0 + 0x5FAC) = *(u16 *)(D_800C34B0 + 0x5FAE) & 7;
    }
    if (*(u16 *)(D_800C34B0 + 0x5FAC) & 0x7F8) {
        *(u16 *)(D_800C34B0 + 0x5FAC) = *(u16 *)(D_800C34B0 + 0x5FAE) & 0x7F8;
    }
}
