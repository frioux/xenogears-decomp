#include "common.h"

extern u8 *D_800C3D60;
extern u8 *D_800C3DFC;
extern u8 *D_800C3E00;
extern u8 *D_800C3E34;
extern u8 *D_800C34B0;
extern u8 *D_800D2DC8;
extern u8 D_800C3E50;
extern u8 D_800CCD3E[];
extern u8 D_800CCD8C[];
extern u8 D_800CCCE8[];
extern u8 D_800CCE4A[];
extern u16 D_800D2C94;
extern s8 func_8009DBFC();

INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009C134);
/* best 72.60%: only the i++ / delay-slot placement and the a2/a3 colour of the
   store base differ.
void func_8009C134(void) {
    u16 *p = &D_800D2C94;
    s32 o;
    u8 i;

    for (i = 0; i < 3; i++) {
        o = i * 0x170;
        if (D_800CCD3E[o] == 3) {
            *p = 1 << i;
        }
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009C198);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009C4B4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009C9C4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009CA90);
void func_8009CB68(u8 arg0) {
    s32 o = arg0 * 0x170;
    u8 *p = D_800CCD8C + o;
    u8 *q = D_800CCD8C + -0xA4 + o;
    u16 v;

    if (*(u16 *)(p + 0x80) & 0x200) {
        v = *(u16 *)(q + 0x84) | 0x20;
    } else {
        v = *(u16 *)(q + 0x84) & 0xFFDF;
    }
    *(u16 *)(q + 0x84) = v;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009CBC4);
void func_8009D354(void) {
    u8 *p;
    u8 i;

    if (func_8009DBFC(0) == 0) {
        p = D_800C34B0;
        i = D_800C3E50;
        (p + i)[0x5FA0] = 6;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009D3A0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009D948);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009DA04);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009DB54);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009DBFC);
void func_8009E268(void) {
    D_800D2DC8[0x99] = 0;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009E278);
/* best 84.83%: single allocator/coalescing difference on the divide result
   (mfhi/mfhi+srl colour) plus the order of the two loads feeding the sb.
void func_8009E278(void) {
    u8 *p = D_800D2DC8;
    s32 v = p[0x4F] * *(s32 *)(p + 0x64);

    (D_800C34B0 + D_800C3E50)[0x5FA0] = 2;
    *(s32 *)(D_800C3E50 * 4 + D_800C34B0 + 0x5F6C) = (u32)v / 10;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009E2EC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009E364);
/* best 99.60%: the mflo result colour (v1 in the original, a1 here) is the
   only difference; instruction multiset is identical.
void func_8009E364(void) {
    s32 v = D_800C3E00[0x5B] * D_800C3DFC[0x11];

    (D_800C34B0 + D_800C3E50)[0x5FA0] = 2;
    *(s32 *)(D_800C3E50 * 4 + D_800C34B0 + 0x5F6C) = v;
}
*/
void func_8009E3C8(void) {
    s32 o;

    D_800C3D60[0] = 4;
    o = D_800C3E50 * 0x170;
    D_800CCE4A[o] = 3;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009E410);
/* best 72.58%: divide is /20 (sra 3) and matches; remaining diff is the
   mfhi/sra/subu placement vs the base load and the a0/v1 colour.
void func_8009E410(void) {
    s32 v = *(u16 *)(D_800D2DC8 + 0x3A) * D_800C3DFC[0x11];

    (D_800C34B0 + D_800C3E50)[0x5FA0] = 0xA;
    *(s32 *)(D_800C3E50 * 4 + D_800C34B0 + 0x5F6C) = v / 20;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009E48C);
void func_8009E508(void) {
    u8 *p = D_800D2DC8;
    u8 *q = D_800C3E34;

    *(u16 *)(p + 0x7C) &= 0xF80B;
    *(u16 *)(q + 0x7A) &= 0xFFDF;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009E53C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009E5C8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009E788);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009E868);
