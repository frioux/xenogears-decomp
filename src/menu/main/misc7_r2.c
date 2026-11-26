#include "common.h"

#include "main/game.h"
#include "system/memory.h"
#include "system/menu.h"

// gear hp/maxHp are 32-bit, so their window numbers have five digits
#define MENU_GEAR_HP_DIGIT_COUNT 5
#define MENU_GEAR_FUEL_DIGIT_COUNT 4

// the icon texture id tables hold one entry per slot of the character's icon strip
#define MENU_ICON_TEXTURE_COUNT 24
#define MENU_ICON_NONE 0xFFFF

// the 'G' glyph drawn after the gold amount
#define MENU_TEX_GOLD_GLYPH 0x10




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
    /* 0x2AE1 */ u8 levelPrimCount; // digit prims the character's level number drew
    /* 0x2AE2 */ u8 unk63DigitsPrimCount; // digit prims GameCharacter.unk63 drew
    /* 0x2AE3 */ u8 firstWideNumberPrimCount; // prims the first wide number drew
    /* 0x2AE4 */ u8 secondWideNumberPrimCount; // prims the second wide number drew
    /* 0x2AE5 */ u8 firstWideNumber2PrimCount; // prims the second wide number pair's first number drew
    /* 0x2AE6 */ u8 secondWideNumber2PrimCount; // prims the second wide number pair's second number drew
    /* 0x2AE7 */ u8 hpPrimCount; // digit prims the hp number drew
    /* 0x2AE8 */ u8 maxHpPrimCount; // digit prims the maxHp number drew
    /* 0x2AE9 */ u8 mpPrimCount; // digit prims the mp number drew
    /* 0x2AEA */ u8 maxMpPrimCount; // digit prims the maxMp number drew
    /* 0x2AEB */ u8 digitPrimCount; // digit prims the gear window's number draws appended
    /* 0x2AEC */ u8 iconPrimCount; // digit prims the character's icon ids drew
} MenuGearWindowPrims; // Size: 0x2AF0

// The gold window's prim buffer (g_Menu->pGoldWindow): the digit prims plus the two
// glyph prims at the tail.
typedef struct {
    /* 0x000 */ POLY_FT4 digitPrims[18]; // two per digit, one per render context
    /* 0x2D0 */ POLY_FT4 glyphPrims[2]; // one per render context, added by MenuAddGoldAndPlayTimePrims
    /* 0x320 */ s32 digitPrimCount;
    /* 0x324 */ u_char renderContext;
} MenuGoldWindow; // Size: 0x328

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
extern s32 g_MenuGearWindowIconX[];
extern s32 g_MenuGearWindowIconY[];
extern s32 g_MenuCharacterIconTexIds[];
extern s32 g_MenuGearWindowIconTexIds[];
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
void MenuSetGearWindowNamePrims(u_char index, u_char characterId);
void MenuSetCharacterIconPrims(u_char characterId);
void MenuSetGearWindowHpDigits(u_char index, u_char characterId);
void MenuSetGearWindowMpDigits(u_char index, u_char characterId);
void MenuSetGearWindowLevelDigits(u_char index, u_char characterId);
void MenuSetGearWindowWideNumberPrims(u_char index, u_char characterId);
void MenuSetGearWindowWideNumber2Prims(u_char index, u_char characterId);
void func_801D7884(u_char index, u_char characterId);

void MenuSetGoldDigits(s32 x, s32 y) {
    s32 i;
    s32 digitX;

    MenuParseNumberToDigits(g_GameState.gold);
    ((MenuGoldWindow*)g_Menu->pGoldWindow)->digitPrimCount = 0;
    for (i = 0, digitX = x; i < MENU_MAX_DIGITS; ) {
        u8 digit = g_Menu->digits[i++];

        if (digit != MENU_DIGIT_NONE) {
            // The prim address is summed in integer space: a pointer sum here
            // gives gcc the commuted `addu a2,t0,a2` retail does not have.
            ((MenuGoldWindow*)g_Menu->pGoldWindow)->digitPrimCount =
                func_8002675C(g_Menu->resources, digit,
                              (POLY_FT4*)(((MenuGoldWindow*)g_Menu->pGoldWindow)->digitPrimCount *
                                              (2 * sizeof(POLY_FT4)) +
                                          (u32)((MenuGoldWindow*)g_Menu->pGoldWindow)->digitPrims),
                              g_Menu->renderContext, digitX, y, 0x1000) +
                ((MenuGoldWindow*)g_Menu->pGoldWindow)->digitPrimCount;
        }
        digitX += MENU_DIGIT_SPACING;
    }
    func_8002675C(g_Menu->resources, MENU_TEX_GOLD_GLYPH,
                  &((MenuGoldWindow*)g_Menu->pGoldWindow)->glyphPrims[0],
                  g_Menu->renderContext, x + 0x50, y, 0x1000);
    g_Menu->pManager->goldDigitsActive = 1;
    ((MenuGoldWindow*)g_Menu->pGoldWindow)->renderContext = *(u8*)&g_Menu->renderContext;
}
void MenuSetPlayTimeDigitPrims(s32 x, s32 y) {
    s32 i;

    // Hours, minutes and seconds, each with its own x spacing. The slot address
    // is summed in integer space: `polys[i]` gives the commuted addu retail does
    // not have (MenuSetGoldDigits above has the same constraint).
    for (i = 0; i < 3; i++) {
        func_8002675C(g_Menu->resources, g_Menu->playTimeDigitIds[i],
                      (POLY_FT4*)(i * sizeof(g_Menu->pPlayTimeDigits->polys[0]) + (s32)g_Menu->pPlayTimeDigits->polys),
                      g_Menu->renderContext, x + i * MENU_DIGIT_SPACING, y, 0x1000);
    }
    for (i = 3; i < 5; i++) {
        func_8002675C(g_Menu->resources, g_Menu->playTimeDigitIds[i],
                      (POLY_FT4*)(i * sizeof(g_Menu->pPlayTimeDigits->polys[0]) + (s32)g_Menu->pPlayTimeDigits->polys),
                      g_Menu->renderContext, x + i * MENU_DIGIT_SPACING + MENU_DIGIT_SPACING, y, 0x1000);
    }
    for (i = 5; i < 7; i++) {
        func_8002675C(g_Menu->resources, g_Menu->playTimeDigitIds[i],
                      (POLY_FT4*)(i * sizeof(g_Menu->pPlayTimeDigits->polys[0]) + (s32)g_Menu->pPlayTimeDigits->polys),
                      g_Menu->renderContext, x + i * MENU_DIGIT_SPACING + MENU_DIGIT_SPACING * 2, y, 0x1000);
    }
    func_8002675C(g_Menu->resources, MENU_TEX_PLAY_TIME_COLON, &g_Menu->pPlayTimeDigits->polys[7],
                  g_Menu->renderContext, x + MENU_DIGIT_SPACING * 3, y, 0x1000);
    func_8002675C(g_Menu->resources, MENU_TEX_PLAY_TIME_COLON, &g_Menu->pPlayTimeDigits->polys[9],
                  g_Menu->renderContext, x + MENU_DIGIT_SPACING * 6, y, 0x1000);
    g_Menu->pPlayTimeDigits->renderContext = *(u8*)&g_Menu->renderContext;
}
void MenuSetGearWindowNamePrims(u_char index, u_char row) {
    // The negated row offset is load-bearing: a plain `+` mirrors the addu operands
    // of every row address in this body against retail's.
    s32 rowOffset = row ? 0x18 : 0;
    u8* pRow;
    u8* pBase;
    s32 texX;
    u16 width;

    func_8002675C(g_Menu->resources, index + 0x14B, (POLY_FT4*)g_Menu->pGearWindowPrims,
                  g_Menu->renderContext, 0x18 - rowOffset, 0xE, 0x1000);
    MenuSetQuadVertices((SVECTOR*)((u8*)g_Menu->pGearWindowPrims + 0x1EA0),
                        ((POLY_FT4*)((u8*)g_Menu->pGearWindowPrims - -(g_Menu->renderContext * sizeof(POLY_FT4))))->x0,
                        ((POLY_FT4*)((u8*)g_Menu->pGearWindowPrims - -(g_Menu->renderContext * sizeof(POLY_FT4))))->y0, 0x30, 0x30);
    MenuInitializePolyFt4((POLY_FT4*)((g_Menu->renderContext * sizeof(POLY_FT4) + 0x50) +
                                      (u8*)g_Menu->pGearWindowPrims));
    *(u16*)((u8*)g_Menu->pGearWindowPrims - -(g_Menu->renderContext * sizeof(POLY_FT4)) + 0x66) =
        GetTPage(0, 0, 0x180, 0);
    if (row == 0) {
        pBase = (u8*)g_Menu->pGearWindowPrims - -(g_Menu->renderContext * sizeof(POLY_FT4));
        if (g_Menu->pManager->currentCharacterIDs[index] & 1) {
            *(u16*)(pBase + 0x5E) = g_SystemPalette2;
        } else {
            *(u16*)(pBase + 0x5E) = g_SystemPalette1;
        }
    } else {
        pBase = (u8*)g_Menu->pGearWindowPrims - -(g_Menu->renderContext * sizeof(POLY_FT4));
        pRow = pBase;
        if ((g_GameState.characters[g_Menu->pManager->currentCharacterIDs[index]].gearId + 0xB) & 1) {
            *(u16*)(pRow + 0x5E) = g_SystemPalette2;
        } else {
            *(u16*)(pRow + 0x5E) = g_SystemPalette1;
        }
    }
    width = 0x18 * row + 0x48;
    texX = g_MenuNameTextureX[3 * row + index];
    MenuSetFt4QuadVertices((POLY_FT4*)((g_Menu->renderContext * sizeof(POLY_FT4) + 0x50) +
                                       (u8*)g_Menu->pGearWindowPrims),
                           0, 0, ((texX << 1) << 1) & 0xFC,
                           g_MenuNameTextureY[(3 * row + index) * 4], width, 0xD);
    MenuSetQuadVertices((SVECTOR*)((u8*)g_Menu->pGearWindowPrims + 0x1EC0),
                        D_801E9D38 - rowOffset, D_801E9D3C, width, 0xD);
}
void MenuSetCharacterIconPrims(u_char characterId) {
    s32 i;
    u8* pBase;
    u8* pEntry;

    ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->iconPrimCount = 0;
    for (i = 0; i < MENU_ICON_TEXTURE_COUNT; i++) {
        if (g_MenuGearWindowIconTexIds[characterId * MENU_ICON_TEXTURE_COUNT + i] == MENU_ICON_NONE) continue;
        ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->iconPrimCount += func_8002675C(
            g_Menu->resources,
            g_MenuGearWindowIconTexIds[characterId * MENU_ICON_TEXTURE_COUNT + i],
            (POLY_FT4*)((u8*)g_Menu->pGearWindowPrims +
                        (((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->iconPrimCount *
                             (2 * sizeof(POLY_FT4)) +
                         0xA0)),
            g_Menu->renderContext,
            g_MenuGearWindowIconX[characterId * MENU_ICON_TEXTURE_COUNT + i],
            g_MenuGearWindowIconY[characterId * MENU_ICON_TEXTURE_COUNT + i],
            0x1000);
    }
    for (i = 0; i < ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->iconPrimCount; i++) {
        POLY_FT4* pPrim;
        s32 offset = (i * 2 + g_Menu->renderContext) * sizeof(POLY_FT4);

        pBase = (u8*)g_Menu->pGearWindowPrims;
        pEntry = pBase + offset;
        pPrim = (POLY_FT4*)(pEntry + 0xA0);
        MenuSetQuadVertices((SVECTOR*)((u8*)g_Menu->pGearWindowPrims + 0x1EE0 + i * 0x20),
                            pPrim->x0, pPrim->y0,
                            pPrim->x1 - *(volatile u16*)&pPrim->x0,
                            pPrim->y3 - *(volatile u16*)&pPrim->y0);
    }
}
extern s32 g_MenuGearWindowHpX;
extern s32 g_MenuGearWindowHpY;
extern s32 g_MenuGearWindowMaxHpX;
extern s32 g_MenuGearWindowMaxHpY;

// The character (3 digits) or gear (5 digits) hp and maxHp window numbers: two
// digit-prim passes and the quad pass that grows each prim to its glyph box.
void MenuSetGearWindowHpDigits(u_char index, u_char characterId) {
    u8* pPrims;
    u8* pBase;
    u8* pEntry;
    s32 vertOffset;
    s32 vertsOffset;
    s32 count;
    s32 digitOffset;
    s32 x;
    s32 i;

    if (characterId == 0) {
        count = MENU_HP_DIGIT_COUNT;
        digitOffset = MENU_MAX_DIGITS - count;
        x = g_MenuGearWindowHpX;
        MenuParseNumberToDigits(
            g_GameState.characters[g_Menu->pManager->currentCharacterIDs[index]].hp);
    } else {
        count = MENU_GEAR_HP_DIGIT_COUNT;
        digitOffset = MENU_MAX_DIGITS - count;
        x = g_MenuGearWindowHpX + 8;
        MenuParseNumberToDigits(
            g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[index]].gearId].hp);
    }
    ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->hpPrimCount = 0;
    for (i = 0; i < count; i++) {
        if (g_Menu->digits[digitOffset + i] != MENU_DIGIT_NONE) {
            ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->hpPrimCount += func_8002675C(
                g_Menu->resources, g_Menu->digits[digitOffset + i],
                (POLY_FT4*)((u8*)g_Menu->pGearWindowPrims +
                            (((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->hpPrimCount *
                                 (2 * sizeof(POLY_FT4)) +
                             0x1630)),
                g_Menu->renderContext, x + i * MENU_DIGIT_SPACING - characterId * 24,
                g_MenuGearWindowHpY, 0x1000);
        }
    }
    for (i = 0; i < ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->hpPrimCount; i++) {
        POLY_FT4* pPrim;
        s32 primOffset;

        primOffset = (i * 2 + g_Menu->renderContext) * sizeof(POLY_FT4);
        pBase = (u8*)g_Menu->pGearWindowPrims;
        pEntry = pBase + primOffset;
        pPrim = (POLY_FT4*)(pEntry + 0x1630);
        vertOffset = 0x2780 + i * 0x20;
        MenuSetQuadVertices((SVECTOR*)(vertOffset + (u32)(u8*)g_Menu->pGearWindowPrims), pPrim->x0,
                            pPrim->y0, pPrim->x1 - *(volatile u16*)&pPrim->x0,
                            pPrim->y3 - *(volatile u16*)&pPrim->y0);
    }

    if (characterId == 0) {
        MenuParseNumberToDigits(
            g_GameState.characters[g_Menu->pManager->currentCharacterIDs[index]].maxHp);
        x = g_MenuGearWindowMaxHpX;
    } else {
        MenuParseNumberToDigits(
            g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[index]].gearId].maxHp);
        x = g_MenuGearWindowMaxHpX - 0x20;
    }
    ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->maxHpPrimCount = 0;
    for (i = 0; i < count; i++) {
        if (g_Menu->digits[digitOffset + i] != MENU_DIGIT_NONE) {
            ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->maxHpPrimCount += func_8002675C(
                g_Menu->resources, g_Menu->digits[digitOffset + i],
                (POLY_FT4*)((u8*)g_Menu->pGearWindowPrims +
                            (((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->maxHpPrimCount *
                                 (2 * sizeof(POLY_FT4)) +
                             0x17C0)),
                g_Menu->renderContext, x - characterId * 24, characterId * 8 + g_MenuGearWindowMaxHpY,
                0x1000);
            x += MENU_DIGIT_SPACING;
        }
    }
    for (i = 0; i < ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->maxHpPrimCount; i++) {
        POLY_FT4* pPrim;
        s32 primOffset = (i * 2 + g_Menu->renderContext) * sizeof(POLY_FT4);

        vertsOffset = 0x2820 + i * 0x20;
        pPrims = (u8*)g_Menu->pGearWindowPrims;
        pBase = pPrims;
        pEntry = pBase + primOffset;
        pPrim = (POLY_FT4*)(pEntry + 0x17C0);
        MenuSetQuadVertices((SVECTOR*)(vertsOffset + (u32)pPrims), pPrim->x0, pPrim->y0,
                            pPrim->x1 - *(volatile u16*)&pPrim->x0,
                            pPrim->y3 - *(volatile u16*)&pPrim->y0);
    }
}
extern s32 D_801E9D00;
extern s32 D_801E9D04;
extern s32 D_801E9D08;
extern s32 D_801E9D0C;

// The gear window's mp (character: two digits) or fuel (gear: four digits) numbers:
// the same two digit-prim passes and quad pass as the stat numbers above.
void MenuSetGearWindowMpDigits(u_char index, u_char characterId) {
    u8* pPrims;
    u8* pBase;
    u8* pEntry;
    s32 vertOffset;
    s32 vertsOffset;
    s32 count;
    s32 digitOffset;
    s32 x;
    s32 y;
    s32 i;

    if (characterId == 0) {
        count = MENU_MP_DIGIT_COUNT;
        digitOffset = MENU_MAX_DIGITS - count;
        x = D_801E9D00;
        y = D_801E9D04;
        MenuParseNumberToDigits(
            g_GameState.characters[g_Menu->pManager->currentCharacterIDs[index]].mp);
    } else {
        count = MENU_GEAR_FUEL_DIGIT_COUNT;
        digitOffset = MENU_MAX_DIGITS - count;
        x = D_801E9D00;
        y = D_801E9D04 + 8;
        MenuParseNumberToDigits(
            g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[index]].gearId].fuel);
    }
    ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->mpPrimCount = 0;
    for (i = 0; i < count; i++) {
        if (g_Menu->digits[digitOffset + i] != MENU_DIGIT_NONE) {
            ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->mpPrimCount += func_8002675C(
                g_Menu->resources, g_Menu->digits[digitOffset + i],
                (POLY_FT4*)((u8*)g_Menu->pGearWindowPrims +
                            (((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->mpPrimCount *
                                 (2 * sizeof(POLY_FT4)) +
                             0x1950)),
                g_Menu->renderContext, x + i * MENU_DIGIT_SPACING - characterId * 24, y, 0x1000);
        }
    }
    for (i = 0; i < ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->mpPrimCount; i++) {
        POLY_FT4* pPrim;
        s32 primOffset;

        primOffset = (i * 2 + g_Menu->renderContext) * sizeof(POLY_FT4);
        pBase = (u8*)g_Menu->pGearWindowPrims;
        pEntry = pBase + primOffset;
        pPrim = (POLY_FT4*)(pEntry + 0x1950);
        vertOffset = 0x28C0 + i * 0x20;
        MenuSetQuadVertices((SVECTOR*)(vertOffset + (u32)(u8*)g_Menu->pGearWindowPrims), pPrim->x0,
                            pPrim->y0, pPrim->x1 - *(volatile u16*)&pPrim->x0,
                            pPrim->y3 - *(volatile u16*)&pPrim->y0);
    }

    if (characterId == 0) {
        MenuParseNumberToDigits(
            g_GameState.characters[g_Menu->pManager->currentCharacterIDs[index]].maxMp);
        x = D_801E9D08;
        y = D_801E9D0C;
    } else {
        MenuParseNumberToDigits(
            g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[index]].gearId].maxFuel);
        x = D_801E9D08 - 0x20;
        y = D_801E9D0C + 0x10;
    }
    ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->maxMpPrimCount = 0;
    for (i = 0; i < count; i++) {
        if (g_Menu->digits[digitOffset + i] != MENU_DIGIT_NONE) {
            ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->maxMpPrimCount += func_8002675C(
                g_Menu->resources, g_Menu->digits[digitOffset + i],
                (POLY_FT4*)((u8*)g_Menu->pGearWindowPrims +
                            (((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->maxMpPrimCount *
                                 (2 * sizeof(POLY_FT4)) +
                             0x1AE0)),
                g_Menu->renderContext, x - characterId * 24, y, 0x1000);
            // The bump is conditional in this half only: retail's loop-back delay slot
            // holds a nop here and the `addiu` inside the block (half 1 is the reverse).
            x += MENU_DIGIT_SPACING;
        }
    }
    for (i = 0; i < ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->maxMpPrimCount; i++) {
        POLY_FT4* pPrim;
        s32 primOffset = (i * 2 + g_Menu->renderContext) * sizeof(POLY_FT4);

        vertsOffset = 0x2960 + i * 0x20;
        pPrims = (u8*)g_Menu->pGearWindowPrims;
        pBase = pPrims;
        pEntry = pBase + primOffset;
        pPrim = (POLY_FT4*)(pEntry + 0x1AE0);
        MenuSetQuadVertices((SVECTOR*)(vertsOffset + (u32)pPrims), pPrim->x0, pPrim->y0,
                            pPrim->x1 - *(volatile u16*)&pPrim->x0,
                            pPrim->y3 - *(volatile u16*)&pPrim->y0);
    }
}
extern s32 g_MenuGearWindowLevelX;
extern s32 g_MenuGearWindowLevelY;
extern s32 g_MenuGearWindowUnk63X;
extern s32 g_MenuGearWindowUnk63Y;

// The gear window's level number and the character's unk63: both are parsed into the
// same g_Menu->digits[6..8] slots and drawn as three digits, then every prim each
// number produced is grown to its glyph box.
void MenuSetGearWindowLevelDigits(u_char index, u_char row) {
    u8* pBase;
    u8* pEntry;
    POLY_FT4* pPrim;
    s32 i;

    MenuParseNumberToDigits(g_GameState.characters[g_Menu->pManager->currentCharacterIDs[index]].level);
    ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->levelPrimCount = 0;
    for (i = 0; i < MENU_LEVEL_DIGIT_COUNT; i++) {
        if (g_Menu->digits[6 + i] != MENU_DIGIT_NONE) {
            ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->levelPrimCount += func_8002675C(
                g_Menu->resources, g_Menu->digits[6 + i],
                (POLY_FT4*)((u8*)g_Menu->pGearWindowPrims +
                            (((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->levelPrimCount *
                                 (2 * sizeof(POLY_FT4)) +
                             0xAF0)),
                g_Menu->renderContext, g_MenuGearWindowLevelX + i * MENU_DIGIT_SPACING - row * 24, g_MenuGearWindowLevelY,
                0x1000);
        }
    }
    for (i = 0; i < ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->levelPrimCount; i++) {
        s32 primOffset = (i * 2 + g_Menu->renderContext) * sizeof(POLY_FT4);

        pBase = (u8*)g_Menu->pGearWindowPrims;
        pEntry = pBase + primOffset;
        pPrim = (POLY_FT4*)(pEntry + 0xAF0);
        MenuSetQuadVertices((SVECTOR*)((u8*)g_Menu->pGearWindowPrims + 0x2300 + i * 0x20), pPrim->x0,
                            pPrim->y0, pPrim->x1 - *(volatile u16*)&pPrim->x0,
                            pPrim->y3 - *(volatile u16*)&pPrim->y0);
    }

    MenuParseNumberToDigits(g_GameState.characters[g_Menu->pManager->currentCharacterIDs[index]].unk63);
    ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk63DigitsPrimCount = 0;
    for (i = 0; i < MENU_LEVEL_DIGIT_COUNT; i++) {
        if (g_Menu->digits[6 + i] != MENU_DIGIT_NONE) {
            ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk63DigitsPrimCount += func_8002675C(
                g_Menu->resources, g_Menu->digits[6 + i],
                (POLY_FT4*)((u8*)g_Menu->pGearWindowPrims +
                            (((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk63DigitsPrimCount *
                                 (2 * sizeof(POLY_FT4)) +
                             0xBE0)),
                g_Menu->renderContext, g_MenuGearWindowUnk63X + i * MENU_DIGIT_SPACING - row * 24, g_MenuGearWindowUnk63Y,
                0x1000);
        }
    }
    for (i = 0; i < ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk63DigitsPrimCount; i++) {
        // Every use re-forms the prim address: a local would keep it in a register across
        // the call. The constant has to precede the offset sum in SetShadeTex's argument
        // (retail adds 0xBE0 to the offset, then the base) while the three stores add the
        // base last.
        SetShadeTex((POLY_FT4*)((u8*)g_Menu->pGearWindowPrims + 0xBE0 +
                                (i * 2 + g_Menu->renderContext) * sizeof(POLY_FT4)),
                    0);
        ((POLY_FT4*)((u8*)g_Menu->pGearWindowPrims +
                     (i * 2 + g_Menu->renderContext) * sizeof(POLY_FT4) + 0xBE0))->r0 = 0;
        ((POLY_FT4*)((u8*)g_Menu->pGearWindowPrims +
                     (i * 2 + g_Menu->renderContext) * sizeof(POLY_FT4) + 0xBE0))->g0 = 0x80;
        ((POLY_FT4*)((u8*)g_Menu->pGearWindowPrims +
                     (i * 2 + g_Menu->renderContext) * sizeof(POLY_FT4) + 0xBE0))->b0 = 0;
        // The double-negated offset is what puts the base first in this sum's addu;
        // every plain spelling mirrors the operands.
        MenuSetQuadVertices((SVECTOR*)((u8*)g_Menu->pGearWindowPrims + 0x2360 + i * 0x20),
                            ((POLY_FT4*)((u8*)g_Menu->pGearWindowPrims -
                                         -((i * 2 + g_Menu->renderContext) * sizeof(POLY_FT4)) + 0xBE0))->x0,
                            ((POLY_FT4*)((u8*)g_Menu->pGearWindowPrims -
                                         -((i * 2 + g_Menu->renderContext) * sizeof(POLY_FT4)) + 0xBE0))->y0,
                            ((POLY_FT4*)((u8*)g_Menu->pGearWindowPrims -
                                         -((i * 2 + g_Menu->renderContext) * sizeof(POLY_FT4)) + 0xBE0))->x1 -
                                *(volatile u16*)&((POLY_FT4*)((u8*)g_Menu->pGearWindowPrims -
                                                              -((i * 2 + g_Menu->renderContext) * sizeof(POLY_FT4)) + 0xBE0))->x0,
                            ((POLY_FT4*)((u8*)g_Menu->pGearWindowPrims -
                                         -((i * 2 + g_Menu->renderContext) * sizeof(POLY_FT4)) + 0xBE0))->y3 -
                                *(volatile u16*)&((POLY_FT4*)((u8*)g_Menu->pGearWindowPrims -
                                                              -((i * 2 + g_Menu->renderContext) * sizeof(POLY_FT4)) + 0xBE0))->y0);
    }
}
extern s32 g_MenuGearWindowWideNumberX;
extern s32 g_MenuGearWindowWideNumberY;
extern s32 g_MenuGearWindowWideNumber2X;
extern s32 g_MenuGearWindowWideNumber2Y;
extern s32 g_MenuGearWindowWideNumber3X;
extern s32 g_MenuGearWindowWideNumber3Y;
extern s32 g_MenuGearWindowWideNumber4X;
extern s32 g_MenuGearWindowWideNumber4Y;
extern s32 D_801E9D30;
extern s32 D_801E9D34;
// The gear window's two stat numbers (unk44, unk48 of the character): each is
// drawn as up to eight digits, then every prim it produced is grown to its glyph box.
void MenuSetGearWindowWideNumberPrims(u_char index, u_char characterId) {
    u8* pPrims;
    u8* pBase;
    u8* pEntry;
    s32 offset;
    s32 vertOffset;
    s32 i;

    MenuParseNumberToDigits(
        g_GameState.characters[g_Menu->pManager->currentCharacterIDs[index]].unk3C);
    offset = characterId * 24;
    ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->firstWideNumberPrimCount = 0;
    for (i = 0; i < MENU_MAX_DIGITS - 1; i++) {
        if (g_Menu->digits[1 + i] != MENU_DIGIT_NONE) {
            ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->firstWideNumberPrimCount +=
                func_8002675C(
                    g_Menu->resources, g_Menu->digits[1 + i],
                    (POLY_FT4*)((u8*)g_Menu->pGearWindowPrims +
                                (((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)
                                         ->firstWideNumberPrimCount *
                                     (2 * sizeof(POLY_FT4)) +
                                 0xCD0)),
                    g_Menu->renderContext, g_MenuGearWindowWideNumberX + i * MENU_DIGIT_SPACING - offset,
                    g_MenuGearWindowWideNumberY, 0x1000);
        }
    }
    for (i = 0; i < ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->firstWideNumberPrimCount;
         i++) {
        POLY_FT4* pPrim;
        s32 primOffset = (i * 2 + g_Menu->renderContext) * sizeof(POLY_FT4);

        pBase = (u8*)g_Menu->pGearWindowPrims;
        pEntry = pBase + primOffset;
        pPrim = (POLY_FT4*)(pEntry + 0xCD0);
        MenuSetQuadVertices((SVECTOR*)((u8*)g_Menu->pGearWindowPrims + 0x23C0 + i * 0x20),
                            pPrim->x0, pPrim->y0,
                            pPrim->x1 - *(volatile u16*)&pPrim->x0,
                            pPrim->y3 - *(volatile u16*)&pPrim->y0);
    }

    MenuParseNumberToDigits(
        g_GameState.characters[g_Menu->pManager->currentCharacterIDs[index]].unk40);
    offset = characterId * 24;
    ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->secondWideNumberPrimCount = 0;
    for (i = 0; i < MENU_MAX_DIGITS - 1; i++) {
        if (g_Menu->digits[1 + i] != MENU_DIGIT_NONE) {
            ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->secondWideNumberPrimCount +=
                func_8002675C(
                    g_Menu->resources, g_Menu->digits[1 + i],
                    (POLY_FT4*)((u8*)g_Menu->pGearWindowPrims +
                                (((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)
                                         ->secondWideNumberPrimCount *
                                     (2 * sizeof(POLY_FT4)) +
                                 0xF50)),
                    g_Menu->renderContext, g_MenuGearWindowWideNumber2X + i * MENU_DIGIT_SPACING - offset,
                    g_MenuGearWindowWideNumber2Y, 0x1000);
        }
    }
    for (i = 0; i < ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->secondWideNumberPrimCount;
         i++) {
        POLY_FT4* pPrim;
        s32 primOffset = (i * 2 + g_Menu->renderContext) * sizeof(POLY_FT4);

        pPrims = (u8*)g_Menu->pGearWindowPrims;
        pBase = pPrims;
        pEntry = pBase + primOffset;
        pPrim = (POLY_FT4*)(pEntry + 0xF50);
        vertOffset = 0x24C0 + i * 0x20;
        MenuSetQuadVertices((SVECTOR*)(vertOffset + (u32)pPrims), pPrim->x0, pPrim->y0,
                            pPrim->x1 - *(volatile u16*)&pPrim->x0,
                            pPrim->y3 - *(volatile u16*)&pPrim->y0);
    }
}
void MenuSetGearWindowWideNumber2Prims(u_char index, u_char characterId) {
    s32 offset;
    s32 i;

    MenuParseNumberToDigits(
        g_GameState.characters[g_Menu->pManager->currentCharacterIDs[index]].unk44);
    offset = characterId * 0x18;
    ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->firstWideNumber2PrimCount = 0;
    for (i = 0; i < MENU_WIDE_NUMBER_DIGIT_COUNT; i++) {
        if (g_Menu->digits[2 + i] != MENU_DIGIT_NONE) {
            s32 primCount = ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->firstWideNumber2PrimCount;

            ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->firstWideNumber2PrimCount += func_8002675C(
                g_Menu->resources, g_Menu->digits[2 + i],
                (POLY_FT4*)((u8*)g_Menu->pGearWindowPrims + (primCount * (2 * sizeof(POLY_FT4)) + 0x11D0)),
                g_Menu->renderContext, i * MENU_DIGIT_SPACING + g_MenuGearWindowWideNumber3X - offset, g_MenuGearWindowWideNumber3Y, 0x1000);
        }
    }
    for (i = 0; i < ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->firstWideNumber2PrimCount; i++) {
        s32 primOffset = (i * 2 + g_Menu->renderContext) * sizeof(POLY_FT4);

        MenuSetQuadVertices((SVECTOR*)((u8*)g_Menu->pGearWindowPrims + (i * 0x20 + 0x25C0)),
                            ((POLY_FT4*)((u8*)g_Menu->pGearWindowPrims + primOffset + 0x11D0))->x0,
                            ((POLY_FT4*)((u8*)g_Menu->pGearWindowPrims + primOffset + 0x11D0))->y0,
                            ((POLY_FT4*)((u8*)g_Menu->pGearWindowPrims + primOffset + 0x11D0))->x1 -
                                *(volatile u16*)&((POLY_FT4*)((u8*)g_Menu->pGearWindowPrims + primOffset + 0x11D0))->x0,
                            ((POLY_FT4*)((u8*)g_Menu->pGearWindowPrims + primOffset + 0x11D0))->y3 -
                                *(volatile u16*)&((POLY_FT4*)((u8*)g_Menu->pGearWindowPrims + primOffset + 0x11D0))->y0);
    }

    MenuParseNumberToDigits(
        g_GameState.characters[g_Menu->pManager->currentCharacterIDs[index]].unk48);
    offset = characterId * 0x18;
    ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->secondWideNumber2PrimCount = 0;
    for (i = 0; i < MENU_WIDE_NUMBER_DIGIT_COUNT; i++) {
        if (g_Menu->digits[2 + i] != MENU_DIGIT_NONE) {
            ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->secondWideNumber2PrimCount += func_8002675C(
                g_Menu->resources, g_Menu->digits[2 + i],
                (POLY_FT4*)((u8*)g_Menu->pGearWindowPrims +
                            (((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->secondWideNumber2PrimCount * (2 * sizeof(POLY_FT4)) + 0x1400)),
                g_Menu->renderContext, i * MENU_DIGIT_SPACING + g_MenuGearWindowWideNumber4X - offset, g_MenuGearWindowWideNumber4Y, 0x1000);
        }
    }
    for (i = 0; i < ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->secondWideNumber2PrimCount; i++) {
        s32 primOffset = (i * 2 + g_Menu->renderContext) * sizeof(POLY_FT4);

        MenuSetQuadVertices((SVECTOR*)((u8*)g_Menu->pGearWindowPrims + (i * 0x20 + 0x26A0)),
                            ((POLY_FT4*)((u8*)g_Menu->pGearWindowPrims + primOffset + 0x1400))->x0,
                            ((POLY_FT4*)((u8*)g_Menu->pGearWindowPrims + primOffset + 0x1400))->y0,
                            ((POLY_FT4*)((u8*)g_Menu->pGearWindowPrims + primOffset + 0x1400))->x1 -
                                *(volatile u16*)&((POLY_FT4*)((u8*)g_Menu->pGearWindowPrims + primOffset + 0x1400))->x0,
                            ((POLY_FT4*)((u8*)g_Menu->pGearWindowPrims + primOffset + 0x1400))->y3 -
                                *(volatile u16*)&((POLY_FT4*)((u8*)g_Menu->pGearWindowPrims + primOffset + 0x1400))->y0);
    }
}

// Best: 99.24% at retail's exact 952 bytes / 238 insns; 28 differing lines.
// Residual: (a) the s4-s7 hand-out is rotated - retail row=$s7, 24*row=$s6, 8*row=$s5, tensDigit=$s4;
// ours 8*row=$s4, 24*row=$s5, row=$s6, tensDigit=$s7, i.e. retail's map is the reverse. `-da -dl -dg`
// on this body gives the hand-out order 8*row (3 refs / 35 insns / size 4 = .343), 24*row (3 / 36 / 4
// = .333), row (4 / 118 / 1, u_char = .068), tensDigit (2 / 77 / 2 = .052), so retail's order needs a
// 2-reference value to outrank 8*row by 6.6x: closed, and a 3-minute permuter run from this body is
// base=180 best=180. (b) the tail preheader's two delay slots are exchanged: retail fills the
// `bnez row == 0` slot with `move s0,zero` and the digit-loop `blez` slot with `li s1,0x2a00`, ours the
// reverse; `for (i = 0, vertOffset = 0x2A00; ...)` measures identically, so it is sched2's pick.
// The 98.82 -> 99.24 win was structural: the digit x is not an `x += 8` walker but the strength-reduced
// giv of `x + i * MENU_DIGIT_SPACING` inside the call (with `i++` inside the `digits[digitOffset + i]`
// subscript), which is what produces retail's preheader `move s1,a0` (40 -> 28 differing lines).
// Measured dead: a second `x = digitX` variable before the loop (98.56; the copy lands in the
// `lw g_Menu` slot, not at the preheader end), and the preset screen gcc-2.6.0 -O3 93.14 / -G0 93.14 /
// 2.7.2 92.68 / cdk 89.05.
// Best: 99.64% at retail's exact 952 bytes / 238 insns (26 differing lines), decomp-permuter
// score 85 against 180 for the round-514 body. Residuals LEFT: one 3-way callee-saved rotation
// plus a register shuffle in the tensDigit prim call's argument block.
//   retail: tensDigit=s4, 24*row=s6, 8*row=s5, row=s7
//   ours:   tensDigit=s6, 24*row=s5, 8*row=s4, row=s7 (row correct)
//   tail:   retail keeps the prim base in t1 and forms the offset in a2 (`lw t1,856(v1)` then
//           `sll a2,t0,0x2 / addu a2,a2,t0 / sll a2,a2,0x4 / addiu a2,a2,7280 / addu a2,a2,t1`);
//           ours holds the base in a2 and the offset in v0/s1 (`addiu s1,v0,7280 / addu a2,s1,a2`).
// What CLOSED in round 515 (all measured, all no-op block boundaries or a value reuse, byte count
// and insn count unchanged at 952 / 238): the prologue save order and BOTH delay-slot fillers now
// match retail - `move s0,zero` in the `bnez row` slot and `li s1,0x2a00` in the loop `blez` slot.
//   (1) `do { if (row == 0) {...} else {...} digitPrimCount = 0; } while (0);`  28 -> 24 lines
//   (2) `do { ... } while (0);` around the row==0 arm's tail, after `scaled = ...`  24 -> 20
//   (3) `vertOffset = tensPrimOffset;` and pass `vertOffset` to the call: this is what exchanges
//       the two delay slots (20 -> 26 lines, but permuter 160 -> 95, objdiff 99.32 -> 99.59)
//   (4) `do { ... } while (0);` around the digit loop's `if`  99.59 -> 99.64, permuter 95 -> 85
//   (5) `do { } while (0);` after the tensDigit prim call
// A 3-minute permuter run from that body: base=85 best=85, closed.
// The rotation reads as arithmetic-unreachable: row / 24*row / 8*row are QI / SI / SI with 4 / 3 / 3
// refs, tensDigit is pinned HI by retail's own `andi a1,s4,0xffff`, so retail's order needs a 2-ref
// 77-insn pseudo to outrank a 3-ref 36-insn one (see lever `a-global-allocno-priority-is`).
// Measured dead: `for (i = 0, vertOffset = 0x2A00; ...)` in either init order (byte-identical, so the
// join block's `li s1,0x2a00` before `move s0,zero` is sched2's pick); `i = 0;` hoisted above the
// `if (row == 0)` (98.99% at 239 insns / 956 bytes: it loses loop.c's iteration-count `blez`); the
// per-arm preset screen (2.6.0 -O3 93.14, -G0 93.14, 2.7.2 92.68, cdk 89.05).
/*
void func_801D7884(u_char index, u_char row) {
    u16 tensDigit;
    s32 characterOffset;
    u32 scaled;
    u16 hundreds;
    s32 remainder;
    s32 count;
    s32 digitOffset;
    s32 x;
    s32 i;
    s32 vertOffset;

    // The character's stat row: characters[] is 0xA4 bytes and statBase/statBonus/statFraction sit 0x2E3/0x2E4/0x2E5 into g_GameState.
    // Retail folds that 0x2E4 into each load's %lo half, so the row is reached through
    // g_GameState (a pointer local would burn a register on the base).
    do {
    if (row == 0) {
        characterOffset = g_Menu->pManager->currentCharacterIDs[index] * sizeof(GameCharacter);
        scaled = (u16)(((*(u8*)((u8*)&g_GameState + 0x2E4 + characterOffset) +
                         *(u8*)((u8*)&g_GameState + 0x2E3 + characterOffset)) *
                            10 +
                        *(u8*)((u8*)&g_GameState + 0x2E5 + characterOffset)) *
                       22);
        do {
        hundreds = scaled / 100;
        remainder = scaled - hundreds * 100;
        count = 3;
        digitOffset = 6;
        tensDigit = remainder / 10;
        MenuParseNumberToDigits(hundreds);
        x = D_801E9D30;
        } while (0);
    } else {
        MenuParseNumberToDigits(g_GameState.gears[*(u8*)((u8*)&g_GameState + 0x30C +
                                                         g_Menu->pManager->currentCharacterIDs[index] *
                                                             sizeof(GameCharacter))]
                                    .weight);
        count = 5;
        digitOffset = 4;
        x = D_801E9D30 + 8;
    }
    ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->digitPrimCount = 0;
    } while (0);
    for (i = 0; i < count; i++) {
        u8 digit = g_Menu->digits[digitOffset + i];

        do {
        if (digit != MENU_DIGIT_NONE) {
            u32 primOffset = ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->digitPrimCount *
                                 (2 * sizeof(POLY_FT4)) + 0x1C70;

            ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->digitPrimCount += func_8002675C(
                g_Menu->resources, digit, (POLY_FT4*)(primOffset + (u32)g_Menu->pGearWindowPrims),
                g_Menu->renderContext, x + i * MENU_DIGIT_SPACING - row * 24, row * 8 + D_801E9D34, 0x1000);
        }
        } while (0);
    }
    if (row == 0) {
        u32 tensPrimOffset = ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->digitPrimCount *
                                 (2 * sizeof(POLY_FT4)) + 0x1C70;

        vertOffset = tensPrimOffset;
        ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->digitPrimCount += func_8002675C(
            g_Menu->resources, tensDigit,
            (POLY_FT4*)(vertOffset + (u32)g_Menu->pGearWindowPrims),
            g_Menu->renderContext, D_801E9D30 + 0x20, D_801E9D34, 0x1000);
        do { } while (0);
    }
    for (i = 0, vertOffset = 0x2A00; i < ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->digitPrimCount; i++) {
        POLY_FT4* pPrim = (POLY_FT4*)((u8*)g_Menu->pGearWindowPrims -
                                      -((i * 2 + g_Menu->renderContext) * sizeof(POLY_FT4)) + 0x1C70);

        MenuSetQuadVertices((SVECTOR*)(vertOffset + (u32)g_Menu->pGearWindowPrims), pPrim->x0,
                            pPrim->y0, pPrim->x1 - *(volatile u16*)&pPrim->x0,
                            pPrim->y3 - *(volatile u16*)&pPrim->y0);
        vertOffset += 0x20;
    }
}*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7_r2", func_801D7884);





void MenuSetGearWindowContents(u_char index, u_char characterId) {
    MenuSetGearWindowNamePrims(index, characterId);
    MenuSetCharacterIconPrims(characterId);
    MenuSetGearWindowHpDigits(index, characterId);
    MenuSetGearWindowMpDigits(index, characterId);
    MenuSetGearWindowLevelDigits(index, characterId);
    MenuSetGearWindowWideNumber2Prims(index, characterId);
    func_801D7884(index, characterId);
    MenuSetGearWindowWideNumberPrims(index, characterId);
    g_Menu->pManager->shouldRenderUnk358Prims = 1;
    ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->renderContext = *(u8*)&g_Menu->renderContext;
}
