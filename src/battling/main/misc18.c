#include "common.h"

typedef struct {
    /* 0x0 */ s32 x;
    /* 0x4 */ s32 y;
    /* 0x8 */ s32 z;
    /* 0xC */ s32 w;
} Vec4; // Size: 0x10

extern s32 func_80073DE4(u8* p, s32 arg1);
extern void func_800767C8(u8* p);
extern void func_8007639C(u8* p, s32 arg1);
extern u8 D_80092884;
extern u32 func_800828C4(s32* p);
extern void func_8008F7B8(u8* p);
extern s32 func_8008F720(u8* p, s32 arg1);
extern s32 func_8008F580(u8* p);
extern s32 D_8009284C;

s32 func_8008F530(u8* pArg0, s32 arg1) {
    if (arg1 != 0) {
        return func_80073DE4(pArg0, *(s16*)(pArg0 + 0xBE));
    }
    return *(s16*)(pArg0 + 0xB6) < 0x1000 - *(s16*)(pArg0 + 0xBE);
}

s32 func_8008F570(u8* pArg0) {
    return 0x1000 - *(s16*)(pArg0 + 0xBE);
}

s32 func_8008F580(u8* pArg0) {
    s32 v1;
    s32 v0;

    v1 = *(s16*)(pArg0 + 0xB6);
    v0 = *(s32*)(*(u8**)(pArg0 + 0x15FC) + 0x24);
    return (v0 - 0x200 < v1) ? !(v0 < v1) : 2;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc18", func_8008F5B4);
s32 func_8008F720(u8* pArg0, s32 arg1) {
    u8* p1;

    p1 = *(u8**)(pArg0 + 0x15FC);
    if (arg1 != 0) {
        if ((rand() & 0xFF) < *(s32*)(p1 + 0x10)) {
            if (D_8009284C < 0x600) {
                return 0;
            }
        }
    }
    if (func_8008F580(pArg0) != 0) {
        return 1;
    }
    return ((rand() & 0xFF) < *(s32*)(p1 + 0x1C));
}INCLUDE_ASM("asm/battling/nonmatchings/main/misc18", func_8008F7B8);
void func_8008F900(u8* pArg0) {
    if (((*(u32*)(*(u8**)(pArg0 + 0xD8) + 0xD0)) & 0x60000000) == 0x20000000) {
        if ((rand() & 3) != 0) {
            return;
        }
    }
    if (D_80092884 != 0) {
        func_800767C8(pArg0);
        func_8007639C(pArg0, 4);
        func_8007639C(pArg0, 3);
        return;
    }
    if (((*(u32*)(pArg0 + 0xD0)) & 0x60000000) != 0x20000000) {
        func_8007639C(pArg0, 3);
        return;
    }
    func_8007639C(pArg0, 4);
    func_8007639C(pArg0, 3);
}s32 func_8008F9B0(s32* pArg0) {
    s32 sp[4];
    s32 a;
    s32 b;

    *(Vec4*)sp = *(Vec4*)pArg0;
    a = sp[0] - 0x3F80;
    b = sp[2] - 0x3F80;
    sp[0] = a;
    sp[2] = b;
    if (a > 0 && b > 0) {
        return 1;
    }
    return ((func_800828C4(pArg0) & 0x3000000) ^ 0x1000000) < 1;
}INCLUDE_ASM("asm/battling/nonmatchings/main/misc18", func_8008FA2C);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc18", func_8008FACC);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc18", func_8008FBD8);
void func_8008FC7C(u8* pArg0) {
    u8* p;

    p = *(u8**)(pArg0 + 0x15FC);
    *(s32*)(p + 0x20) = 0;
    *(u8*)(p + 0x8) = 0;
    *(s16*)(p + 0x4) = (2 - *(u8*)(p + 0xF)) * 30 + 0x5A;
    func_8008F7B8(p);
}INCLUDE_ASM("asm/battling/nonmatchings/main/misc18", func_8008FCC8);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc18", func_8008FE80);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc18", func_8008FF24);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc18", func_8008FFEC);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc18", func_80090174);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc18", func_80090258);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc18", func_8009031C);
void func_80090504(u8* pArg0) {
    u8* p;
    s32 r;

    p = *(u8**)(pArg0 + 0x15FC);
    *(u8*)(p + 0x8) = 3;
    r = rand();
    *(u8*)(p + 0x9) = r % 4 + 1;
    *(s16*)(p + 0x4) = 0;
    func_8008F7B8(p);
    *(u8*)(p + 0xE) = func_8008F720(pArg0, 1);
    *(u8*)(p + 0x2E) = 0;
}INCLUDE_ASM("asm/battling/nonmatchings/main/misc18", func_80090580);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc18", func_80090894);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc18", func_80090990);
void func_80090C88(u8* pArg0) {
    u8* p1;
    u8* p0;

    p1 = *(u8**)(pArg0 + 0x1600);
    p0 = *(u8**)(pArg0 + 0x15FC);
    *(s32*)(p0 + 0x10) = *(u8*)(p1 + 0x6);
    *(s32*)(p0 + 0x14) = *(u8*)(p1 + 0x7);
    *(s32*)(p0 + 0x18) = *(u8*)(p1 + 0x8);
    *(s32*)(p0 + 0x1C) = *(u8*)(p1 + 0x9);
}INCLUDE_ASM("asm/battling/nonmatchings/main/misc18", func_80090CC0);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc18", func_80090E10);
