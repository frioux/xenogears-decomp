#include "common.h"

#include "main/game.h"
#include "system/memory.h"
#include "system/menu.h"

// gear hp/maxHp are 32-bit, so their window numbers have five digits
#define MENU_GEAR_HP_DIGIT_COUNT 5




void MenuParseNumberToDigits(u32 number);

// A window's tween state, one per window: &((MenuWindowTween*)g_Menu->windowTweens)[index].
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

// The gear window's prim buffer (g_Menu->pGearWindowPrims): this unit only needs the copy
// of the render context at its tail.
typedef struct {
    /* 0x0000 */ u8 unk0[0x2AE0];
    /* 0x2AE0 */ u_char renderContext;
    /* 0x2AE1 */ u8 unk2AE1;
    /* 0x2AE2 */ u8 unk2AE2;
    /* 0x2AE3 */ u8 unk2AE3[0x2];
    /* 0x2AE5 */ u8 unk2AE5; // prims the first weight-digit block drew
    /* 0x2AE6 */ u8 unk2AE6; // prims the second weight-digit block drew
    /* 0x2AE7 */ u8 hpPrimCount; // digit prims the hp number drew
    /* 0x2AE8 */ u8 maxHpPrimCount; // digit prims the maxHp number drew
    /* 0x2AE9 */ u8 unk2AE9[0x2];
    /* 0x2AEB */ u8 digitPrimCount; // digit prims the gear window's number draws appended
} MenuGearWindowPrims; // Size: 0x2AEC

void MenuSetWindowScrollBar(u_char a0, u_short a1, u_short a2, u_short a3, u_short a4);
void MenuInitializeWindowBorderCorners(u_char a0, u_short a1, u_short a2, u_short a3, u_short a4);
void MenuSetWindowBorderTop(u_char a0, u_short a1, u_short a2, u_short a3);
void MenuSetWindowBorderBottom(u_char a0, u_short a1, u_short a2, u_short a3, u_short a4);
void MenuSetWindowBorderLeft(u_char a0, u_short a1, u_short a2, u_short a3);
void MenuSetWindowBorderRight(u_char a0, u_short a1, u_short a2, u_short a3, u_short a4);
void MenuSetCharacterPortraitPrims(u_char a0, u_char a1, s32 a2, s32 a3);
void MenuSetCharacterWindowIcons(u_char a0, s32 a1, s32 a2);
void MenuSetCharacterHpDigits(u_char a0, u_char a1, s32 a2, s32 a3);
void MenuSetCharacterMpDigits(u_char a0, u_char a1, s32 a2, s32 a3);
void MenuSetCharacterWideNumberDigits(u_char a0, u_char a1, s32 a2, s32 a3);
void MenuSetCharacterLevelDigits(u_char a0, u_char a1, s32 a2, s32 a3);
extern s32 g_MenuCharacterIconX[];
extern s32 g_MenuCharacterIconY[];
extern s32 D_801E9B60[];
extern s32 D_801E9C20[];
extern s32 g_MenuCharacterIconTexIds[];
extern s32 D_801EA39C[];
extern s32 g_MenuCharacterHpDigitsX;
extern s32 g_MenuCharacterMpDigitsX;
extern s32 g_MenuCharacterMpDigitsY;
extern s32 g_MenuCharacterMaxMpDigitsX;
extern s32 g_MenuCharacterMaxMpDigitsY;
extern s32 g_MenuCharacterWideNumberDigitsX;
extern s32 g_MenuCharacterWideNumberDigitsY;
extern s32 g_MenuCharacterWideNumber2DigitsX;
extern s32 g_MenuCharacterWideNumber2DigitsY;
extern s32 g_MenuCharacterHpDigitsY;
extern s32 g_MenuCharacterMaxHpDigitsX;
extern s32 g_MenuCharacterMaxHpDigitsY;
extern s32 g_MenuCharacterNumberOrigins;
extern s32 g_MenuCharacterLevelDigitsY;
extern s32 g_MenuCharacterUnk63DigitsX;
extern s32 g_MenuCharacterUnk63DigitsY;
extern u16 g_MenuCharacterPortraitX;
extern u16 D_801E9D38;
extern u16 D_801E9D3C;
extern u16 g_MenuCharacterPortraitY;
extern s32 g_MenuNameTextureX[];
extern u8 g_MenuNameTextureY[];
extern u16 g_SystemPalette1;
extern u16 g_SystemPalette2;
void MenuSetFt4QuadVertices(POLY_FT4* pPoly, u_short x, u_short y, u_char u, u_char v, s32 width, s32 height);
void MenuInitializePolyFt4(POLY_FT4* pPoly);

void MenuSetQuadVertices(SVECTOR* pVertices, u16 x, u16 y, u16 width, u16 height);
s32 func_800263E4(MenuResourceDirectory* pDir, u32 index, POLY_FT4* pPrims, s32 context, s32 x, s32 y, s32 scale, s32 mirrorX, s32 mirrorY);
s32 func_8002675C(MenuResourceDirectory* pDir, u32 index, POLY_FT4* pPrims, s32 context, s32 x, s32 y, s32 scale);
void func_801D5ED4(u_char index, u_char characterId);
void MenuSetCharacterIconPrims(u_char characterId);
void func_801D6338(u_char index, u_char characterId);
void func_801D680C(u_char index, u_char characterId);
void func_801D6CF4(u_char index, u_char characterId);
void MenuSetGearWindowWideNumberPrims(u_char index, u_char characterId);
void func_801D74EC(u_char index, u_char characterId);
void func_801D7884(u_char index, u_char characterId);

void MenuSetWindowScrollBar(u_char index, u_short x, u_short y, u_short width, u_short height) {
    MenuWindow* pWindow = g_Menu->windows[index];

    func_8002675C(g_Menu->resources, MENU_TEX_SCROLL_BAR_ORNAMENT,
                  pWindow->polysScrollBarEnds, g_Menu->renderContext, x, y, 0x1000);
    func_800263E4(g_Menu->resources, MENU_TEX_SCROLL_BAR_ORNAMENT,
                  &pWindow->polysScrollBarEnds[2], g_Menu->renderContext,
                  x, y + height - 8, 0x1000, 0, 1);
    func_8002675C(g_Menu->resources, MENU_TEX_SCROLL_BAR_EMPTY,
                  pWindow->polysScrollBarEmpty, g_Menu->renderContext, x, y + 8, 0x1000);
    MenuSetQuadVertices(pWindow->vertsScrollBarEnds, x, y, 8, 8);
    MenuSetQuadVertices(&pWindow->vertsScrollBarEnds[4], x, y + height, 8, -8);
    MenuSetQuadVertices(pWindow->vertsScrollBarEmpty, x, y + 8, 8, height - 8);
}

void MenuSetPolySemiTransGrey(P_TAG* pTag);
void MenuInitializeWindowBorderCorners(u_char index, u_short x, u_short y, u_short width, u_short height) {
    MenuWindow* pWindow = g_Menu->windows[index];
    u16 left;
    u16 top;
    u16 right;
    u16 bottom;
    s32 i;

    pWindow->borderPrimCount = 0;
    pWindow->borderPrimCount = func_8002675C(g_Menu->resources, MENU_TEX_WINDOW_BORDER_TOP_LEFT,
                                    pWindow->polysWindowBorderCorners,
                                    g_Menu->renderContext, 0, 0, 0x1000)
                    + pWindow->borderPrimCount;
    pWindow->borderPrimCount = func_8002675C(g_Menu->resources, MENU_TEX_WINDOW_BORDER_TOP_RIGHT,
                                    &pWindow->polysWindowBorderCorners[2 * pWindow->borderPrimCount],
                                    g_Menu->renderContext, 0, 0, 0x1000)
                    + pWindow->borderPrimCount;
    pWindow->borderPrimCount = func_8002675C(g_Menu->resources, MENU_TEX_WINDOW_BORDER_BOTTOM_LEFT,
                                    &pWindow->polysWindowBorderCorners[2 * pWindow->borderPrimCount],
                                    g_Menu->renderContext, 0, 0, 0x1000)
                    + pWindow->borderPrimCount;
    pWindow->borderPrimCount = func_8002675C(g_Menu->resources, MENU_TEX_WINDOW_BORDER_BOTTOM_RIGHT,
                                    &pWindow->polysWindowBorderCorners[2 * pWindow->borderPrimCount],
                                    g_Menu->renderContext, 0, 0, 0x1000)
                    + pWindow->borderPrimCount;
    left = x - MENU_WINDOW_BORDER_SIZE;
    top = y + MENU_WINDOW_BORDER_SIZE;
    MenuSetQuadVertices(pWindow->vertsWindowBorderCorners, left, top, 0x10, 0xFFF0);
    right = x + width + MENU_WINDOW_BORDER_SIZE;
    MenuSetQuadVertices(&pWindow->vertsWindowBorderCorners[4], right, top, 0xFFF0, 0xFFF0);
    bottom = y + height - MENU_WINDOW_BORDER_SIZE;
    MenuSetQuadVertices(&pWindow->vertsWindowBorderCorners[8], left, bottom, 0x10, 0x10);
    MenuSetQuadVertices(&pWindow->vertsWindowBorderCorners[12], right, bottom, 0xFFF0, 0x10);
    for (i = 0; i < MENU_WINDOW_BORDER_CORNER_COUNT; i++) {
        MenuSetPolySemiTransGrey((P_TAG*)&pWindow->polysWindowBorderCorners[i * 2 + g_Menu->renderContext]);
    }
}
void MenuSetWindowBorderTop(u_char index, u_short x, u_short y, u_short width) {
    MenuWindow* pWindow = g_Menu->windows[index];
    s32 halfWidth;
    s32 i;

    (pWindow->polysWindowBorderTop + g_Menu->renderContext)->u0 = 0;
    (pWindow->polysWindowBorderTop + g_Menu->renderContext)->v0 = 0x84;
    (pWindow->polysWindowBorderTop + g_Menu->renderContext)->u1 = 7;
    (pWindow->polysWindowBorderTop + g_Menu->renderContext)->v1 = 0x84;
    (pWindow->polysWindowBorderTop + g_Menu->renderContext)->u2 = 0;
    (pWindow->polysWindowBorderTop + g_Menu->renderContext)->v2 = 0x94;
    (pWindow->polysWindowBorderTop + g_Menu->renderContext)->u3 = 7;
    (pWindow->polysWindowBorderTop + g_Menu->renderContext)->v3 = 0x94;
    (pWindow->polysWindowBorderTop + 2 + g_Menu->renderContext)->u0 = 0;
    (pWindow->polysWindowBorderTop + 2 + g_Menu->renderContext)->v0 = 0x84;
    (pWindow->polysWindowBorderTop + 2 + g_Menu->renderContext)->u1 = 7;
    (pWindow->polysWindowBorderTop + 2 + g_Menu->renderContext)->v1 = 0x84;
    (pWindow->polysWindowBorderTop + 2 + g_Menu->renderContext)->u2 = 0;
    (pWindow->polysWindowBorderTop + 2 + g_Menu->renderContext)->v2 = 0x94;
    (pWindow->polysWindowBorderTop + 2 + g_Menu->renderContext)->u3 = 7;
    (pWindow->polysWindowBorderTop + 2 + g_Menu->renderContext)->v3 = 0x94;
    halfWidth = (width - MENU_WINDOW_BORDER_SIZE * 2) / 2;
    MenuSetQuadVertices(pWindow->vertsWindowBorderTop1, x + MENU_WINDOW_BORDER_SIZE,
                        y - MENU_WINDOW_BORDER_SIZE, halfWidth, 0x10);
    x = halfWidth + MENU_WINDOW_BORDER_SIZE + x;
    MenuSetQuadVertices(pWindow->vertsWindowBorderTop2, x, y - MENU_WINDOW_BORDER_SIZE, halfWidth, 0x10);
    for (i = 0; i < 2; i++) {
        MenuSetPolySemiTransGrey((P_TAG*)&pWindow->polysWindowBorderTop[i * 2 + g_Menu->renderContext]);
    }
}

void MenuSetWindowBorderBottom(u_char index, u_short x, u_short y, u_short width, u_short height) {
    MenuWindow* pWindow = g_Menu->windows[index];
    u16 bottom;
    s32 halfWidth;
    s32 i;

    // Prim addresses are formed on the prim pointer, not with [index + 2]: the
    // index form makes gcc commute retail's `addu v0,v0,s6`.
    (pWindow->polysWindowBorderBottom + g_Menu->renderContext)->u0 = 8;
    (pWindow->polysWindowBorderBottom + g_Menu->renderContext)->v0 = 0x84;
    (pWindow->polysWindowBorderBottom + g_Menu->renderContext)->u1 = 0xF;
    (pWindow->polysWindowBorderBottom + g_Menu->renderContext)->v1 = 0x84;
    (pWindow->polysWindowBorderBottom + g_Menu->renderContext)->u2 = 8;
    (pWindow->polysWindowBorderBottom + g_Menu->renderContext)->v2 = 0x94;
    (pWindow->polysWindowBorderBottom + g_Menu->renderContext)->u3 = 0xF;
    (pWindow->polysWindowBorderBottom + g_Menu->renderContext)->v3 = 0x94;
    (pWindow->polysWindowBorderBottom + 2 + g_Menu->renderContext)->u0 = 8;
    (pWindow->polysWindowBorderBottom + 2 + g_Menu->renderContext)->v0 = 0x84;
    (pWindow->polysWindowBorderBottom + 2 + g_Menu->renderContext)->u1 = 0xF;
    (pWindow->polysWindowBorderBottom + 2 + g_Menu->renderContext)->v1 = 0x84;
    (pWindow->polysWindowBorderBottom + 2 + g_Menu->renderContext)->u2 = 8;
    (pWindow->polysWindowBorderBottom + 2 + g_Menu->renderContext)->v2 = 0x94;
    (pWindow->polysWindowBorderBottom + 2 + g_Menu->renderContext)->u3 = 0xF;
    (pWindow->polysWindowBorderBottom + 2 + g_Menu->renderContext)->v3 = 0x94;
    bottom = y + height - MENU_WINDOW_BORDER_SIZE;
    halfWidth = (width - MENU_WINDOW_BORDER_SIZE * 2) / 2;
    MenuSetQuadVertices(pWindow->vertsWindowBorderBottom1, x + MENU_WINDOW_BORDER_SIZE, bottom, halfWidth, 0x10);
    x = halfWidth + MENU_WINDOW_BORDER_SIZE + x;
    MenuSetQuadVertices(pWindow->vertsWindowBorderBottom2, x, bottom, halfWidth, 0x10);
    for (i = 0; i < 2; i++) {
        MenuSetPolySemiTransGrey((P_TAG*)&pWindow->polysWindowBorderBottom[i * 2 + g_Menu->renderContext]);
    }
}
void MenuSetWindowBorderLeft(u_char index, u_short x, u_short y, u_short height) {
       MenuWindow* pWindow = g_Menu->windows[index];
       u16 left;
       s32 halfHeight;
       s32 i;
   
       (pWindow->polysWindowBorderLeft + g_Menu->renderContext)->u0 = 0x10;
       (pWindow->polysWindowBorderLeft + g_Menu->renderContext)->v0 = 0x84;
       (pWindow->polysWindowBorderLeft + g_Menu->renderContext)->u1 = 0x20;
       (pWindow->polysWindowBorderLeft + g_Menu->renderContext)->v1 = 0x84;
       (pWindow->polysWindowBorderLeft + g_Menu->renderContext)->u2 = 0x10;
       (pWindow->polysWindowBorderLeft + g_Menu->renderContext)->v2 = 0x8B;
       (pWindow->polysWindowBorderLeft + g_Menu->renderContext)->u3 = 0x20;
       (pWindow->polysWindowBorderLeft + g_Menu->renderContext)->v3 = 0x8B;
       (pWindow->polysWindowBorderLeft + 2 + g_Menu->renderContext)->u0 = 0x10;
       (pWindow->polysWindowBorderLeft + 2 + g_Menu->renderContext)->v0 = 0x84;
       (pWindow->polysWindowBorderLeft + 2 + g_Menu->renderContext)->u1 = 0x20;
       (pWindow->polysWindowBorderLeft + 2 + g_Menu->renderContext)->v1 = 0x84;
       (pWindow->polysWindowBorderLeft + 2 + g_Menu->renderContext)->u2 = 0x10;
       (pWindow->polysWindowBorderLeft + 2 + g_Menu->renderContext)->v2 = 0x8B;
       (pWindow->polysWindowBorderLeft + 2 + g_Menu->renderContext)->u3 = 0x20;
       (pWindow->polysWindowBorderLeft + 2 + g_Menu->renderContext)->v3 = 0x8B;
       left = x - 8;
       halfHeight = (height - 0x10) / 2;
       MenuSetQuadVertices(pWindow->vertsWindowBorderLeft1, left, y + 8, 0x10, halfHeight);
       y = halfHeight + 8 + y;
       MenuSetQuadVertices(pWindow->vertsWindowBorderLeft2, left, y, 0x10, halfHeight);
       for (i = 0; i < 2; i++) {
           MenuSetPolySemiTransGrey((P_TAG*)&pWindow->polysWindowBorderLeft[i * 2 + g_Menu->renderContext]);
       }
   }

void MenuSetWindowBorderRight(u_char index, u_short x, u_short y, u_short width, u_short height) {
    MenuWindow* pWindow = g_Menu->windows[index];
    u16 right;
    s32 halfHeight;
    s32 i;

    (pWindow->polysWindowBorderRight + g_Menu->renderContext)->u0 = 0x10;
    (pWindow->polysWindowBorderRight + g_Menu->renderContext)->v0 = 0x8C;
    (pWindow->polysWindowBorderRight + g_Menu->renderContext)->u1 = 0x20;
    (pWindow->polysWindowBorderRight + g_Menu->renderContext)->v1 = 0x8C;
    (pWindow->polysWindowBorderRight + g_Menu->renderContext)->u2 = 0x10;
    (pWindow->polysWindowBorderRight + g_Menu->renderContext)->v2 = 0x93;
    (pWindow->polysWindowBorderRight + g_Menu->renderContext)->u3 = 0x20;
    (pWindow->polysWindowBorderRight + g_Menu->renderContext)->v3 = 0x93;
    (pWindow->polysWindowBorderRight + 2 + g_Menu->renderContext)->u0 = 0x10;
    (pWindow->polysWindowBorderRight + 2 + g_Menu->renderContext)->v0 = 0x8C;
    (pWindow->polysWindowBorderRight + 2 + g_Menu->renderContext)->u1 = 0x20;
    (pWindow->polysWindowBorderRight + 2 + g_Menu->renderContext)->v1 = 0x8C;
    (pWindow->polysWindowBorderRight + 2 + g_Menu->renderContext)->u2 = 0x10;
    (pWindow->polysWindowBorderRight + 2 + g_Menu->renderContext)->v2 = 0x93;
    (pWindow->polysWindowBorderRight + 2 + g_Menu->renderContext)->u3 = 0x20;
    (pWindow->polysWindowBorderRight + 2 + g_Menu->renderContext)->v3 = 0x93;
    right = x + width - 8;
    halfHeight = (height - 0x10) / 2;
    MenuSetQuadVertices(pWindow->vertsWindowBorderRight1, right, y + 8, 0x10, halfHeight);
    y = halfHeight + 8 + y;
    MenuSetQuadVertices(pWindow->vertsWindowBorderRight2, right, y, 0x10, halfHeight);
    for (i = 0; i < 2; i++) {
        MenuSetPolySemiTransGrey((P_TAG*)&pWindow->polysWindowBorderRight[i * 2 + g_Menu->renderContext]);
    }
}

void MenuSetWindow(u_char index, u_short x, u_short y, u_short width, u_short height,
                   u_char useDefaultMatrix, s32 zIndex, u_char hasScrollBar) {
    MenuWindow* pWindow = g_Menu->windows[index];

    g_Menu->pManager->shouldRenderWindow[index] = 0;
    MenuSetQuadVertices(pWindow->vertsBackground, x, y, width, height);
    MenuInitializeWindowBorderCorners(index, x, y, width, height);
    MenuSetWindowBorderTop(index, x, y, width);
    MenuSetWindowBorderBottom(index, x, y, width, height);
    MenuSetWindowBorderLeft(index, x, y, height);
    MenuSetWindowBorderRight(index, x, y, width, height);
    if (hasScrollBar != 0) {
        MenuSetWindowScrollBar(index, x, y, width, height);
    }
    pWindow->hasScrollBar = hasScrollBar;
    pWindow->useDefaultMatrix = useDefaultMatrix;
    pWindow->zIndex = zIndex;
    pWindow->renderContext = *(u8*)&g_Menu->renderContext;
    g_Menu->pManager->shouldRenderWindow[index] = 1;
}
void MenuFreeWindow(u_char index) {
    g_Menu->pManager->shouldRenderWindow[index] = 0;
    g_Menu->pManager->isWindowOpening[index] = 0;
    HeapFree(g_Menu->windows[index]);
    HeapFree(g_Menu->windowParameters[index]);
}
// MenuCharacterWindow.polysName, reached through a 0x50-byte stand-in for the
// polysPortrait pair before it (the scale function writes both pairs by offset).
typedef struct {
    /* 0x00 */ u8 unk0[0x50];
    /* 0x50 */ POLY_FT4 portraitPrims[2]; // one per render context
} MenuCharacterPortraitPrims; // Size: 0xA0

void MenuSetCharacterPortraitPrims(u_char index, u_char characterId, s32 x, s32 y) {
    MenuCharacterPortraitPrims* pWindow =
        (MenuCharacterPortraitPrims*)g_Menu->characterWindows[index];
    u8* pBase;
    s32 texX;

    func_8002675C(g_Menu->resources, index + MENU_TEX_PORTRAIT_PARTY_MEMBER_1,
                  (POLY_FT4*)pWindow, g_Menu->renderContext, x, y, 0x1000);
    MenuInitializePolyFt4(&pWindow->portraitPrims[g_Menu->renderContext]);
    pWindow->portraitPrims[g_Menu->renderContext].tpage = GetTPage(0, 0, 0x180, 0);
    // The clut store stays byte arithmetic: a POLY_FT4 member access charges the
    // +0x50 of portraitPrims to the address and drops retail's 0x5E displacement.
    pBase = (u8*)pWindow + g_Menu->renderContext * sizeof(POLY_FT4);
    if (g_Menu->pManager->currentCharacterIDs[index] & 1) {
        *(u16*)(pBase + 0x5E) = g_SystemPalette2;
    } else {
        *(u16*)(pBase + 0x5E) = g_SystemPalette1;
    }
    texX = g_MenuNameTextureX[index];
    MenuSetFt4QuadVertices(&pWindow->portraitPrims[g_Menu->renderContext], g_MenuCharacterPortraitX + x,
                           g_MenuCharacterPortraitY + y, ((texX << 1) << 1) & 0xFC, g_MenuNameTextureY[index * 4],
                           0x48, 0xD);
}
void MenuSetCharacterWindowIcons(u_char index, s32 x, s32 y) {
    MenuCharacterWindow* pWindow = g_Menu->characterWindows[index];
    s32 i;

    pWindow->iconsPrimCount = 0;
    for (i = 0; i < MENU_CHARACTER_ICON_COUNT; i++) {
        if (g_MenuCharacterIconTexIds[i] != MENU_RESOURCE_NONE) {
            pWindow->iconsPrimCount += func_8002675C(
                g_Menu->resources,
                g_MenuCharacterIconTexIds[i],
                &pWindow->polysIcons[pWindow->iconsPrimCount * 2],
                g_Menu->renderContext,
                x + g_MenuCharacterIconX[i],
                y + g_MenuCharacterIconY[i],
                0x1000);
        }
    }
}
void MenuSetCharacterHpDigits(u_char index, u_char characterId, s32 x, s32 y) {
    MenuCharacterWindow* pWindow = g_Menu->characterWindows[index];
    s32 i;
    s32 n;

    MenuParseNumberToDigits(g_GameState.characters[characterId].hp);
    pWindow->hpPrimCount = 0;
    for (i = 0; i < MENU_HP_DIGIT_COUNT; i++) {
        if (g_Menu->digits[6 + i] != MENU_DIGIT_NONE) {
            pWindow->hpPrimCount += func_8002675C(
                g_Menu->resources, g_Menu->digits[6 + i],
                &pWindow->polysHp[pWindow->hpPrimCount * 2],
                g_Menu->renderContext, x + g_MenuCharacterHpDigitsX + i * MENU_DIGIT_SPACING,
                y + g_MenuCharacterHpDigitsY, 0x1000);
        }
    }
    i = 0;
    MenuParseNumberToDigits(g_GameState.characters[characterId].maxHp);
    pWindow->maxHpPrimCount = 0;
    n = 0;
    for (; i < MENU_HP_DIGIT_COUNT; ) {
        u8 digit = g_Menu->digits[6 + i++];

        if (digit != MENU_DIGIT_NONE) {
            pWindow->maxHpPrimCount += func_8002675C(
                g_Menu->resources, digit,
                &pWindow->polysMaxHp[pWindow->maxHpPrimCount * 2],
                g_Menu->renderContext, x + g_MenuCharacterMaxHpDigitsX + n * MENU_DIGIT_SPACING,
                y + g_MenuCharacterMaxHpDigitsY, 0x1000);
            n++;
        }
    }
}
void MenuSetCharacterMpDigits(u_char index, u_char characterId, s32 x, s32 y) {
    MenuCharacterWindow* pWindow = g_Menu->characterWindows[index];
    s32 i;
    s32 n;

    MenuParseNumberToDigits(g_GameState.characters[characterId].mp);
    pWindow->mpPrimCount = 0;
    for (i = 0; i < MENU_MP_DIGIT_COUNT; i++) {
        if (g_Menu->digits[7 + i] != MENU_DIGIT_NONE) {
            pWindow->mpPrimCount += func_8002675C(
                g_Menu->resources, g_Menu->digits[7 + i],
                &pWindow->polysMp[pWindow->mpPrimCount * 2],
                g_Menu->renderContext, x + g_MenuCharacterMpDigitsX + i * MENU_DIGIT_SPACING,
                y + g_MenuCharacterMpDigitsY, 0x1000);
        }
    }
    i = 0;
    MenuParseNumberToDigits(g_GameState.characters[characterId].maxMp);
    pWindow->maxMpPrimCount = 0;
    n = 0;
    for (; i < MENU_MP_DIGIT_COUNT; ) {
        u8 digit = g_Menu->digits[7 + i++];

        if (digit != MENU_DIGIT_NONE) {
            pWindow->maxMpPrimCount += func_8002675C(
                g_Menu->resources, digit,
                &pWindow->polysMaxMp[pWindow->maxMpPrimCount * 2],
                g_Menu->renderContext, x + g_MenuCharacterMaxMpDigitsX + n * MENU_DIGIT_SPACING,
                y + g_MenuCharacterMaxMpDigitsY, 0x1000);
            n++;
        }
    }
}
void MenuSetCharacterWideNumberDigits(u_char index, u_char characterId, s32 x, s32 y) {
    MenuCharacterWindow* pWindow = g_Menu->characterWindows[index];
    s32 i;

    MenuParseNumberToDigits(g_GameState.characters[characterId].unk44);
    pWindow->wideNumberPrimCount = 0;
    for (i = 0; i < MENU_WIDE_NUMBER_DIGIT_COUNT; i++) {
        if (g_Menu->digits[2 + i] != MENU_DIGIT_NONE) {
            pWindow->wideNumberPrimCount += func_8002675C(
                g_Menu->resources, g_Menu->digits[2 + i],
                &pWindow->polysWideNumber[pWindow->wideNumberPrimCount * 2],
                g_Menu->renderContext, x + g_MenuCharacterWideNumberDigitsX + i * MENU_DIGIT_SPACING,
                y + g_MenuCharacterWideNumberDigitsY, 0x1000);
        }
    }
    MenuParseNumberToDigits(g_GameState.characters[characterId].unk48);
    pWindow->wideNumber2PrimCount = 0;
    for (i = 0; i < MENU_WIDE_NUMBER_DIGIT_COUNT; i++) {
        if (g_Menu->digits[2 + i] != MENU_DIGIT_NONE) {
            pWindow->wideNumber2PrimCount += func_8002675C(
                g_Menu->resources, g_Menu->digits[2 + i],
                &pWindow->polysWideNumber2[pWindow->wideNumber2PrimCount * 2],
                g_Menu->renderContext, x + g_MenuCharacterWideNumber2DigitsX + i * MENU_DIGIT_SPACING,
                y + g_MenuCharacterWideNumber2DigitsY, 0x1000);
        }
    }
}
void MenuSetCharacterLevelDigits(u_char index, u_char characterId, s32 x, s32 y) {
    MenuCharacterWindow* pWindow = g_Menu->characterWindows[index];
    s32 i;

    MenuParseNumberToDigits(g_GameState.characters[characterId].level);
    pWindow->levelPrimCount = 0;
    for (i = 0; i < MENU_LEVEL_DIGIT_COUNT; i++) {
        if (g_Menu->digits[6 + i] != MENU_DIGIT_NONE) {
            pWindow->levelPrimCount += func_8002675C(
                g_Menu->resources, g_Menu->digits[6 + i],
                &pWindow->polysLevel[pWindow->levelPrimCount * 2],
                g_Menu->renderContext, x + g_MenuCharacterNumberOrigins + i * MENU_DIGIT_SPACING, y + g_MenuCharacterLevelDigitsY,
                0x1000);
        }
    }
    MenuParseNumberToDigits(g_GameState.characters[characterId].unk63);
    pWindow->unk63DigitsPrimCount = 0;
    for (i = 0; i < MENU_LEVEL_DIGIT_COUNT; i++) {
        if (g_Menu->digits[6 + i] != MENU_DIGIT_NONE) {
            pWindow->unk63DigitsPrimCount += func_8002675C(
                g_Menu->resources, g_Menu->digits[6 + i],
                &pWindow->polysUnk63Digits[pWindow->unk63DigitsPrimCount * 2],
                g_Menu->renderContext, x + g_MenuCharacterUnk63DigitsX + i * MENU_DIGIT_SPACING, y + g_MenuCharacterUnk63DigitsY,
                0x1000);
        }
    }
    for (i = 0; i < pWindow->unk63DigitsPrimCount; i++) {
        SetShadeTex(&pWindow->polysUnk63Digits[i * 2 + g_Menu->renderContext], 0);
        // The element pointer must be formed before the base is added here; the
        // indexed spelling (.polysUnk63Digits[i * 2 + ctx].r0) commutes retail's addu.
        (pWindow->polysUnk63Digits + (i * 2 + g_Menu->renderContext))->r0 = 0;
        (pWindow->polysUnk63Digits + (i * 2 + g_Menu->renderContext))->g0 = 0x80;
        (pWindow->polysUnk63Digits + (i * 2 + g_Menu->renderContext))->b0 = 0;
    }
}
