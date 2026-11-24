#include "common.h"

#include "system/menu.h"

// Frames the load-game screen waits with no input before it asks for the second disc.
#define MENU_DISC_CHANGE_IDLE_FRAMES 601

extern u8 g_MenuOverlayIndex; // Index of the menu overlay the menu was entered with
extern u_char g_MenuMainTitleStringIds[]; // String IDs of the menu strings

void MenuUploadStringTextures(MenuString* pStringList, u_char* pStringIdList, s32 yOffset, s32 count);

u_char MenuRunSelectedChoice(u_char arg0);
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

extern s32 g_MenuMainSelectionResourceIds[];
extern u8 g_MenuMainRowStringIds[];
extern u8 g_MenuMainRowXOffsets[];
extern s32 g_MenuLoadGameSelectionResourceIds[];
extern u8 g_MenuLoadGameRowStringIds[];
extern u8 g_MenuLoadGameRowXOffsets[];
extern u8 g_MenuSoftResetEnabled;
void MenuHidePointerCursors(void);
void func_801D2D38(void);
void MenuSetupScreen(void);
extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);
extern void ArchiveCdDataSync(int mode);
extern void ArchiveReadFileToBuffer(s32 index, void* pBuffer, u32 arg2, u32 flags);

extern u8 g_MenuCardChangeDetectionEnabled;
extern u8 g_MenuWindowTextBackPressed;
extern u8 D_801E96A4;
extern u8 g_MenuSaveSlotForceDiscOne;

extern u8 g_ShopIndex;
extern u8 g_GameDiscChangeRequest;
extern u8 g_MenuSavedMainMenuChoice;
void MenuTweenCharacterWindows(s32 arg0, s32 arg1);

void MenuStartCloseAnimation(void);
void MenuFreeSubMenuPanels(u8 arg0);
void MenuFreeShoulderButtonUi(void);
s32 MenuRunUnk430Panel(u_char arg0, u_char arg1);
int MenuInitializeUnk35CPanel(u8 arg0, u8 arg1);
u8 MenuRunCharacterDressingRoomLoop(void);
s32 func_801D9F98(s32 arg0, s32 arg1);
s32 func_801D9808(void);
u8 func_801E23CC(void);
s32 func_801DBE54(void);
void func_8001B970(void);

u_char MenuRunSelectedChoice(u_char arg0) {
    u_char result;
    u_char shouldContinue;

    g_MenuCardChangeDetectionEnabled = 1;
    shouldContinue = 1;
    result = 1;
    switch (g_Menu->mainMenuChoice + arg0) {
        case 7:
            result = func_801D9808();
            break;
        case 8:
            if ((u_char)func_801D9F98(1, 0) != 0) {
                g_GameDiscChangeRequest = 2;
                shouldContinue = 0;
            }
            break;
        case 1:
            result = func_801D9F98(0, D_801E96A4);
            break;
        case 2:
            result = func_801E23CC();
            break;
        case 3:
            result = MenuRunUnk430Panel(g_Menu->characterSlot, 1);
            break;
        case 4:
            result = func_801DBE54();
            break;
        case 5:
            result = MenuInitializeUnk35CPanel(g_Menu->characterSlot, 1);
            break;
        case 6:
            result = MenuRunCharacterDressingRoomLoop();
            break;
        case 9:
            func_8001B970();
            // fall through
        case 0:
            result = 0;
            shouldContinue = 0;
            break;
    }
    g_Menu->menuUnk2->cardPollMode = 0;
    if (result != 0) {
        MenuStartCloseAnimation();
        if (g_MenuOverlayIndex == 0) {
            MenuTweenCharacterWindows(1, 0);
        } else if (g_MenuOverlayIndex == 2) {
            MenuLoadStringsFromIds(8, g_Menu->optionStrings, g_MenuLoadGameRowStringIds,
                g_Menu->pManager->shouldRenderStrings);
            g_Menu->pCursorPrims->shadeWidth = 0x4C;
        }
    }
    MenuFreeSubMenuPanels(arg0);
    MenuFreeShoulderButtonUi();
    g_Menu->pSelectionMenu->isDisabled = 0;
    g_Menu->pSelectionMenu->isDisabledRendered = 1;
    g_Menu->pManager->shouldRenderPointerCursor = 1;
    g_Menu->pManager->unk3 = 1;
    g_Menu->mainMenuPrevChoice = 0xFF;
    g_Menu->pManager->shouldRenderSubmenuStrings = 0;
    g_MenuSoftResetEnabled = 1;
    return shouldContinue;
}

void MenuNormalMenuMain(void) {
    u_char shouldContinue;
    u_char isHandled;

    shouldContinue = 1;
    do {
        MenuRenderFrame();
        switch (g_Menu->input) {
            case MENU_INPUT_CONFIRM:
                isHandled = 1;
                if (g_Menu->mainMenuChoice == 2 && g_Menu->gearCharacterCount == 0) {
                    isHandled = 0;
                    MenuPlaySoundEffect(4);
                }
                if (isHandled) {
                    g_Menu->pSelectionMenu->isDisabled = 1;
                    MenuResetSelectionCursorFlags();
                    MenuClearBytes(8, g_Menu->pManager->shouldRenderStrings);
                    shouldContinue = MenuRunSelectedChoice(0);
                }
                break;
            case MENU_INPUT_BACK:
                shouldContinue = 0;
                break;
            case MENU_INPUT_DOWN:
                if (g_Menu->mainMenuChoice != 0) {
                    g_Menu->mainMenuChoice--;
                } else {
                    g_Menu->mainMenuChoice = 6;
                }
                break;
            case MENU_INPUT_UP:
                if (++g_Menu->mainMenuChoice >= 7) {
                    g_Menu->mainMenuChoice = 0;
                }
                break;
        }
        if (g_Menu->mainMenuChoice != g_Menu->mainMenuPrevChoice) {
            MenuSetSelectionMenuPrims(7, g_Menu->mainMenuChoice, g_MenuMainSelectionResourceIds);
            MenuSetRowQuadVertices(8, g_Menu->optionStrings, g_MenuMainRowStringIds, g_MenuMainRowXOffsets,
                g_Menu->pManager->shouldRenderStrings, g_Menu->mainMenuChoice, 0, 0);
            g_Menu->mainMenuPrevChoice = g_Menu->mainMenuChoice;
        }
    } while (shouldContinue);
}
void MenuCdChangeMain(void) {
    u_char shouldReset;

    g_ShopIndex = 1;
    g_MenuCardChangeDetectionEnabled = 0;
    MenuStartOpenAnimation();
    shouldReset = 1;
    while (g_Menu->transitionEffectState != 0) {
        MenuRenderFrame();
    }
    MenuSetupPointerCursors(0);
    if ((u_char)MenuLoadWindow2Text(0x7D, 0xFF, 1) != 0 || g_MenuWindowTextBackPressed != 0) {
        if (MenuLoadWindow2Text(0x80, 0xFF, 1) != 0) {
            shouldReset = 0;
        }
    }
    MenuHidePointerCursors();
    if (shouldReset) {
        g_MenuSaveSlotForceDiscOne = 1;
        D_801E96A4 = 1;
        MenuRunSelectedChoice(0);
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
    MenuLoadStringsFromIds(8, g_Menu->optionStrings, g_MenuLoadGameRowStringIds, g_Menu->pManager->shouldRenderStrings);
    g_Menu->idleFrameCount = 0;
    do {
        MenuRenderFrame();
        switch (g_Menu->input) {
            case MENU_INPUT_CONFIRM:
                g_Menu->pSelectionMenu->isDisabled = 1;
                MenuResetSelectionCursorFlags();
                MenuClearBytes(8, g_Menu->pManager->shouldRenderStrings);
                g_Menu->pCursorPrims->shadeWidth = MENU_CURSOR_SHADE_WIDTH;
                shouldContinue = MenuRunSelectedChoice(7);
                g_MenuSoftResetEnabled = 0;
                g_Menu->idleFrameCount = 0;
                break;
            case MENU_INPUT_DOWN:
                if (g_Menu->mainMenuChoice != 0) {
                    g_Menu->mainMenuChoice--;
                } else {
                    g_Menu->mainMenuChoice = 2;
                }
                g_Menu->idleFrameCount = 0;
                break;
            case MENU_INPUT_UP:
                if (++g_Menu->mainMenuChoice >= 3) {
                    g_Menu->mainMenuChoice = 0;
                }
                g_Menu->idleFrameCount = 0;
                break;
        }
        if (g_Menu->mainMenuChoice != g_Menu->mainMenuPrevChoice) {
            MenuSetSelectionMenuPrims(3, g_Menu->mainMenuChoice, g_MenuLoadGameSelectionResourceIds);
            MenuSetRowQuadVertices(8, g_Menu->optionStrings, g_MenuLoadGameRowStringIds, g_MenuLoadGameRowXOffsets,
                g_Menu->pManager->shouldRenderStrings, g_Menu->mainMenuChoice, 0, 0);
            g_Menu->mainMenuPrevChoice = g_Menu->mainMenuChoice;
        }
        if (ArchiveGetDiscNumber() == 1 && g_Menu->idleFrameCount >= MENU_DISC_CHANGE_IDLE_FRAMES) {
            shouldContinue = 0;
            g_GameDiscChangeRequest = 1;
        }
    } while (shouldContinue);
    MenuClearBytes(8, g_Menu->pManager->shouldRenderStrings);
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
        g_Menu->pPlayTimeDigits = HeapAlloc(sizeof(MenuPlayTimeDigits), 0);
        bzero(g_Menu->pPlayTimeDigits, sizeof(MenuPlayTimeDigits));
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
            g_Menu->characterWindows[i] = HeapAlloc(sizeof(MenuCharacterWindow), 0);
            bzero(g_Menu->characterWindows[i], sizeof(MenuCharacterWindow));
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
            MenuTweenCharacterWindows(0, 0);
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
                    MenuWaitForDiscChange(g_GameState.fieldScriptMemory[0x52]);
                    break;
            }
            break;
        case 6:
            MenuCdChangeMain();
            break;
    }
    MenuFinalizeMenuData();
}
/* Best: 98.62% (500 bytes = retail's exact size, 125 insns), instruction-identical except
   the three saved-register NAMES: retail pBuffer=s0, i=s1, lineCount=s2 against gcc's
   i=s0, lineCount=s1, pBuffer=s2. Re-measured round 419 (unparked, whole diff read):
   same 98.62% / 500 bytes. That round also closed the declaration-order axis - ALL SIX
   permutations of the three candidates among their declaration slots measure 98.62%, so
   pseudo numbering is not what hands out s0/s1/s2 here. Nothing else left to spell.
   REACHABILITY, MEASURED (round 396) - read this before trying another spelling. `-da` on
   build/src/menu/main/misc1.c.i gives the pre-global-alloc lreg/flow dump, whose per-pseudo
   "Register N used X times across Y insns; ... Z bytes" lines are exactly the terms of gcc
   2.6.0 flow.c's allocno priority floor_log2(refs) * refs / live_length * size, and the greg
   dump prints the resulting order ("11 regs to allocate: 119 72 73 142 146 74 71 ...") with
   pseudo 72 (i), 74 (lineCount), 71 (pBuffer) going to s0, s1, s2. The numbers: i = 5*38/70*4
   = 108570, lineCount = 3*8/34*2 = 14117, pBuffer = 2*6/42*4 = 11428. Retail needs pBuffer to
   outrank an 38-reference pseudo while holding 6, so no spelling of this body reaches it
   (same conclusion as levers.md a-global-allocno-priority-is, menu/main/misc10).
   Fixed on the way: the `cardPollMode = 0` store was missing (5 insns, 90.45 -> 94.45), the
   newline scan must read `if (c != 0x0A) { i++; continue; }` (newline block out of line after
   the `j` to the loop top), and the 0xFFFF needs a local (lineCountDelta) or `li a0,0xa` and
   `li a1,0xffff` swap order (98.53).
   Tried for the rotation, all measured: declaration orders (4), a named `u8 c = pBuffer[i]`
   temp in the scan (98.62, the pseudo count does not move the three-way order), separate scan
   index (96.75), separate init counter (96.75, the counter falls to v1), lineCount u32/s32
   (97.82), s16 (97.66), i u32 (97.74), s16 (73.26), lineCountDelta u32 (98.62), s16 (97.62),
   u8 (98.05), j u32 (98.14), s16 (86.49), u8 (92.73), a `u8 *pBuffer = NULL;` initializer
   (98.62), `i = i + 2`, `!= 10`, `pBuffer + i`, `if (lineCount)`, `(u8 *)(pBuffer + i)` (all
   98.62), gcc 2.7.2 (98.62), -O3 (98.62), cdk (94.69), permuter 3x4min (stuck at 170).
   Load-bearing: the scan shape above, lineCountDelta, the `cardPollMode = 0` store, and pInfo
   for the two tail stores (inline g_Menu->menuUnk2 there is 93.02% at 520 bytes, 5 insns too
   many).
*/
/*
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
INCLUDE_ASM("asm/menu/nonmatchings/main/misc1", func_801C6400);

#define FIELD_TEX_COUNT(_i) ((_i) * 6) + 0
#define FIELD_TEX_PAGE(_i)  ((_i) * 6) + 1
#define FIELD_CLUT_X(_i)    ((_i) * 6) + 2
#define FIELD_CLUT_Y(_i)    ((_i) * 6) + 3
#define FIELD_TPAGE_X(_i)   ((_i) * 6) + 4
#define FIELD_TPAGE_Y(_i)   ((_i) * 6) + 5

extern u32* g_MenuResourcesFile; // Relative offsets into the menu resources archive
extern SoundFile* g_MenuSedsFile;

// The game code the save file header is stamped with, "BASLUS-00664" (disc one) and
// "BASLUS-01160" (disc two); 12 characters and the terminator.
extern u32 g_MenuGameSerialCodeDiscOne[]; // "BASLUS-00664" as words
extern u32 g_MenuGameSerialCodeDiscTwo[]; // "BASLUS-01160" as words

extern void ResolveArchiveEntryPointers(u32* pFile);
extern void* LZSSHeapDecompress(void* pCompressedData, int flags);
extern void ModelLoadTextureBlock(u8* pData);
extern void SoundAddSedsEntry(SoundFile* pSoundFile);
extern void ResourceHelperGetTexCoords(MenuResourceDirectory* pDir, u32 index, s32* pDirentCount, s32* pTexPageId,
    s32* pClutX, s32* pClutY, s32* pTexPageX, s32* pTexPageY);

void MenuLoadResourceFiles(void) {
    TIM_IMAGE tim;
    s32 metadata[18];
    SoundFile* pSoundFile;
    u32* pFileHandle;
    u8* pData;
    s32 i;

    pFileHandle = g_MenuResourcesFile;
    ResolveArchiveEntryPointers(pFileHandle);
    pData = LZSSHeapDecompress((void*)pFileHandle[1], 1);
    OpenTIM((u_long*)pData);
    ReadTIM(&g_Menu->menuUnk2->tim);
    memcpy(g_Menu->menuUnk2->gameSerialCode, g_MenuGameSerialCodeDiscOne, sizeof(g_Menu->menuUnk2->gameSerialCode));
    memcpy(g_Menu->menuUnk2->discTwoGameSerialCode, g_MenuGameSerialCodeDiscTwo, sizeof(g_Menu->menuUnk2->gameSerialCode));
    g_Menu->menuUnk2->saveFileMagic[0] = 0x53;
    g_Menu->menuUnk2->saveFileMagic[1] = 0x43;
    g_Menu->menuUnk2->saveIconDisplayFlag = 0x11;
    g_Menu->menuUnk2->saveBlockCount = 1;
    bzero(g_Menu->menuUnk2->saveTitle, sizeof(g_Menu->menuUnk2->saveTitle));
    memmove(g_Menu->menuUnk2->saveIconPalette, g_Menu->menuUnk2->tim.caddr,
        sizeof(g_Menu->menuUnk2->saveIconPalette));
    memmove(g_Menu->menuUnk2->saveIconImage, g_Menu->menuUnk2->tim.paddr,
        sizeof(g_Menu->menuUnk2->saveIconImage));
    HeapFree(pData);
    pData = LZSSHeapDecompress((void*)pFileHandle[2], 1);
    ModelLoadTextureBlock(pData);
    HeapFree(pData);
    g_Menu->resources = LZSSHeapDecompress((void*)pFileHandle[3], 0);
    g_Menu->pStringBank = LZSSHeapDecompress((void*)pFileHandle[4], 0);
    ResourceHelperGetTexCoords(g_Menu->resources, MENU_TEX_STRING_DEFENSE,
        &metadata[FIELD_TEX_COUNT(0)], &metadata[FIELD_TEX_PAGE(0)],
        &metadata[FIELD_CLUT_X(0)], &metadata[FIELD_CLUT_Y(0)],
        &metadata[FIELD_TPAGE_X(0)], &metadata[FIELD_TPAGE_Y(0)]);
    ResourceHelperGetTexCoords(g_Menu->resources, MENU_TEX_PORTRAIT_PARTY_MEMBER_1,
        &metadata[FIELD_TEX_COUNT(0)], &metadata[FIELD_TEX_PAGE(0)],
        &metadata[FIELD_CLUT_X(0)], &metadata[FIELD_CLUT_Y(0)],
        &metadata[FIELD_TPAGE_X(0)], &metadata[FIELD_TPAGE_Y(0)]);
    ResourceHelperGetTexCoords(g_Menu->resources, MENU_TEX_PORTRAIT_PARTY_MEMBER_2,
        &metadata[FIELD_TEX_COUNT(1)], &metadata[FIELD_TEX_PAGE(1)],
        &metadata[FIELD_CLUT_X(1)], &metadata[FIELD_CLUT_Y(1)],
        &metadata[FIELD_TPAGE_X(1)], &metadata[FIELD_TPAGE_Y(1)]);
    ResourceHelperGetTexCoords(g_Menu->resources, MENU_TEX_PORTRAIT_PARTY_MEMBER_3,
        &metadata[FIELD_TEX_COUNT(2)], &metadata[FIELD_TEX_PAGE(2)],
        &metadata[FIELD_CLUT_X(2)], &metadata[FIELD_CLUT_Y(2)],
        &metadata[FIELD_TPAGE_X(2)], &metadata[FIELD_TPAGE_Y(2)]);
    metadata[FIELD_TPAGE_X(1)] += 0xC;
    pData = LZSSHeapDecompress((void*)pFileHandle[5], 1);
    for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
        if (g_Menu->pManager->currentCharacterIDs[i] != CHARACTER_ID_NONE) {
            OpenTIM((u_long*)&pData[g_Menu->pManager->currentCharacterIDs[i] * 0xB20]);
            ReadTIM(&tim);
            tim.crect->x = metadata[FIELD_CLUT_X(i)];
            tim.crect->y = metadata[FIELD_CLUT_Y(i)];
            tim.prect->x = metadata[FIELD_TPAGE_X(i)];
            tim.prect->y = metadata[FIELD_TPAGE_Y(i)];
            LoadImage(tim.crect, tim.caddr);
            LoadImage(tim.prect, tim.paddr);
        }
    }
    DrawSync(0);
    HeapFree(pData);
    if (g_MenuDebugEnabled) {
        ArchiveSetIndex(0x10, 2);
        pSoundFile = HeapAlloc(ArchiveDecodeAlignedSize(5), 0);
        g_MenuSedsFile = pSoundFile;
        ArchiveReadFileToBuffer(5, pSoundFile, 0, 0x80);
        ArchiveCdDataSync(0);
        ArchiveSetIndex(0x10, 0);
        SoundAddSedsEntry(g_MenuSedsFile);
    }
    g_Menu->pSoundFile = g_MenuSedsFile;
    HeapFree(pFileHandle);
}

u_short MenuAndBitMaskLsbFirst(u_short mask, u_char index);

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
    mask = (g_GameState.characterFlags & g_GameState.FrMask) & MENU_CHARACTER_FLAG_MASK;
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
    MenuLoadResourceFiles();
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
// Screen size in pixels; the dim effect quad covers the whole framebuffer.
#define MENU_SCREEN_WIDTH 320
#define MENU_SCREEN_HEIGHT 224

extern void MenuSetPolyG4Gradient(POLY_G4* pPoly, u_char r, u_char g, u_char b);

void MenuInitializeCursorPrims(void) {
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
        ((LINE_F3*)(i * sizeof(LINE_F3) + (u32)g_Menu->pCursorPrims->lines1))->r0 = 0;
        ((LINE_F3*)(i * sizeof(LINE_F3) + (u32)g_Menu->pCursorPrims->lines1))->g0 = 0x40;
        ((LINE_F3*)(i * sizeof(LINE_F3) + (u32)g_Menu->pCursorPrims->lines1))->b0 = 0;
        SetLineF3(&g_Menu->pCursorPrims->lines2[i]);
        ((LINE_F3*)(i * sizeof(LINE_F3) + (u32)g_Menu->pCursorPrims->lines2))->r0 = 0;
        ((LINE_F3*)(i * sizeof(LINE_F3) + (u32)g_Menu->pCursorPrims->lines2))->g0 = 0x40;
        ((LINE_F3*)(i * sizeof(LINE_F3) + (u32)g_Menu->pCursorPrims->lines2))->b0 = 0;
        SetPolyF4(&g_Menu->pCursorPrims->polysDimEffect[i]);
        ((POLY_F4*)(i * sizeof(POLY_F4) + (u32)g_Menu->pCursorPrims->polysDimEffect))->x0 = 0;
        ((POLY_F4*)(i * sizeof(POLY_F4) + (u32)g_Menu->pCursorPrims->polysDimEffect))->y0 = 0;
        ((POLY_F4*)(i * sizeof(POLY_F4) + (u32)g_Menu->pCursorPrims->polysDimEffect))->x1 = MENU_SCREEN_WIDTH;
        ((POLY_F4*)(i * sizeof(POLY_F4) + (u32)g_Menu->pCursorPrims->polysDimEffect))->y1 = 0;
        ((POLY_F4*)(i * sizeof(POLY_F4) + (u32)g_Menu->pCursorPrims->polysDimEffect))->x2 = 0;
        ((POLY_F4*)(i * sizeof(POLY_F4) + (u32)g_Menu->pCursorPrims->polysDimEffect))->y2 = MENU_SCREEN_HEIGHT;
        ((POLY_F4*)(i * sizeof(POLY_F4) + (u32)g_Menu->pCursorPrims->polysDimEffect))->x3 = MENU_SCREEN_WIDTH;
        ((POLY_F4*)(i * sizeof(POLY_F4) + (u32)g_Menu->pCursorPrims->polysDimEffect))->y3 = MENU_SCREEN_HEIGHT;
        ((POLY_F4*)(i * sizeof(POLY_F4) + (u32)g_Menu->pCursorPrims->polysDimEffect))->r0 = 0x80;
        ((POLY_F4*)(i * sizeof(POLY_F4) + (u32)g_Menu->pCursorPrims->polysDimEffect))->g0 = 0x80;
        ((POLY_F4*)(i * sizeof(POLY_F4) + (u32)g_Menu->pCursorPrims->polysDimEffect))->b0 = 0x80;
        SetSemiTrans(&g_Menu->pCursorPrims->polysDimEffect[i], 1);
        SetDrawMode(&g_Menu->pCursorPrims->drModes1[i], 0, 0, GetTPage(0, 0, 0x140, 0x80), &rect);
        SetDrawMode(&g_Menu->pCursorPrims->drawModeDimEffect[i], 0, 0, GetTPage(0, 2, 0x180, 0), &rect);
    }
}
