#include "common.h"
#include "system/memory.h"

extern u8 D_800915AC[];
extern u8 D_80091660[];
extern u16 D_80091672;
extern u16 D_800916AE;
extern s32 *D_80092734;
extern s32 *D_80092738;
extern u8 D_80092758;
extern s32 *D_80092760;
extern s32 D_800928C8;
extern s32 D_800928D8;
extern s32 D_80092924;
extern s32 D_80092940;

extern void func_8007F834(void);
extern void func_800719F0(void);
extern void func_8008509C(s32 a0, s32 a1);
extern void func_80039FF8(void);
extern s32 func_800891C0(s32 a0);
extern void ArchiveCdDataSync(s32 mode);

void func_800808F4(void) {
    D_80092924 = 1;
    func_8007F834();
}

void func_80080920(void) {
    D_80092924 = 1;
    func_800719F0();
    func_8008509C(0, 0);
    func_8008509C(1, 1);
}

void func_80080964(s32 a0) {
    if (a0 == 0xFF) {
        D_80092734 = D_80092738;
    } else {
        D_80092738 = D_80092734;
        D_80092734 = (s32 *)(D_800915AC + a0 * 0x3C);
    }
}

s32 func_800809BC(void) {
    return D_80092734 == (s32 *)D_80091660;
}

void func_800809D8(void) {
    func_80039FF8();
    D_80092734 = 0;
    func_80080964(3);
    D_80091672 = 0;
    D_800916AE = 0;
    D_800928C8 = 0;
    D_80092758 = 0;
    func_8007F834();
    D_80092924 = 0;
    func_80080AA0(0);
    D_80092940 = 0;
    D_800928D8 = func_800891C0(6);
}

void func_80080A58(void) {
    if (D_80092940 == 0) {
        ArchiveCdDataSync(0);
        HeapFree(D_800928D8);
        D_80092940 = 1;
    }
}

void func_80080AA0(s32 a0) {
    if (a0 != 0) {
        D_80092760 = 0;
    }
    if (D_80092760 != 0) {
        HeapFree(D_80092760);
        D_80092760 = 0;
    }
}

INCLUDE_ASM("asm/battling/nonmatchings/main/misc8", func_80080AE8);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc8", func_80080B58);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc8", func_80080C48);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc8", func_80080D10);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc8", func_80080D20);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc8", func_80080F04);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc8", func_80081094);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc8", func_80081100);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc8", func_800811AC);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc8", func_800812BC);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc8", func_800814AC);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc8", func_8008151C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc8", func_8008162C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc8", func_80081A44);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc8", func_80081D2C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc8", func_80081E00);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc8", func_80081E6C);

INCLUDE_ASM("asm/battling/nonmatchings/main/misc8", func_80081ECC);
