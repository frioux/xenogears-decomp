#include "common.h"

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

void func_80070808(s32* p, s32 n) {
    s32 d;

    d = func_800707D8(p[0], D_8009867C, n);
    D_8009867C += d;
    d = func_800707D8(p[2], D_80098684, n);
    D_80098684 += d;
    d = func_800707D8(p[1], D_80098680 + D_800925F4, n);
    D_80098680 += d;
}

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

extern int ratan2(int y, int x);
extern s32 D_80092934;

s32 func_80070FD8(u8* p) {
    s32 v[4];
    s32 angle;

    v[0] = *(s32*)(p + 0x0);
    v[1] = *(s32*)(p + 0x4);
    v[2] = *(s32*)(p + 0x8);
    v[3] = *(s32*)(p + 0xC);
    v[0] -= 0x3F80;
    v[2] -= 0x3F80;
    angle = ratan2(v[0], v[2]) & 0xFFF;
    if (angle < 0x201) {
        angle = 0x800;
    } else {
        angle = 0xC00;
    }
    *(s32*)(p + 0x58) = angle - D_80092934;
    *(s32*)(p + 0x48) = 0xFF;
    *(s16*)(p + 0xCE) = 0;
    return 0;
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_8007107C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_80071724);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_80071794);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_800718C0);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_8007191C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_800719F0);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_80071AD0);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_80071DA4);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_80071F8C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_800720C4);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_800720D4);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_80072170);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_800725A8);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_800725B0);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_800726B4);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_8007273C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc1", func_80072858);
