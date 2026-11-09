#include "common.h"
#include "psyq/libgpu.h"

extern u8 *D_800C3EA4;
extern u8 *D_800D2D28;

extern s32 func_8008ABB8(s32, s32);
extern void func_80077074(void);
extern void HeapFree(void *pMem);
extern void bzero(void *p, s32 n);

void func_80076AC8(u8 *p) {
    SetSemiTrans(p, 1);
    SetShadeTex(p, 0);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_80076B00);
void func_80076B68(u8 *p) {
    func_80076AC8(p);
    p[4] = 0x80;
    p[5] = 0x80;
    p[6] = 0x80;
    *(u16 *)(p + 0x16) |= 0x20;
}

void func_80076BAC(u8 *p) {
    func_80076AC8(p);
    p[4] = 0x40;
    p[5] = 0x40;
    p[6] = 0x40;
    *(u16 *)(p + 0x16) |= 0x20;
}

void func_80076BF0(u8 *p) {
    func_80076AC8(p);
    p[4] = 0x80;
    p[5] = 0x80;
    p[6] = 0x80;
    *(u16 *)(p + 0x16) |= 0x40;
}

void func_80076C34(u8 *p) {
    func_80076AC8(p);
    p[4] = 0x40;
    p[5] = 0x40;
    p[6] = 0x40;
    *(u16 *)(p + 0x16) |= 0x40;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_80076C78);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_80076CE8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_80076D58);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_80076EA4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_80077074);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_80077364);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_80077454);
void func_80077610(void) {
    u8 *p = (u8 *)func_8008ABB8(0x670, 0);

    *(u8 **)(D_800C3EA4 + 0xA230) = p;
    bzero(p, 0x670);
    func_80077074();
}

void func_8007765C(void) {
    func_800716D8();
    HeapFree(*(u8 **)(D_800C3EA4 + 0xA230));
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_80077698);
void func_80077980(void) {
    D_800D2D28[0xC6] = 0;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_80077990);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_800780A8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_8007819C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_80078310);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_80078508);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_800785D4);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_80078658);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc3", func_800787E0);
