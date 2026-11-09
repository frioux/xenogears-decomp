#include "common.h"

extern u8 *D_800C3EAC;
extern u8 *D_800D2D28;
extern u8 D_800C3EB4[];
extern u8 D_800D301C[];
extern u16 D_800D39DC;
extern u16 D_800C48E8;
extern u16 D_800D2C94;
extern u16 D_800D2C96;

extern void func_80085454(u8);
extern void func_80085618(u8);
extern void func_80098C6C(u16);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_80085618);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_80085AC4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_80085B58);
void func_80085C48(s32 arg0, u16 arg1, u16 arg2) {
    u16 v = D_800D39DC;

    D_800C48E8 = 0;
    D_800D2C94 = arg1;
    D_800D2C96 = v;
    func_80098C6C(arg2);
}
void func_80085C88(u8 arg0) {
    u8 v = arg0;

    func_80085454(v);
    func_80085618(v);
    D_800D2D28[0xAD] = 0;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_80085CCC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_80085D34);
void func_80085E78(void) {
    s32 i;

    for (i = 0; i < 7; i++) {
        u8 *p = D_800C3EAC + i;

        p[0x2CC] = 0xFF;
    }
    D_800C3EAC[0x2D6] = 0;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_80085EB4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_80086028);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_800861D0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_80086B88);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_80086C88);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_80086F98);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_800877E0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_800879A8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_80087A38);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_80087AF0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_80087EDC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_800881B8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_800883AC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_80088490);
u8 func_800885D0(u8 arg0) {
    return D_800D301C[(D_800C3EB4[arg0 * 28] + 0x18) << 2];
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_8008860C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc13", func_8008887C);
