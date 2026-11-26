#include "common.h"

#include "main/game.h"
#include "system/menu.h"

#define MENU_LABEL_GLYPH_COUNT 9 // glyph ids per character label row (arg5 picks the row)
#define MENU_LABEL_GLYPH_NONE 0xFFFF // g_MenuCharacterLabelGlyphIds entry with no glyph
#define MENU_EQUIPMENT_PANEL_ROW_COUNT 5 // rows of the equipment panel (g_Menu->pEquipmentPanel)
#define MENU_CHARACTER_CELL_TIMER_FRAMES 15 // characterCellTimer wraps at this frame count
#define MENU_CHARACTER_CELL_FRAME_COUNT 6 // frames in one character cell animation cycle
#define MENU_SELECTION_TINT_DISABLED 0x20 // r0/g0/b0 of a disabled selection menu prim
#define MENU_SELECTION_TINT_NORMAL 0x80 // r0/g0/b0 of an enabled one
#define MENU_GLYPH_CELL_COUNT 0x20 // glyph cells of the menu font, one quad pair each in the prim buffer
#define MENU_ICON_COUNT 16 // icon quads packed in the shared prim buffer

void MenuParseNumberToDigits(u32 number);
s32 func_8002675C(MenuResourceDirectory* pDir, u32 index, POLY_FT4* pPrims, s32 context, s32 x, s32 y, s32 scale);

extern s32 g_MenuCardSlotOrder[];
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

/* 99.29% (1120 bytes = retail's exact size, 280 insns; round 423). ONE residual left, and
   it is cc1's PRE-RELOAD scheduler, not the C: the else arm's `li s6,4` (glyphBase)
   is hoisted to the block top where retail emits it right after the value `lw`
   (levers.md c-two-loads-hoist-the, which names this park).
   ROUND 423 CLOSED THE OTHER RESIDUAL: the prologue parameter copies used to come
   out in parm order (a1 then a3) where retail has a3 then a1; DROPPING the
   `s32* pX = (s32*)arg3;` local and spelling the x arguments `((s32*)arg3)[12]` in
   place fixed it (99.17 -> 99.29, 2 differing lines). So the a3/a1 pair order WAS
   source-reachable after all, through the pseudo structure rather than statement
   order. Measured on the remaining `li`: (value,base,count) 99.29 [this body],
   (value,count,base) 99.26, (count,base,value) 99.26, (count,value,base) 99.26,
   and a `u_char gearId` local first 99.29 (2 lines, same residual); permute.sh
   4 min from this body: base=60 best=60, 1715 iterations.
   Structure worth keeping: `value` is assigned in each arm and parsed once (retail
   has one jal MenuParseNumberToDigits per block); each block re-reads
   `g_Menu->digits[...]` in its loop test; blocks 2 and 4 carry a separate
   placed-glyph counter for the x argument while blocks 1 and 3 use the digit
   index; blocks 3 and 4 only run for the character arm (arg5 == 0).
   Round 436 re-measured 99.29 (1120 bytes = retail's size, 280 insns) and added two dead
   spellings: an empty `do { } while (0)` between the value load and the base assignment is
   byte-identical 99.29 (the round-433 barrier that pins a `li` below a STORE does not work
   across a load), and the else arm written as its own `gearId` statement, then glyphCount,
   then value, then glyphBase - retail's exact stream order with scheduling off - is 99.26 with
   FOUR differing lines, because sched1 hoists the FIRST `li` (count) to the block top and drops
   the second (base) into the `lbu` delay slot.  So the statement order really is irrelevant here
   and only `-fno-schedule-insns` reproduces retail (-O1, which turns sched1 off, is 69.74).  */
/*
void func_801CDC6C(MenuCharacter* pCharacter, u_char characterIndex, u_char localIndex, s32 arg3, s32 arg4, u_char arg5) {
    s32* pY = (s32*)arg4;
    u32 value;
    s32 glyphCount;
    s32 glyphBase;
    s32 i;
    s32 j;
    u_char digit;

    if (arg5 == 0) {
        value = g_GameState.characters[characterIndex].hp;
        glyphCount = 3;
        glyphBase = 6;
    } else {
        value = g_GameState.gears[g_GameState.characters[characterIndex].gearId].hp;
        glyphBase = 4;
        glyphCount = 5;
    }
    MenuParseNumberToDigits(value);
    pCharacter->hpStringLength = 0;
    for (i = 0; i < glyphCount; i++) {
        digit = g_Menu->digits[glyphBase + i];
        if (digit != MENU_DIGIT_NONE) {
            pCharacter->hpStringLength += func_8002675C(g_Menu->resources, digit,
                &pCharacter->polysHpString[pCharacter->hpStringLength * 2], g_Menu->renderContext,
                i * MENU_DIGIT_SPACING + ((s32*)arg3)[12], localIndex * 0x38 + pY[12], 0x1000);
        }
    }
    if (arg5 == 0) {
        value = g_GameState.characters[characterIndex].maxHp;
    } else {
        value = g_GameState.gears[g_GameState.characters[characterIndex].gearId].maxHp;
    }
    MenuParseNumberToDigits(value);
    pCharacter->maxHpStringLength = 0;
    for (i = 0, j = 0; i < glyphCount; i++) {
        digit = g_Menu->digits[glyphBase + i];
        if (digit != MENU_DIGIT_NONE) {
            pCharacter->maxHpStringLength += func_8002675C(g_Menu->resources, digit,
                &pCharacter->polysMaxHpString[pCharacter->maxHpStringLength * 2], g_Menu->renderContext,
                j * MENU_DIGIT_SPACING + ((s32*)arg3)[13], localIndex * 0x38 + pY[13], 0x1000);
            j++;
        }
    }
    if (arg5 == 0) {
        MenuParseNumberToDigits(g_GameState.characters[characterIndex].mp);
        pCharacter->mpStringLength = 0;
        for (i = 0; i < 2; i++) {
            digit = g_Menu->digits[7 + i];
            if (digit != MENU_DIGIT_NONE) {
                pCharacter->mpStringLength += func_8002675C(g_Menu->resources, digit,
                    &pCharacter->polysMpString[pCharacter->mpStringLength * 2], g_Menu->renderContext,
                    i * MENU_DIGIT_SPACING + ((s32*)arg3)[14], localIndex * 0x38 + pY[14], 0x1000);
            }
        }
        MenuParseNumberToDigits(g_GameState.characters[characterIndex].maxMp);
        pCharacter->maxMpStringLength = 0;
        for (i = 0, j = 0; i < 2; i++) {
            digit = g_Menu->digits[7 + i];
            if (digit != MENU_DIGIT_NONE) {
                pCharacter->maxMpStringLength += func_8002675C(g_Menu->resources, digit,
                    &pCharacter->polysMaxMpString[pCharacter->maxMpStringLength * 2], g_Menu->renderContext,
                    j * MENU_DIGIT_SPACING + ((s32*)arg3)[15], localIndex * 0x38 + pY[15], 0x1000);
                j++;
            }
        }
    }
}
*/
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
// 90.29% (796 vs 792 bytes, 199 vs 191 insns) under XenoMenu260 (gcc 2.6.0, -O2 -G8). The body is retail's except for one loop-level cause: gcc strength reduction turns i*0x20 and i*0x80 into two extra mult givs (callee-saved s4/s5, +0x20/+0x80 at the latch), which pushes the frame to 0x48 with six saves and re-forms the verts base as addiu(pData,K) plus the giv instead of retail's sll, addiu, addu. DECISIVE: compiling this body with cc1_probe.sh --opt '-O2 -fno-strength-reduce' reproduces retail's frame 0x48, saved set s0-s3+ra, 191 insns and BOTH sll sites (sll a0,s0,5 and sll a1,s0,7); the only residue left there is operand side (retail puts the constant on the index, that build on the base) and pData in a2 vs v1. So the residual is a per-file flag question, not a source one: report PRESET_NO_STRENGTH_REDUCE and do not edit gears.toml. C-level handles measured: s32 i with the realistic body 88.90, 804 B, 201 insns; s16 i 89.59, 800 B, 199 (kills the givs because a HImode biv makes the mult operand a subreg, so general_induction_var refuses, but costs sll 16 and sra 16 per iteration); s32 i plus an s16 row = i copy used only in the four products 90.29, 796 B, 199 (parked below). Measured and worse: all four products inline with no accumulator locals 85.24, 828 B, 207; i shl 5 and i shl 7 for the products 88.90 identical; a rowOfs local for i*0x20 88.90.
   Round 500 measured the FLAG BUILD ITSELF (optimization_flag = -O2 -fno-strength-reduce straight on this unit's cc edge in build.ninja, then diff_func, so objdiff scores it rather than cc1_probe reading it): with the s16 row copy dropped to the plain s32 i body it is 91.68% / 776 B / 194 insns, and the residues that survive are NOT the givs: the prologue save order (i is saved last where retail saves s0 first), g_Menu->unk35C in $a2 in the first block where retail has $v1 (so the constant rides the base: addiu v0,a2,12768 then addu a0,v1,v0, against retail's addiu a0,a0,12768 then addu a0,a0,v1), and retail's sll a1,s0,0x7 hoisted into the AddPrim jal slot where this build emits nop then the sll. So switching the preset buys no match on this function and is NOT worth adding on its account. The parked s16-row body's 199 insns are exactly retail's 191 plus the four sll 16 / sra 16 pairs, i.e. the giv killing is free of any other structural change: kill loop.c's shift givs under -O2 without a per-use subreg extension and this closes.
/*
void func_801CE860(void) {
    s32 i;
    s16 row;
    s32 offset0 = 0;
    s32 offset1 = 0x2030;
    s32 offset2 = 0x2228;
    long interpolated;
    long flag;
    SVECTOR* pVerts;

    for (i = 0; i < 7; i++) {
        row = i;
        if ((((u8*)g_Menu->unk35C) + i)[0x32EA]) {
            if (((u8*)g_Menu->unk35C)[0x32F2]) {
                pVerts = (SVECTOR*)((row * 0x20 + 0x31E0) + (u32)g_Menu->unk35C);
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
                              (SVECTOR*)((row * 0x80 + 0x2D80) + (u32)g_Menu->unk35C),
                              (POLY_FT4*)((offset0 + 0x1770) + (u32)g_Menu->unk35C),
                              (((u8*)g_Menu->unk35C) + i)[0x32D5]);
            }
            pVerts = (SVECTOR*)((row * 0x20 + 0x3100) + (u32)g_Menu->unk35C);
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
                          (SVECTOR*)((row * 0x80 + 0x2A00) + (u32)g_Menu->unk35C),
                          (POLY_FT4*)((offset0 + 0xEB0) + (u32)g_Menu->unk35C),
                          (((u8*)g_Menu->unk35C) + i)[0x32C7]);
        }
        offset0 += 0x140;
        offset1 += 0x48;
        offset2 += 0x48;
    }
}*/
INCLUDE_ASM("asm/menu/nonmatchings/main/misc4_260", func_801CE860);



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
// The selection menu's own prims.  Every cursor and text quad is re-tinted (0x20
// dimmed while the option is disabled, 0x80 normal) the frame the disabled state
// changes, then both prim runs go into the ordering table.  Each store re-reads
// g_Menu->pSelectionMenu: the prim block is reached through two global loads and
// every store invalidates them.
void MenuAddSelectionMenuPrims(void) {
    if (!g_Menu->pManager->shouldRenderSelectionMenu) {
        return;
    }
    if (g_Menu->pSelectionMenu->isDisabled != g_Menu->pSelectionMenu->isDisabledRendered) {
        s32 i;

        if (g_Menu->pSelectionMenu->isDisabled) {
            for (i = 0; i < g_Menu->pSelectionMenu->numTexts; i++) {
                SetSemiTrans(&g_Menu->pSelectionMenu->polysTexts[i * 2 + g_Menu->pSelectionMenu->textsRenderCtx], 1);
                SetShadeTex(&g_Menu->pSelectionMenu->polysTexts[i * 2 + g_Menu->pSelectionMenu->textsRenderCtx], 0);
                g_Menu->pSelectionMenu->polysTexts[i * 2 + g_Menu->pSelectionMenu->textsRenderCtx].tpage |= 0x20;
                (&g_Menu->pSelectionMenu->polysTexts[i * 2 + g_Menu->pSelectionMenu->textsRenderCtx])->r0 = MENU_SELECTION_TINT_DISABLED;
                (&g_Menu->pSelectionMenu->polysTexts[i * 2 + g_Menu->pSelectionMenu->textsRenderCtx])->g0 = MENU_SELECTION_TINT_DISABLED;
                (&g_Menu->pSelectionMenu->polysTexts[i * 2 + g_Menu->pSelectionMenu->textsRenderCtx])->b0 = MENU_SELECTION_TINT_DISABLED;
            }
            for (i = 0; i < g_Menu->pSelectionMenu->numCursors; i++) {
                SetSemiTrans(&g_Menu->pSelectionMenu->polysCursors[i * 2 + g_Menu->pSelectionMenu->cursorsRenderCtx], 1);
                SetShadeTex(&g_Menu->pSelectionMenu->polysCursors[i * 2 + g_Menu->pSelectionMenu->cursorsRenderCtx], 0);
                g_Menu->pSelectionMenu->polysCursors[i * 2 + g_Menu->pSelectionMenu->cursorsRenderCtx].tpage |= 0x20;
                (&g_Menu->pSelectionMenu->polysCursors[i * 2 + g_Menu->pSelectionMenu->cursorsRenderCtx])->r0 = MENU_SELECTION_TINT_DISABLED;
                (&g_Menu->pSelectionMenu->polysCursors[i * 2 + g_Menu->pSelectionMenu->cursorsRenderCtx])->g0 = MENU_SELECTION_TINT_DISABLED;
                (&g_Menu->pSelectionMenu->polysCursors[i * 2 + g_Menu->pSelectionMenu->cursorsRenderCtx])->b0 = MENU_SELECTION_TINT_DISABLED;
            }
        } else {
            for (i = 0; i < g_Menu->pSelectionMenu->numTexts; i++) {
                SetSemiTrans(&g_Menu->pSelectionMenu->polysTexts[i * 2 + g_Menu->pSelectionMenu->textsRenderCtx], 0);
                SetShadeTex(&g_Menu->pSelectionMenu->polysTexts[i * 2 + g_Menu->pSelectionMenu->textsRenderCtx], 0);
                g_Menu->pSelectionMenu->polysTexts[i * 2 + g_Menu->pSelectionMenu->textsRenderCtx].tpage |= 0x20;
                (&g_Menu->pSelectionMenu->polysTexts[i * 2 + g_Menu->pSelectionMenu->textsRenderCtx])->r0 = MENU_SELECTION_TINT_NORMAL;
                (&g_Menu->pSelectionMenu->polysTexts[i * 2 + g_Menu->pSelectionMenu->textsRenderCtx])->g0 = MENU_SELECTION_TINT_NORMAL;
                (&g_Menu->pSelectionMenu->polysTexts[i * 2 + g_Menu->pSelectionMenu->textsRenderCtx])->b0 = MENU_SELECTION_TINT_NORMAL;
            }
            for (i = 0; i < g_Menu->pSelectionMenu->numCursors; i++) {
                SetSemiTrans(&g_Menu->pSelectionMenu->polysCursors[i * 2 + g_Menu->pSelectionMenu->cursorsRenderCtx], 0);
                SetShadeTex(&g_Menu->pSelectionMenu->polysCursors[i * 2 + g_Menu->pSelectionMenu->cursorsRenderCtx], 0);
                g_Menu->pSelectionMenu->polysCursors[i * 2 + g_Menu->pSelectionMenu->cursorsRenderCtx].tpage |= 0x20;
                (&g_Menu->pSelectionMenu->polysCursors[i * 2 + g_Menu->pSelectionMenu->cursorsRenderCtx])->r0 = MENU_SELECTION_TINT_NORMAL;
                (&g_Menu->pSelectionMenu->polysCursors[i * 2 + g_Menu->pSelectionMenu->cursorsRenderCtx])->g0 = MENU_SELECTION_TINT_NORMAL;
                (&g_Menu->pSelectionMenu->polysCursors[i * 2 + g_Menu->pSelectionMenu->cursorsRenderCtx])->b0 = MENU_SELECTION_TINT_NORMAL;
            }
        }
        g_Menu->pSelectionMenu->isDisabledRendered = g_Menu->pSelectionMenu->isDisabled;
    }
    MenuRenderString(g_Menu->pSelectionMenu->numTexts, g_Menu->pSelectionMenu->polysTexts,
                     g_Menu->pSelectionMenu->textsRenderCtx);
    MenuRenderString(g_Menu->pSelectionMenu->numCursors, g_Menu->pSelectionMenu->polysCursors,
                     g_Menu->pSelectionMenu->cursorsRenderCtx);
}
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
   Re-measured round 394 (2.6.0, 152 insns): the missing instruction is the QI mask on
   the constant-1 edge.  Retail masks on BOTH edges into the merge block (in the second
   bne's delay slot for the constant edge, after `move v1,v0` in the 0xFF arm) and the
   merge label holds only `beqz v0`; this build folds the (u8)1 and emits one `andi` AT
   the merge.  Measured this round, all short: (u8) casts at the assignment and the test
   98.89 (identical); `valid = character != 0xFF || i == order` 95.51 (624 B, re-loads
   g_Menu per use); `valid = character == 0xFF ? i == order : 1` 95.64 (628 B);
   permute.sh 3 min base=314 best=314.  A source shape that masks the constant edge must
   also keep the g_Menu reloads where retail has them, and the || form does not.
   Notes: g_MenuCardSlotOrder is the overlay dlabel table in asm/menu/data/246A4.data.s so
   it only needs an extern declaration.  The three colour bytes read the low byte
   at 0x4D4 through a u8* cast (retail uses lbu) and the AddPrim offsets must be
   written as &pData[ctx * N + K] (array subscript): that is what makes the addiu
   land before the addu.  Also tried: else-branch assignment to valid (96%), the
   || form valid = character != 0xFF || ... (worse), a u32 offset local hoisted
   above each AddPrim (95.10%), and offset + pData operand order (96.78%).
   98.89% (608 vs 612 bytes, 153 vs 154 insns) under 2.6.0 at -O2 and -O3 both:
   retail duplicates the merge block's u8 conversion into BOTH predecessors (andi v0,v1,0xff
   in the second bne delay slot for the const edge, and again after move v1,v0 at the end of
   the 0xFF arm) and keeps only beqz at the merge; this build emits ONE andi AT the merge and
   puts li v1,0x1 in the second slot. Same source, one dead mask short, so never linkable.
   Tried this round: the arm carrying its own 'if (!valid) continue;' plus the outer one
   (costing the arm its own andi+beqz, the two compare tails do NOT cross-jump because they
   differ by the reorg nop: 96.94 / 624 B / 156 insns); explicit if/else with both arms
   assigning valid (const edge then keeps li v0,1 + andi v0,v0,0xff, i.e. the constant
   conversion does survive when the blocks are split, but the body reloads g_Menu and gains a
   j: 95.64 / 628 B). Nothing here adds back exactly one andi. Already tried in earlier rounds:
   (u8) casts at assignment and test (identical), || form (95.51), ternary (95.64), else-branch
   assignment (96%), u32 offset local above AddPrim (95.10), offset+pData operand order
   (96.78), permute.sh 3 min base=314 best=314. -O1 75.79, 2.7.2-cdk compile-fails.
   Notes kept from before: g_MenuCardSlotOrder needs only an extern; the three colour bytes
   read g_Menu+0x4D4 through a u8* cast (retail lbu); AddPrim offsets must be &pData[ctx*N+K]
   so the addiu lands before the addu.
   Round 432 re-measured this body at 98.89 / 153 insns (608 B) and re-confirmed both if/else
   arm orders: `if (character != 0xFF) valid = 1; else ...` 96.03 (620 B, 155 insns - it keeps
   the constant edge's mask but moves it into the arm, drops the `move v1,v0` and mirrors the
   g_Menu loads to $v0/$v1) and the 0xFF-first order 95.64 (628 B, 157 insns, `j` + reloads).
   `if (valid == 0)` compiles byte-identically to `if (!valid)`.
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
    character = g_Menu->menuUnk2->slotRecordIndexes[g_MenuCardSlotOrder[g_Menu->menuUnk2->selectedSlot]];
    for (i = 0; i < 0x20; i++) {
        if (character != g_Menu->menuUnk2->slotRecordIndexes[i]) {
            continue;
        }
        valid = 1;
        if (character == 0xFF) {
            valid = i == g_MenuCardSlotOrder[g_Menu->menuUnk2->selectedSlot];
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


// Partial view of the per-character-cell prim block g_Menu->pCellPrimBlocks points at
// (allocated in misc11's MenuCreateCellPrimBlocks): the glyph quad every render context
// gets, the two line prims, and the source vertices the quad is rendered from.
typedef struct {
    /* 0x000 */ POLY_FT4 quads[2]; // Glyph quad, one per render context
    /* 0x050 */ LINE_F3 lines[2][2]; // [line][render context]
    /* 0x0B0 */ u8 unkB0[0x30];
    /* 0x0E0 */ SVECTOR quadVerts[4]; // Source vertices of quads
    /* 0x100 */ SVECTOR lineVerts[2][4]; // Source vertices of lines[line]
} MenuCellPrimBlock; // Size: 0x140
// Fills the cell glyph quads of the cells [start, end): one prim per chunk of the
// record, reduced against the chunk's own vertex ring.  Retail re-reads the chunk
// count and the colour table per iteration and emits `j++` ahead of the count load,
// so the counter is bumped at the end of the body, not in the for clause; the
// do/while(0) block schedules the tail (v3/clut/call) after the first seven stores.
void MenuBuildCellGlyphQuads(s32 start, s32 primIndex, s32 end) {
    s32 i;
    s32 j;
    s32 last = end;
    s32 prim;
    MenuCellPrimBlock* pBlock;
    u16 clut;

    i = start;
    prim = primIndex;
    for (; i < last; i++) {
        if (g_Menu->menuUnk2->records[i].unk58) {
            for (j = 0; j < g_Menu->menuUnk2->chunks[i].pad0[3]; ) {
                pBlock = (MenuCellPrimBlock*)g_Menu->pCellPrimBlocks[prim];
                // The render context is summed in first (`ctx + pBlock->quads`): the struct
                // subscript is the same address but emits the commuted `addu v0,s0,v0`
                // where retail has `addu v0,v0,s0` (99.58).
                (g_Menu->renderContext + pBlock->quads)->u0 = i * 16;
                (g_Menu->renderContext + pBlock->quads)->v0 = g_Menu->menuUnk2->records[i].unk0[g_Menu->characterCellFrame * 4];
                (g_Menu->renderContext + pBlock->quads)->u1 = i * 16 + 16;
                (g_Menu->renderContext + pBlock->quads)->v1 = g_Menu->menuUnk2->records[i].unk0[g_Menu->characterCellFrame * 4];
                (g_Menu->renderContext + pBlock->quads)->u2 = i * 16;
                (g_Menu->renderContext + pBlock->quads)->v2 = g_Menu->menuUnk2->records[i].unk0[g_Menu->characterCellFrame * 4] + 16;
                (g_Menu->renderContext + pBlock->quads)->u3 = i * 16 + 16;
                do {
                    (g_Menu->renderContext + pBlock->quads)->v3 =
                        g_Menu->menuUnk2->records[i].unk0[g_Menu->characterCellFrame * 4] + 16;
                    clut = GetClut(i * 16, i / 16 + 0x1C1);
                    // Base-first sum: retail's clut store is `addu v1,s0,v1`, the reverse
                    // of the eight vertex stores above (99.95 with the index-first form here).
                    pBlock->quads[g_Menu->renderContext].clut = clut;
                    MenuRenderTransformedQuads(1, pBlock->quadVerts, pBlock->quads, g_Menu->renderContext);
                } while (0);
                j++;
                prim++;
            }
        }
    }
}

// Cursor and glyph prim block of one memory card, in MenuUnk2's anonymous blob at 0x4D94
// (one block per card): a cursor prim per render context, then the card's slot number's
// glyph prims, each glyph also a context pair.
typedef struct {
    /* 0x00 */ POLY_FT4 cursors[2]; // Cursor prim, one per render context
    /* 0x50 */ POLY_FT4 glyphs[4]; // Glyph j's prim for context c at 2 * j + c
} MenuCardPrimBlock; // Size: 0xF0

void MenuRenderCardSlotPrims(void) {
    MenuUnk2* pData;
    u_char useAlt;
    u8 rendered[MENU_CARD_COUNT];
    // retail's frame is 0x58 with its save block at 0x30: 8 bytes of its local area at
    // 0x22..0x2F are never touched, which only an unused local reproduces.
    u8 unusedFrame[8];
    s32 x0;
    s32 x1;
    s32 y;
    s32 scale;
    s32 base;
    s32 count;
    s32 slotOffset;
    s32 i;
    s32 j;

    rendered[1] = 0;
    rendered[0] = 0;
    if (g_Menu->cardSlotState) {
        i = 0;
        y = 0x36;
        scale = 0x1000;
        base = 0x4D94;
        x1 = 0x1B;
        x0 = 0x1E;
        for (; i < MENU_CARD_COUNT; i++) {
            pData = g_Menu->menuUnk2;
            if (pData->cardStatus[i] && g_Menu->pManager->isPanelActive[5]) {
                useAlt = 1;
                if (g_Menu->pManager->shouldRenderCursors && i == g_MenuCardSlotOrder[pData->selectedSlot] / 16) {
                    useAlt = 0;
                }
                if (useAlt) {
                    func_8002675C(g_Menu->resources, 0x115, ((MenuCardPrimBlock*)((u8*)g_Menu->menuUnk2 + base))->cursors,
                                  g_Menu->renderContext, x0, y, scale);
                } else {
                    func_8002675C(g_Menu->resources, 0x122, ((MenuCardPrimBlock*)((u8*)g_Menu->menuUnk2 + base))->cursors,
                                  g_Menu->renderContext, x0, y, scale);
                }
                count = func_8002675C(g_Menu->resources, i + 0x162,
                                      ((MenuCardPrimBlock*)((u8*)g_Menu->menuUnk2 + base))->glyphs,
                                      g_Menu->renderContext, x1, y, scale);
                rendered[i] = 1;
            }
            // rendered[i] is set only by the branch above, which is what defines count.
            if (rendered[i]) {
                j = 0;
                if (count > 0) {
                    slotOffset = base;
                    do {
                        AddPrim(&g_Menu->pGfxEnv->ot[4],
                                &((MenuCardPrimBlock*)((u8*)g_Menu->menuUnk2 + slotOffset))->glyphs[2 * j + g_Menu->renderContext]);
                    } while (++j < count);
                }
                AddPrim(&g_Menu->pGfxEnv->ot[4],
                        &((MenuCardPrimBlock*)((u8*)g_Menu->menuUnk2 + base))->cursors[g_Menu->renderContext]);
            }
            base += sizeof(MenuCardPrimBlock);
            x1 += 0x90;
            x0 += 0x90;
        }
    }
}


// Partial view of the per-character-cell prim block g_Menu->pCellPrimBlocks points at
// (allocated in misc11's MenuCreateCellPrimBlocks): the two line prims, one per render
// context each, and the source vertices the lines are built from.
void MenuDrawCardSlotCellLines(void) {
    MenuCellPrimBlock* pBlock;
    long interpolated;
    long flag;
    u8 character;
    u8 valid;
    s32 i;

    if (g_Menu->cardSlotState == 0) {
        return;
    }
    character = g_Menu->menuUnk2->slotRecordIndexes[g_MenuCardSlotOrder[g_Menu->menuUnk2->selectedSlot]];
    for (i = 0; i < 0x20; i++) {
        pBlock = (MenuCellPrimBlock*)g_Menu->pCellPrimBlocks[i];
        if (!g_Menu->menuUnk2->cardStatus[i / 16]) {
            continue;
        }
        if (i / 16 * 16 == i - 15) {
            continue;
        }
        valid = 0;
        if (character == g_Menu->menuUnk2->slotRecordIndexes[i] && g_Menu->cardSlotState == 2) {
            valid = 1;
            if (character == 0xFF) {
                valid = i == g_MenuCardSlotOrder[g_Menu->menuUnk2->selectedSlot];
            }
        }
        // The colour stores reach the line prims by summing the render context first:
        // `pBlock->lines[0][renderContext].r0` is the same address but gcc then emits
        // the commuted `addu v0,s0,v0` where retail has `addu v0,v0,s0` (all twelve
        // stores, 99.58 with the struct spelling).
        if (valid) {
            ((LINE_F3*)(g_Menu->renderContext * sizeof(LINE_F3) + (u32)pBlock + 0x50))->r0 = 0xFF;
            ((LINE_F3*)(g_Menu->renderContext * sizeof(LINE_F3) + (u32)pBlock + 0x50))->g0 = 0;
            ((LINE_F3*)(g_Menu->renderContext * sizeof(LINE_F3) + (u32)pBlock + 0x50))->b0 = 0;
            ((LINE_F3*)(g_Menu->renderContext * sizeof(LINE_F3) + (u32)pBlock + 0x80))->r0 = 0xFF;
            ((LINE_F3*)(g_Menu->renderContext * sizeof(LINE_F3) + (u32)pBlock + 0x80))->g0 = 0;
            ((LINE_F3*)(g_Menu->renderContext * sizeof(LINE_F3) + (u32)pBlock + 0x80))->b0 = 0;
        } else {
            ((LINE_F3*)(g_Menu->renderContext * sizeof(LINE_F3) + (u32)pBlock + 0x50))->r0 = 0;
            ((LINE_F3*)(g_Menu->renderContext * sizeof(LINE_F3) + (u32)pBlock + 0x50))->g0 = 0xFF;
            ((LINE_F3*)(g_Menu->renderContext * sizeof(LINE_F3) + (u32)pBlock + 0x50))->b0 = 0;
            ((LINE_F3*)(g_Menu->renderContext * sizeof(LINE_F3) + (u32)pBlock + 0x80))->r0 = 0;
            ((LINE_F3*)(g_Menu->renderContext * sizeof(LINE_F3) + (u32)pBlock + 0x80))->g0 = 0xFF;
            ((LINE_F3*)(g_Menu->renderContext * sizeof(LINE_F3) + (u32)pBlock + 0x80))->b0 = 0;
        }
        RotTransPers3(&pBlock->lineVerts[0][0], &pBlock->lineVerts[0][1], &pBlock->lineVerts[0][3],
                      (long*)&pBlock->lines[0][g_Menu->renderContext].x0,
                      (long*)&pBlock->lines[0][g_Menu->renderContext].x1,
                      (long*)&pBlock->lines[0][g_Menu->renderContext].x2, &interpolated, &flag);
        AddPrim(&g_Menu->pGfxEnv->ot[4], &pBlock->lines[0][g_Menu->renderContext]);
        RotTransPers3(&pBlock->lineVerts[1][0], &pBlock->lineVerts[1][2], &pBlock->lineVerts[1][3],
                      (long*)&pBlock->lines[1][g_Menu->renderContext].x0,
                      (long*)&pBlock->lines[1][g_Menu->renderContext].x1,
                      (long*)&pBlock->lines[1][g_Menu->renderContext].x2, &interpolated, &flag);
        AddPrim(&g_Menu->pGfxEnv->ot[4], &pBlock->lines[1][g_Menu->renderContext]);
    }
}
// Cell grid panel block (g_Menu->pCellGrid), allocated in MenuSetCellGridPanelState: the quad
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
    MenuBuildCellGlyphQuads(0, 0, 0x10);
    MenuBuildCellGlyphQuads(0x10, 0x10, 0x20);
    MenuDrawCardSlotCellLines();
    MenuRenderCardSlotPrims();
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
// One party slot's prim context in the scratch prim buffer g_Menu->pPrimBuffer
// (allocated in misc11's MenuAllocatePrimBuffer): the portrait quad, the slot label,
// the six three-digit stat strings, the highlight quad, then the counts and render
// contexts those strings were packed with.  The prims of one string hold two per
// glyph, one per render context, which is what the [glyph][context] shapes are.
typedef struct {
    /* 0x000 */ POLY_FT4 portrait[2]; // Portrait quad, one prim per render context
    /* 0x050 */ POLY_FT4 labelGlyphs[9][2];
    /* 0x320 */ POLY_FT4 statGlyphs0[3][2];
    /* 0x410 */ POLY_FT4 statGlyphs1[3][2];
    /* 0x500 */ POLY_FT4 statGlyphs2[3][2];
    /* 0x5F0 */ POLY_FT4 statGlyphs3[3][2];
    /* 0x6E0 */ POLY_FT4 statGlyphs4[2][2];
    /* 0x780 */ POLY_FT4 statGlyphs5[2][2];
    /* 0x820 */ POLY_FT4 highlight[2];
    /* 0x870 */ u8 statCount0;
    /* 0x871 */ u8 statCount1;
    /* 0x872 */ u8 statCount2;
    /* 0x873 */ u8 statCount3;
    /* 0x874 */ u8 statCount4;
    /* 0x875 */ u8 statCount5;
    /* 0x876 */ u_char labelRenderContext;
    /* 0x877 */ u_char renderContext;
    /* 0x878 */ u_char enabled; // Packed by MenuDrawCharacterSlots, 0 = the slot stays blank
    /* 0x879 */ u_char highlightRenderContext;
    /* 0x87A */ u_char labelGlyphCount;
    /* 0x87B */ u8 pad87B;
} MenuSlotPrimBlock; // Size: 0x87C

// The scratch prim buffer g_Menu->pPrimBuffer, as far as this unit draws from it.
typedef struct {
    /* 0x0000 */ POLY_FT4 glyphQuads[MENU_GLYPH_CELL_COUNT][2]; // Menu glyph cells, one prim per render context
    /* 0x0A00 */ POLY_FT4 slotHighlightQuads[2]; // Quad behind a party slot, one per render context
    /* 0x0A50 */ POLY_G4 slotHighlightShades[2]; // Dim quad drawn behind the highlight
    /* 0x0A98 */ MenuSlotPrimBlock slots[MAX_PARTY_MEMBERS];
    /* 0x240C */ POLY_FT4 playTimeGlyphs[11][2];
    /* 0x277C */ POLY_FT4 iconQuads[MENU_ICON_COUNT][2];
    /* 0x2C7C */ POLY_FT4 unk2C7C[4][2];
    /* 0x2DBC */ u8 slotPrimsBuilt; // Set by misc12's MenuUpdateCharacterWindow once the slots were packed
} MenuPrimBuffer; // Size: 0x2DC0
// Draws the character window's prims: without a packed slot list the menu glyph cells
// and the selected character's portrait quad, with one the per-slot portrait, label and
// stat strings plus the slot highlight, then the dim quad behind them.  Kicked off by
// the character window's own prim add (misc5), so it only runs while that window is up.
// Every use re-reads g_Menu->pPrimBuffer: the calls above can change it, so a pointer
// kept in a local would emit retail's registers differently.  The portrait quad is
// reached index-first (`renderContext + quads`): the `quads[renderContext]` spelling
// gives gcc the commuted `addu` retail does not have (98.48 with the subscript form).
void MenuAddCharacterSlotPrims(void) {
    s32 i;

    if (!g_Menu->pManager->characterWindowBuilt) {
        return;
    }
    if (((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slotPrimsBuilt == 0) {
        MenuRenderString(MENU_GLYPH_CELL_COUNT, ((MenuPrimBuffer*)g_Menu->pPrimBuffer)->glyphQuads[0],
                         g_Menu->renderContext);
        (g_Menu->renderContext + ((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slotHighlightQuads)->u0 =
            g_Menu->menuUnk2->slotRecordIndexes[g_MenuCardSlotOrder[g_Menu->menuUnk2->selectedSlot]] * 16;
        (g_Menu->renderContext + ((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slotHighlightQuads)->v0 =
            g_Menu->menuUnk2->records[g_Menu->menuUnk2->slotRecordIndexes[g_MenuCardSlotOrder[g_Menu->menuUnk2->selectedSlot]]]
                .unk0[g_Menu->characterCellFrame * 4];
        (g_Menu->renderContext + ((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slotHighlightQuads)->u1 =
            g_Menu->menuUnk2->slotRecordIndexes[g_MenuCardSlotOrder[g_Menu->menuUnk2->selectedSlot]] * 16 + 0xF;
        (g_Menu->renderContext + ((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slotHighlightQuads)->v1 =
            g_Menu->menuUnk2->records[g_Menu->menuUnk2->slotRecordIndexes[g_MenuCardSlotOrder[g_Menu->menuUnk2->selectedSlot]]]
                .unk0[g_Menu->characterCellFrame * 4];
        (g_Menu->renderContext + ((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slotHighlightQuads)->u2 =
            g_Menu->menuUnk2->slotRecordIndexes[g_MenuCardSlotOrder[g_Menu->menuUnk2->selectedSlot]] * 16;
        (g_Menu->renderContext + ((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slotHighlightQuads)->v2 =
            g_Menu->menuUnk2->records[g_Menu->menuUnk2->slotRecordIndexes[g_MenuCardSlotOrder[g_Menu->menuUnk2->selectedSlot]]]
                .unk0[g_Menu->characterCellFrame * 4] + 0xF;
        (g_Menu->renderContext + ((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slotHighlightQuads)->u3 =
            g_Menu->menuUnk2->slotRecordIndexes[g_MenuCardSlotOrder[g_Menu->menuUnk2->selectedSlot]] * 16 + 0xF;
        (g_Menu->renderContext + ((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slotHighlightQuads)->v3 =
            g_Menu->menuUnk2->records[g_Menu->menuUnk2->slotRecordIndexes[g_MenuCardSlotOrder[g_Menu->menuUnk2->selectedSlot]]]
                .unk0[g_Menu->characterCellFrame * 4] + 0xF;
        ((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slotHighlightQuads[g_Menu->renderContext].clut =
            GetClut(g_Menu->menuUnk2->slotRecordIndexes[g_MenuCardSlotOrder[g_Menu->menuUnk2->selectedSlot]] * 16,
                    g_Menu->menuUnk2->slotRecordIndexes[g_MenuCardSlotOrder[g_Menu->menuUnk2->selectedSlot]] / 16 + 0x1C1);
        AddPrim(&g_Menu->pGfxEnv->ot[4],
                &((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slotHighlightQuads[g_Menu->renderContext]);
    } else {
        for (i = 0; i < MAX_PARTY_MEMBERS; i++) {
            if (((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slots[i].enabled) {
                MenuRenderString(((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slots[i].labelGlyphCount,
                                 ((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slots[i].labelGlyphs[0],
                                 ((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slots[i].labelRenderContext);
                MenuRenderString(((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slots[i].statCount0,
                                 ((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slots[i].statGlyphs0[0],
                                 ((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slots[i].renderContext);
                MenuRenderString(((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slots[i].statCount1,
                                 ((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slots[i].statGlyphs1[0],
                                 ((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slots[i].renderContext);
                MenuRenderString(((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slots[i].statCount2,
                                 ((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slots[i].statGlyphs2[0],
                                 ((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slots[i].renderContext);
                MenuRenderString(((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slots[i].statCount3,
                                 ((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slots[i].statGlyphs3[0],
                                 ((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slots[i].renderContext);
                MenuRenderString(((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slots[i].statCount4,
                                 ((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slots[i].statGlyphs4[0],
                                 ((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slots[i].renderContext);
                MenuRenderString(((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slots[i].statCount5,
                                 ((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slots[i].statGlyphs5[0],
                                 ((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slots[i].renderContext);
                AddPrim(&g_Menu->pGfxEnv->ot[4],
                        &((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slots[i].portrait[
                            ((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slots[0].renderContext]);
                AddPrim(&g_Menu->pGfxEnv->ot[4],
                        &((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slots[i].highlight[
                            ((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slots[i].highlightRenderContext]);
            }
        }
        MenuRenderString(11, ((MenuPrimBuffer*)g_Menu->pPrimBuffer)->playTimeGlyphs[0],
                         ((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slots[0].renderContext);
        MenuRenderString(4, ((MenuPrimBuffer*)g_Menu->pPrimBuffer)->unk2C7C[0],
                         ((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slots[0].renderContext);
        MenuRenderString(MENU_ICON_COUNT, ((MenuPrimBuffer*)g_Menu->pPrimBuffer)->iconQuads[0],
                         ((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slots[0].renderContext);
    }
    AddPrim(&g_Menu->pGfxEnv->ot[4],
            &((MenuPrimBuffer*)g_Menu->pPrimBuffer)->slotHighlightShades[g_Menu->renderContext]);
}
void MenuRenderQuad(SVECTOR* pVertices, POLY_FT4* pPolys, s32 polyIndex, s32 otIndex) {
    long interpolated;
    long flag;

    RotTransPers4(&pVertices[0], &pVertices[1], &pVertices[2], &pVertices[3],
                  (long*)&pPolys[polyIndex].x0, (long*)&pPolys[polyIndex].x1,
                  (long*)&pPolys[polyIndex].x2, (long*)&pPolys[polyIndex].x3,
                  &interpolated, &flag);
    AddPrim(&g_Menu->pGfxEnv->ot[otIndex], &pPolys[polyIndex]);
}
