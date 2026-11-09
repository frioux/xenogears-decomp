#include "common.h"

extern u8 D_800D2E5D[];
extern u8 D_800D2E60[];
extern u8 D_800D2E62[];
extern u16 D_800D39E0;
extern u8* D_800C3EAC;
extern u8 D_800C402F[];
extern void func_800785D4(u8 arg0, u8 arg1);
extern s32 func_80079ED8(u8 arg0, u8 arg1, u8 arg2, s32 arg3);

INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_8007887C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_8007893C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80078998);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80078B34);
void func_80078C9C(u8 arg0, u8 arg1) {
    D_800C402F[D_800C3EAC[0x2DA] * 72] = 0xFC;
    func_800785D4(arg0, arg1);
}
void func_80078CEC(u8 arg0, u8 arg1) {
    D_800C402F[D_800C3EAC[0x2DA] * 72] = D_800D2E60[arg1 * 8];
    func_800785D4(arg0, arg1);
}

void func_80078D48(u8 arg0, u8 arg1) {
    *(u16*)&D_800D39E0 = *(u16*)(D_800D2E62 + arg1 * 8);
}

INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80078D6C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80078E24);
void func_80079054(u8 arg0, u8 arg1) {
    func_80079ED8(arg0, D_800D2E5D[arg1 * 8], D_800D2E60[arg1 * 8], 0);
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80079098);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80079114);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_8007916C);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_800791FC);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80079270);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_800792F8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_800793F0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80079674);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80079778);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80079840);
void func_80079934(u8** pCursor) {
    *pCursor += 4;
}
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80079948);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_800799C8);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80079AB0);
INCLUDE_ASM("asm/battle/nonmatchings/main/misc4", func_80079C24);
