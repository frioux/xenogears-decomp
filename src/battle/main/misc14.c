#include "common.h"
#include "psyq/libgpu.h"
#include "libgear/main.h"

extern u16 D_800C3468[];
extern u16 D_800C3448[];
extern u8 D_800C3E4C;
extern u8 D_800CCDC4[];
extern u8 D_800CCDC6[];
extern u8 D_800C3CF9[];
extern s32 D_800C3A9C;
extern u8 D_800C3A94;
extern s32 D_800C3A8C;
extern u8 D_800C3A98;
extern s32 D_800C3A90;
extern s32 D_800C2080;
extern s32 D_800C2084;
extern s32 D_800C3A7C;
extern s32 D_800C3A84;
extern s32 D_800C3A80;
extern s32 D_800C3A88;
extern u8 D_800C207C;
extern u8 D_800D2C36;
extern u8 D_800C3CFA[];
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

void func_80088990(void) {
    s32 i = 0;

    for (i = 0; i < D_800C3A9C; i++) {
        if (D_800C3A94 != 0) {
            D_800C2080 -= D_800C3A8C;
        } else {
            D_800C2080 += D_800C3A8C;
        }
        if (D_800C3A98 != 0) {
            D_800C2084 -= D_800C3A90;
        } else {
            D_800C2084 += D_800C3A90;
        }
    }
    if (D_800C3A8C == 0x100) {
        if (D_800C3A94 != 0) {
            if (D_800C2080 / 0x100 + D_800C3A7C < D_800C3A84) {
                D_800C207C = 1;
            }
        } else if (D_800C3A84 < D_800C2080 / 0x100 + D_800C3A7C) {
            D_800C207C = 1;
        }
    } else if (D_800C3A98 != 0) {
        if (D_800C2084 / 0x100 + D_800C3A80 < D_800C3A88) {
            D_800C207C = 1;
        }
    } else if (D_800C3A88 < D_800C2084 / 0x100 + D_800C3A80) {
        D_800C207C = 1;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_80088B80);
void func_80089038(void) {
    s32 i = 0;
    s32 pitch = 0x6E << 16;
    s32 off;

    D_800D2DB4[0x5DA1] = 0;
    for (i = 0; i < 5; i++) {
        if ((u16)func_80089C6C(D_800D2C30, i) != 0) {
            off = D_800D2DB4[0x5DA1] * 0x50 + 0x4CE0;
            D_800D2DB4[0x5DA1] += func_80076A10(i + 0xC4, (s32)(off + (s32)D_800D2DB4), 0xE0, pitch >> 16);
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_800891E4);
// Best attempt: objdiff 95.11%, same addu operand order / register-allocation issue in the two tail loops
/*
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
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_80089348);
// Best attempt: objdiff 96.84%, 292 bytes (the original size). The boost loop matches
// exactly with the "off" local + "(s32)(off + (s32)D_800D2DB4)" (index-first addu, and
// the +0x4E70 stays on the offset chain). The tail loop is one instruction off: the
// original keeps the base in $a1 and folds the constant into the offset chain
// (sll a0,a0,3 / addiu a0,a0,0x4E70 / addu a0,a0,a1), gcc folds it onto the base
// instead (addiu v0,a1,0x4E70 / addu a0,a0,v0). Inline form for the tail loop (no
// "off" local) is the same 96.84%; the "off"-local form for the tail loop is 94.23%
// (base lands in $a0 and the offset in $v0).
/*
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
        func_80076B68((u8*)((i * 2 + D_800D2DB4[0x5D87]) * 0x28 + 0x4E70 + (s32)D_800D2DB4));
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_8008946C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_8008963C);
// Best attempt: objdiff 89.89%. Tail loop chain matches with "&D_800D2DB4[...]" but the
// final addu comes out base-first (addu a0,a1,a0) where the original is offset-first;
// the two func_80076A10 call sites put the base in $a1 and the offset chain in $v0
// (original: base $v1, chain $a1), and reorg fills the beq delay slot with the arg0
// addiu instead of the loop increment. The "n" counter is real (the tail call's a2 is
// (s16)(0x11A + n*6), NOT pitch>>16).
/*
void func_8008963C(void) {
    s32 i = 0;
    s32 n = 0;
    s32 pitch = 0x11A << 16;
    s32 off;

    func_8008AAA0(D_800D2C36);
    for (i = 0; i < 3; i++) {
        if (D_800C3CFA[i] != 0xFF) {
            off = D_800D2DB4[0x5D7A] * 0x50 + 0x50A0;
            D_800D2DB4[0x5D7A] += func_80076A10(D_800C3CFA[i] + 0x92, (s32)&D_800D2DB4[off], pitch >> 16, 0x56);
            pitch += 0x60000;
            n++;
        }
    }
    off = D_800D2DB4[0x5D7A] * 0x50 + 0x50A0;
    D_800D2DB4[0x5D7A] += func_80076A10(0x9D, (s32)&D_800D2DB4[off], 0x11A + n * 6, 0x56);
    D_800D2DB4[0x5D89] = D_800CCB34;
    for (i = 0; i < D_800D2DB4[0x5D7A]; i++) {
        func_80076B68(&D_800D2DB4[(i * 2 + D_800D2DB4[0x5D89]) * 0x28 + 0x50A0]);
    }
}
*/

INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_800897CC);
// Best attempt: objdiff 96.84%, 292 bytes; the twin of func_80089348 (same one-instruction
// tail-loop difference: original "addiu a0,a0,0x51E0 / addu a0,a0,a1", gcc
// "addiu v0,a1,0x51E0 / addu a0,a0,v0"). Inline index-first form for both loops: 87.45%.
/*
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
        func_80076B68((u8*)((i * 2 + D_800D2DB4[0x5D8A]) * 0x28 + 0x51E0 + (s32)D_800D2DB4));
    }
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_800898F0);
// Best attempt: objdiff 91.38% (528 bytes, the original size). Only two classes of diff,
// both shared with the rest of the func_80076A10 family in this unit: every call site puts
// the D_800D2DB4 base in $a1 and the offset chain in $v0 where the original has base $v1 /
// chain $a1 (the final addu is the mirror image), and the second loop's "pitch = 0x48 << 16"
// plus its 0x80000 increment are scheduled a few instructions differently. Pitch values are
// 0x20<<16 / 0x48<<16 with a +0x80000 step, the step is OUTSIDE the if, and the table
// lookup index is arg0 * 0x170 into a u8 table at D_800CCDC4 / D_800CCDC6.
/*
void func_800898F0(u8 arg0) {
    s32 i = 0;
    s32 pitch = 0x20 << 16;
    s32 off;

    D_800D2DB4[0x5D7C] = 0;
    func_8008AAA0(*(u16*)(D_800CCDC4 + arg0 * 0x170));
    for (i = 0; i < 4; i++) {
        if (D_800C3CF9[i] != 0xFF) {
            off = D_800D2DB4[0x5D7C] * 0x50 + 0x5280;
            D_800D2DB4[0x5D7C] += func_80076A10(D_800C3CF9[i] + 0x92, (s32)(off + (s32)D_800D2DB4), pitch >> 16, 0xCC);
        }
        pitch += 0x80000;
    }
    D_800D2DB4[0x5D8B] = D_800CCB34;
    D_800D2DB4[0x5D7D] = 0;
    func_8008AAA0(*(u16*)(D_800CCDC6 + arg0 * 0x170));
    pitch = 0x48 << 16;
    for (i = 0; i < 4; i++) {
        if (D_800C3CF9[i] != 0xFF) {
            off = D_800D2DB4[0x5D7D] * 0x50 + 0x53C0;
            D_800D2DB4[0x5D7D] += func_80076A10(D_800C3CF9[i] + 0x92, (s32)(off + (s32)D_800D2DB4), pitch >> 16, 0xCC);
        }
        pitch += 0x80000;
    }
    off = D_800D2DB4[0x5D7D] * 0x50 + 0x53C0;
    D_800D2DB4[0x5D7D] += func_80076A10(0x9C, (s32)(off + (s32)D_800D2DB4), 0x41, 0xCC);
    D_800D2DB4[0x5D8C] = D_800CCB34;
}
*/
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
// Best attempt: objdiff 90.38%, 144 bytes (the original size). Everything up to the
// div matches exactly, including the folded "return 0" in the beqz delay slot; the only
// diff is that the original puts the remainder AND the sum in $v1
// (div zero,v0,v1 / mfhi v1 / addu v1,s1,v1 / andi v0,v1,0xffff) while gcc coalesces
// both into $v0 and then merges the sum's u16 conversion with the other branch's mask
// at the shared exit. Every spelling that masks the dividend ((u16)rand(),
// rand() & 0xFFFF, a u16 temp) instead makes gcc put the rand call in the beqz delay
// slot and duplicate it -> 85.38% / 156 bytes. Also tried: s32 rem and u16 res locals,
// swapping the range/rem declaration order, "return rand()" vs "return rand() & 0xFFFF".
/*
u16 func_80089B50(u16 arg0, u16 arg1) {
    s32 rem;
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
        return rand();
    }
    rem = rand() % (range + 1);
    return (u16)(arg0 + rem);
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_8008A144);
// Best attempt: objdiff 91.84%; single diff: the (field*4 + 0x8970) temp lands in $v1, original reuses $a1
// (coalesced with the field load). Everything else identical
/*
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
*/
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
