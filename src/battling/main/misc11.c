#include "common.h"

extern u8 D_8006F978[];
extern u8 *D_800927CC;
extern u8 *D_800927D0;
extern u8 D_80095968[];

extern void *func_8008820C();

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_80087068);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_800875EC);

void func_80087650(void) {
    s32 i;

    for (i = 0; i < 0x80; i++) {
        D_800927CC[i] = 0;
        D_800927D0[i] = 0xFF;
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_80087698);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_8008779C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_80087830);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_800878DC);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_80087AB0);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_80087B74);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_80087E38);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_80087EA0);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_8008820C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_800882D4);

void func_80088308(void) {
    s32 i;

    for (i = 0x141C; i >= 0; i -= 0x34) {
        *(s16 *)(D_80095968 + i) = 0;
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_8008832C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_800884E0);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_8008859C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_80088658);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_800886FC);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_80088754);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_800887A4);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_80088838);

void func_800888B0(s32 arg0) {
    s32 bit = 1;

    bit <<= arg0 & 7;
    D_8006F978[arg0 >> 3] |= bit;
}

s32 func_800888E4(s32 arg0) {
    s32 bit = 1;

    bit <<= arg0 & 7;
    return D_8006F978[arg0 >> 3] & bit;
}

void func_80088908(s32 arg0) {
    s32 mask = 1;

    mask <<= arg0 & 7;
    D_8006F978[arg0 >> 3] &= ~mask;
}
