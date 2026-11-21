#include "common.h"
#include "system/controller.h"
#include "psyq/libapi.h"
#include "psyq/libgpu.h"
#include "psyq/pc.h"
#include "system/memory.h"

extern u_char D_8005938C;
extern int g_DebugTable;
extern int g_ControllerDebugBreakEnabled;
extern int g_ControllerFrameCallback;
extern int D_80050204;

extern void ControllerResetState(void);
extern void ControllerPoll(void);
extern void ControllerPushState(void);

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

typedef struct {
    /* 0x0 */ u_long magic;
    /* 0x4 */ u_long flags;
    /* 0x8 */ u_long size;
    /* 0xC */ RECT rect;
} TimHeader; // Size: 0x14

extern void ControllerUpdateActuatorState(ControllerBState* pState);
extern void SendPAD(void* pDest, int destSize, void* pSrc, int srcSize);

extern u_char g_PlayTimeStopped;
extern u_char g_PlayTimeFrameCounter;
extern u_char g_PlayTimeSeconds;
extern u_char g_PlayTimeMinutes;
extern u_char g_PlayTimeHours;

void ControllerUpdatePlayTime(void) {
    int limit;

    limit = 0x3C;
    if (g_PlayTimeStopped != 0)
        return;
    if (++g_PlayTimeFrameCounter == limit) {
        g_PlayTimeFrameCounter = 0;
        g_PlayTimeSeconds++;
    }
    if (g_PlayTimeSeconds == limit) {
        g_PlayTimeSeconds = 0;
        g_PlayTimeMinutes++;
    }
    if (g_PlayTimeMinutes == limit) {
        g_PlayTimeMinutes = 0;
        g_PlayTimeHours++;
    }
    if (g_PlayTimeHours == 0x64)
        g_PlayTimeStopped = 1;
}

void ControllerDumpVramToTim(RECT* pRect, char* pPath) {
    TimHeader header;
    int fd;

    StoreImage(pRect, (u_long*)0x80700000);
    header.magic = 0x10;
    header.flags = 2;
    header.size = pRect->w * pRect->h * 2 + 0xC;
    header.rect.x = pRect->x;
    header.rect.y = pRect->y;
    header.rect.w = pRect->w;
    header.rect.h = pRect->h;
    fd = PCcreate(pPath, 0);
    PCwrite(fd, (char*)&header, 0x14);
    PCwrite(fd, (char*)0x80700000, pRect->w * pRect->h * 2);
    PCclose(fd);
}

extern char D_80018ABC[];

typedef struct {
    /* 0x0 */ u_short red : 5;
    /* 0x0 */ u_short green : 5;
    /* 0x0 */ u_short blue : 5;
} Pixel16; // Size: 0x2

int ControllerDumpVramToRgb(RECT* pRect, char* pPath) {
    char buffer[0x100];
    Pixel16* pSrc;
    u_char* pDst;
    int count;
    s32 i; // s32 and not int: the typedef is what makes cc1 keep retail's loop copy
    int fd;

    // 0x80600000/0x80700000 are the raw VRAM scratch buffers, spelled as literals
    // in the original (a bare lui), which is why they are not symbol references.
    StoreImage(pRect, (u_long*)0x80600000);
    DrawSync(0);
    Sprintf(buffer, D_80018ABC, pRect->w, pRect->h);

    count = pRect->w * pRect->h;
    pSrc = (Pixel16*)0x80600000;
    pDst = (u_char*)0x80700000;
    i = count;
    while (i-- != 0) {
        *pDst++ = pSrc->red << 3;
        *pDst++ = pSrc->green << 3;
        *pDst++ = pSrc->blue << 3;
        pSrc++;
    }

    fd = PCcreate(pPath, 0);
    if (fd == -1)
        return -1;
    PCwrite(fd, buffer, strlen(buffer));
    PCwrite(fd, (char*)0x80700000, count * 3);
    PCclose(fd);
    return 0;
}

void ControllerResetActuators(void) {
    D_8005A1BC[0].unk0 = 0;
    D_8005A1BC[0].unk4 = 0;
    D_8005A1BC[0].unk6 = 0;
    D_8005A1BC[0].unk7 = 0;
    D_8005A1BC[1] = D_8005A1BC[0];
    SendPAD(&D_8005A1BC[0], 4, &D_8005A1BC[1], 4);
}

void ControllerUpdateActuatorState(ControllerBState* pState) {
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

void ControllerUpdateActuators(void) {
    ControllerBState* pState = D_8005A1BC;

    ControllerUpdateActuatorState(&pState[0]);
    ControllerUpdateActuatorState(&pState[1]);
}

void ControllerSetActuatorDuration(int index, u_short value) {
    D_8005A1BC[index].unk4 = value;
}

void ControllerSetActuatorDisabled(int index, u_char value) {
    D_8005A1BC[index].unk7 = value;
}

void ControllerInit(void) {
    u_char* pMapping;
    int i;

    InitPAD(g_C1Buffer, CONTROLLER_BUFFER_SIZE, g_C1Buffer + CONTROLLER_BUFFER_SIZE, CONTROLLER_BUFFER_SIZE);
    StartPAD();
    ChangeClearPAD(0);
    ControllerResetState();
    ControllerResetActuators();

    D_80050204 = 0;
    D_8005938C = 1;
    g_ControllerDebugBreakEnabled = 0;
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

void ControllerSetControllerFlag(u_char value) {
    D_8005938C = value;
}

void ControllerUpdate(void) {
    s32 pad[10]; // unused: reproduces the original's 0x40-byte frame
    g_SpuChannelMask++;
    ControllerPoll();
    ControllerPushState();
    ControllerUpdatePlayTime();
    ControllerUpdateActuators();
    if (g_ControllerFrameCallback != 0)
        ((void (*)(void))g_ControllerFrameCallback)();
    if (g_DebugTable != -1 && g_ControllerDebugBreakEnabled != 0)
        asm("break 0x400");
}

void ControllerSetDebugBreak(int value) {
    g_ControllerDebugBreakEnabled = value;
}

void ControllerSetFrameCallback(int value) {
    g_ControllerFrameCallback = value;
}

void ControllerSetFlag80050200(int value) {
    D_80050200 = value;
}

int ControllerIsStateStackFull(void) {
    return g_ControllerIsStateStackFull;
}

