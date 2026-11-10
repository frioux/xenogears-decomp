#include "common.h"
#include "psyq/libgpu.h"

extern u8 D_80092954[];

extern void func_80083C0C(int mode);
extern void func_800346D4(void* pString);

void func_800707A8(void) {
    func_80083C0C(3);
    func_800346D4(&D_80092954[0]);
}

s32 func_800707D8(s32 a0, s32 a1, s32 a2) {
    s32 d;

    d = a0 - a1;
    if (d < 0) {
        d++;
        d -= a2;
    } else {
        d--;
        d += a2;
    }
    return d / a2;
}

extern s32 D_8009867C;
extern s32 D_80098680;
extern s32 D_80098684;
extern s32 D_800925F4;
extern s32 D_800925F8;
extern s32 D_800925FC;
extern s32 D_80097058;
extern s32 D_800970E0;
extern s32 D_80098774;
extern s32 D_800987FC;

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_80070808);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_800708C4);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_8007099C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_80070C7C);

void func_80070F80(s32 value) {
    D_800925F8 = value;
    D_80098774 = 0;
    D_80097058 = 0;
    D_800925FC = 0;
    D_800987FC &= 0xFFFF7FFF;
    D_800970E0 &= 0xFFFF7FFF;
}

extern s32 D_80092934;

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_80070FD8);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_8007107C);

extern s32 D_800925E0;
extern s32 D_800925E4;
extern void* D_80092868;
extern u8 D_800928A0;
extern u8 D_800929E4[];

void func_80071724(void* pOt) {
    u8* p;

    p = D_80092868;
    *(s32*)(p + 0x7C) = D_800925E0 | (D_800925E4 << 16);
    AddPrim(pOt, p + 0x74);
    AddPrim(pOt, &D_800929E4[D_800928A0 * 8]);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_80071794);

extern s32 D_800925D4;
extern s32 D_800925D8;
extern u8 D_8009868C[];
extern s32 func_80032F54(void* pString, s16 a, s16 b, s16 c, s16 d, s16 e, s16 f);

void func_800718C0(void) {
    D_800925D8 = -1;
    D_800925D4 = 0;
    func_80032F54(&D_8009868C[0], 0x140, 0x30, 0x1C, 0x9A, 0x40, 4);
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_8007191C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_800719F0);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_80071AD0);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_80071DA4);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_80071F8C);

extern u8 D_8009293C;

void func_800720C4(void) {
    D_8009293C = 0;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_800720D4);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_80072170);

void func_800725A8(void) {
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_800725B0);

extern void func_8008BC04(void);
extern void func_8007F834(void);
extern void func_80030988(s32 a0, s32 a1, s32 a2, s32 a3);
extern void func_8008A5BC(void* p);
extern void func_80089D5C(void* p);
extern void func_8008976C(s32 a0, s32 a1);
extern void func_80083C0C(s32 mode);
extern void func_8007E954(s32 value);
extern void func_80080D10(void);
extern void* D_800910F0;
extern void* D_80092610;

void func_800726B4(void) {
    func_8008BC04();
    func_8007F834();
    func_80030988(1, 1, 0x40, 0x40);
    func_8008A5BC(D_800910F0);
    func_80089D5C(D_80092610);
    func_8008976C(0x140, 0xDA);
    SetGeomScreen(0xC0);
    func_80083C0C(3);
    func_8007E954(0x100);
    func_80080D10();
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_8007273C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_80072858);
