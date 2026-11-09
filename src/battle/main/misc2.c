#include "common.h"
#include "psyq/libgpu.h"

extern u8 D_8005959C;
extern u8 *D_800C3EA4;
extern u8 *D_800D2D28;
extern u8 *D_800D2DB4;

extern void func_800728B8(u8 *pDst, s32 arg1, s32 arg2);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80073B64);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80073E88);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80073F08);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80073FB8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_800742A0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_800743A4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_800744BC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80074554);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_800745EC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80074AB8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80074D4C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80074EEC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80074F70);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_8007500C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80075168);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80075938);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80076418);

void func_800764B4(void) {
    D_8005959C = 0;
    func_8008FAD8();
    func_801DE594();
    func_80073FB8();
}

void func_800764EC(void) {
    func_8008FAD8();
    func_80073538();
    func_80073F08();
    func_8007500C();
    func_80074F70();
    func_80073FB8();
    func_80088B80();
    func_80074AB8();
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80076544);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_800765C4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80076710);

void func_800769E8(RECT *pRect, u_long *pData) {
    LoadImage(pRect, pData);
    DrawSync(0);
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80076A10);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80076A6C);
