#include "common.h"
#include "main/game.h"

extern u8 *D_800C3D60;
extern u8 *D_800C3DFC;
extern u8 *D_800C3E00;
extern u8 *D_800C3E34;
extern u8 *D_800C34B0;
extern u8 *D_800D2DC8;
extern u8 *D_800D2D6C;
extern u8 D_800C3E50;
extern u8 D_800CCD3E[];
extern u8 D_800CCD8C[];
extern u8 D_800CCCE8[];
extern u8 D_800CCE4A[];
extern u16 D_800D2C94;
extern s8 func_8009DBFC();
extern s32 func_80097610();

void func_8009C134(void) {
    u8 i = 0;
    s32 target = 3; /* local, not a literal: the compare constant must live in a
                       register outside the loop */
    u16 *p = &D_800D2C94;
    s32 o;

    for (; i < 3; i++) {
        o = i * 0x170;
        if (D_800CCD3E[o] == target) {
            *p = 1 << i;
        }
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009C198);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009C4B4);
void func_8009C9C4(void) {
    u16 a = *(u16 *)(D_800D2D6C + 0x84);
    u16 b = *(u16 *)(D_800C3E00 + 0x8A);
    u16 c;

    D_800C34B0[0x5FBC] = D_800C3DFC[0x20];
    D_800C34B0[0x5FBD] = D_800C3DFC[0x21];
    D_800C34B0[0x5FBE] = D_800C3DFC[0x22];
    D_800C34B0[0x5FBF] = D_800C3DFC[0x23];
    D_800C34B0[0x5FC0] = D_800C34B0[0x5FC2];
    c = (a | b) >> 12;
    if ((D_800C34B0[0x5FBE] & 0x3F) == 0) {
        D_800C34B0[0x5FBE] |= c;
    }
}
/* best 99.91%: head, both index chains, both stores and the shared store all
   match. Only the flag test's fail branch differs: the original targets the
   else block (.L8009CAFC), gcc jumps to the epilogue (same net effect, one
   instruction different). Writing the flag test as a flat `A && B && C` chain
   produces the original branch but flips the index add's operands to
   `addu v1,v1,a0` (99.63%); the nested `if` gives `addu v1,a0,v1` but jumps to
   the epilogue. Six shapes tried.
void func_8009CA90(void) {
    u8 *p = D_800C34B0;

    if (p[0x5FC2] >= 0xC && p[0x5FC2] < 0xF) {
        u8 *q = p + D_800C3E50 * 0x170;
        if (!(q[0x15A] & 0x80)) {
            D_800C3DFC += 0x78;
        }
    } else {
        if (p[0x5FC2] < 3) {
            u8 *q = p + D_800C3E50 * 0x170;
            if (!(q[0x15A] & 0x80)) {
                D_800C3DFC += 0x258;
            }
        }
    }
}
*/
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
/* best 95.32%: head, loop and both stores match exactly. Only the last
   return-value mask is emitted after the `lw ra` of the epilogue instead of
   in the join block, and the stack restore takes the jr delay slot (the
   original leaves it as a nop). Three declarations/mask shapes (u16 vs s32
   return, two returns, explicit `& 0xFFFF`) all reproduce the same epilogue.
   Note the call returns int (declaring it u16 moves the mask to the call).
u16 func_8009D948(void) {
    u16 ret = func_80096FBC();
    u16 f;
    u32 b;
    s32 n;
    if (*(u16 *)(D_800C3DFC + 0xA) & 0x100) {
        f = *(u16 *)(D_800D2D6C + 0x7C);
        b = D_800D2D6C[0x9E];
        if ((f & 0x100) || b == 0) {
            n = 4;
        } else {
            n = b;
        }
        ret = n * ret / 4;
        if ((*(u16 *)(D_800D2D6C + 0x80) | *(u16 *)(D_800D2D6C + 0x82)) & 0x40) {
            ret = ret + ((ret & 0xFFFF) >> 1);
        }
    }
    return ret;
}
*/
/* best 98.21%: arm order, both divide-by-100 sites, the shared guard add and
   the return mask all match. Remaining: the guard add comes out as
   `addu a1,a1,v0` after the branch instead of the original's speculative
   `addu v0,a1,a0` + `move a1,v0` in the beqz delay slot, the >>1 is sra here
   and srl in the original (so `ret` wants an unsigned type, but an unsigned
   `ret` turns the /100 into an unsigned division), and the 0x15A index chain
   lands in v0 instead of v1.  (u16 v) vs (s32 v) + cast: 98.21 / 97.20.
u16 func_8009DA04(void) {
    s32 ret = func_80097610();
    u16 v;
    u8 n;

    if (*(u16 *)(D_800C3DFC + 0xA) & 0x100) {
        v = *(u16 *)(D_800D2DC8 + 0x72);
        n = D_800D2DC8[0x99];
        if (n != 0 && v != 0) {
            v = v * (100 - n) / 100;
        }
        if (*(u8 *)(D_800C34B0 + D_800C3E50 * 0x170 + 0x15A) & 0x80) {
            ret = ret + v;
        }
        if ((*(u16 *)(D_800D2DC8 + 0x80) | *(u16 *)(D_800D2DC8 + 0x82)) & 0x20) {
            ret = ret + ((ret & 0xFFFF) >> 1);
        }
    } else {
        n = D_800D2DC8[0x99];
        if (n != 0) {
            ret = (ret & 0xFFFF) * (100 - n) / 100;
        }
    }
    return ret;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009DA04);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009DB54);
/* best 93.93% under gcc 2.6.0 (92.38% under 2.7.2): the head and the divide
   match, but the original has one extra block - the `break` path copies the
   index into v1 (`j .L8009DB8C; addu v1,a1,zero`) before the shared
   `lbu 0x88(D_800D2DC8 + i)` load, so the loop counter and the index used after
   the loop are not the same source variable there.  Also the two loop
   invariants come out mirrored (0x8000 in a2 / mask in v1 here, a3 / a2 in the
   original); making 0x8000 a local did not move it.
s32 func_8009DB54(s32 arg0) {
    u8 i = 0;
    u16 mask = *(u16 *)(D_800C3DFC + 8);
    s32 bit = 0x8000;
    u8 v;

    for (; i < 0x10; i++) {
        if (mask & (bit >> i)) {
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
void func_8009E278(void) {
    u8 *p = D_800D2DC8;
    u32 v = p[0x4F] * *(s32 *)(p + 0x64);
    u32 q = v / 10;
    u8 *base = D_800C34B0;

    (base + D_800C3E50)[0x5FA0] = 2;
    *(s32 *)(D_800C3E50 * 4 + D_800C34B0 + 0x5F6C) = q;
}
void func_8009E2EC(void) {
    u32 v = D_800C3DFC[0x11] * *(s32 *)(D_800D2DC8 + 0x64);
    u32 q = v / 20;
    u8 *base = D_800C34B0;

    (base + D_800C3E50)[0x5FA0] = 2;
    *(s32 *)(D_800C3E50 * 4 + D_800C34B0 + 0x5F6C) = q;
}
void func_8009E364(void) {
    s32 v = D_800C3E00[0x5B] * D_800C3DFC[0x11];

    (D_800C34B0 + D_800C3E50)[0x5FA0] = 2;
    *(s32 *)(D_800C3E50 * 4 + D_800C34B0 + 0x5F6C) = v;
}
void func_8009E3C8(void) {
    s32 o;

    D_800C3D60[0] = 4;
    o = D_800C3E50 * 0x170;
    D_800CCE4A[o] = 3;
}
void func_8009E410(void) {
    u8 *base;
    s32 v = *(u16 *)(D_800D2DC8 + 0x3A) * D_800C3DFC[0x11];
    s32 q = v / 20;

    base = D_800C34B0;
    (base + D_800C3E50)[0x5FA0] = 0xA;
    *(s32 *)(D_800C3E50 * 4 + D_800C34B0 + 0x5F6C) = q;
}
void func_8009E48C(void) {
    u8 *base;
    s32 v = *(u16 *)(D_800D2DC8 + 0x3A) * D_800C3DFC[0x11];
    s32 q = v / 20;

    base = D_800C34B0;
    (base + D_800C3E50)[0x5FA0] = 0xB;
    *(s32 *)(D_800C3E50 * 4 + D_800C34B0 + 0x5F6C) = q;
}
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
/* best 99.91%: head, the three weapon compares, both pointers and the loop
   all match instruction for instruction. One instruction differs: the last
   store in the loop body adds the row offset to the reloaded base as
   `addu v0,a3,v0` here vs the original's `addu v0,v0,a3` (same registers,
   commutative operand order only); the four stores above it do come out
   base-first because their index is a gcc-CSEd temporary. Shapes tried for
   that one store: array form, pointer arith, parenthesised row sum, the
   product first, a cast to u8 (*)[0x170] - all flip both adds or the
   schedule and lose bytes. 8 shapes.
void func_8009E5C8(u8 arg0) {
    u8 *p = &D_800C34B0[arg0 * 0x14 + 0x5750];
    u8 v = p[0xF];
    u8 id = g_GameState.characters[4].gearId;
    u8 sel;
    u8 i;

    if (g_GameState.gears[id].weaponData[0].unk5 == v) {
        sel = 0;
    }
    if (g_GameState.gears[id].weaponData[1].unk5 == v) {
        sel = 1;
    }
    if (g_GameState.gears[id].weaponData[2].unk5 == v) {
        sel = 3;
    }
    g_GameState.gears[id].weaponData[sel].unk2 = p[0xE];
    g_GameState.gears[id].weaponData[sel].unk4 = p[0x11];
    g_GameState.gears[id].weaponData[sel].unk3 = p[0x10];
    g_GameState.gears[id].weaponData[sel].unk4 = p[0x11];
    g_GameState.gears[id].unk4[sel] = arg0;
    g_GameState.gearPartsIds[0x6C + arg0] = p[0xC];
    for (i = 0; i < 3; i++) {
        if (D_800C34B0[i * 0x170 + 0x56] == 4) {
            *(u8 *)(D_800C34B0 + (i * 0x170 + sel * 8) + 0xB6) = p[0xE];
            *(u8 *)(D_800C34B0 + (i * 0x170 + sel * 8) + 0xB8) = p[0x11];
            *(u8 *)(D_800C34B0 + (i * 0x170 + sel * 8) + 0xB7) = p[0x10];
            *(u8 *)(D_800C34B0 + (i * 0x170 + sel * 8) + 0xB8) = p[0x11];
            *(u8 *)(D_800C34B0 + i * 0x170 + sel + 0xA8) = arg0;
        }
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009E788);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc20", func_8009E868);
