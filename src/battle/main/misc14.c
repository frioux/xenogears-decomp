#include "common.h"
#include "libgear/main.h"

extern u16 D_800C3468[];
extern u16 D_800C3448[];
extern u8 D_800C3E4C;
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
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_8008A144);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc14", func_8008A274);
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
