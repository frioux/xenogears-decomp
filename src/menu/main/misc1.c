#include "common.h"

#include "system/menu.h"

extern u8 g_MenuOverlayIndex; // Index of the menu overlay the menu was entered with
extern u_char g_MenuMainTitleStringIds[]; // String IDs of the menu strings

void MenuUploadStringTextures(MenuString* pStringList, u_char* pStringIdList, s32 yOffset, s32 count);

u_char func_801C531C(u_char arg0);
void MenuRenderFrame(void);
void MenuPlaySoundEffect(u_char soundEffectId);
void MenuResetSelectionCursorFlags(void);
void MenuClearBytes(s32 arg0, void* pArg1);
void MenuSetRowQuadVertices(s32 arg0, void* pArg1, void* pArg2, void* pArg3, void* pArg4, u_char arg5,
    u_char arg6, u_char arg7);
void MenuSetSelectionMenuPrims(u_char count, u_char selected, s32* pEntries);
void MenuLoadStringsFromIds(s32 arg0, void* pArg1, void* pArg2, void* pArg3);
void MenuRevealSelectionMenu(s32 arg0, void* pArg1);
extern int ArchiveGetDiscNumber(void);
void MenuWaitForDiscChange(u_char arg0);
s32 MenuLoadWindow2Text(u_char a0, u_char a1, u_char a2);
void MenuStartOpenAnimation(void);
void MenuSetupPointerCursors(s32 arg0);

extern u8 g_MenuMainSelectionResourceIds[];
extern u8 g_MenuMainRowStringIds[];
extern u8 g_MenuMainRowXOffsets[];
extern u8 g_MenuLoadGameSelectionResourceIds[];
extern u8 g_MenuLoadGameRowStringIds[];
extern u8 g_MenuLoadGameRowXOffsets[];
extern u8 g_MenuSoftResetEnabled;
void MenuHidePointerCursors(void);
void func_801D2D38(void);
void MenuSetupScreen(void);
extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);
extern void ArchiveCdDataSync(int mode);
extern void ArchiveReadFileToBuffer(s32 index, void* pBuffer, u32 arg2, u32 flags);

extern u8 D_801E977A;
extern u8 D_801EA8FC;
extern u8 D_801E96A4;
extern u8 g_MenuSaveSlotForceDiscOne;

extern u8 g_ShopIndex;
extern u8 g_GameDiscChangeRequest;
extern u8 g_MenuSavedMainMenuChoice;
void func_801D29A8(s32 arg0, s32 arg1);

INCLUDE_ASM("asm/menu/nonmatchings/main/misc1", func_801C531C);

void MenuNormalMenuMain(void) {
    u_char shouldContinue;
    u_char isHandled;

    shouldContinue = 1;
    do {
        MenuRenderFrame();
        switch (g_Menu->input) {
            case 4:
                isHandled = 1;
                if (g_Menu->mainMenuChoice == 2 && g_Menu->gearCharacterCount == 0) {
                    isHandled = 0;
                    MenuPlaySoundEffect(4);
                }
                if (isHandled) {
                    g_Menu->pSelectionMenu->isDisabled = 1;
                    MenuResetSelectionCursorFlags();
                    MenuClearBytes(8, (u8*)g_Menu->pManager + 0xC);
                    shouldContinue = func_801C531C(0);
                }
                break;
            case 5:
                shouldContinue = 0;
                break;
            case 1:
                if (g_Menu->mainMenuChoice != 0) {
                    g_Menu->mainMenuChoice--;
                } else {
                    g_Menu->mainMenuChoice = 6;
                }
                break;
            case 3:
                if (++g_Menu->mainMenuChoice >= 7) {
                    g_Menu->mainMenuChoice = 0;
                }
                break;
        }
        if (g_Menu->mainMenuChoice != g_Menu->mainMenuPrevChoice) {
            MenuSetSelectionMenuPrims(7, g_Menu->mainMenuChoice, (s32*)g_MenuMainSelectionResourceIds);
            MenuSetRowQuadVertices(8, g_Menu->optionStrings, g_MenuMainRowStringIds, g_MenuMainRowXOffsets,
                (u8*)g_Menu->pManager + 0xC, g_Menu->mainMenuChoice, 0, 0);
            g_Menu->mainMenuPrevChoice = g_Menu->mainMenuChoice;
        }
    } while (shouldContinue);
}
void MenuCdChangeMain(void) {
    u_char shouldReset;

    g_ShopIndex = 1;
    D_801E977A = 0;
    MenuStartOpenAnimation();
    shouldReset = 1;
    while (g_Menu->transitionEffectState != 0) {
        MenuRenderFrame();
    }
    MenuSetupPointerCursors(0);
    if ((u_char)MenuLoadWindow2Text(0x7D, 0xFF, 1) != 0 || D_801EA8FC != 0) {
        if (MenuLoadWindow2Text(0x80, 0xFF, 1) != 0) {
            shouldReset = 0;
        }
    }
    MenuHidePointerCursors();
    if (shouldReset) {
        g_MenuSaveSlotForceDiscOne = 1;
        D_801E96A4 = 1;
        func_801C531C(0);
        D_801E96A4 = 0;
        g_MenuSaveSlotForceDiscOne = 0;
    }
    g_Menu->pManager->shouldRenderPointerCursor = 0;
    g_Menu->pManager->unk3 = 0;
    MenuWaitForDiscChange(g_ShopIndex);
}

void MenuLoadGameMain(void) {
    u_char shouldContinue;

    shouldContinue = 1;
    MenuRevealSelectionMenu(4, g_MenuLoadGameSelectionResourceIds);
    MenuLoadStringsFromIds(8, (u8*)g_Menu + 0x6E0, g_MenuLoadGameRowStringIds, (u8*)g_Menu->pManager + 0xC);
    g_Menu->idleFrameCount = 0;
    do {
        MenuRenderFrame();
        switch (g_Menu->input) {
            case 4:
                g_Menu->pSelectionMenu->isDisabled = 1;
                MenuResetSelectionCursorFlags();
                MenuClearBytes(8, (u8*)g_Menu->pManager + 0xC);
                g_Menu->pCursorPrims->shadeWidth = 0x40;
                shouldContinue = func_801C531C(7);
                g_MenuSoftResetEnabled = 0;
                g_Menu->idleFrameCount = 0;
                break;
            case 1:
                if (g_Menu->mainMenuChoice != 0) {
                    g_Menu->mainMenuChoice--;
                } else {
                    g_Menu->mainMenuChoice = 2;
                }
                g_Menu->idleFrameCount = 0;
                break;
            case 3:
                if (++g_Menu->mainMenuChoice >= 3) {
                    g_Menu->mainMenuChoice = 0;
                }
                g_Menu->idleFrameCount = 0;
                break;
        }
        if (g_Menu->mainMenuChoice != g_Menu->mainMenuPrevChoice) {
            MenuSetSelectionMenuPrims(3, g_Menu->mainMenuChoice, (s32*)g_MenuLoadGameSelectionResourceIds);
            MenuSetRowQuadVertices(8, g_Menu->optionStrings, g_MenuLoadGameRowStringIds, g_MenuLoadGameRowXOffsets,
                (u8*)g_Menu->pManager + 0xC, g_Menu->mainMenuChoice, 0, 0);
            g_Menu->mainMenuPrevChoice = g_Menu->mainMenuChoice;
        }
        if (ArchiveGetDiscNumber() == 1 && g_Menu->idleFrameCount >= 0x259) {
            shouldContinue = 0;
            g_GameDiscChangeRequest = 1;
        }
    } while (shouldContinue);
    MenuClearBytes(8, (u8*)g_Menu->pManager + 0xC);
}

void MenuMenuUnk2Manager(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->menuUnk2 = HeapAlloc(sizeof(MenuUnk2), 0);
        bzero(g_Menu->menuUnk2, sizeof(MenuUnk2));
        return;
    }
    HeapFree(g_Menu->menuUnk2);
}

void MenuMenuManagerManager(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->pManager = HeapAlloc(sizeof(MenuManager), 0);
        bzero(g_Menu->pManager, sizeof(MenuManager));
        return;
    }
    HeapFree(g_Menu->pManager);
}

void MenuSelectionMenuManager(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->pSelectionMenu = HeapAlloc(sizeof(MenuSelectionMenu), 0);
        bzero(g_Menu->pSelectionMenu, sizeof(MenuSelectionMenu));
        return;
    }
    HeapFree(g_Menu->pSelectionMenu);
}

void MenuMenuUnk5Manager(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->pSubmenuPrims = HeapAlloc(sizeof(MenuUnk5), 0);
        bzero(g_Menu->pSubmenuPrims, sizeof(MenuUnk5));
        return;
    }
    HeapFree(g_Menu->pSubmenuPrims);
}

void MenuDressingRoomManager(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->pDressingRoom = HeapAlloc(sizeof(MenuDressingRoom), 0);
        bzero(g_Menu->pDressingRoom, sizeof(MenuDressingRoom));
        return;
    }
    HeapFree(g_Menu->pDressingRoom);
}

void MenuUnk340Manager(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->pGoldWindow = HeapAlloc(0x328, 0);
        bzero(g_Menu->pGoldWindow, 0x328);
        return;
    }
    HeapFree(g_Menu->pGoldWindow);
}

void MenuUnk344Manager(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->pPlayTimeDigits = HeapAlloc(0x374, 0);
        bzero(g_Menu->pPlayTimeDigits, 0x374);
        return;
    }
    HeapFree(g_Menu->pPlayTimeDigits);
}

void MenuMenuUnk1Manager(u_char isInitialization) {
    if (isInitialization) {
        g_Menu->pCursorPrims = HeapAlloc(sizeof(MenuUnk1), 0);
        bzero(g_Menu->pCursorPrims, sizeof(MenuUnk1));
        return;
    }
    HeapFree(g_Menu->pCursorPrims);
}

void MenuUnk39CManager(u_char isInitialization) {
    s32 i;

    if (isInitialization) {
        for (i = 0; i < 3; i++) {
            g_Menu->characterWindows[i] = HeapAlloc(0x127C, 0);
            bzero(g_Menu->characterWindows[i], 0x127C);
        }
        return;
    }
    for (i = 0; i < 3; i++) {
        HeapFree(g_Menu->characterWindows[i]);
    }
}

void MenuInitializeMenuData(void) {
    MenuMenuManagerManager(1);
    MenuSelectionMenuManager(1);
    MenuMenuUnk5Manager(1);
    MenuDressingRoomManager(1);
    MenuMenuUnk1Manager(1);
    g_Menu->pCursors = HeapAlloc(sizeof(MenuPointerCursors), 0);
    bzero(g_Menu->pCursors, sizeof(MenuPointerCursors));
    switch (g_MenuOverlayIndex) {
        case 0:
            MenuMenuUnk2Manager(1);
            MenuUnk340Manager(1);
            MenuUnk344Manager(1);
            MenuUnk39CManager(1);
            break;
        case 2:
        case 6:
            MenuMenuUnk2Manager(1);
            break;
    }
}
void MenuFinalizeMenuData(void) {
    switch (g_MenuOverlayIndex) {
        case 0:
            MenuResetSelectionCursorFlags();
            func_801D29A8(0, 0);
            g_Menu->pManager->playTimeDigitsActive = 0;
            g_Menu->pManager->goldDigitsActive = 0;
            if (g_ShopIndex == 0) {
                g_MenuSavedMainMenuChoice = g_Menu->mainMenuChoice;
            }
            break;
        case 2:
        case 6:
            break;
    }
    MenuRenderFrame();
    MenuRenderFrame();
    g_Menu->shouldDrawMenu = FALSE;
    MenuRenderFrame();
    do {
        MenuRenderFrame();
    } while (g_Menu->renderContext != 0);
    MenuMenuManagerManager(MENU_DATA_FREE);
    MenuSelectionMenuManager(MENU_DATA_FREE);
    MenuMenuUnk5Manager(MENU_DATA_FREE);
    MenuDressingRoomManager(MENU_DATA_FREE);
    MenuMenuUnk1Manager(MENU_DATA_FREE);
    HeapFree(g_Menu->pCursors);
    HeapFree(g_Menu->resources);
    HeapFree(g_Menu->pStringBank);
    HeapFree(g_Menu->titleStrings[0].pVramBuffer);
    if (g_MenuDebugEnabled) {
        SoundStopElementsForSoundFile(g_Menu->pSoundFile);
        MenuRenderFrame();
        SoundRemoveSedsEntry(g_Menu->pSoundFile);
        MenuRenderFrame();
        HeapFree(g_Menu->pSoundFile);
    }
    switch (g_MenuOverlayIndex) {
        case 0:
            MenuMenuUnk2Manager(MENU_DATA_FREE);
            MenuUnk340Manager(MENU_DATA_FREE);
            MenuUnk344Manager(MENU_DATA_FREE);
            MenuUnk39CManager(MENU_DATA_FREE);
            HeapFree(g_Menu->windows[0]);
            HeapFree(g_Menu->windowParameters[0]);
            HeapFree(g_Menu->windows[1]);
            HeapFree(g_Menu->windowParameters[1]);
            break;
        case 2:
        case 6:
            MenuMenuUnk2Manager(MENU_DATA_FREE);
            break;
    }
    HeapFree(g_Menu);
}
void func_801C62A8(void) {
    MenuInitializeMenuData();
    MenuSetupScreen();
    g_Menu->shouldDrawMenu = TRUE;
    g_Menu->soundEffectsEnabled = TRUE;
    switch (g_MenuOverlayIndex) {
        case 0:
            func_801D2D38();
            MenuNormalMenuMain();
            break;
        case 2:
            g_GameDiscChangeRequest = 0;
            MenuLoadGameMain();
            g_Menu->pManager->shouldRenderSelectionMenu = 0;
            g_Menu->pManager->shouldRenderPointerCursor = 0;
            g_Menu->pManager->unk3 = 0;
            switch (g_GameDiscChangeRequest) {
                case 0:
                    MenuWaitForDiscChange(0);
                    break;
                case 2:
                    MenuWaitForDiscChange(((u8*)g_GameState.fieldScriptMemory)[0xA4]);
                    break;
            }
            break;
        case 6:
            MenuCdChangeMain();
            break;
    }
    MenuFinalizeMenuData();
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc1", func_801C6400);

/* Best: 90.45% (480 bytes, same size).  Remaining: gcc rotates the saved registers
   (pBuffer/i/lineCount are s2/s0/s1 here vs s0/s1/s2 in the original) and puts the
   newline block in-line where the original keeps it after a `j` back to the loop top.
   Tried: per-statement re-deref of g_Menu->menuUnk2 (needed, the original reloads),
   `while` vs `if (n) for(;;)` vs do-while loop shapes, explicit pointer for the
   0x4FFC copy loop (needed, the original strength-reduces), declaration permutations.
void func_801C6400(void) {
   u8* pBuffer;
   s32 i;
   s32 j;
   u16 lineCount;
   u8* p;
   MenuUnk2* pInfo;

   for (i = 0; i < 2; i++) {
       g_Menu->menuUnk2->cardEnumerated[i] = 0;
       g_Menu->menuUnk2->cardSlotCounts[i] = 0;
       ((u8*)g_Menu->menuUnk2->previousCardSlotCounts)[i] = 0xFF;
   }
   for (i = 0; i < 0x20; i++) {
       g_Menu->menuUnk2->records[i].unk58 = 0;
       g_Menu->menuUnk2->slotRecordIndexes[i] = 0xFF;
   }
   lineCount = g_GameState.fieldScriptMemory[0];
   i = 0;
   ArchiveSetIndex(0x10, 1);
   pBuffer = HeapAlloc(ArchiveDecodeAlignedSize(1), 1);
   ArchiveReadFileToBuffer(1, pBuffer, 0, 0x80);
   ArchiveCdDataSync(0);
   if (lineCount != 0) {
       for (;;) {
           if (pBuffer[i] >= 0x80) {
               i += 2;
               continue;
           }
           if (pBuffer[i] == '\n') {
               lineCount += 0xFFFF;
               if (lineCount != 0) {
                   i++;
                   continue;
               }
               break;
           }
           i++;
       }
   }
   p = &pBuffer[i];
   for (j = 0; j < 0x1E; j++) {
       g_Menu->menuUnk2->saveTitleSuffix[j] = p[j];
   }
   pInfo = g_Menu->menuUnk2;
   pInfo->saveTitleSuffix[0x1F] = 0;
   pInfo->saveTitleSuffix[0x1E] = 0;
   ArchiveSetIndex(0x10, 0);
   HeapFree(pBuffer);
}
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc1", func_801C65F4);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc1", func_801C6AA0);

void MenuResetRenderContext(void) {
    g_Menu->renderContext = 0;
}

void MenuResetAnimationCounters(void) {
    g_Menu->characterCellFrame = 0;
    g_Menu->characterCellTimer = 0;
    g_Menu->cardSlotState = 0;
    g_Menu->characterCellShadeReversing = 0;
    g_Menu->characterCellShade = 0;
}

void MenuFillVramRectWhite(void) {
    RECT rect;
    u_short* pVramData;
    u8 _unused[0x8];

    pVramData = HeapAlloc(0x20, 0);
    bzero(pVramData, 0x20);
    pVramData[1] = 0x7FFF;
    rect.y = 448;
    rect.w = 16;
    rect.x = 0;
    rect.h = 1;
    LoadImage(&rect, pVramData);
    DrawSync(0);
    HeapFree(pVramData);
}

void MenuInitializeMenuStrings(void) {
    SystemTransferPaletteToVRAM(0, 0x1D1);
    g_Menu->titleStrings[0].pVramBuffer = HeapAlloc(0x38E, 0);
    MenuUploadStringTextures(g_Menu->titleStrings, g_MenuMainTitleStringIds, 0, 4);
    MenuFillVramRectWhite();
}
void MenuInitializeWindowBorders(void) {
    POLY_FT4 _unusued;

    ResourceHelperGetTexCoords(
        g_Menu->resources, MENU_TEX_WINDOW_BORDER_TOP,
        &g_Menu->texCount0,
        &g_Menu->texPage0,
        &g_Menu->clutX0, &g_Menu->clutY0,
        &g_Menu->texPageX0, &g_Menu->texPageY0
    );
    ResourceHelperGetTexCoords(
        g_Menu->resources, MENU_TEX_WINDOW_BORDER_BOTTOM,
        &g_Menu->texCount1,
        &g_Menu->texPage1,
        &g_Menu->clutX1, &g_Menu->clutY1,
        &g_Menu->texPageX1, &g_Menu->texPageY1
    );
    ResourceHelperGetTexCoords(
        g_Menu->resources, MENU_TEX_WINDOW_BORDER_LEFT,
        &g_Menu->texCount2,
        &g_Menu->texPage2,
        &g_Menu->clutX2, &g_Menu->clutY2,
        &g_Menu->texPageX2, &g_Menu->texPageY2
    );
    ResourceHelperGetTexCoords(
        g_Menu->resources, MENU_TEX_WINDOW_BORDER_RIGHT,
        &g_Menu->texCount3,
        &g_Menu->texPage3,
        &g_Menu->clutX3, &g_Menu->clutY3,
        &g_Menu->texPageX3, &g_Menu->texPageY3
    );
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc1", func_801C6F70);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc1", func_801C72BC);
