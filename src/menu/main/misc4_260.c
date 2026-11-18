#include "common.h"

#include "main/game.h"
#include "system/menu.h"

#define MENU_LABEL_GLYPH_COUNT 9 // glyph ids per character label row (arg5 picks the row)
#define MENU_LABEL_GLYPH_NONE 0xFFFF // g_MenuCharacterLabelGlyphIds entry with no glyph
#define MENU_CHARACTER_WINDOW_COUNT 3 // one window per party character: menu.h's characterWindows[3]
#define MENU_EQUIPMENT_PANEL_ROW_COUNT 5 // rows of the equipment panel (g_Menu->pEquipmentPanel)
#define MENU_CHARACTER_CELL_TIMER_FRAMES 15 // characterCellTimer wraps at this frame count
#define MENU_CHARACTER_CELL_FRAME_COUNT 6 // frames in one character cell animation cycle

void MenuParseNumberToDigits(u32 number);
s32 func_8002675C(MenuResourceDirectory* pDir, u32 index, POLY_FT4* pPrims, s32 context, s32 x, s32 y, s32 scale);

extern s32 D_801E981C[];
extern s32 g_MenuCharacterLabelGlyphIds[];
extern s32 g_MenuNameTextureX[];
extern u8 g_MenuNameTextureY[];
extern u16 g_SystemPalette1;
extern u16 g_SystemPalette2;
void MenuInitializePolyFt4(POLY_FT4* pPoly);
void MenuSetFt4QuadVertices(POLY_FT4* pPoly, u_short x, u_short y, u_char u, u_char v, s32 width, s32 height);

// Character info window: the LV/HP/MP label glyphs and the small portrait, then
// the prim that holds them. arg5 picks the glyph/UV row, the prim width and the
// palette source (the character's own parity or its gear's).
void MenuBuildCharacterLabelsAndPortrait(MenuCharacter* pCharacter, u_char characterIndex, u_char localIndex, s32 arg3, s32 arg4, u_char arg5) {
    POLY_FT4* pPoly;
    s32* pPositionsX = (s32*)arg3;
    s32 i;
    u_short width;
    s32 textureX;

    pCharacter->descriptionStringsLength = 0;
    for (i = 0; i < MENU_LABEL_GLYPH_COUNT; i++) {
        if (g_MenuCharacterLabelGlyphIds[arg5 * MENU_LABEL_GLYPH_COUNT + i] != MENU_LABEL_GLYPH_NONE) {
            pCharacter->descriptionStringsLength += func_8002675C(
                g_Menu->resources, g_MenuCharacterLabelGlyphIds[arg5 * MENU_LABEL_GLYPH_COUNT + i],
                &pCharacter->polysDescriptionStrings[pCharacter->descriptionStringsLength * 2],
                g_Menu->renderContext,
                pPositionsX[i],
                localIndex * 0x38 + ((s32*)arg4)[i],
                0x1000);
        }
    }
    func_8002675C(g_Menu->resources, localIndex + 0x14B, pCharacter->polysPortraitSmall,
                  g_Menu->renderContext, pPositionsX[9], localIndex * 0x38 + ((s32*)arg4)[9], 0x1000);
    MenuInitializePolyFt4(&pCharacter->polys4B0[g_Menu->renderContext]);
    pCharacter->polys4B0[g_Menu->renderContext].tpage = GetTPage(0, 0, 0x180, 0);
    if (arg5 == 0) {
        pCharacter->polys4B0[g_Menu->renderContext].clut = characterIndex % 2 ? g_SystemPalette2 : g_SystemPalette1;
    } else {
        pCharacter->polys4B0[g_Menu->renderContext].clut =
            g_GameState.characters[characterIndex].gearId & 1 ? g_SystemPalette1 : g_SystemPalette2;
    }
    width = arg5 * 24 + 0x48;
    textureX = g_MenuNameTextureX[arg5 * 3 + localIndex];
    pPoly = &pCharacter->polys4B0[g_Menu->renderContext];
    MenuSetFt4QuadVertices(pPoly,
                           pPositionsX[16], ((s32*)arg4)[16] + localIndex * 0x38,
                           (textureX << 2) & 0xFC,
                           g_MenuNameTextureY[(arg5 * 3 + localIndex) * 4],
                           width, FONT_LETTER_HEIGHT);
}
void MenuBuildCharacterLevelDigits(MenuCharacter* pCharacter, u_char characterIndex, u_char localIndex, s32 arg3, s32 arg4) {
    s32* pPositionsY = (s32*)arg4;
    s32 i;
    u_short width;
    s32 textureX;
    u_char digit;

    MenuParseNumberToDigits(g_GameState.characters[characterIndex].level);
    pCharacter->levelStringLength = 0;
    for (i = 0; i < MENU_LEVEL_DIGIT_COUNT; i++) {
        digit = g_Menu->digits[i + 6];
        if (digit != MENU_DIGIT_NONE) {
            pCharacter->levelStringLength += func_8002675C(
                g_Menu->resources, digit,
                &pCharacter->polysLevelString[pCharacter->levelStringLength * 2],
                g_Menu->renderContext,
                i * MENU_DIGIT_SPACING + ((s32*)arg3)[10],
                localIndex * 0x38 + pPositionsY[10],
                0x1000);
        }
    }
    MenuParseNumberToDigits(g_GameState.characters[characterIndex].unk63);
    pCharacter->polys5F0PrimCount = 0;
}

INCLUDE_ASM("asm/menu/nonmatchings/main/misc4_260", func_801CDC6C);
void MenuBuildCharacterPrims(MenuCharacter* pCharacter, u_char arg1, u_char arg2, s32 arg3, s32 arg4, u_char arg5) {
    MenuBuildCharacterLabelsAndPortrait(pCharacter, arg1, arg2, arg3, arg4, arg5);
    MenuBuildCharacterLevelDigits(pCharacter, arg1, arg2, arg3, arg4);
    func_801CDC6C(pCharacter, arg1, arg2, arg3, arg4, arg5);
    pCharacter->shouldRender = 1;
    pCharacter->renderContext = *(u8*)&g_Menu->renderContext;
}
void MenuRenderTransformedQuads(s32 count, SVECTOR* pVertices, POLY_FT4* pPolys, s32 renderContext) {
    s32 i;
    long interpolated;
    long flag;

    for (i = 0; i < count; i++) {
        RotTransPers4(&pVertices[4 * i], &pVertices[4 * i + 1], &pVertices[4 * i + 2],
                      &pVertices[4 * i + 3], (long*)&pPolys[2 * i + renderContext].x0,
                      (long*)&pPolys[2 * i + renderContext].x1, (long*)&pPolys[2 * i + renderContext].x2,
                      (long*)&pPolys[2 * i + renderContext].x3, &interpolated, &flag);
        AddPrim(&g_Menu->pGfxEnv->ot[4], &pPolys[2 * i + renderContext]);
    }
}
void MenuRenderString(int length, POLY_FT4* pPolys, int renderContext) {
    s32 i;

    for (i = 0; i < length; i++) {
        AddPrim(&g_Menu->pGfxEnv->ot[4], &pPolys[2 * i + renderContext]);
    }
}
// The prim is reached index-first (`ctx + array`): the `array[ctx]` spelling gives
// gcc the commuted addu retail does not have.
void MenuRenderSelectionCursor(void) {
    AddPrim(&g_Menu->pGfxEnv->ot[4],
            g_Menu->pCursorPrims->drawModeRenderContext + g_Menu->pCursorPrims->drModes1);
    if (g_Menu->pManager->shouldRenderPointerCursor) {
        AddPrim(&g_Menu->pGfxEnv->ot[4],
                g_Menu->pCursorPrims->cursorRenderContext + g_Menu->pCursorPrims->polysPointerCursor);
    }
}
void MenuRenderPointerCursors(void) {
    s32 i;
    u32 offset;
    MenuPointerCursors* pCursors;

    if (g_Menu->pManager->shouldRenderCursors) {
        for (i = 0; i < 4; i++) {
            pCursors = g_Menu->pCursors;
            if (pCursors->shouldRender[i]) {
                // The prim address is summed in integer space, then added to the block:
                // a pointer sum here gives gcc the commuted `addu a1,a2,a1` retail does
                // not have (99.74).
                offset = (2 * i + pCursors->renderContexts[i]) * sizeof(POLY_FT4);
                AddPrim(&g_Menu->pGfxEnv->ot[4], (void*)(offset + (u32)pCursors));
            }
        }
    }
}
// Gold window prim block (g_Menu->pGoldWindow), the same layout misc7's
// MenuSetGoldDigits builds: the nine digit quads, the 'G' glyph quad per render
// context, then the digit count the quads were packed from.
typedef struct {
    /* 0x000 */ POLY_FT4 digitPrims[18]; // Two per digit, one per render context
    /* 0x2D0 */ POLY_FT4 glyphPrims[2];  // One per render context
    /* 0x320 */ s32 digitPrimCount;
    /* 0x324 */ u_char renderContext;
} MenuGoldWindow; // Size: 0x328
void MenuAddGoldAndPlayTimePrims(void) {
    if (g_Menu->pManager->goldDigitsActive) {
        MenuRenderString(((MenuGoldWindow*)g_Menu->pGoldWindow)->digitPrimCount,
                         ((MenuGoldWindow*)g_Menu->pGoldWindow)->digitPrims,
                         ((MenuGoldWindow*)g_Menu->pGoldWindow)->renderContext);
        AddPrim(&g_Menu->pGfxEnv->ot[4],
                &((MenuGoldWindow*)g_Menu->pGoldWindow)->glyphPrims[((MenuGoldWindow*)g_Menu->pGoldWindow)->renderContext]);
    }
    if (g_Menu->pManager->playTimeDigitsActive) {
        MenuRenderString(7, g_Menu->pPlayTimeDigits->polys[0], g_Menu->pPlayTimeDigits->renderContext);
        MenuRenderString(4, g_Menu->pPlayTimeDigits->polys[7], g_Menu->pPlayTimeDigits->renderContext);
    }
}
// 2.6.0 is what puts the loop increment in retail's `beqz` delay slot, so this
// function only matches on this preset (97.78 under Default).  The two AddPrim
// addresses reach the still opaque four prim table at offset 0 (`unk0` in the
// header) through a byte pointer, and must keep the `&pData[ctx * size + K]`
// subscript spelling: the pointer-sum form loses retail's addiu 0x50 order.
void MenuAddCharacterWindowPrims(void) {
    s32 i;
    MenuCharacterWindow* pWindow;

    for (i = 0; i < MENU_CHARACTER_WINDOW_COUNT; i++) {
        pWindow = g_Menu->characterWindows[i];
        if (g_Menu->pManager->shouldRenderCharacterWindows[i]) {
            AddPrim(&g_Menu->pGfxEnv->ot[4], (void*)&((u8*)pWindow)[pWindow->renderContext * sizeof(POLY_FT4)]);
            AddPrim(&g_Menu->pGfxEnv->ot[4],
                    (void*)&((u8*)pWindow)[pWindow->renderContext * sizeof(POLY_FT4) + 0x50]);
            MenuRenderString(pWindow->iconsPrimCount, pWindow->polysIcons, pWindow->renderContext);
            MenuRenderString(pWindow->hpPrimCount, pWindow->polysHp, pWindow->renderContext);
            MenuRenderString(pWindow->maxHpPrimCount, pWindow->polysMaxHp, pWindow->renderContext);
            MenuRenderString(pWindow->mpPrimCount, pWindow->polysMp, pWindow->renderContext);
            MenuRenderString(pWindow->maxMpPrimCount, pWindow->polysMaxMp, pWindow->renderContext);
            MenuRenderString(pWindow->wideNumberPrimCount, pWindow->polysWideNumber, pWindow->renderContext);
            MenuRenderString(pWindow->levelPrimCount, pWindow->polysLevel, pWindow->renderContext);
        }
    }
}
void MenuAddUnk358Prims(void) {
    u8* pData;

    if (g_Menu->pManager->shouldRenderUnk358Prims) {
        // The body written twice behind an uninitialised test is what makes gcc
        // 2.7.2 keep g_Menu in $v1 here; one copy scores 99.80.
        if (pData) {
            pData = (u8*)g_Menu->pGearWindowPrims;
            MenuRenderTransformedQuads(1, (SVECTOR*)(pData + 0x1EA0), (POLY_FT4*)pData, pData[0x2AE0]);
            pData = (u8*)g_Menu->pGearWindowPrims;
            MenuRenderTransformedQuads(1, (SVECTOR*)(pData + 0x1EC0), (POLY_FT4*)(pData + 0x50), pData[0x2AE0]);
            pData = (u8*)g_Menu->pGearWindowPrims;
            MenuRenderTransformedQuads(pData[0x2AED], (SVECTOR*)(pData + 0x2AA0), (POLY_FT4*)(pData + 0x1E00),
                          pData[0x2AEE]);
            pData = (u8*)g_Menu->pGearWindowPrims;
            MenuRenderTransformedQuads(pData[0x2AEC], (SVECTOR*)(pData + 0x1EE0), (POLY_FT4*)(pData + 0xA0), pData[0x2AE0]);
            pData = (u8*)g_Menu->pGearWindowPrims;
            MenuRenderTransformedQuads(pData[0x2AE7], (SVECTOR*)(pData + 0x2780), (POLY_FT4*)(pData + 0x1630), pData[0x2AE0]);
            pData = (u8*)g_Menu->pGearWindowPrims;
            MenuRenderTransformedQuads(pData[0x2AE8], (SVECTOR*)(pData + 0x2820), (POLY_FT4*)(pData + 0x17C0), pData[0x2AE0]);
            pData = (u8*)g_Menu->pGearWindowPrims;
            MenuRenderTransformedQuads(pData[0x2AE9], (SVECTOR*)(pData + 0x28C0), (POLY_FT4*)(pData + 0x1950), pData[0x2AE0]);
            pData = (u8*)g_Menu->pGearWindowPrims;
            MenuRenderTransformedQuads(pData[0x2AEA], (SVECTOR*)(pData + 0x2960), (POLY_FT4*)(pData + 0x1AE0), pData[0x2AE0]);
            pData = (u8*)g_Menu->pGearWindowPrims;
            MenuRenderTransformedQuads(pData[0x2AE1], (SVECTOR*)(pData + 0x2300), (POLY_FT4*)(pData + 0xAF0), pData[0x2AE0]);
            pData = (u8*)g_Menu->pGearWindowPrims;
            MenuRenderTransformedQuads(pData[0x2AE3], (SVECTOR*)(pData + 0x23C0), (POLY_FT4*)(pData + 0xCD0), pData[0x2AE0]);
            pData = (u8*)g_Menu->pGearWindowPrims;
            MenuRenderTransformedQuads(pData[0x2AE5], (SVECTOR*)(pData + 0x25C0), (POLY_FT4*)(pData + 0x11D0), pData[0x2AE0]);
            pData = (u8*)g_Menu->pGearWindowPrims;
            MenuRenderTransformedQuads(pData[0x2AEB], (SVECTOR*)(pData + 0x2A00), (POLY_FT4*)(pData + 0x1C70), pData[0x2AE0]);
    } else {
            pData = (u8*)g_Menu->pGearWindowPrims;
            MenuRenderTransformedQuads(1, (SVECTOR*)(pData + 0x1EA0), (POLY_FT4*)pData, pData[0x2AE0]);
            pData = (u8*)g_Menu->pGearWindowPrims;
            MenuRenderTransformedQuads(1, (SVECTOR*)(pData + 0x1EC0), (POLY_FT4*)(pData + 0x50), pData[0x2AE0]);
            pData = (u8*)g_Menu->pGearWindowPrims;
            MenuRenderTransformedQuads(pData[0x2AED], (SVECTOR*)(pData + 0x2AA0), (POLY_FT4*)(pData + 0x1E00),
                          pData[0x2AEE]);
            pData = (u8*)g_Menu->pGearWindowPrims;
            MenuRenderTransformedQuads(pData[0x2AEC], (SVECTOR*)(pData + 0x1EE0), (POLY_FT4*)(pData + 0xA0), pData[0x2AE0]);
            pData = (u8*)g_Menu->pGearWindowPrims;
            MenuRenderTransformedQuads(pData[0x2AE7], (SVECTOR*)(pData + 0x2780), (POLY_FT4*)(pData + 0x1630), pData[0x2AE0]);
            pData = (u8*)g_Menu->pGearWindowPrims;
            MenuRenderTransformedQuads(pData[0x2AE8], (SVECTOR*)(pData + 0x2820), (POLY_FT4*)(pData + 0x17C0), pData[0x2AE0]);
            pData = (u8*)g_Menu->pGearWindowPrims;
            MenuRenderTransformedQuads(pData[0x2AE9], (SVECTOR*)(pData + 0x28C0), (POLY_FT4*)(pData + 0x1950), pData[0x2AE0]);
            pData = (u8*)g_Menu->pGearWindowPrims;
            MenuRenderTransformedQuads(pData[0x2AEA], (SVECTOR*)(pData + 0x2960), (POLY_FT4*)(pData + 0x1AE0), pData[0x2AE0]);
            pData = (u8*)g_Menu->pGearWindowPrims;
            MenuRenderTransformedQuads(pData[0x2AE1], (SVECTOR*)(pData + 0x2300), (POLY_FT4*)(pData + 0xAF0), pData[0x2AE0]);
            pData = (u8*)g_Menu->pGearWindowPrims;
            MenuRenderTransformedQuads(pData[0x2AE3], (SVECTOR*)(pData + 0x23C0), (POLY_FT4*)(pData + 0xCD0), pData[0x2AE0]);
            pData = (u8*)g_Menu->pGearWindowPrims;
            MenuRenderTransformedQuads(pData[0x2AE5], (SVECTOR*)(pData + 0x25C0), (POLY_FT4*)(pData + 0x11D0), pData[0x2AE0]);
            pData = (u8*)g_Menu->pGearWindowPrims;
            MenuRenderTransformedQuads(pData[0x2AEB], (SVECTOR*)(pData + 0x2A00), (POLY_FT4*)(pData + 0x1C70), pData[0x2AE0]);
        }
    }
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc4_260", func_801CE860);
/* 88.90% (804 vs 792 bytes) under the Default preset: every expression and call
   is right; the only
   delta is that loop.c strength-reduces `i * 0x20` and `i * 0x80` here, so gcc
   allocates two extra induction registers ($s4/$s5: +0x20 and +0x80 at the
   latch) and computes the verts base as addiu(pData,K) + biv instead of
   retail's `sll; addiu; addu`.  Retail has no mult-giv registers at all - it
   re-emits `sll $a0,$s0,5` in both blocks - so its givs must have kept a
   non-zero add_val (0x31E0 vs 0x3100) and each stayed below the benefit
   threshold.  Tried: the base as a local pointer, `(pData + i)[K]` for the
   byte indexes (that fixed the addu operand order), a shared `rowOfs = i*0x20`
   local, and -O1 (55.6, so not a preset problem).  Same 88.90 in all shapes.
void func_801CE860(void) {
    s32 i;
    s32 offset0 = 0;
    s32 offset1 = 0x2030;
    s32 offset2 = 0x2228;
    long interpolated;
    long flag;
    SVECTOR* pVerts;
    s32 rowOfs;

    for (i = 0; i < 7; i++) {
        rowOfs = i * 0x20;
        if ((((u8*)g_Menu->unk35C) + i)[0x32EA]) {
            if (((u8*)g_Menu->unk35C)[0x32F2]) {
                pVerts = (SVECTOR*)((rowOfs + 0x31E0) + (u32)g_Menu->unk35C);
                RotTransPers4(&pVerts[0], &pVerts[1], &pVerts[2], &pVerts[3],
                              (long*)(offset2 + (u32)g_Menu->unk35C +
                                      (((u8*)g_Menu->unk35C) + i)[0x32E3] * 36 + 8),
                              (long*)(offset2 + (u32)g_Menu->unk35C +
                                      (((u8*)g_Menu->unk35C) + i)[0x32E3] * 36 + 0x10),
                              (long*)(offset2 + (u32)g_Menu->unk35C +
                                      (((u8*)g_Menu->unk35C) + i)[0x32E3] * 36 + 0x18),
                              (long*)(offset2 + (u32)g_Menu->unk35C +
                                      (((u8*)g_Menu->unk35C) + i)[0x32E3] * 36 + 0x20),
                              &interpolated, &flag);
                AddPrim(&g_Menu->pGfxEnv->ot[4],
                        (void*)(offset2 + (u32)g_Menu->unk35C +
                                (((u8*)g_Menu->unk35C) + i)[0x32E3] * 36));
                MenuRenderTransformedQuads((((u8*)g_Menu->unk35C) + i)[0x32CE],
                              (SVECTOR*)((rowOfs * 4 + 0x2D80) + (u32)g_Menu->unk35C),
                              (POLY_FT4*)((offset0 + 0x1770) + (u32)g_Menu->unk35C),
                              (((u8*)g_Menu->unk35C) + i)[0x32D5]);
            }
            pVerts = (SVECTOR*)((rowOfs + 0x3100) + (u32)g_Menu->unk35C);
            RotTransPers4(&pVerts[0], &pVerts[1], &pVerts[2], &pVerts[3],
                          (long*)(offset1 + (u32)g_Menu->unk35C +
                                  (((u8*)g_Menu->unk35C) + i)[0x32DC] * 36 + 8),
                          (long*)(offset1 + (u32)g_Menu->unk35C +
                                  (((u8*)g_Menu->unk35C) + i)[0x32DC] * 36 + 0x10),
                          (long*)(offset1 + (u32)g_Menu->unk35C +
                                  (((u8*)g_Menu->unk35C) + i)[0x32DC] * 36 + 0x18),
                          (long*)(offset1 + (u32)g_Menu->unk35C +
                                  (((u8*)g_Menu->unk35C) + i)[0x32DC] * 36 + 0x20),
                          &interpolated, &flag);
            AddPrim(&g_Menu->pGfxEnv->ot[4],
                    (void*)(offset1 + (u32)g_Menu->unk35C +
                            (((u8*)g_Menu->unk35C) + i)[0x32DC] * 36));
            MenuRenderTransformedQuads((((u8*)g_Menu->unk35C) + i)[0x32C0],
                          (SVECTOR*)((rowOfs * 4 + 0x2A00) + (u32)g_Menu->unk35C),
                          (POLY_FT4*)((offset0 + 0xEB0) + (u32)g_Menu->unk35C),
                          (((u8*)g_Menu->unk35C) + i)[0x32C7]);
        }
        offset0 += 0x140;
        offset1 += 0x48;
        offset2 += 0x48;
    }
}
*/

void MenuAddUnk35CPrims(void) {
    u8* pData;

    if (g_Menu->pManager->shouldRenderUnk35C) {
        pData = (u8*)g_Menu->unk35C;
        MenuRenderTransformedQuads(pData[0x32F3], (SVECTOR*)(pData + 0x2420), (POLY_FT4*)pData, pData[0x32F1]);
        func_801CE860();
    }
}
// Equipment panel block (g_Menu->pEquipmentPanel, 0x2AC bytes, allocated in misc10):
// five rows of a quad prim per render context plus its four source vertices, then one
// visibility flag per row.
typedef struct {
    /* 0x00 */ POLY_FT4 prims[2]; // One per render context
    /* 0x50 */ SVECTOR verts[4];
    /* 0x70 */ u8 unk70[0x10];
} MenuEquipmentPanelRow; // Size: 0x80
typedef struct {
    /* 0x000 */ MenuEquipmentPanelRow rows[MENU_EQUIPMENT_PANEL_ROW_COUNT];
    /* 0x280 */ u8 unk280[0x14];
    /* 0x294 */ u8 shouldRenderRows[MENU_EQUIPMENT_PANEL_ROW_COUNT];
    /* 0x299 */ u_char renderContext;
    /* 0x29A */ u8 unk29A[0x12];
} MenuEquipmentPanel; // Size: 0x2AC
void MenuAddUnk360Prims(void) {
    s32 i;
    MenuEquipmentPanel* pPanel;
    MenuEquipmentPanelRow* pRow;

    if (g_Menu->pManager->shouldRenderPanel[1]) {
        for (i = 0; i < MENU_EQUIPMENT_PANEL_ROW_COUNT; i++) {
            pPanel = (MenuEquipmentPanel*)g_Menu->pEquipmentPanel;
            if (pPanel->shouldRenderRows[i]) {
                // The row address has to be summed in integer space: a pointer sum gives
                // gcc the commuted `addu a2,v1,a2` retail does not have (99.71).
                pRow = (MenuEquipmentPanelRow*)(i * sizeof(MenuEquipmentPanelRow) + (u32)pPanel);
                MenuRenderTransformedQuads(1, pRow->verts, pRow->prims, pPanel->renderContext);
            }
        }
    }
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc4_260", func_801CEC40);
void MenuAddUnk5StringPrims(void) {
    if (g_Menu->pManager->shouldRenderSubmenuStrings) {
        MenuRenderString(g_Menu->pSubmenuPrims->polys500PrimCount, g_Menu->pSubmenuPrims->polys500, g_Menu->pSubmenuPrims->polys500RenderCtx);
        MenuRenderString(g_Menu->pSubmenuPrims->polys0PrimCount, g_Menu->pSubmenuPrims->polys0, g_Menu->pSubmenuPrims->polys0RenderCtx);
    }
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc4_260", func_801CF37C);
/* 98.89% (608 vs 612 bytes) under BOTH presets: everything matches except one
   delay slot.  Retail fills the `character != slotRecordIndexes[i]` branch slot
   with `li v1,0x1` (valid = 1) and leaves a dead `andi v0,v1,0xff` in the
   `character != 0xFF` slot; gcc puts `li v0,0xFF` in the first slot and `li
   v1,0x1` in the second, so the body is one dead u8 truncation short.  That
   means retail's `valid` was truncated once more than `if (!valid)` needs.
   Notes: D_801E981C is the overlay dlabel table in asm/menu/data/246A4.data.s so
   it only needs an extern declaration.  The three colour bytes read the low byte
   at 0x4D4 through a u8* cast (retail uses lbu) and the AddPrim offsets must be
   written as &pData[ctx * N + K] (array subscript): that is what makes the addiu
   land before the addu.  Also tried: else-branch assignment to valid (96%), the
   || form valid = character != 0xFF || ... (worse), a u32 offset local hoisted
   above each AddPrim (95.10%), and offset + pData operand order (96.78%).
void func_801CF37C(void) {
    s32 i;
    u8 character;
    u8 valid;
    u8* pData;
    long interpolated;
    long flag;

    if (g_Menu->cardSlotState != 2) {
        return;
    }
    character = g_Menu->menuUnk2->slotRecordIndexes[D_801E981C[g_Menu->menuUnk2->selectedSlot]];
    for (i = 0; i < 0x20; i++) {
        if (character != g_Menu->menuUnk2->slotRecordIndexes[i]) {
            continue;
        }
        valid = 1;
        if (character == 0xFF) {
            valid = i == D_801E981C[g_Menu->menuUnk2->selectedSlot];
        }
        if (!valid) {
            continue;
        }
        pData = (u8*)g_Menu->pCellPrimBlocks[i];
        *(u8*)(g_Menu->renderContext * 0x18 + (u32)pData + 0xB4) = ((u8*)g_Menu)[0x4D4];
        *(u8*)(g_Menu->renderContext * 0x18 + (u32)pData + 0xB5) = ((u8*)g_Menu)[0x4D4];
        *(u8*)(g_Menu->renderContext * 0x18 + (u32)pData + 0xB6) = ((u8*)g_Menu)[0x4D4];
        RotTransPers4((SVECTOR*)(pData + 0xE0), (SVECTOR*)(pData + 0xE8), (SVECTOR*)(pData + 0xF0),
                      (SVECTOR*)(pData + 0xF8),
                      (long*)(g_Menu->renderContext * 0x18 + (u32)pData + 0xB8),
                      (long*)(g_Menu->renderContext * 0x18 + (u32)pData + 0xBC),
                      (long*)(g_Menu->renderContext * 0x18 + (u32)pData + 0xC0),
                      (long*)(g_Menu->renderContext * 0x18 + (u32)pData + 0xC4), &interpolated, &flag);
        AddPrim(&g_Menu->pGfxEnv->ot[4], (void*)&pData[g_Menu->renderContext * 0x18 + 0xB0]);
        AddPrim(&g_Menu->pGfxEnv->ot[4], (void*)&pData[g_Menu->renderContext * 12 + 0x140]);
    }
}
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc4_260", func_801CF5E4);
INCLUDE_ASM("asm/menu/nonmatchings/main/misc4_260", func_801CF8D8);
/* 72.99% (568 vs 620 bytes). The whole loop sits inside `if (g_Menu->cardSlotState)`
   (retail loads it before the frame adjust for the guard's lbu 0x4D8); the
   old park was missing that test, which is 52 bytes of the gap. Everything else
   is retail structure: 5 loop-invariant constants (x0 0x1E, x1 0x1B, y 0x36,
   scale 0x1000, base 0x4D94), rendered[2] zeroed on the stack, two func_8002675C
   calls and the AddPrim pair. Still open: the register assignment of those
   constants, and the first call's index argument, which retail builds as a u8
   flag - `li v1,1` before the `unk2F` test, cleared when i ==
   D_801E981C[selectedSlot] / 16, then `andi v0,v1,0xFF` + `beqz` choosing
   0x122/0x115 - so it was a named local (`u8 useAlt = 1; if (...) useAlt = 0;`)
   and not the single ternary expression written here. Retail also recomputes
   `j*2` (sll at the loop latch) and copies base into s2 for the loop.
void func_801CF8D8(void) {
    s32 x0 = 0x1E;
    s32 x1 = 0x1B;
    s32 y = 0x36;
    s32 scale = 0x1000;
    s32 base = 0x4D94;
    u8 rendered[2] = { 0, 0 };
    MenuUnk2* pData;
    s32 count;
    s32 i;
    s32 j;

    if (g_Menu->cardSlotState) {
        for (i = 0; i < 2; i++) {
            pData = g_Menu->menuUnk2;
            if (pData->cardStatus[i] && g_Menu->pManager->isPanelActive[5]) {
                func_8002675C(g_Menu->resources,
                              (g_Menu->pManager->shouldRenderCursors && i == D_801E981C[pData->selectedSlot] / 16)
                                  ? 0x122
                                  : 0x115,
                              (POLY_FT4*)((u8*)pData + base), g_Menu->renderContext, x0, y, scale);
                count = func_8002675C(g_Menu->resources, i + 0x162, (POLY_FT4*)((u8*)pData + base + 0x50),
                                      g_Menu->renderContext, x1, y, scale);
                rendered[i] = 1;
            }
            if (rendered[i]) {
                for (j = 0; j < count; j++) {
                    AddPrim(&g_Menu->pGfxEnv->ot[4],
                            (void*)((u32)((u8*)g_Menu->menuUnk2 + base) +
                                    ((2 * j + g_Menu->renderContext) * sizeof(POLY_FT4) + 0x50)));
                }
                AddPrim(&g_Menu->pGfxEnv->ot[4],
                        (void*)((u32)((u8*)g_Menu->menuUnk2 + base) +
                                g_Menu->renderContext * sizeof(POLY_FT4)));
            }
            base += 0xF0;
            x0 += 0x90;
            x1 += 0x90;
        }
    }
}
*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc4_260", func_801CFB48);
// Cell grid panel block (g_Menu->pCellGrid), allocated in func_801CAE08: the quad
// MenuScrollCellGridRow fills, one per render context, the two strings below it and
// the scroll that drives the quad's width.
typedef struct {
    /* 0x000 */ POLY_F4 quads[2];          // The row quad, one per render context
    /* 0x030 */ POLY_FT4 glyphs12[12][2];  // A twelve glyph string, one prim per render context
    /* 0x3F0 */ POLY_FT4 glyphs2[2][2];    // A two glyph string
    /* 0x490 */ u8 unk490[0x320];
    /* 0x7B0 */ s32 scrollX;               // Grows by scrollStep per scroll frame, wraps past 0x100
    /* 0x7B4 */ s32 scrollStep;
    /* 0x7B8 */ u_char renderContext;       // Picks this frame's prim out of every pair above
    /* 0x7B9 */ u8 unk7B9[7];
} MenuCellGridPanel; // Size: 0x7C0
// pCellGrid is a void*, so every access reloads and re-casts it: a pointer local
// lets gcc hoist the load out of the eight quad stores below.  The quad has to be
// reached index-first (`ctx + pGrid->quads`): `pGrid->quads[ctx]` gives gcc the
// commuted `addu a0,a0,v0` retail does not have (98.48).
// The `rem >= 4` form is required (sltiu+beqz), not `rem < 4`.
void MenuScrollCellGridRow(void) {
    s32 value;

    if (g_Menu->pManager->cellGridScrollState == 0) {
        return;
    }
    (((MenuCellGridPanel*)g_Menu->pCellGrid)->renderContext + ((MenuCellGridPanel*)g_Menu->pCellGrid)->quads)->x0 = 0x20;
    (((MenuCellGridPanel*)g_Menu->pCellGrid)->renderContext + ((MenuCellGridPanel*)g_Menu->pCellGrid)->quads)->y0 = 0x61;
    (((MenuCellGridPanel*)g_Menu->pCellGrid)->renderContext + ((MenuCellGridPanel*)g_Menu->pCellGrid)->quads)->x1 =
        ((MenuCellGridPanel*)g_Menu->pCellGrid)->scrollX + 0x20;
    (((MenuCellGridPanel*)g_Menu->pCellGrid)->renderContext + ((MenuCellGridPanel*)g_Menu->pCellGrid)->quads)->y1 = 0x61;
    (((MenuCellGridPanel*)g_Menu->pCellGrid)->renderContext + ((MenuCellGridPanel*)g_Menu->pCellGrid)->quads)->x2 = 0x20;
    (((MenuCellGridPanel*)g_Menu->pCellGrid)->renderContext + ((MenuCellGridPanel*)g_Menu->pCellGrid)->quads)->y2 = 0x68;
    (((MenuCellGridPanel*)g_Menu->pCellGrid)->renderContext + ((MenuCellGridPanel*)g_Menu->pCellGrid)->quads)->x3 =
        ((MenuCellGridPanel*)g_Menu->pCellGrid)->scrollX + 0x20;
    (((MenuCellGridPanel*)g_Menu->pCellGrid)->renderContext + ((MenuCellGridPanel*)g_Menu->pCellGrid)->quads)->y3 = 0x68;
    if (g_Menu->idleFrameCount % 6 >= 4 || g_Menu->pManager->cellGridScrollState == 2) {
        MenuRenderString(2, ((MenuCellGridPanel*)g_Menu->pCellGrid)->glyphs2[0],
                         ((MenuCellGridPanel*)g_Menu->pCellGrid)->renderContext);
    }
    MenuRenderString(12, ((MenuCellGridPanel*)g_Menu->pCellGrid)->glyphs12[0],
                     ((MenuCellGridPanel*)g_Menu->pCellGrid)->renderContext);
    AddPrim(&g_Menu->pGfxEnv->ot[4],
            (((MenuCellGridPanel*)g_Menu->pCellGrid)->renderContext + ((MenuCellGridPanel*)g_Menu->pCellGrid)->quads));
    if (g_Menu->pManager->cellGridScrollState == 1) {
        value = ((MenuCellGridPanel*)g_Menu->pCellGrid)->scrollX + ((MenuCellGridPanel*)g_Menu->pCellGrid)->scrollStep;
        ((MenuCellGridPanel*)g_Menu->pCellGrid)->scrollX = value;
        if (value >= 0x101) {
            ((MenuCellGridPanel*)g_Menu->pCellGrid)->scrollX = 0;
        }
    }
}
void MenuAddCellGridPrims(void) {
    func_801CF37C();
    func_801CF5E4(0, 0, 0x10);
    func_801CF5E4(0x10, 0x10, 0x20);
    func_801CFB48();
    func_801CF8D8();
    MenuScrollCellGridRow();
    if (++g_Menu->characterCellTimer == MENU_CHARACTER_CELL_TIMER_FRAMES) {
        g_Menu->characterCellTimer = 0;
        if (++g_Menu->characterCellFrame == MENU_CHARACTER_CELL_FRAME_COUNT) {
            g_Menu->characterCellFrame = 0;
        }
    }
    if (!g_Menu->characterCellShadeReversing) {
        g_Menu->characterCellShade += 4;
        if (g_Menu->characterCellShade >= 0x81) {
            g_Menu->characterCellShadeReversing = 1;
            g_Menu->characterCellShade = 0x7C;
        }
    } else {
        g_Menu->characterCellShade -= 4;
        if (g_Menu->characterCellShade < 0) {
            g_Menu->characterCellShadeReversing = 0;
            g_Menu->characterCellShade = 4;
        }
    }
}
INCLUDE_ASM("asm/menu/nonmatchings/main/misc4_260", func_801D02D8);
void MenuRenderQuad(SVECTOR* pVertices, POLY_FT4* pPolys, s32 polyIndex, s32 otIndex) {
    long interpolated;
    long flag;

    RotTransPers4(&pVertices[0], &pVertices[1], &pVertices[2], &pVertices[3],
                  (long*)&pPolys[polyIndex].x0, (long*)&pPolys[polyIndex].x1,
                  (long*)&pPolys[polyIndex].x2, (long*)&pPolys[polyIndex].x3,
                  &interpolated, &flag);
    AddPrim(&g_Menu->pGfxEnv->ot[otIndex], &pPolys[polyIndex]);
}
