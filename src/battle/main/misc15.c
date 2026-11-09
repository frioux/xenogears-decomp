#include "common.h"

extern u8 *D_800C3EAC;
extern u8 D_800C3CF4[];
extern u8 *D_800D2D28;
extern u8 D_800D366C;
extern int ArchiveDataSync(void);
extern void func_8008AA40(u8 arg0);
extern void func_800716D8(void);
extern void func_8007765C(void);
extern void func_80077980(void);
extern void func_8008FA60(s32 arg0);
extern u16 func_80089C08(u8 arg0);
extern void func_800BC404(u16 arg0);
extern void func_800BCD98(u16 arg0);

void func_8008AA74(u8 arg0) {
    if (D_800D366C != 0) {
        func_8008AA40(arg0);
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008AAA0);
/* Best: 66.02%. Digit loop is right; only diff is where gcc emits the
   `srl` half of `divisor /= 10` (it sinks it to the block end, the original
   keeps it before the sb) plus the register naming that follows from it. The
   digit-storing loop below it matches exactly.
void func_8008AAA0(u32 arg0) {
    u32 divisor;
    u32 digit;
    s32 i;
    u8 *p;

    divisor = 100000000;
    for (i = 0; i < 9; i++) {
        digit = arg0 / divisor;
        arg0 %= divisor;
        divisor /= 10;
        D_800C3CF4[i] = digit;
    }
    p = D_800C3CF4;
    for (i = 1; i < 9; i++, p++) {
        if (D_800C3CF4[i] == 0) {
            *p = 0xFF;
        } else {
            if (*p == 0) {
                *p = 0xFF;
            }
            break;
        }
    }
}
*/
void func_8008AB4C(void) {
    ArchiveSetIndex(0x20, 0);
}
void func_8008AB70(void) {
    ArchiveSetIndex(0x20, 2);
}
void func_8008AB94(void) {
    ArchiveSetIndex(0x20, 3);
}
s32 func_8008ABB8(s32 arg0, s32 arg1) {
    HeapChangeCurrentUser(2, 0);
    return (s32)HeapAlloc(arg0, arg1);
}
s32 func_8008AC00(s32 arg0) {
    HeapChangeCurrentUser(2, 0);
    return (s32)HeapAlloc((arg0 + 3) * 26, 0);
}
void func_8008AC50(void) {
    while (ArchiveDataSync() != 0) {
        func_800716D8();
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008AC88);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008ADD0);
void func_8008B108(u8 arg0) {
    D_800D2D28[0x9C] = D_800D2D28[0x9D] = D_800D2D28[0x9E] = 0;
    D_800D2D28[0xB0] = D_800D2D28[0xB1] = D_800D2D28[0xB2] = D_800D2D28[0xB3] = 0;
    D_800D2D28[0xB7] = 0;
    if (arg0 == 0) {
        D_800D2D28[0xCB] = 1;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008B168);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008B224);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008B478);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008B908);
void func_8008BC40(u8 arg0) {
    D_800D2D28[0x9C] = D_800D2D28[0x9D] = D_800D2D28[0x9E] = 0;
    D_800D2D28[0xB0] = D_800D2D28[0xB1] = 0;
    D_800D2D28[0xB7] = 0;
    if (arg0 == 0) {
        D_800D2D28[0xCB] = 1;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008BC98);
/* Best: 99.35%. One instruction differs: the last `D_800C3EAC + arg0*0x40`
   comes out `addu s1,s1,v0` where the original has `addu v0,v0,s1`; both
   operands die there, so it is a global.c tie-break, not a source shape.
   Tried: array subscript vs pointer arithmetic.
void func_8008BC98(u8 arg0) {
    u16 value;

    D_800D2D28[0x9C] = D_800D2D28[0x9D] = D_800D2D28[0x9E] = 1;
    D_800D2D28[0xB0] = D_800D2D28[0xB1] = 1;
    D_800D2D28[0xB7] = 2;
    value = func_80089C08(arg0) | func_80089C08(D_800C3EAC[arg0 * 0x40 + 0x3C]);
    func_800BC404(value);
    func_800BCD98(func_80089C08((D_800C3EAC + arg0 * 0x40)[0x3C]));
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008BD50);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008BED8);
void func_8008C360(u8 arg0) {
    D_800D2D28[0x9C] = D_800D2D28[0x9D] = D_800D2D28[0x9E] = 0;
    D_800D2D28[0xB7] = 0;
    if (arg0 != 0) {
        D_800D2D28[0xC6] = 0;
        func_8008FA60(0);
        func_8008FA60(1);
        func_800716D8();
        func_8007765C();
        func_80077980();
    } else {
        D_800D2D28[0xB0] = D_800D2D28[0xB1] = 0;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008C3F0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008C4A8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008C81C);
void func_8008CCCC(u8 arg0) {
    D_800D2D28[0x9C] = D_800D2D28[0x9D] = D_800D2D28[0x9E] = 0;
    D_800D2D28[0xB0] = D_800D2D28[0xB1] = D_800D2D28[0xB2] = 0;
    D_800D2D28[0xB7] = 0;
    if (arg0 == 0) {
        D_800D2D28[0xCB] = 1;
    }
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc15", func_8008CD28);
