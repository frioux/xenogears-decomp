#include "common.h"

extern s32 D_800C367C;
extern s16 D_800C3740;
extern u8 D_800C37C8;

extern void func_800BC2F0(s32);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB248);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB314);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB350);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB540);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB620);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB690);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB6E0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB760);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB7F8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB844);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BB9D4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BBAB8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BBEE0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BC018);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BC158);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BC2F0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BC3F8);
/* objdiff 45%: only difference is the lui destination, gcc emits the assembler
   macro form (lui $at + sw %lo($at)) while the original loads %hi into $v0.
   Sizes > -G8 (arrays), a pointer local and volatile all still give $at. */
/* void func_800BC3F8(s32 arg0) { */
/*     D_800C367C = arg0; */
/* } */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BC404);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BC454);
/* objdiff 45%: same lui $at vs lui $v0 store shape as func_800BC3F8. */
/* void func_800BC454(s16 arg0) { */
/*     D_800C3740 = arg0; */
/* } */
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BC460);

void func_800BCAA4(void) {
    if (D_800C37C8 == 0) {
        func_800BC2F0(4);
    }
}

void func_800BCAD0(void) {
    if (D_800C37C8 == 0) {
        func_800BC2F0(1);
    }
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BCAFC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BCB54);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc31", func_800BCBB4);
