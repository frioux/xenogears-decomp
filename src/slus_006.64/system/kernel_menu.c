#include "common.h"
#include "system/kernel.h"
#include "system/memory.h"
#include "system/controller.h"
#include "system/font.h"

void KernelMenuInitializeCursor(int index) {
    KernelMenuRenderEnvironment* pCursor;

    pCursor = &g_KernelMenuRenderEnvironments[index];
    pCursor->drawEnv.isbg = 1;
    pCursor->drawEnv.dtd = 1;
    pCursor->drawEnv.r0 = 0;
    pCursor->drawEnv.g0 = 0;
    pCursor->drawEnv.b0 = 0x20;
    SetPolyF3(&pCursor->cursor);
    setRGB0(&pCursor->cursor, 0xFF, 0xFF, 0xFF);
}

void KernelMenuInitialize(void) {
    char _pad[4];
    
    HeapChangeCurrentUser(HEAP_USER_SUGI, NULL);
    FontLoadFont(8, 0x10, 0x170, 0x1E0, 0x3E8, 1, 0x3C0, 0x100, 0x3C0, 0x1FF, 0);
    SetDefDrawEnv(&g_KernelMenuRenderEnvironments[0].drawEnv, 0, 0, 0x140, 0xE0);
    SetDefDrawEnv(&g_KernelMenuRenderEnvironments[1].drawEnv, 0, 0xF0, 0x140, 0xE0);
    SetDefDispEnv(&g_KernelMenuRenderEnvironments[0].dispEnv, 0, 0xF0, 0x140, 0xE0);
    SetDefDispEnv(&g_KernelMenuRenderEnvironments[1].dispEnv, 0, 0, 0x140, 0xE0);
    KernelMenuInitializeCursor(0);
    KernelMenuInitializeCursor(1);
}

extern char D_800182B0; // "%02d:%02d:%02d"
extern char D_800182C0; // " XENOGEARS Kernel MENU\n  %s %s MODE\n\n"
extern char D_800182F0; // "CD EMU"
extern char D_800182E8; // "PC HDD"
extern char D_800182F8; // "    Field\n    Battle\n    Worldmap\n    Battling\n    Menu\n    Movie\n\n"
void KernelMenuUpdate(void) {
    char sBuffer[24];
    char* pMode;
    int nChoice;
    int nOffsetY;

    if (g_C1ButtonStatePressedOnce & CTRL_BTN_UP) {
        nChoice = g_KernelMenuCurChoice - 1;
        g_KernelMenuCurChoice = nChoice;
        if (nChoice < 0)
            g_KernelMenuCurChoice = 5;
    }

    if (g_C1ButtonStatePressedOnce & CTRL_BTN_DOWN) {
        nChoice = g_KernelMenuCurChoice + 1;
        g_KernelMenuCurChoice = nChoice;
        if (nChoice >= 6)
            g_KernelMenuCurChoice = 0;
    }

    if (g_C1ButtonStateReleased & CTRL_BTN_CIRCLE) {
        ChangeGameState(g_KernelMenuCurChoice + 1);
        g_KernelMenuIsRunning = FALSE;
    }

    // Version?
    Sprintf(&sBuffer, &D_800182B0, g_PlayTimeHours, g_PlayTimeMinutes, (s32) g_PlayTimeSeconds);

    // Get mode / device
    pMode = &D_800182F0;
    if (g_DebugTable)
        pMode = &D_800182E8;
    
    FontPrintf(&D_800182C0, &sBuffer, pMode);
    FontPrintf(&D_800182F8);

    // Set cursor position
    nOffsetY = g_KernelMenuCurChoice * 8;
    setXY0Fast(&g_KernelMenuCurRenderEnvironment->cursor, 0x20, nOffsetY + 0x28);
    setXY1Fast(&g_KernelMenuCurRenderEnvironment->cursor, 0x27, nOffsetY + 0x2C);
    setXY2Fast(&g_KernelMenuCurRenderEnvironment->cursor, 0x20, nOffsetY + 0x30);
}

void KernelMenuMain(void) {
    void* pOtag;

    KernelMenuInitialize();
    g_KernelMenuIsRunning = 1;
    D_800592C8 = 0;
    while (g_KernelMenuIsRunning || !D_800592C8) {
        D_800592C4++;
        D_800592C8 = D_800592C4 & 1; // Render context
        g_KernelMenuCurRenderEnvironment = &g_KernelMenuRenderEnvironments[D_800592C8];
        pOtag = &g_KernelMenuCurRenderEnvironment->ot;
        TermPrim(pOtag);
        FontDrawLetters(pOtag);
        KernelMenuUpdate();
        AddPrim(pOtag, &g_KernelMenuCurRenderEnvironment->cursor);
        DrawSync(0);
        Vsync(0);
        PutDrawEnv(&g_KernelMenuCurRenderEnvironment->drawEnv);
        PutDispEnv(&g_KernelMenuCurRenderEnvironment->dispEnv);
        DrawOTag(pOtag);
    }
    DrawSync(0);
    MainLoop(0);
}
