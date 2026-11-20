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
void func_801D6194(u_char characterId);
void func_801D6338(u_char index, u_char characterId);
void func_801D680C(u_char index, u_char characterId);
void func_801D6CF4(u_char index, u_char characterId);
void func_801D7154(u_char index, u_char characterId);
void func_801D74EC(u_char index, u_char characterId);
void func_801D7884(u_char index, u_char characterId);

INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D3C4C);
/* Best: 94.00%. The sibling ShopMenuInitializeScrollBar
   (src/shop_menu/main/misc.c) is this exact function and matches, so the C is
   right; the whole difference is s0<->s1: retail puts the 5th param `height`
   in s1 and the 0x1000 scale in s0, gcc does the reverse. Note func_800263E4
   must be declared with wide (s32) x/y/scale - a u16 there adds an andi 0xFFFF
   after the -8 and drops to 87%. Tried: s32 scale local, u16 x/y/scale for
   func_800263E4 (87.70%). Also gcc-2.7.2-cdk: 93.33 (and the matched
   MenuSetCharacterWideNumberDigits drops to 82.14 under cdk, so this is not a cdk unit). The tie
   is priority(0x1000) > priority(height) in retail, the reverse in 2.7.2; both
   are live across the same two calls with 3 uses each. Round 307: a `s32 scale =
   0x1000;` local used at all three sites measures the same 94.00% (356 bytes).
   void func_801D3C4C(u_char index, u_short x, u_short y, u_short width, u_short height) {
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
*/
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
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D3FF8);
/* Best: 99.89% (836 bytes, the right size). Only difference: retail emits the
   first call's `addiu a1,s4,0x8` (x + 8) and then `addiu s2,a2,-0x8` (the shared
   top), gcc emits the s2 one 8 slots earlier. The 16 UV byte stores on the
   border prims (one POLY_FT4 per render context) and the rest of the decode are
   exact in the spelling the matched MenuSetWindowBorderBottom uses; that
   spelling was re-diffed here at the same 99.89% / 836 bytes.
   Tried: `top` local assigned before/after halfWidth (99.89), `y - 8` inline in
   both calls (99.89, but then the `andi a1,a1,0xFFFF`/store-lw pair swaps
   instead), halfWidth inline (99.89), `(u16)(x + 8)` cast (98.09), `x += 8` then
   `x += halfWidth` (97.01, loses retail's `addiu s0,s0,8`). So it is an emission
   order tie, not a source shape. Note the second call's x must be written back
   into the `x` parameter (`x = halfWidth + 8 + x;`) - a separate local costs a
   register and 4 bytes. Round 121: permuter, 1547 iterations from this body,
   best = base (no improvement); `y - 8` twice inline puts the a1 add in retail's
   slot but moves the s2 add one insn late (98.09). Still an emission-order tie.
   void func_801D3FF8(u_char index, u_short x, u_short y, u_short width) {
       MenuWindow* pWindow = g_Menu->windows[index];
       u16 top;
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
       halfWidth = (width - 0x10) / 2;
       top = y - 8;
       MenuSetQuadVertices(pWindow->vertsWindowBorderTop1, x + 8, top, halfWidth, 0x10);
       x = halfWidth + 8 + x;
       MenuSetQuadVertices(pWindow->vertsWindowBorderTop2, x, top, halfWidth, 0x10);
       for (i = 0; i < 2; i++) {
           MenuSetPolySemiTransGrey((P_TAG*)&pWindow->polysWindowBorderTop[i * 2 + g_Menu->renderContext]);
       }
   }
*/
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
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D4688);
/* Best: 94.26% (844 bytes; retail is 836). Decode is complete - the vertical
   border sibling of func_801D3FF8 (see its note): 16 UV byte stores to
   polysWindowBorderLeft[ctx] / [ctx+2] (values 0x10/0x84/0x20/0x84
   then 0x10/0x8B/0x20/0x8B, repeated for the second prim pair), then
   MenuSetQuadVertices(vertsWindowBorderLeft1, x - 8, y + 8, 0x10, halfHeight) and
   MenuSetQuadVertices(vertsWindowBorderLeft2, x - 8, halfHeight + 8 + y, 0x10,
   halfHeight) where halfHeight = (height - 0x10) / 2, then a 2-iteration
   MenuSetPolySemiTransGrey loop over polysWindowBorderLeft[i * 2 + renderContext].
   The 4th param is the border HEIGHT (it is the MenuSetQuadVertices height arg), and
   the accumulated param must be written back into it (`y = halfHeight + 8 + y;`)
   - retail keeps it in s3. Remaining diff: gcc copies a2 to s3 in the prologue
   and needs one extra g_Menu reload (2 instructions), and the constant
   registers come out t1/t2 vs a2/t1.
void func_801D4688(u_char index, u_short x, u_short y, u_short height) {
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
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D49D0);
/* Best: 94.60%. This is the RIGHT border, the sibling of the parked
   func_801D3FF8/433C/4688 above: member_change_menu/main/misc.c has the same
   function matched as MemberChangeMenuSetWindowBorderRight, with UVs
   16/140/32/140/16/147/32/147 = the 0x10/0x8C/0x20/0x93 stores here and that
   file's source shape kept below. All 16 UV byte stores, both quad calls and the
   2-iteration MenuSetPolySemiTransGrey loop are exact (848 bytes, same insn multiset).
   The whole remaining diff is instruction ORDER inside the one basic block:
   retail interleaves the arg setup with the last 4 UV stores and has its
   `move s3,a2` (the y copy) at store 12, gcc emits source order and copies a2 in
   the entry block. Because retail keeps a2 live until mid-block it allocates
   0x10 to t1 and 0x20 to t2; ours frees a2 at once and uses a2/t1.
   Tried: this shape, u16 right + `y = halfHeight + 8 + y` write-back (the best
   one, kept below) and member_change_menu's exact expressions (94.36).
   void func_801D49D0(u_char index, u_short x, u_short y, u_short width, u_short height) {
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
*/
void MenuSetWindow(u_char index, u_short x, u_short y, u_short width, u_short height,
                   u_char useDefaultMatrix, s32 zIndex, u_char hasScrollBar) {
    MenuWindow* pWindow = g_Menu->windows[index];

    g_Menu->pManager->shouldRenderWindow[index] = 0;
    MenuSetQuadVertices(pWindow->vertsBackground, x, y, width, height);
    MenuInitializeWindowBorderCorners(index, x, y, width, height);
    func_801D3FF8(index, x, y, width);
    MenuSetWindowBorderBottom(index, x, y, width, height);
    func_801D4688(index, x, y, height);
    func_801D49D0(index, x, y, width, height);
    if (hasScrollBar != 0) {
        func_801D3C4C(index, x, y, width, height);
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
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D4F2C);
/* Best: 87.79%. Structure and every called function/prototype is right; what is
   left is allocation and instruction order: (1) retail emits `addu v1,s1,v1`
   for the tpage-store address where gcc commutes it to `addu v1,v1,s1`, (2)
   retail keeps a0 in s3 / a3 in s4, gcc the reverse, (3) in the MenuSetFt4QuadVertices
   arg setup retail does `andi a3,a3,0xFC` after the a1/a2 masks and keeps the
   7th stack store (`sw v0,0x18(sp)`) before the jal, gcc puts it in the delay
   slot. MenuSetFt4QuadVertices must be declared `u_short x, u_short y` HERE (misc12.c
   defines it as `short`): with `short` the caller gets sll/sra 0x10 instead of
   retail's `andi 0xFFFF`.
   void func_801D4F2C(u_char a0, u_char a1, s32 a2, s32 a3) {
       u8* pWindow = (u8*)g_Menu->characterWindows[a0];
       u8* pBase;
   
       func_8002675C(g_Menu->resources, a0 + 0x14B, (POLY_FT4*)pWindow,
                     g_Menu->renderContext, a2, a3, 0x1000);
       MenuInitializePolyFt4((POLY_FT4*)(pWindow + (g_Menu->renderContext * 0x28 + 0x50)));
       *(u16*)(pWindow + g_Menu->renderContext * 0x28 + 0x66) = GetTPage(0, 0, 0x180, 0);
       pBase = pWindow + g_Menu->renderContext * 0x28;
       if (g_Menu->pManager->currentCharacterIDs[a0] & 1) {
           *(u16*)(pBase + 0x5E) = g_SystemPalette2;
       } else {
           *(u16*)(pBase + 0x5E) = g_SystemPalette1;
       }
       MenuSetFt4QuadVertices((POLY_FT4*)(pWindow + (g_Menu->renderContext * 0x28 + 0x50)),
                     D_801E9B58 + a2, D_801E9B5C + a3,
                     (g_MenuNameTextureX[a0] << 2) & 0xFC, g_MenuNameTextureY[a0], 0x48, 0xD);
   }
*/
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
    func_8002675C(g_Menu->resources, 0x10,
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
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D5ED4);
/* Best: 87.38%, frame 0x38 as retail. Decode is complete (this is the sibling of
   func_801D4F2C: same texture/TPage/palette/UV sequence). Facts worth keeping:
   (1) the palette condition is `a1 == 0` and the two paths read
   pManager->currentCharacterIDs[a0] & 1 and
   (g_GameState.characters[ids[a0]].gearId + 0xB) & 1;
   (2) `rowOffset = a1 ? 0x18 : 0` is shared by the 5th arg of func_8002675C
   (0x18 - rowOffset) and by the y of the last MenuSetQuadVertices (D_801E9D38 -
   rowOffset), which is why retail materialises it branchlessly in the prologue
   (sltu/negu/andi) into s4;
   (3) g_MenuNameTextureY is read at a *4 stride, i.e. index ((3 * a1 + a0) * 4), while
   g_MenuNameTextureX uses the plain s32 index - the original table is 4-byte entries
   under two names 0x4C apart;
   (4) width = 0x18 * a1 + 0x48 is assigned just before its first use (after the
   palette if/else), not at the top, and shared by MenuSetFt4QuadVertices and the last
   MenuSetQuadVertices.
   Left: retail hoists the 0x5E store base (pGearWindowPrims + renderContext * 0x28) out
   of the *then* branch but recomputes it in the else branch; a shared pBase
   local gets 87.38%, inline in both branches 85.93%, and the remaining diffs are
   two addu operand orders and a register pick. Also tried: flipping the operand
   order of the pGearWindowPrims+ctx*0x28 sums (no change at all).
   void func_801D5ED4(u_char a0, u_char a1) {
       s32 rowOffset = a1 ? 0x18 : 0;
       u8* pBase;
       u16 width;
   
       func_8002675C(g_Menu->resources, a0 + 0x14B, (POLY_FT4*)g_Menu->pGearWindowPrims,
                     g_Menu->renderContext, 0x18 - rowOffset, 0xE, 0x1000);
       MenuSetQuadVertices((SVECTOR*)((u8*)g_Menu->pGearWindowPrims + 0x1EA0),
                     *(u16*)((g_Menu->renderContext * 0x28 + (u8*)g_Menu->pGearWindowPrims) + 0x8),
                     *(u16*)((g_Menu->renderContext * 0x28 + (u8*)g_Menu->pGearWindowPrims) + 0xA),
                     0x30, 0x30);
       MenuInitializePolyFt4((POLY_FT4*)((g_Menu->renderContext * 0x28 + 0x50) +
                                 (u8*)g_Menu->pGearWindowPrims));
       *(u16*)((u8*)g_Menu->pGearWindowPrims + g_Menu->renderContext * 0x28 + 0x66) =
           GetTPage(0, 0, 0x180, 0);
       pBase = (u8*)g_Menu->pGearWindowPrims + g_Menu->renderContext * 0x28;
       if (a1 == 0) {
           if (g_Menu->pManager->currentCharacterIDs[a0] & 1) {
               *(u16*)(pBase + 0x5E) = g_SystemPalette2;
           } else {
               *(u16*)(pBase + 0x5E) = g_SystemPalette1;
           }
       } else {
           if ((g_GameState.characters[g_Menu->pManager->currentCharacterIDs[a0]].gearId +
                0xB) & 1) {
               *(u16*)(pBase + 0x5E) = g_SystemPalette2;
           } else {
               *(u16*)(pBase + 0x5E) = g_SystemPalette1;
           }
       }
       width = 0x18 * a1 + 0x48;
       MenuSetFt4QuadVertices((POLY_FT4*)((g_Menu->renderContext * 0x28 + 0x50) +
                                 (u8*)g_Menu->pGearWindowPrims),
                     0, 0, (g_MenuNameTextureX[3 * a1 + a0] << 2) & 0xFC,
                     g_MenuNameTextureY[(3 * a1 + a0) * 4], width, 0xD);
       MenuSetQuadVertices((SVECTOR*)((u8*)g_Menu->pGearWindowPrims + 0x1EC0),
                     D_801E9D38 - rowOffset, D_801E9D3C, width, 0xD);
   }
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D6194);
/* Best: 93.40%. Two diffs left. (1) retail has `addiu s0,s0,1` in the delay
   slot of the `digit == 0xFFFF` beq and then does the render; gcc puts the
   increment at the end of the body - the same delay-slot-fill puzzle as the
   MenuSetGoldDigits note. (2) second loop: retail loads lhu 0xA8 twice (a1 for the
   arg, v0 for the width subtraction) and loads x/y straight into a1/a2; gcc
   CSEs the twin lhu and then emits `move a1,v1`/`move a2,t0`.
   Tried: `if (== 0xFFFF) continue;` form (same), outer `if (count > 0)` guard
   around loop 2 (extra beqz+nop - a bare `for` gives retail's `blez`).
   Symbols added: D_801E9B60, D_801E9C20, D_801EA39C (all live in
   asm/menu/data/246A4.data.s).
   Round 121: the increment-in-the-beq-delay-slot shape is `digits[i++]` plus a
   value local (that closed MenuSetCharacterHpDigits/53D0/5BA4 here) - for this function it
   is `pId = &D_801EA39C[a0 * 0x18 + i++]` + `*pId` twice, but taken literally it
   collapses to 77.82% (432 bytes): the address local changes the whole loop, so
   the x/y arrays must be indexed off a separate counter. Loop 2's twin `lhu`
   (0xA8/0xAA loaded once for the x/y args and again for the width/height
   difference, 0x112C8-0x112E0) is NOT a basic-block effect - the two loads sit
   in one block with no branch or store between them - so retail's source must
   spell the two reads in a way gcc 2.7.2 will not unify. Same pattern in
   func_801D74EC (0x11D8/0x11DA) and func_801D7154 (0xCD8/0xCDA).
   void func_801D6194(u_char a0) {
       s32 i;
       u8* pEntry;
   
       ((u8*)g_Menu->pGearWindowPrims)[0x2AEC] = 0;
       for (i = 0; i < 0x18; i++) {
           if (D_801EA39C[a0 * 0x18 + i] == 0xFFFF) continue;
           {
               ((u8*)g_Menu->pGearWindowPrims)[0x2AEC] += func_8002675C(
                   g_Menu->resources,
                   D_801EA39C[a0 * 0x18 + i],
                   (POLY_FT4*)(((u8*)g_Menu->pGearWindowPrims) +
                               (((u8*)g_Menu->pGearWindowPrims)[0x2AEC] * 0x50 + 0xA0)),
                   g_Menu->renderContext,
                   D_801E9B60[a0 * 0x18 + i],
                   D_801E9C20[a0 * 0x18 + i],
                   0x1000);
           }
       }
       {
           for (i = 0; i < ((u8*)g_Menu->pGearWindowPrims)[0x2AEC]; i++) {
               pEntry = (u8*)g_Menu->pGearWindowPrims + (g_Menu->renderContext + i * 2) * 0x28;
               MenuSetQuadVertices((SVECTOR*)((u8*)g_Menu->pGearWindowPrims + 0x1EE0 + i * 0x20),
                             *(u16*)(pEntry + 0xA8),
                             *(u16*)(pEntry + 0xAA),
                             *(u16*)(pEntry + 0xB0) - *(u16*)(pEntry + 0xA8),
                             *(u16*)(pEntry + 0xC2) - *(u16*)(pEntry + 0xAA));
           }
       }
   }
*/
extern s32 D_801E9CF0;
extern s32 D_801E9CF4;
extern s32 D_801E9CF8;
extern s32 D_801E9CFC;

INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D6338);
/* Best: 95.53% at retail's EXACT 1236 bytes (all 309 instructions present). Two
   diffs left, both in the two identical blocks and both only a PLACEMENT:
   (1) retail fills the g_Menu load-delay slot with `addu v0,s5,s0` (the
   `digitOffset + i` sum) where gcc emits its `nop` and puts the sum one slot
   earlier; (2) retail defers the prim address's base add to the jal delay slot
   (`sll/addu/sll a2` then `addiu a2,a2,0x1630` -- wait, it keeps 0x1630 in the
   sll chain and adds the g_Menu->pGearWindowPrims base in the slot) while gcc
   computes `addiu t0,t0,0x1630` on the base FIRST and then `addu a2,a2,t0`, and
   swaps which call-clobbered temp holds the base (t1 retail, t0 here) and the
   count (t0/t1). Same instruction multiset, same registers otherwise
   (s0=i, s1=x/vertOffset, s2=count, s3=24*characterId, s4=index,
   s5=digitOffset, s6=characterId all agree with retail).
   FACTS LEARNED (worth keeping, they got it from 85.01% to here):
   - the two 24/8 offsets must be INLINE (`x - characterId * 24`,
     `characterId * 8 + D_801E9CFC`), not locals: with locals the body is 1200
     bytes (9 instructions short) and 85.01%; inlining gives retail's 1236.
   - the first block's MenuParseNumberToDigits call is SHARED: write
     `x = ...; MenuParseNumberToDigits(...)` in each arm (x BEFORE the call) so
     cross-jumping merges the identical call tails into retail's single jal. The
     second block's arms each keep their OWN jal (there x is assigned AFTER the
     call, so the tails differ) - block B goes 89.62% if you hoist it out.
   - names: block A draws hp into g_Menu->pGearWindowPrims 0x2AE7 (now
     hpPrimCount) at prim base +0x1630, verts +0x2780; block B draws maxHp into
     0x2AE8 (maxHpPrimCount) at +0x17C0, verts +0x2820. x starts at
     D_801E9CF0 / D_801E9CF0 + 8 and D_801E9CF8 / D_801E9CF8 - 0x20, y is
     D_801E9CF4 and D_801E9CFC + 8*characterId, digits are g_Menu->digits[
     digitOffset + i] with digitOffset 6 (3 digits, hp) or 4 (5 digits, gear hp).
   - the quad loops need the twin-lhu volatile minuend
     (`pPrim->x1 - *(volatile u16*)&pPrim->x0`) or gcc CSEs the two loads.
   NOT DONE: the permuter rejects this body ("conversion to non-scalar type
   requested", same message as func_801D7884's park) - almost certainly the
   `(SVECTOR*)(vertOffset + (u32)g_Menu->pGearWindowPrims)` casts; spelling them
   `(SVECTOR*)((u8*)g_Menu->pGearWindowPrims + vertOffset)` may let it parse.
// The character (3 digits of hp/maxHp) or gear (5 digits) window numbers: two
// digit-prim passes and the quad pass that grows each prim to its glyph box.
void func_801D6338(u_char index, u_char characterId) {
    s32 i;
    s32 x;
    s32 count;
    s32 digitOffset;
    s32 vertOffset;

    if (characterId == 0) {
        count = MENU_HP_DIGIT_COUNT;
        digitOffset = 6;
        x = D_801E9CF0;
        MenuParseNumberToDigits(g_GameState.characters[g_Menu->pManager->currentCharacterIDs[index]].hp);
    } else {
        count = MENU_GEAR_HP_DIGIT_COUNT;
        digitOffset = 4;
        x = D_801E9CF0 + 8;
        MenuParseNumberToDigits(g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[index]].gearId].hp);
    }
    ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->hpPrimCount = 0;
    for (i = 0; i < count; ) {
        u8 digit = g_Menu->digits[digitOffset + i++];

        if (digit != MENU_DIGIT_NONE) {
            ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->hpPrimCount += func_8002675C(
                g_Menu->resources, digit,
                (POLY_FT4*)(((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->hpPrimCount *
                                (2 * sizeof(POLY_FT4)) +
                            0x1630 + (u32)g_Menu->pGearWindowPrims),
                g_Menu->renderContext, x - characterId * 24, D_801E9CF4, 0x1000);
        }
        x += 8;
    }
    vertOffset = 0x2780;
    for (i = 0; i < ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->hpPrimCount; i++) {
        POLY_FT4* pPrim = (POLY_FT4*)((u8*)g_Menu->pGearWindowPrims +
                                      (i * 2 + g_Menu->renderContext) * sizeof(POLY_FT4) + 0x1630);

        MenuSetQuadVertices((SVECTOR*)(vertOffset + (u32)g_Menu->pGearWindowPrims), pPrim->x0,
                            pPrim->y0, pPrim->x1 - *(volatile u16*)&pPrim->x0,
                            pPrim->y3 - *(volatile u16*)&pPrim->y0);
        vertOffset += 0x20;
    }

    if (characterId == 0) {
        MenuParseNumberToDigits(g_GameState.characters[g_Menu->pManager->currentCharacterIDs[index]].maxHp);
        x = D_801E9CF8;
    } else {
        MenuParseNumberToDigits(g_GameState.gears[g_GameState.characters[g_Menu->pManager->currentCharacterIDs[index]].gearId].maxHp);
        x = D_801E9CF8 - 0x20;
    }
    ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->maxHpPrimCount = 0;
    for (i = 0; i < count; ) {
        u8 digit = g_Menu->digits[digitOffset + i++];

        if (digit != MENU_DIGIT_NONE) {
            ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->maxHpPrimCount += func_8002675C(
                g_Menu->resources, digit,
                (POLY_FT4*)(((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->maxHpPrimCount *
                                (2 * sizeof(POLY_FT4)) +
                            0x17C0 + (u32)g_Menu->pGearWindowPrims),
                g_Menu->renderContext, x - characterId * 24, characterId * 8 + D_801E9CFC, 0x1000);
        }
        x += 8;
    }
    vertOffset = 0x2820;
    for (i = 0; i < ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->maxHpPrimCount; i++) {
        POLY_FT4* pPrim = (POLY_FT4*)((u8*)g_Menu->pGearWindowPrims +
                                      (i * 2 + g_Menu->renderContext) * sizeof(POLY_FT4) + 0x17C0);

        MenuSetQuadVertices((SVECTOR*)(vertOffset + (u32)g_Menu->pGearWindowPrims), pPrim->x0,
                            pPrim->y0, pPrim->x1 - *(volatile u16*)&pPrim->x0,
                            pPrim->y3 - *(volatile u16*)&pPrim->y0);
        vertOffset += 0x20;
    }
}
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D680C);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D6CF4);
/* Best: 63.92% (892 bytes; retail is 1120 = 0x460). Decode, all seven calls and both
   blocks are laid out below; the 228-byte shortfall is a source shape, not a missing
   statement - start from the first difference, which is that retail computes rowOffset
   (s0 = row * 24) AFTER the first MenuParseNumberToDigits (mine hoists it into the
   prologue) and keeps the row parameter in s3 masked at each use.
   Decode: two identical gear-stat blocks, the shape of the parked func_801D7154.
   Block A: MenuParseNumberToDigits(characters[id].level, g_GameState + 0x2CE), count
   byte 0x2AE1, a 3-digit loop over g_Menu->digits[6 + i] (!= 0xFF) calling
   func_8002675C with 0x50-byte prims at +0xAF0, x = i * 8 + D_801E9CE0 - row * 24,
   y = D_801E9CE4, then a count-time MenuSetQuadVertices loop over SVECTORs at
   +0x2300 + i * 0x20 reading the +0xAF0 prim at context (renderContext + 2 * i):
   x0 +0xAF8, y0 +0xAFA, x1 +0xB00, y3 +0xB12.
   Block B is the same with characters[id].unk63 (0x2CF), count 0x2AE2, prims +0xBE0,
   D_801E9CE8 / D_801E9CEC, verts +0x2360 - plus, per prim, SetShadeTex(pPrim, 0) and
   the three shade stores r0 = 0, g0 = 0x80, b0 = 0 (the same shape as the matched
   MenuSetCharacterUnk63Digits above).
   Both quad loops read x0 and y0 twice each (0xAF8 / 0xAFA and 0xBE8 / 0xBEA): the
   volatile-minuend trick measured in the func_801D7884 park above is used here.
/*
extern s32 D_801E9CE0;
extern s32 D_801E9CE4;
extern s32 D_801E9CE8;
extern s32 D_801E9CEC;

void func_801D6CF4(u_char index, u_char row) {
    s32 characterOffset;
    s32 rowOffset;
    s32 i;
    s32 vertOffset;

    characterOffset = g_Menu->pManager->currentCharacterIDs[index] * sizeof(GameCharacter);
    rowOffset = row * 24;
    MenuParseNumberToDigits(*(u8*)((u8*)&g_GameState + 0x2CE + characterOffset));
    ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk2AE1 = 0;
    for (i = 0; i < 3; i++) {
        u8 digit = g_Menu->digits[6 + i];

        if (digit != MENU_DIGIT_NONE) {
            ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk2AE1 += func_8002675C(
                g_Menu->resources, digit,
                (POLY_FT4*)(((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk2AE1 *
                                (2 * sizeof(POLY_FT4)) +
                            0xAF0 + (u32)g_Menu->pGearWindowPrims),
                g_Menu->renderContext, i * MENU_DIGIT_SPACING + D_801E9CE0 - rowOffset, D_801E9CE4,
                0x1000);
        }
    }
    vertOffset = 0x2300;
    for (i = 0; i < ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk2AE1; i++) {
        POLY_FT4* pPrim = (POLY_FT4*)((u8*)g_Menu->pGearWindowPrims +
                                      (g_Menu->renderContext + i * 2) * sizeof(POLY_FT4) + 0xAF0);

        MenuSetQuadVertices((SVECTOR*)(vertOffset + (u32)g_Menu->pGearWindowPrims), pPrim->x0,
                            pPrim->y0, pPrim->x1 - *(volatile u16*)&pPrim->x0,
                            pPrim->y3 - *(volatile u16*)&pPrim->y0);
        vertOffset += 0x20;
    }

    MenuParseNumberToDigits(*(u8*)((u8*)&g_GameState + 0x2CF + characterOffset));
    ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk2AE2 = 0;
    for (i = 0; i < 3; i++) {
        u8 digit = g_Menu->digits[6 + i];

        if (digit != MENU_DIGIT_NONE) {
            ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk2AE2 += func_8002675C(
                g_Menu->resources, digit,
                (POLY_FT4*)(((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk2AE2 *
                                (2 * sizeof(POLY_FT4)) +
                            0xBE0 + (u32)g_Menu->pGearWindowPrims),
                g_Menu->renderContext, i * MENU_DIGIT_SPACING + D_801E9CE8 - rowOffset, D_801E9CEC,
                0x1000);
        }
    }
    vertOffset = 0x2360;
    for (i = 0; i < ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk2AE2; i++) {
        POLY_FT4* pPrim = (POLY_FT4*)((u8*)g_Menu->pGearWindowPrims +
                                      (g_Menu->renderContext + i * 2) * sizeof(POLY_FT4) + 0xBE0);

        SetShadeTex(pPrim, 0);
        pPrim->r0 = 0;
        pPrim->g0 = 0x80;
        pPrim->b0 = 0;
        MenuSetQuadVertices((SVECTOR*)(vertOffset + (u32)g_Menu->pGearWindowPrims), pPrim->x0,
                            pPrim->y0, pPrim->x1 - *(volatile u16*)&pPrim->x0,
                            pPrim->y3 - *(volatile u16*)&pPrim->y0);
        vertOffset += 0x20;
    }
}
*/
extern s32 D_801E9D10;
extern s32 D_801E9D14;
extern s32 D_801E9D18;
extern s32 D_801E9D1C;
extern s32 D_801E9D20;
extern s32 D_801E9D24;
extern s32 D_801E9D28;
extern s32 D_801E9D2C;
extern s32 D_801E9D30;
extern s32 D_801E9D34;
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D7154);
/* Best: 93.29% (912 bytes, retail frame 0x48 and both loops decoded). Second of
   the two-block gear-stat renders (the sibling func_801D6194 above is the same
   shape one field over). Block A: MenuParseNumberToDigits of the word at
   g_GameState.characters[pManager->currentCharacterIDs[a0]] + 0x44
   (weightFromEquipment|unk46), count byte g_Menu->pGearWindowPrims + 0x2AE3, then an
   8-iteration loop over g_Menu->digits[1 + i] (!= 0xFF) calling func_8002675C
   with 0x50-byte prims at +0xCD0, x = D_801E9D10 + i * 8 - 0x18 * a1, y =
   D_801E9D14, then a count-time loop of MenuSetQuadVertices over SVECTORs at
   +0x23C0 + i * 0x20 reading u16s at entry + 0xCD8/0xCDA/0xCE0/0xCF2 (POLY_FT4
   x0/y0/x1/y3 of the +0xCD0 prim at context (renderContext + 2 * i)). Block B
   repeats it with 0x48, +0x2AE4, +0xF50, D_801E9D18/1C, SVECTORs at +0x24C0 and
   u16s at +0xF58/0xF5A/0xF60/0xF72.
   Remaining diff: (1) retail loads entry+0xCD8 and entry+0xCDA TWICE (once for
   the x/y args, once for the width/height subtraction) and gcc CSEs them into
   one load plus two `move`s - the same un-CSE-able twin `lhu` as the parked
   func_801D6194, so the source must read the fields through two spellings that
   gcc cannot prove identical; (2) the second loop keeps the SVECTOR pointer in
   s1 and the counter in s0 in retail, the reverse here, and retail forms the
   entry address as `addu v1,a0,v1` off pGearWindowPrims where gcc uses a third register
   (t0). Not an allocator tie: fix (1) first, it changes the whole block.
    void func_801D7154(u_char a0, u_char a1) {
        u8* pEntry;
        s32 offset;
        s32 i;

        MenuParseNumberToDigits(
            g_GameState.characters[g_Menu->pManager->currentCharacterIDs[a0]].unk44);
        offset = a1 * 0x18;
        ((u8*)g_Menu->pGearWindowPrims)[0x2AE3] = 0;
        for (i = 0; i < 8; i++) {
            if (g_Menu->digits[1 + i] != 0xFF) {
                ((u8*)g_Menu->pGearWindowPrims)[0x2AE3] += func_8002675C(
                    g_Menu->resources, g_Menu->digits[1 + i],
                    (POLY_FT4*)((u8*)g_Menu->pGearWindowPrims +
                                (((u8*)g_Menu->pGearWindowPrims)[0x2AE3] * 0x50 + 0xCD0)),
                    g_Menu->renderContext, D_801E9D10 + i * 8 - offset, D_801E9D14, 0x1000);
            }
        }
        for (i = 0; i < ((u8*)g_Menu->pGearWindowPrims)[0x2AE3]; i++) {
            pEntry = (u8*)g_Menu->pGearWindowPrims + (g_Menu->renderContext + i * 2) * 0x28;
            MenuSetQuadVertices((SVECTOR*)((u8*)g_Menu->pGearWindowPrims + 0x23C0 + i * 0x20),
                                *(u16*)(pEntry + 0xCD8), *(u16*)(pEntry + 0xCDA),
                                *(u16*)(pEntry + 0xCE0) - *(u16*)(pEntry + 0xCD8),
                                *(u16*)(pEntry + 0xCF2) - *(u16*)(pEntry + 0xCDA));
        }
        MenuParseNumberToDigits(
            g_GameState.characters[g_Menu->pManager->currentCharacterIDs[a0]].unk48);
        ((u8*)g_Menu->pGearWindowPrims)[0x2AE4] = 0;
        for (i = 0; i < 8; i++) {
            if (g_Menu->digits[1 + i] != 0xFF) {
                ((u8*)g_Menu->pGearWindowPrims)[0x2AE4] += func_8002675C(
                    g_Menu->resources, g_Menu->digits[1 + i],
                    (POLY_FT4*)((u8*)g_Menu->pGearWindowPrims +
                                (((u8*)g_Menu->pGearWindowPrims)[0x2AE4] * 0x50 + 0xF50)),
                    g_Menu->renderContext, D_801E9D18 + i * 8 - offset, D_801E9D1C, 0x1000);
            }
        }
        for (i = 0; i < ((u8*)g_Menu->pGearWindowPrims)[0x2AE4]; i++) {
            pEntry = (u8*)g_Menu->pGearWindowPrims + (g_Menu->renderContext + i * 2) * 0x28;
            MenuSetQuadVertices((SVECTOR*)((u8*)g_Menu->pGearWindowPrims + 0x24C0 + i * 0x20),
                                *(u16*)(pEntry + 0xF58), *(u16*)(pEntry + 0xF5A),
                                *(u16*)(pEntry + 0xF60) - *(u16*)(pEntry + 0xF58),
                                *(u16*)(pEntry + 0xF72) - *(u16*)(pEntry + 0xF5A));
        }
    }
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D74EC);
/* Best: 99.83% (920 bytes, retail's exact size). Both halves decode completely and
   the ONLY remaining difference is two `addu` operand orders, one per half:
   retail `addu a2,a2,t1` (prim address) where gcc gives `addu a2,t1,a2`, and
   retail `addu a0,s1,a0` (verts pointer, offset biv + base) where gcc gives
   `addu a0,a0,s1`. Measured: spelling either sum offset-first
   (`(off) + (u32)base`, or the flat `field * 0x50 + K + (u32)base`) gives
   retail's operand order but rotates t0/t1 and drops to 94.43%; an explicit
   `vertOffset` variable (the shape that matches in func_801D7884) is 98.00 and
   `(u8*)base + i * 0x20 + K` flat is 93.17 (928 bytes). A 2-minute permuter run
   (885 iters, base=40) found nothing. Everything else - the twin lhu (via the
   volatile minuend), `(i * 2 + renderContext) * 0x28` (fixes the prim base
   register) and `i * 8 + D_x - offset` - is exact.
void func_801D74EC(u_char index, u_char characterId) {
    s32 offset;
    s32 i;

    MenuParseNumberToDigits(
        g_GameState.characters[g_Menu->pManager->currentCharacterIDs[index]].unk44);
    offset = characterId * 0x18;
    ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk2AE5 = 0;
    for (i = 0; i < 7; i++) {
        if (g_Menu->digits[2 + i] != 0xFF) {
            s32 primCount = ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk2AE5;

            ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk2AE5 += func_8002675C(
                g_Menu->resources, g_Menu->digits[2 + i],
                (POLY_FT4*)((u8*)g_Menu->pGearWindowPrims + (primCount * 0x50 + 0x11D0)),
                g_Menu->renderContext, i * 8 + D_801E9D20 - offset, D_801E9D24, 0x1000);
        }
    }
    for (i = 0; i < ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk2AE5; i++) {
        u8* pEntry = (u8*)g_Menu->pGearWindowPrims + (i * 2 + g_Menu->renderContext) * 0x28;

        MenuSetQuadVertices((SVECTOR*)((u8*)g_Menu->pGearWindowPrims + (i * 0x20 + 0x25C0)),
                            *(u16*)(pEntry + 0x11D8), *(u16*)(pEntry + 0x11DA),
                            *(u16*)(pEntry + 0x11E0) - *(volatile u16*)(pEntry + 0x11D8),
                            *(u16*)(pEntry + 0x11F2) - *(volatile u16*)(pEntry + 0x11DA));
    }

    MenuParseNumberToDigits(
        g_GameState.characters[g_Menu->pManager->currentCharacterIDs[index]].unk48);
    offset = characterId * 0x18;
    ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk2AE6 = 0;
    for (i = 0; i < 7; i++) {
        if (g_Menu->digits[2 + i] != 0xFF) {
            ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk2AE6 += func_8002675C(
                g_Menu->resources, g_Menu->digits[2 + i],
                (POLY_FT4*)((u8*)g_Menu->pGearWindowPrims +
                            (((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk2AE6 * 0x50 + 0x1400)),
                g_Menu->renderContext, i * 8 + D_801E9D28 - offset, D_801E9D2C, 0x1000);
        }
    }
    for (i = 0; i < ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->unk2AE6; i++) {
        u8* pEntry = (u8*)g_Menu->pGearWindowPrims + (i * 2 + g_Menu->renderContext) * 0x28;

        MenuSetQuadVertices((SVECTOR*)((u8*)g_Menu->pGearWindowPrims + (i * 0x20 + 0x26A0)),
                            *(u16*)(pEntry + 0x1408), *(u16*)(pEntry + 0x140A),
                            *(u16*)(pEntry + 0x1410) - *(volatile u16*)(pEntry + 0x1408),
                            *(u16*)(pEntry + 0x1422) - *(volatile u16*)(pEntry + 0x140A));
    }
}
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc7", func_801D7884);
/* Best: 92.68% at retail's exact 952 bytes. Structure, every call, every branch,
   the frame (0x58, s0-s7) and the quad loop are right; what is left is one
   allocator tie in the head: retail ranks `tensDigit` above `row` (s4 = tens,
   s7 = row) and gcc ranks it below (s7 = tens, s6 = row), which rotates the
   whole s4-s7 chain (row*8, row*24) and puts the /100 magic constant in a0
   instead of the free a1, so it cannot hoist to the block start as retail does.
   From global.c's allocno_compare: tens = 100*2refs/90live = 2.2 and
   row = 100*4refs/151live = 2.6, a floor_log2 tie broken by pseudo order.
   Tried: declaring tensDigit first (no change at all), inlining `remainder`
   (90.87%, 944 bytes - the local is required), `(i * 2 + ctx)` operand order
   (92.09 -> 92.68, keep it). The permuter rejects this base
   ("conversion to non-scalar type requested").
void func_801D7884(u_char index, u_char row) {
    u16 tensDigit;
    s32 characterOffset;
    u32 scaled;
    u16 hundreds;
    s32 remainder;
    s32 count;
    s32 digitOffset;
    s32 x;
    s32 vertOffset;
    s32 i;

    // The character's stat row: characters[] is 0xA4 bytes and statBase/statBonus/unk79 sit 0x2E3/0x2E4/0x2E5 into g_GameState.
    // Retail folds that 0x2E4 into each load's %lo half, so the row is reached through
    // g_GameState (a pointer local would burn a register on the base).
    if (row == 0) {
        characterOffset = g_Menu->pManager->currentCharacterIDs[index] * sizeof(GameCharacter);
        scaled = (u16)(((*(u8*)((u8*)&g_GameState + 0x2E4 + characterOffset) +
                         *(u8*)((u8*)&g_GameState + 0x2E3 + characterOffset)) *
                            10 +
                        *(u8*)((u8*)&g_GameState + 0x2E5 + characterOffset)) *
                       22);
        hundreds = scaled / 100;
        remainder = scaled - hundreds * 100;
        count = 3;
        digitOffset = 6;
        tensDigit = remainder / 10;
        MenuParseNumberToDigits(hundreds);
        x = D_801E9D30;
    } else {
        characterOffset = g_Menu->pManager->currentCharacterIDs[index] * sizeof(GameCharacter);
        MenuParseNumberToDigits(g_GameState.gears[*(u8*)((u8*)&g_GameState + 0x30C + characterOffset)].weight);
        count = 5;
        digitOffset = 4;
        x = D_801E9D30 + 8;
    }
    ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->digitPrimCount = 0;
    for (i = 0; i < count; ) {
        u8 digit = g_Menu->digits[digitOffset + i++];

        if (digit != MENU_DIGIT_NONE) {
            ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->digitPrimCount += func_8002675C(
                g_Menu->resources, digit,
                (POLY_FT4*)(((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->digitPrimCount *
                                (2 * sizeof(POLY_FT4)) +
                            0x1C70 + (u32)g_Menu->pGearWindowPrims),
                g_Menu->renderContext, x - row * 24, row * 8 + D_801E9D34, 0x1000);
        }
        x += MENU_DIGIT_SPACING;
    }
    if (row == 0) {
        ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->digitPrimCount += func_8002675C(
            g_Menu->resources, tensDigit,
            (POLY_FT4*)(((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->digitPrimCount *
                            (2 * sizeof(POLY_FT4)) +
                        0x1C70 + (u32)g_Menu->pGearWindowPrims),
            g_Menu->renderContext, D_801E9D30 + 0x20, D_801E9D34, 0x1000);
    }
    vertOffset = 0x2A00;
    for (i = 0; i < ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->digitPrimCount; i++) {
        POLY_FT4* pPrim = (POLY_FT4*)((u8*)g_Menu->pGearWindowPrims +
                                      (i * 2 + g_Menu->renderContext) * sizeof(POLY_FT4) + 0x1C70);

        MenuSetQuadVertices((SVECTOR*)(vertOffset + (u32)g_Menu->pGearWindowPrims), pPrim->x0,
                            pPrim->y0, pPrim->x1 - *(volatile u16*)&pPrim->x0,
                            pPrim->y3 - *(volatile u16*)&pPrim->y0);
        vertOffset += 0x20;
    }
}
*/
void MenuSetGearWindowContents(u_char index, u_char characterId) {
    func_801D5ED4(index, characterId);
    func_801D6194(characterId);
    func_801D6338(index, characterId);
    func_801D680C(index, characterId);
    func_801D6CF4(index, characterId);
    func_801D74EC(index, characterId);
    func_801D7884(index, characterId);
    func_801D7154(index, characterId);
    g_Menu->pManager->shouldRenderUnk358Prims = 1;
    ((MenuGearWindowPrims*)g_Menu->pGearWindowPrims)->renderContext = *(u8*)&g_Menu->renderContext;
}
