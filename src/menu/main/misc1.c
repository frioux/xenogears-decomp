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
void MenuOverlayMain(void) {
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

/* Best: 98.62% (500 bytes = retail's exact size), instruction-identical modulo the saved
   register ROTATION: retail pBuffer=s0, i=s1, lineCount=s2 against gcc's i=s0, lineCount=s1,
   pBuffer=s2. Both compiles save exactly s0-s2 plus ra, so it is a register-assignment tie,
   not a missing or extra local, and nothing below moves it.
   Fixed on the way: the `cardPollMode = 0` store was missing (5 insns, 90.45 -> 94.45), the
   newline scan must read `if (c != '\n') { i++; continue; }` (newline block out of line after
   the `j` to the loop top), and the 0xFFFF needs a local (lineCountDelta) or `li a0,0xa` and
   `li a1,0xffff` swap order (98.53).
   Tried for the rotation, all measured: declaration orders (4), separate scan index (96.75),
   separate init counter (96.75, the counter falls to v1), lineCount u32/s32 (97.82), s16
   (97.66), i u32 (97.74), s16 (73.26), lineCountDelta u32 (98.62), s16 (97.62), u8 (98.05),
   j u32 (98.14), s16 (86.49), u8 (92.73), a `u8 *pBuffer = NULL;` initializer (98.62 - the
   dead store is eliminated, the pseudo number does not move the tie), `i = i + 2`, `!= 10`,
   `pBuffer + i`, `if (lineCount)`, `(u8 *)(pBuffer + i)` (all 98.62), gcc 2.7.2 (98.62), -O3
   (98.62), cdk (94.69), permuter 3x4min (stuck at 170). Load-bearing: the scan shape above,
   lineCountDelta, the `cardPollMode = 0` store, and pInfo for the two tail stores (inline
   g_Menu->menuUnk2 there is 93.02% at 520 bytes, 5 insns too many).
void func_801C6400(void) {
   u8* pBuffer;
   s32 i;
   s32 j;
   u16 lineCount;
   s32 lineCountDelta;
   u8* p;
   MenuUnk2* pInfo;

   for (i = 0; i < 2; i++) {
       g_Menu->menuUnk2->cardEnumerated[i] = 0;
       g_Menu->menuUnk2->cardSlotCounts[i] = 0;
       g_Menu->menuUnk2->previousCardSlotCounts[i] = 0xFF;
   }
   g_Menu->menuUnk2->cardPollMode = 0;
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
       lineCountDelta = 0xFFFF;
       for (;;) {
           if (pBuffer[i] >= 0x80) {
               i += 2;
               continue;
           }
           if (pBuffer[i] != '\n') {
               i++;
               continue;
           }
           lineCount += lineCountDelta;
           i++;
           if (lineCount != 0) {
               continue;
           }
           break;
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
u_short MenuAndBitMaskLsbFirst(u_short mask, u_char index);
void func_801C65F4(void);

void MenuBuildAvailableCharacters(void) {
    u32 mask;
    s32 i;
    s32 characterId;

    if (g_MenuOverlayIndex == 0 && g_ShopIndex == 0) {
        g_Menu->mainMenuChoice = g_MenuSavedMainMenuChoice;
    } else {
        g_Menu->mainMenuChoice = 1;
    }
    g_Menu->mainMenuPrevChoice = 0xFF;
    g_Menu->cardCheckTimer = 60;
    g_Menu->unk334 = 0;
    g_Menu->unk335 = 0;
    g_Menu->availableCharacterCount = 0;
    mask = (g_GameState.characterFlags & g_GameState.FrMask) & 0x7FF;
    for (i = 0; i < 0x10; i++) {
        if (MenuAndBitMaskLsbFirst(mask, i) != 0) {
            g_Menu->availableCharacters[i] = 1;
        } else {
            g_Menu->availableCharacters[i] = 0;
        }
    }
    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        g_Menu->pManager->isGearScreenActive[i] = 0;
        characterId = g_GameState.partyMembers[i];
        if (characterId != CHARACTER_ID_NONE && g_Menu->availableCharacters[characterId] != 0) {
            g_Menu->pManager->currentCharacterIDs[i] = characterId;
            g_Menu->availableCharacterCount++;
            if (g_GameState.characters[g_Menu->pManager->currentCharacterIDs[i]].gearId != GEAR_ID_NONE) {
                g_Menu->pManager->isGearScreenActive[i] = 1;
                g_Menu->gearCharacterCount++;
            }
        } else {
            g_Menu->pManager->currentCharacterIDs[i] = CHARACTER_ID_NONE;
        }
    }
    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        if (g_Menu->pManager->currentCharacterIDs[i] != CHARACTER_ID_NONE) {
            g_Menu->characterSlot = i;
            break;
        }
    }
    func_801C65F4();
}

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
extern void MenuSetPolyG4Gradient(POLY_G4* pPoly, u_char r, u_char g, u_char b);

void func_801C6F70(void) {
    RECT rect;
    s32 i;

    rect.y = 0;
    rect.x = 0;
    rect.h = 0x100;
    rect.w = 0x100;
    MenuResetSelectionCursorFlags();
    for (i = 0; i < 2; i++) {
        MenuSetPolyG4Gradient(&g_Menu->pCursorPrims->polyG4s[i], 0x80, 0x80, 0);
        SetSemiTrans(&g_Menu->pCursorPrims->polyG4s[i], 1);
        SetLineF3(&g_Menu->pCursorPrims->lines1[i]);
        *(u8*)(i * 0x18 + (u32)g_Menu->pCursorPrims + 0xCC) = 0;
        *(u8*)(i * 0x18 + (u32)g_Menu->pCursorPrims + 0xCD) = 0x40;
        *(u8*)(i * 0x18 + (u32)g_Menu->pCursorPrims + 0xCE) = 0;
        SetLineF3(&g_Menu->pCursorPrims->lines2[i]);
        *(u8*)(i * 0x18 + (u32)g_Menu->pCursorPrims + 0xFC) = 0;
        *(u8*)(i * 0x18 + (u32)g_Menu->pCursorPrims + 0xFD) = 0x40;
        *(u8*)(i * 0x18 + (u32)g_Menu->pCursorPrims + 0xFE) = 0;
        SetPolyF4(&g_Menu->pCursorPrims->polysDimEffect[i]);
        *(s16*)(i * 0x18 + (u32)g_Menu->pCursorPrims + 0xA0) = 0;
        *(s16*)(i * 0x18 + (u32)g_Menu->pCursorPrims + 0xA2) = 0;
        *(s16*)(i * 0x18 + (u32)g_Menu->pCursorPrims + 0xA4) = 0x140;
        *(s16*)(i * 0x18 + (u32)g_Menu->pCursorPrims + 0xA6) = 0;
        *(s16*)(i * 0x18 + (u32)g_Menu->pCursorPrims + 0xA8) = 0;
        *(s16*)(i * 0x18 + (u32)g_Menu->pCursorPrims + 0xAA) = 0xE0;
        *(s16*)(i * 0x18 + (u32)g_Menu->pCursorPrims + 0xAC) = 0x140;
        *(s16*)(i * 0x18 + (u32)g_Menu->pCursorPrims + 0xAE) = 0xE0;
        *(u8*)(i * 0x18 + (u32)g_Menu->pCursorPrims + 0x9C) = 0x80;
        *(u8*)(i * 0x18 + (u32)g_Menu->pCursorPrims + 0x9D) = 0x80;
        *(u8*)(i * 0x18 + (u32)g_Menu->pCursorPrims + 0x9E) = 0x80;
        SetSemiTrans(&g_Menu->pCursorPrims->polysDimEffect[i], 1);
        SetDrawMode(&g_Menu->pCursorPrims->drModes1[i], 0, 0, GetTPage(0, 0, 0x140, 0x80), &rect);
        SetDrawMode(&g_Menu->pCursorPrims->drawModeDimEffect[i], 0, 0, GetTPage(0, 2, 0x180, 0), &rect);
    }
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc1", func_801C72BC);
