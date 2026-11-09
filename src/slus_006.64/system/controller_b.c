#include "common.h"
#include "system/controller.h"

extern u_char D_8005938C;
extern int D_80059390;
extern int D_800501FC;

typedef struct {
    /* 0x0 */ u_char unk0;
    /* 0x1 */ u_char unk1;
    /* 0x2 */ u_char unk2;
    /* 0x3 */ u_char unk3;
    /* 0x4 */ u_short unk4;
    /* 0x6 */ u_char unk6;
    /* 0x7 */ u_char unk7;
} ControllerBState; // Size: 0x8

extern ControllerBState D_8005A1BC[];

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/controller_b", func_80035E44);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/controller_b", func_80035F1C);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/controller_b", func_80035FF8);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/controller_b", func_8003611C);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/controller_b", func_80036188);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/controller_b", func_80036220);

void func_80036258(int index, u_short value) {
    *(u_short*)((u_char*)D_8005A1BC + index * 8 + 4) = value;
}

void func_80036270(int index, u_char value) {
    D_8005A1BC[index].unk7 = value;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/controller_b", ControllerInit);

void func_8003633C(u_char value) {
    D_8005938C = value;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/controller_b", func_8003634C);

void func_800363E0(int value) {
    D_80059390 = value;
}

void func_800363F0(int value) {
    D_800501FC = value;
}

void func_80036400(int value) {
    D_80050200 = value;
}

int func_80036410(void) {
    return g_ControllerIsStateStackFull;
}
