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

void func_8007EE08(s32 arg);
void func_8007F834(void);
void func_80083C0C(s32 mode);
s32 func_8007FF70(s32 value, s32 max, s32 flags);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_8007F854);

void func_8007F8B4(void) {
    s32 tmp[2];

    func_80083C0C(1);
    D_80092734 = 0;
    func_8007F834();
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_8007F8E4);

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

void func_80080054(void) {
    u8* pValue = &D_80099D98;

    *pValue = func_8007FF70(*pValue, 2, 0);
}

void func_80080090(void) {
    u8* pValue = &D_80099DA1;

    *pValue = func_8007FF70(*pValue, 7, 2);
}

void func_800800CC(void) {
    u8* pValue = &D_80099D9A;

    *pValue = func_8007FF70(*pValue, 4, 2);
}

void func_80080108(void) {
    u8* pValue = &D_80099D9B;

    *pValue = func_8007FF70(*pValue, 1, 1);
}

void func_80080144(void) {
    u8* pValue = &D_80099D9C;

    *pValue = func_8007FF70(*pValue, 1, 1);
}

void func_80080180(void) {
    u8* pValue = &D_80099D9D;

    *pValue = func_8007FF70(*pValue, 1, 1);
}

void func_800801BC(void) {
    u8* pValue = &D_80099D9E;

    *pValue = func_8007FF70(*pValue, 1, 1);
}

void func_800801F8(void) {
    u8* pValue = &D_80099D9F;

    *pValue = func_8007FF70(*pValue, 3, 2);
}

s32 func_80080234(void) {
    s32 value = func_8007FF70(D_80092884, 1, 1);

    D_80092884 = value;
    return value;
}

void func_80080268(void) {
    u8* pValue = &D_80099DA2;

    *pValue = func_8007FF70(*pValue, 0xD, 3);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_800802A4);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_8008040C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_80080570);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_80080644);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc7", func_80080780);
