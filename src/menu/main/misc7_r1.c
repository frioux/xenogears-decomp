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

void func_801D3C4C(u_char a0, u_short a1, u_short a2, u_short a3, u_short a4);
void MenuInitializeWindowBorderCorners(u_char a0, u_short a1, u_short a2, u_short a3, u_short a4);
void func_801D3FF8(u_char a0, u_short a1, u_short a2, u_short a3);
void MenuSetWindowBorderBottom(u_char a0, u_short a1, u_short a2, u_short a3, u_short a4);
void func_801D4688(u_char a0, u_short a1, u_short a2, u_short a3);
void func_801D49D0(u_char a0, u_short a1, u_short a2, u_short a3, u_short a4);
void func_801D4F2C(u_char a0, u_char a1, s32 a2, s32 a3);
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
extern u16 D_801E9B58;
extern u16 D_801E9D38;
extern u16 D_801E9D3C;
extern u16 D_801E9B5C;
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
void func_801D7154(u_char index, u_char characterId);
void func_801D74EC(u_char index, u_char characterId);
void func_801D7884(u_char index, u_char characterId);

void MenuSetCharacterWindowContents(u_char index, u_char characterId) {
    MenuWindowTween* pTween;
    MenuCharacterWindow* pWindow;
    s32 x;
    s32 y;

    if (index == CHARACTER_ID_NONE) {
        return;
    }
    pTween = &((MenuWindowTween*)(void*)g_Menu->windowTweens)[index];
    pWindow = g_Menu->characterWindows[index];
    x = pTween->offsetX / 256 + pTween->startX;
    y = pTween->offsetY / 256 + pTween->startY;
    func_801D4F2C(index, characterId, x, y);
    MenuSetCharacterWindowIcons(index, x, y);
    MenuSetCharacterHpDigits(index, characterId, x, y);
    MenuSetCharacterMpDigits(index, characterId, x, y);
    MenuSetCharacterWideNumberDigits(index, characterId, x, y);
    MenuSetCharacterLevelDigits(index, characterId, x, y);
    g_Menu->pManager->shouldRenderCharacterWindows[index] = 1;
    pWindow->renderContext = *(u8*)&g_Menu->renderContext;
}
// Gold window prims (g_Menu->pGoldWindow): the nine digit quads, the 'G' glyph quad
// per render context and the digit count the quads are packed from.
typedef struct {
    /* 0x000 */ POLY_FT4 digitPrims[18]; // two per digit, one per render context
    /* 0x2D0 */ POLY_FT4 glyphPrims[2]; // one per render context, added by MenuAddGoldAndPlayTimePrims
    /* 0x320 */ s32 digitPrimCount;
    /* 0x324 */ u_char renderContext;
} MenuGoldWindow; // Size: 0x328

