#include "common.h"

#include "main/game.h"
#include "system/menu.h"

extern u8 g_MenuOverlayIndex;
extern u_char func_801CB304(void);
extern u_char func_801CBD90(u_char arg0);
extern u_char func_801CC6D8(void);
extern u_char MenuEraseCardSaveFile(void);
extern void MenuSetupPointerCursors(s32 arg0);
extern void MenuHidePointerCursors(void);

extern void MenuResetSlotSelection(void);
extern u_char func_801C93A8(void);
extern s32 MenuIsSlotUsable(s32 arg0);
extern s32 MenuFindFirstValidSlot(s32 arg0);
extern u_char MenuLoadWindow2Text(u_char arg0, u_char arg1, u_char arg2);
extern s32 func_801CA750(s32 arg0);
extern void MenuCreateWindow2Strings(u_char arg0);
extern void MenuFreeWindow2Strings(void);
extern void MenuPlaySoundEffect(u_char soundEffectId);
extern void MenuRenderFrame(void);
extern s32 g_MenuCardSlotOrder[];
extern u8 D_801E9778;

// The memory card directory prefix a slot's filename is appended to: g_MenuCardPathPrefix0
// reads "bu00:", g_MenuCardPathPrefix1 "bu10:".
typedef struct {
    /* 0x0 */ u32 unk0;
    /* 0x4 */ s16 unk4;
} MenuCardPrefix; // Size: 0x6

extern MenuCardPrefix g_MenuCardPathPrefix0;
extern MenuCardPrefix g_MenuCardPathPrefix1;

// Reads the save file of the slot MenuResetSlotSelection/MenuFindFirstValidSlot picked
// off the card and erases it from the card. Returns the "stay in the menu" flag, and
// repeats the whole selection loop while the erase was skipped.
// path holds the "bu00:"/"bu10:" card prefix followed by the record's own id string.
u_char MenuEraseCardSaveFile(void) {
    char path[0x40];
    u_char ready = 1;
    s32 repeat = 1;
    u_char result = 0;
    s32 slot;
    s32 i;
    s32 recordIndex;
    MenuUnk2Record* pRecord;

    MenuResetSlotSelection();
    do {
        if (func_801C93A8() & 0xFF) {
            result = 1;
            break;
        }
        if (ready) {
            ready = 0;
            if (g_Menu->pManager->window2StringsActive) {
                MenuFreeWindow2Strings();
            }
            g_Menu->pCursors->shouldRender[0] = 1;
        }
        if (!MenuIsSlotUsable(0)) {
            slot = MenuFindFirstValidSlot(0);
            g_Menu->menuUnk2->selectedSlot = slot;
            if (slot == MENU_SLOT_NONE) {
                g_Menu->pCursors->shouldRender[0] = 0;
                g_Menu->pManager->characterWindowBuilt = 0;
                MenuLoadWindow2Text(0x62, MENU_WINDOW_TEXT_NONE, 0);
                break;
            }
        }
        g_Menu->pManager->shouldRenderCursors = 1;
        switch (func_801CA750(0)) {
            case 1:
                g_Menu->pCursors->shouldUpdatePosition[0] = 0;
                g_Menu->menuUnk2->cardPollMode = 0;
                if (MenuLoadWindow2Text(0x56, 0x59, 1) != 0) {
                    MenuCreateWindow2Strings(0x50);
                    g_Menu->pManager->shouldRenderCursors = 0;
                    g_Menu->pManager->characterWindowBuilt = 0;
                    g_Menu->menuUnk2->previousSelectedSlot = MENU_SLOT_NONE;
                    if (g_Menu->menuUnk2->selectedSlot < MENU_CARD_SLOT_COUNT) {
                        memcpy(path, &g_MenuCardPathPrefix0, 6);
                    } else {
                        memcpy(path, &g_MenuCardPathPrefix1, 6);
                    }
                    recordIndex = g_Menu->menuUnk2->slotRecordIndexes[g_MenuCardSlotOrder[g_Menu->menuUnk2->selectedSlot]];
                    // The byte offset first: written as &records[recordIndex] gcc commutes
                    // this sum and the menu base takes $a1 instead of the index.
                    pRecord = (MenuUnk2Record *)(recordIndex * sizeof(MenuUnk2Record) + (u32)g_Menu->menuUnk2->records);
                    strcat(path, pRecord->ids);
                    erase(path);
                    MenuFreeWindow2Strings();
                    g_Menu->soundEffectsEnabled = 1;
                    MenuPlaySoundEffect(0x34);
                    g_Menu->soundEffectsEnabled = 0;
                    MenuLoadWindow2Text(0x5C, MENU_WINDOW_TEXT_NONE, 0);
                    g_Menu->menuUnk2->cardPollMode = 2;
                    while (g_Menu->cardCheckTimer != 1) {
                        MenuRenderFrame();
                    }
                    for (i = 0; i < 0x20; i++) {
                        g_Menu->menuUnk2->slotRecordIndexes[i] = MENU_SLOT_NONE;
                        g_Menu->menuUnk2->slotHasGameSave[i] = 0;
                        g_Menu->menuUnk2->records[i].unk58 = 0;
                    }
                    g_Menu->menuUnk2->cardEnumerated[0] = 0;
                    g_Menu->menuUnk2->cardEnumerated[1] = 0;
                    g_Menu->menuUnk2->previousCardSlotCounts[0] = MENU_SLOT_NONE;
                    g_Menu->menuUnk2->previousCardSlotCounts[1] = MENU_SLOT_NONE;
                    D_801E9778 = 1;
                }
                g_Menu->pCursors->shouldUpdatePosition[0] = 1;
                // Falls through: the cursor update above is the last thing the card
                // erase arm does before the selection loop is left.
            case 2:
                repeat = 0;
                break;
        }
    } while (repeat);
    g_Menu->menuUnk2->cardPollMode = 1;
    g_Menu->unk334 = 1;
    g_Menu->soundEffectsEnabled = 1;
    return result;
}
s32 MenuUpdateSubMenuChoice(u_char arg0) {
    u_char result = 1;
    u8 choice;

    MenuSetupPointerCursors(0);
    g_Menu->pManager->shouldRenderCursors = 0;
    choice = g_Menu->subMenuChoice;
    switch (choice) {
        case 0:
            g_Menu->unk334 = 7;
            if (MenuEraseCardSaveFile()) {
                result = 0;
            }
            break;
        case 1:
            g_Menu->unk334 = 6;
            if (func_801CC6D8()) {
                result = 0;
            }
            break;
        case 2:
            if (g_MenuOverlayIndex != g_Menu->subMenuChoice) {
                g_Menu->unk334 = 3;
                if (func_801CBD90(arg0)) {
                    result = 0;
                }
            } else {
                g_Menu->unk334 = 2;
                if (func_801CB304()) {
                    result = 0;
                }
            }
            break;
    }
    MenuHidePointerCursors();
    return result;
}
