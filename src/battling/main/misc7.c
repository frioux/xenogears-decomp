#include "common.h"

extern u8 D_80099D98;
extern u8 D_80099D9A;
extern u8 D_80099DA1;
extern u8 D_80099D9B;
extern u8 D_80099D9C;
extern u8 D_80099D9D;
extern u8 D_80099D9E;
extern u8 D_80099D9F;
extern u8 D_80099DA2;
extern u32 D_800912F4[];
extern s32* D_80092734;
extern u8 D_80092884;
extern s32 D_800912F0;
extern s32 D_80098800;
extern s32 D_800970E4;
extern s32 D_80092950;

void func_8007EE08(s32 arg);
void func_8007F834(void);
void func_80080C48(s32 arg);
void func_80083C0C(s32 mode);
s32 func_8007FF70(s32 value, s32 max, s32 flags);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_8007F854);

/* 63% under the unit's preset, 96.08% under -G0: the original materialises
   &D_80098800 and &D_800970E4 into a1/a0 and does lw/ori/sw through them,
   which is gcc's -G0 addressing. This unit may want an XenoMain-style -G0
   preset (gears.toml, not touched here). */
/* void func_8007F854(void) {
    D_800912F0 = 1;
    func_80083C0C(1);
    D_80092734 = 0;
    func_8007F834();
    D_80098800 |= 0xC;
    D_800970E4 |= 0xC;
} */

void func_8007F8B4(void) {
    s32 tmp[2]; // reproduces the original's unused 8-byte stack slot

    func_80083C0C(1);
    D_80092734 = 0;
    func_8007F834();
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_8007F8E4);

/* 87%: gcc hoists the merged `li a0,3` into the bnez delay slot and folds the
   second test into a beq to the shared call; the original keeps `j; li a0,3`
   as a middle block. Also tried: mode local, `if (A == 0 && B != 1)`. */
/* void func_8007F8E4(void) {
    func_80080C48(0);
    if (D_80099D9F != 0) {
        func_80083C0C(3);
    } else if (D_80092950 == 1) {
        func_80083C0C(3);
    } else {
        D_80092950--;
        func_80083C0C(6);
    }
} */

typedef struct {
    /* 0x0 */ u8 unk0[0x12];
    /* 0x12 */ s16 unk12;
} Misc7Struct; // Size: 0x14

void func_8007F948(Misc7Struct* p, s32 arg) {
    if (p->unk12 == arg) {
        func_8007EE08(1);
    } else {
        func_8007EE08(0);
    }
}

u32 func_8007F97C(void) {
    return D_800912F4[D_80099D98];
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_8007F9A0);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_8007FB0C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_8007FBEC);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_8007FE48);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_8007FF70);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_80080054);

/* 55%: the original keeps &D_80099D98 in $s0 across the call and loads and
   stores through it; every C shape tried (*p = f(*p), a pointer local,
   a value local, the plain assignment) makes gcc rematerialise the
   address instead, and -G0 does not change it. */
/* void func_80080054(void) {
    D_80099D98 = func_8007FF70(D_80099D98, 2, 0);
} */

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_80080090);

/* 55%: the original keeps &D_80099DA1 in $s0 across the call and loads and
   stores through it; every C shape tried (*p = f(*p), a pointer local,
   a value local, the plain assignment) makes gcc rematerialise the
   address instead. */
/* void func_80080090(void) {
    D_80099DA1 = func_8007FF70(D_80099DA1, 7, 2);
} */

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_800800CC);

/* 55%: the original keeps &D_80099D9A in $s0 across the call and loads and
   stores through it; every C shape tried (*p = f(*p), a pointer local,
   a value local, the plain assignment) makes gcc rematerialise the
   address instead. */
/* void func_800800CC(void) {
    D_80099D9A = func_8007FF70(D_80099D9A, 4, 2);
} */

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_80080108);

/* 55%: the original keeps &D_80099D9B in $s0 across the call and loads and
   stores through it; every C shape tried (*p = f(*p), a pointer local,
   a value local, the plain assignment) makes gcc rematerialise the
   address instead. */
/* void func_80080108(void) {
    D_80099D9B = func_8007FF70(D_80099D9B, 1, 1);
} */

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_80080144);

/* 55%: the original keeps &D_80099D9C in $s0 across the call and loads and
   stores through it; every C shape tried (*p = f(*p), a pointer local,
   a value local, the plain assignment) makes gcc rematerialise the
   address instead. */
/* void func_80080144(void) {
    D_80099D9C = func_8007FF70(D_80099D9C, 1, 1);
} */

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_80080180);

/* 55%: the original keeps &D_80099D9D in $s0 across the call and loads and
   stores through it; every C shape tried (*p = f(*p), a pointer local,
   a value local, the plain assignment) makes gcc rematerialise the
   address instead. */
/* void func_80080180(void) {
    D_80099D9D = func_8007FF70(D_80099D9D, 1, 1);
} */

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_800801BC);

/* 55%: the original keeps &D_80099D9E in $s0 across the call and loads and
   stores through it; every C shape tried (*p = f(*p), a pointer local,
   a value local, the plain assignment) makes gcc rematerialise the
   address instead. */
/* void func_800801BC(void) {
    D_80099D9E = func_8007FF70(D_80099D9E, 1, 1);
} */

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_800801F8);

/* 55%: the original keeps &D_80099D9F in $s0 across the call and loads and
   stores through it; every C shape tried (*p = f(*p), a pointer local,
   a value local, the plain assignment) makes gcc rematerialise the
   address instead. */
/* void func_800801F8(void) {
    D_80099D9F = func_8007FF70(D_80099D9F, 3, 2);
} */

s32 func_80080234(void) {
    s32 value = func_8007FF70(D_80092884, 1, 1);

    D_80092884 = value;
    return value;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_80080268);

/* 55%: the original keeps &D_80099DA2 in $s0 across the call and loads and
   stores through it; every C shape tried (*p = f(*p), a pointer local,
   a value local, the plain assignment) makes gcc rematerialise the
   address instead. */
/* void func_80080268(void) {
    D_80099DA2 = func_8007FF70(D_80099DA2, 0xD, 3);
} */

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_800802A4);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_8008040C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_80080570);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_80080644);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_80080780);
