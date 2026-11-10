#include "common.h"

extern void func_800379C8(char*, ...);
extern void func_80077584(u8* p);
extern char D_8006FCC0[];
extern char D_8006FCC4[];
extern char D_8006FCC8[];

s32 func_80075738(u8* p0, u8* p1) {
    return *(s16*)(p1 + 0x15EA) - *(s16*)(p0 + 0x15E8);
}

void func_80075748(void) {
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc3", func_80075750);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc3", func_80075888);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc3", func_80075A4C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc3", func_80075B50);

void func_8007639C(u8* p, u8 value) {
    u8 idx;
    u8* pSlot;

    if (p[0x9C2] < 0x20) {
        idx = p[0x9C0];
        p[0x9C0] = idx + 1;
        pSlot = p + (idx & 0x1F);
        pSlot[0x9A0] = value;
        p[0x9C2]++;
    }
}

u8 func_800763E4(u8* p) {
    u8 idx;
    u8* pSlot;
    u8 value;

    if (p[0x9C2] == 0) {
        return 0;
    }
    idx = p[0x9C1]++;
    pSlot = p + (idx & 0x1F);
    value = pSlot[0x9A0];
    p[0x9C2]--;
    return value;
}

void func_80076424(u8* p) {
    p[0x9C2] = 0;
    p[0x9C0] = 0;
    p[0x9C1] = 0;
    p[0x9C3] = 0;
}

void func_80076438(u8* p) {
    u8 idx = p[0x9C1];
    u8 value;
    s32 i;

    func_800379C8(D_8006FCC0);
    for (i = 0; i < p[0x9C2]; i++) {
        value = *(u8*)(p + (idx & 0x1F) + 0x9A0);
        idx++;
        func_800379C8(D_8006FCC4, value);
    }
    func_800379C8(D_8006FCC8);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc3", func_800764CC);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc3", func_8007661C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc3", func_800767C8);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc3", func_80076884);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc3", func_80077038);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc3", func_80077584);

void func_8007762C(u8* p) {
    p[0x4C] = 8;
    p[0x4E] = 0xFF;
    *(u16*)(p + 0xCA) = 0x3C;
    *(u16*)(p + 0x916) = 0x28;
    *(u32*)(p + 0xE8) = 0x28;
    p[0xC4] = 4;
    p[0xC5] = 0;
    *(u32*)(p + 0xD0) |= 0x1000;
    *(u32*)(p + 0xD4) &= ~0x10;
    *(u32*)(p + 0xD0) &= ~0x400;
    func_80077584(p);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc3", func_800776A8);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc3", func_80077770);

extern u8 D_80091178[];
extern u8 D_80091179[];

u8* func_80077A38(u8* p, s32 arg1) {
    s32 idx = p[0x9C3] * 2;

    if (arg1 != 0) {
        p[0x9C3] = D_80091178[idx];
    } else {
        p[0x9C3] = D_80091179[idx];
    }
    return *(u8**)(p + 0x80) + p[0x9C3] * 4;
}

void func_80077A88(u8* p) {
    *(u32*)(p + 0xD0) |= 0x80000;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc3", func_80077A9C);

typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
    /* 0x8 */ s32 z;
    /* 0xC */ s32 w;
} Vec4;

void func_80078154(u8* p) {
    *(Vec4*)p = *(Vec4*)(p + 0x93C);
    p[0x4C] = 0;
    p[0xC4] = 0;
    p[0xC5] = 0;
    *(u32*)(p + 0xD0) |= 0x2000000;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc3", func_80078194);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc3", func_80078704);
