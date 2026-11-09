#include "common.h"

typedef struct {
    /* 0x0 */ u32 unk0;
    /* 0x4 */ s16 unk4;
    /* 0x6 */ s16 unk6;
} BattleMisc22Unk; // Size: 0x8

extern s32 D_800D3344;
extern s32 D_800D39CC;

INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A2E88);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A2F94);
/* 99.41%: instruction-for-instruction identical except that gcc puts the multu
   high part in a2 and shifts into v1, where retail mfhi's straight into v1.
   Source is right: 124 = 4*31 is the only divisor shape that reproduces the
   0x21084211/srl 2 sequence (a plain /31 gives the +1-magic variant).
s32 func_800A2F94(BattleMisc22Unk *pArg, u8 *pEnd) {
    s32 count = ((u32)pEnd - pArg->unk0) / 124;

    if (pArg->unk6 >= count) {
        pArg->unk6 = count;
    }
    ((s16 *)pEnd)[0xB] = -1;
    return count;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A2FD8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A32D8);
void func_800A3484(s16 *pArg) {
    *pArg = -1;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A3490);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A3514);
/* byte-exact under maspsx --expand-div; the Default preset does not pass it, so
   the div-by-zero trap sequence is missing from our object (99.41% otherwise).
s16 func_800A3514(s16 angle, s16 divisor, s16 base) {
    s16 result = base + angle / divisor;

    if (result >= 0x21) {
        return -1;
    }
    return result;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A3578);
/* byte-exact under maspsx --expand-div, see above.
s16 func_800A3578(s16 angle, s16 divisor, s16 base) {
    return base - angle / divisor;
}
*/
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A35C8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A3640);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A3E98);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A429C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A4348);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A43F8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A44C0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A4654);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A4820);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A48EC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A4B3C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A4CF8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A4DB8);
s32 func_800A577C(void) {
    return D_800D3344;
}

s32 func_800A578C(void) {
    return D_800D39CC;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A579C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc22", func_800A5870);
