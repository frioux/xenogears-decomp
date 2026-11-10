#include "common.h"

extern u8 *D_800C34B0;
extern u8 *D_800C3DFC;
extern u8 *D_800C3E00;
extern u8 D_800C34AD;
extern u8 *D_800C3E34;
extern u8 D_800C3E50;
extern u8 D_800D2DC4;
extern u8 *D_800D2D6C;

extern u8 D_800D220E[];
extern u16 D_800D2C98;
extern u16 D_800D2C9E;

extern s32 rand(void);
extern s8 func_80097964(u8, u8, u16);
extern void func_800995A0(u8, u8, u16, s32);
extern void func_80098D2C(u8, u8);
extern void func_80094D24(void);

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
void func_80095BAC(void) {
    u8 t = D_800C3DFC[0x1C];

    if (t < rand() % 100) {
        u8 *p = D_800C34B0;

        *(u8 *)(p + D_800C3E50 + 0x5FA0) = 6;
    } else {
        if (D_800C3DFC[0x1D] & 0x80) {
            *(u16 *)(D_800C3E34 + 0x7C) &= 0xC000;
        }
        if (D_800C3DFC[0x1D] & 0x40) {
            *(u16 *)(D_800C3E34 + 0x80) = 0;
            *(u16 *)(D_800C3E34 + 0x7A) &= 0xFFDF;
        }
        if (D_800C3DFC[0x1D] & 0x20) {
            *(u16 *)(D_800C3E34 + 0x84) = 0;
        }
        if (D_800C3DFC[0x1D] & 0x10) {
            *(u16 *)(D_800C3E34 + 0x88) = 0;
        }
        if (D_800C3DFC[0x1D] & 0x8) {
            *(u16 *)(D_800C3E34 + 0x8C) = 0;
        }
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80095D4C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80096018);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80096494);
void func_80096824(void) {
    s32 t = D_800C3E00[0x5B] + D_800C3DFC[0x14];

    if (rand() % 100 >= t) {
        D_800C34B0[0x5FC7] = 0x38;
        D_800D2DC4 = 1;
    }
}
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
/* best: 95.16%, 94 instructions (same count as the original), in every preset
   screened (2.7.2, 2.6.0, 2.7.2-cdk), so the addiu fold below is not a preset
   question. Body, both
   branches, the outer/inner branch layout, the 9*x tails, the 0xA5/0xA0 clamp,
   the rand()%8-4 delta and the call are byte-identical. Left: gcc folds the
   row stride's +0x1058 / +0x2228 into the base add (addiu v0,a1,K then
   addu v1,v1,v0) where the original keeps it on the index (addiu v1,v1,K then
   addu v1,v1,a1), which then names the row pointer v0 instead of v1. Tried:
   the whole sum as ints with (s32)pBase, &pBase[idx + K] subscript form,
   (u8 *)(idx + K) + (s32)pBase (this one, 95.16), and a separate `s32 off`
   local for idx + K which DOES fix the addiu but flips t/other between a0 and
   v1 and drops to 90.97. Row strides: 0x5F0 (+0x1058) when bit 0x80 of
   actor[0x15A] is clear, 0x690 (+0x2228) when set, plus
   D_800C34B0[0x5FC2]*0x28; t = (s8)row[0x27]; v = t < other ? 9*(other-t) : 9
   with other = actor[0x5A] / actor[0x13C] respectively, 9*actor[0x5A] when
   sel >= 3; then v = 0xA5 - v; d = rand()%8 - 4; v -= d; func_80094D24();
   return v. The `d` local is load-bearing: `v -= rand() % 8 - 4` makes gcc
   fold the 4 into v before the call. func_80094D24 takes no arguments.

u8 func_80098AF8(u8 sel) {
    u8 *pBase = D_800C34B0;
    u8 *pActor = (u8 *)(sel * 0x170 + (s32)pBase);
    u8 *pRow;
    u8 other;
    s32 t;
    s32 v;
    s32 d;

    D_800C3E00 = pActor;
    D_800D2D6C = pActor + 0xA4;

    if (sel < 3) {
        if ((pActor[0x15A] & 0x80) == 0) {
            pRow = (u8 *)(sel * 0x5F0 + 0x1058) + (s32)pBase + pBase[0x5FC2] * 0x28;
            D_800C3DFC = pRow;
            t = (s8)pRow[0x27];
            other = pActor[0x5A];
            if (t < other) {
                v = (other - t) * 9;
            } else {
                v = 9;
            }
        } else {
            pRow = (u8 *)(sel * 0x690 + 0x2228) + (s32)pBase + pBase[0x5FC2] * 0x28;
            D_800C3DFC = pRow;
            t = (s8)pRow[0x27];
            other = pActor[0x13C];
            if (t < other) {
                v = (other - t) * 9;
            } else {
                v = 9;
            }
        }
    } else {
        v = pActor[0x5A] * 9;
    }

    if ((u16)v >= 0xA6) {
        v = 0xA0;
    }
    v = 0xA5 - v;
    d = rand() % 8 - 4;
    v -= d;
    func_80094D24();
    return v;
}
*/
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80098D2C);
s32 func_80099498(void) {
    u8 i;
    u8 n;
    s32 ret = 0;
    s32 mode = D_800C34AD;

    switch (mode) {
    case 1:
        break;
    case 2:
        n = 0;
        for (i = 0; i < 3; i++) {
            s32 off = i * 368;

            if ((*(u16 *)((s32)D_800C34B0 + off + 0x7C) & 0xC002) != 0) {
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
            s32 off = i * 368;

            if ((*(u16 *)((s32)D_800C34B0 + off + 0x7C) & 0xC002) != 0) {
                n++;
            }
        }
        if (n != 2) {
            ret = 1;
        }
        break;
    default:
        break;
    }
    return ret;
}
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
