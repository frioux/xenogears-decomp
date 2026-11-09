#include "common.h"
#include "system/controller.h"

typedef void (*ControllerPrintFunc)(int);

extern ControllerPrintFunc D_80050594;

extern int func_80036410(void);
extern void ControllerResetState(void);

void func_80036420(void) {
    u_short nC2PressedOnce;
    u_short nC1PressedOnce;
    u_short nC2Released;
    u_short nC1Released;
    u_short nC2ButtonState;
    u_short nC1ButtonState;

    nC2PressedOnce = 0;
    nC1PressedOnce = 0;
    nC2Released = 0;
    nC1Released = 0;
    nC2ButtonState = 0;
    nC1ButtonState = 0;

    if (func_80036410()) {
        ControllerResetState();
    } else {
        while (ControllerPopState()) {
            nC1ButtonState |= g_C1ButtonState;
            nC2ButtonState |= g_C2ButtonState;
            nC1Released |= g_C1ButtonStateReleased;
            nC2Released |= g_C2ButtonStateReleased;
            nC1PressedOnce |= g_C1ButtonStatePressedOnce;
            nC2PressedOnce |= g_C2ButtonStatePressedOnce;
        }
    }

    g_C1ButtonState = nC1ButtonState;
    g_C2ButtonState = nC2ButtonState;
    g_C1ButtonStateReleased = nC1Released;
    g_C2ButtonStateReleased = nC2Released;
    g_C1ButtonStatePressedOnce = nC1PressedOnce;
    g_C2ButtonStatePressedOnce = nC2PressedOnce;
}

extern char D_80018ACC[]; // "%02x "
extern char D_80018AD4[]; // "\n"
extern char D_80018AD8[]; // "%04x\n"

void func_80036528(u_char* pBuffer) {
    int nCurAddr;
    int nEndAddr;
    int nSize;

    nSize = (pBuffer[CONTROLLER_TYPE] & 0xF) * 2 + 2;
    if (nSize != 0) {
        nCurAddr = (int)pBuffer;
        nEndAddr = nSize + nCurAddr;
        do {
            FontPrintf(D_80018ACC, *(u_char*)nCurAddr);
            nCurAddr++;
        } while (nCurAddr < nEndAddr);
    }
    FontPrintf(D_80018AD4);
    if (pBuffer[CONTROLLER_STATUS] == 0) {
        if ((pBuffer[CONTROLLER_TYPE] & 0xF0) == CONTROLLER_INTERNAL_TYPE_DIGITAL_PAD) {
            FontPrintf(D_80018AD8, (~pBuffer[CONTROLLER_BUTTONS_2] & 0xFF) |
                ((pBuffer[CONTROLLER_BUTTONS_1] << 8) ^ 0xFF00));
        }
    }
}

extern char D_80018AE0[]; // "vect0 %02x %02x\n"
extern char D_80018AF4[]; // "vect1 %02x %02x\n"
extern char D_80018B08[]; // "PADD %04x %04x\n"
extern char D_80018B18[]; // "%04x %04x %04x %04x\n"

extern int ControllerGetButtonState(int controllerIndex);
extern int ControllerPopState(void);

void ControllerDebugPrint(void) {
    u_char* pBuffer;

    pBuffer = g_C1Buffer;
    func_80036528(pBuffer);
    func_80036528(pBuffer + CONTROLLER_BUFFER_SIZE);
    FontPrintf(D_80018AE0, g_C1LeftStickXAxis, g_C1LeftStickYAxis);
    FontPrintf(D_80018AF4, g_C2LeftStickXAxis, g_C2LeftStickYAxis);
    FontPrintf(D_80018B08, g_C1ButtonState, g_C2ButtonState);
    while (ControllerPopState()) {
        FontPrintf(D_80018B18, ControllerGetButtonState(0), g_C1ButtonState,
            g_C1ButtonStateReleased, g_C1ButtonStatePressedOnce);
    }
}

void func_800366E0(ControllerPrintFunc pPrintFunc) {
    D_80050594 = pPrintFunc;
}

void func_800366F0(int c) {
    D_80050594(c);
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/controller_c", func_80036718);
