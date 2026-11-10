#include "common.h"
#include "main/game.h"

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
/* best 72.60% (100 bytes, same size): only the i++ / delay-slot placement and
   the a2/a3 colour of the store base differ.  Using D_800D2C94 directly scores
   76.56% but is one instruction short, so the pointer local is the right shape.
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
/* best 92.38%: single-exit shape with the parameter overwritten in place (that
   is what puts the accumulator in a0 and frees a2 for the mask). Left over: the
   loop-invariant colours (0x8000 a3 / mask a2 original, a2 / v1 here) and the
   mfhi result colour, which turn every register after the loop into a mirror.
s32 func_8009DB54(s32 arg0) {
    u16 mask = *(u16 *)(D_800C3DFC + 8);
    u8 i;
    u8 v;

    for (i = 0; i < 0x10; i++) {
        if (mask & (0x8000 >> i)) {
            break;
        }
    }
    v = D_800D2DC8[i + 0x88];
    if (v != 0) {
        if (v < 0x14) {
            arg0 = arg0 * (0x14 - v) / 20;
        } else {
            arg0 = 0;
        }
    }
    return arg0;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009DBFC);
void func_8009E268(void) {
    D_800D2DC8[0x99] = 0;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009E278);
/* best 98.97%: D_800C34B0 loaded through a pointer local reproduces the original
   load order of the first store. Only the mfhi result colour differs (v1 in the
   original, a1 here), and the store base colour follows from it.
void func_8009E278(void) {
    u8 *base;
    u8 *p = D_800D2DC8;
    u32 v = p[0x4F] * *(s32 *)(p + 0x64);

    base = D_800C34B0;
    (base + D_800C3E50)[0x5FA0] = 2;
    *(s32 *)(D_800C3E50 * 4 + D_800C34B0 + 0x5F6C) = v / 10;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009E2EC);
/* best 99.00%: divide is unsigned /20 (multu 0xCCCCCCCD, mfhi, srl 4); the
   pointer local fixes the load order of the first store. Only the mfhi result
   colour differs (v1 original, a1 here).
void func_8009E2EC(void) {
    u8 *base;
    u32 v = D_800C3DFC[0x11] * *(s32 *)(D_800D2DC8 + 0x64);

    base = D_800C34B0;
    (base + D_800C3E50)[0x5FA0] = 2;
    *(s32 *)(D_800C3E50 * 4 + D_800C34B0 + 0x5F6C) = v / 20;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009E364);
/* best 99.60%: the mflo result colour is the only difference (v1 in the
   original, a1 here); the instruction multiset is identical.
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
/* best 72.58%: signed /20 (magic 0x66666667, sra 3, subu); the head matches,
   but the original emits the mfhi/sra/subu tail before the second store base
   load while this shape emits it after, so every later register mirrors.
void func_8009E410(void) {
    u8 *base;
    s32 v = *(u16 *)(D_800D2DC8 + 0x3A) * D_800C3DFC[0x11];

    base = D_800C34B0;
    (base + D_800C3E50)[0x5FA0] = 0xA;
    *(s32 *)(D_800C3E50 * 4 + D_800C34B0 + 0x5F6C) = v / 20;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009E48C);
/* best 72.58%: twin of func_8009E410, only the stored byte differs (0xB).
void func_8009E48C(void) {
    u8 *base;
    s32 v = *(u16 *)(D_800D2DC8 + 0x3A) * D_800C3DFC[0x11];

    base = D_800C34B0;
    (base + D_800C3E50)[0x5FA0] = 0xB;
    *(s32 *)(D_800C3E50 * 4 + D_800C34B0 + 0x5F6C) = v / 20;
}
*/
void func_8009E508(void) {
    u8 *p = D_800D2DC8;
    u8 *q = D_800C3E34;

    *(u16 *)(p + 0x7C) &= 0xF80B;
    *(u16 *)(q + 0x7A) &= 0xFFDF;
}
s32 func_8009E53C(u8 arg0) {
    u8 id = g_GameState.characters[4].gearId;
    u8 v = D_800C34B0[arg0 * 0x14 + 0x5B47];

    if (g_GameState.gears[id].weaponData[0].unk5 == v ||
        g_GameState.gears[id].weaponData[1].unk5 == v) {
        return 1;
    }
    return g_GameState.gears[id].weaponData[2].unk5 == v;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009E5C8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009E788);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009E868);
