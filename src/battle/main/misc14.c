#include "common.h"

extern u16 D_800C3468[];
extern u16 D_800C3448[];

INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_80088990);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_80088B80);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_80089038);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_80089110);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_800891E4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_80089348);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_8008946C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_8008963C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_800897CC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_800898F0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_80089AF8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_80089B50);
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_8008A274);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_8008A3EC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_8008A684);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_8008A9C0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_8008AA40);
