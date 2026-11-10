#include "common.h"
#include "psyq/libgte.h"
#include "psyq/gtemac.h"
#include "psyq/inline_c.h"

extern u8 D_8006F978[];
extern u8 *D_800927CC;
extern u8 *D_800927D0;
extern u8 D_80095968[];

extern void *func_8008820C();
extern void func_80085EC8(void *);

typedef struct {
    /* 0x000 */ u32 unk0[0xC4];
} BattlingWork; // Size: 0x310

extern BattlingWork D_8009A2F8[2];

extern s32 D_800928F8;
extern u32 D_80059598;
extern u8 D_80091934[];
extern u8 *D_80092854;
extern u8 *D_80092858;
extern long MathGetLargestVectorComponent(long, long, long);
extern void func_800732AC(void *, void *, s32);
extern void func_80072D18(s32, s32, s32);

typedef struct {
    /* 0x00 */ s16 unk0;
    /* 0x02 */ s16 unk2;
    /* 0x04 */ s16 unk4;
    /* 0x06 */ u8 pad6[0x62];
} BattlingEntry68; // Size: 0x68

extern BattlingEntry68 D_8009A988[];

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_80087068);

void func_800875EC(void) {
    func_80085EC8(D_8009A2F8);
    D_8009A2F8[1] = D_8009A2F8[0];
}

void func_80087650(void) {
    s32 i;

    for (i = 0; i < 0x80; i++) {
        D_800927CC[i] = 0;
        D_800927D0[i] = 0xFF;
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_80087698);

void func_8008779C(s32 arg0, s32 arg1, s32 arg2) {
    D_80059598 = (D_80059598 & 0xFFFFFF) | 0x24000000;
    gte_ldrgb(&D_80059598);
    func_800732AC((void *)0x1F800120, D_80091934, 0x30);
    func_80072D18(arg0, arg1, arg2);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_80087830);
/* Best 80.95%: the original CSEs &D_80092854 for the store and the following load
   and keeps it in s0 (frame 0x18); gcc recomputes the address with $at each time
   (frame 0x10, 4 bytes short). Tried: p = g + 7, p = g then p += 7 (80.95),
   p initialized at declaration (74.72), explicit u8 **pBuf = &D_80092854.
void func_80087830(void) {
    s32 i;
    u8 *p;

    D_800927CC = (u8 *)0x1F800000;
    D_800927D0 = (u8 *)0x1F800080;
    D_80092854 = HeapAlloc(0xE100, 0);
    D_80092858 = HeapAlloc(0xE100, 0);
    p = D_80092854 + 7;
    for (i = 0; i < 0x708; i++) {
        p[-4] = 7;
        p[0] = 0x24;
        p += 0x20;
    }
    func_800732AC(D_80092858, D_80092854, 0xE100);
}
*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_800878DC);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_80087AB0);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_80087B74);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_80087E38);
/* Best 98.08%: the only difference is which register gets the sum. The original
   keeps the base in a0 and the result in v0 (addu v0,v0,a0); gcc reuses the dead
   base register (addu a0,v0,a0) and the i+1 temp then moves to v0. Tried: p += i,
   p = p + i, p = &p[i], p = base + i, u8* + i*0x68, n = i + 1, decls outside the
   if (all 98.08 or worse), &D_8009A988[i] and i + D_8009A988 (84.04).
void func_80087E38(s32 *arg0) {
    s32 i = D_800928F8;

    if (i < 0x1F) {
        BattlingEntry68 *p = D_8009A988;

        p += i;
        p->unk0 = arg0[0];
        p->unk2 = arg0[1];
        p->unk4 = arg0[2];
        D_800928F8 = i + 1;
    }
}
*/
INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_80087EA0);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_8008820C);

void func_800882D4(s32 arg0, s32 *arg1, s32 arg2, s16 arg3) {
    u8 *p = func_8008820C(arg0, arg1, arg2);

    if (p != NULL) {
        *(s16 *)(p + 0x30) = arg3;
    }
}

void func_80088308(void) {
    s32 i;

    for (i = 0x141C; i >= 0; i -= 0x34) {
        *(s16 *)(D_80095968 + i) = 0;
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_8008832C);

void func_800884E0(VECTOR *arg0, VECTOR *arg1) {
    VECTOR normal;
    s32 sqrt;
    s32 largestComponent;

    normal = *arg0;
    largestComponent = MathGetLargestVectorComponent(normal.vx, normal.vy, normal.vz);
    if (largestComponent < 0) {
        largestComponent = -largestComponent;
    }
    sqrt = SquareRoot0(largestComponent);
    normal.vx = normal.vx / sqrt;
    normal.vy = normal.vy / sqrt;
    normal.vz = normal.vz / sqrt;
    VectorNormal(&normal, arg1);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc11", func_8008859C);

void func_80088658(SVECTOR *arg0, SVECTOR *arg1) {
    VECTOR normal;
    s32 sqrt;
    s32 largestComponent;

    normal.vx = arg0->vx;
    normal.vy = arg0->vy;
    normal.vz = arg0->vz;
    largestComponent = MathGetLargestVectorComponent(normal.vx, normal.vy, normal.vz);
    if (largestComponent < 0) {
        largestComponent = -largestComponent;
    }
    sqrt = SquareRoot0(largestComponent);
    normal.vx = normal.vx / sqrt;
    normal.vy = normal.vy / sqrt;
    normal.vz = normal.vz / sqrt;
    VectorNormalS(&normal, arg1);
}

s32 func_800886FC(s32 *arg0) {
    long out[3];

    gte_ldlvl(arg0);
    gte_nop();
    gte_nop();
    gte_sqr0_b();
    gte_stlvnl(out);
    return SquareRoot0(out[0] + out[1] + out[2]);
}

s32 func_80088754(s32 *arg0) {
    long out[3];

    gte_ldlvl(arg0);
    gte_nop();
    gte_nop();
    gte_sqr0_b();
    gte_stlvnl(out);
    return SquareRoot0(out[0] + out[2]);
}

s32 func_800887A4(s32 *arg0, s32 *arg1) {
    long out[3];

    out[0] = arg1[0] - arg0[0];
    out[1] = arg1[1] - arg0[1];
    out[2] = arg1[2] - arg0[2];
    gte_ldlvl(out);
    gte_nop();
    gte_nop();
    gte_sqr0_b();
    gte_stlvnl(out);
    return SquareRoot0(out[0] + out[1] + out[2]);
}

s32 func_80088838(s32 *arg0, s32 *arg1) {
    long out[3];

    out[0] = arg1[0] - arg0[0];
    out[2] = arg1[2] - arg0[2];
    gte_ldlvl(out);
    gte_nop();
    gte_nop();
    gte_sqr0_b();
    gte_stlvnl(out);
    return SquareRoot0(out[0] + out[2]);
}

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
