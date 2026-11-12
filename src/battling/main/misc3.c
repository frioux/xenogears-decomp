#include "common.h"
#include "psyq/libgte.h"

extern void func_800379C8(char*, ...);
extern char D_8006FCC0[];
extern char D_8006FCC4[];
extern char D_8006FCC8[];
extern s32 D_8009284C;
extern u8 D_80091178[];
extern u8 D_80091179[];

s32 func_80075738(u8* p0, u8* p1) {
    return *(s16*)(p1 + 0x15EA) - *(s16*)(p0 + 0x15E8);
}

void func_80075748(void) {
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc3", func_80075750);

/* 85.58%: the whole body, frame (0x40, three saved regs), the two divides,
   the 0xFFF rounding and the tail all line up; the only residual is that gcc
   gives $s1 to `ex = x3 - x1` and $s0 to `ey = y3 - y1` while the original has
   the reverse (every downstream mult operand is swapped with them). Tried:
   both declaration orders for ex/ey, both assignment orders (that one also
   swaps the $v1/$t0 temps and drops to 85.13%), ex/ey declared after the other
   locals. Allocation order follows the RTL definition order, so the pair cannot
   be flipped without also flipping the emitted order.
s32 func_80075750(s32 x1, s32 y1, s32 x2, s32 y2, s32 x3, s32 y3, s32 r) {
    VECTOR v;
    VECTOR vSq;
    s32 dist;
    s32 nx;
    s32 ny;
    s32 along;
    s32 cross;
    s32 ey;
    s32 ex;

    v.vx = x2 - x1;
    v.vy = y2 - y1;
    v.vz = r;
    ex = x3 - x1;
    ey = y3 - y1;
    Square0(&v, &vSq);
    dist = SquareRoot0(vSq.vx + vSq.vy);
    if (dist == 0) {
        return 0;
    }
    nx = (v.vx << 12) / dist;
    ny = (v.vy << 12) / dist;
    along = ex * nx + ey * ny;
    if (along < 0) {
        along += 0xFFF;
    }
    along >>= 12;
    cross = ey * nx - ex * ny;
    if (cross < 0) {
        cross += 0xFFF;
    }
    cross >>= 12;
    if (along < 0) {
        return 0;
    }
    return (cross * cross - r * r < 0) & (dist + r >= along);
}
*/

INCLUDE_ASM("asm/battling/nonmatchings/main/misc3", func_80075888);

extern s32 D_80092654;
extern s32 D_80092658;

INCLUDE_ASM("asm/battling/nonmatchings/main/misc3", func_80075A4C);

/* 83.62%: two residuals. The original stores D_80092654/58 AFTER the four
   arg-register loads (order: stack stores, arg loads, global stores, jal);
   this source emits the global stores before the stack stores. And the
   original's three `return 1` blocks are threaded into the bnez delay slots
   while gcc leaves the third as a j + li block after the last call. Tried:
   if/return chain, || chain (adds a saved reg), r = call(); if (r != 0)...,
   and `? 1 : 0` for the tail.
s32 func_80075A4C(u8* p, s32 a, s32 b, s32 c) {
    s32 r;

    D_80092654 = a;
    D_80092658 = b;
    r = func_80075750(*(s32*)(p + 0x10), *(s32*)(p + 0x18), *(s32*)(p + 0x0), *(s32*)(p + 0x8), a, b, c);
    if (r != 0) {
        return 1;
    }
    r = func_80075750(*(s32*)(p + 0x30), *(s32*)(p + 0x38), *(s32*)(p + 0x20), *(s32*)(p + 0x28), a, b, c);
    if (r != 0) {
        return 1;
    }
    r = func_80075750(*(s32*)(p + 0x0), *(s32*)(p + 0x8), *(s32*)(p + 0x20), *(s32*)(p + 0x28), a, b, c);
    if (r != 0) {
        return 1;
    }
    return func_80075750(*(s32*)(p + 0x10), *(s32*)(p + 0x18), *(s32*)(p + 0x30), *(s32*)(p + 0x38), a, b, c) ? 1 : 0;
}
*/

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
    s32 i;
    s32 idx;

    idx = p[0x9C1];
    func_800379C8(D_8006FCC0);
    for (i = 0; i < p[0x9C2]; i++) {
        func_800379C8(D_8006FCC4, *(u8*)(p + ((idx++) & 0x1F) + 0x9A0));
    }
    func_800379C8(D_8006FCC8);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc3", func_800764CC);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc3", func_8007661C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc3", func_800767C8);

/* 89.36%: identical instructions except delay slots. The original leaves a nop
   after the "bne flags & 0x60000000" test where gcc/as always fills the slot
   with the lbu from the target block (one instruction, 4 bytes). Tried: early
   returns, one big && chain (83.28%), nested if for the last test.
void func_800767C8(u8* p) {
    u32 flags;

    if (D_8009284C >= 0x201) {
        return;
    }
    if (p[0xC5] == 4) {
        return;
    }
    flags = *(u32*)(p + 0xD0);
    if (flags & 0x1000000) {
        return;
    }
    if (!(flags & 0x40000)) {
        return;
    }
    if (!(*(u32*)(*(u8**)(p + 0xD8) + 0xD0) & 0x40000)) {
        return;
    }
    if ((flags & 0x60000000) != 0x20000000 || (p[0x90A] & 2)) {
        func_80076424(p);
        func_8007639C(p, 5);
    }
}
*/

INCLUDE_ASM("asm/battling/nonmatchings/main/misc3", func_80076884);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc3", func_80077038);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc3", func_80077584);

/* 97.62%: the only difference is a nop in the original's beqz delay slot before
   the "p->unk14 = -0x82" clamp, where gcc always fills the slot with the addiu.
   Tried: plain if, empty else branch, pointer local for the field, s32 local
   holding the limit.
void func_80077584(u8* p, s32 ang, s32 shift, s32 dec) {
    *(s32*)(p + 0x20) += rcos(ang) >> shift;
    *(s32*)(p + 0x28) += rsin(ang) >> shift;
    if (*(s32*)(p + 0x14) >= -0x8B) {
        *(s32*)(p + 0x14) -= dec;
    }
    if (*(s32*)(p + 0x14) < -0x82) {
        *(s32*)(p + 0x14) = -0x82;
    }
}
*/

void func_8007762C(u8* p) {
    *(u32*)(p + 0xD0) |= 0x1000;
    p[0x4C] = 8;
    p[0x4E] = 0xFF;
    *(u16*)(p + 0xCA) = 0x3C;
    *(u16*)(p + 0x916) = 0x28;
    *(u32*)(p + 0xE8) = 0x28;
    p[0xC4] = 4;
    p[0xC5] = 0;
    *(u32*)(p + 0xD4) &= ~0x10;
    *(u32*)(p + 0xD0) &= ~0x400;
    func_80077584(p);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc3", func_800776A8);

/* 88.00%: gcc emits the mask constant before the flags load, so reorg steals
   the lui into the beq delay slot and maspsx must add a load-delay nop; the
   original has the load first and an unfilled slot (same shape as func_800767C8).
void func_800776A8(u8* p, s32 mode) {
    u32 flags;
    s32 kind;

    if (func_80083CD8(p) == 4) {
        return;
    }
    flags = *(u32*)(p + 0xD0);
    if (flags & 0x8000000) {
        if (D_800928C8 != 2 && D_800928C8 != 4) {
            return;
        }
        if (!(D_80099D9C & 1)) {
            return;
        }
        if (flags & 0x40) {
            return;
        }
        kind = 1;
    } else {
        if (!(D_80099D9B & 1)) {
            return;
        }
        if (flags & 0x40) {
            return;
        }
        kind = 0;
    }
    func_80036258(kind, mode);
}
*/

INCLUDE_ASM("asm/battling/nonmatchings/main/misc3", func_80077770);

u8* func_80077A38(u8* p, s32 flag) {
    s32 idx;

    idx = p[0x9C3] * 2;
    if (flag != 0) {
        p[0x9C3] = D_80091178[idx];
    } else {
        p[0x9C3] = D_80091179[idx];
    }
    return (u8*)(*(u32*)(p + 0x80) + p[0x9C3] * 4);
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
