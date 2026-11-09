#include "common.h"

typedef struct {
    /* 0x0 */ u8 r;
    /* 0x1 */ u8 g;
    /* 0x2 */ u8 b;
    /* 0x3 */ u8 a;
} Color4; // Size: 0x4

typedef struct {
    /* 0x00 */ u8 unk0[0x28];
} SlotMotion; // Size: 0x28

typedef struct {
    /* 0x00 */ SlotMotion unk0;
    /* 0x28 */ SlotMotion unk28;
    /* 0x50 */ u8 unk50;
    /* 0x51 */ s8 unk51;
    /* 0x52 */ u8 unk52;
    /* 0x53 */ u8 unk53;
    /* 0x54 */ u16 unk54;
    /* 0x56 */ u16 unk56;
    /* 0x58 */ u16 unk58;
    /* 0x5A */ u16 unk5A;
    /* 0x5C */ u16 unk5C;
    /* 0x5E */ u16 unk5E;
    /* 0x60 */ u32 unk60;
    /* 0x64 */ u16 unk64;
    /* 0x66 */ u16 unk66;
    /* 0x68 */ u8 unk68[0x14];
} BattleSlot; // Size: 0x7C

extern BattleSlot D_80092AD8[];
extern Color4 D_800926B8;
extern u32 D_800926A4;
extern u32 D_800926B0;
extern u32 D_800926B4;
extern u16 D_800926A8;
extern u16 D_800926AA;
extern u16 D_800926AC;
extern u16 D_800926AE;
extern u16 D_800926A0;
extern s16 D_80092698;
extern s16 D_8009269C;

void func_8007B270(u8 *p0, u8 *p1) {
    s32 r;
    s32 g;
    s32 b;

    r = p0[0] - 0x80;
    b = p0[2] - 0x80;
    g = p0[1] - 0x80;
    if (r < 0) {
        r = 0;
    }
    if (g < 0) {
        g = 0;
    }
    if (b < 0) {
        b = 0;
    }
    D_800926A8 = ((r >> 2) & 0x1F) + ((g << 3) & 0x3E0) + ((b << 8) & 0x7C00) | -0x8000;

    r = p1[0] - 0x80;
    b = p1[2] - 0x80;
    g = p1[1] - 0x80;
    if (r < 0) {
        r = 0;
    }
    if (g < 0) {
        g = 0;
    }
    if (b < 0) {
        b = 0;
    }
    D_800926AA = ((r >> 2) & 0x1F) + ((g << 3) & 0x3E0) + ((b << 8) & 0x7C00) | -0x8000;

    D_800926AC = 0;
    D_800926AE = 0x1111;
    D_800926A0 = LoadClut2((u_long *)&D_800926A8, D_80092698, D_8009269C);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007B388);

void func_8007BACC(void) {
    BattleSlot *p = D_80092AD8;
    s32 i;
    u32 n;
    u32 v;

    for (i = 0; i < 0x3C; i++, p++) {
        if (p->unk50 != 0) {
            if (p->unk51 == p->unk52) {
                p->unk50 = 0;
            } else {
                p->unk51++;
                if (p->unk53 == 0) {
                    p->unk66 += p->unk64;
                    p->unk56 += p->unk66;
                }
            }
        }
    }
    n = D_800926A4;
    v = D_800926B0;
    D_800926B0 = 0;
    D_800926A4 = n + 1;
    D_800926B4 = v;
}

void func_8007BB7C(void) {
    u8 *p = (u8 *)D_80092AD8;
    s32 i = 0x3B;

    do {
        p[0x50] = 0;
        i--;
        p += 0x7C;
    } while (i >= 0);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007BBA0);

void func_8007C100(Color4 *pColor) {
    D_800926B8 = *pColor;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007C124);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007C280);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007C880);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007CAA4);

s32 func_8007CD14(s32 a0, s32 a1, s32 a2, s32 a3) {
    return (a3 & 0x7F) | ((a0 << 7) & 0x80) | (a1 << 24) | ((a2 << 8) & 0xFFFF00);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007CD44);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007CF78);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007D068);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007D0B4);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007D190);

s32 func_8007D25C(s32 arg0) {
    if (arg0 < 0x10) {
        return 0;
    }
    return arg0 < 0x20;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007D274);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007D334);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007D65C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007D6B8);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007D7A8);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007D918);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007DB28);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc5", func_8007DC74);
