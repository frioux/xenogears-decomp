#include "common.h"
#include "system/controller.h"
#include "system/memory.h"

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

// Best attempt at func_80036718 so far: matches every debug-format spec but
// the frame, the state struct (16 bytes copied from D_8005A1CC, with the sign
// char at offset 0x1 of the flags word) and the register allocation still
// differ. Best objdiff: 24.96%.
INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/controller_c", func_80036718);

/*
     extern char D_80018B30[];
     extern char D_80018B44[];
     extern void* memchr(u_char* pString, u_char c, int nLength);

     typedef struct {
          int nFlags;
          char cSign;
          u_char pad5[3];
          int nWidth;
          int nPrecision;
          int nBase;
          char* pPad;
     } ControllerPrintState;

     int func_80036718(int nChannel, u_char* pFormat, int* pArgs) {
         ControllerPrintState state;
         u_char cBuffer[0x20];
         u_char* pBuffer;
         u_char* pCur;
         char* pDigits;
         int nValue;
         int nBase;
         int nCur;
         int nEnd;
         int i;
         int nCount;

         nCount = 0;
         for (; *pFormat != 0; pFormat++) {
             if (*pFormat != '%') {
                 func_800366F0(*pFormat);
                 nCount++;
                 continue;
             }
             state.nFlags = 0;
             state.cSign = 0;
             while (1) {
                 pFormat++;
                 if (*pFormat == '-') {
                     state.nFlags |= 1;
                 } else if (*pFormat == '+') {
                     state.nFlags |= 2;
                 } else if (*pFormat == ' ') {
                     state.cSign = ' ';
                 } else if (*pFormat == '0') {
                     state.nFlags |= 4;
                 } else {
                     break;
                 }
             }
             if (*pFormat == '*') {
                 pArgs++;
                 if (*pArgs < 0) {
                     state.nFlags |= 1;
                     *pArgs = -*pArgs;
                 }
                 state.nWidth = *pArgs;
                 pFormat++;
             } else {
                 while (*pFormat >= '0' && *pFormat <= '9') {
                     state.nWidth = state.nWidth * 10 + (*pFormat - '0');
                     pFormat++;
                 }
             }
             if (*pFormat == '.') {
                 pFormat++;
                 if (*pFormat == '*') {
                     pArgs++;
                     state.nPrecision = *pArgs;
                     pFormat++;
                 } else {
                     while (*pFormat >= '0' && *pFormat <= '9') {
                         state.nPrecision = state.nPrecision * 10 + (*pFormat - '0');
                         pFormat++;
                     }
                 }
                 if (state.nPrecision >= 0) {
                     state.nFlags |= 8;
                 }
             }
             if (state.nFlags & 1) {
                 state.nFlags &= ~4;
             }
             pBuffer = cBuffer + 0x20;
             switch (*pFormat) {
             case 'b':
                 pArgs++;
                 nValue = *pArgs;
                 nBase = 2;
                 goto unsignedNumber;
             case 'd':
             case 'i':
                 pArgs++;
                 nValue = *pArgs;
                 if (nValue < 0) {
                     nValue = -nValue;
                     state.cSign = '-';
                 } else if (state.nFlags & 2) {
                     state.cSign = '+';
                 }
                 nBase = 10;
                 goto number;
             case 'u':
                 pArgs++;
                 nValue = *pArgs;
                 nBase = 10;
             unsignedNumber:
                 state.cSign = 0;
             number:
                 state.nBase = nBase;
                 if (!(state.nFlags & 8)) {
                     if (state.nFlags & 4) {
                         if (state.cSign != 0) {
                             state.nPrecision = state.nWidth - 1;
                         } else {
                             state.nPrecision = state.nWidth;
                         }
                     }
                     if (state.nPrecision <= 0) {
                         state.nPrecision = 1;
                     }
                 }
                 i = 0;
                 while (nValue != 0) {
                     pBuffer--;
                     *pBuffer = nValue % state.nBase + '0';
                     nValue /= state.nBase;
                     i++;
                 }
                 if (i < state.nPrecision) {
                     do {
                         pBuffer--;
                         *pBuffer = '0';
                         i++;
                     } while (i < state.nPrecision);
                 }
                 if (state.cSign != 0) {
                     pBuffer--;
                     *pBuffer = state.cSign;
                     i++;
                 }
                 goto output;
             case 'p':
                 state.nPrecision = 8;
                 state.nFlags |= 8;

             case 'X':
                 pDigits = D_80018B30;
                 goto hexNumber;
             case 'x':
                 pDigits = D_80018B44;
             hexNumber:
                 pArgs++;
                 nValue = *pArgs;
                 if (!(state.nFlags & 8)) {
                     if (state.nFlags & 4) {
                         state.nPrecision = state.nWidth;
                     }
                     if (state.nPrecision <= 0) {
                         state.nPrecision = 1;
                     }
                 }
                 i = 0;
                 while (nValue != 0) {
                     pBuffer--;
                     *pBuffer = pDigits[nValue & 0xF];
                     nValue >>= 4;
                     i++;
                 }
                 if (i < state.nPrecision) {
                     do {
                         pBuffer--;
                         *pBuffer = '0';
                         i++;
                     } while (i < state.nPrecision);
                 }
                 goto output;
             case 'c':
                 pArgs++;
                 pBuffer--;
                 *pBuffer = *pArgs;
                 i = 1;
                 goto output;
             case 't':
                 pArgs++;
                 pBuffer = (u_char*)*pArgs;
                 if (state.nFlags & 8) {
                     pCur = memchr(pBuffer, 0, state.nPrecision);
                     i = pCur - pBuffer;
                     if (pCur == 0) {
                         i = state.nPrecision;
                     }
                 } else {
                     i = strlen((char*)pBuffer);
                 }
                 goto output;
             case 'n':
                 pArgs++;
                 *pArgs = nCount;
                 continue;
             default:
                 if (*pFormat != '%') {
                     return nCount;
                 }
                 func_800366F0(*pFormat);
                 nCount++;
                 continue;
             }
         output:
             if (i < state.nWidth && !(state.nFlags & 1)) {
                 do {
                     func_800366F0(' ');
                     nCount++;
                     state.nWidth--;
                 } while (i < state.nWidth);
             }
             if (i > 0) {
                 nCur = (int)pBuffer;
                 nEnd = i + nCur;
                 do {
                     func_800366F0(*(u_char*)nCur);
                     nCur++;
                 } while (nCur < nEnd);
             }
             nCount += i;
             while (i < state.nWidth) {
                 func_800366F0(' ');
                 i++;
                 nCount++;
             }
         }
         return nCount;
     }
*/
