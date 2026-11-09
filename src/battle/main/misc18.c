#include "common.h"

extern u8 *D_800C34B0;
extern u8 *D_800C3DFC;
extern u8 *D_800C3E00;
extern u8 D_800C3E50;
extern u8 D_800D2DC4;

extern s32 rand(void);
extern s8 func_80097964(u8, u8, u16);
extern void func_800995A0(u8, u8, u16, s32);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80095A78);
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
   computes it in a0, gcc 2.7.2 hands it a1. The instruction multiset around it
   is byte-identical, so this is the allocator tie, not the source shape.

void func_80096824(void) {
    u8 *p = D_800C3E00;
    u8 *q = D_800C3DFC;
    s32 t = p[0x5B] + q[0x14];

    if (rand() % 100 >= t) {
        D_800C34B0[0x5FC7] = 0x38;
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80098D2C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80099498);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_800995A0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80099890);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80099CF0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc18", func_80099FB0);
void func_8009A074(void) {
    if (*(u16 *)(D_800C34B0 + 0x5FAC) & 7) {
        *(u16 *)(D_800C34B0 + 0x5FAC) = *(u16 *)(D_800C34B0 + 0x5FAE) & 7;
    }
    if (*(u16 *)(D_800C34B0 + 0x5FAC) & 0x7F8) {
        *(u16 *)(D_800C34B0 + 0x5FAC) = *(u16 *)(D_800C34B0 + 0x5FAE) & 0x7F8;
    }
}
