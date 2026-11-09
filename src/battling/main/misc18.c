#include "common.h"

extern s32 func_80073DE4(u8* p, s32 arg1);
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
INCLUDE_ASM("asm/battling/nonmatchings/main/misc18", func_8008F900);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc18", func_8008F9B0);
INCLUDE_ASM("asm/battling/nonmatchings/main/misc18", func_8008FA2C);
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
