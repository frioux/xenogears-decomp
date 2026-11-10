#include "common.h"

/* The nine flag bytes below are declared as open arrays on purpose: -G8
   classifies a 1-byte extern as small data, which makes its address one
   gp-relative insn, so gcc rematerialises it at each use instead of holding
   it in a saved register across the call the way the original does. */
extern u8 D_80099D98[];
extern u8 D_80099D9A[];
extern u8 D_80099DA1[];
extern u8 D_80099D9B[];
extern u8 D_80099D9C[];
extern u8 D_80099D9D[];
extern u8 D_80099D9E[];
extern u8 D_80099D9F[];
extern u8 D_80099DA2[];
extern u32 D_800912F4[];
extern s32* D_80092734;
extern u8 D_80092884;
extern s32 D_800912F0;
extern s32 D_80098800[];
extern s32 D_800970E4[];
extern s32 D_80092950;

void func_8007EE08(s32 arg);
void func_8007F834(void);
void func_80080C48(s32 arg);
void func_80083C0C(s32 mode);
s32 func_8007FF70(s32 value, s32 max, s32 flags);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_8007F854);

/* 96.25%: everything matches except the currency allocation, which is mirrored
   - the original loads/stores D_80098800 side through a1/v1 and D_800970E4
   through a0/v0, ours picks the pair the other way round, so both value and
   address registers are swapped (and the two lw/ori/sw groups with them).
   Also tried: statement order reversed (95.42), no tmp array (96.08).
   The D_80098800[]/D_800970E4[] open-array declarations below are what got it
   this far: a scalar s32 extern is small data under -G8, and then gcc
   rematerialises each address instead of holding it in a register. */
/* void func_8007F854(void) {
    s32 tmp[2];

    D_800912F0 = 1;
    func_80083C0C(1);
    D_80092734 = 0;
    func_8007F834();
    D_80098800[0] |= 0xC;
    D_800970E4[0] |= 0xC;
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
    return D_800912F4[D_80099D98[0]];
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_8007F9A0);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_8007FB0C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_8007FBEC);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_8007FE48);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_8007FF70);

void func_80080054(void) {
    D_80099D98[0] = func_8007FF70(D_80099D98[0], 2, 0);
}

void func_80080090(void) {
    D_80099DA1[0] = func_8007FF70(D_80099DA1[0], 7, 2);
}

void func_800800CC(void) {
    D_80099D9A[0] = func_8007FF70(D_80099D9A[0], 4, 2);
}

void func_80080108(void) {
    D_80099D9B[0] = func_8007FF70(D_80099D9B[0], 1, 1);
}

void func_80080144(void) {
    D_80099D9C[0] = func_8007FF70(D_80099D9C[0], 1, 1);
}

void func_80080180(void) {
    D_80099D9D[0] = func_8007FF70(D_80099D9D[0], 1, 1);
}

void func_800801BC(void) {
    D_80099D9E[0] = func_8007FF70(D_80099D9E[0], 1, 1);
}

void func_800801F8(void) {
    D_80099D9F[0] = func_8007FF70(D_80099D9F[0], 3, 2);
}

s32 func_80080234(void) {
    s32 value = func_8007FF70(D_80092884, 1, 1);

    D_80092884 = value;
    return value;
}

void func_80080268(void) {
    D_80099DA2[0] = func_8007FF70(D_80099DA2[0], 0xD, 3);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_800802A4);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_8008040C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_80080570);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_80080644);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_80080780);
