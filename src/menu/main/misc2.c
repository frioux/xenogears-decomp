#include "common.h"

#include "psyq/libapi.h"
#include "psyq/libgte.h"
#include "psyq/libgpu.h"
#include "system/controller.h"
#include "system/menu.h"

// Size of the selection menu's framebuffer rect; MenuRenderFrame copies it to the other
// display buffer one MENU_SCREEN_HEIGHT further down in VRAM.
#define MENU_SCREEN_WIDTH 320
#define MENU_SCREEN_HEIGHT 224
// MENU_SCREEN_VRAM_X/Y (where the rect's image sits in VRAM) are in system/menu.h
// MENU_CURSOR_SHADE_WIDTH (the shaded rectangle behind the cursor) is in system/menu.h
// Frames a card operation is given to settle, counted in frames or in vsyncs.
#define MENU_CARD_OPERATION_WAIT_FRAMES 59
// Frames MenuWaitForDiscChange renders once the disc change prompt is under way
#define MENU_DISC_CHANGE_WAIT_FRAMES 29
// Menu sound effect ids; MENU_SOUND_CONFIRM and MENU_SOUND_ERROR are in system/menu.h
// Mapped inputs; the rest of the MENU_INPUT_* group is in system/menu.h

extern u16 g_MenuBitMasksLsbFirst[]; // Bit masks, LSB first
extern u16 g_MenuBitMasksMsbFirst[]; // Bit masks, MSB first
extern u32 g_MenuBitMasksLsbFirst32[];
extern s32 g_MenuCardEventErrorCodes[]; // Card result to error code
extern u8 g_MenuCardCheckInterval;
extern s32 g_MenuCardSlotTotals[];
extern u8 g_MenuOverlayIndex; // Index of the menu overlay the menu was entered with

s32 MenuWaitForCardEvent(void);
extern void SoundPlaySoundEffectOnLastPair(s32 arg0);
extern int Vsync(int mode);

s32 MenuCheckCardSlot(u8 index);
extern void MenuPollInput(void);
extern void MenuUpdateTransitionEffect(void);
extern void MenuUpdatePlayTimeDigits(void);
extern void MenuAddScreenPrims(void);
extern void MenuUpdateCardStatus(void);
extern void MenuRunCardSlotOperations(void);
extern void GameRandomByteRange(s32 arg0, u8 arg1);
extern void GameCheckAndHandleSoftReset(void);
extern s32* g_DebuggerState; // -1 when no debugger is attached
extern int g_SpuChannelMask;
extern u8 g_MenuSoftResetEnabled;
extern s32 ArchiveGetDiscNumber(void);
extern int ControllerGetType(int controllerIndex);
extern int ControllerPopState(void);
extern void ControllerResetState(void);
extern void SoundEnableAllSpuChannels(void);
extern void SoundMuteAllSpuChannels(void);
extern int ControllerIsStateStackFull(void);
void MenuStartOpenAnimation(void);
void MenuSetupPointerCursors(s32 arg0);
void MenuHidePointerCursors(void);
s32 MenuCreateWindow2Strings(u8 arg0);
s32 MenuFreeWindow2Strings(void);
void MenuPauseCdDrive(void);
s32 func_801E93A0(s32 arg0);
void func_801C6400(void);
void MenuBuildAvailableCharacters(void);
void MenuResetRenderContext(void);
void MenuResetAnimationCounters(void);
void MenuInitializeMenuStrings(void);
void MenuInitializeWindowBorders(void);
void MenuInitializeCursorPrims(void);

void MenuSetupScreen(void) {
    g_Menu->pSelectionMenu->vramRect.x = MENU_SCREEN_VRAM_X;
    g_Menu->pSelectionMenu->vramRect.y = MENU_SCREEN_VRAM_Y;
    g_Menu->pSelectionMenu->vramRect.w = MENU_SCREEN_WIDTH;
    g_Menu->pSelectionMenu->vramRect.h = MENU_SCREEN_HEIGHT;
    g_Menu->pCursorPrims->shadeWidth = MENU_CURSOR_SHADE_WIDTH;
    MenuBuildAvailableCharacters();
    MenuResetRenderContext();
    MenuInitializeMenuStrings();
    MenuInitializeCursorPrims();
    MenuInitializeWindowBorders();
    switch (g_MenuOverlayIndex) {
    case 2:
        g_Menu->pCursorPrims->shadeWidth = 0x4C;
        /* fallthrough */
    case 0:
    case 6:
        func_801C6400();
        MenuResetAnimationCounters();
        break;
    }
}
void MenuRenderFrame(void) {
    GfxEnvironment* pNextGfxEnv;
    int renderContext;

    // Debugging
    if (*g_DebuggerState != -1) {
        asm("break 0x400");
    }

    MenuPollInput();

    if (g_MenuSoftResetEnabled) {
        GameCheckAndHandleSoftReset();
    }

    pNextGfxEnv = &g_Menu->gfxEnvs[0];
    if (g_Menu->pGfxEnv == &g_Menu->gfxEnvs[0]) {
        pNextGfxEnv = &g_Menu->gfxEnvs[1];
    }
    g_Menu->pGfxEnv = pNextGfxEnv;

    g_Menu->renderContext = g_Menu->renderContext == 0;
    ClearOTagR(g_Menu->pGfxEnv->ot, 0x10);
    GameRandomByteRange(0, 0xFF);
    MenuUpdateTransitionEffect();
    g_Menu->idleFrameCount++;
    MenuSetPlayTimeDigits(g_SpuChannelMask);
    MenuUpdatePlayTimeDigits();
    MenuAddScreenPrims();
    renderContext = g_Menu->renderContext == 0;
    DrawSync(0);
    Vsync(0);
    PutDrawEnv(&g_Menu->pGfxEnv->drawEnv);
    PutDispEnv(&g_Menu->pGfxEnv->dispEnv);
    MoveImage(&g_Menu->pSelectionMenu->vramRect, 0, renderContext * MENU_SCREEN_HEIGHT);
    DrawOTag(&g_Menu->pGfxEnv->ot[0xF]);
    MenuUpdateCardStatus();
    MenuRunCardSlotOperations();
}
void MenuPollInput(void) {
    u8 wasControllerUnplugged;
    u8 isLooping;
    s32 savedVolume;
    u8 input;

    isLooping = 1;
    wasControllerUnplugged = 0;
    while (isLooping) {
        // Is the main controller not plugged in?
        if (ControllerGetType(0) == CONTROLLER_TYPE_NONE) {
            if (wasControllerUnplugged == 0) {
                wasControllerUnplugged++;
                SoundMuteAllSpuChannels();
                savedVolume = g_SpuChannelMask;
            }
            continue;
        }

        isLooping--;
        if (wasControllerUnplugged) {
            SoundEnableAllSpuChannels();
            g_SpuChannelMask = savedVolume;
        }
    }

    input = MENU_INPUT_IDLE;
    if (ControllerIsStateStackFull()) {
        ControllerResetState();
    } else {
        while (ControllerPopState()) {
            if (g_C1ButtonStatePressedOnce & CTRL_BTN_RIGHT) {
                input = MENU_INPUT_RIGHT;
                MenuPlaySoundEffect(MENU_SOUND_CURSOR);
                break;
            }
            if (g_C1ButtonStatePressedOnce & CTRL_BTN_DOWN) {
                input = MENU_INPUT_DOWN;
                MenuPlaySoundEffect(MENU_SOUND_CURSOR);
                break;
            }
            if (g_C1ButtonStatePressedOnce & CTRL_BTN_LEFT) {
                input = MENU_INPUT_LEFT;
                MenuPlaySoundEffect(MENU_SOUND_CURSOR);
                break;
            }
            if (g_C1ButtonStatePressedOnce & CTRL_BTN_UP) {
                input = MENU_INPUT_UP;
                MenuPlaySoundEffect(MENU_SOUND_CURSOR);
                break;
            }
            if (g_C1ButtonStateReleased & CTRL_BTN_CIRCLE) {
                input = MENU_INPUT_CONFIRM;
                MenuPlaySoundEffect(MENU_SOUND_CONFIRM);
                break;
            }
            if (g_C1ButtonStateReleased & CTRL_BTN_CROSS) {
                input = MENU_INPUT_BACK;
                MenuPlaySoundEffect(MENU_SOUND_CANCEL);
                break;
            }
            if (g_C1ButtonStateReleased & CTRL_BTN_SQUARE) {
                input = MENU_INPUT_SQUARE;
                break;
            }
            if (g_C1ButtonStateReleased & CTRL_BTN_TRIANGLE) {
                input = MENU_INPUT_TRIANGLE;
                break;
            }
            if (g_C1ButtonStatePressedOnce & CTRL_BTN_L1) {
                input = MENU_INPUT_PREV;
                MenuPlaySoundEffect(MENU_SOUND_CURSOR);
                break;
            }
            if (g_C1ButtonStatePressedOnce & CTRL_BTN_R1) {
                input = MENU_INPUT_NEXT;
                MenuPlaySoundEffect(MENU_SOUND_CURSOR);
                break;
            }
            if (g_C1ButtonStateReleased & CTRL_BTN_SELECT) {
                input = MENU_INPUT_SELECT;
                break;
            }
        }
    }
    g_Menu->input = input;
}
void MenuSetPlayTimeDigits(u32 value) {
    ((u32*)g_Menu->playTimeDigitIds)[0] = value / 21600000;
    value %= 21600000;
    ((u32*)g_Menu->playTimeDigitIds)[1] = value / 2160000;
    value %= 2160000;
    ((u32*)g_Menu->playTimeDigitIds)[2] = value / 216000;
    value %= 216000;
    ((u32*)g_Menu->playTimeDigitIds)[3] = value / 36000;
    value %= 36000;
    ((u32*)g_Menu->playTimeDigitIds)[4] = value / 3600;
    value %= 3600;
    ((u32*)g_Menu->playTimeDigitIds)[5] = value / 600;
    value %= 600;
    ((u32*)g_Menu->playTimeDigitIds)[6] = value / 60;
}
void MenuParseNumberToDigits(u32 number) {
    s32 i;
    u32 divisor;

    divisor = 100000000;
    for (i = 0; i < MENU_MAX_DIGITS; i++) {
        g_Menu->digits[i] = number / divisor;
        number %= divisor;
        divisor /= 10;
    }

    for (i = 1; i < MENU_MAX_DIGITS; i++) {
        if (g_Menu->digits[i]) {
            if (g_Menu->digits[i - 1] == 0) {
                g_Menu->digits[i - 1] = MENU_DIGIT_NONE;
            }
            break;
        }
        g_Menu->digits[i - 1] = MENU_DIGIT_NONE;
    }
}
void MenuSetPolyG4Gradient(POLY_G4* pPoly, u_char r, u_char g, u_char b) {
    SetPolyG4(pPoly);
    pPoly->r0 = r;
    pPoly->g0 = g;
    pPoly->b0 = b;
    pPoly->r1 = r;
    pPoly->g1 = g;
    pPoly->b1 = b;
    pPoly->r2 = 0;
    pPoly->g2 = 0;
    pPoly->b2 = 0;
    pPoly->r3 = 0;
    pPoly->g3 = 0;
    pPoly->b3 = 0;
}
// A window's tween state, one per window: &((MenuWindowTweenTable*)g_Menu)->tweens[index].
// misc6.c and misc7.c declare the same view of the three 0x24-byte records.
typedef struct {
    /* 0x00 */ s32 startX; // the window's x at offsetX == 0
    /* 0x04 */ s32 endX; // where the tween stops
    /* 0x08 */ s32 startY;
    /* 0x0C */ s32 endY;
    /* 0x10 */ s32 stepX; // added to offsetX per step, in 8.8 fixed point
    /* 0x14 */ s32 stepY;
    /* 0x18 */ s32 offsetX; // x offset from startX, in 8.8 fixed point
    /* 0x1C */ s32 offsetY; // y offset from startY, in 8.8 fixed point
    /* 0x20 */ u8 isDecreasingX; // set when endX < startX
    /* 0x21 */ u8 isDecreasingY; // set when endY < startY
    /* 0x22 */ u8 stepCount; // steps taken per update
    /* 0x23 */ u8 isComplete; // set once the offset reaches its end
} MenuWindowTween; // Size: 0x24

// g_Menu's head is the three tweens; this view keeps the array a real member so the
// index scales against g_Menu itself.
typedef struct {
    /* 0x00 */ MenuWindowTween tweens[3];
} MenuWindowTweenTable;

// A window's tween record. Every access re-forms the address, which is what retail does:
// a store through it may alias g_Menu, so g_Menu is reloaded per statement.
#define MENU_WINDOW_TWEEN(i) (((MenuWindowTweenTable*)g_Menu)->tweens[i])

void MenuStartWindowTween(s32 startX, s32 startY, s32 endX, s32 endY, u_char stepCount, u_char index) {
    s32 deltaX;
    s32 deltaY;
    MenuWindowTween* pTween;

    pTween = ((MenuWindowTweenTable*)g_Menu)->tweens;
    pTween += index;
    pTween->startX = startX;
    pTween->startY = startY;
    pTween->endX = endX;
    pTween->endY = endY;
    if (endX < startX) {
        deltaX = startX - endX;
        pTween->isDecreasingX = 1;
    } else {
        deltaX = endX - startX;
        pTween->isDecreasingX = 0;
    }
    if (endY < startY) {
        deltaY = startY - endY;
        MENU_WINDOW_TWEEN(index).isDecreasingY = 1;
    } else {
        deltaY = endY - startY;
        MENU_WINDOW_TWEEN(index).isDecreasingY = 0;
    }
    if (deltaX >= deltaY) {
        MENU_WINDOW_TWEEN(index).stepX = 0x100;
        MENU_WINDOW_TWEEN(index).stepY = (deltaY << 8) / deltaX;
    } else {
        MENU_WINDOW_TWEEN(index).stepY = 0x100;
        MENU_WINDOW_TWEEN(index).stepX = (deltaX << 8) / deltaY;
    }
    MENU_WINDOW_TWEEN(index).stepCount = stepCount;
    MENU_WINDOW_TWEEN(index).offsetX = 0;
    MENU_WINDOW_TWEEN(index).offsetY = 0;
    MENU_WINDOW_TWEEN(index).isComplete = 0;
}
void MenuUpdateWindowTween(u_char index) {
    s32 i;

    for (i = 0; i < MENU_WINDOW_TWEEN(index).stepCount; i++) {
        if (MENU_WINDOW_TWEEN(index).isDecreasingX) {
            MENU_WINDOW_TWEEN(index).offsetX -= MENU_WINDOW_TWEEN(index).stepX;
        } else {
            MENU_WINDOW_TWEEN(index).offsetX += MENU_WINDOW_TWEEN(index).stepX;
        }
        if (MENU_WINDOW_TWEEN(index).isDecreasingY) {
            MENU_WINDOW_TWEEN(index).offsetY -= MENU_WINDOW_TWEEN(index).stepY;
        } else {
            MENU_WINDOW_TWEEN(index).offsetY += MENU_WINDOW_TWEEN(index).stepY;
        }
    }
    // stepX is 1.0 in 8.8 fixed point when the x delta is the smaller one, and then the
    // y offset is the one that has to be rounded back to whole pixels.
    if (MENU_WINDOW_TWEEN(index).stepX == 0x100) {
        if (MENU_WINDOW_TWEEN(index).isDecreasingX) {
            if (MENU_WINDOW_TWEEN(index).startX + MENU_WINDOW_TWEEN(index).offsetX / 256 < MENU_WINDOW_TWEEN(index).endX) {
                MENU_WINDOW_TWEEN(index).isComplete = 1;
            }
        } else if (MENU_WINDOW_TWEEN(index).endX < MENU_WINDOW_TWEEN(index).startX + MENU_WINDOW_TWEEN(index).offsetX / 256) {
            MENU_WINDOW_TWEEN(index).isComplete = 1;
        }
    } else {
        if (MENU_WINDOW_TWEEN(index).isDecreasingY) {
            if (MENU_WINDOW_TWEEN(index).startY + MENU_WINDOW_TWEEN(index).offsetY / 256 < MENU_WINDOW_TWEEN(index).endY) {
                MENU_WINDOW_TWEEN(index).isComplete = 1;
            }
        } else if (MENU_WINDOW_TWEEN(index).endY < MENU_WINDOW_TWEEN(index).startY + MENU_WINDOW_TWEEN(index).offsetY / 256) {
            MENU_WINDOW_TWEEN(index).isComplete = 1;
        }
    }
}
void MenuSetQuadVertices(SVECTOR* pVertices, u16 x, u16 y, u16 width, u16 height) {
    pVertices[0].vx = x + 0xFF60;
    pVertices[0].vy = y + 0xFF90;
    pVertices[0].vz = 0;
    pVertices[1].vx = x + width - 0xA0;
    pVertices[1].vy = y + 0xFF90;
    pVertices[1].vz = 0;
    pVertices[2].vx = x + 0xFF60;
    pVertices[2].vy = y + height - 0x70;
    pVertices[2].vz = 0;
    pVertices[3].vx = x + width - 0xA0;
    pVertices[3].vy = y + height - 0x70;
    pVertices[3].vz = 0;
}
void MenuPlaySoundEffect(u_char soundEffectId) {
    if (g_Menu->soundEffectsEnabled) {
        SoundPlaySoundEffectOnLastPair((g_Menu->pSoundFile->sedId << 16) | soundEffectId);
    }
}
u16 MenuGetBitMaskMsbFirst(u8 index) {
    return g_MenuBitMasksMsbFirst[index];
}
u16 MenuGetBitMaskLsbFirst(u8 index) {
    return g_MenuBitMasksLsbFirst[index];
}
u16 MenuGetInvertedBitMaskMsbFirst(u8 index) {
    return ~g_MenuBitMasksMsbFirst[index];
}
u16 MenuGetInvertedBitMaskLsbFirst(u8 index) {
    return ~g_MenuBitMasksLsbFirst[index];
}
u32 MenuAndBitMaskMsbFirst(u32 mask, u8 index) {
    return g_MenuBitMasksMsbFirst[index] & mask;
}
u32 MenuAndBitMaskLsbFirst(u32 mask, u8 index) {
    return g_MenuBitMasksLsbFirst[index] & mask;
}
u32 MenuAndBitMaskLsbFirst32(u32 mask, u8 index) {
    return mask & g_MenuBitMasksLsbFirst32[index];
}
void MenuWaitForDiscChange(u8 index) {
    u_char flag;
    u_char counter;

    MenuStartOpenAnimation();
    flag = 1;
    while (g_Menu->transitionEffectState) {
        MenuRenderFrame();
    }
    MenuSetupPointerCursors(0);
    while (flag) {
        if (ArchiveGetDiscNumber() == index + 1) {
            flag = 0;
        } else {
            MenuPauseCdDrive();
            MenuCreateWindow2Strings(index * 3 - 0x7D);
            if (func_801E93A0(index + 1)) {
                MenuFreeWindow2Strings();
                MenuCreateWindow2Strings(0x89);
                for (counter = MENU_DISC_CHANGE_WAIT_FRAMES; counter != 0; counter--) {
                    MenuRenderFrame();
                }
                MenuFreeWindow2Strings();
                MenuRenderFrame();
            } else {
                MenuFreeWindow2Strings();
                flag = 0;
            }
        }
    }
    MenuHidePointerCursors();
}
void MenuClearCardEvents(void) {
    UnDeliverEvent(0xF4000001, 0x4);
    UnDeliverEvent(0xF4000001, 0x8000);
    UnDeliverEvent(0xF4000001, 0x100);
    UnDeliverEvent(0xF4000001, 0x2000);
}
s32 MenuWaitForCardEvent(void) {
    while (1) {
        if (TestEvent(g_Menu->menuUnk2->cardEvents[3]) == 1) {
            MenuClearCardEvents();
            return 3;
        }
        if (TestEvent(g_Menu->menuUnk2->cardEvents[1]) == 1) {
            MenuClearCardEvents();
            return 1;
        }
        if (TestEvent(g_Menu->menuUnk2->cardEvents[0]) == 1) {
            MenuClearCardEvents();
            return 0;
        }
        if (TestEvent(g_Menu->menuUnk2->cardEvents[2]) == 1) {
            MenuClearCardEvents();
            return 2;
        }
    }
}
s32 MenuGetCardErrorCode(long chan) {
    if (_card_info(chan)) {
        return g_MenuCardEventErrorCodes[MenuWaitForCardEvent() & 0xFF];
    }
    return -1;
}
void MenuCloseCardEvents(void) {
    MenuRenderFrame();
    EnterCriticalSection();
    CloseEvent(g_Menu->menuUnk2->cardEvents[0]);
    CloseEvent(g_Menu->menuUnk2->cardEvents[1]);
    CloseEvent(g_Menu->menuUnk2->cardEvents[2]);
    CloseEvent(g_Menu->menuUnk2->cardEvents[3]);
    ExitCriticalSection();
}
s32 MenuCheckCardSlot(u8 index) {
    s32 cardResult;
    s32 result;
    s32 i;

    result = 1;
    g_Menu->menuUnk2->cardStatus[index] = result;
    cardResult = MenuGetCardErrorCode((index != 0) << 4);
    if (cardResult == 0 && g_Menu->menuUnk2->cardErrorCodes[index] == -1) {
        cardResult = 1;
        g_Menu->menuUnk2->cardErrorCodes[index] = 0;
    } else {
        g_Menu->menuUnk2->cardErrorCodes[index] = cardResult;
    }
    if (cardResult == -1) {
        ((u8*)g_Menu->menuUnk2 + index)[0x4F8A] = 0;
        g_Menu->menuUnk2->cardStatus[index] = 0;
        g_MenuCardSlotTotals[index] = 0;
        for (i = 0; i < MENU_CARD_RECORD_COUNT; i++) {
            ((u8*)g_Menu->menuUnk2 + (index * MENU_CARD_RECORD_COUNT + i))[0x4FAE] = MENU_SLOT_NONE;
            ((u8*)g_Menu->menuUnk2 + (index * MENU_CARD_RECORD_COUNT + i))[0x4F8E] = 0;
            g_Menu->menuUnk2->records[index * MENU_CARD_RECORD_COUNT + i].unk58 = 0;
        }
    }
    if (cardResult == -2) {
        result = 0;
    }
    if (g_Menu->menuUnk2->previousCardStatus[index] != g_Menu->menuUnk2->cardStatus[index]) {
        ((u8*)g_Menu->menuUnk2 + index)[0x4F88] = 0;
        g_Menu->menuUnk2->previousCardStatus[index] = g_Menu->menuUnk2->cardStatus[index];
    }
    return result;
}
void MenuUpdateCardStatus(void) {
    u_char count;

    if (g_Menu->menuUnk2->cardPollMode) {
        count = ++g_Menu->cardCheckTimer;
        if (g_MenuCardCheckInterval < count) {
            MenuCheckCardSlot(0);
            MenuCheckCardSlot(1);
            if (g_Menu->menuUnk2->cardErrorCodes[0] == -1 && g_Menu->menuUnk2->cardErrorCodes[1] == -1) {
                g_Menu->unk334 = 0;
            }
            g_Menu->cardCheckTimer = 0;
        }
    }
}
void MenuWaitCardOperationFrames(u8 index) {
    s32 i;

    if (g_Menu->menuUnk2->cardErrorCodes[index] != -2) {
        g_Menu->menuUnk2->cardPollMode = 2;
        for (i = MENU_CARD_OPERATION_WAIT_FRAMES; i != 0; i--) {
            MenuRenderFrame();
        }
        g_Menu->menuUnk2->cardPollMode = 0;
    }
}
void MenuWaitCardOperationVsyncs(u8 index) {
    s32 i;

    if (g_Menu->menuUnk2->cardErrorCodes[index] != -2) {
        for (i = MENU_CARD_OPERATION_WAIT_FRAMES; i != 0; i--) {
            Vsync(0);
        }
    }
}
