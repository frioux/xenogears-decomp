#include "common.h"
#include "psyq/libgte.h"
#include "system/controller.h"
#include "system/menu.h"
#include "system/memory.h"

extern s32* D_8005917C; // Debugger state pointer. -1 when no debugger is attached
extern void FontDrawLetters(void* ot);
extern void FontPrintf(char* pFormat, ...);

// Debug menu state shared with the menu overlays
extern u8 D_80059171;       // Sub-parameter: character, shop or CD number
extern u8 D_80059460;       // Index of the menu overlay the menu was entered with
extern char* D_8004FA9C[];  // Per-overlay name tables, one entry per overlay
extern char D_8001833C[];   // "\n\n     Menu=(%d)%s\n"
extern char D_80018350[];   // "\n\n     Chr =(%d)\n"
extern char D_80018364[];   // "\n\n     Robo =(%d)\n"
extern char D_80018378[];   // "\n\n     ShopNo =(%d)\n"
extern char D_80018390[];   // "\n\n     CdNo =(%d)\n"
extern void* D_8005945C;    // Menu resources file
extern void* D_800658CC;
extern void* D_8006BE24;
extern void* D_8005A4AC;
extern void* D_8005A4B0;
extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);
extern void ArchiveCdDataSync(int mode);
extern void ArchiveReadFileToBuffer(s32 index, void* pBuffer, u32 arg2, u32 flags);
extern void ChangeGameState(unsigned int state);

// Menu overlay entry points, loaded at 0x801C5000
extern void func_801C62A8(void);
extern void func_801CB0A8(void);
extern void func_801CBDBC(void);
extern void func_801CCD28(void);
extern void func_801CE024(void);

void MenuInitializeGfxEnvironment(GfxEnvironment* pGfxEnv) {
    pGfxEnv->drawEnv.dtd = 1;
    pGfxEnv->dispEnv.screen.y = 10;
    pGfxEnv->dispEnv.screen.w = 256;
    pGfxEnv->drawEnv.isbg = 0;
    pGfxEnv->drawEnv.r0 = 0;
    pGfxEnv->drawEnv.g0 = 0;
    pGfxEnv->drawEnv.b0 = 0;
    pGfxEnv->dispEnv.screen.x = 0;
    pGfxEnv->dispEnv.screen.h = 216;
}

void MenuInitializeGfxEnvironments(void) {
    SetGeomOffset(160, 112);
    SetGeomScreen(0x200);
    SetDefDispEnv(&g_Menu->gfxEnvs[0].dispEnv, 0, 0xE0, 0x140, 0xE0);
    SetDefDrawEnv(&g_Menu->gfxEnvs[0].drawEnv, 0, 0, 0x140, 0xE0);
    SetDefDispEnv(&g_Menu->gfxEnvs[1].dispEnv, 0, 0, 0x140, 0xE0);
    SetDefDrawEnv(&g_Menu->gfxEnvs[1].drawEnv, 0, 0xE0, 0x140, 0xE0);
    MenuInitializeGfxEnvironment(&g_Menu->gfxEnvs[0]);
    MenuInitializeGfxEnvironment(&g_Menu->gfxEnvs[1]);
}

void func_8001BEEC(void) {
    g_Menu->translation.vz = 0x800;
    g_Menu->cameraPosition.vz = 0x800;
    g_Menu->rotation.vz = 0;
    g_Menu->rotation.vy = 0;
    g_Menu->rotation.vx = 0;
    g_Menu->translation.vy = 0;
    g_Menu->translation.vx = 0;
    g_Menu->unk218.vz = 0;
    g_Menu->unk218.vy = 0;
    g_Menu->unk218.vx = 0;
    g_Menu->cameraPosition.vy = 0;
    g_Menu->cameraPosition.vx = 0;
    g_Menu->unk2E8 = 1;
    g_Menu->transitionEffectState = 0;
}

void MenuProcessControllerInput(void) {
    u_char input = MENU_INPUT_IDLE;
    if (func_80036410() != 0) {
        ControllerResetState();
        g_Menu->input = input;
        return;
    }
    while (ControllerPopState()) {
        if (g_C1ButtonStatePressedOnce & CTRL_BTN_RIGHT) {
            input = MENU_INPUT_RIGHT;
            break;
        }
        if (g_C1ButtonStatePressedOnce & CTRL_BTN_DOWN) {
            input = MENU_INPUT_DOWN;
            break;
        }
        if (g_C1ButtonStatePressedOnce & CTRL_BTN_LEFT) {
            input = MENU_INPUT_LEFT;
            break;
        }
        if (g_C1ButtonStatePressedOnce & CTRL_BTN_UP) {
            input = MENU_INPUT_UP;
            break;
        }
        if (g_C1ButtonStatePressedOnce & CTRL_BTN_CIRCLE) {
            input = MENU_INPUT_CONFIRM;
            break;
        }
        if (g_C1ButtonStatePressedOnce & CTRL_BTN_SELECT) {
            input = 12;
            g_Menu->unk1E94 = g_Menu->unk1E94 == 0;
            break;
        }
        if (g_C1ButtonStatePressedOnce & CTRL_BTN_L1) {
            if (g_Menu->unk1E95) {
                g_Menu->unk1E95 -= 1;
            }
            break;
        }
        if (g_C1ButtonStatePressedOnce & CTRL_BTN_L2) {
            g_Menu->unk1E95 += 1;
            break;
        }
    }
    g_Menu->input = input;
}

void func_8001C074(void) {
    GfxEnvironment* pNextGfxEnv;

    MenuProcessControllerInput();

    // Rotate the graphics environment
    pNextGfxEnv = &g_Menu->gfxEnvs[0];
    if (g_Menu->pGfxEnv == &g_Menu->gfxEnvs[0]) {
        pNextGfxEnv = &g_Menu->gfxEnvs[1];
    }
    g_Menu->pGfxEnv = pNextGfxEnv;

    g_Menu->renderContext = g_Menu->renderContext == 0;
    ClearOTagR(g_Menu->pGfxEnv->ot, 0x10);
    if (*D_8005917C != -1) {
        if (g_Menu->unk1E94) {
            // unk1E95 is volatile for MenuProcessControllerInput's sake only; the
            // original read it plainly here
            HeapDebugDump(3, *(u_char*)&g_Menu->unk1E95, 0xF, 0x80AC);
        }
        if (*D_8005917C != -1) {
            FontDrawLetters(g_Menu->pGfxEnv->ot);
        }
    }
    DrawSync(0);
    Vsync(0);
    PutDrawEnv(&g_Menu->pGfxEnv->drawEnv);
    PutDispEnv(&g_Menu->pGfxEnv->dispEnv);
    DrawOTag(&g_Menu->pGfxEnv->ot[0xF]);
}

void MenuExecute(void) {
    int nMenuIndex;
    int nSubIndex;
    u_char bShouldContinue;
    void* pResources;
    void* pDebugData;
    void* pBlockReserve;
    void* pOverlayReserve;

    nMenuIndex = 0;
    nSubIndex = 0;
    bShouldContinue = 1;

    if (g_MenuDebugEnabled) {
        // Debug screen: pick a menu overlay and a sub-parameter, then run it
        do {
            FontPrintf(D_8001833C, nMenuIndex, D_8004FA9C[nMenuIndex]);
            if (nMenuIndex < 4) {
                if (nSubIndex < 0xB) {
                    FontPrintf(D_80018350, nSubIndex);
                } else {
                    FontPrintf(D_80018364, nSubIndex - 0xB);
                }
            } else {
                FontPrintf(nMenuIndex != 6 ? D_80018378 : D_80018390, nSubIndex);
            }

            switch (g_Menu->input) {
                case MENU_INPUT_CONFIRM:
                    bShouldContinue = 0;
                    break;
                case MENU_INPUT_RIGHT:
                    nMenuIndex += 1;
                    nSubIndex = 0;
                    if (nMenuIndex >= 7) {
                        nMenuIndex = 0;
                    }
                    break;
                case MENU_INPUT_LEFT:
                    nMenuIndex -= 1;
                    nSubIndex = 0;
                    if (nMenuIndex < 0) {
                        nMenuIndex = 6;
                    }
                    break;
                case MENU_INPUT_UP:
                    if (nMenuIndex < 4) {
                        nSubIndex += 1;
                        if (nSubIndex >= 0x1F) {
                            nSubIndex = 0;
                        }
                    } else if (nMenuIndex == 6) {
                        nSubIndex = !nSubIndex;
                    } else {
                        nSubIndex += 1;
                    }
                    break;
                case MENU_INPUT_DOWN:
                    nSubIndex -= 1;
                    if (nSubIndex < 0) {
                        if (nMenuIndex < 4) {
                            nSubIndex = 0x1E;
                        } else if (nMenuIndex != 6) {
                            nSubIndex = 0xFF;
                        } else {
                            nSubIndex = !nSubIndex;
                        }
                    }
                    break;
            }

            func_8001C074();
        } while (bShouldContinue);

        D_80059460 = nMenuIndex;
        D_80059171 = nSubIndex;
        g_Menu->gfxEnvs[0].drawEnv.isbg = 0;
        g_Menu->gfxEnvs[1].drawEnv.isbg = 0;
        SetDispMask(0);
        g_Menu->pGfxEnv = &g_Menu->gfxEnvs[1];
        SetDispMask(1);
    }

    ArchiveSetIndex(0x10, 0);
    if (g_MenuDebugEnabled) {
        g_GameState.gold = 0x3B9AC9FF;
        HeapChangeCurrentUser(HEAP_USER_HIG, NULL);
        pResources = HeapAlloc(ArchiveDecodeAlignedSize(1), 0);
        D_8005945C = pResources;
        ArchiveReadFileToBuffer(1, pResources, 0, 0x80);
        ArchiveCdDataSync(0);
        if (D_80059460 == 5) {
            ArchiveSetIndex(4, 0);
            pDebugData = HeapAlloc(4, 1);
            D_800658CC = pDebugData;
            // Reserve the rest of the heap, up to the top of memory
            D_8006BE24 = HeapAlloc((u32)pDebugData - 0x801DC000, 1);
            ArchiveReadFileToBuffer(0x6B9, (void*)0x801DC000, 0, 0x80);
            ArchiveCdDataSync(0);
            ArchiveSetIndex(0x10, 0);
            D_8005A4AC = HeapAlloc(0x4000, 0);
            D_8005A4B0 = HeapAlloc(0x4000, 0);
        }
        // Reserve the rest of the heap, up to where the menu overlay is loaded
        pBlockReserve = HeapAlloc(4, 1);
        pOverlayReserve = HeapAlloc((u32)pBlockReserve - 0x801C5000, 1);
        ArchiveReadFileToBuffer(D_80059460 + 5, (void*)0x801C5000, 0, 0x80);
        ArchiveCdDataSync(0);
    }

    ArchiveSetIndex(0x10, 0);
    switch (D_80059460) {
        case 0:
            func_801C62A8();
            break;
        case 1:
            func_801CB0A8();
            break;
        case 3:
            func_801CBDBC();
            break;
        case 4:
            func_801CCD28();
            break;
        case 2:
        case 6:
            func_801C62A8();
            ChangeGameState(1);
            break;
        case 5:
            func_801CE024();
            break;
    }

    if (g_MenuDebugEnabled) {
        HeapFree(pBlockReserve);
        HeapFree(pOverlayReserve);
        if (D_80059460 == 5) {
            HeapFree(D_800658CC);
            HeapFree(D_8006BE24);
            HeapFree(D_8005A4AC);
            HeapFree(D_8005A4B0);
        }
        g_MenuDebugEnabled = 1;
        MainLoop(0);
    }
}

// Before calling this, it's expected that the caller have loaded the correct
// menu overlay to the correct address (0x801C5000).
void MenuMain() {
    g_Menu = HeapAlloc(sizeof(SystemMenu), 0);
    bzero(g_Menu, sizeof(SystemMenu));
    g_Menu->input = 8;
    HeapChangeCurrentUser(HEAP_USER_HIG, NULL);
    g_Menu->pGfxEnv = &g_Menu->gfxEnvs[1];
    g_Menu->unk1E94 = 0;
    g_Menu->unk1E95 = 1;
    g_Menu->unk2D8 = 0;
    g_Menu->shouldDrawMenu = FALSE;
    MenuInitializeGfxEnvironments();
    if (g_MenuDebugEnabled) {
        g_Menu->gfxEnvs[0].drawEnv.isbg = 1;
        g_Menu->gfxEnvs[1].drawEnv.isbg = 1;
    }
    func_8001BEEC();
    Vsync(0);
    PutDrawEnv(&g_Menu->gfxEnvs[0].drawEnv);
    PutDrawEnv(&g_Menu->gfxEnvs[1].drawEnv);
    PutDispEnv(&g_Menu->gfxEnvs[0].dispEnv);
    PutDispEnv(&g_Menu->gfxEnvs[1].dispEnv);
    SetDispMask(1);

    // This call takes over control flow and runs the loaded menu until it returns
    MenuExecute();

    g_MenuDebugEnabled = 1;
}

INCLUDE_ASM("asm/slus_006.64/nonmatchings/system/menu", func_8001C76C);
