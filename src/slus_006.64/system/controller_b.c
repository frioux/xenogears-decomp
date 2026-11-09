#include "common.h"
#include "system/controller.h"
#include "psyq/libapi.h"

extern u_char D_8005938C;
extern int D_80059390;
extern int D_800501FC;
extern int D_80050204;

extern void ControllerResetState(void);

typedef struct {
    /* 0x0 */ u_char unk0;
    /* 0x1 */ u_char unk1;
    /* 0x2 */ u_char unk2;
    /* 0x3 */ u_char unk3;
    /* 0x4 */ short unk4;
    /* 0x6 */ u_char unk6;
    /* 0x7 */ u_char unk7;
} ControllerBState; // Size: 0x8

extern ControllerBState D_8005A1BC[];
// D_8005A1C0/D_8005A1C3 are the record array's unk4/unk7 seen through their own
// symbol, which is how splat split the .bss labels; element stride stays 0x8.
extern u_short D_8005A1C0[];
extern u_char D_8005A1C3[];

extern void func_80036188(ControllerBState* pState);
extern void func_80040C3C(void* pDest, int destSize, void* pSrc, int srcSize);

extern u_char D_800501F8;
extern u_char D_80059370;
extern u_char D_80059418;
extern u_char D_80059420;
extern u_char D_80059484;

void func_80035E44(void) {
    int limit;

    limit = 0x3C;
    if (D_800501F8 != 0)
        return;
    if (++D_80059370 == limit) {
        D_80059370 = 0;
        D_80059418++;
    }
    if (D_80059418 == limit) {
        D_80059418 = 0;
        D_80059420++;
    }
    if (D_80059420 == limit) {
        D_80059420 = 0;
        D_80059484++;
    }
    if (D_80059484 == 0x64)
        D_800501F8 = 1;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/controller_b", func_80035F1C);

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/controller_b", func_80035FF8);

void func_8003611C(void) {
    D_8005A1BC[0].unk0 = 0;
    D_8005A1BC[0].unk4 = 0;
    D_8005A1BC[0].unk6 = 0;
    D_8005A1BC[0].unk7 = 0;
    D_8005A1BC[1] = D_8005A1BC[0];
    func_80040C3C(&D_8005A1BC[0], 4, &D_8005A1BC[1], 4);
}

void func_80036188(ControllerBState* pState) {
    if (pState->unk7) {
        return;
    }
    if (pState->unk4) {
        pState->unk0 = 1;
        pState->unk1 = 0x40;
        pState->unk2 = 1;
        pState->unk3 = 0;
        pState->unk6 = 1;
        pState->unk4 -= 1;
    } else if (pState->unk6 == 1) {
        pState->unk0 = 1;
        pState->unk1 = 0x40;
        pState->unk2 = 0;
        pState->unk3 = 0;
        pState->unk6 = 2;
    } else if (pState->unk6 == 2) {
        pState->unk0 = 0;
        pState->unk6 = 0;
    }
}

void func_80036220(void) {
    ControllerBState* pState = D_8005A1BC;

    func_80036188(&pState[0]);
    func_80036188(&pState[1]);
}

void func_80036258(int index, u_short value) {
    D_8005A1C0[index * 4] = value;
}

void func_80036270(int index, u_char value) {
    D_8005A1C3[index * 8] = value;
}

void ControllerInit(void) {
    u_char* pMapping;
    int i;

    InitPAD(g_C1Buffer, CONTROLLER_BUFFER_SIZE, g_C1Buffer + CONTROLLER_BUFFER_SIZE, CONTROLLER_BUFFER_SIZE);
    StartPAD();
    ChangeClearPAD(0);
    ControllerResetState();
    func_8003611C();

    D_80050204 = 0;
    D_8005938C = 1;
    D_80059390 = 0;
    i = 7;
    pMapping = &g_ControllerButtonMappings[7];
    for (; i >= 0; i--) {
        *pMapping-- = i;
    }
    g_ControllerButtonMappings[0] = 1;
    g_ControllerButtonMappings[1] = 0;
    g_ControllerButtonMappings[2] = 3;
    g_ControllerButtonMappings[3] = 2;
}

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
