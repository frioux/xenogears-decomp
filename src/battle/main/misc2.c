#include "common.h"
#include "psyq/libgpu.h"

extern u8 D_8005959C;
extern s32 D_800C3EA4;
extern s32 D_800CCB34;
extern void *D_800D2F5C;
extern u8 *D_800D2D28;
extern u8 *D_800D2DB4;

extern s32 func_8002675C(void *pDir, s32 index, void *pPrims, s32 context, s16 x, s16 y, s16 scale);
extern void func_800728B8(u8 *pDst, s32 arg1, s32 arg2);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc2", func_80073B64);

void func_80073E88(void) {
    s32 i;
    s32 offset;
    u8 *p;

    if (D_800D2D28[0xCB] == 0) {
        return;
    }

    i = 0;
    offset = 0x641C;
    do {
        p = D_800D2D28;
        func_800728B8((u8 *)(offset + D_800C3EA4), *(s32 *)(p + i * 4 + 0xD0), p[0xA3]);
        offset += 0xFA0;
        i++;
    } while (i < 2);
}

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

void func_80076A10(s32 arg0, s32 arg1, s16 arg2, s16 arg3) {
    func_8002675C(D_800D2F5C, arg0, arg1, D_800CCB34, arg2, arg3, 0x1000);
}

void func_80076A6C(s32 arg0, s32 arg1, s16 arg2, s16 arg3) {
    func_8002675C(D_800D2F5C, arg0, arg1, D_800CCB34, arg2, arg3, 0x800);
}

