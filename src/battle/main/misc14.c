#include "common.h"
#include "psyq/libgpu.h"
#include "libgear/main.h"

extern u16 D_800C3468[];
extern u16 D_800C3448[];
extern u8 D_800C3E4C;
extern u8 D_800D2C38;
extern u16 D_800D2C30;
extern s32 func_80089C6C(s32 mask, u8 idx);
extern u8 D_800CCB34;
extern u8* D_800D2DB4;
extern s32 func_80076A10(s32 arg0, s32 arg1, s16 arg2, s16 arg3);
extern void func_80076B68(u8* p);
extern Temp8* D_8005919C;
extern void func_80039DB8(s32 arg0);
extern void func_8008860C(void);
extern void func_80089038(void);
extern void func_80089110(void);
extern void func_800891E4(void);
extern void func_80089348(void);
extern void func_8008946C(void);
extern void func_8008963C(void);
extern void func_800897CC(void);
extern void func_8008A3EC(u8 arg0);
extern void func_8008A274(u8 arg0);
extern void func_8008A684(u8 arg0);
extern int rand(void);
extern u8 *D_800C3EA4;
extern u8 *D_800D2D28;
extern u8 D_800C3CF8[];
extern u8 D_800C3CFB[];
extern u8 D_800CCC58;
extern u8 D_800D2C34;
extern u8 D_800D2C35;
extern u8 D_800D39D4;
extern s32 D_800D3288;
extern u16 D_800D2C2A;
extern void func_8007171C(void);
extern void func_80089CCC(s32 arg0);
extern void func_8008AAA0(s32 arg0);
extern void func_80076BF0(u8 *p);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_80088990);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_80088B80);
void func_80089038(void) {
    s32 i = 0;
    s32 pitch = 0x6E << 16;

    D_800D2DB4[0x5DA1] = 0;
    for (i = 0; i < 5; i++) {
        if ((u16)func_80089C6C(D_800D2C30, i) != 0) {
            D_800D2DB4[0x5DA1] += func_80076A10(i + 0xC4, (s32)(D_800D2DB4 + 0x4CE0 + D_800D2DB4[0x5DA1] * 0x50), 0xE0, pitch >> 16);
            pitch += 0xA0000;
        }
    }
    D_800D2DB4[0x5DA0] = D_800CCB34;
    *(u16*)(D_800D2DB4 + 0x5DA2) = 0;
}
void func_80089110(void) {
    s32 arg0;
    s32 pEntry;
    s32 i;

    arg0 = 0xA0;
    if (D_800D2C38 != 0) {
        arg0 = 0xA1;
    }
    D_800D2DB4[0x5D75] = func_80076A10(arg0, (s32)(D_800D2DB4 + 0x3AC0), 0xA0, 0x64);
    D_800D2DB4[0x5D84] = D_800CCB34;
    for (i = 0; i < D_800D2DB4[0x5D75]; i++) {
        pEntry = (i * 2 + D_800D2DB4[0x5D84]) * 0x28 + 0x3AC0;
        func_80076B68((u8*)(pEntry + (s32)D_800D2DB4));
    }
}
void func_800891E4(void) {
    u8 v;
    s32 i;
    s32 off;

    v = D_800D2C34;
    D_800D2DB4[0x5D77] = func_80076A10((u8)(v - 0x5D), (s32)(D_800D2DB4 + 0x3E80), 0xA0, 0x64);
    D_800D2DB4[0x5D86] = D_800CCB34;
    D_800D2DB4[0x5D7F] = func_80076A10((u8)(v - 0x25), (s32)(D_800D2DB4 + 0x43D0), 0xA0, 0x64);
    D_800D2DB4[0x5D8E] = D_800CCB34;
    for (i = 0; i < D_800D2DB4[0x5D77]; i++) {
        off = (i * 2 + D_800D2DB4[0x5D86]) * 0x28 + 0x3E80;
        func_80076B68((u8*)(off + (s32)D_800D2DB4));
    }
    for (i = 0; i < D_800D2DB4[0x5D7F]; i++) {
        off = (i * 2 + D_800D2DB4[0x5D8E]) * 0x28 + 0x43D0;
        func_80076BF0((u8*)(off + (s32)D_800D2DB4));
    }
}
void func_80089348(void) {
    s32 i = 0;
    s32 pitch = 0x11A << 16;
    s32 off;

    func_8008AAA0(D_800D2C2A);
    for (i = 0; i < 5; i++) {
        if (D_800C3CF8[i] != 0xFF) {
            off = D_800D2DB4[0x5D78] * 0x50 + 0x4E70;
            D_800D2DB4[0x5D78] += func_80076A10(D_800C3CF8[i] + 0x92, (s32)(off + (s32)D_800D2DB4), pitch >> 16, 0x46);
            pitch += 0x60000;
        }
    }
    D_800D2DB4[0x5D87] = D_800CCB34;
    for (i = 0; i < D_800D2DB4[0x5D78]; i++) {
        off = (i * 2 + D_800D2DB4[0x5D87]) * 0x28 + 0x4E70;
        func_80076B68((u8*)(off + (s32)D_800D2DB4));
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_8008946C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_8008963C);
void func_800897CC(void) {
    s32 i = 0;
    s32 pitch = 0x11A << 16;
    s32 off;

    func_8008AAA0(D_800D2C35);
    for (i = 0; i < 2; i++) {
        if (D_800C3CFB[i] != 0xFF) {
            off = D_800D2DB4[0x5D7B] * 0x50 + 0x51E0;
            D_800D2DB4[0x5D7B] += func_80076A10(D_800C3CFB[i] + 0x92, (s32)(off + (s32)D_800D2DB4), pitch >> 16, 0x5E);
            pitch += 0x60000;
        }
    }
    D_800D2DB4[0x5D8A] = D_800CCB34;
    for (i = 0; i < D_800D2DB4[0x5D7B]; i++) {
        off = (i * 2 + D_800D2DB4[0x5D8A]) * 0x28 + 0x51E0;
        func_80076B68((u8*)(off + (s32)D_800D2DB4));
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_800898F0);
void func_80089AF8(void) {
    func_8008860C();
    func_80089038();
    func_80089110();
    func_800891E4();
    func_80089348();
    func_8008946C();
    func_8008963C();
    func_800897CC();
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_80089B50);
// Best attempt: objdiff 90.38%, 144 bytes (the original size). The original masks
// rand()'s result to u16 right after each call; every spelling that makes that mask
// appear (u16 temp or cast) also re-lays out the arg1 == 0 test and grows the
// function to 156 bytes (85.38%), so it is parked here.
// Also tried: "return rand();" in the >0xFFFE branch with
// "return arg0 + (u16)rand() % (range + 1)" -> 85.38% / 156 bytes, because gcc
// masks arg1 into $v0 instead of $v1 and so cannot fold "return 0" into the beqz
// delay slot (it emits bnez/nop/j instead). Moving "range = arg1 - arg0" above the
// arg0 == arg1 test -> 83.59% / 136 bytes.
/*
u16 func_80089B50(u16 arg0, u16 arg1) {
    s32 range;

    if (arg0 == 0xFFFF) {
        return 0xFFFF;
    }
    if (arg1 == 0) {
        return 0;
    }
    if (arg0 == arg1) {
        return arg0;
    }
    range = arg1 - arg0;
    if (range > 0xFFFE) {
        return rand() & 0xFFFF;
    }
    return (u16)(arg0 + rand() % (range + 1));
}
*/
u16 func_80089BEC(u8 idx) {
    return D_800C3468[idx];
}
u16 func_80089C08(u8 idx) {
    return D_800C3448[idx];
}
u16 func_80089C24(u8 idx) {
    return ~D_800C3468[idx];
}
u16 func_80089C48(u8 idx) {
    return ~D_800C3448[idx];
}
s32 func_80089C6C(s32 mask, u8 idx) {
    s32 flags;

    if (idx < 0x10) {
        flags = D_800C3468[idx] & mask;
    } else {
        flags = 0;
    }
    return flags;
}
s32 func_80089C9C(s32 mask, u8 idx) {
    s32 flags;

    if (idx < 0x10) {
        flags = D_800C3448[idx] & mask;
    } else {
        flags = 0;
    }
    return flags;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_80089CCC);
void func_8008A144(void) {
    D_800D2D28[0xAA] += 1;
    if ((D_800D2D28[0xAA] & 1) == 0) {
        return;
    }
    LoadImage((RECT*)(D_800C3EA4 + 0x8950), (u_long*)(*(u32*)(D_800D2D28 + 0x30) * 4 + 0x8970 + (s32)D_800C3EA4));
    LoadImage((RECT*)(D_800C3EA4 + 0x8958), (u_long*)(*(u32*)(D_800D2D28 + 0x30) * 4 + 0x8FA0 + (s32)D_800C3EA4));
    LoadImage((RECT*)(D_800C3EA4 + 0x8960), (u_long*)(*(u32*)(D_800D2D28 + 0x30) * 4 + 0x95D0 + (s32)D_800C3EA4));
    LoadImage((RECT*)(D_800C3EA4 + 0x8968), (u_long*)(*(u32*)(D_800D2D28 + 0x30) * 4 + 0x9C00 + (s32)D_800C3EA4));
    *(u32*)(D_800D2D28 + 0x30) += 4;
    if (*(u32*)(D_800D2D28 + 0x30) >= 0xC7) {
        *(u32*)(D_800D2D28 + 0x30) = 0;
    }
}
void func_8008A274(u8 arg0) {
    if (D_800CCC58 != 0) {
        func_8007171C();
    }
    if (arg0 == 0) {
        func_80089CCC(0);
    }
    D_800D2D28[0xA9] += 6;
    D_800D2D28[0xAB] += 1;
    if (D_800C3EA4[0x6415] != 0) {
        if (D_800C3EA4[0x6416] == 0) {
            *(s32*)(D_800C3EA4 + 0x6410) += 4;
            if (*(s32*)(D_800C3EA4 + 0x6410) >= 0x81) {
                *(s32*)(D_800C3EA4 + 0x6410) = 0x7C;
                D_800C3EA4[0x6416] = 1;
            }
        } else {
            *(s32*)(D_800C3EA4 + 0x6410) -= 4;
            if (*(s32*)(D_800C3EA4 + 0x6410) < 0) {
                *(s32*)(D_800C3EA4 + 0x6410) = 4;
                D_800C3EA4[0x6416] = 0;
            }
        }
    }
    switch (D_800D39D4) {
    case 1:
    case 3:
        D_800D3288 += 1;
        break;
    case 2:
    case 4:
        D_800D3288 -= 1;
        break;
    }
    func_8008A144();
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_8008A3EC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_8008A684);
void func_8008A9C0(u8 arg0) {
    switch (D_800C3E4C) {
    case 0:
        func_8008A684(arg0);
        break;
    case 1:
        func_8008A274(arg0);
        break;
    case 2:
        func_8008A3EC(arg0);
        break;
    }
}
void func_8008AA40(u8 arg0) {
    func_80039DB8((D_8005919C->unk14 << 16) | arg0);
}
